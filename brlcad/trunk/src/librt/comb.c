/*
 *			C O M B . C
 *
 * XXX Move to db_tree.c when complete.
 *
 *  Authors -
 *	John R. Anderson
 *	Michael John Muuss
 *  
 *  Source -
 *	The U. S. Army Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5068  USA
 *  
 *  Distribution Notice -
 *	Re-distribution of this software is restricted, as described in
 *	your "Statement of Terms and Conditions for the Release of
 *	The BRL-CAD Package" agreement.
 *
 *  Copyright Notice -
 *	This software is Copyright (C) 1996-2004 by the United States Army
 *	in all countries except the USA.  All rights reserved.
 */
#ifndef lint
static const char RCSid[] = "@(#)$Header$ (ARL)";
#endif


#include <stdio.h>
#include <math.h>
#include <string.h>
#include "machine.h"
#include "externs.h"
#include "bu.h"
#include "vmath.h"
#include "bn.h"
#include "nmg.h"
#include "rtgeom.h"
#include "raytrace.h"
#include "../librt/debug.h"

/* --- Begin John's pretty-printer --- */

void
Print_tree(union tree *tree)
{
	char *str;

	str = (char *)rt_pr_tree_str( tree );
	if( str != NULL )
	{
		printf( "%s\n" , str );
		bu_free( str , "Print_tree" );
	}
	else
		printf( "NULL Tree\n" );
}

int
main(int argc, char *argv[])
{
	struct db_i		*dbip;
	struct directory	*dp;
	struct rt_db_internal	ip;
	struct rt_comb_internal	*comb;
	mat_t			identity_mat;
	int			i;
	struct bu_vls		file;

	bu_debug = BU_DEBUG_MEM_CHECK | BU_DEBUG_COREDUMP;

	if( argc < 3 )
	{
		fprintf( stderr , "Usage:\n\t%s db_file object1 object2 ...\n" , argv[0] );
		exit( 1 );
	}

	MAT_IDN( identity_mat );
	bu_vls_init( &file );

	if( (dbip = db_open( argv[1] , "r" ) ) == NULL )
	{
		fprintf( stderr , "Cannot open %s\n" , argv[1] );
		perror( "test" );
		exit( 1 );
	}

	rt_init_resource( &rt_uniresource, 0, NULL );

	/* Scan the database */
	db_dirbuild( dbip );

	for( i=2 ; i<argc ; i++ )
	{
		printf( "%s\n" , argv[i] );

		dp = db_lookup( dbip , argv[i] , 1 );
		if( rt_db_get_internal( &ip, dp, dbip, NULL, &rt_uniresource ) < 0 )  {
			bu_log("import of %s failed\n", dp->d_namep);
			continue;
		}

		RT_CK_DB_INTERNAL( &ip );
		bu_mem_barriercheck();

		if( ip.idb_type != ID_COMBINATION )
		{
			bu_log( "idb_type = %d\n" , ip.idb_type );
			rt_db_free_internal( &ip, &rt_uniresource );
			continue;
		}

		comb = (struct rt_comb_internal *)ip.idb_ptr;
		RT_CK_COMB(comb);
		if( comb->region_flag )
		{
			bu_log( "\tRegion id = %d, aircode = %d GIFTmater = %d, los = %d\n",
				comb->region_id, comb->aircode, comb->GIFTmater, comb->los );
		}
		bu_log( "\trgb_valid = %d, color = %d/%d/%d\n" , comb->rgb_valid , V3ARGS( comb->rgb ) );
		bu_log( "\tshader = %s (%s)\n" ,
				bu_vls_addr( &comb->shader ),
				bu_vls_addr( &comb->material )
		);

		/* John's way */
		bu_log( "Pretty print:\n" );
		Print_tree( comb->tree );

		/* Standard way */
		bu_log( "Standard print:\n" );
		rt_pr_tree( comb->tree, 1 );

		/* Compact way */
		{
			struct bu_vls	str;
			bu_vls_init( &str );
			rt_pr_tree_vls( &str, comb->tree );
			bu_log("%s\n", bu_vls_addr(&str) );
			bu_vls_free(&str );
		}

		/* Test the support routines */
		if( db_ck_v4gift_tree( comb->tree ) < 0 )
			bu_log("ERROR: db_ck_v4gift_tree is unhappy\n");

		/* Test the lumberjacks */
		rt_db_free_internal( &ip, &rt_uniresource );

	}

	return 0;
}
