/*	SCCSID	%W%	%E%	*/
/*
 *				G 2 . C
 *
 *			G E D ,   P a r t   I I
 *
 * This module deals with building an edge description of a region.
 *
 * Functions -
 *
 * Authors -
 *	Keith Applin
 *	Gary Kuehl
 *
 *	Ballistic Research Laboratory
 *	U. S. Army
 *
 *		R E V I S I O N   H I S T O R Y
 *
 *	06/01/83  MJM	Split ged2.c into g1.c and g2.c
 *
 *	09-Sep-83 DAG	Overhauled.
 *
 *	11/02/83  CMK	Modified to use g3.c module (device independence).
 *			Moved display processor FD to g3.c module
 */

#include	<math.h>
#include "ged_types.h"
#include "3d.h"
#include "commo.h"
#include "ged2.h"
#include "vmath.h"
#include "dm.h"

extern int	printf();

static void	center(), cpoint(), dwreg(), ellin(), move(), neg(), points(),
		regin(), ret_error(), solin(), solpl(), tgcin(), tplane(),
		vectors();
static int	arb(), cgarbs(), compar(), gap(), plane(), redoarb(),
		region();

static union record input;		/* Holds an object file record */

/* following variables are used in processing regions
 *  for producing edge representations
 */
#define NMEMB	100	/* max number of members in a region */
static int	nmemb = 0;		/* actual number of members */
static int	m_type[NMEMB];		/* member solid types */
static char	m_op[NMEMB];		/* member operations */
static float	m_param[NMEMB*24];	/* member parameters */
static int	memb_count = 0;		/* running count of members */
static int	param_count = 0; 	/* location in m_param[] array */

/*
 * This routine processes each member(solid) of a region.
 *
 * When the last member is processed, dwreg() is executed.
 *
 */

void
proc_reg( dp, xform, flag )
struct directory *dp;
mat_t xform;
int flag;
{
	register int i;
	static float *op;	/* Used for scanning vectors */
	static vect_t	work;		/* Vector addition work area */
	static vect_t	homog;		/* Vector/Homog.Vector conversion buffer */
	int length, type;
	int uvec[8], svec[11];
	int cgtype;

 	db_getrec( dp, &input, 0 );

#define	ip	(&input.s)

	type = ip->s_type;
	cgtype = type;

	if(type == GENARB8) {
		/* check for arb8, arb7, arb6, arb5, arb4 */
		if(input.s.s_cgtype >= 0) {
			points();
			if( (i = cgarbs(uvec, svec)) == 0 ) {
				ret_error();
				return;
			}
			if(redoarb(uvec, svec, i) == 0) {
				ret_error();
				return;
			}
			vectors();
		}
		cgtype = input.s.s_cgtype * -1;
	}
	if(cgtype == RPP || cgtype == BOX || cgtype == GENARB8)
		cgtype = ARB8;

	if(commo.o_flag == ROOT)
		m_op[memb_count] = '+';
	else
	if(commo.o_flag == 999)
		m_op[memb_count] = 'u';
	else
		m_op[memb_count] = '-';

	m_type[memb_count++] = cgtype;
	if(memb_count > NMEMB) {
		(void)printf("proc_reg: region has more than %d members\n", NMEMB);
		ret_error();
		return;
	}

	switch( cgtype )  {

	case ARB8:
		length = 8;
arbcom:		/* common area for arbs */
		MAT4X3PNT( work, commo.o_mat, ip->s_values );
		VMOVE( ip->s_values, work );
		VMOVE(&m_param[param_count], &(ip->s_values[0]));
		param_count += 3;
		op = &ip->s_values[1*3];
		for( i=1; i<length; i++ )  {
			MAT4X3VEC( work, commo.o_mat, op );
			VADD2(op, ip->s_values, work);
			VMOVE(&m_param[param_count], op);
			param_count += 3;
			op += 3;
		}
		break;

	case ARB7:
		length = 7;
		goto arbcom;

	case ARB6:
		VMOVE(&input.s.s_values[15], &input.s.s_values[18]);
		length = 6;
		goto arbcom;

	case ARB5:
		length = 5;
		goto arbcom;

	case ARB4:
		length = 4;
		VMOVE(&input.s.s_values[9], &input.s.s_values[12]);
		goto arbcom;

	case GENTGC:
		op = &ip->s_values[0*3];
		MAT4X3PNT( work, commo.o_mat, op );
		VMOVE( op, work );
		VMOVE(&m_param[param_count], op);
		param_count += 3;
		op += 3;

		for( i=1; i<6; i++ )  {
			MAT4X3VEC( work, commo.o_mat, op );
			VMOVE( op, work );
			VMOVE(&m_param[param_count], op);
			param_count += 3;
			op += 3;
		}
		break;

	case GENELL:
		op = &ip->s_values[0*3];
		MAT4X3PNT( work, commo.o_mat, op );
		VMOVE( op, work );
		VMOVE(&m_param[param_count], op);
		param_count += 3;
		op += 3;

		for( i=1; i<4; i++ )  {
			MAT4X3VEC( work, commo.o_mat, op );
			VMOVE( op, work );
			VMOVE(&m_param[param_count], op);
			param_count += 3;
			op += 3;
		}
		break;

	default:
		(void)printf("proc_reg:  Cannot draw solid type %d (%s)\n",
			type, type == TOR ? "TOR":
			 type == ARS ? "ARS" : "UNKNOWN TYPE" );
		ret_error();
		return;
	}

	if(commo.o_more == 0) {
		/* this was the last member solid - draw the region */
		nmemb = memb_count;
		param_count = memb_count = 0;
		if(nmemb == 0) {
			ret_error();
			return;
		}
		dwreg();
	} else {
		commi.i_type = MS_MORE;
	}
}

/*
 *  called to return to ged1 if any errors were
 *  found while processing a region
 */
static void
ret_error()
{

	commi.i_type = MS_ERROR;
	nmemb = param_count = memb_count = 0;
}

#define NO	0
#define YES	1

/* C G A R B S :   determines COMGEOM arb types from GED general arbs
 */
static int
cgarbs( uvec, svec )
register int *uvec;	/* array of unique points */
register int *svec;	/* array of like points */
{
	register int i,j;
	static int numuvec, unique, done;
	static int si;

	done = NO;		/* done checking for like vectors */

	svec[0] = svec[1] = 0;
	si = 2;

	for(i=0; i<7; i++) {
		unique = YES;
		if(done == NO)
			svec[si] = i;
		for(j=i+1; j<8; j++) {
			if(compar(&input.s.s_values[i*3], &input.s.s_values[j*3]) == YES) {
				if( done == NO )
					svec[++si] = j;
				unique = NO;
			}
		}
		if( unique == NO ) {  	/* point i not unique */
			if( si > 2 && si < 6 ) {
				svec[0] = si - 1;
				if(si == 5 && svec[5] >= 6)
					done = YES;
				si = 6;
			}
			if( si > 6 ) {
				svec[1] = si - 5;
				done = YES;
			}
		}
	}
	if( si > 2 && si < 6 ) 
		svec[0] = si - 1;
	if( si > 6 )
		svec[1] = si - 5;
	for(i=1; i<=svec[1]; i++)
		svec[svec[0]+1+i] = svec[5+i];
	for(i=svec[0]+svec[1]+2; i<11; i++)
		svec[i] = -1;
	/* find the unique points */
	numuvec = 0;
	for(j=0; j<8; j++) {
		unique = YES;
		for(i=2; i<svec[0]+svec[1]+2; i++) {
			if( j == svec[i] ) {
				unique = NO;
				break;
			}
		}
		if( unique == YES )
			uvec[numuvec++] = j;
	}

	/* put comgeom solid typpe into s_cgtype */
	switch( numuvec ) {

	case 8:
		input.s.s_cgtype = -8;  /* ARB8 */
		break;

	case 6:
		input.s.s_cgtype = -7;	/* ARB7 */
		break;

	case 4:
		if(svec[0] == 2)
			input.s.s_cgtype = -6;	/* ARB6 */
		else
			input.s.s_cgtype = -5;	/* ARB5 */
		break;

	case 2:
		input.s.s_cgtype = -4;	/* ARB4 */
		break;

	default:
		(void)printf("solid: %s  bad number of unique vectors (%d)\n",
			input.s.s_name, numuvec);
		return(0);
	}
	return( numuvec );
}

/*  R E D O A R B :   rearranges arbs to be GIFT compatible
 */
static int
redoarb( uvec, svec, numvec )
register int *uvec, *svec;
int numvec;
{
	register int i, j;
	static int prod, cgtype;

	cgtype = input.s.s_cgtype * -1;

	switch( cgtype ) {

	case ARB8:
		/* do nothing */
		break;

	case ARB7:
		/* arb7 vectors: 0 1 2 3 4 5 6 4 */
		switch( svec[2] ) {
			case 0:
				/* 0 = 1, 3, or 4 */
				if(svec[3] == 1)
					move(4,7,6,5,1,4,3,1);
				if(svec[3] == 3)
					move(4,5,6,7,0,1,2,0);
				if(svec[3] == 4)
					move(1,2,6,5,0,3,7,0);
				break;
			case 1:
				/* 1 = 2 or 5 */
				if(svec[3] == 2)
					move(0,4,7,3,1,5,6,1);
				if(svec[3] == 5)
					move(0,3,7,4,1,2,6,1);
				break;
			case 2:
				/* 2 = 3 or 6 */
				if(svec[3] == 3)
					move(6,5,4,7,2,1,0,2);
				if(svec[3] == 6)
					move(3,0,4,7,2,1,5,2);
				break;
			case 3:
				/* 3 = 7 */
				move(2,1,5,6,3,0,4,3);
				break;
			case 4:
				/* 4 = 5 */
				/* if 4 = 7  do nothing */
				if(svec[3] == 5)
					move(1,2,3,0,5,6,7,5);
				break;
			case 5:
				/* 5 = 6 */
				move(2,3,0,1,6,7,4,6);
				break;
			case 6:
				/* 6 = 7 */
				move(3,0,1,2,7,4,5,7);
				break;
			default:
				(void)printf("redoarb: %s - bad arb7\n",
					input.s.s_name);
				return( 0 );
			}
			break;    	/* end of ARB7 case */

		case ARB6:
			/* arb6 vectors:  0 1 2 3 4 4 6 6 */

			prod = 1;
			for(i=0; i<numvec; i++)
				prod = prod * (uvec[i] + 1);
			switch( prod ) {
			case 24:
				/* 0123 unique */
				/* 4=7 and 5=6  OR  4=5 and 6=7 */
				if(svec[3] == 7)
					move(3,0,1,2,4,4,5,5);
				else
					move(0,1,2,3,4,4,6,6);
				break;
			case 1680:
				/* 4567 unique */
				/* 0=3 and 1=2  OR  0=1 and 2=3 */
				if(svec[3] == 3)
					move(7,4,5,6,0,0,1,1);
				else
					move(4,5,6,7,0,0,2,2);
				break;
			case 160:
				/* 0473 unique */
				/* 1=2 and 5=6  OR  1=5 and 2=6 */
				if(svec[3] == 2)
					move(0,3,7,4,1,1,5,5);
				else
					move(4,0,3,7,1,1,2,2);
				break;
			case 672:
				/* 3267 unique */
				/* 0=1 and 4=5  OR  0=4 and 1=5 */
				if(svec[3] == 1)
					move(3,2,6,7,0,0,4,4);
				else
					move(7,3,2,6,0,0,1,1);
				break;
			case 252:
				/* 1256 unique */
				/* 0=3 and 4=7  OR 0=4 and 3=7 */
				if(svec[3] == 3)
					move(1,2,6,5,0,0,4,4);
				else
					move(5,1,2,6,0,0,3,3);
				break;
			case 60:
				/* 0154 unique */
				/* 2=3 and 6=7  OR  2=6 and 3=7 */
				if(svec[3] == 3)
					move(0,1,5,4,2,2,6,6);
				else
					move(5,1,0,4,2,2,3,3);
				break;
			default:
				(void)printf("redoarb: %s: bad arb6\n",
					input.s.s_name);
				return( 0 );
			}
			break; 		/* end of ARB6 case */

		case ARB5:
			/* arb5 vectors:  0 1 2 3 4 4 4 4 */
			prod = 1;
			for(i=2; i<6; i++)
				prod = prod * (svec[i] + 1);
			switch( prod ) {
			case 24:
				/* 0=1=2=3 */
				move(4,5,6,7,0,0,0,0);
				break;
			case 1680:
				/* 4=5=6=7 */
				/* do nothing */
				break;
			case 160:
				/* 0=3=4=7 */
				move(1,2,6,5,0,0,0,0);
				break;
			case 672:
				/* 2=3=7=6 */
				move(0,1,5,4,2,2,2,2);
				break;
			case 252:
				/* 1=2=5=6 */
				move(0,3,7,4,1,1,1,1);
				break;
			case 60:
				/* 0=1=5=4 */
				move(3,2,6,7,0,0,0,0);
				break;
			default:
				(void)printf("redoarb: %s: bad arb5\n",
					input.s.s_name);
				return( 0 );
			}
			break;		/* end of ARB5 case */

		case ARB4:
			/* arb4 vectors:  0 1 2 0 4 4 4 4 */
			j = svec[6];
			if( svec[0] == 2 )
				j = svec[4];
			move(uvec[0],uvec[1],svec[2],uvec[0],j,j,j,j);
			break;

		default:
			(void)printf("redoarb %s: unknown arb type (%d)\n",
				input.s.s_name,input.s.s_cgtype);
			return( 0 );
	}

	return( 1 );
}


static void
move( p0, p1, p2, p3, p4, p5, p6, p7 )
int p0, p1, p2, p3, p4, p5, p6, p7;
{
	register int i, j;
	static int pts[8];
	static float copy[24];

	pts[0] = p0 * 3;
	pts[1] = p1 * 3;
	pts[2] = p2 * 3;
	pts[3] = p3 * 3;
	pts[4] = p4 * 3;
	pts[5] = p5 * 3;
	pts[6] = p6 * 3;
	pts[7] = p7 * 3;

	/* copy of the record */
	for(i=0; i<=21; i+=3) {
		VMOVE(&copy[i], &input.s.s_values[i]);
	}

	for(i=0; i<8; i++) {
		j = pts[i];
		VMOVE(&input.s.s_values[i*3], &copy[j]);
	}
}



static int
compar( x, y )
register float *x,*y;
{
	register int i;

	for(i=0; i<3; i++) {
		if( fabs( *x++ - *y++ ) > 0.0001 )
			return(0);   /* different */
	}
	return(1);  /* same */
}


static void
vectors()
{
	register int i;

	for(i=3; i<=21; i+=3) {
		VSUB2(&input.s.s_values[i],&input.s.s_values[i],&input.s.s_values[0]);
	}
}



static void
points()
{
	register int i;

	for(i=3; i<=21; i+=3) {
		VADD2(&input.s.s_values[i],&input.s.s_values[i],&input.s.s_values[0]);
	}
}




/*
 *      		D W R E G
 *
 *	Draws an "edge approximation" of a region
 *
 *	All solids are converted to planar approximations.
 *	Logical operations are then applied to the solids to
 *	produce the edge representation of the region.
 *
 * 	Gary Kuehl 	2Feb83
 *
 * 	Routines used by dwreg():
 *	  1. gap()   	puts gaps in the edges
 *	  2. region()	finds intersection of ray with a region
 *	  3. neg()	vector A = - vector B
 *	  4. cpoint()	finds point of intersection of three planes
 *	  5. center()	finds center point of an arb
 *	  6. tplane()	tests if plane is inside enclosing rpp
 *	  7. arb()	finds intersection of ray with an arb
 *	  8. tgcin()	converts tgc to arbn
 *	  9. ellin()	converts ellg to arbn
 *	 10. regin()	process region to planes
 *	 11. solin()	finds enclosing rpp for solids
 *	 12. solpl()	process solids to arbn's
 *	 13. plane()	finds normalized plane from 3 points
 *
 *		R E V I S I O N   H I S T O R Y
 *
 *	11/02/83  CMK	Modified to use g3.c module (device independence).
 */


#define NPLANES	500
static float	peq[NPLANES*4];		/* plane equations for EACH region */
static float	solrpp[NMEMB*6];	/* enclosing RPPs for each member of the region */
static float	regi[NMEMB], rego[NMEMB];	/* distances along ray where ray enters and leaves the region */
static float	pc[3];			/* center (interior) point of a solid */
static float	rmn[3], rmx[3];		/* min,max's for the region */
static float	smn[3], smx[3];		/* min,max's for a solid */
static float	xb[3];			/* starting point of ray */
static float	wb[3];			/* direction cosines of ray */
static float	rin, rout;		/* location where ray enters and leaves a solid */
static float	ri, ro;			/* location where ray enters and leaves a region */
static float	tol;			/* tolerance */
static int	lmemb, umemb;		/* lower and upper limit of members of a region
					 * from one OR to the next OR
					 */
static float	*sp, *savesp;		/* pointers to the solid parameter array m_param[] */
static int	mlc[NMEMB];		/* location of last plane for each member */
static int	la, lb, lc, id, jd, nint, ngaps;
static float	pinf = 1000000.0;
static int	negpos;
static char	oper;

static void
dwreg()
{
	register int i,j;
	static int k,l;
	static int n,m;
	static int itemp;
	static float pi[3],po[3];
	static float lenwb;
	static unsigned	count;
	static unsigned	addr;
	static float xmin,ymin,zmin,xmax,ymax,zmax;
	static float c1[3*4]={1.,0.,0.,0.,0.,1.,0.,0.,0.,0.,1.,0.};

	xmin = ymin = zmin = 100000000.0;
	xmax = ymax = zmax = -100000000.0;

	/* set up for drawing with VECTOR ABSOLUTE Solid Lines */
	dl_preamble( 1 );

	/* calculate center and scale for COMPLETE REGION since may have ORs */
	lmemb = umemb = 0;
	savesp = &m_param[0];
	while( 1 ) {
		for(umemb = lmemb+1; (umemb < nmemb && m_op[umemb] != 'u'); umemb++)
			;
		lc = 0;
		regin(1);
		MIN(xmin, rmn[0]);
		MIN(ymin, rmn[1]);
		MIN(zmin, rmn[2]);
		MAX(xmax, rmx[0]);
		MAX(ymax, rmx[1]);
		MAX(zmax, rmx[2]);
		lmemb = umemb;
		if(umemb >= nmemb)
			break;
	}
	dl_scale = xmax - xmin;
	MAX(dl_scale, ymax - ymin);
	MAX(dl_scale, zmax - zmin);
	dl_xcent = (xmax + xmin) / 2.0;
	dl_ycent = (ymax + ymin) / 2.0;
	dl_zcent = (zmax + zmin) / 2.0;

	lmemb = 0;
	savesp = &m_param[0];

orregion:	/* sent here if region has or's */
	for(umemb = lmemb+1; (umemb < nmemb && m_op[umemb] != 'u'); umemb++)
		;

	lc = 0;
	regin(0);


	for(i=0; i<3; i++) c1[(i*4)+3]=rmn[i];
	l=0;

	/* id loop processes all member to the next OR */
	for(id=lmemb; id<umemb; id++) {
		if(mlc[id] < l)
			goto skipid;

		/* plane i is associated with member id */
		for(i=l; (i<(lc-1) && i<=mlc[id]); i++){
			m = i + 1;
			itemp = id;
			if(i == mlc[id])
				itemp = id + 1;
			for(jd=itemp; jd<umemb; jd++) {

				negpos = 0;
				if( (m_op[id] == '-' && m_op[jd] != '-') ||
				    (m_op[id] != '-' && m_op[jd] == '-') )
					negpos = 1;

				/* plane j is associated with member jd */
				for(j=m; j<=mlc[jd]; j++){
					if(id!=jd && m_op[id]=='-' && m_op[jd]=='-') { 
						for(k=0; k<3; k++) {
							if(solrpp[6*id+k] > solrpp[6*jd+k+3] || 
							   solrpp[6*id+k+3] < solrpp[6*jd+k] )
								goto sksolid;
						}

						for(k=mlc[jd-1]+1; k<=mlc[jd]; k++) {
							if(fabs(peq[i*4+3] - peq[k*4+3]) < tol && 
							    VDOT(&peq[i*4], &peq[k*4]) > .9999) {
								for(k=l; k<=mlc[id]; k++) {
									if(fabs(peq[j*4+3] - peq[k*4+3]) < tol && 
									    VDOT(&peq[j*4], &peq[k*4]) > .9999)
										goto noskip;
								}
								goto sksolid;
							}
						}
						for(k=l; k<= mlc[id]; k++) {
							if(fabs(peq[j*4+3] - peq[k*4+3]) < tol && 
							    VDOT(&peq[j*4], &peq[k*4]) > .9999)
								goto skplane;
						}
					}
noskip:
					if(fabs(VDOT(&peq[i*4],&peq[j*4]))<.9999){
						/* planes not parallel */

						/* compute vector for ray */
						VCROSS(wb,&peq[i*4],&peq[j*4]);
						lenwb = MAGNITUDE( wb );
						VSCALE(wb, wb, 1.0/lenwb);

						/* starting point for this ray */
						k=0;
						if(fabs(wb[1]) > fabs(wb[0])) k=1;
						if(fabs(wb[2]) > fabs(wb[k])) k=2;
						if(wb[k] < 0.0)
							neg( wb, wb, 3 );
						cpoint(&c1[k*4],&peq[i*4],&peq[j*4]);

						/* check if ray intersects region */
						if(region()>0){
							/* ray intersects region */
							/* plot this ray  including gaps */
							for(n=0; n<nint; n++){
								for(k=0; k<3; k++){
									 pi[k]=xb[k]+wb[k]*regi[n];
									 po[k]=xb[k]+wb[k]*rego[n];
								}
								dl_goto(&pi[0], UP);
								dl_goto(&po[0], DOWN);
							}
						}
					}
skplane:				 ;
				}
sksolid:
				m = mlc[jd] + 1;
			}
		}
skipid:
		l=mlc[id]+1;
	}


	lmemb = umemb;
	if(umemb < nmemb)
		goto orregion;

	nmemb = 0;

	/* Finish off the display subroutine */
	dl_epilogue();

	/* Build output record */
	commi.i_center[X] = dl_xcent;
	commi.i_center[Y] = dl_ycent;
	commi.i_center[Z] = dl_zcent;
	commi.i_center[H] = 1;
	commi.i_scale = dl_scale;

	/* Determine VG memory requirement */
	count = dl_size();

	/* Allocate VG storage for object */
	addr = memalloc( count );

	commi.i_addr = addr;
	commi.i_type = MS_DREG;

	if( addr == 0 )  {
		(void)printf("dwreg: no more Displaylist memory\n");
		commi.i_size = 0;	/* FLAG:  error */
	} else {
		commi.i_size = dl_load( addr, count );
	}
}


/* put gaps into region line */
static int
gap(si,so)
float si,so;
{
	register int igap,lgap,i;

	if(si<=ri+tol) goto front;
	if(so>=ro-tol) goto back;
	if(ngaps>0){
		for(igap=0;igap<ngaps;igap++) {
			/* locate position of gap along ray */
			if(si<=(regi[igap+1]+tol)) 
				goto insert;
		}
	}
	if((++ngaps)==25) return(-1);

	/* last gap along ray */
	rego[ngaps-1]=si;
	regi[ngaps]=so;
	return(1);

insert:		/* insert gap in ray between existing gaps */
	if(so < (rego[igap]-tol)) goto novlp;

	/* have overlapping gaps - sort out */
	MIN(rego[igap],si);
	if(regi[igap+1]>=so) return(1);
	regi[igap+1]=so;
	for(lgap=igap;(lgap<ngaps&&so<(rego[lgap]-tol));lgap++)
		;
	if(lgap==igap) return(1);
	if(so>regi[lgap+1]) {
		lgap++;
		igap++;
	}
	while(lgap<ngaps){
		rego[igap]=rego[lgap];
		regi[igap+1]=regi[lgap+1];
		lgap++;
		igap++;
	}
	ngaps=ngaps-lgap+igap;
	return(1);

novlp:		/* no overlapping gaps */
	if((++ngaps)>=25) return(-1);
	for(lgap=igap+1;lgap<ngaps;lgap++){
		rego[lgap]=rego[lgap-1];
		regi[lgap+1]=regi[lgap];
	}
	rego[igap]=si;
	regi[igap+1]=so;
	return(1);

front:		/* gap starts before beginning of ray */
	if((so+tol)>ro)
		return(0);
	MAX(ri,so);
	if(ngaps<1) return(1);
	for(igap=0; ((igap<ngaps) && ((ri+tol)>rego[igap])); igap++)
		;
	if(igap<1) return(1);
	MAX(ri, regi[igap]);
	lgap=ngaps;
	ngaps=0;
	if(igap>=lgap) return(1);
	for(i=igap;i<lgap;i++){
		rego[ngaps++]=rego[i];
		regi[ngaps]=regi[i+1];
	}
	return(1);

back:		/* gap ends after end of ray */
	MIN(ro,si);
	if(ngaps<1) return(1);
	for(igap=ngaps; (igap>0 && (ro<(regi[igap]+tol))); igap--)
		;
	if(igap < ngaps) {
		MIN(ro, rego[igap]);
	}
	ngaps=igap;
	return(1);
}


/* computes intersection of ray with region 
 *   	returns 1  if intersection
 *	        0  if no intersection
 */

static int
region()
{
	register int	i, kd;
	static float s1[3],s2[3];
	static float a1, a2;
	static float dum1, dum2;

	nint=0;
	ngaps=0;
	ri = -1.0 * pinf;
	ro=pinf;

	/* does ray intersect region rpp */
	for(i=0;i<3;i++){
		s1[i]=rmn[i]-xb[i];
		s2[i]=rmx[i]-xb[i];
	}

	/* find start & end point of ray with enclosing rpp */
	for(i=0;i<3;i++){
		if(fabs(wb[i])>.001){
			a1=s1[i]/wb[i];
			a2=s2[i]/wb[i];
			if(wb[i] <= 0.){
				if(a1<tol) return(0);
				MAX(ri,a2);
				MIN(ro,a1);
			}
			else {
				if(a2<tol) return(0);
				MAX(ri,a1);
				MIN(ro,a2);
			}
			if((ri+tol)>=ro) return(0);
		}
		else {
			if(s1[i]>tol || s2[i]<(-1.0*tol)) 
				return(0);
		}
	}

	/* ray intersects region - find intersection with each member */
	la=0;
	for(kd=lmemb;kd<umemb;kd++){
		oper=m_op[kd];
		lb=mlc[kd];

		/* if la > lb then no planes to check for this member */
		if(la > lb)
			goto nullsolid;

		if(kd==id || kd==jd) oper='+';
		if(oper!='-'){
			/* positive solid  recalculate end points of ray */
			if( arb() == 0 )
				return(0);
			if(ngaps==0){
				MAX(ri,rin);
				MIN(ro,rout);
			}
			else{
				dum1 = -1.0 * pinf;
				dum2 = rin;
				if(gap(dum1, dum2) <= 0)
					return(0);
				dum1 = rout;
				dum2 = pinf;
				if(gap(dum1, dum2) <= 0)
					return(0);
			}
		}
		if(oper == '-') {
			/* negative solid  look for gaps in ray */
			if(arb() != 0) {
				dum1 = rin;
				dum2 = rout;
				if(gap(dum1, dum2) <= 0)
					return(0);
			}
		}
		if(ri+tol>=ro) return(0);
		la=lb+1;
nullsolid:
		;
	}

	/*end of region - set number of intersections*/
	nint=ngaps+1;
	regi[0]=ri;
	rego[nint-1]=ro;
	return(1);
}


static void
neg(a,b,n)
register float *a, *b;
int n;
{
	register int i;

	for(i=0; i<n; i++)
		*(b+i) = -1.0 * (*(a+i));
}


/* computes point of intersection of three planes */
static void
cpoint(c1,c2,c3)
register float *c1, *c2, *c3;
{
	static float v1[4], v2[4], v3[4];
	register int i;
	static float d;

	VCROSS(v1,c2,c3);
	if((d=VDOT(c1,v1))==0) return;
	d = 1.0 / d;
	VCROSS(v2,c1,c3);
	VCROSS(v3,c1,c2);
	for(i=0; i<3; i++)
		xb[i]=d*(*(c1+3)*v1[i]-(*(c2+3))*v2[i]+(*(c3+3))*v3[i]);
}


/* find center point */
static void
center()
{
	register float ppc;
	register int i,j,k;

	for(i=0;i<3;i++){
		k=i;
		ppc=0.0;
		for(j=0; j<m_type[id]; j++) {
			ppc += *(sp+k);
			k+=3;
		}
		pc[i]=ppc/(float)m_type[id];
	}
}


/* test if plane inside enclosing rpp */
static void
tplane(p,q,r,s)
register float *p, *q, *r, *s;
{
	static float t;
	static float *pp,*pf;
	register int i;

	pp = &peq[lc*4];
	for(i=0;i<3;i++){
		t=rmn[i]-tol;
		if(*(p+i)<t && *(q+i)<t && *(r+i)<t && *(s+i)<t)
			return;
		t=rmx[i]+tol;
		if(*(p+i)>t && *(q+i)>t && *(r+i)>t  && *(s+i)>t)
			return;
	}

	/* Does plane already exist? */
	if(plane(p,q,r)==0) return;

	if((*(pp+3)-VDOT(pp,pc)) > 0.) neg(pp,pp,4);
	for(pf = &peq[0];pf<pp;pf+=4) {
		 if(VDOT(pp,pf)>0.9999 && fabs(*(pp+3)-*(pf+3))<tol) 
			return;
	}

	/* increment plane counter */
	if(++lc > NPLANES) {
		(void)printf("tplane: More than %d planes for a region - ABORTING\n", NPLANES);
		lc--;
		return;
	}
}

/* finds intersection of ray with arbitrary convex polyhedron */
static int
arb()
{
	static float s,*pp,dxbdn,wbdn,te;

	rin = ri;
	rout = ro;

	te = tol;
	if(oper == '-' && negpos)
		te = -1.0 * tol;

	for(pp = &peq[la*4];pp <= &peq[lb*4];pp+=4){
		dxbdn = *(pp+3)-VDOT(xb,pp);
		wbdn=VDOT(wb,pp);
		if(fabs(wbdn)>.001){
			s=dxbdn/wbdn;
			if(wbdn > 0.0) {
				MAX(rin, s);
			}
			else {
				MIN(rout,s);
			}
		}
		else{
			if(dxbdn>te) return(0);
		}
		if((rin+tol)>=rout || rout<=tol) return(0);
	}
	/* ray starts inside */
	MAX(rin,0);
	return(1);
}



/* convert tgc to an arbn */
static void
tgcin()
{
	static float vt[3], p[3], q[3], r[3], t[3];
	static float s,sa,c,ca,sd=.38268343,cd=.92387953;
	register int i,j,lk;

	lk = lc;

	for(i=0;i<3;i++){
		vt[i] = *(sp+i) + *(sp+i+3);
		pc[i]=( *(sp+i) + vt[i])*.5;
		p[i] = *(sp+i) + *(sp+i+6);
		t[i] = vt[i] + *(sp+i+12);
	}
	s=0.;
	c=1.;
	for(i=0;i<16;i++){
		sa=s*cd+c*sd;
		ca=c*cd-s*sd;
		for(j=0;j<3;j++){
			q[j] = *(sp+j) + ca * *(sp+j+6) + sa * *(sp+j+9);
			r[j]=vt[j]+ ca * *(sp+j+12) + sa* *(sp+j+15);
		}
		tplane( p, q, r, t );
		s=sa;
		c=ca;
		for(j=0; j<3; j++) {
			p[j] = q[j];
			t[j] = r[j];
		}
	}
	if(lc == lk)	return;

	/* top plane */
	for(i=0; i<3; i++) {
		p[i]=vt[i] + *(sp+i+12) + *(sp+i+15);
		q[i]=vt[i] + *(sp+i+15) - *(sp+i+12);
		r[i]=vt[i] - *(sp+i+12) - *(sp+i+15);
		t[i]=vt[i] + *(sp+i+12) - *(sp+i+15);
	}
	tplane( p, q, r, t );

	/* bottom plane */
	for(i=0;i<3;i++){
		p[i] = *(sp+i) + *(sp+i+6) + *(sp+i+9);
		q[i] = *(sp+i) + *(sp+i+9) - *(sp+i+6);
		r[i] = *(sp+i) - *(sp+i+6) - *(sp+i+9);
		t[i] = *(sp+i) + *(sp+i+6) - *(sp+i+9);
	}
	tplane( p, q, r, t );
}


/* convert ellg to an arbn */
static void
ellin()
{
	static float p[3], q[3], r[3], t[3];
	static float s1,s2,sa,c1,c2,ca,sd=.5,cd=.8660254,sgn;
	static float se, ce;
	register int i,j,ih,ie;

	sgn = -1.;
	for(i=0;i<3;i++) pc[i] = *(sp+i);
	for(ih=0;ih<2;ih++){
		c2=1.;
		s2=0.;

		/* find first point */
		for(ie=0;ie<3;ie++){
			s1=0.;
			c1=1.;
			se=s2*cd+c2*sd;
			ce=c2*cd-s2*sd;
			for(i=0;i<3;i++) {
				p[i] = *(sp+i) + (c2 * (*(sp+i+3)))
				       + (sgn * s2 * (*(sp+i+9)));
				t[i] = *(sp+i) + (ce * (*(sp+i+3)))
				       + (sgn * se * (*(sp+i+9)));
			}
			for(i=0;i<12;i++){
				sa=s1*cd+c1*sd;
				ca=c1*cd-s1*sd;
				for(j=0;j<3;j++){
					q[j] = *(sp+j) + (ca * c2 * (*(sp+j+3))) 
						+ (sa * c2 * (*(sp+j+6))) 
						+ (s2 * sgn * (*(sp+j+9)));
					r[j] = *(sp+j) + (c1 * ce* (*(sp+j+3))) 
						+ (s1 * ce * (*(sp+j+6))) 
						+ (se * sgn * (*(sp+j+9)));
				}
				tplane( p, q, r, t );
				s1=sa;
				c1=ca;
				for(j=0; j<3; j++) {
					p[j] = q[j];
					t[j] = r[j];
				}
			}
			c2=ce;
			s2=se;
		}
		sgn = -sgn;
	}
}


/* process region into planes */
static void
regin(flag)
int flag;	/* 1 if only calculating min,maxs   NO PLANE EQUATIONS */
{
	register int i,j;

	tol=rmn[0]=rmn[1]=rmn[2] = -pinf;
	rmx[0]=rmx[1]=rmx[2]=pinf;
	sp = savesp;

	/* find min max's */
	for(i=lmemb;i<umemb;i++){
		solin(i);
		if(m_op[i] != '-') {
			for(j=0;j<3;j++){
				MAX(rmn[j],smn[j]);
				MIN(rmx[j],smx[j]);
			}
		}
	}
	for(i=0;i<3;i++){
		MAX(tol,fabs(rmn[i]));
		MAX(tol,fabs(rmx[i]));
	}
	tol=tol*0.00001;

	if(flag == 0 ) {
		/* find planes for each solid */
		sp = savesp;
		solpl();
	}

	/* save the parameter pointer in case region has ORs */
	savesp = sp;
}


/* finds enclosing RPP for a solid using the solid's parameters */
static void
solin(num)
int num;
{
	static int amt[19]={0,0,0,12,15,18,21,24,0,0,0,0,0,0,0,0,0,18,12};
	register int *ity,i,j;
	static float a,b,c,d,v1,v2,vt,vb;

	ity = &m_type[num];
	if(*ity==20) *ity=8;
	if(*ity>19 || amt[*ity-1]==0){
		(void)printf("solin: Type %d Solid not known\n",*ity);
		return;
	}
	smn[0]=smn[1]=smn[2]=pinf;
	smx[0]=smx[1]=smx[2] = -pinf;

	/* ARB4,5,6,7,8 */
	if(*ity<18){
		for(i=0;i<*ity;i++){
			for(j=0;j<3;j++){
				MIN(smn[j],*sp);
				MAX(smx[j],*sp);
				sp++;
			}
		}
	}

	/* TGC */
	if(*ity==18){
		for(i=0;i<3;i++,sp++){
			vt = *sp + *(sp+3);
			a = *(sp+6);
			b = *(sp+9);
			c = *(sp+12);
			d = *(sp+15);
			v1=sqrt(a*a+b*b);
			v2=sqrt(c*c+d*d);
			MIN(smn[i],*(sp)-v1);
			MIN(smn[i],vt-v2);
			MAX(smx[i],*(sp)+v1);
			MAX(smx[i],vt+v2);			
		}
		sp+=15;
	}

	if(*ity > 18) {
		/* ELLG */
		for(i=0;i<3;i++,sp++){
			vb = *sp - *(sp+3);
			vt = *sp + *(sp+3);
			a = *(sp+6);
			b = *(sp+9);
			v1=sqrt(a*a+b*b);
			v2=sqrt(c*c+d*d);
			MIN(smn[i],vb-v1);
			MIN(smn[i],vt-v2);
			MAX(smx[i],vb+v1);
			MAX(smx[i],vt+v2);
		}
		sp+=9;
	}

	/* save min,maxs for this solid */
	for(i=0; i<3; i++) {
		solrpp[num*6+i] = smn[i];
		solrpp[num*6+i+3] = smx[i];
	}
}


/* converts all solids to ARBNs */
static void
solpl()
{
	static float tt;
	static int ls, n4;
	static float *pp,*p1,*p2,*p3,*p4;
	register int i,j;
	static int nfc[5]={4,5,5,6,6};
	static int iv[5*24]={
		 0,1,2,0, 3,0,1,0, 3,1,2,1, 3,2,0,0, 0,0,0,0, 0,0,0,0,
		 0,1,2,3, 4,0,1,0, 4,1,2,1, 4,2,3,2, 4,3,0,0, 0,0,0,0,
		 0,1,2,3, 1,2,5,4, 0,4,5,3, 4,0,1,0, 5,2,3,2, 0,0,0,0,
		 0,1,2,3, 4,5,6,4, 0,3,4,0, 1,2,6,5, 0,1,5,4, 3,2,6,4,
		 0,1,2,3, 4,5,6,7, 0,4,7,3, 1,2,6,5, 0,1,5,4, 3,2,6,7};

	lc=0;
	tt=tol*10.;
	for(id=lmemb;id<umemb;id++){
		ls=lc;
		if(m_type[id]<18){

			/* ARB 4,5,6,7,8 */
			n4=m_type[id]-4;
			center();
			j=n4*24;
			for(i=0;i<nfc[n4];i++){
				p1=sp+iv[j++]*3;
				p2=sp+iv[j++]*3;
				p3=sp+iv[j++]*3;
				p4=sp+iv[j++]*3;
				tplane(p1,p2,p3,p4);
			}
			sp+=m_type[id]*3;
		}
		if(m_type[id]==18){
			tgcin();
			sp+=18;
		}
		if(m_type[id]==19){
			ellin();
			sp+=12;
		}

		if(m_op[id]=='-'){
			pp = &peq[4*ls]+3;
			while(ls++ < lc) {
				*pp-=tt;
				pp+=4;
			}
		}

		mlc[id]=lc-1;
	}
}


/* computes normalized plane eq from three points */
static int
plane(p1,p2,p3)
float *p1, *p2, *p3;
{
	static float vecl;
	static float va[3],vb[3],vc[3];
	register int i;

	for(i=0;i<3;i++){
		va[i] = *(p2+i) - *(p1+i);
		vb[i] = *(p3+i) - *(p1+i);
	}
	VCROSS(vc,va,vb);
	vecl = MAGNITUDE( vc );
	if(vecl<.0001) return(0);
	VSCALE(&peq[lc*4], vc, 1.0/vecl);
	peq[(lc*4)+3] = VDOT(&peq[lc*4], p1);
	return(1);
}
