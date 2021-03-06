/*                      S S A M P - B W . C
 * BRL-CAD
 *
 * Copyright (c) 2004-2014 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */
/** @file rttherm/ssamp-bw.c
 *
 * Program to convert spectral sample data into a single-channel
 * monochrome image.
 *
 */

#include "common.h"

#include <stdlib.h>
#include <math.h>
#include "bio.h"

#include "bu/getopt.h"
#include "bu/debug.h"
#include "vmath.h"
#include "bn.h"
#include "raytrace.h"
#include "spectrum.h"
#include "optical.h"


int verbose = 0;

int width = 64;
int height = 64;
int nwave = 2;

char *datafile_basename = NULL;
char spectrum_name[100];

struct bn_tabdata *data;		/* a big array */

struct bn_tabdata *filt;		/* filter kernel */

fastf_t *pixels;			/* single values */

fastf_t forced_minval = -1;
fastf_t forced_maxval = -1;

fastf_t computed_minval;
fastf_t computed_maxval;

fastf_t lower_wavelen = -1;
fastf_t upper_wavelen = -1;

static char usage[] = "\
Usage: ssamp-bw [-s squarefilesize] [-w file_width] [-n file_height]\n\
		[-l lower_wavelen] [-u upper_wavelen] [-v]\n\
		[-m minval] [-M maxval]\n\
		file.ssamp\n";


int
get_args(int argc, char **argv)
{
    int c;

    while ((c = bu_getopt(argc, argv, "vs:w:n:l:u:m:M:")) != -1) {
	switch (c) {
	    case 'v':
		verbose++;
		break;
	    case 's':
		/* square file size */
		height = width = atoi(bu_optarg);
		break;
	    case 'w':
		width = atoi(bu_optarg);
		break;
	    case 'n':
		height = atoi(bu_optarg);
		break;
	    case 'l':
		lower_wavelen = atof(bu_optarg);
		break;
	    case 'u':
		upper_wavelen = atof(bu_optarg);
		break;
	    case 'm':
		forced_minval = atof(bu_optarg);
		break;
	    case 'M':
		forced_maxval = atof(bu_optarg);
		break;

	    default:		/* '?' */
		return 0;
	}
    }

    if (bu_optind >= argc) return 0;
    return 1;	/* OK */
}


void
find_minmax(void)
{
    fastf_t max, min;
    int i;

    max = -INFINITY;
    min =  INFINITY;

    for (i = width * height - 1; i >= 0; i--) {
	fastf_t v;

	if ((v = pixels[i]) > max) max = v;
	if (v < min) min = v;
    }
    computed_maxval = max;
    computed_minval = min;
}


int
main(int argc, char **argv)
{
    int i;
    fastf_t scale;

    if (!get_args(argc, argv)) {
	(void)fputs(usage, stderr);
	return 1;
    }

    if (verbose) bu_debug = BU_DEBUG_COREDUMP;

    datafile_basename = bu_realpath(argv[bu_optind], NULL);
    if (BU_STR_EQUAL(datafile_basename, ""))
	datafile_basename = bu_strdup("ssamp-bw");

    /* Read spectrum definition */
    snprintf(spectrum_name, 100, "%s.spect", datafile_basename);
    if (!bu_file_exists(spectrum_name, NULL)) {
	bu_free(datafile_basename, "datafile_basename realpath");
	bu_exit(EXIT_FAILURE, "Spectrum file [%s] does not exist\n", spectrum_name);
    }

    spectrum = (struct bn_table *)bn_table_read(spectrum_name);
    if (spectrum == NULL) {
	bu_free(datafile_basename, "datafile_basename realpath");
	bu_exit(EXIT_FAILURE, "ssamp-bw: Unable to read spectrum\n");
    }
    BN_CK_TABLE(spectrum);
    if (verbose) bu_log("%s defines %zu spectral samples\n", datafile_basename, spectrum->nx);
    nwave = spectrum->nx;	/* shared with Tcl */

    /* Allocate and read 2-D spectral samples array */
    data = bn_tabdata_binary_read(datafile_basename, width*height, spectrum);
    bu_free(datafile_basename, "datafile_basename realpath");
    if (!data) {
	bu_exit(EXIT_FAILURE, "bn_tabdata_binary_read() of datafile_basename failed\n");
    }

    if (lower_wavelen <= 0) lower_wavelen = spectrum->x[0];
    if (upper_wavelen <= 0) upper_wavelen = spectrum->x[spectrum->nx];

    /* Build filter to obtain portion of spectrum user wants */
    filt = bn_tabdata_mk_linear_filter(spectrum, lower_wavelen, upper_wavelen);
    if (!filt) bu_exit(EXIT_FAILURE, "bn_tabdata_mk_linear_filter() failed\n");
    if (verbose) {
	bn_pr_table("spectrum", spectrum);
	bn_pr_tabdata("filter", filt);
    }

    /* Convert each of the spectral sample curves into scalar values */
    pixels = (fastf_t *)bu_malloc(sizeof(fastf_t) * width * height, "fastf_t pixels");

    for (i = width*height-1; i >= 0; i--) {
	struct bn_tabdata *sp;
	/* Filter spectral data into a single power level */
	sp = (struct bn_tabdata *)
	    (((char *)data)+i*BN_SIZEOF_TABDATA(spectrum));
	BN_CK_TABDATA(sp);
	/* Assumes interpretation #1 of the input data (see bn.h) */
	pixels[i] = bn_tabdata_mul_area1(filt, sp);
    }

    /* Obtain min and max values of that power level */
    find_minmax();
    bu_log("computed min = %g, max=%g\n", computed_minval, computed_maxval);

    if (forced_minval < 0) forced_minval = computed_minval;
    if (forced_maxval < 0) forced_maxval = computed_maxval;
    bu_log("rescale min = %g, max=%g\n", forced_minval, forced_maxval);
    BU_ASSERT(forced_minval < forced_maxval);

    if (isatty(fileno(stdout))) {
	bu_log("ssamp-bw: Attempting to send binary output to the terminal, aborting\n");
	return 1;
    }

    /* Convert to 0..255 range and output */
    scale = 255 / (forced_maxval - forced_minval);
    for (i = 0; i < width*height; i++) {
	int val;
	val = (int)((pixels[i] - forced_minval) * scale);
	if (val > 255) val = 255;
	else if (val < 0) val = 0;
	putc(val, stdout);
    }
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
