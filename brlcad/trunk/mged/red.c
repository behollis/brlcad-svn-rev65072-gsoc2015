/*
 *			R E D . C
 *
 *	These routines allow editing of a combination using the text editor
 *	of the users choice.
 *
 *  Author -
 *	John Anderson
 *  
 *  Source -
 *	SECAD/VLD Computing Consortium
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5066
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1992 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif


#include <stdio.h>
#include <signal.h>
#include "machine.h"
#include "vmath.h"
#include "externs.h"
#include "db.h"
#include "raytrace.h"
#include "./ged.h"
#include "./sedit.h"
#include <ctype.h>
#ifdef SYSV
# include <string.h>
#else
# include <strings.h>
#endif

extern int errno;

static char	red_tmpfil[] = "/tmp/GED.aXXXXX";
static char	red_tmpcomb[] = "red_tmp.aXXXXX";

void restore_comb();
int editit();
int clear_comb(),build_comb(),save_comb();

static int ident;
static int air;

void
f_red( argc , argv )
int argc;
char **argv;
{

	union record record;
	struct directory *dp;

	if( argc != 2 )
	{
		(void)printf( "Usage:\n\tred object_name\n" );
		return;
	}

	if( (dp=db_lookup( dbip , argv[1] , LOOKUP_NOISY )) == DIR_NULL )
	{
		(void)printf( " Cannot edit: %s\n" , argv[1] );
		return;
	}

	if( db_get( dbip , dp , &record , 0 , 1 ) < 0 ) READ_ERR_return;
	if( record.u_id != ID_COMB )	/* Not a combination */
	{
		(void)printf( " %s is not a combination, so cannot be edited this way\n", argv[1] );
		return;
	}

	/* Save for later use in rebuilding */
	ident = record.c.c_regionid;
	air = record.c.c_aircode;


	/* Make a file for the text editor */
	(void)mktemp( red_tmpfil );

	/* Write the combination components to the file */
	if( writecomb( dp ) )
	{
		(void)printf( "Unable to edit %s\n" , argv[1] );
		unlink( red_tmpfil );
		return;
	}

	/* Edit the file */
	if( editit( red_tmpfil ) )
	{
		if( checkcomb() ) /* Do some quick checking on the edited file */
		{
			(void)printf( "Error in edited region, no changes made\n" );
			(void)unlink( red_tmpfil );
			return;
		}
		if( save_comb( dp ) )	/* Save combination to a temp name */
		{
			(void)printf( "No changes made\n" );
			(void)unlink( red_tmpfil );
			return;
		}
		if( clear_comb( dp ) )	/* Empty this combination */
		{
			(void)printf( "Unable to empty %s, original restored\n" , dp->d_namep );
			restore_comb( dp );
			(void)unlink( red_tmpfil );
			return;
		}
		if( build_comb( dp ) )	/* Use comb_add() to rebuild combination */
		{
			(void)printf( "Unable to construct new %s, original restored\n" , dp->d_namep );
			restore_comb( dp );
			(void)unlink( red_tmpfil );
			return;
		}
		else			/* eliminate the temporary combination */
		{
			char linebuf[NAMESIZE + 7];

			strcpy( linebuf , "kill " );
			strcat( linebuf , red_tmpcomb );
			strcat( linebuf , "\n" );
			cmdline( linebuf );
		}
	}

	(void)unlink( red_tmpfil );
}

int
writecomb( dp )
struct directory *dp;
{
/*	Writes the file for later editing */
	union record record;
	FILE *fp;
	mat_t matrix;
	int offset,i;

	/* open the file */
	if( (fp=fopen( red_tmpfil , "w" )) == NULL )
	{
		(void)printf( "Cannot open create file for editing\n" );
		perror( "MGED" );
		return(1);
	}

	/* Get combo info and write it to file */
	for( offset=1 ; offset< dp->d_len ; offset++ )
	{
		if( db_get( dbip , dp , &record , offset , 1 ) )
		{
			fclose( fp );
			(void)printf( "Cannot get combination information\n" );
			return( 1 );
		}

		if( record.u_id != ID_MEMB )
		{
			(void)printf( "This combination appears to be corrupted\n" );
			return( 1 );
		}

		for( i=0 ; i<ELEMENTS_PER_MAT ; i++ )
		{
			if( record.M.m_mat[i] != identity[i] )
			{
				(void)printf( "Member `%s` has been object edited\n" , record.M.m_instname );
				(void)printf( "\tCombination must be `pushed` before editing\n" );
				return( 1 );
			}
		}

		fprintf( fp , " %c %s\n" , record.M.m_relation , record.M.m_instname );
	}
	fclose( fp );
	return( 0 );
}

int
checkcomb()
{
/*	Do some minor checking of the edited file */

	FILE *fp;
	int nonsubs=0;
	int i,j,done,ch;
	char relation;
	char name[NAMESIZE+1];
	char line[MAXLINE];
	struct directory *dp;

	if( (fp=fopen( red_tmpfil , "r" )) == NULL )
	{
		(void)printf( "Cannot open create file for editing\n" );
		perror( "MGED" );
		return(1);
	}

	/* Read a line at a time */
	done = 0;
	while( !done )
	{
		/* Read a line */
		i = (-1);
		while( (ch=getc( fp )) != EOF && ch != '\n' && i<MAXLINE )
			line[++i] = ch;

		if( ch == EOF )	/* We must be done */
		{
			done = 1;
			break;
		}
		if( i == MAXLINE )
		{
			(void)printf( "Line too long in edited file\n" );
			return( 1 );
		}

		line[++i] = '\0';

		/* skip leading white space */
		i = (-1);
		while( isspace( line[++i] ));

		/* First non-white is the relation operator */
		relation = line[i];
		if( relation == '\0' )
		{
			done = 1;
			break;
		}

		/* Skip more white space */
		while( isspace( line[++i] ));

		/* Next must be the member name */
		strncpy( name , &line[i] , NAMESIZE );
		name[NAMESIZE] = '\0';

		/* Eliminate trailing white space from name */
		j = NAMESIZE;
		while( isspace( name[--j] ) )
			name[j] = '\0';

		/* Skip over name */
		while( !isspace( line[++i] ) && line[i] != '\0' );

		if( line[i] != '\0' )
		{
			/* Look for junk on the tail end of the line */
			while( isspace( line[++i] ) );
			if( line[i] != '\0' )
			{
				/* found some junk */
				(void)printf( "Error in format of edited file\n" );
				(void)printf( "Must be just one operator and object per line\n" );
				fclose( fp );
				return( 1 );
			}
		}

		if( relation != '+' && relation != 'u' & relation != '-' )
		{
			(void)printf( " %c is not a legal operator\n" , relation );
			fclose( fp );
			return( 1 );
		}
		if( relation != '-' )
			nonsubs++;

		if( name[0] == '\0' )
		{
			(void)printf( " operand name missing\n" );
			fclose( fp );
			return( 1 );
		}

		if( (dp=db_lookup( dbip , name , LOOKUP_NOISY )) == DIR_NULL )
		{
			(void)printf( " %s does not exist\n" , name );
			fclose( fp );
			return( 1 );
		}
	}

	fclose( fp );

	if( nonsubs == 0 )
	{
		(void)printf( "Cannot create a combination with all subtraction operators\n" );
		return( 1 );
	}
	return( 0 );
}

int clear_comb( dp )
struct directory *dp;
{

	register int i, rec;
	union record record;

	if( dp == DIR_NULL )
		return( 1 );

	/* Delete all the Member records, one at a time */

	rec = dp->d_len;
	for( i = 1; i < rec; i++ )
	{
		if( db_get( dbip,  dp, &record, 1 , 1) < 0 )
		{
			(void)printf( "Unable to clear %s\n" , dp->d_namep );
			return( 1 );
		}

		if( db_delrec( dbip, dp, 1 ) < 0 )
		{
			(void)printf("Error in deleting member.\n");
			return( 1 );
		}
	}
	return( 0 );
}

int build_comb( dp )
struct directory *dp;
{
/*	Build the new combination by adding to the recently emptied combination
	This keeps combo info associated with this combo intact */

	FILE *fp;
	char relation;
	char name[NAMESIZE+1];
	char line[MAXLINE];
	int ch;
	int i;
	int done=0;
	int region;
	struct directory *dp1,*dp2;

	if( (fp=fopen( red_tmpfil , "r" )) == NULL )
	{
		(void)printf( " Cannot open edited file: %s\n" , red_tmpfil );
		return( 1 );
	}

	/* Will need to know whether this is a region later */
	if( dp->d_flags == DIR_REGION )
		region = 1;
	else
		region = 0;

	/* Read edited file */
	while( !done )
	{
		/* Read a line */
		i = (-1);
		while( (ch=getc( fp )) != EOF && ch != '\n' && i<MAXLINE )
			line[++i] = ch;

		if( ch == EOF )	/* We must be done */
		{
			done = 1;
			break;
		}

		line[++i] = '\0';

		/* skip leading white space */
		i = (-1);
		while( isspace( line[++i] ));

		/* First non-white is the relation operator */
		relation = line[i];
		if( relation == '\0' )
		{
			done = 1;
			break;
		}

		/* Skip more white space */
		while( isspace( line[++i] ));

		/* Next must be the member name */
		strncpy( name , &line[i] , NAMESIZE );
		name[NAMESIZE] = '\0';

		/* Eliminate trailing white space from name */
		i = NAMESIZE;
		while( isspace( name[--i] ) )
			name[i] = '\0';

		/* Check for existence of member */
		if( (dp1=db_lookup( dbip , name , LOOKUP_NOISY )) == DIR_NULL )
		{
			(void)printf( " %s does not exist\n" , name );
			return( 1 );
		}

		/* Add it to the combination */
		if( (dp2=combadd( dp1 , dp->d_namep , region , relation , ident , air ) ) == DIR_NULL )
		{
			(void)printf( " Error in rebuilding combination\n" );
			return( 1 );
		}
	}
	return( 0 );
}

void
mktemp_comb( str )
char *str;
{
/* Make a temporary name for a combination
	a template name is expected as in "mk_temp()" with 
	5 trailing X's */

	struct directory *dp;
	int counter,done;
	char *ptr;


	/* Set "ptr" to start of X's */

	ptr = str;
	while( *ptr != '\0' )
		ptr++;

	while( *(--ptr) == 'X' );
	ptr++;


	counter = 1;
	done = 0;
	while( !done && counter < 99999 )
	{
		sprintf( ptr , "%d" , counter );
		if( (dp=db_lookup( dbip , str , LOOKUP_QUIET )) == DIR_NULL )
			done = 1;
		else
			counter++;
	}
}

int save_comb( dpold )
struct directory *dpold;
{
/* Save a combination under a temporory name */

	register struct directory *dp;
	union record		*rp;

	/* Make a new name */
	mktemp_comb( red_tmpcomb );

	/* Following code is lifted from "f_copy()" and slightly modified */
	if( (rp = db_getmrec( dbip, dpold )) == (union record *)0 )
	{
		(void)printf( "Cannot save copy of %s, no changes made\n" , dp->d_namep );
		return( 1 );
	}

	if( (dp=db_diradd( dbip, red_tmpcomb, -1, dpold->d_len, dpold->d_flags)) == DIR_NULL ||
	    db_alloc( dbip, dp, dpold->d_len ) < 0 )
	{
		(void)printf( "Cannot save copy of %s, no changes made\n" , dp->d_namep );
		return( 1 );
	}

	/* All objects have name in the same place */
	NAMEMOVE( red_tmpcomb, rp->c.c_name );
	if( db_put( dbip, dp, rp, 0, dpold->d_len ) < 0 )
	{
		(void)printf( "Cannot save copy of %s, no changes made\n" , dp->d_namep );
		return( 1 );
	}

	rt_free( (char *)rp, "record" );
	return( 0 );
}

void
restore_comb( dp )
struct directory *dp;
{
/* restore a combination that was saved in "red_tmpcomb" */

	char linebuf[2*NAMESIZE + 8];
	char name[NAMESIZE];

	/* Save name of original combo */
	strcpy( name , dp->d_namep );

	/* Kill original combination */
	strcpy( linebuf , "kill " );
	strcat( linebuf , dp->d_namep );
	strcat( linebuf , "\n" );
	cmdline( linebuf );

	/* Move temp to original */
	strcpy( linebuf , "mv " );
	strcat( linebuf , red_tmpcomb );
	strcat( linebuf , " " );
	strcat( linebuf , name );
	strcat( linebuf , "\n" );

	cmdline( linebuf );
}
