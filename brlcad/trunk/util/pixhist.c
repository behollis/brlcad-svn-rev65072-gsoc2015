/*
 *		P I X H I S T . C
 *
 * Display a color histogram of a pix file.
 * 0 is top of screen, 255 bottom.
 *
 *  Author -
 *	Phillip Dykstra
 *	16 June 1986
 *  
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5066
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1986 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include "fb.h"

#define	IBUFSIZE 3*2048		/* Max read size in rgb pixels */
unsigned char ibuf[IBUFSIZE];	/* Input buffer */

long	bin_r[256];		/* Histogram bins */
long	bin_g[256];
long	bin_b[256];
int	verbose = 0;

FBIO	*fbp;

static char *Usage = "usage: pixhist [-v] [file.pix]\n";

long	max;
double	scale;
Pixel	line[512];
FILE	*fp;

main( argc, argv )
int argc;
char **argv;
{
	register int i;

	/* check for verbose flag */
	if( argc > 1 && strcmp(argv[1], "-v") == 0 ) {
		verbose++;
		argv++;
		argc--;
	}

	/* look for optional input file */
	if( argc > 1 ) {
		if( (fp = fopen(argv[1],"r")) == 0 ) {
			fprintf( stderr, "pixhist: can't open \"%s\"\n", argv[1] );
			exit( 1 );
		}
		argv++;
		argc--;
	} else
		fp = stdin;

	/* check usage */
	if( argc > 1 || isatty(fileno(fp)) ) {
		fputs( Usage, stderr );
		exit( 1 );
	}

	while( (i = fread(&ibuf[0], sizeof(*ibuf), sizeof(ibuf), fp)) > 0 ) {
		register unsigned char *bp;
		register int j;

		bp = &ibuf[0];
		for( j = i/3; j > 0; j-- )  {
			bin_r[ *bp++ ]++;
			bin_g[ *bp++ ]++;
			bin_b[ *bp++ ]++;
		}
	}

	/* find max */
	max = 1;
	for( i = 0; i < 256; i++ ) {
		if( bin_r[i] > max ) max = bin_r[i];
		if( bin_g[i] > max ) max = bin_g[i];
		if( bin_b[i] > max ) max = bin_b[i];
	}
	scale = 511.0 / ((double)max);

	/* Display the max? */
	printf("Max bin count=%d.  %g count/pixel\n", max, scale );

	if( (fbp = fb_open( NULL, 512, 512 )) == NULL )  {
		fprintf(stderr,"fb_open failed\n");
		exit(12);
	}

	/* Display them */
	for( i = 0; i < 256; i++ ) {
		register int j;
		register int level;
		register int npix;

		level = (int)((double)bin_r[i]*scale);
		if( level > 511 )  level = 511;
		for( j = 0; j < level; j++ )  line[j].red = 255;
		for( ; j < 512; j++ )  line[j].red = 0;
		npix = level;

		level = (int)((double)bin_g[i]*scale);
		if( level > 511 )  level = 511;
		for( j = 0; j < level; j++ )  line[j].green = 255;
		for( ; j < 512; j++ )  line[j].green = 0;
		if( level > npix )  npix = level;

		level = (int)((double)bin_b[i]*scale);
		if( level > 511 )  level = 511;
		for( j = 0; j < level; j++ )  line[j].blue = 255;
		for( ; j < 512; j++ )  line[j].blue = 0;
		if( level > npix )  npix = level;

		fb_write( fbp, 0, 2*i, line, npix );
		fb_write( fbp, 0, 2*i+1, line, npix );
		if( verbose )
			printf( "%3d: %10d %10d %10d  (%d)\n",
				i, bin_r[i], bin_g[i], bin_b[i], npix );
	}
	fb_close( fbp );
}
