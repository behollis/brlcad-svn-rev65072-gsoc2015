/*                     B A C K T R A C E . C
 * BRL-CAD
 *
 * Copyright (c) 2007 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */
/** @addtogroup bu_log */
/** @{ */
/** @file backtrace.c
 *
 * Extract a backtrace of the current call stack.
 *
 * Author -
 *   Christopher Sean Morrison
 *
 * Source -
 *   BRL-CAD Open Source
 */

#include "common.h"

/* system headers */
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_SYS_TIME_H
#   include <sys/time.h>
#endif

#ifdef HAVE_SYS_TIMES_H
#   include <sys/times.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#   include <sys/types.h>
#endif

#ifdef HAVE_TIME_H
#   include <time.h>
#endif

#ifdef HAVE_UNISTD_H
#   include <unistd.h>
#endif

#ifdef HAVE_FCNTL_H
#   include <fcntl.h>
#endif

#ifdef HAVE_PROCESS_H
#   include <process.h>
#endif

#ifdef HAVE_SYS_SELECT_H
#   include <sys/select.h>
#endif

#ifdef HAVE_STRING_H
#   include <string.h>
#endif

/* common headers */
#include "bu.h"


/* so we don't have to worry as much about stack stomping */
#define BT_BUFSIZE 4096
static char buffer[BT_BUFSIZE] = {0};

static pid_t pid = (pid_t)0;
static int backtrace_done = 0;
static int interrupt_wait = 0;

/* avoid stack variables for backtrace() */
static int input[2] = {0,0};
static int output[2] = {0,0};
static fd_set fdset;
static fd_set readset;
static struct timeval tv;
static int result;
static int position;
static int processing_bt;
static char c = 0;
static int warned;

/* avoid stack variables for bu_backtrace() */
static char *args[4] = { NULL, NULL, NULL, NULL };
static const char *locate_gdb = NULL;


/* SIGCHLD handler for backtrace() */
static void
backtrace_sigchld(int signum)
{
    backtrace_done = 1;
    interrupt_wait = 1;
}

/* SIGINT handler for bu_backtrace() */
static void
backtrace_sigint(int signum)
{
    interrupt_wait = 1;
}


/* actual guts to bu_backtrace() used to invoke gdb and parse out the
 * backtrace from gdb's output.
 */
static void
backtrace(char **args, int fd)
{
    /* receiving a SIGCHLD signal indicates something happened to a
     * child process, which should be this backtrace since it is
     * invoked after a fork() call as the child.
     */
#ifdef SIGCHLD
    signal(SIGCHLD, backtrace_sigchld);
#endif
#ifdef SIGINT
    signal(SIGINT, backtrace_sigint);
#endif

    if ((pipe(input) == -1) || (pipe(output) == -1)) {
	perror("unable to open pipe");
	fflush(stderr);
	exit(1); /* can't call bu_bomb() */
    }

    pid = fork();
    if (pid == 0) {
	close(0); dup(input[0]); /* set the stdin to the in pipe */
	close(1); dup(output[1]); /* set the stdout to the out pipe */
	close(2); dup(output[1]); /* set the stderr to the out pipe */

	execvp(args[0], args); /* invoke debugger */
	perror("exec failed");
	fflush(stderr);
	exit(1); /* can't call bu_bomb() */
    } else if (pid == (pid_t) -1) {
	perror("unable to fork");
	fflush(stderr);
	exit(1); /* can't call bu_bomb() */
    }

    FD_ZERO(&fdset);
    FD_SET(output[0], &fdset);

    write(input[1], "set prompt\n", 12);
    write(input[1], "set confirm off\n", 16);
    write(input[1], "set backtrace past-main on\n", 27);
    write(input[1], "bt full\n", 8);
    /* can add additional gdb commands here.  output will contain
     * everything up to the "Detaching from process" statement from
     * quit.
     */
    write(input[1], "quit\n", 5);

    position = 0;
    processing_bt = 0;
    memset(buffer, 0, BT_BUFSIZE);

    /* get/print the trace */
    warned = 0;
    while (1) {
	readset = fdset;

	tv.tv_sec = 0;
	tv.tv_usec = 42;

	result = select(FD_SETSIZE, &readset, NULL, NULL, &tv);
	if (result == -1) {
	    break;
	}

	if ((result > 0) && (FD_ISSET(output[0], &readset))) {
	    if (read(output[0], &c, 1)) {
		switch (c) {
		    case '\n':
			if (bu_debug & BU_DEBUG_BACKTRACE) {
			    bu_log("BACKTRACE DEBUG: [%s]\n", buffer);
			}
			if (position+1 < BT_BUFSIZE) {
			    buffer[position++] = c;
			    buffer[position] = '\0';
			} else {
			    position++;
			}
			if (strncmp(buffer, "No locals", 9) == 0) {
			    /* skip it */
			} else if (strncmp(buffer, "No symbol table", 15) == 0) {
			    /* skip it */
			} else if (strncmp(buffer, "Detaching", 9) == 0) {
			    /* done processing backtrace output */
			    processing_bt = 0;
			} else if (processing_bt == 1) {
			    if (write(fd, buffer, strlen(buffer)) != strlen(buffer)) {
				perror("error writing stack to file");
				break;
			    }
			    if (position > BT_BUFSIZE) {
				if (write(fd, " [TRIMMED]\n", 11) != 11) {
				    perror("error writing trim message to file");
				    break;
				}
			    }				
			}
			position = 0;
			continue;
		    case '#':
			/* once we find a # on the beginning of a
			 * line, begin keeping track of the output.
			 * the first #0 backtrace frame (i.e. that for
			 * the bu_backtrace() call) is not included in
			 * the output intentionally (because of the
			 * gdb prompt).
			 */
			if (position == 0) {
			    processing_bt = 1;
			}
			break;
		    default:
			break;
		}
		if (position+1 < BT_BUFSIZE) {
		    buffer[position++] = c;
		    buffer[position] = '\0';
		} else {
		    if (!warned && (bu_debug & BU_DEBUG_ATTACH)) {
			bu_log("Warning: debugger output overflow\n");
			warned = 1;
		    }
		    position++;
		}
	    }
	} else if (backtrace_done) {
	    break;
	}
    }

    fflush(stdout);
    fflush(stderr);

    close(input[0]);
    close(input[1]);
    close(output[0]);
    close(output[1]);

    if (bu_debug & BU_DEBUG_ATTACH) {
	bu_log("\nBacktrace complete.\nAttach debugger or interrupt to continue...\n");
    } else {
#  ifdef HAVE_KILL
	/* not attaching, so let the parent continue */
#    ifdef SIGINT
	kill(getppid(), SIGINT);
#    endif
#    ifdef SIGCHLD
	kill(getppid(), SIGCHLD);
#    endif
#  endif
	sleep(2);
    }

    exit(0);
}


/**
 * b u _ b a c k t r a c e
 *
 * this routine provides a trace of the call stack to the caller,
 * generally called either directly, via a signal handler, or through
 * bu_bomb() with the appropriate bu_debug flags set.
 *
 * the routine waits indefinitely (in a spin loop) until a signal
 * (SIGINT) is received, at which point execution continues, or until
 * some other signal is received that terminates the application.
 *
 * the stack backtrace will be written to the provided 'fp' file
 * pointer.  it's the caller's responsibility to open and close
 * that pointer if necessary.  If 'fp' is NULL, stdout will be used.
 *
 * returns truthfully if a backtrace was attempted.
 */
int
bu_backtrace(FILE *fp)
{
    if (!fp) {
	fp = stdout;
    }

    /* make sure the debugger exists */
    if ((locate_gdb = bu_which("gdb"))) {
	args[0] = bu_strdup(locate_gdb);
	if (bu_debug & BU_DEBUG_BACKTRACE) {
	    bu_log("Found gdb in USER path: %s\n", locate_gdb);
	}
    } else if ((locate_gdb = bu_whereis("gdb"))) {
	args[0] = bu_strdup(locate_gdb);
	if (bu_debug & BU_DEBUG_BACKTRACE) {
	    bu_log("Found gdb in SYSTEM path: %s\n", locate_gdb);
	}
    } else {
	if (bu_debug & BU_DEBUG_BACKTRACE) {
	    bu_log("gdb was NOT found, no backtrace available\n");
	}
	return 0;
    }
    locate_gdb = NULL;

#ifdef SIGINT
    signal(SIGINT, backtrace_sigint);
#endif

    snprintf(buffer, BT_BUFSIZE, "%d", bu_process_id());

    args[1] = (char*) bu_argv0();
    args[2] = buffer;

    if (bu_debug & BU_DEBUG_BACKTRACE) {
	bu_log("CALL STACK BACKTRACE REQUESTED\n");
	bu_log("Invoking Debugger: %s %s %s\n\n", args[0], args[1], args[2]);
    }

    /* fork so that trace symbols stop _here_ instead of in some libc
     * routine (e.g., in wait(2)).
     */
    pid = fork();
    if (pid == 0) {
	/* child */
	backtrace(args, fileno(fp));
	bu_free(args[0], "gdb strdup");
	args[0] = NULL;
	exit(0);
    } else if (pid == (pid_t) -1) {
	/* failure */
	bu_free(args[0], "gdb strdup");
	args[0] = NULL;
	perror("unable to fork for gdb");
	return 0;
    }
    /* parent */
    if (args[0]) {
	bu_free(args[0], "gdb strdup");
	args[0] = NULL;
    }
    fflush(fp);

    /* could probably do something better than this to avoid hanging
     * indefinitely.  keeps the trace clean, though, and allows for a
     * debugger to be attached interactively if needed.
     */
    interrupt_wait = 0;
#ifdef HAVE_KILL
    {
	struct timeval start, end;
	gettimeofday(&start, NULL);
	gettimeofday(&end, NULL);
	while ((interrupt_wait == 0) && (end.tv_sec - start.tv_sec < 60)) {
	    /* do nothing, wait for debugger to attach but don't wait too long */;
	    gettimeofday(&end, NULL);
	    sleep(1);
	}
    }
#else
    /* FIXME: need something better here for win32 */
    sleep(10);
#endif

    if (bu_debug & BU_DEBUG_BACKTRACE) {
	bu_log("\nContinuing.\n");
    }

#ifdef SIGINT
    signal(SIGINT, SIG_DFL);
#endif
#ifdef SIGCHLD
    signal(SIGCHLD, SIG_DFL);
#endif

    fflush(fp);

    return 1;
}


#ifdef TEST_BACKTRACE
int bar(char **argv)
{
    int moo = 5;
    bu_backtrace(NULL);
    return 0;
}


int foo(char **argv)
{
    return bar(argv);
}


int
main(int argc, char *argv[])
{
    if (argc > 1) {
	bu_bomb("this is a test\n");
    } else {
	(void)foo(argv);
    }
    return 0;
}
#endif  /* TEST_BACKTRACE */

/** @} */

/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
