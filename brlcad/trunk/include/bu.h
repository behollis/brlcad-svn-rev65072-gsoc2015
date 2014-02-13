/*                            B U . H
 * BRL-CAD
 *
 * Copyright (c) 2004-2014 United States Government as represented by
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

/** @file bu.h
 *
 * Main header file for the BRL-CAD Utility Library, LIBBU.
 *
 * The two letters "BU" stand for "BRL-CAD" and "Utility".  This
 * library provides several layers of low-level utility routines,
 * providing features that make cross-platform coding easier.
 *
 * Parallel processing support:  threads, semaphores, parallel-malloc.
 * Consolidated logging support:  bu_log(), bu_exit(), and bu_bomb().
 *
 * The intention is that these routines are general extensions to the
 * data types offered by the C language itself, and to the basic C
 * runtime support provided by the system LIBC.  All routines in LIBBU
 * are designed to be "parallel-safe" (sometimes called "mp-safe" or
 * "thread-safe" if parallelism is via threading) to greatly ease code
 * development for multiprocessor systems.
 *
 * All of the data types provided by this library are defined in bu.h
 * or appropriate included files from the ./bu subdirectory; none of
 * the routines in this library will depend on data types defined in
 * other BRL-CAD header files, such as vmath.h.  Look for those
 * routines in LIBBN.
 *
 * All truly fatal errors detected by the library use bu_bomb() to
 * exit with a status of 12.  The LIBBU variants of system calls
 * (e.g., bu_malloc()) do not return to the caller (unless there's a
 * bomb hook defined) unless they succeed, thus sparing the programmer
 * from constantly having to check for NULL return codes.
 *
 */
#ifndef BU_H
#define BU_H

#include "common.h"

#include <stdlib.h>
#include <sys/types.h>
#include <stdarg.h>

__BEGIN_DECLS

#include "./bu/defines.h"



/* system interface headers */
#include <setjmp.h> /* for bu_setjmp */
#include <stddef.h> /* for size_t */
#include <limits.h> /* for CHAR_BIT */

#ifdef HAVE_STDINT_H
#  include <stdint.h> /* for [u]int[16|32|64]_t */
#endif

#ifdef HAVE_DLFCN_H
#  include <dlfcn.h>	/* for RTLD_* */
#endif

/* common interface headers */
#include "tcl.h"	/* Included for Tcl_Interp definition */
#include "bu/magic.h"

/* FIXME Temporary global interp.  Remove me.  */
BU_EXPORT extern Tcl_Interp *brlcad_interp;


/** @file libbu/vers.c
 *
 * version information about LIBBU
 *
 */

/**
 * returns the compile-time version of libbu
 */
BU_EXPORT extern const char *bu_version(void);

/**
 * MAX_PSW - The maximum number of processors that can be expected on
 * this hardware.  Used to allocate application-specific per-processor
 * tables at compile-time and represent a hard limit on the number of
 * processors/threads that may be spawned. The actual number of
 * available processors is found at runtime by calling bu_avail_cpus()
 */
#define MAX_PSW 1024


/*----------------------------------------------------------------------*/

#include "./bu/cv.h"

/*----------------------------------------------------------------------*/


/** @file libbu/endian.c
 *
 * Run-time byte order detection.
 *
 */

typedef enum {
    BU_LITTLE_ENDIAN = 1234, /* LSB first: i386, VAX order */
    BU_BIG_ENDIAN    = 4321, /* MSB first: 68000, IBM, network order */
    BU_PDP_ENDIAN    = 3412  /* LSB first in word, MSW first in long */
} bu_endian_t;


/**
 * returns the platform byte ordering (e.g., big-/little-endian)
 */
BU_EXPORT extern bu_endian_t bu_byteorder(void);


/*----------------------------------------------------------------------*/

#include "./bu/list.h"

/*----------------------------------------------------------------------*/


/**
 * fastf_t - Intended to be the fastest floating point data type on
 * the current machine, with at least 64 bits of precision.  On 16 and
 * 32 bit machines, this is typically "double", but on 64 bit machines,
 * it is often "float".  Virtually all floating point variables (and
 * more complicated data types, like vect_t and mat_t) are defined as
 * fastf_t.  The one exception is when a subroutine return is a
 * floating point value; that is always declared as "double".
 *
 * TODO: If used pervasively, it should eventually be possible to make
 * fastf_t a GMP C++ type for fixed-precision computations.
 */
typedef double fastf_t;

/**
 * Definitions about limits of floating point representation
 * Eventually, should be tied to type of hardware (IEEE, IBM, Cray)
 * used to implement the fastf_t type.
 *
 * MAX_FASTF - Very close to the largest value that can be held by a
 * fastf_t without overflow.  Typically specified as an integer power
 * of ten, to make the value easy to spot when printed.  TODO: macro
 * function syntax instead of constant (DEPRECATED)
 *
 * SQRT_MAX_FASTF - sqrt(MAX_FASTF), or slightly smaller.  Any number
 * larger than this, if squared, can be expected to * produce an
 * overflow.  TODO: macro function syntax instead of constant
 * (DEPRECATED)
 *
 * SMALL_FASTF - Very close to the smallest value that can be
 * represented while still being greater than zero.  Any number
 * smaller than this (and non-negative) can be considered to be
 * zero; dividing by such a number can be expected to produce a
 * divide-by-zero error.  All divisors should be checked against
 * this value before actual division is performed.  TODO: macro
 * function syntax instead of constant (DEPRECATED)
 *
 * SQRT_SMALL_FASTF - sqrt(SMALL_FASTF), or slightly larger.  The
 * value of this is quite a lot larger than that of SMALL_FASTF.  Any
 * number smaller than this, when squared, can be expected to produce
 * a zero result.  TODO: macro function syntax instead of constant
 * (DEPRECATED)
 *
 */
#if defined(vax)
/* DEC VAX "D" format, the most restrictive */
#  define MAX_FASTF		1.0e37	/* Very close to the largest number */
#  define SQRT_MAX_FASTF	1.0e18	/* This squared just avoids overflow */
#  define SMALL_FASTF		1.0e-37	/* Anything smaller is zero */
#  define SQRT_SMALL_FASTF	1.0e-18	/* This squared gives zero */
#else
/* IBM format, being the next most restrictive format */
#  define MAX_FASTF		1.0e73	/* Very close to the largest number */
#  define SQRT_MAX_FASTF	1.0e36	/* This squared just avoids overflow */
#  define SMALL_FASTF		1.0e-77	/* Anything smaller is zero */
#  if defined(aux)
#    define SQRT_SMALL_FASTF	1.0e-40 /* _doprnt error in libc */
#  else
#    define SQRT_SMALL_FASTF	1.0e-39	/* This squared gives zero */
#  endif
#endif

/** DEPRECATED, do not use */
#define SMALL SQRT_SMALL_FASTF


/**
 * It is necessary to have a representation of 1.0/0.0 or log(0),
 * i.e., "infinity" that fits within the dynamic range of the machine
 * being used.  This constant places an upper bound on the size object
 * which can be represented in the model.  With IEEE 754 floating
 * point, this may print as 'inf' and is represented with all 1 bits
 * in the biased-exponent field and all 0 bits in the fraction with
 * the sign indicating positive (0) or negative (1) infinity.
 */
#ifndef INFINITY
#  if defined(HUGE_VAL)
#    define INFINITY ((fastf_t)HUGE_VAL)
#  elif defined(HUGE_VALF)
#    define INFINITY ((fastf_t)HUGE_VALF)
#  elif defined(HUGE)
#    define INFINITY ((fastf_t)HUGE)
#  elif defined(MAXDOUBLE)
#    define INFINITY ((fastf_t)MAXDOUBLE)
#  elif defined(MAXFLOAT)
#    define INFINITY ((fastf_t)MAXFLOAT)
#  else
     /* all else fails, just pick something big slightly over 32-bit
      * single-precision floating point that has worked well before.
      */
#    define INFINITY ((fastf_t)1.0e40)
#  endif
#endif


/*----------------------------------------------------------------------*/

#include "./bu/bitv.h"

/*----------------------------------------------------------------------*/


/** @addtogroup hist */
/** @ingroup data */
/** @{ */
/** @file libbu/hist.c
 *
 * General purpose histogram handling routines.
 *
 * The subroutine bu_hist_range() is used to record items that may
 * extend across multiple "bin"s.
 *
 */

/**
 * histogram support
 */
struct bu_hist  {
    uint32_t magic;		/**< magic # for id/check */
    fastf_t hg_min;		/**< minimum value */
    fastf_t hg_max;		/**< maximum value */
    fastf_t hg_clumpsize;	/**< (max-min+1)/nbins+1 */
    size_t hg_nsamples;		/**< total number of samples spread into histogram */
    size_t hg_nbins;		/**< # of bins in hg_bins[]  */
    long *hg_bins;		/**< array of counters */
};
typedef struct bu_hist bu_hist_t;
#define BU_HIST_NULL ((struct bu_hist *)0)

/**
 * assert the integrity of a bu_hist struct.
 */
#define BU_CK_HIST(_p) BU_CKMAG(_p, BU_HIST_MAGIC, "struct bu_hist")

/**
 * initialize a bu_hist struct without allocating any memory.
 */
#define BU_HIST_INIT(_hp) { \
	(_hp)->magic = BU_HIST_MAGIC; \
	(_hp)->hg_min = (_hp)->hg_max = (_hp)->hg_clumpsize = 0.0; \
	(_hp)->hg_nsamples = (_hp)->hg_nbins = 0; \
	(_hp)->hg_bins = NULL; \
    }

/**
 * macro suitable for declaration statement initialization of a
 * bu_hist struct.  does not allocate memory.
 */
#define BU_HIST_INIT_ZERO {BU_HIST_MAGIC, 0.0, 0.0, 0.0, 0, 0, NULL}

/**
 * returns truthfully whether a bu_hist has been initialized via
 * BU_HIST_INIT() or BU_HIST_INIT_ZERO.
 */
#define BU_HIST_IS_INITIALIZED(_hp) (((struct bu_hist *)(_hp) != BU_HIST_NULL) && LIKELY((_hp)->magic == BU_HIST_MAGIC))

#define BU_HIST_TALLY(_hp, _val) { \
	if ((_val) <= (_hp)->hg_min) { \
	    (_hp)->hg_bins[0]++; \
	} else if ((_val) >= (_hp)->hg_max) { \
	    (_hp)->hg_bins[(_hp)->hg_nbins]++; \
	} else { \
	    (_hp)->hg_bins[(int)(((_val)-(_hp)->hg_min)/(_hp)->hg_clumpsize)]++; \
	} \
	(_hp)->hg_nsamples++;  }

#define BU_HIST_TALLY_MULTIPLE(_hp, _val, _count) { \
	int __count = (_count); \
	if ((_val) <= (_hp)->hg_min) { \
	    (_hp)->hg_bins[0] += __count; \
	} else if ((_val) >= (_hp)->hg_max) { \
	    (_hp)->hg_bins[(_hp)->hg_nbins] += __count; \
	} else { \
	    (_hp)->hg_bins[(int)(((_val)-(_hp)->hg_min)/(_hp)->hg_clumpsize)] += __count; \
	} \
	(_hp)->hg_nsamples += __count;  }

/** @} */


/*----------------------------------------------------------------------*/

#include "./bu/ptbl.h"

/*----------------------------------------------------------------------*/


/** @addtogroup mf */
/** @ingroup memory */
/** @{ */
/** @file libbu/mappedfile.c
 *
 * Routines for sharing large read-only data files.
 *
 * Routines for sharing large read-only data files like height fields,
 * bit map solids, texture maps, etc.  Uses memory mapped files where
 * available.
 *
 * Each instance of the file has the raw data available as element
 * "buf".  If a particular application needs to transform the raw data
 * in a manner that is identical across all uses of that application
 * (e.g. height fields, EBMs, etc.), then the application should
 * provide a non-null "appl" string, to tag the format of the "apbuf".
 * This will keep different applications from sharing that instance of
 * the file.
 *
 * Thus, if the same filename is opened for interpretation as both an
 * EBM and a height field, they will be assigned different mapped file
 * structures, so that the "apbuf" pointers are distinct.
 *
 */

/**
 * @struct bu_mapped_file bu.h
 *
 * Structure for opening a mapped file.
 *
 * Each file is opened and mapped only once (per application, as
 * tagged by the string in "appl" field).  Subsequent opens require an
 * exact match on both strings.
 *
 * Before allocating apbuf and performing data conversion into it,
 * openers should check to see if the file has already been opened and
 * converted previously.
 *
 * When used in RT, the mapped files are not closed at the end of a
 * frame, so that subsequent frames may take advantage of the large
 * data files having already been read and converted.  Examples
 * include EBMs, texture maps, and height fields.
 *
 * For appl == "db_i", file is a ".g" database & apbuf is (struct db_i *).
 */
struct bu_mapped_file {
    struct bu_list l;
    char *name;		/**< bu_strdup() of file name */
    genptr_t buf;	/**< In-memory copy of file (may be mmapped)  */
    size_t buflen;	/**< # bytes in 'buf'  */
    int is_mapped;	/**< 1=mmap() used, 0=bu_malloc/fread */
    char *appl;		/**< bu_strdup() of tag for application using 'apbuf'  */
    genptr_t apbuf;	/**< opt: application-specific buffer */
    size_t apbuflen;	/**< opt: application-specific buflen */
    time_t modtime;	/**< date stamp, in case file is modified */
    int uses;		/**< # ptrs to this struct handed out */
    int dont_restat;	/**< 1=on subsequent opens, don't re-stat()  */
};
typedef struct bu_mapped_file bu_mapped_file_t;
#define BU_MAPPED_FILE_NULL ((struct bu_mapped_file *)0)

/**
 * assert the integrity of a bu_mapped_file struct.
 */
#define BU_CK_MAPPED_FILE(_mf) BU_CKMAG(_mf, BU_MAPPED_FILE_MAGIC, "bu_mapped_file")

/**
 * initialize a bu_mapped_file struct without allocating any memory.
 */
#define BU_MAPPED_FILE_INIT(_mf) { \
	BU_LIST_INIT_MAGIC(&(_mf)->l, BU_MAPPED_FILE_MAGIC); \
	(_mf)->name = (_mf)->buf = NULL; \
	(_mf)->buflen = (_mf)->is_mapped = 0; \
	(_mf)->appl = (_mf)->apbuf = NULL; \
	(_mf)->apbuflen = (_mf)->modtime = (_mf)->uses = (_mf)->dont_restat = 0; \
    }

/**
 * macro suitable for declaration statement initialization of a
 * bu_mapped_file struct.  does not allocate memory.
 */
#define BU_MAPPED_FILE_INIT_ZERO { {BU_MAPPED_FILE_MAGIC, BU_LIST_NULL, BU_LIST_NULL}, NULL, NULL, 0, 0, NULL, NULL, 0, 0, 0, 0 }

/**
 * returns truthfully whether a bu_mapped_file has been initialized via
 * BU_MAPPED_FILE_INIT() or BU_MAPPED_FILE_INIT_ZERO.
 */
#define BU_MAPPED_FILE_IS_INITIALIZED(_hp) (((struct bu_mapped_file *)(_hp) != BU_MAPPED_FILE_NULL) && LIKELY((_hp)->l.magic == BU_MAPPED_FILE_MAGIC))


/** @} */
/*----------------------------------------------------------------------*/

/** log indentation hook */
typedef int (*bu_hook_t)(genptr_t, genptr_t);

struct bu_hook_list {
    struct bu_list l; /**< linked list */
    bu_hook_t hookfunc; /**< function to call */
    genptr_t clientdata; /**< data for caller */
};
typedef struct bu_hook_list bu_hook_list_t;
#define BU_HOOK_LIST_NULL ((struct bu_hook_list *) 0)

/**
 * assert the integrity of a non-head node bu_hook_list struct.
 */
#define BU_CK_HOOK_LIST(_hl) BU_CKMAG(_hl, BU_HOOK_LIST_MAGIC, "bu_hook_list")

/**
 * initialize a bu_hook_list struct without allocating any memory.
 * this macro is not suitable for initialization of a list head node.
 */
#define BU_HOOK_LIST_INIT(_hl) { \
	BU_LIST_INIT_MAGIC(&(_hl)->l, BU_HOOK_LIST_MAGIC); \
	(_hl)->hookfunc = (_hl)->clientdata = NULL; \
    }

/**
 * macro suitable for declaration statement initialization of a
 * bu_hook_list struct.  does not allocate memory.  not suitable for
 * initialization of a list head node.
 */
#define BU_HOOK_LIST_INIT_ZERO { {BU_HOOK_LIST_MAGIC, BU_LIST_NULL, BU_LIST_NULL}, NULL, NULL }

/**
 * returns truthfully whether a non-head node bu_hook_list has been
 * initialized via BU_HOOK_LIST_INIT() or BU_HOOK_LIST_INIT_ZERO.
 */
#define BU_HOOK_LIST_IS_INITIALIZED(_p) (((struct bu_hook_list *)(_p) != BU_HOOK_LIST_NULL) && LIKELY((_p)->l.magic == BU_HOOK_LIST_MAGIC))

/*----------------------------------------------------------------------*/

#include "./bu/vls.h"

/*----------------------------------------------------------------------*/

#include "./bu/avs.h"


/*----------------------------------------------------------------------*/

#include "./bu/vlb.h"

/*----------------------------------------------------------------------*/

/** @addtogroup debug Debugging */
/** @ingroup io */
/** @{ */

/**
 * controls the libbu debug level
 */
BU_EXPORT extern int bu_debug;

/**
 * Section for BU_DEBUG values
 *
 * These can be set from the command-line of RT-compatible programs
 * using the "-!" option.
 *
 * These definitions are each for one bit.
 */
#define BU_DEBUG_OFF 0	/* No debugging */

#define BU_DEBUG_COREDUMP	0x00000001	/* bu_bomb() should dump core on exit */
#define BU_DEBUG_MEM_CHECK	0x00000002	/* Mem barrier & leak checking */
#define BU_DEBUG_MEM_LOG	0x00000004	/* Print all dynamic memory operations */
#define BU_DEBUG_UNUSED_0	0x00000008	/* unused */

#define BU_DEBUG_PARALLEL	0x00000010	/* Parallel debug logging */
#define BU_DEBUG_MEM_QCHECK	0x00000020	/* Fast mem leak checking (won't work with corruption) */
#define BU_DEBUG_BACKTRACE	0x00000040	/* Log backtrace details during abnormal exit */
#define BU_DEBUG_ATTACH		0x00000080	/* Waits for a debugger to attach during a crash */

#define BU_DEBUG_MATH		0x00000100	/* Fundamental math routines (plane.c, mat.c) */
#define BU_DEBUG_PTBL		0x00000200	/* bu_ptbl_*() logging */
#define BU_DEBUG_AVS		0x00000400	/* bu_avs_*() logging */
#define BU_DEBUG_MAPPED_FILE	0x00000800	/* bu_mapped_file logging */

#define BU_DEBUG_PATHS		0x00001000	/* File and path debug logging */
#define BU_DEBUG_UNUSED_1	0x00002000	/* unused */
#define BU_DEBUG_UNUSED_2	0x00004000	/* unused */
#define BU_DEBUG_UNUSED_3	0x00008000	/* unused */

#define BU_DEBUG_TABDATA	0x00010000	/* LIBBN: tabdata */
#define BU_DEBUG_UNUSED_4	0x00020000	/* unused */
#define BU_DEBUG_UNUSED_5	0x00040000	/* unused */
#define BU_DEBUG_UNUSED_6	0x00080000	/* unused */

/* Format string for bu_printb() */
#define BU_DEBUG_FORMAT	\
    "\020\
\025TABDATA\
\015?\
\014MAPPED_FILE\013AVS\012PTBL\011MATH\010?\7?\6MEM_QCHECK\5PARALLEL\
\4?\3MEM_LOG\2MEM_CHECK\1COREDUMP"

/** @} */
/*----------------------------------------------------------------------*/
/* parse.c */
/** @addtogroup parse */
/** @ingroup container */
/** @{ */
/*
 * Structure parse/print
 *
 * Definitions and data structures needed for routines that assign
 * values to elements of arbitrary data structures, the layout of
 * which is described by tables of "bu_structparse" structures.
 */

/**
 * The general problem of word-addressed hardware where (int *) and
 * (char *) have different representations is handled in the parsing
 * routines that use sp_offset, because of the limitations placed on
 * compile-time initializers.
 *
 * Files using bu_offsetof or bu_offsetofarray will need to include
 * stddef.h in order to get offsetof()
 */
/* FIXME - this is a temporary cast. The bu_structparse sp_offset member
 *         should be a size_t.
 */
#ifndef offsetof
#  define offsetof(_t, _m) (size_t)(&(((_t *)0)->_m))
#endif
#define bu_offsetof(_t, _m) (size_t)offsetof(_t, _m)
#define bu_offsetofarray(_t, _a, _d, _i) bu_offsetof(_t, _a) + sizeof(_d) * _i


/**
 * Convert address of global data object into byte "offset" from
 * address 0.
 *
 * Strictly speaking, the C language only permits initializers of the
 * form: address +- constant, where here the intent is to measure the
 * byte address of the indicated variable.  Matching compensation code
 * for the CRAY is located in librt/parse.c
 */
#if defined(__ia64__) || defined(__x86_64__) || defined(__sparc64__) || defined(_HPUX_SOURCE) || defined(__clang__)
#    define bu_byteoffset(_i)	((size_t)((char *)&(_i)))
#else
/* "Conservative" way of finding # bytes as diff of 2 char ptrs */
#  define bu_byteoffset(_i)	((size_t)(((char *)&(_i))-((char *)0)))
#endif


/**
 * The "bu_structparse" struct describes one element of a structure.
 * Collections of these are combined to describe entire structures (or at
 * least those portions for which parse/print/import/export support is
 * desired.
 *
 * Provides a convenient way of describing a C data structure, and
 * reading and writing it in both human-readable ASCII and efficient
 * binary forms.
 *
 * For example:
 *
 @code

 struct data_structure {
   char a_char;
   char str[32];
   short a_short;
   int a_int;
   fastf_t a_fastf_t;
   double a_double;
 }

 struct data_structure default = { 'c', "the default string", 32767, 1, 1.0, 1.0 };

 struct data_structure my_values;

 struct bu_structparse data_sp[] ={
   {"%c", 1,     "a_char",   bu_offsetof(data_structure, a_char), BU_STRUCTPARSE_FUNC_NULL,                      "a single character", (void*)&default.a_char},
   {"%s", 32,       "str", bu_offsetofarray(data_structure, str), BU_STRUCTPARSE_FUNC_NULL,         "This is a full character string", (void*)default.str},
   {"%i", 1,    "a_short",  bu_offsetof(data_structure, a_short), BU_STRUCTPARSE_FUNC_NULL,                         "A 16bit integer", (void*)&default.a_short},
   {"%d", 1,      "a_int",    bu_offsetof(data_structure, a_int), BU_STRUCTPARSE_FUNC_NULL,                          "A full integer", (void*)&default.a_int},
   {"%f", 1,   "a_fastf_t", bu_offsetof(data_structure, a_fastf_t), BU_STRUCTPARSE_FUNC_NULL, "A variable-precision fasf_t floating point value", (void*)&default.a_fastf_t},
   {"%g", 1,   "a_double", bu_offsetof(data_structure, a_double), BU_STRUCTPARSE_FUNC_NULL, "A double-precision fasf_t floating point value", (void*)&default.a_double},
   {  "", 0, (char *)NULL,                                     0, BU_STRUCTPARSE_FUNC_NULL,                              (char *)NULL, (void *)NULL}
 };

 @endcode
 *
 * To parse a string, call:
 *
 * bu_struct_parse(vls_string, data_sp, (char *)my_values)
 *
 * this will parse the vls string and assign values to the members of
 * the structure my_values
 *
 * A gross hack: To set global variables (or others for that matter)
 * you can store the actual address of the variable in the sp_offset
 * field and pass a null pointer as the last argument to
 * bu_struct_parse.  If you don't understand why this would work, you
 * probably shouldn't use this technique.
 */
struct bu_structparse {
    const char sp_fmt[4];		/**< "%i" or "%f", etc. */
    size_t sp_count;		/**< number of elements */
    const char *sp_name;		/**< Element's symbolic name */
    size_t sp_offset;		/**< Byte offset in struct */
    void (*sp_hook)(const struct bu_structparse *,
		    const char *,
		    void *,
		    const char *);	/**< Optional hooked function, or indir ptr */
    const char *sp_desc;		/**< description of element */
    void *sp_default;		       /**< ptr to default value */
};
typedef struct bu_structparse bu_structparse_t;
#define BU_STRUCTPARSE_NULL ((struct bu_structparse *)0)

#define BU_STRUCTPARSE_FUNC_NULL ((void(*)(const struct bu_structparse *, const char *, void *, const char *))0)

/**
 * assert the integrity of a bu_structparse struct.
 */
#define BU_CK_STRUCTPARSE(_sp) /* nothing to do */

/**
 * initialize a bu_structparse struct without allocating any memory.
 */
#define BU_STRUCTPARSE_INIT(_sp) { \
	(_sp)->sp_fmt[0] = (_sp)->sp_fmt[1] = (_sp)->sp_fmt[2] = (_sp)->sp_fmt[3] = '\0'; \
	(_sp)->sp_count = 0; \
	(_sp)->sp_name = NULL; \
	(_sp)->sp_offset = 0; \
	(_sp)->sp_hook = BU_STRUCTPARSE_FUNC_NULL; \
	(_sp)->sp_desc = NULL; \
	(_sp)->sp_default = NULL; \
    }

/**
 * macro suitable for declaration statement initialization of a bu_structparse
 * struct.  does not allocate memory.
 */
#define BU_STRUCTPARSE_INIT_ZERO { {'\0', '\0', '\0', '\0'}, 0, NULL, 0, BU_STRUCTPARSE_FUNC_NULL, NULL, NULL }

/**
 * returns truthfully whether a bu_structparse struct has been
 * initialized.  validates whether pointer is non-NULL.
 */
#define BU_STRUCTPARSE_IS_INITIALIZED(_sp) ((struct bu_structparse *)(_sp) != BU_STRUCTPARSE_NULL)


/*----------------------------------------------------------------------*/
/**
 * An "opaque" handle for holding onto objects, typically in some kind
 * of external form that is not directly usable without passing
 * through an "importation" function.
 *
 * A "bu_external" struct holds the "external binary" representation
 * of a structure or other block of arbitrary data.
 */
struct bu_external  {
    uint32_t ext_magic;
    size_t ext_nbytes;
#if defined(USE_BINARY_ATTRIBUTES)
    unsigned char widcode; /* needed for decoding binary attributes,
			    * same type as 'struct
			    * db5_raw_internal.a_width' */
#endif
    uint8_t *ext_buf;
};
typedef struct bu_external bu_external_t;
#define BU_EXTERNAL_NULL ((struct bu_external *)0)

/**
 * assert the integrity of a bu_external struct.
 */
#define BU_CK_EXTERNAL(_p) BU_CKMAG(_p, BU_EXTERNAL_MAGIC, "bu_external")

/**
 * initializes a bu_external struct without allocating any memory.
 */
#define BU_EXTERNAL_INIT(_p) { \
	(_p)->ext_magic = BU_EXTERNAL_MAGIC; \
	(_p)->ext_nbytes = 0; \
	(_p)->ext_buf = NULL; \
    }

/**
 * macro suitable for declaration statement initialization of a
 * bu_external struct. does not allocate memory.
 */
#define BU_EXTERNAL_INIT_ZERO { BU_EXTERNAL_MAGIC, 0, NULL }

/**
 * returns truthfully whether a bu_external struct has been
 * initialized.  is not reliable unless the struct has been
 * initialized with BU_EXTERNAL_INIT().
 */
#define BU_EXTERNAL_IS_INITIALIZED(_p) (((struct bu_external *)(_p) != BU_EXTERNAL_NULL) && (_p)->ext_magic == BU_EXTERNAL_MAGIC)


/** @} */
/*----------------------------------------------------------------------*/
/* color.c */

#define RED 0
#define GRN 1
#define BLU 2

#define HUE 0
#define SAT 1
#define VAL 2

#define ACHROMATIC	-1.0

struct bu_color
{
    uint32_t buc_magic;
    fastf_t buc_rgb[3];
};
typedef struct bu_color bu_color_t;
#define BU_COLOR_NULL ((struct bu_color *) 0)

/**
 * asserts the integrity of a bu_color struct.
 */
#define BU_CK_COLOR(_c) BU_CKMAG(_c, BU_COLOR_MAGIC, "bu_color")

/**
 * initializes a bu_bitv struct without allocating any memory.
 */
#define BU_COLOR_INIT(_c) { \
	(_c)->buc_magic = BU_COLOR_MAGIC; \
	(_c)->buc_rgb[0] = (_c)->buc_rgb[1] = (_c)->buc_rgb[2] = 0; \
    }

/**
 * macro suitable for declaration statement initialization of a bu_color
 * struct.  does not allocate memory.
 */
#define BU_COLOR_INIT_ZERO { BU_COLOR_MAGIC, {0, 0, 0} }

/**
 * returns truthfully whether a bu_color has been initialized
 */
#define BU_COLOR_IS_INITIALIZED(_c) (((struct bu_color *)(_c) != BU_COLOR_NULL) && LIKELY((_c)->magic == BU_COLOR_MAGIC))


/*----------------------------------------------------------------------*/

#include "bu/rb.h"

/*----------------------------------------------------------------------*/

/**
 * TBD
 */
struct bu_observer {
    struct bu_list l;
    struct bu_vls observer;
    struct bu_vls cmd;
};
typedef struct bu_observer bu_observer_t;
#define BU_OBSERVER_NULL ((struct bu_observer *)0)

/**
 * asserts the integrity of a non-head node bu_observer struct.
 */
#define BU_CK_OBSERVER(_op) BU_CKMAG(_op, BU_OBSERVER_MAGIC, "bu_observer magic")

/**
 * initializes a bu_observer struct without allocating any memory.
 */
#define BU_OBSERVER_INIT(_op) { \
	BU_LIST_INIT_MAGIC(&(_op)->l, BU_OBSERVER_MAGIC); \
	BU_VLS_INIT(&(_op)->observer); \
	BU_VLS_INIT(&(_op)->cmd); \
    }

/**
 * macro suitable for declaration statement initialization of a bu_observer
 * struct.  does not allocate memory.  not suitable for a head node.
 */
#define BU_OBSERVER_INIT_ZERO { {BU_OBSERVER_MAGIC, BU_LIST_NULL, BU_LIST_NULL}, BU_VLS_INIT_ZERO, BU_VLS_INIT_ZERO }

/**
 * returns truthfully whether a bu_observer has been initialized.
 */
#define BU_OBSERVER_IS_INITIALIZED(_op) (((struct bu_observer *)(_op) != BU_OBSERVER_NULL) && LIKELY((_op)->magic == BU_OBSERVER_MAGIC))

/** @addtogroup log */
/** @ingroup io */
/** @{ */
/** @file libbu/backtrace.c
 *
 * Extract a backtrace of the current call stack.
 *
 */

/**
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
BU_EXPORT extern int bu_backtrace(FILE *fp);

/**
 * Adds a hook to the list of bu_bomb hooks.  The top (newest) one of these
 * will be called with its associated client data and a string to be
 * processed.  Typically, these hook functions will display the output
 * (possibly in an X window) or record it.
 *
 * NOTE: The hook functions are all non-PARALLEL.
 */
BU_EXPORT extern void bu_bomb_add_hook(bu_hook_t func, genptr_t clientdata);

/**
 * Abort the running process.
 *
 * The bu_bomb routine is called on a fatal error, generally where no
 * recovery is possible.  Error handlers may, however, be registered
 * with BU_SETJUMP.  This routine intentionally limits calls to other
 * functions and intentionally uses no stack variables.  Just in case
 * the application is out of memory, bu_bomb deallocates a small
 * buffer of memory.
 *
 * Before termination, it optionally performs the following operations
 * in the order listed:
 *
 * 1. Outputs str to standard error
 *
 * 2. Calls any callback functions set in the global bu_bomb_hook_list
 *    variable with str passed as an argument.
 *
 * 3. Jumps to any user specified error handler registered with the
 *    bu_setjmp_valid/bu_jmpbuf setjmp facility.
 *
 * 4. Outputs str to the terminal device in case standard error is
 *    redirected.
 *
 * 5. Aborts abnormally (via abort()) if BU_DEBUG_COREDUMP is defined.
 *
 * 6. Exits with exit(12).
 *
 * Only produce a core-dump when that debugging bit is set.  Note that
 * this function is meant to be a last resort semi-graceful abort.
 *
 * This routine should never return unless there is a bu_setjmp
 * handler registered.
 */
BU_EXPORT extern void bu_bomb(const char *str) _BU_ATTR_NORETURN;

/**
 * Semi-graceful termination of the application that doesn't cause a
 * stack trace, exiting with the specified status after printing the
 * given message.  It's okay for this routine to use the stack,
 * contrary to bu_bomb's behavior since it should be called for
 * expected termination situations.
 *
 * This routine should generally not be called within a library.  Use
 * bu_bomb or (better) cascade the error back up to the application.
 *
 * This routine should never return.
 */
BU_EXPORT extern void bu_exit(int status, const char *fmt, ...) _BU_ATTR_NORETURN _BU_ATTR_PRINTF23;

/** @file libbu/crashreport.c
 *
 * Generate a crash report file, including a call stack backtrace and
 * other system details.
 *
 */

/**
 * this routine writes out details of the currently running process to
 * the specified file, including an informational header about the
 * execution environment, stack trace details, kernel and hardware
 * information, and current version information.
 *
 * returns truthfully if the crash report was written.
 *
 * due to various reasons, this routine is NOT thread-safe.
 */
BU_EXPORT extern int bu_crashreport(const char *filename);

/** @file libbu/fgets.c
 *
 * fgets replacement function that also handles CR as an EOL marker
 *
 */

/**
 * Reads in at most one less than size characters from stream and
 * stores them into the buffer pointed to by s. Reading stops after an
 * EOF, CR, LF, or a CR/LF combination. If a LF or CR is read, it is
 * stored into the buffer. If a CR/LF is read, just a CR is stored
 * into the buffer. A '\\0' is stored after the last character in the
 * buffer. Returns s on success, and NULL on error or when end of file
 * occurs while no characters have been read.
 */
BU_EXPORT extern char *bu_fgets(char *s, int size, FILE *stream);

/** @} */
/** @addtogroup color */
/** @ingroup container */
/** @{ */

/**
 * Convert between RGB and HSV color models
 *
 * R, G, and B are in {0, 1, ..., 255},
 *
 * H is in [0.0, 360.0), and S and V are in [0.0, 1.0],
 *
 * unless S = 0.0, in which case H = ACHROMATIC.
 *
 * These two routines are adapted from:
 * pp. 592-3 of J.D. Foley, A. van Dam, S.K. Feiner, and J.F. Hughes,
 * _Computer graphics: principles and practice_, 2nd ed., Addison-Wesley,
 * Reading, MA, 1990.
 */

/* color.c */
BU_EXPORT extern void bu_rgb_to_hsv(unsigned char *rgb, fastf_t *hsv);
BU_EXPORT extern int bu_hsv_to_rgb(fastf_t *hsv, unsigned char *rgb);
BU_EXPORT extern int bu_str_to_rgb(char *str, unsigned char *rgb);
BU_EXPORT extern int bu_color_from_rgb_floats(struct bu_color *cp, fastf_t *rgb);
BU_EXPORT extern int bu_color_to_rgb_floats(struct bu_color *cp, fastf_t *rgb);

/* UNIMPLEMENTED
 *
 * BU_EXPORT export void bu_color_from_rgb_chars(struct bu_color *cp, unsigned char *rgb);
 * BU_EXPORT export int bu_color_to_rgb_chars(struct bu_color *cp, unsigned char *rgb);
 * BU_EXPORT export int bu_color_from_hsv_floats(struct bu_color *cp, fastf_t *hsv);
 * BU_EXPORT export int bu_color_to_hsv_floats(struct bu_color *cp, fastf_t *hsv);
 */


/** @} */
/** @addtogroup file */
/** @ingroup io */
/** @{ */

/** @file libbu/file.c
 *
 * Support routines for identifying properties of files and
 * directories such as whether they exist or are the same as another
 * given file.
 *
 */

/**
 * Returns truthfully whether the given file path exists or not.  An
 * empty or NULL path name is treated as a non-existent file and, as
 * such, will return false.  If fd is non-NULL, it will be set to an
 * open file descriptor for the provided path.
 *
 * @return >0 The given filename exists.
 * @return 0 The given filename does not exist.
 */
BU_EXPORT extern int bu_file_exists(const char *path, int *fd);

/**
 * Returns truthfully as to whether the two provided filenames are the
 * same file.  If either file does not exist, the result is false.  If
 * either filename is empty or NULL, it is treated as non-existent
 * and, as such, will also return false.
 */
BU_EXPORT extern int bu_same_file(const char *fn1, const char *fn2);

/**
 * returns truthfully as to whether or not the two provided file
 * descriptors are the same file.  if either file does not exist, the
 * result is false.
 */
BU_EXPORT extern int bu_same_fd(int fd1, int fd2);

/**
 * returns truthfully if current user can read the specified file or
 * directory.
 */
BU_EXPORT extern int bu_file_readable(const char *path);

/**
 * returns truthfully if current user can write to the specified file
 * or directory.
 */
BU_EXPORT extern int bu_file_writable(const char *path);

/**
 * returns truthfully if current user can run the specified file or
 * directory.
 */
BU_EXPORT extern int bu_file_executable(const char *path);

/**
 * Returns truthfully whether the given file path is a directory.  An
 * empty or NULL path name is treated as a non-existent directory and,
 * as such, will return false.
 *
 * @return >0 The given filename is a directory
 * @return 0 The given filename is not a directory.
 */
BU_EXPORT extern int bu_file_directory(const char *path);

/**
 * Returns truthfully whether the given file path is a symbolic link.
 * An empty or NULL path name is treated as a non-existent link and,
 * as such, will return false.
 *
 * @return >0 The given filename is a symbolic link
 * @return 0 The given filename is not a symbolic link.
 */
BU_EXPORT extern int bu_file_symbolic(const char *path);

/**
 * forcibly attempts to delete a specified file.  if the file can be
 * deleted by relaxing file permissions, they will be changed in order
 * to delete the file.
 *
 * returns truthfully if the specified file was deleted.
 */
BU_EXPORT extern int bu_file_delete(const char *path);

/**
 * matches a filepath pattern to directory entries.  if non-NULL,
 * matching paths are dynamically allocated, stored into the provided
 * 'matches' array, and followed by a terminating NULL entry.
 *
 * If '*matches' is NULL, the caller is expected to free the matches
 * array with bu_free_argv() If '*matches' is non-NULL (i.e., string
 * array is already allocated or on the stack), the caller is expected
 * to ensure adequate entries are allocated and call bu_free_array()
 * to clean up.  If 'matches' is NULL, no entries will be allocated or
 * stored, but the number of matches will still be returned.
 *
 * Example:
 *
 * char **my_matches = NULL;
 * bu_file_glob("src/libbu/[a-e]*.c", &my_matches);
 *
 * This will allocate an array for storing glob matches, filling in
 * the array with all of the directory entries starting with 'a'
 * through 'e' and ending with a '.c' suffix in the src/libbu
 * directory.
 *
 * returns the number of matches
 */
BU_EXPORT extern size_t bu_file_glob(const char *pattern, char ***matches);

/**
 * Call canonicalization routines to both expand and validate
 * a path name.
 *
 * returns a pointer to the canonical path.  Caller must free
 * the path.
 */
BU_EXPORT extern char * bu_file_path_canonicalize(const char *path);

/** @file libbu/fnmatch.c
 *
 */

#define BU_FNMATCH_NOESCAPE    0x01 /**< bu_fnmatch() flag.  Backslash escaping. */
#define BU_FNMATCH_PATHNAME    0x02 /**< bu_fnmatch() flag.  Slash must be matched by slash. */
#define BU_FNMATCH_PERIOD      0x04 /**< bu_fnmatch() flag.  Period must be matched by period. */
#define BU_FNMATCH_LEADING_DIR 0x08 /**< bu_fnmatch() flag.  Ignore /<tail> after Imatch. */
#define BU_FNMATCH_CASEFOLD    0x10 /**< bu_fnmatch() flag.  Case-insensitive searching. */

/**
 * bu_fnmatch() return value when no match is found (0 if found)
 */
#define BU_FNMATCH_NOMATCH 1       /* Match failed. */

/**
 * Function fnmatch() as specified in POSIX 1003.2-1992, section B.6.
 * Compares a string filename or pathname to a pattern.
 *
 * Returns 0 if a match is found or BU_FNMATCH_NOMATCH otherwise.
 *
 */
BU_EXPORT extern int bu_fnmatch(const char *pattern, const char *pathname, int flags);


/** @file libbu/dirent.c
 *
 * Functionality for accessing all files in a directory.
 *
 */

/**
 * Returns the number of directory entries for a given path matching
 * an optional glob pattern.  If the caller provides a pointer to an
 * argv-style 'files' array, this function will allocate the array
 * with dynamically allocated strings for any matching file(s).
 *
 * It is the caller's responsibility to free a non-NULL 'files' array
 * with bu_free_argv().
 */
BU_EXPORT extern size_t bu_dir_list(const char *path, const char *pattern, char ***files);


/** @file libbu/realpath.c
 *
 */

/**
 * Call canonicalization routines to both expand and validate
 * a path name.
 *
 * Returns a pointer to the canonical path. If resolved_path is
 * NULL, caller is responsible for freeing the returned path
 * via bu_free.  If supplying a result string, the string must hold
 * at least MAXPATHLEN characters.
 */
BU_EXPORT extern char * bu_realpath(const char *path, char *resolved_path);


/** @file libbu/brlcad_path.c
 *
 * @brief
 * A support routine to provide the executable code with the path
 * to where the BRL-CAD programs and libraries are installed.
 *
 */

/**
 * DEPRECATED: This routine is replaced by bu_argv0_full_path().
 *             Do not use.
 *
 * this routine is used by the brlcad-path-finding routines when
 * attempting to locate binaries, libraries, and resources.  This
 * routine will set argv0 if path is provided and should generally be
 * set early on by bu_setprogname().
 *
 * this routine will return "(BRL-CAD)" if argv[0] cannot be
 * identified but should never return NULL.
 */
DEPRECATED BU_EXPORT extern const char *bu_argv0(void);

/**
 * DEPRECATED: This routine is replaced by bu_getcwd().
 *             Do not use.
 *
 * returns the full path to argv0, regardless of how the application
 * was invoked.
 *
 * this routine will return "(BRL-CAD)" if argv[0] cannot be
 * identified but should never return NULL.  this routine is not
 * thread-safe.
 */
BU_EXPORT extern const char *bu_argv0_full_path(void);

/**
 * Get the name of the running application.  application codes should
 * call bu_setprogname() first to ensure that the program name is
 * stored appropriately on platforms that do not have an intrinsic
 * method for tracking the program name automatically.
 *
 * while this routine is thread-safe and reentrant, the static string
 * returned is shared amongst all threads.
 */
BU_EXPORT extern const char *bu_getprogname(void);

/**
 * Set the name of the running application.  This isn't strictly
 * necessary on platforms that have an intrinsic method for tracking
 * the program name automatically, but is still recommended for
 * portability and is necessary on some strict modes of compilation.
 *
 * while the implementation relies on a static string shared across
 * all threads, this routine is thread-safe and reentrant.
 */
BU_EXPORT extern void bu_setprogname(const char *path);

/**
 * returns the pathname for the current working directory.
 */
BU_EXPORT extern char *bu_getcwd(char *buf, size_t size);

/**
 * Report the relative paths being used to hold BRL-CAD applications,
 * libraries, and data.
 *
 * Recognized keys include:
 *
 *   bin     - Directory containing binary applications
 *   lib     - Directory containing libraries
 *   include - Directory containing headers
 *   data    - Directory containing shared data
 *   share   - Directory containing shared data
 *   doc     - Directory containing documentation
 *   man     - Directory containing Unix man pages
 *
 * @return
 * A STATIC buffer is returned.  It is the caller's responsibility to
 * call bu_strdup() or make other provisions to save the returned
 * string, before calling again.
 */
BU_EXPORT extern const char *bu_brlcad_dir(const char *dirkey, int fail_quietly);

/**
 * Locate where the BRL-CAD applications and libraries are installed.
 *
 * The BRL-CAD root is searched for in the following order of
 * precedence by testing for the rhs existence if provided or the
 * directory existence otherwise:
 *
 *   BRLCAD_ROOT environment variable if set
 *   BRLCAD_ROOT compile-time path
 *   run-time path identification
 *   /usr/brlcad static path
 *   current directory
 *
 * @return
 * A STATIC buffer is returned.  It is the caller's responsibility to
 * call bu_strdup() or make other provisions to save the returned
 * string, before calling again.
 */
BU_EXPORT extern const char *bu_brlcad_root(const char *rhs, int fail_quietly);

/**
 * Locate where the BRL-CAD data resources are installed.
 *
 * The BRL-CAD data resources are searched for in the following order
 * of precedence by testing for the existence of rhs if provided or
 * the directory existence otherwise:
 *
 *   BRLCAD_DATA environment variable if set
 *   BRLCAD_DATA compile-time path
 *   bu_brlcad_root/DATA_DIR path
 *   bu_brlcad_root/share path
 *   current directory
 *
 * A STATIC buffer is returned.  It is the caller's responsibility to
 * call bu_strdup() or make other provisions to save the returned
 * string, before calling again.
 */
BU_EXPORT extern const char *bu_brlcad_data(const char *rhs, int fail_quietly);

/**
 * returns the first USER path match to a given executable name.
 *
 * Routine to provide BSD "which" functionality, locating binaries of
 * specified programs from the user's PATH. This is useful to locate
 * binaries and resources at run-time.
 *
 * caller should not free the result, though it will not be preserved
 * between calls either.  the caller should strdup the result if they
 * need to keep it around.
 *
 * routine will return NULL if the executable command cannot be found.
 */
BU_EXPORT extern const char *bu_which(const char *cmd);

/**
 * returns the first SYSTEM path match to a given executable cmd name.
 *
 * Routine to provide BSD "whereis" functionality, locating binaries
 * of specified programs from the SYSTEM path.  This is useful to
 * locate binaries and resources at run-time.
 *
 * caller should not free the result, though it will not be preserved
 * between calls either.  the caller should strdup the result if they
 * need to keep it around.
 *
 * routine will return NULL if the executable command cannot be found.
 */
BU_EXPORT extern const char *bu_whereis(const char *cmd);

/** @file libbu/temp.c
 *
 * Routine to open a temporary file.
 *
 */

/**
 * Create a temporary file.  The first readable/writable directory
 * will be used, searching TMPDIR/TEMP/TMP environment variable
 * directories followed by default system temp directories and
 * ultimately trying the current directory.
 *
 * This routine is guaranteed to return a new unique file or return
 * NULL on failure.  The temporary file will be automatically unlinked
 * on application exit.  It is the caller's responsibility to set file
 * access settings, preserve file contents, or destroy file contents
 * if the default behavior is non-optimal.
 *
 * The name of the temporary file will be copied into a user-provided
 * (filepath) buffer if it is a non-NULL pointer and of a sufficient
 * (len) length to contain the filename.
 *
 * This routine is NOT thread-safe.
 *
 * Typical Use:
 @code
  FILE *fp;
  char filename[MAXPATHLEN];
  fp = bu_temp_file(&filename, MAXPATHLEN); // get file name
  ...
  fclose(fp); // close the file when you're done
  ...
  fp = bu_temp_file(NULL, 0); // don't need file name
  bu_fchmod(fileno(fp), 0777);
  ...
  rewind(fp);
  while (fputc(0, fp) == 0);
  fclose(fp);
 @endcode
 */
BU_EXPORT extern FILE *bu_temp_file(char *filepath, size_t len);

/** @} */
/** @addtogroup getopt */
/** @ingroup data */
/** @{ */

/** @file libbu/getopt.c
 *
 * @brief
 * Special portable re-entrant version of getopt.
 *
 * Everything is prefixed with bu_, to distinguish it from the various
 * getopt routines found in libc.
 *
 * Important note -
 * If bu_getopt() is going to be used more than once, it is necessary
 * to reinitialize bu_optind=1 before beginning on the next argument
 * list.
 */

/**
 * for bu_getopt().  set to zero to suppress errors.
 */
BU_EXPORT extern int bu_opterr;

/**
 * for bu_getopt().  current index into parent argv vector.
 */
BU_EXPORT extern int bu_optind;

/**
 * for bu_getopt().  current option being checked for validity.
 */
BU_EXPORT extern int bu_optopt;

/**
 * for bu_getopt().  current argument associated with current option.
 */
BU_EXPORT extern char *bu_optarg;

/**
 * Get option letter from argument vector.
 *
 * returns the next known option character in ostr.  If bu_getopt()
 * encounters a character not found in ostr or if it detects a missing
 * option argument, it returns `?' (question mark).  If ostr has a
 * leading `:' then a missing option argument causes `:' to be
 * returned instead of `?'.  In either case, the variable bu_optopt is
 * set to the character that caused the error.  The bu_getopt()
 * function returns -1 when the argument list is exhausted.
 */
BU_EXPORT extern int bu_getopt(int nargc, char * const nargv[], const char *ostr);

/** @} */
/** @addtogroup hist */
/** @ingroup data */
/** @{ */

/* hist.c */
/* These are a set of data histogramming routines. */

BU_EXPORT extern void bu_hist_free(struct bu_hist *histp);

/**
 * Initialize a bu_hist structure.
 *
 * It is expected that the structure is junk upon entry.
 */
BU_EXPORT extern void bu_hist_init(struct bu_hist *histp, fastf_t min, fastf_t max, size_t nbins);

BU_EXPORT extern void bu_hist_range(struct bu_hist *hp, fastf_t low, fastf_t high);

/**
 * Print a histogram.
 */
BU_EXPORT extern void bu_hist_pr(const struct bu_hist *histp, const char *title);

/** @} */


/** @addtogroup thread */
/** @ingroup parallel */
/** @{ */
/** @file libbu/parallel.c
 *
 * subroutine to determine if we are multi-threaded
 *
 * This subroutine is separated off from parallel.c so that bu_bomb()
 * and others can call it, without causing either parallel.c or
 * semaphore.c to get referenced and thus causing the loader to drag
 * in all the parallel processing stuff from the vendor library.
 *
 */

/**
 * A clean way for bu_bomb() to tell if this is a parallel
 * application.  If bu_parallel() is active, this routine will return
 * non-zero.
 */
BU_EXPORT extern int bu_is_parallel(void);

/**
 * Used by bu_bomb() to help terminate parallel threads,
 * without dragging in the whole parallel library if it isn't being used.
 */
BU_EXPORT extern void bu_kill_parallel(void);

/**
 * returns the CPU number of the current bu_parallel() invoked thread.
 */
BU_EXPORT extern int bu_parallel_id(void);


/** @} */

/** @addtogroup log */
/** @ingroup io */
/** @{ */
/** @file libbu/linebuf.c
 *
 * A portable way of doing setlinebuf().
 *
 */

BU_EXPORT extern void bu_setlinebuf(FILE *fp);

/** @} */

/** @addtogroup log */
/** @ingroup io */
/** @{ */
/** @file libbu/hook.c
 *
 * @brief
 * BRL-CAD support library's hook utility.
 *
 */
BU_EXPORT extern void bu_hook_list_init(struct bu_hook_list *hlp);
BU_EXPORT extern void bu_hook_add(struct bu_hook_list *hlp,
				  bu_hook_t func,
				  genptr_t clientdata);
BU_EXPORT extern void bu_hook_delete(struct bu_hook_list *hlp,
				     bu_hook_t func,
				     genptr_t clientdata);
BU_EXPORT extern void bu_hook_call(struct bu_hook_list *hlp,
				   genptr_t buf);
BU_EXPORT extern void bu_hook_save_all(struct bu_hook_list *hlp,
				       struct bu_hook_list *save_hlp);
BU_EXPORT extern void bu_hook_delete_all(struct bu_hook_list *hlp);
BU_EXPORT extern void bu_hook_restore_all(struct bu_hook_list *hlp,
					  struct bu_hook_list *restore_hlp);

/** @} */
/** @addtogroup log */
/** @ingroup io */
/** @{ */
/** @file libbu/log.c
 *
 * @brief
 * parallel safe version of fprintf for logging
 *
 * BRL-CAD support library, error logging routine.  Note that the user
 * may provide his own logging routine, by replacing these functions.
 * That is why this is in file of its own.  For example, LGT and
 * RTSRV take advantage of this.
 *
 * Here is an example of how to set up a custom logging callback.
 * While bu_log presently writes to STDERR by default, this behavior
 * should not be relied upon and may be changed to STDOUT in the
 * future without notice.
 *
 @code
 --- BEGIN EXAMPLE ---

 int log_output_to_file(genptr_t data, genptr_t str)
 {
   FILE *fp = (FILE *)data;
   fprintf(fp, "LOG: %s", str);
   return 0;
 }

 int main(int ac, char *av[])
 {
   FILE *fp = fopen("whatever.log", "w+");
   bu_log_add_hook(log_output_to_file, (genptr_t)fp);
   bu_log("Logging to file.\n");
   bu_log_delete_hook(log_output_to_file, (genptr_t)fp);
   bu_log("Logging to stderr.\n");
   fclose(fp);
   return 0;
 }

 --- END EXAMPLE ---
 @endcode
 *
 */


/**
 * Change global indentation level by indicated number of characters.
 * Call with a large negative number to cancel all indentation.
 */
BU_EXPORT extern void bu_log_indent_delta(int delta);

/**
 * For multi-line vls generators, honor logindent level like bu_log() does,
 * and prefix the proper number of spaces.
 * Should be called at the front of each new line.
 */
BU_EXPORT extern void bu_log_indent_vls(struct bu_vls *v);

/**
 * Adds a hook to the list of bu_log hooks.  The top (newest) one of these
 * will be called with its associated client data and a string to be
 * processed.  Typically, these hook functions will display the output
 * (possibly in an X window) or record it.
 *
 * NOTE: The hook functions are all non-PARALLEL.
 */
BU_EXPORT extern void bu_log_add_hook(bu_hook_t func, genptr_t clientdata);

/**
 * Removes the hook matching the function and clientdata parameters from
 * the hook list.  Note that it is not necessarily the active (top) hook.
 */
BU_EXPORT extern void bu_log_delete_hook(bu_hook_t func, genptr_t clientdata);

BU_EXPORT extern void bu_log_hook_save_all(struct bu_hook_list *save_hlp);
BU_EXPORT extern void bu_log_hook_delete_all(void);
BU_EXPORT extern void bu_log_hook_restore_all(struct bu_hook_list *restore_hlp);

/**
 * Log a single character with no flushing.
 */
BU_EXPORT extern void bu_putchar(int c);

/**
 * The routine is primarily called to log library events.
 *
 * The function is essentially a semaphore-protected version of
 * fprintf(stderr) with optional logging hooks and automatic
 * indentation options.  The main difference is that this function
 * does not keep track of characters printed, so nothing is returned.
 *
 * This function recognizes a %V format specifier to print a bu_vls
 * struct pointer.  See bu_vsscanf() for details.
 */
BU_EXPORT extern void bu_log(const char *, ...) _BU_ATTR_PRINTF12;

/**
 * Just like bu_log() except that you can send output to a specified
 * file pointer.
 */
BU_EXPORT extern void bu_flog(FILE *, const char *, ...) _BU_ATTR_PRINTF23;

/**
 * Custom vsscanf which wraps the system sscanf, and is wrapped by bu_sscanf.
 *
 * bu_vsscanf differs notably from the underlying system sscanf in that:
 *
 *  - A maximum field width is required for unsuppressed %s and %[...]
 *    conversions. If a %s or %[...] conversion is encountered which does
 *    not include a maximum field width, the routine bombs in order to avoid
 *    an accidental buffer overrun.
 *
 *  - %V and %#V have been added as valid conversions. Both expect a pointer to
 *    a struct bu_vls as their argument.
 *
 *    %V is comparable to %[^]. It instructs bu_vsscanf to read arbitrary
 *    characters from the source and store them in the vls buffer. The default
 *    maximum field width is infinity.
 *
 *    %#V is comparable to %s. It instructs bu_vsscanf to skip leading
 *    whitespace, and then read characters from the source and store them in the
 *    vls buffer until the next whitespace character is encountered. The default
 *    maximum field width is infinity.
 *
 *  - 0 is always a valid field width for unsuppressed %c, %s, and %[...]
 *    conversions and causes '\0' to be written to the supplied char*
 *    argument.
 *
 *  - a/e/f/g and A/E/F/G are always synonyms for float conversion.
 *
 *  - The C99 conversions hh[diouxX], z[diouxX], and t[diouxX] are always
 *    supported.
 *
 * This routine has an associated test program named test_sscanf, which
 * compares its behavior to the system sscanf.
 */
BU_EXPORT extern int bu_vsscanf(const char *src, const char *fmt, va_list ap);

/**
 * Initializes the va_list, then calls bu_vsscanf.
 *
 * This routine has an associated test program named test_sscanf, which
 * compares its behavior to the system sscanf.
 */
BU_EXPORT extern int bu_sscanf(const char *src, const char *fmt, ...) _BU_ATTR_SCANF23;

/** @} */

/** @addtogroup malloc */
/** @ingroup memory */
/** @{ */
/** @file libbu/malloc.c
 *
 * @brief
 * Parallel-protected debugging-enhanced wrapper around system malloc().
 *
 * Provides a parallel-safe interface to the system memory allocator
 * with standardized error checking, optional memory-use logging, and
 * optional run-time pointer and memory corruption testing.
 *
 * The bu_*alloc() routines can't use bu_log() because that uses the
 * bu_vls() routines which depend on bu_malloc().  So it goes direct
 * to stderr, semaphore protected.
 *
 */

BU_EXPORT extern long bu_n_malloc;
BU_EXPORT extern long bu_n_free;
BU_EXPORT extern long bu_n_realloc;

/**
 * This routine only returns on successful allocation.  We promise
 * never to return a NULL pointer; caller doesn't have to check.
 * Allocation failure results in bu_bomb() being called.
 */
BU_EXPORT extern genptr_t bu_malloc(size_t siz,
				    const char *str);

/**
 * This routine only returns on successful allocation.
 * We promise never to return a NULL pointer; caller doesn't have to check.
 * Failure results in bu_bomb() being called.
 */
BU_EXPORT extern genptr_t bu_calloc(size_t nelem,
				    size_t elsize,
				    const char *str);

BU_EXPORT extern void bu_free(genptr_t ptr,
			      const char *str);

/**
 * bu_malloc()/bu_free() compatible wrapper for realloc().
 *
 * this routine mimics the C99 standard behavior of realloc() except
 * that NULL will never be returned.  it will bomb if siz is zero and
 * ptr is NULL.  it will return a minimum allocation suitable for
 * bu_free() if siz is zero and ptr is non-NULL.
 *
 * While the string 'str' is provided for the log messages, don't
 * disturb the str value, so that this storage allocation can be
 * tracked back to its original creator.
 */
BU_EXPORT extern genptr_t bu_realloc(genptr_t ptr,
				     size_t siz,
				     const char *str);

/**
 * Print map of memory currently in use.
 */
BU_EXPORT extern void bu_prmem(const char *str);

/**
 * On systems with the CalTech malloc(), the amount of storage
 * ACTUALLY ALLOCATED is the amount requested rounded UP to the
 * nearest power of two.  For structures which are acquired and
 * released often, this works well, but for structures which will
 * remain unchanged for the duration of the program, this wastes as
 * much as 50% of the address space (and usually memory as well).
 * Here, we round up a byte size to the nearest power of two, leaving
 * off the malloc header, so as to ask for storage without wasting
 * any.
 *
 * On systems with the traditional malloc(), this strategy will just
 * consume the memory in somewhat larger chunks, but overall little
 * unused memory will be consumed.
 */
BU_EXPORT extern int bu_malloc_len_roundup(int nbytes);

/**
 * For a given pointer allocated by bu_malloc(), bu_calloc(), or
 * BU_ALLOC() check the magic number stored after the allocation area
 * when BU_DEBUG_MEM_CHECK is set.
 *
 * This is the individual version of bu_mem_barriercheck().
 *
 * returns if pointer good or BU_DEBUG_MEM_CHECK not set, bombs if
 * memory is corrupted.
 */
BU_EXPORT extern void bu_ck_malloc_ptr(genptr_t ptr, const char *str);

/**
 * Check *all* entries in the memory debug table for barrier word
 * corruption.  Intended to be called periodically through an
 * application during debugging.  Has to run single-threaded, to
 * prevent table mutation.
 *
 * This is the bulk version of bu_ck_malloc_ptr()
 *
 * Returns -
 *  -1	something is wrong
 *   0	all is OK;
 */
BU_EXPORT extern int bu_mem_barriercheck(void);

/**
 * really fast heap-based memory allocation intended for "small"
 * allocation sizes (e.g., single structs).
 *
 * the implementation allocates chunks of memory ('pages') in order to
 * substantially reduce calls to system malloc.  it has a nice
 * property of having O(1) constant time complexity and profiles
 * significantly faster than system malloc().
 *
 * release memory with bu_heap_put() only.
 */
BU_EXPORT extern void *bu_heap_get(size_t sz);

/**
 * counterpart to bu_heap_get() for releasing fast heap-based memory
 * allocations.
 *
 * the implementation may do nothing, relying on free-on-exit, or may
 * mark deallocations for reuse.  pass a NULL pointer and zero size to
 * force compaction of any unused memory.
 */
BU_EXPORT extern void bu_heap_put(void *ptr, size_t sz);

/**
 * Convenience typedef for the printf()-style callback function used
 * during application exit to print summary statistics.
 */
typedef int (*bu_heap_func_t)(const char *, ...);

/**
 * This function registers and returns the current printing function
 * that will be used during application exit (via an atexit() handler)
 * if the BU_HEAP_PRINT environment variable is set.  Statistics on
 * calls to bu_heap_get() and bu_heap_put() will be logged.  If log is
 * NULL, the currently set function will remain unchanged and will be
 * returned.
 */
BU_EXPORT extern bu_heap_func_t bu_heap_log(bu_heap_func_t log);


/** @} */

/** @addtogroup log */
/** @ingroup io */
/** @{ */
/** @file libbu/dirname.c
 *
 * @brief
 * Routines to process file and path names.
 *
 */

/**
 * Given a string containing a hierarchical path, return a dynamic
 * string to the parent path.
 *
 * This function is similar if not identical to most dirname() BSD
 * system function implementations; but that system function cannot be
 * used due to significantly inconsistent behavior across platforms.
 *
 * This function always recognizes paths separated by a '/' (i.e.,
 * geometry paths) as well as whatever the native platform directory
 * separator may be.  It is assumed that all file and directory names
 * in the path will not contain a path separator, even if escaped.
 *
 * It is the caller's responsibility to bu_free() the pointer returned
 * from this routine.
 *
 * Examples of strings returned:
 *
 *	/usr/dir/file	/usr/dir
 * @n	/usr/dir/	/usr
 * @n	/usr/file	/usr
 * @n	/usr/		/
 * @n	/usr		/
 * @n	/		/
 * @n	.		.
 * @n	..		.
 * @n	usr		.
 * @n	a/b		a
 * @n	a/		.
 * @n	../a/b		../a
 *
 * This routine will return "." if other valid results are not available
 * but should never return NULL.
 */
BU_EXPORT extern char *bu_dirname(const char *path);

/**
 * Given a string containing a hierarchical path, return a dynamic
 * string to the portion after the last path separator.
 *
 * This function is similar if not identical to most basename() BSD
 * system function implementations; but that system function cannot be
 * used due to significantly inconsistent behavior across platforms.
 *
 * This function always recognizes paths separated by a '/' (i.e.,
 * geometry paths) as well as whatever the native platform directory
 * separator may be.  It is assumed that all file and directory names
 * in the path will not contain a path separator, even if escaped.
 *
 * It is the caller's responsibility allocate basename with
 * strlen(path).
 *
 * Examples of strings returned:
 *
 *	/usr/dir/file	file
 * @n	/usr/dir/	dir
 * @n	/usr/		usr
 * @n	/usr		usr
 * @n	/		/
 * @n	.		.
 * @n	..		..
 * @n	usr		usr
 * @n	a/b		b
 * @n	a/		a
 * @n	///		/
 */
BU_EXPORT extern void bu_basename(char *basename, const char *path);

/** @} */

/** @addtogroup mf */
/** @ingroup io */
/** @{ */

/**
 * Provides a standardized interface for acquiring the entire contents
 * of an existing file mapped into the current address space, using
 * the virtual memory capabilities of the operating system (such as
 * mmap()) where available, or by allocating sufficient dynamic memory
 * and reading the entire file.
 *
 * If the file can not be opened, as descriptive an error message as
 * possible will be printed, to simplify code handling in the caller.
 *
 * Mapped files are always opened read-only.
 *
 * If the system does not support mapped files, the data is read into
 * memory.
 */
BU_EXPORT extern struct bu_mapped_file *bu_open_mapped_file(const char *name,
							    const char *appl);

/**
 * Release a use of a mapped file.  Because it may be re-used shortly,
 * e.g. by the next frame of an animation, don't release the memory
 * even on final close, so that it's available when next needed.
 *
 * Call bu_free_mapped_files() after final close to reclaim space.
 * But only do that if you're SURE that ALL these files will never
 * again need to be mapped by this process.  Such as when running
 * multi-frame animations.
 */
BU_EXPORT extern void bu_close_mapped_file(struct bu_mapped_file *mp);

BU_EXPORT extern void bu_pr_mapped_file(const char *title,
					const struct bu_mapped_file *mp);

/**
 * Release storage being used by mapped files with no remaining users.
 * This entire routine runs inside a critical section, for parallel
 * protection.  Only call this routine if you're SURE that ALL these
 * files will never again need to be mapped by this process.  Such as
 * when running multi-frame animations.
 */
BU_EXPORT extern void bu_free_mapped_files(int verbose);

/**
 * A wrapper for bu_open_mapped_file() which uses a search path to
 * locate the file.
 *
 * The search path is specified as a normal C argv array, terminated
 * by a null string pointer.  If the file name begins with a slash
 * ('/') the path is not used.
 */
BU_EXPORT extern struct bu_mapped_file *bu_open_mapped_file_with_path(char * const *path,
								      const char *name,
								      const char *appl);


/** @} */

/** @addtogroup thread */
/** @ingroup parallel */
/** @{ */
/** @file libbu/kill.c
 *
 * terminate a given process.
 *
 */

/**
 * terminate a given process.
 *
 * returns truthfully whether the process could be killed.
 */
BU_EXPORT extern int bu_terminate(int process);

/** @file libbu/process.c
 *
 * process management routines
 *
 */

/**
 * returns the process ID of the calling process
 */
BU_EXPORT extern int bu_process_id(void);

/** @file libbu/parallel.c
 *
 * routines for parallel processing
 *
 * Machine-specific routines for portable parallel processing.
 *
 */

/**
 * Without knowing what the current UNIX "nice" value is, change to a
 * new absolute "nice" value.  (The system routine makes a relative
 * change).
 */
BU_EXPORT extern void bu_nice_set(int newnice);

/**
 * Return the maximum number of physical CPUs that are considered to
 * be available to this process now.
 */
BU_EXPORT extern size_t bu_avail_cpus(void);

/**
 * Create 'ncpu' copies of function 'func' all running in parallel,
 * with private stack areas.  Locking and work dispatching are handled
 * by 'func' using a "self-dispatching" paradigm.
 *
 * 'func' is called with one parameter, its thread number.  Threads
 * are given increasing numbers, starting with zero.  Processes may
 * also call bu_parallel_id() to obtain their thread number.
 *
 * Threads created with bu_parallel() automatically set CPU affinity
 * where available for improved performance.  This behavior can be
 * disabled at runtime by setting the LIBBU_AFFINITY environment
 * variable to 0.
 *
 * This function will not return control until all invocations of the
 * subroutine are finished.
 */
BU_EXPORT extern void bu_parallel(void (*func)(int ncpu, genptr_t arg), int ncpu, genptr_t arg);

/** @} */

/** @addtogroup parse */
/** @ingroup container */
/** @{ */
/** @file libbu/parse.c
 *
 * routines for parsing arbitrary structures
 *
 * Routines to assign values to elements of arbitrary structures.  The
 * layout of a structure to be processed is described by a structure
 * of type "bu_structparse", giving element names, element formats, an
 * offset from the beginning of the structure, and a pointer to an
 * optional "hooked" function that is called whenever that structure
 * element is changed.
 */

/**
 * ASCII to struct elements.
 *
 * Parse the structure element description in the vls string "vls"
 * according to the structure description in "parsetab"
 *
 * @return <0 failure
 * @return  0 OK
 */
BU_EXPORT extern int bu_struct_parse(const struct bu_vls *in_vls,
				     const struct bu_structparse *desc,
				     const char *base);

/**
 * struct elements to ASCII.
 */
BU_EXPORT extern void bu_struct_print(const char *title,
				      const struct bu_structparse *parsetab,
				      const char *base);

/**
 * struct elements to machine-independent binary.
 *
 * copies ext data to base
 */
BU_EXPORT extern int bu_struct_export(struct bu_external *ext,
				      const genptr_t base,
				      const struct bu_structparse *imp);

/**
 * machine-independent binary to struct elements.
 *
 * copies ext data to base
 */
BU_EXPORT extern int bu_struct_import(genptr_t base,
				      const struct bu_structparse *imp,
				      const struct bu_external *ext);

/**
 * Put a structure in external form to a stdio file.  All formatting
 * must have been accomplished previously.
 *
 * Returns number of bytes written.  On error, a short byte count (or
 * zero) is returned.  Use feof(3) or ferror(3) to determine which
 * errors occur.
 */
BU_EXPORT extern size_t bu_struct_put(FILE *fp,
				      const struct bu_external *ext);

/**
 * Obtain the next structure in external form from a stdio file.
 *
 * Returns number of bytes read into the bu_external.  On error, zero
 * is returned.
 */
BU_EXPORT extern size_t bu_struct_get(struct bu_external *ext,
				      FILE *fp);

/**
 * Given a buffer with an external representation of a structure
 * (e.g. the ext_buf portion of the output from bu_struct_export),
 * check it for damage in shipment, and if it's OK, wrap it up in an
 * bu_external structure, suitable for passing to bu_struct_import().
 */
BU_EXPORT extern void bu_struct_wrap_buf(struct bu_external *ext,
					 genptr_t buf);

/**
 * This differs from bu_struct_print in that this output is less
 * readable by humans, but easier to parse with the computer.
 */
BU_EXPORT extern void bu_vls_struct_print(struct bu_vls *vls,
					  const struct bu_structparse *sdp,
					  const char *base);

/**
 * This differs from bu_struct_print in that it prints to a vls.
 */
BU_EXPORT extern void bu_vls_struct_print2(struct bu_vls *vls,
					   const char *title,
					   const struct bu_structparse *sdp,
					   const char *base);

/**
 * Convert a structure element (indicated by sdp) to its ASCII
 * representation in a VLS.
 */
BU_EXPORT extern void bu_vls_struct_item(struct bu_vls *vp,
					 const struct bu_structparse *sdp,
					 const char *base,
					 int sep_char);

/**
 * Convert a structure element called "name" to an ASCII
 * representation in a VLS.
 */
BU_EXPORT extern int bu_vls_struct_item_named(struct bu_vls *vp,
					      const struct bu_structparse *sdp,
					      const char *name,
					      const char *base,
					      int sep_char);

/**
 * This allows us to specify the "size" parameter as values like ".5m"
 * or "27in" rather than using mm all the time.
 */
BU_EXPORT extern void bu_parse_mm(const struct bu_structparse *sdp,
				  const char *name,
				  char *base,
				  const char *value);

BU_EXPORT extern int bu_key_eq_to_key_val(const char *in,
					  const char **next,
					  struct bu_vls *vls);

/**
 * Take an old v4 shader specification of the form
 *
 *   shadername arg1=value1 arg2=value2 color=1/2/3
 *
 * and convert it into the v5 {} list form
 *
 *   shadername {arg1 value1 arg2 value2 color 1/2/3}
 *
 * Note -- the input string is smashed with nulls.
 *
 * Note -- the v5 version is used everywhere internally, and in v5
 * databases.
 *
 * @return 1 error
 * @return 0 OK
 */
BU_EXPORT extern int bu_shader_to_list(const char *in, struct bu_vls *vls);

BU_EXPORT extern int bu_shader_to_key_eq(const char *in, struct bu_vls *vls);

/**
 * Take a block of memory, and write it into a file.
 *
 * Caller is responsible for freeing memory of external representation,
 * using bu_free_external().
 *
 * @return <0 error
 * @return  0 OK
 */
BU_EXPORT extern int bu_fwrite_external(FILE *fp,
					const struct bu_external *ep);

BU_EXPORT extern void bu_hexdump_external(FILE *fp, const struct bu_external *ep,
					  const char *str);

BU_EXPORT extern void bu_free_external(struct bu_external *ep);

BU_EXPORT extern void bu_copy_external(struct bu_external *op,
				       const struct bu_external *ip);

/**
 * Advance pointer through string over current token,
 * across white space, to beginning of next token.
 */
BU_EXPORT extern char *bu_next_token(char *str);

BU_EXPORT extern void bu_structparse_get_terse_form(struct bu_vls *logstr,
						    const struct bu_structparse *sp);

/**
 * Support routine for db adjust and db put.  Much like the
 * bu_struct_parse routine which takes its input as a bu_vls. This
 * routine, however, takes the arguments as lists, a more Tcl-friendly
 * method. There is a log vls for storing messages.
 *
 * Operates on argv[0] and argv[1], then on argv[2] and argv[3], ...
 *
 * @param str  - vls for dumping info that might have gone to bu_log
 * @param argc - number of elements in argv
 * @param argv - contains the keyword-value pairs
 * @param desc - structure description
 * @param base - base addr of users struct
 *
 * @retval BRLCAD_OK if successful,
 * @retval BRLCAD_ERROR on failure
 */
BU_EXPORT extern int bu_structparse_argv(struct bu_vls *str,
					 int argc,
					 const char **argv,
					 const struct bu_structparse *desc,
					 char *base);

/**
 * Skip the separator(s) (i.e. whitespace and open-braces)
 *
 * @param _cp	- character pointer
 */
#define BU_SP_SKIP_SEP(_cp) { \
	while (*(_cp) && (*(_cp) == ' ' || *(_cp) == '\n' || \
			  *(_cp) == '\t' || *(_cp) == '{'))  ++(_cp); \
    }


/** @file libbu/booleanize.c
 *
 * routines for parsing boolean values from strings
 */

/**
 * Returns truthfully if a given input string represents an
 * "affirmative string".
 *
 * Input values that are null, empty, begin with the letter 'n', or
 * are 0-valued return as false.  Any other input value returns as
 * true.  Strings that strongly indicate true return as 1, other
 * values still return as true but may be a value greater than 1.
 */
BU_EXPORT extern int bu_str_true(const char *str);

/**
 * Returns truthfully if a given input string represents a
 * "negative string".
 *
 * Input values that are null, empty, begin with the letter 'n', or
 * are 0-valued return as true.  Any other input value returns as
 * false.
 */
BU_EXPORT extern int bu_str_false(const char *str);


/** @} */

/** @addtogroup thread */
/** @ingroup parallel */
/** @{ */

/** @file libbu/semaphore.c
 *
 * semaphore implementation
 *
 * Machine-specific routines for parallel processing.  Primarily for
 * handling semaphores to protect critical sections of code.
 *
 * The new paradigm: semaphores are referred to, not by a pointer, but
 * by a small integer.  This module is now responsible for obtaining
 * whatever storage is needed to implement each semaphore.
 *
 * Note that these routines can't use bu_log() for error logging,
 * because bu_log() acquires semaphore #0 (BU_SEM_SYSCALL).
 */

/*
 * Section for manifest constants for bu_semaphore_acquire()
 */
#define BU_SEM_SYSCALL 0
#define BU_SEM_LISTS 1
#define BU_SEM_BN_NOISE 2
#define BU_SEM_MAPPEDFILE 3
#define BU_SEM_THREAD 4
#define BU_SEM_MALLOC 5
#define BU_SEM_DATETIME 6
#define BU_SEM_LAST (BU_SEM_DATETIME+1)	/* allocate this many for LIBBU+LIBBN */
/*
 * Automatic restart capability in bu_bomb().  The return from
 * BU_SETJUMP is the return from the setjmp().  It is 0 on the first
 * pass through, and non-zero when re-entered via a longjmp() from
 * bu_bomb().  This is only safe to use in non-parallel applications.
 */
#define BU_SETJUMP setjmp((bu_setjmp_valid=1, bu_jmpbuf))
#define BU_UNSETJUMP (bu_setjmp_valid=0)
/* These are global because BU_SETJUMP must be macro.  Please don't touch. */
BU_EXPORT extern int bu_setjmp_valid;		/* !0 = bu_jmpbuf is valid */
BU_EXPORT extern jmp_buf bu_jmpbuf;			/* for BU_SETJUMP() */


/**
 * Prepare 'nsemaphores' independent critical section semaphores.  Die
 * on error.
 *
 * Takes the place of 'n' separate calls to old RES_INIT().  Start by
 * allocating array of "struct bu_semaphores", which has been arranged
 * to contain whatever this system needs.
 *
 */
BU_EXPORT extern void bu_semaphore_init(unsigned int nsemaphores);

/**
 * Release all initialized semaphores and any associated memory.
 */
BU_EXPORT extern void bu_semaphore_free(void);

/**
 * Prepare 'nsemaphores' independent critical section semaphores.  Die
 * on error.
 */
BU_EXPORT extern void bu_semaphore_reinit(unsigned int nsemaphores);

BU_EXPORT extern void bu_semaphore_acquire(unsigned int i);

BU_EXPORT extern void bu_semaphore_release(unsigned int i);

/** @} */

/** @file libbu/str.c
 *
 * Compatibility routines to various string processing functions
 * including strlcat and strlcpy.
 *
 */

/**
 * concatenate one string onto the end of another, returning the
 * length of the dst string after the concatenation.
 *
 * bu_strlcat() is a macro to bu_strlcatm() so that we can report the
 * file name and line number of any erroneous callers.
 */
BU_EXPORT extern size_t bu_strlcatm(char *dst, const char *src, size_t size, const char *label);
#define bu_strlcat(dst, src, size) bu_strlcatm(dst, src, size, BU_FLSTR)

/**
 * copies one string into another, returning the length of the dst
 * string after the copy.
 *
 * bu_strlcpy() is a macro to bu_strlcpym() so that we can report the
 * file name and line number of any erroneous callers.
 */
BU_EXPORT extern size_t bu_strlcpym(char *dst, const char *src, size_t size, const char *label);
#define bu_strlcpy(dst, src, size) bu_strlcpym(dst, src, size, BU_FLSTR)

/**
 * Given a string, allocate enough memory to hold it using
 * bu_malloc(), duplicate the strings, returns a pointer to the new
 * string.
 *
 * bu_strdup() is a macro that includes the current file name and line
 * number that can be used when bu debugging is enabled.
 */
BU_EXPORT extern char *bu_strdupm(const char *cp, const char *label);
#define bu_strdup(s) bu_strdupm(s, BU_FLSTR)

/**
 * Compares two strings for equality.  It performs the comparison more
 * robustly than the standard library's strcmp() function by defining
 * consistent behavior for NULL and empty strings.  It accepts NULL as
 * valid input values and considers "" and NULL as equal.  Returns 0
 * if the strings match.
 */
BU_EXPORT extern int bu_strcmp(const char *string1, const char *string2);

/**
 * Compares two strings for equality.  No more than n-characters are
 * compared.  It performs the comparison more robustly than the
 * standard library's strncmp() function by defining consistent
 * behavior for NULL and empty strings.  It accepts NULL as valid
 * input values and considers "" and NULL as equal.  Returns 0 if the
 * strings match.
 */
BU_EXPORT extern int bu_strncmp(const char *string1, const char *string2, size_t n);

/**
 * Compares two strings for equality without regard for the case in
 * the string.  It performs the comparison more robustly than the
 * standard strcasecmp()/stricmp() function by defining consistent
 * behavior for NULL and empty strings.  It accepts NULL as valid
 * input values and considers "" and NULL as equal.  Returns 0 if the
 * strings match.
 */
BU_EXPORT extern int bu_strcasecmp(const char *string1, const char *string2);

/**
 * Compares two strings for equality without regard for the case in
 * the string.  No more than n-characters are compared.  It performs
 * the comparison more robustly than the standard
 * strncasecmp()/strnicmp() function by defining consistent behavior
 * for NULL and empty strings.  It accepts NULL as valid input values
 * and considers "" and NULL as equal.  Returns 0 if the strings
 * match.
 */
BU_EXPORT extern int bu_strncasecmp(const char *string1, const char *string2, size_t n);

/**
 * BU_STR_EMPTY() is a convenience macro that tests a string for
 * emptiness, i.e. "" or NULL.
 */
#define BU_STR_EMPTY(s) (bu_strcmp((s), "") == 0)

/**
 * BU_STR_EQUAL() is a convenience macro for testing two
 * null-terminated strings for equality.  It is equivalent to
 * (bu_strcmp(s1, s2) == 0) whereby NULL strings are allowed and
 * equivalent to an empty string.  Evaluates true when the strings
 * match and false if they do not.
 */
#define BU_STR_EQUAL(s1, s2) (bu_strcmp((s1), (s2)) == 0)

/**
 * BU_STR_EQUIV() is a convenience macro that compares two
 * null-terminated strings for equality without regard for case.  Two
 * strings are equivalent if they are a case-insensitive match.  NULL
 * strings are allowed and equivalent to an empty string.  Evaluates
 * true if the strings are similar and false if they are not.
 */
#define BU_STR_EQUIV(s1, s2) (bu_strcasecmp((s1), (s2)) == 0)


/** @file escape.c
 *
 * These routines implement support for escaping and unescaping
 * generalized strings that may represent filesystem paths, URLs,
 * object lists, and more.
 *
 */

/**
 * Escapes an input string with preceding '\'s for any characters
 * defined in the 'expression' string.  The input string is written to the
 * specified output buffer of 'size' capacity.  The input and output
 * pointers may overlap or be the same memory (assuming adequate space
 * is available).  If 'output' is NULL, then dynamic memory will be
 * allocated and returned.
 *
 * The 'expression' parameter is a regular "bracket expression"
 * commonly used in globbing and POSIX regular expression character
 * matching.  An expression can be either a matching list (default) or
 * a non-matching list (starting with a circumflex '^' character).
 * For example, "abc" matches any of the characters 'a', 'b', or 'c'.
 * Specifying a non-matching list expression matches any character
 * except for the ones listed after the circumflex.  For example,
 * "^abc" matches any character except 'a', 'b', or 'c'.
 *
 * Backslash escape sequences are not allowed (e.g., \t or \x01) as
 * '\' will be matched literally.
 *
 * A range expression consists of two characters separated by a hyphen
 * and will match any single character between the two characters.
 * For example, "0-9a-c" is equivalent to "0123456789abc".  To match a
 * '-' dash literally, include it as the last or first (after any '^')
 * character within the expression.
 *
 * The expression may also contain named character classes but only
 * for ASCII input strings:
 *
 * [:alnum:] Alphanumeric characters: a-zA-Z0-9
 * [:alpha:] Alphabetic characters: a-zA-Z
 * [:blank:] Space and TAB characters
 * [:cntrl:] Control characters: ACSII 0x00-0X7F
 * [:digit:] Numeric characters: 0-9
 * [:graph:] Characters that are both printable and visible: ASCII 0x21-0X7E
 * [:lower:] Lowercase alphabetic characters: a-z
 * [:print:] Visible and space characters (not control characters): ASCII 0x20-0X7E
 * [:punct:] Punctuation characters (not letters, digits, control, or space): ][!"#$%&'()*+,./:;<=>?@^_`{|}~-\
 * [:upper:] Uppercase alphabetic characters: A-Z
 * [:xdigit:] Hexadecimal digits: a-fA-F0-9
 * [:word:] (non-POSIX) Alphanumeric plus underscore: a-zA-Z0-9_
 *
 * A non-NULL output string is always returned.  This allows
 * expression chaining and embedding as function arguments but care
 * should be taken to free the dynamic memory being returned when
 * 'output' is NULL.
 *
 * If output 'size' is inadequate for holding the escaped input
 * string, bu_bomb() is called.
 *
 * Example:
 *   char *result;
 *   char buf[128];
 *   result = bu_str_escape("my fair lady", " ", buf, 128);
 *   :: result == buf == "my\ fair\ lady"
 *   result = bu_str_escape(buf, "\", NULL, 0);
 *   :: result == "my\\ fair\\ lady"
 *   :: buf == "my\ fair\ lady"
 *   bu_free(result, "bu_str_escape");
 *   result = bu_str_escape(buf, "a-zA-Z", buf, 128);
 *   :: result == buf == "\m\y\ \f\a\i\r\ \l\a\d\y"
 *
 * This function should be thread safe and re-entrant if the
 * input/output buffers are not shared (and strlen() is threadsafe).
 */
BU_EXPORT extern char *bu_str_escape(const char *input, const char *expression, char *output, size_t size);

/**
 * Removes one level of '\' escapes from an input string.  The input
 * string is written to the specified output buffer of 'size'
 * capacity.  The input and output pointers may overlap or be the same
 * memory.  If 'output' is NULL, then dynamic memory will be allocated
 * and returned.
 *
 * A non-NULL output string is always returned.  This allows
 * expression chaining and embedding as function arguments but care
 * should be taken to free the dynamic memory being returned when
 * 'output' is NULL.
 *
 * If output 'size' is inadequate for holding the unescaped input
 * string, bu_bomb() is called.
 *
 * Example:
 *   char *result;
 *   char buf[128];
 *   result = bu_str_unescape("\m\y\\ \f\a\i\r\\ \l\a\d\y", buf, 128);
 *   :: result == buf == "my\ fair\ lady"
 *   result = bu_str_unescape(buf, NULL, 0);
 *   :: result == "my fair lady"
 *   :: buf == "my\ fair\ lady"
 *   bu_free(result, "bu_str_unescape");
 *
 * This function should be thread safe and re-entrant if the
 * input/output buffers are not shared (and strlen() is threadsafe).
 */
BU_EXPORT extern char *bu_str_unescape(const char *input, char *output, size_t size);


/** @} */

/** @addtogroup log */
/** @ingroup io */
/** @{ */

/** @file libbu/units.c
 *
 * Module of libbu to handle units conversion between strings and mm.
 *
 */

/**
 * Given a string representation of a unit of distance (e.g., "feet"),
 * return the multiplier which will convert that unit into the default
 * unit for the dimension (millimeters for length, mm^3 for volume,
 * and grams for mass.)
 *
 * Returns 0.0 on error and >0.0 on success
 */
BU_EXPORT extern double bu_units_conversion(const char *str);

/**
 * Given a conversion factor to mm, search the table to find what unit
 * this represents.
 *
 * To accommodate floating point fuzz, a "near miss" is allowed.  The
 * algorithm depends on the table being sorted small-to-large.
 *
 * Returns -
 * char* units string
 * NULL	No known unit matches this conversion factor.
 */
BU_EXPORT extern const char *bu_units_string(const double mm);
BU_EXPORT extern struct bu_vls *bu_units_strings_vls(void);

/**
 * Given a conversion factor to mm, search the table to find the
 * closest matching unit.
 *
 * Returns -
 * char* units string
 * NULL	Invalid conversion factor (non-positive)
 */
BU_EXPORT extern const char *bu_nearest_units_string(const double mm);

/**
 * Given a string of the form "25cm" or "5.2ft" returns the
 * corresponding distance in mm.
 *
 * Returns -
 * -1 on error
 * >0 on success
 */
BU_EXPORT extern double bu_mm_value(const char *s);

/**
 * Used primarily as a hooked function for bu_structparse tables to
 * allow input of floating point values in other units.
 */
BU_EXPORT extern void bu_mm_cvt(const struct bu_structparse *sdp,
				const char *name,
				void *base,
				const char *value);

/** @} */

/** @addtogroup hton */
/** @ingroup data */
/** @{ */
/** @file libbu/htester.c
 *
 * @brief
 * Test network float conversion.
 *
 * Expected to be used in pipes, or with TTCP links to other machines,
 * or with files RCP'ed between machines.
 *
 */

/** @file libbu/xdr.c
 *
 * DEPRECATED.
 *
 * Routines to implement an external data representation (XDR)
 * compatible with the usual InterNet standards, e.g.:
 * big-endian, twos-compliment fixed point, and IEEE floating point.
 *
 * Routines to insert/extract short/long's into char arrays,
 * independent of machine byte order and word-alignment.
 * Uses encoding compatible with routines found in libpkg,
 * and BSD system routines htonl(), htons(), ntohl(), ntohs().
 *
 */

/**
 * DEPRECATED: use ntohll()
 * Macro version of library routine bu_glonglong()
 * The argument is expected to be of type "unsigned char *"
 */
#define BU_GLONGLONG(_cp)	\
    ((((uint64_t)((_cp)[0])) << 56) |	\
     (((uint64_t)((_cp)[1])) << 48) |	\
     (((uint64_t)((_cp)[2])) << 40) |	\
     (((uint64_t)((_cp)[3])) << 32) |	\
     (((uint64_t)((_cp)[4])) << 24) |	\
     (((uint64_t)((_cp)[5])) << 16) |	\
     (((uint64_t)((_cp)[6])) <<  8) |	\
     ((uint64_t)((_cp)[7])))
/**
 * DEPRECATED: use ntohl()
 * Macro version of library routine bu_glong()
 * The argument is expected to be of type "unsigned char *"
 */
#define BU_GLONG(_cp)	\
    ((((uint32_t)((_cp)[0])) << 24) |	\
     (((uint32_t)((_cp)[1])) << 16) |	\
     (((uint32_t)((_cp)[2])) <<  8) |	\
     ((uint32_t)((_cp)[3])))
/**
 * DEPRECATED: use ntohs()
 * Macro version of library routine bu_gshort()
 * The argument is expected to be of type "unsigned char *"
 */
#define BU_GSHORT(_cp)	\
    ((((uint16_t)((_cp)[0])) << 8) | \
     (_cp)[1])

/**
 * DEPRECATED: use ntohs()
 */
DEPRECATED BU_EXPORT extern uint16_t bu_gshort(const unsigned char *msgp);

/**
 * DEPRECATED: use ntohl()
 */
DEPRECATED BU_EXPORT extern uint32_t bu_glong(const unsigned char *msgp);

/**
 * DEPRECATED: use htons()
 */
DEPRECATED BU_EXPORT extern unsigned char *bu_pshort(unsigned char *msgp, uint16_t s);

/**
 * DEPRECATED: use htonl()
 */
DEPRECATED BU_EXPORT extern unsigned char *bu_plong(unsigned char *msgp, uint32_t l);

/**
 * DEPRECATED: use htonll()
 */
DEPRECATED BU_EXPORT extern unsigned char *bu_plonglong(unsigned char *msgp, uint64_t l);

/** @} */

/** @addtogroup tcl */
/** @ingroup binding */
/** @{ */
/** @file libbu/observer.c
 *
 * @brief
 * Routines for implementing the observer pattern.
 *
 */

/**
 * runs a given command, calling the corresponding observer callback
 * if it matches.
 */
BU_EXPORT extern int bu_observer_cmd(void *clientData, int argc, const char *argv[]);

/**
 * Notify observers.
 */
BU_EXPORT extern void bu_observer_notify(Tcl_Interp *interp, struct bu_observer *headp, char *self);

/**
 * Free observers.
 */
BU_EXPORT extern void bu_observer_free(struct bu_observer *);

/**
 * Bu_Init
 *
 * Allows LIBBU to be dynamically loaded to a vanilla tclsh/wish with
 * "load /usr/brlcad/lib/libbu.so"
 *
 * @param interp	- tcl interpreter in which this command was registered.
 *
 * @return BRLCAD_OK if successful, otherwise, BRLCAD_ERROR.
 */
BU_EXPORT extern int Bu_Init(void *interp);


/** @} */

/** @addtogroup log */
/** @ingroup io */
/** @{ */
/** @file libbu/lex.c
 *
 */

#define BU_LEX_ANY 0	/* pseudo type */
struct bu_lex_t_int {
    int type;
    int value;
};
#define BU_LEX_INT 1
struct bu_lex_t_dbl {
    int type;
    double value;
};
#define BU_LEX_DOUBLE 2
struct bu_lex_t_key {
    int type;
    int value;
};
#define BU_LEX_SYMBOL 3
#define BU_LEX_KEYWORD 4
struct bu_lex_t_id {
    int type;
    char *value;
};
#define BU_LEX_IDENT 5
#define BU_LEX_NUMBER 6	/* Pseudo type */
union bu_lex_token {
    int type;
    struct bu_lex_t_int t_int;
    struct bu_lex_t_dbl t_dbl;
    struct bu_lex_t_key t_key;
    struct bu_lex_t_id t_id;
};
struct bu_lex_key {
    int tok_val;
    char *string;
};
#define BU_LEX_NEED_MORE 0


BU_EXPORT extern int bu_lex(union bu_lex_token *token,
			    struct bu_vls *rtstr,
			    struct bu_lex_key *keywords,
			    struct bu_lex_key *symbols);


/** @file libbu/mread.c
 *
 * multiple-read to fill a buffer
 *
 * Provide a general means to a read some count of items from a file
 * descriptor reading multiple times until the quantity desired is
 * obtained.  This is useful for pipes and network connections that
 * don't necessarily deliver data with the same grouping as it is
 * written with.
 *
 * If a read error occurs, a negative value will be returns and errno
 * should be set (by read()).
 *
 */

/**
 * "Multiple try" read.  Read multiple times until quantity is
 * obtained or an error occurs.  This is useful for pipes.
 */
BU_EXPORT extern long int bu_mread(int fd, void *bufp, long int n);

/** @} */

/** @addtogroup hash */
/** @ingroup container */
/** @{ */
/** @file libbu/hash.c
 *
 * @brief
 * An implementation of hash tables.
 */

/**
 * A hash entry
 */
struct bu_hash_entry {
    uint32_t magic;
    unsigned char *key;
    unsigned char *value;
    int key_len;
    struct bu_hash_entry *next;
};
typedef struct bu_hash_entry bu_hash_entry_t;
#define BU_HASH_ENTRY_NULL ((struct bu_hash_entry *)0)

/**
 * asserts the integrity of a non-head node bu_hash_entry struct.
 */
#define BU_CK_HASH_ENTRY(_ep) BU_CKMAG(_ep, BU_HASH_ENTRY_MAGIC, "bu_hash_entry")

/**
 * initializes a bu_hash_entry struct without allocating any memory.
 */
#define BU_HASH_ENTRY_INIT(_h) { \
	(_h)->magic = BU_HASH_ENTRY_MAGIC; \
	(_h)->key = (_h)->value = NULL; \
	(_h)->key_len = 0; \
	(_h)->next = NULL; \
    }

/**
 * macro suitable for declaration statement initialization of a
 * bu_hash_entry struct.  does not allocate memory.
 */
#define BU_HASH_ENTRY_INIT_ZERO { BU_HASH_ENTRY_MAGIC, NULL, NULL, 0, NULL }

/**
 * returns truthfully whether a bu_hash_entry has been initialized.
 */
#define BU_HASH_ENTRY_IS_INITIALIZED(_h) (((struct bu_hash_entry *)(_h) != BU_HASH_ENTRY_NULL) && LIKELY((_h)->magic == BU_HASH_ENTRY_MAGIC))


/**
 * A table of hash entries
 */
struct bu_hash_tbl {
    uint32_t magic;
    unsigned long mask;
    unsigned long num_lists;
    unsigned long num_entries;
    struct bu_hash_entry **lists;
};
typedef struct bu_hash_tbl bu_hash_tbl_t;
#define BU_HASH_TBL_NULL ((struct bu_hash_tbl *)0)

/**
 * asserts the integrity of a non-head node bu_hash_tbl struct.
 */
#define BU_CK_HASH_TBL(_hp) BU_CKMAG(_hp, BU_HASH_TBL_MAGIC, "bu_hash_tbl")

/**
 * initializes a bu_hash_tbl struct without allocating any memory.
 */
#define BU_HASH_TBL_INIT(_h) { \
	(_h)->magic = BU_HASH_TBL_MAGIC; \
	(_h)->mask = (_h)->num_lists = (_h)->num_entries = 0; \
	(_h)->lists = NULL; \
    }

/**
 * macro suitable for declaration statement initialization of a
 * bu_hash_tbl struct.  does not allocate memory.
 */
#define BU_HASH_TBL_INIT_ZERO { BU_HASH_TBL_MAGIC, 0, 0, 0, NULL }

/**
 * returns truthfully whether a bu_hash_tbl has been initialized.
 */
#define BU_HASH_TBL_IS_INITIALIZED(_h) (((struct bu_hash_tbl *)(_h) != BU_HASH_TBL_NULL) && LIKELY((_h)->magic == BU_HASH_TBL_MAGIC))


/**
 * A hash table entry record
 */
struct bu_hash_record {
    uint32_t magic;
    const struct bu_hash_tbl *tbl;
    unsigned long index;
    struct bu_hash_entry *hsh_entry;
};
typedef struct bu_hash_record bu_hash_record_t;
#define BU_HASH_RECORD_NULL ((struct bu_hash_record *)0)

/**
 * asserts the integrity of a non-head node bu_hash_record struct.
 */
#define BU_CK_HASH_RECORD(_rp) BU_CKMAG(_rp, BU_HASH_RECORD_MAGIC, "bu_hash_record")

/**
 * initializes a bu_hash_record struct without allocating any memory.
 */
#define BU_HASH_RECORD_INIT(_h) { \
	(_h)->magic = BU_HASH_RECORD_MAGIC; \
	(_h)->tbl = NULL; \
	(_h)->index = 0; \
	(_h)->hsh_entry = NULL; \
    }

/**
 * macro suitable for declaration statement initialization of a
 * bu_hash_record struct.  does not allocate memory.
 */
#define BU_HASH_RECORD_INIT_ZERO { BU_HASH_RECORD_MAGIC, NULL, 0, NULL }

/**
 * returns truthfully whether a bu_hash_record has been initialized.
 */
#define BU_HASH_RECORD_IS_INITIALIZED(_h) (((struct bu_hash_record *)(_h) != BU_HASH_RECORD_NULL) && LIKELY((_h)->magic == BU_HASH_RECORD_MAGIC))


/**
 * the hashing function
 */
BU_EXPORT extern unsigned long bu_hash(const unsigned char *str,
				       int len);

/**
 * Create an empty hash table
 *
 * The input is the number of desired hash bins.  This number will be
 * rounded up to the nearest power of two.
 */
BU_EXPORT extern struct bu_hash_tbl *bu_hash_tbl_create(unsigned long tbl_size);

/**
 * Find the hash table entry corresponding to the provided key
 *
 * @param[in] hsh_tbl - The hash table to look in
 * @param[in] key - the key to look for
 * @param[in] key_len - the length of the key in bytes
 *
 * Output:
 * @param[out] prev - the previous hash table entry (non-null for entries that not the first in hash bin)
 * @param[out] idx - the index of the hash bin for this key
 *
 * @return
 * the hash table entry corresponding to the provided key, or NULL if
 * not found.
 */
BU_EXPORT extern struct bu_hash_entry *bu_hash_tbl_find(const struct bu_hash_tbl *hsh_tbl,
							const unsigned char *key,
							int key_len,
							struct bu_hash_entry **prev,
							unsigned long *idx);

/**
 * Set the value for a hash table entry
 *
 * Note that this is just a pointer copy, the hash table does not
 * maintain its own copy of this value.
 */
BU_EXPORT extern void bu_set_hash_value(struct bu_hash_entry *hsh_entry,
					unsigned char *value);

/**
 * get the value pointer stored for the specified hash table entry
 */
BU_EXPORT extern unsigned char *bu_get_hash_value(const struct bu_hash_entry *hsh_entry);

/**
 * get the key pointer stored for the specified hash table entry
 */
BU_EXPORT extern unsigned char *bu_get_hash_key(const struct bu_hash_entry *hsh_entry);

/**
 * Add an new entry to a hash table
 *
 * @param[in] hsh_tbl - the hash table to accept the new entry
 * @param[in] key - the key (any byte string)
 * @param[in] key_len - the number of bytes in the key
 *
 * @param[out] new_entry - a flag, non-zero indicates that a new entry
 * was created.  zero indicates that an entry already exists with the
 * specified key and key length
 *
 * @return
 * a hash table entry. If "new" is non-zero, a new, empty entry is
 * returned.  if "new" is zero, the returned entry is the one matching
 * the specified key and key_len.
 */
BU_EXPORT extern struct bu_hash_entry *bu_hash_tbl_add(struct bu_hash_tbl *hsh_tbl,
						       const unsigned char *key,
						       int key_len,
						       int *new_entry);

/**
 * Print the specified hash table to stderr.
 *
 * (Note that the keys and values are printed as pointers)
 */
BU_EXPORT extern void bu_hash_tbl_print(const struct bu_hash_tbl *hsh_tbl,
					const char *str);

/**
 * Free all the memory associated with the specified hash table.
 *
 * Note that the keys are freed (they are copies), but the "values"
 * are not freed.  (The values are merely pointers)
 */
BU_EXPORT extern void bu_hash_tbl_free(struct bu_hash_tbl *hsh_tbl);

/**
 * get the "first" entry in a hash table
 *
 * @param[in] hsh_tbl - the hash table of interest
 * @param[in] rec - an empty "bu_hash_record" structure for use by this routine and "bu_hash_tbl_next"
 *
 * @return
 * the first non-null entry in the hash table, or NULL if there are no
 * entries (Note that the order of entries is not likely to have any
 * significance)
 */
BU_EXPORT extern struct bu_hash_entry *bu_hash_tbl_first(const struct bu_hash_tbl *hsh_tbl,
							 struct bu_hash_record *rec);

/**
 * get the "next" entry in a hash table
 *
 * input:
 * rec - the "bu_hash_record" structure that was passed to
 * "bu_hash_tbl_first"
 *
 * return:
 * the "next" non-null hash entry in this hash table
 */
BU_EXPORT extern struct bu_hash_entry *bu_hash_tbl_next(struct bu_hash_record *rec);

/**
 * Pass each table entry to a supplied function, along with an
 * additional argument (which may be NULL).
 *
 * Returns when func returns !0 or every entry has been visited.
 *
 * Example, freeing all memory associated with a table whose values
 * are dynamically allocated ints:
 @code
 static int
 free_entry(struct bu_hash_entry *entry, void *UNUSED(arg))
 {
     bu_free(bu_get_hash_value(entry), "table value");
     return 0;
 }

 bu_hash_table_traverse(table, free_entry, NULL);
 bu_hash_table_free(table);
 @endcode
 *
 * @return
 * If func returns !0 for an entry, that entry is returned.
 * Otherwise NULL is returned.
 */
BU_EXPORT extern struct bu_hash_entry *bu_hash_tbl_traverse(struct bu_hash_tbl *hsh_tbl, int (*func)(struct bu_hash_entry *, void *), void *func_arg);


/** @} */

/** @addtogroup file */
/** @ingroup io */
/** @{ */
/** @file libbu/fchmod.c
 *
 * Wrapper around fchmod.
 *
 */

/**
 * Portable wrapper for setting a file descriptor's permissions ala
 * fchmod().
 */
BU_EXPORT extern int bu_fchmod(int fd, unsigned long pmode);


/** @file libbu/argv.c
 *
 * Functions related to argv processing.
 *
 */

/**
 * Build argv[] array from input buffer, by splitting whitespace
 * separated "words" into null terminated strings.
 *
 * 'lim' indicates the maximum number of elements that can be stored
 * in the argv[] array not including a terminating NULL.
 *
 * The 'lp' input buffer is altered by this process.  The argv[] array
 * points into the input buffer.
 *
 * The argv[] array needs to have at least lim+1 pointers allocated
 * for lim items plus a terminating pointer to NULL.  The input buffer
 * should not be freed until argv has been freed or passes out of
 * scope.
 *
 * Returns -
 * 0	no words in input
 * argc	number of words of input, now in argv[]
 */
BU_EXPORT extern size_t bu_argv_from_string(char *argv[],
					    size_t lim,
					    char *lp);

/**
 * Deallocate all strings in a given argv array and the array itself
 *
 * This call presumes the array has been allocated with bu_dup_argv()
 * or bu_argv_from_path().
 */
BU_EXPORT extern void bu_free_argv(int argc, char *argv[]);

/**
 * free up to argc elements of memory allocated to an array without
 * free'ing the array itself.
 */
BU_EXPORT extern void bu_free_array(int argc, char *argv[], const char *str);

/**
 * Dynamically duplicate an argv array and all elements in the array
 *
 * Duplicate an argv array by duplicating all strings and the array
 * itself.  It is the caller's responsibility to free the array
 * returned including all elements in the array by calling bu_free()
 * or bu_free_argv().
 */
BU_EXPORT extern char **bu_dup_argv(int argc, const char *argv[]);

/**
 * Combine two argv arrays into one new (duplicated) argv array.
 *
 * If insert is negative, the insertArgv array elements will be
 * prepended into the new argv array.  If insert is greater than or
 * equal to argc, the insertArgv array elements will be appended after
 * all duplicated elements in the specified argv array.  Otherwise,
 * the insert argument is the position where the insertArgv array
 * elements will be merged with the specified argv array.
 */
BU_EXPORT extern char **bu_dupinsert_argv(int insert, int insertArgc, const char *insertArgv[], int argc, const char *argv[]);

/**
 * Generate an argv array from a path
 *
 * Given a path string, separate the path elements into a dynamically
 * allocated argv array with the path separators removed.  It is the
 * caller's responsibility to free the array that is returned as well
 * as all elements in the array using bu_free_argv() or manually
 * calling bu_free().
 */
BU_EXPORT extern char **bu_argv_from_path(const char *path, int *ac);


/** @file libbu/interrupt.c
 *
 * Routines for managing signals.  In particular, provide a common
 * means to temporarily buffer processing a signal during critical
 * write operations.
 *
 */

/**
 * Defer signal processing and interrupts before critical sections.
 *
 * Signal processing for a variety of signals that would otherwise
 * disrupt the logic of an application are put on hold until
 * bu_restore_interrupts() is called.
 *
 * If an interrupt signal is received while suspended, it will be
 * raised when/if interrupts are restored.
 *
 * Returns 0 on success.
 * Returns non-zero on error (with perror set if signal() failure).
 */
BU_EXPORT extern int bu_suspend_interrupts(void);

/**
 * Resume signal processing and interrupts after critical sections.
 *
 * If a signal was raised since bu_suspend_interrupts() was called,
 * the previously installed signal handler will be immediately called
 * albeit only once even if multiple signals were received.
 *
 * Returns 0 on success.
 * Returns non-zero on error (with perror set if signal() failure).
 */
BU_EXPORT extern int bu_restore_interrupts(void);

/** @} */

/** @addtogroup file */
/** @ingroup io */
/** @{ */
/** @file libbu/simd.c
 * Detect SIMD type at runtime.
 */

#define BU_SIMD_SSE4_2 7
#define BU_SIMD_SSE4_1 6
#define BU_SIMD_SSE3 5
#define BU_SIMD_ALTIVEC 4
#define BU_SIMD_SSE2 3
#define BU_SIMD_SSE 2
#define BU_SIMD_MMX 1
#define BU_SIMD_NONE 0
/**
 * Detect SIMD capabilities at runtime.
 */
BU_EXPORT extern int bu_simd_level(void);

/**
 * Detect if requested SIMD capabilities are available at runtime.
 * Returns 1 if they are, 0 if they are not.
 */
BU_EXPORT extern int bu_simd_supported(int level);

/** @} */

/** @addtogroup file */
/** @ingroup io */
/** @{ */
/** @file libbu/timer.c
 * Return microsecond accuracy time information.
 */
BU_EXPORT extern int64_t bu_gettime(void);

/** @} */

/** @addtogroup file */
/** @ingroup io */
/** @{ */
/**
 * Evaluate the time_t input as UTC time in ISO format.
 *
 * The UTC time is written into the user-provided bu_vls struct and is
 * also returned and guaranteed to be a non-null result, returning a
 * static "NULL" UTC time if an error is encountered.
 */
BU_EXPORT void bu_utctime(struct bu_vls *utc_result, const int64_t time_val);

/** @} */

/** @addtogroup file */
/** @ingroup io */
/** @{ */
/** @file libbu/dlfcn.c
 * Dynamic Library functionality
 */
#ifdef HAVE_DLOPEN
# define BU_RTLD_LAZY RTLD_LAZY
# define BU_RTLD_NOW RTLD_NOW
# define BU_RTLD_GLOBAL RTLD_GLOBAL
# define BU_RTLD_LOCAL RTLD_LOCAL
#else
# define BU_RTLD_LAZY 1
# define BU_RTLD_NOW 2
# define BU_RTLD_GLOBAL 0x100
# define BU_RTLD_LOCAL 0
#endif
BU_EXPORT extern void *bu_dlopen(const char *path, int mode);
BU_EXPORT extern void *bu_dlsym(void *path, const char *symbol);
BU_EXPORT extern int bu_dlclose(void *handle);
BU_EXPORT extern const char *bu_dlerror(void);

/** NEW: Do not use. */
BU_EXPORT extern int bu_fseek(FILE *stream, off_t offset, int origin);
/** NEW: Do not use. */
BU_EXPORT extern off_t bu_ftell(FILE *stream);

/** @} file */


/** @file libbu/ctype.c
 *
 * Routines for checking ctypes.
 *
 */
BU_EXPORT extern int bu_str_isprint(const char *cp);

/**
 * Get the current operating host's name.  This is usually also the
 * network name of the current host.  The name is written into the
 * provided hostname buffer of at least len size.  The hostname is
 * always null-terminated and should be sized accordingly.
 */
BU_EXPORT extern int bu_gethostname(char *hostname, size_t len);


/** @addtogroup file */
/** @ingroup io */
/** @{ */
/** @file libbu/sort.c
 * platform-independent re-entrant version of qsort, where the first argument
 * is the array to sort, the second the number of elements inside the array, the
 * third the size of one element, the fourth the comparison-function and the
 * fifth a variable which is handed as a third argument to the comparison-function.
 */
BU_EXPORT extern void bu_sort(void *array, size_t nummemb, size_t sizememb,
            int (*compare)(const void *, const void *, void *), void *context);
/** @} */

__END_DECLS

#endif  /* BU_H */

/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
