/*
 *  Authors -
 *	John R. Anderson
 *	Susanne L. Muuss
 *	Earl P. Weaver
 *
 *  Source -
 *	VLD/ASB Building 1065
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1990 by the United States Army.
 *	All rights reserved.
 */

#include <stdio.h>
#include "machine.h"
#include "vmath.h"
#include "./iges_struct.h"

#define	STKBLK	100	/* Allocation block size */

extern int errno;
static struct node **stk;
static int jtop,stklen;

void
Initstack()
{

	jtop = (-1);
	stklen = STKBLK;
	stk = (struct node **)malloc( stklen*sizeof( struct node * ) );
	if( stk == NULL )
	{
		fprintf( stderr , "Cannot allocate stack space\n" );
		perror( "Initstack" );
		exit( 1 );
	}
}

/*  This function pushes a pointer onto the stack. */

void
Push(ptr)
struct node *ptr;
{

	jtop++;
	if( jtop == stklen )
	{
		stklen += STKBLK;
		stk = (struct node **)realloc( stk , stklen*sizeof( struct node *) );
		if( stk == NULL )
		{
			fprintf( stderr , "Cannot reallocate stack space\n" );
			perror( "Push" );
			exit( 1 );
		}
	}
	stk[jtop] = ptr;
}


/*  This function pops the top of the stack. */


struct node *
Pop()
{
	struct node *ptr;

	if( jtop == (-1) )
		ptr=NULL;
	else
	{
		ptr = stk[jtop];
		jtop--;
	}

	return(ptr);
}

void
Freestack()
{
	jtop = (-1);
	return;
}
