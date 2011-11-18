/*                        C L I E N T . C
 * BRL-CAD
 *
 * Copyright (c) 2006-2011 United States Government as represented by
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
/** @file libpkg/example/client.c
 *
 * Relatively simple example file transfer program using libpkg,
 * written in a ttcp style.
 *
 */

#include "common.h"

/* system headers */
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include "bio.h"

/* interface headers */
#include "bu.h"
#include "pkg.h"
#include "ntp.h"

/* used by the client to pass the dbip and opened transfer file
 * descriptor.
 */
typedef struct _my_data_ {
    struct pkg_conn *connection;
    const char *server;
    int port;
} my_data;


/**
 * print a usage statement when invoked with bad, help, or no arguments
 */
static void
usage(const char *msg, const char *argv0)
{
    if (msg) {
	bu_log("%s\n", msg);
    }
    bu_log("Client Usage: %s [-p#] [host] file\n\t-p#\tport number to send to (default 2000)\n\thost\thostname or IP address of receiving server\n\tfile\tsome file to transfer\n", argv0 ? argv0 : MAGIC_ID);

    bu_log("\n%s", pkg_version());

    exit(1);
}

/**
 * simple "valid" port number check used by client and server
 */
static void
validate_port(int port) {
    if (port < 0 || port > 0xffff) {
        bu_bomb("Invalid negative port range\n");
    }
}

/**
 * start up a client that connects to the given server, and sends
 * serialized file data.
 */
void
run_client(const char *server, int port, const char *file)
{
    my_data stash;
    char s_port[MAX_DIGITS + 1] = {0};
    long bytes = 0;
    FILE *fp = (FILE *)NULL;
    static const unsigned int TPKG_BUFSIZE = 2048;
    char *buffer;

    buffer = (char *)bu_calloc(TPKG_BUFSIZE, 1, "buffer allocation");

    /* make sure the file can be opened */
    fp = fopen(file, "rb");
    if (fp == NULL) {
	bu_log("Unable to open %s\n", file);
	bu_bomb("Unable to read file\n");
    }

    /* open a connection to the server */
    validate_port(port);

    snprintf(s_port, MAX_DIGITS, "%d", port);
    stash.connection = pkg_open(server, s_port, "tcp", NULL, NULL, NULL, NULL);
    if (stash.connection == PKC_ERROR) {
	bu_log("Connection to %s, port %d, failed.\n", server, port);
	bu_bomb("ERROR: Unable to open a connection to the server\n");
    }
    stash.server = server;
    stash.port = port;

    /* let the server know we're cool.  also, send the database title
     * along with the MAGIC ident just because we can.
     */
    bytes = pkg_send(MSG_HELO, MAGIC_ID, strlen(MAGIC_ID) + 1, stash.connection);
    if (bytes < 0) {
	pkg_close(stash.connection);
	bu_log("Connection to %s, port %d, seems faulty.\n", server, port);
	bu_bomb("ERROR: Unable to communicate with the server\n");
    }

    /* send the file data to the server */
    while (!feof(fp) && !ferror(fp)) {
	bytes = fread(buffer, 1, 2048, fp);
	bu_log("Read %ld bytes from %s\n", bytes, file);

	if (bytes > 0) {
	    bytes = pkg_send(MSG_DATA, buffer, (size_t)bytes, stash.connection);
	    if (bytes < 0) {
		pkg_close(stash.connection);
		bu_log("Unable to successfully send data to %s, port %d.\n", stash.server, stash.port);
		bu_free(buffer, "buffer release");
		return;
	    }
	}
    }

    /* Wait for the server to tell us it's done */
    buffer = pkg_bwaitfor (MSG_CIAO, stash.connection);

    /* let the server know we're done.  not necessary, but polite. */
    bytes = pkg_send(MSG_CIAO, "BYE", 4, stash.connection);
    if (bytes < 0) {
	bu_log("Unable to cleanly disconnect from %s, port %d.\n", server, port);
    }

    /* flush output and close */
    pkg_close(stash.connection);
    fclose(fp);
    bu_free(buffer, "buffer release");

    return;
}


/**
 * main application for both the client and server
 */
int
main(int argc, char *argv[]) {
    const char * const argv0 = argv[0];
    int c;
    int port = 2000;

    /* client stuff */
    const char *server_name = NULL;
    const char *file = NULL;

    /* process the command-line arguments after the application name */
    while ((c = bu_getopt(argc, argv, "p:P:hH")) != -1) {
	switch (c) {
	    case 'p':
	    case 'P':
		port = atoi(bu_optarg);
		break;
	    case 'h':
	    case 'H':
		/* help */
		usage(NULL, argv0);
		break;
	    default:
		usage("ERROR: Unknown argument\n", argv0);
	}
    }

    argc -= bu_optind;
    argv += bu_optind;


    /* prep up the client */
    if (argc < 1) {
	usage("ERROR: Missing file argument\n", argv0);
    } else if (argc > 2) {
	usage("ERROR: Too many arguments provided\n", argv0);
    }

    if (argc < 2) {
	server_name = "127.0.0.1";
    } else {
	server_name = *argv++;
    }
    file = *argv++;

    /* make sure the file exists */
    if (!bu_file_exists(file)) {
	bu_log("File does not exist: %s\n", file);
	return 1;
    }

    /* fire up the client */
    bu_log("Connecting to %s, port %d\n", server_name, port);
    run_client(server_name, port, file);

    return 0;
}

/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
