/*
 *			N M G _ M O D . C
 *
 *  Routines for modifying n-Manifold Geometry data structures.
 *
 *  Authors -
 *	Lee A. Butler
 *	Michael John Muuss
 *  
 *  Source -
 *	SECAD/VLD Computing Consortium, Bldg 394
 *	The U. S. Army Ballistic Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5066
 *  
 *  Copyright Notice -
 *	This software is Copyright (C) 1991 by the United States Army.
 *	All rights reserved.
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (BRL)";
#endif

#include <stdio.h>
#include "machine.h"
#include "vmath.h"
#include "nmg.h"
#include "raytrace.h"

/************************************************************************
 *									*
 *				SHELL Routines				*
 *									*
 ************************************************************************/

/*
 *			N M G _ S H E L L _ C O P L A N A R _ F A C E _ M E R G E
 *
 *  A geometric routine to
 *  find all pairs of faces in a shell that have the same plane equation
 *  (to within the given tolerance), and combine them into a single face.
 *
 *  Note that this may result in some of the verticies being very slightly
 *  off the plane equation, but the geometry routines need to be prepared
 *  for this in any case.
 *  If the "simplify" flag is set, pairs of loops in the face that touch
 *  will be combined into a single loop where possible.
 *
 *  XXX Perhaps should be recast as "nmg_shell_shared_face_merge()", leaving
 *  XXX all the geometric calculations to the code in nmg_fuse.c ?
 */
void
nmg_shell_coplanar_face_merge( s, tol, simplify )
struct shell		*s;
CONST struct rt_tol	*tol;
CONST int		simplify;
{
	struct model	*m;
	int		len;
	int		*flags1;
	int		*flags2;
	struct faceuse	*fu1;
	struct faceuse	*fu2;
	struct face	*f1;
	struct face	*f2;
	struct face_g	*fg1;
	struct face_g	*fg2;

	NMG_CK_SHELL(s);
	m = nmg_find_model( &s->l.magic );
	len = sizeof(int) * m->maxindex;
	flags1 = (int *)rt_calloc( sizeof(int), m->maxindex,
		"nmg_shell_coplanar_face_merge flags1[]" );
	flags2 = (int *)rt_calloc( sizeof(int), m->maxindex,
		"nmg_shell_coplanar_face_merge flags2[]" );

	/* Visit each face in the shell */
	for( RT_LIST_FOR( fu1, faceuse, &s->fu_hd ) )  {
		plane_t		n1;

		if( RT_LIST_NEXT_IS_HEAD(fu1, &s->fu_hd) )  break;
		f1 = fu1->f_p;
		NMG_CK_FACE(f1);
		if( NMG_INDEX_TEST(flags1, f1) )  continue;
		NMG_INDEX_SET(flags1, f1);

		fg1 = f1->fg_p;
		NMG_CK_FACE_G(fg1);
		NMG_GET_FU_PLANE( n1, fu1 );

		/* For this face, visit all remaining faces in the shell. */
		/* Don't revisit any faces already considered. */
		bcopy( flags1, flags2, len );
		for( fu2 = RT_LIST_NEXT(faceuse, &fu1->l);
		     RT_LIST_NOT_HEAD(fu2, &s->fu_hd);
		     fu2 = RT_LIST_NEXT(faceuse,&fu2->l)
		)  {
			register fastf_t	dist;
			plane_t			n2;

			f2 = fu2->f_p;
			NMG_CK_FACE(f2);
			if( NMG_INDEX_TEST(flags2, f2) )  continue;
			NMG_INDEX_SET(flags2, f2);

			fg2 = f2->fg_p;
			NMG_CK_FACE_G(fg2);

			/* See if face geometry is shared & same direction */
			if( fg1 != fg2 || f1->flip != f2->flip )  {
				/* If plane equations are different, done */
				NMG_GET_FU_PLANE( n2, fu2 );

				/* Compare distances from origin */
				dist = n1[3] - n2[3];
				if( !NEAR_ZERO(dist, tol->dist) )  continue;

				/*
				 *  Compare angle between normals.
				 *  Can't just use RT_VECT_ARE_PARALLEL here,
				 *  because they must point in the same direction.
				 */
				dist = VDOT( n1, n2 );
				if( !(dist >= tol->para) )  continue;
			}

			/*
			 * Plane equations are the same, within tolerance,
			 * or by shared fg topology.
			 * Move everything into fu1, and
			 * kill now empty faceuse, fumate, and face
			 */
			{
				struct faceuse	*prev_fu;
				prev_fu = RT_LIST_PREV(faceuse, &fu2->l);
				/* The prev_fu can never be the head */
				if( RT_LIST_IS_HEAD(prev_fu, &s->fu_hd) )
					rt_bomb("prev is head?\n");

				nmg_jf( fu1, fu2 );

				fu2 = prev_fu;
			}

			/* There is now the option of simplifying the face,
			 * by removing unnecessary edges.
			 */
			if( simplify )  {
				struct loopuse *lu;

				for (RT_LIST_FOR(lu, loopuse, &fu1->lu_hd))
					nmg_simplify_loop(lu);
			}
		}
	}
	rt_free( (char *)flags1, "nmg_shell_coplanar_face_merge flags1[]" );
	rt_free( (char *)flags2, "nmg_shell_coplanar_face_merge flags2[]" );

	if (rt_g.NMG_debug & DEBUG_BASIC)  {
		rt_log("nmg_shell_coplanar_face_merge(s=x%x, tol=x%x, simplify=%d)\n",
			s, tol, simplify);
	}
}

/*
 *			N M G _ S I M P L I F Y _ S H E L L
 *
 *  Simplify all the faces in this shell, where possible.
 *  Under some circumstances this may result in an empty shell as a result.
 *
 * Returns -
 *	0	If all was OK
 *	1	If shell is now empty
 */
int
nmg_simplify_shell(s)
struct shell *s;
{
	struct faceuse *fu;
	int ret_val;

	NMG_CK_SHELL(s);

	for (RT_LIST_FOR(fu, faceuse, &s->fu_hd)) {
		if( nmg_simplify_face(fu) )  {
			struct faceuse	*kfu = fu;
			fu = RT_LIST_PREV( faceuse, &fu->l );
			nmg_kfu( kfu );
		}
	}

	ret_val = nmg_shell_is_empty(s);

	if (rt_g.NMG_debug & DEBUG_BASIC)  {
		rt_log("nmg_simplify_shell(s=x%x) returns %d\n", s , ret_val);
	}

	return( ret_val );
}

/*
 *			N M G _ R M _ R E D U N D A N C I E S
 *
 *  Remove all redundant parts between the different "levels" of a shell.
 *  Remove wire loops that match face loops.
 *  Remove wire edges that match edges in wire loops or face loops.
 *  Remove lone vertices (stored as wire loops on a single vertex) that
 *  match vertices in a face loop, wire loop, or wire edge.
 */
void
nmg_rm_redundancies(s)
struct shell	*s;
{
	struct faceuse	*fu;
	struct loopuse	*lu;
	struct edgeuse	*eu;
	struct vertexuse	*vu;
	long		magic1;

	NMG_CK_SHELL(s);

	if( RT_LIST_NON_EMPTY( &s->fu_hd ) )  {
		/* Compare wire loops -vs- loops in faces */
		lu = RT_LIST_FIRST( loopuse, &s->lu_hd );
		while( RT_LIST_NOT_HEAD( lu, &s->lu_hd ) )  {
			register struct loopuse	*nextlu;
			NMG_CK_LOOPUSE(lu);
			NMG_CK_LOOP(lu->l_p);
			nextlu = RT_LIST_PNEXT( loopuse, lu );
			if( nmg_is_loop_in_facelist( lu->l_p, &s->fu_hd ) )  {
				/* Dispose of wire loop (and mate)
				 * which match face loop */
				if( nextlu == lu->lumate_p )
					nextlu = RT_LIST_PNEXT(loopuse, nextlu);
				nmg_klu( lu );
			}
			lu = nextlu;
		}

		/* Compare wire edges -vs- edges in loops in faces */
		eu = RT_LIST_FIRST( edgeuse, &s->eu_hd );
		while( RT_LIST_NOT_HEAD( eu, &s->eu_hd ) )  {
			register struct edgeuse *nexteu;
			NMG_CK_EDGEUSE(eu);
			NMG_CK_EDGE(eu->e_p);
			nexteu = RT_LIST_PNEXT( edgeuse, eu );
			if( nmg_is_edge_in_facelist( eu->e_p, &s->fu_hd ) )  {
				/* Dispose of wire edge (and mate) */
				if( nexteu == eu->eumate_p )
					nexteu = RT_LIST_PNEXT(edgeuse, nexteu);
				(void)nmg_keu(eu);
			}
			eu = nexteu;
		}
	}

	/* Compare wire edges -vs- edges in wire loops */
	eu = RT_LIST_FIRST( edgeuse, &s->eu_hd );
	while( RT_LIST_NOT_HEAD( eu, &s->eu_hd ) )  {
		register struct edgeuse *nexteu;
		NMG_CK_EDGEUSE(eu);
		NMG_CK_EDGE(eu->e_p);
		nexteu = RT_LIST_PNEXT( edgeuse, eu );
		if( nmg_is_edge_in_looplist( eu->e_p, &s->lu_hd ) )  {
			/* Kill edge use and mate */
			if( nexteu == eu->eumate_p )
				nexteu = RT_LIST_PNEXT(edgeuse, nexteu);
			(void)nmg_keu(eu);
		}
		eu = nexteu;
	}

	/* Compare lone vertices against everything else */
	/* Individual vertices are stored as wire loops on a single vertex */
	lu = RT_LIST_FIRST( loopuse, &s->lu_hd );
	while( RT_LIST_NOT_HEAD( lu, &s->lu_hd ) )  {
		register struct loopuse	*nextlu;
		NMG_CK_LOOPUSE(lu);
		nextlu = RT_LIST_PNEXT( loopuse, lu );
		magic1 = RT_LIST_FIRST_MAGIC( &lu->down_hd );
		if( magic1 != NMG_VERTEXUSE_MAGIC )  {
			lu = nextlu;
			continue;
		}
		vu = RT_LIST_PNEXT( vertexuse, &lu->down_hd );
		NMG_CK_VERTEXUSE(vu);
		NMG_CK_VERTEX(vu->v_p);
		if( nmg_is_vertex_in_facelist( vu->v_p, &s->fu_hd ) ||
		    nmg_is_vertex_in_looplist( vu->v_p, &s->lu_hd,0 ) ||
		    nmg_is_vertex_in_edgelist( vu->v_p, &s->eu_hd ) )  {
		    	/* Kill lu and mate */
			if( nextlu == lu->lumate_p )
				nextlu = RT_LIST_PNEXT(loopuse, nextlu);
			nmg_klu( lu );
			lu = nextlu;
			continue;
		}
		lu = nextlu;
	}

	/* There really shouldn't be a lone vertex by now */
	if( s->vu_p )  rt_log("nmg_rm_redundancies() lone vertex?\n");

	if (rt_g.NMG_debug & DEBUG_BASIC)  {
		rt_log("nmg_rm_redundancies(s=x%x)\n", s);
	}
}

/*
 *			N M G _ S A N I T I Z E _ S _ L V
 *
 *	Remove those pesky little vertex-only loops of orientation "orient".
 *	Typically these will be OT_BOOLPLACE markers created in the
 *	process of doing intersections for the boolean operations.
 */
void
nmg_sanitize_s_lv(s, orient)
struct shell	*s;
int		orient;
{
	struct faceuse *fu;
	struct loopuse *lu, *lustart;
	pointp_t pt;

	NMG_CK_SHELL(s);

	/* sanitize the loop lists in the faces of the shell */
	fu = RT_LIST_FIRST(faceuse, &s->fu_hd);
	while (RT_LIST_NOT_HEAD(fu, &s->fu_hd) ) {

		/* all of those vertex-only/orient loops get deleted
		 */
		lu = RT_LIST_FIRST(loopuse, &fu->lu_hd);
		while (RT_LIST_NOT_HEAD(lu, &fu->lu_hd)) {
			if (lu->orientation == orient) {
				lu = RT_LIST_PNEXT(loopuse,lu);
				nmg_klu(RT_LIST_PLAST(loopuse, lu));
			} else if (lu->orientation == OT_UNSPEC &&
			    RT_LIST_FIRST_MAGIC(&lu->down_hd) ==
			    NMG_VERTEXUSE_MAGIC) {
				register struct vertexuse *vu;
				vu = RT_LIST_FIRST(vertexuse, &lu->down_hd);
				pt = vu->v_p->vg_p->coord;
				VPRINT("nmg_sanitize_s_lv() OT_UNSPEC at", pt);
				lu = RT_LIST_PNEXT(loopuse,lu);
			} else {
				lu = RT_LIST_PNEXT(loopuse,lu);
			}
		}

		/* step forward, avoiding re-processing our mate (which would
		 * have had it's loops processed with ours)
		 */
		if (RT_LIST_PNEXT(faceuse, fu) == fu->fumate_p)
			fu = RT_LIST_PNEXT_PNEXT(faceuse, fu);
		else
			fu = RT_LIST_PNEXT(faceuse, fu);

		/* If previous faceuse has no more loops, kill it */
		if (RT_LIST_IS_EMPTY( &(RT_LIST_PLAST(faceuse, fu))->lu_hd )) {
			/* All of the loopuses of the previous face were
			 * BOOLPLACE's so the face will now go away
			 */
			nmg_kfu(RT_LIST_PLAST(faceuse, fu));
		}
	}

	/* Sanitize any wire/vertex loops in the shell */
	lu = RT_LIST_FIRST(loopuse, &s->lu_hd);
	while (RT_LIST_NOT_HEAD(lu, &s->lu_hd) ) {
		if (lu->orientation == orient) {
			lu = RT_LIST_PNEXT(loopuse,lu);
			nmg_klu(RT_LIST_PLAST(loopuse, lu));
		} else if (lu->orientation == OT_UNSPEC &&
		    RT_LIST_FIRST_MAGIC(&lu->down_hd) ==
		    NMG_VERTEXUSE_MAGIC) {
			register struct vertexuse *vu;
			vu = RT_LIST_FIRST(vertexuse, &lu->down_hd);
			pt = vu->v_p->vg_p->coord;
			VPRINT("nmg_sanitize_s_lv() OT_UNSPEC at", pt);
			lu = RT_LIST_PNEXT(loopuse,lu);
		} else {
			lu = RT_LIST_PNEXT(loopuse,lu);
		}
	}

	if (rt_g.NMG_debug & DEBUG_BASIC)  {
		rt_log("nmg_sanitize_s_lv(s=x%x, orient=%d)\n",	s, orient);
	}
}

/*
 *			N M G _ S _ S P L I T _ T O U C H I N G L O O P S
 *
 *  For every loop in a shell, invoke nmg_split_touchingloops() on it.
 *
 *  Needed before starting classification, to separate interior (touching)
 *  loop segments into true interior loops, so each can be processed
 *  separately.
 */
void
nmg_s_split_touchingloops(s, tol)
struct shell		*s;
CONST struct rt_tol	*tol;
{
	struct faceuse	*fu;
	struct loopuse	*lu;

	NMG_CK_SHELL(s);
	RT_CK_TOL(tol);

	/* First, handle any splitting */
	for( RT_LIST_FOR( fu, faceuse, &s->fu_hd ) )  {
		for( RT_LIST_FOR( lu, loopuse, &fu->lu_hd ) )  {
			nmg_split_touchingloops( lu, tol );
		}
	}
	for( RT_LIST_FOR( lu, loopuse, &s->lu_hd ) )  {
		nmg_split_touchingloops( lu, tol );
	}

	/* Second, reorient any split loop fragments */
	for( RT_LIST_FOR( fu, faceuse, &s->fu_hd ) )  {
		for( RT_LIST_FOR( lu, loopuse, &fu->lu_hd ) )  {
			if( lu->orientation != OT_UNSPEC )  continue;
			nmg_lu_reorient( lu, tol );
		}
	}

	if (rt_g.NMG_debug & DEBUG_BASIC)  {
		rt_log("nmg_s_split_touching_loops(s=x%x, tol=x%x)\n", s, tol);
	}
}

/*
 *			N M G _ J S
 *
 *  Join two shells into one.
 *  This is mostly an up-pointer re-labeling activity, as it is left up to
 *  the caller to ensure that there are no non-explicit intersections.
 *
 *  Upon return, s2 will no longer exist.
 *
 *  The 'tol' arg is used strictly for printing purposes.
 */
void
nmg_js( s1, s2, tol )
register struct shell	*s1;		/* destination */
register struct shell	*s2;		/* source */
CONST struct rt_tol	*tol;
{
	struct faceuse	*fu2;
	struct faceuse	*nextfu;
	struct loopuse	*lu;
	struct loopuse	*nextlu;
	struct edgeuse	*eu;
	struct edgeuse	*nexteu;
	struct vertexuse *vu;
	struct vertex	*v;

	NMG_CK_SHELL(s1);
	NMG_CK_SHELL(s2);
	RT_CK_TOL(tol);

	if (rt_g.NMG_debug & DEBUG_BASIC)  {
		rt_log("nmg_js(s1=x%x, s2=x%x) START\n", s1, s2);
	}

	if( rt_g.NMG_debug & DEBUG_VERIFY )
		nmg_vshell( &s1->r_p->s_hd, s1->r_p );

	/*
	 *  For each face in the shell, process all the loops in the face,
	 *  and then handle the face and all loops as a unit.
	 */
	fu2 = RT_LIST_FIRST( faceuse, &s2->fu_hd );
	while( RT_LIST_NOT_HEAD( fu2, &s2->fu_hd ) )  {
		struct faceuse	*fu1;

		nextfu = RT_LIST_PNEXT( faceuse, fu2 );

		/* Faceuse mates will be handled at same time as OT_SAME fu */
		if( fu2->orientation != OT_SAME )  {
			fu2 = nextfu;
			continue;
		}

		/* Consider this face */
		NMG_CK_FACEUSE(fu2);
		NMG_CK_FACE(fu2->f_p);

		if( nextfu == fu2->fumate_p )
			nextfu = RT_LIST_PNEXT(faceuse, nextfu);

		/* If there is a face in the destination shell that
		 * shares face geometry with this face, then
		 * move all the loops into the other face,
		 * and eliminate this redundant face.
		 */
		fu1 = nmg_find_fu_with_fg_in_s( s1, fu2 );
		if( fu1 )  {
			if (rt_g.NMG_debug & DEBUG_BASIC)
				rt_log("nmg_js(): shared face_g, doing nmg_jf()\n");
			nmg_jf( fu1, fu2 );
			/* fu2 pointer is invalid here */
			fu2 = fu1;
		} else {
			nmg_mv_fu_between_shells( s1, s2, fu2 );
		}

		fu2 = nextfu;
	}
#if 0
	if( rt_g.NMG_debug & DEBUG_VERIFY )
		nmg_vshell( &s1->r_p->s_hd, s1->r_p );
#endif

	/*
	 *  For each loop in the shell, process.
	 *  Each loop is either a wire-loop, or a vertex-with-self-loop.
	 *  Both get the same treatment.
	 */
	lu = RT_LIST_FIRST( loopuse, &s2->lu_hd );
	while( RT_LIST_NOT_HEAD( lu, &s2->lu_hd ) )  {
		nextlu = RT_LIST_PNEXT( loopuse, lu );

		NMG_CK_LOOPUSE(lu);
		NMG_CK_LOOP( lu->l_p );
		if( nextlu == lu->lumate_p )
			nextlu = RT_LIST_PNEXT(loopuse, nextlu);

		nmg_mv_lu_between_shells( s1, s2, lu );
		lu = nextlu;
	}
#if 0
	if( rt_g.NMG_debug & DEBUG_VERIFY )
		nmg_vshell( &s1->r_p->s_hd, s1->r_p );
#endif

	/*
	 *  For each wire-edge in the shell, ...
	 */
	eu = RT_LIST_FIRST( edgeuse, &s2->eu_hd );
	while( RT_LIST_NOT_HEAD( eu, &s2->eu_hd ) )  {
		nexteu = RT_LIST_PNEXT( edgeuse, eu );

		/* Consider this edge */
		NMG_CK_EDGEUSE(eu);
		NMG_CK_EDGE( eu->e_p );
		if( nexteu == eu->eumate_p )
			nexteu = RT_LIST_PNEXT(edgeuse, nexteu);
		nmg_mv_eu_between_shells( s1, s2, eu );
		eu = nexteu;
	}
#if 0
	if( rt_g.NMG_debug & DEBUG_VERIFY )
		nmg_vshell( &s1->r_p->s_hd, s1->r_p );
#endif

	/*
	 * Final case:  shell of a single vertexuse
	 */
	if( vu = s2->vu_p )  {
		NMG_CK_VERTEXUSE( vu );
		NMG_CK_VERTEX( vu->v_p );
		nmg_mv_vu_between_shells( s1, s2, vu );
		s2->vu_p = (struct vertexuse *)0;	/* sanity */
	}
	if( rt_g.NMG_debug & DEBUG_VERIFY )
		nmg_vshell( &s1->r_p->s_hd, s1->r_p );

	if( RT_LIST_NON_EMPTY( &s2->fu_hd ) )  {
		rt_bomb("nmg_js():  s2 still has faces!\n");
	}
	if( RT_LIST_NON_EMPTY( &s2->lu_hd ) )  {
		rt_bomb("nmg_js():  s2 still has wire loops!\n");
	}
	if( RT_LIST_NON_EMPTY( &s2->eu_hd ) )  {
		rt_bomb("nmg_js():  s2 still has wire edges!\n");
	}
	if(s2->vu_p) {
		rt_bomb("nmg_js():  s2 still has verts!\n");
	}

	/* s2 is completely empty now, which is an invalid condition */
	nmg_ks( s2 );

	if (rt_g.NMG_debug & DEBUG_BASIC)  {
		rt_log("nmg_js(s1=x%x, s2=x%x) END\n", s1, s2);
	}
}

/*
 *			N M G _ I N V E R T _ S H E L L
 *
 *  Reverse the surface normals, and invert the orientation state of
 *  all faceuses in a shell.
 * 
 *  This turns the shell "inside out", such as might be needed for the
 *  right hand side term of a subtraction operation.
 *
 *  While this function is operating, the parity of faceuses radially
 *  around edgeuses is disrupted, hence this atomic interface to
 *  invert the shell.
 *
 *  The 'tol' argument is used strictly for printing.
 */
void
nmg_invert_shell( s, tol )
struct shell		*s;
CONST struct rt_tol	*tol;
{
	struct model	*m;
	struct faceuse	*fu;
	char		*tags;

	NMG_CK_SHELL(s);
	m = s->r_p->m_p;
	NMG_CK_MODEL(m);
	RT_CK_TOL(tol);

	if (rt_g.NMG_debug & DEBUG_BASIC)  {
		rt_log("nmg_invert_shell(x%x)\n", s);
	}

	/* Allocate map of faces visited */
	tags = rt_calloc( m->maxindex+1, 1, "nmg_invert_shell() tags[]" );

	for( RT_LIST_FOR( fu, faceuse, &s->fu_hd ) )  {
		NMG_CK_FACEUSE(fu);
		/* By tagging on faces, marks fu and fumate together */
		if( NMG_INDEX_TEST( tags, fu->f_p ) )  continue;
		NMG_INDEX_SET( tags, fu->f_p );
		/* Process fu and fumate together */
		nmg_reverse_face(fu);
	}
	rt_free( tags, "nmg_invert_shell() tags[]" );
}

/************************************************************************
 *									*
 *				FACE Routines				*
 *									*
 ************************************************************************/

/*
 *			N M G _ C M F A C E
 *
 *	Create a face with exactly one exterior loop (and no holes),
 *	given an array of pointers to struct vertex pointers.
 *	Intended to help create a "3-manifold" shell, where
 *	each edge has only two faces alongside of it.
 *	(Shades of winged edges!)
 *
 *	"verts" is an array of "n" pointers to pointers to (struct vertex).
 *	"s" is the parent shell for the new face.
 *
 *	The new face will consist of a single loop
 *	made from n edges between the n vertices.  Before an edge is created
 *	between a pair of verticies, we check to see if there is already an
 *	edge with exactly one edgeuse+mate (in this shell)
 *	that runs between the two verticies.
 *	If such an edge can be found, the newly created edgeuses will just
 *	use the existing edge.
 *	This means that no special call to nmg_gluefaces() is needed later.
 *
 *	If a pointer in verts is a pointer to a null vertex pointer, a new
 *	vertex is created.  In this way, new verticies can be created
 *	conveniently within a user's list of known verticies
 *
 *	verts		pointers to struct vertex	    vertex structs
 *
 *	-------		--------
 *   0	|  +--|-------->|   +--|--------------------------> (struct vertex)
 *	-------		--------	---------
 *   1	|  +--|------------------------>|   +---|---------> (struct vertex)
 *	-------		--------	---------
 *   2	|  +--|-------->|   +--|--------------------------> (struct vertex)
 *	-------		--------
 *  ...
 *	-------				---------
 *   n	|  +--|------------------------>|   +---|---------> (struct vertex)
 *	-------				---------
 *
 *
 *	The vertices *must* be listed in "counter-clockwise" (CCW) order.
 *	This routine makes only topology, without reference to any geometry.
 *
 *	Note that this routine inserts new vertices (by edge use splitting)
 *	at the head of the loop, which reverses the order.
 *	Therefore, the caller's vertices are traversed in reverse order
 *	to counter this behavior, and
 *	to effect the proper vertex order in the final face loop.
 *
 *	Also note that this routine uses one level more of indirection
 *	in the verts[] array than nmg_cface().
 */
struct faceuse *
nmg_cmface(s, verts, n)
struct shell	*s;
struct vertex	**verts[];
int		n;
{
	struct faceuse *fu;
	struct edgeuse *eu, *eur, *euold;
	struct loopuse	*lu;
	struct vertexuse	*vu;
	int i;

	NMG_CK_SHELL(s);

	if (n < 1) {
		rt_log("nmg_cmface(s=x%x, verts=x%x, n=%d.)\n",
			s, verts, n );
		rt_bomb("nmg_cmface() trying to make bogus face\n");
	}

	/* make sure verts points to some real storage */
	if (!verts) {
		rt_log("nmg_cmface(s=x%x, verts=x%x, n=%d.) null pointer to array start\n",
			s, verts, n );
		rt_bomb("nmg_cmface\n");
	}

	/* validate each of the pointers in verts */
	for (i=0 ; i < n ; ++i) {
		if (verts[i]) {
			if (*verts[i]) {
				/* validate the vertex pointer */
				NMG_CK_VERTEX(*verts[i]);
			}
		} else {
			rt_log("nmg_cmface(s=x%x, verts=x%x, n=%d.) verts[%d]=NULL\n",
				s, verts, n, i );
			rt_bomb("nmg_cmface\n");
		}
	}

	lu = nmg_mlv(&s->l.magic, *verts[n-1], OT_SAME);
	fu = nmg_mf(lu);
	fu->orientation = OT_SAME;
	fu->fumate_p->orientation = OT_OPPOSITE;
	vu = RT_LIST_FIRST( vertexuse, &lu->down_hd);
	NMG_CK_VERTEXUSE(vu);
	eu = nmg_meonvu(vu);
	NMG_CK_EDGEUSE(eu);

	if (!(*verts[n-1]))  {
		NMG_CK_VERTEXUSE(eu->vu_p);
		NMG_CK_VERTEX(eu->vu_p->v_p);
		*verts[n-1] = eu->vu_p->v_p;
	}

	for (i = n-2 ; i >= 0 ; i--) {
		/* Get the edgeuse most recently created */
		euold = RT_LIST_FIRST( edgeuse, &lu->down_hd );
		NMG_CK_EDGEUSE(euold);

		if (rt_g.NMG_debug & DEBUG_CMFACE)
			rt_log("nmg_cmface() euold: %8x\n", euold);

		/* look for pre-existing edge between these
		 * verticies
		 */
		if (*verts[i]) {
			/* look for an existing edge to share */
			eur = nmg_findeu(*verts[i+1], *verts[i], s, euold, 1);
			eu = nmg_eusplit(*verts[i], euold);
			if (eur) {
				nmg_moveeu(eur, eu);

				if (rt_g.NMG_debug & DEBUG_CMFACE)
					rt_log("nmg_cmface() found another edgeuse (%8x) between %8x and %8x\n",
						eur, *verts[i+1], *verts[i]);
			} else {
				if (rt_g.NMG_debug & DEBUG_CMFACE)
				    rt_log("nmg_cmface() didn't find edge from verts[%d]%8x to verts[%d]%8x\n",
					i+1, *verts[i+1], i, *verts[i]);
			}
		} else {
			eu = nmg_eusplit(*verts[i], euold);
			*verts[i] = eu->vu_p->v_p;

			if (rt_g.NMG_debug & DEBUG_CMFACE)  {
				rt_log("nmg_cmface() *verts[%d] was null, is now %8x\n",
					i, *verts[i]);
			}
		}
	}

	if (eur = nmg_findeu(*verts[n-1], *verts[0], s, euold, 1))  {
		nmg_moveeu(eur, euold);
	} else  {
	    if (rt_g.NMG_debug & DEBUG_CMFACE)
		rt_log("nmg_cmface() didn't find edge from verts[%d]%8x to verts[%d]%8x\n",
			n-1, *verts[n-1], 0, *verts[0]);
	}
	if (rt_g.NMG_debug & DEBUG_BASIC)  {
		rt_log("nmg_cmface(s=x%x, verts[]=x%x, n=%d) fu=x%x\n",
			s, verts, n, fu);
	}
	return (fu);
}

/*
 *			N M G _ C F A C E
 *
 *	Create a loop within a face, given a list of vertices.
 *
 *	"verts" is an array of "n" pointers to (struct vertex).  "s" is the
 *	parent shell for the new face.  The face will consist of a single loop
 *	made from edges between the n vertices.
 *
 *	If verts is a null pointer (no vertex list), all vertices of the face
 *	will be new points.  Otherwise, verts is a pointer to a list of
 *	vertices to use in creating the face/loop.  Null entries within the
 *	list will cause a new vertex to be created for that point.  Such new
 *	vertices will be inserted into the list for return to the caller.
 *
 *	The vertices should be listed in
 *	"counter-clockwise" (CCW) order if this is an ordinary face (loop),
 *	and in "clockwise" (CW) order if this is an interior
 * 	("hole" or "subtracted") face (loop).
 *	This routine makes only topology, without reference to any geometry.
 *
 *	Note that this routine inserts new vertices (by edge use splitting)
 *	at the head of the loop, which reverses the order.
 *	Therefore, the caller's vertices are traversed in reverse order
 *	to counter this behavior, and
 *	to effect the proper vertex order in the final face loop.
 */
struct faceuse *
nmg_cface(s, verts, n)
struct shell *s;
struct vertex *verts[];
int n;
{
	struct faceuse *fu;
	struct edgeuse *eu;
	struct loopuse	*lu;
	struct vertexuse *vu;
	int i;

	NMG_CK_SHELL(s);
	if (n < 1) {
		rt_log("nmg_cface(s=x%x, verts=x%x, n=%d.)\n",
			s, verts, n );
		rt_bomb("nmg_cface() trying to make bogus face\n");
	}

	if (verts) {
		for (i=0 ; i < n ; ++i) {
			if (verts[i]) {
				NMG_CK_VERTEX(verts[i]);
			}
		}
		lu = nmg_mlv(&s->l.magic, verts[n-1], OT_SAME);
		fu = nmg_mf(lu);
		vu = RT_LIST_FIRST(vertexuse, &lu->down_hd);
		eu = nmg_meonvu(vu);

		if (!verts[n-1])
			verts[n-1] = eu->vu_p->v_p;

		for (i = n-2 ; i >= 0 ; i--) {
			eu = RT_LIST_FIRST(edgeuse, &lu->down_hd);
			eu = nmg_eusplit(verts[i], eu);
			if (!verts[i])
				verts[i] = eu->vu_p->v_p;
		}

	} else {
		lu = nmg_mlv(&s->l.magic, (struct vertex *)NULL, OT_SAME);
		fu = nmg_mf(lu);
		vu = RT_LIST_FIRST(vertexuse, &lu->down_hd);
		eu = nmg_meonvu(vu);
		while (--n) {
			(void)nmg_eusplit((struct vertex *)NULL, eu);
		}
	}
	if (rt_g.NMG_debug & DEBUG_BASIC)  {
		rt_log("nmg_cface(s=x%x, verts[]=x%x, n=%d) fu=x%x\n",
			s, verts, n, fu);
	}
	return (fu);
}

/*
 *			N M G _ A D D _ L O O P _ T O _ F A C E
 *
 *	Create a new loop within a face, given a list of vertices.
 *	Modified version of nmg_cface().
 *
 *	"verts" is an array of "n" pointers to (struct vertex).  "s" is the
 *	parent shell for the new face.  The face will consist of a single loop
 *	made from edges between the n vertices.
 *
 *	If verts is a null pointer (no vertex list), all vertices of the face
 *	will be new points.  Otherwise, verts is a pointer to a list of
 *	vertices to use in creating the face/loop.  Null entries within the
 *	list will cause a new vertex to be created for that point.  Such new
 *	vertices will be inserted into the list for return to the caller.
 *
 *	The vertices should be listed in "counter-clockwise" (CCW) order if
 *	this is an ordinary face (loop), and in "clockwise" (CW) order if
 *	this is an interior ("hole" or "subtracted") face (loop).  This
 *	routine makes only topology, without reference to any geometry.
 *
 *	Note that this routine inserts new vertices (by edge use splitting)
 *	at the head of the loop, which reverses the order.  Therefore, the
 *	caller's vertices are traversed in reverse order to counter this
 *	behavior, and to effect the proper vertex order in the final face
 *	loop.
 */
struct faceuse *
nmg_add_loop_to_face(s, fu, verts, n, dir)
struct shell *s;
struct faceuse *fu;
struct vertex *verts[];
int n, dir;
{
	int i, j;
	struct edgeuse *eu;
	struct loopuse *lu;
	struct vertexuse *vu;

	NMG_CK_SHELL(s);
	if (n < 1) {
		rt_log("nmg_add_loop_to_face(s=x%x, verts=x%x, n=%d.)\n",
			s, verts, n );
		rt_bomb("nmg_add_loop_to_face: request to make 0 faces\n");
	}

	if (verts) {
		if (!fu) {
			lu = nmg_mlv(&s->l.magic, verts[n-1], dir);
			fu = nmg_mf(lu);
		} else {
			lu = nmg_mlv(&fu->l.magic, verts[n-1], dir);
		}
		vu = RT_LIST_FIRST(vertexuse, &lu->down_hd);
		eu = nmg_meonvu(vu);
		if (!verts[n-1])
			verts[n-1] = eu->vu_p->v_p;

		for (i = n-2 ; i >= 0 ; i--) {
			eu = RT_LIST_FIRST(edgeuse, &lu->down_hd);
			eu = nmg_eusplit(verts[i], eu);
			if (!verts[i])
				verts[i] = eu->vu_p->v_p;
		}
	} else {
		if (!fu) {
			lu = nmg_mlv(&s->l.magic, (struct vertex *)NULL, dir);
			fu = nmg_mf(lu);
		} else {
			lu = nmg_mlv(&fu->l.magic, (struct vertex *)NULL, dir);
		}
		vu = RT_LIST_FIRST(vertexuse, &lu->down_hd);
		eu = nmg_meonvu(vu);
		while (--n) {
			(void)nmg_eusplit((struct vertex *)NULL, eu);
		}
	}
	if (rt_g.NMG_debug & DEBUG_BASIC)  {
		rt_log("nmg_add_loop_to_face(s=x%x, fu=x%x, verts[]=x%x, n=%d, %s) fu=x%x\n",
			s, fu, verts, n,
			nmg_orientation(dir) );
	}
	return (fu);
}

/*
 *			N M G _ F U _ P L A N E E Q N
 *
 *  Given a convex face that has been constructed with edges listed in
 *  counter-clockwise (CCW) order, compute the surface normal and plane
 *  equation for this face.
 *
 *
 *			D                   C
 *	                *-------------------*
 *	                |                   |
 *	                |   .<...........   |
 *	   ^     N      |   .           ^   |     ^
 *	   |      \     |   .  counter- .   |     |
 *	   |       \    |   .   clock   .   |     |
 *	   |C-B     \   |   .   wise    .   |     |C-B
 *	   |         \  |   v           .   |     |
 *	   |          \ |   ...........>.   |     |
 *	               \|                   |
 *	                *-------------------*
 *	                A                   B
 *			      <-----
 *				A-B
 *
 *  If the vertices in the loop are given in the order A B C D
 *  (e.g., counter-clockwise),
 *  then the outward pointing surface normal can be computed as:
 *
 *		N = (C-B) x (A-B)
 *
 *  This is the "right hand rule".
 *  For reference, note that a vector which points "into" the loop
 *  can be subsequently found by taking the cross product of the
 *  surface normal and any edge vector, e.g.:
 *
 *		Left = N x (B-A)
 *	or	Left = N x (C-B)
 *
 *  This routine will skip on past edges that start and end on
 *  the same vertex, in an attempt to avoid trouble.
 *  However, the loop *must* be convex for this routine to work.
 *  Otherwise, the surface normal may be inadvertently reversed.
 *
 *  Returns -
 *	0	OK
 *	-1	failure
 */
int
nmg_fu_planeeqn( fu, tol )
struct faceuse		*fu;
CONST struct rt_tol	*tol;
{
	struct edgeuse		*eu, *eu_final, *eu_next;
	struct loopuse		*lu;
	plane_t			plane;
	struct vertex		*a, *b, *c;
	register int		both_equal;

	RT_CK_TOL(tol);

	NMG_CK_FACEUSE(fu);
	lu = RT_LIST_FIRST(loopuse, &fu->lu_hd);
	NMG_CK_LOOPUSE(lu);

	if( RT_LIST_FIRST_MAGIC(&lu->down_hd) != NMG_EDGEUSE_MAGIC )
	{
		rt_log( "nmg_fu_planeeqn(): First loopuse does not contain edges\n" );
		return(-1);
	}
	eu = RT_LIST_FIRST(edgeuse, &lu->down_hd);
	NMG_CK_EDGEUSE(eu);
	NMG_CK_VERTEXUSE(eu->vu_p);
	a = eu->vu_p->v_p;
	NMG_CK_VERTEX(a);
	NMG_CK_VERTEX_G(a->vg_p);

	eu_next = eu;
	do {
		eu_next = RT_LIST_PNEXT_CIRC(edgeuse, eu_next);
		NMG_CK_EDGEUSE(eu_next);
		if( eu_next == eu )  return -1;
		NMG_CK_VERTEXUSE(eu_next->vu_p);
		b = eu_next->vu_p->v_p;
		NMG_CK_VERTEX(b);
		NMG_CK_VERTEX_G(b->vg_p);
	} while( (b == a
		|| VAPPROXEQUAL(a->vg_p->coord, b->vg_p->coord, tol->dist))
		&& eu_next->vu_p != eu->vu_p );

	eu_final = eu_next;
	do {
		eu_final = RT_LIST_PNEXT_CIRC(edgeuse, eu_final);
		NMG_CK_EDGEUSE(eu_final);
		if( eu_final == eu )  return -1;
		NMG_CK_VERTEXUSE(eu_final->vu_p);
		c = eu_final->vu_p->v_p;
		NMG_CK_VERTEX(c);
		NMG_CK_VERTEX_G(c->vg_p);
		both_equal = (c == b) ||
		    VAPPROXEQUAL(a->vg_p->coord, c->vg_p->coord, tol->dist) ||
		    VAPPROXEQUAL(b->vg_p->coord, c->vg_p->coord, tol->dist);
	} while( (both_equal
		|| rt_3pts_collinear(a->vg_p->coord, b->vg_p->coord,
			c->vg_p->coord, tol))
		&& eu_next->vu_p != eu->vu_p );

	if (rt_mk_plane_3pts(plane,
	    a->vg_p->coord, b->vg_p->coord, c->vg_p->coord, tol) < 0 ) {
		rt_log("nmg_fu_planeeqn(): rt_mk_plane_3pts failed on (%g,%g,%g) (%g,%g,%g) (%g,%g,%g)\n",
			V3ARGS( a->vg_p->coord ),
			V3ARGS( b->vg_p->coord ),
			V3ARGS( c->vg_p->coord ) );
	    	HPRINT("plane", plane);
		return(-1);
	}
	if (plane[0] == 0.0 && plane[1] == 0.0 && plane[2] == 0.0) {
		rt_log("nmg_fu_planeeqn():  Bad plane equation from rt_mk_plane_3pts\n" );
	    	HPRINT("plane", plane);
		return(-1);
	}
	nmg_face_g( fu, plane);

	if (rt_g.NMG_debug & DEBUG_BASIC)  {
		rt_log("nmg_fu_paneeqn(fu=x%x, tol=x%x)\n", fu, tol);
	}
	return(0);
}

/*
 *			N M G _ G L U E F A C E S
 *
 *	given a shell containing "n" faces whose outward oriented faceuses are
 *	enumerated in "fulist", glue the edges of the faces together
 *	Most especially useful after using nmg_cface() several times to
 *	make faces which share vertex structures.
 *
 */
void
nmg_gluefaces(fulist, n)
struct faceuse *fulist[];
int n;
{
	struct shell	*s;
	struct loopuse	*lu;
	struct edgeuse	*eu;
	int		i;
	int		f_no;		/* Face number */
	
	NMG_CK_FACEUSE(fulist[0]);
	s = fulist[0]->s_p;
	NMG_CK_SHELL(s);

	/* First, perform some checks */
	for (i = 0 ; i < n ; ++i) {
		register struct faceuse	*fu;

		fu = fulist[i];
		NMG_CK_FACEUSE(fu);
		if (fu->s_p != s) {
			rt_log("nmg_gluefaces() in %s at %d. faceuses don't share parent\n",
				__FILE__, __LINE__);
			rt_bomb("nmg_gluefaces\n");
		}
	}

	for (i=0 ; i < n ; ++i) {
		for( RT_LIST_FOR( lu , loopuse , &fulist[i]->lu_hd ) ) {
			NMG_CK_LOOPUSE( lu );

			if( RT_LIST_FIRST_MAGIC(&lu->down_hd) != NMG_EDGEUSE_MAGIC )
				continue;

			for( RT_LIST_FOR( eu, edgeuse, &lu->down_hd ) )  {
				for( f_no = i+1; f_no < n; f_no++ )  {
					struct loopuse		*lu2;
					register struct edgeuse	*eu2;

					if( eu->radial_p != eu->eumate_p )  break;

					for( RT_LIST_FOR( lu2 , loopuse , &fulist[f_no]->lu_hd ) ) {
						NMG_CK_LOOPUSE( lu2 );
						if( RT_LIST_FIRST_MAGIC(&lu2->down_hd) != NMG_EDGEUSE_MAGIC )
							continue;
						for( RT_LIST_FOR( eu2, edgeuse, &lu2->down_hd ) )  {
							if (EDGESADJ(eu, eu2))
							    	nmg_moveeu(eu, eu2);
						}
					}
				}
			}
		}
	}

	if (rt_g.NMG_debug & DEBUG_BASIC)  {
		rt_log("nmg_gluefaces(fulist=x%x, n=%d)\n", fulist, n);
	}
}


/*			N M G _ S I M P L I F Y _ F A C E
 *
 *
 *	combine adjacent loops within a face which serve no apparent purpose
 *	by remaining separate and distinct.  Kill "wire-snakes" in face.
 *
 * Returns -
 *	0	If all was OK
 *	1	If faceuse is now empty
 */
int
nmg_simplify_face(fu)
struct faceuse *fu;
{
	struct loopuse *lu, *lu2;
	int overlap;
	int ret_val;

	NMG_CK_FACEUSE(fu);

	for (RT_LIST_FOR(lu, loopuse, &fu->lu_hd))  {
		nmg_simplify_loop(lu);
	}

	for (RT_LIST_FOR(lu, loopuse, &fu->lu_hd))  {
		if( nmg_kill_snakes(lu) )  {
			struct loopuse	*klu = lu;
			lu = RT_LIST_PREV( loopuse, &lu->l );
			nmg_klu(klu);
		}
	}

	ret_val = RT_LIST_IS_EMPTY(&fu->lu_hd);

	if (rt_g.NMG_debug & DEBUG_BASIC)  {
		rt_log("nmg_simplify_face(fut=x%x) return=%d\n", fu, ret_val);
	}

	return( ret_val );
}

/*
 *			N M G _ R E V E R S E _ F A C E
 *
 *
 *  This routine reverses the direction of the Normal vector which defines
 *  the plane of the face.  
 *
 *  The OT_SAME faceuse becomes the OT_OPPOSITE faceuse, and vice versa.  
 *  This preserves the convention that OT_SAME loopuses in the
 *  OT_SAME faceuse are counter-clockwise rotating about the surface normal.
 *
 *
 *	     Before			After
 *
 * 
 * N	  OT_SAME		  OT_OPPOSITE
 *  \	.<---------.		.<---------.
 *   \	|fu        ^		|fu        ^
 *    \	|  .------ | ->.	|  .------ | ->.
 *     \|  ^fumate |   |	|  ^fumate |   |
 *	|  |       |   |	|  |       |   |
 *	|  |       |   |	|  |       |   |
 *	V  |       |   |	V  |       |   |\
 *	.--------->.   |	.--------->.   | \
 *	   |           V	   |           V  \
 *	   .<----------.	   .<----------.   \
 *	    OT_OPPOSITE		     OT_SAME        \
 *						     N
 *
 *
 *  Also note that this reverses the same:opposite:opposite:same
 *  parity radially around each edge.  This can create parity errors
 *  until all faces of this shell have been processed.
 *
 *  Applications programmers should use nmg_invert_shell(),
 *  which does not have this problem.
 *  This routine is for internal use only.
 */
void
nmg_reverse_face( fu )
register struct faceuse	*fu;
{
	register struct faceuse	*fumate;
	register vectp_t	v;

	NMG_CK_FACEUSE(fu);
	fumate = fu->fumate_p;
	NMG_CK_FACEUSE(fumate);
	NMG_CK_FACE(fu->f_p);
	NMG_CK_FACE_G(fu->f_p->fg_p);

	/* reverse face normal vector */
	fu->f_p->flip = !fu->f_p->flip;

	/* switch which face is "outside" face */
	if (fu->orientation == OT_SAME)  {
		if (fumate->orientation != OT_OPPOSITE)  {
			rt_log("nmg_reverse_face(fu=x%x) fu:SAME, fumate:%d\n",
				fu, fumate->orientation);
			rt_bomb("nmg_reverse_face() orientation clash\n");
		} else {
			fu->orientation = OT_OPPOSITE;
			fumate->orientation = OT_SAME;
		}
	} else if (fu->orientation == OT_OPPOSITE) {
		if (fumate->orientation != OT_SAME)  {
			rt_log("nmg_reverse_face(fu=x%x) fu:OPPOSITE, fumate:%d\n",
				fu, fumate->orientation);
			rt_bomb("nmg_reverse_face() orientation clash\n");
		} else {
			fu->orientation = OT_SAME;
			fumate->orientation = OT_OPPOSITE;
		}
	} else {
		/* Unoriented face? */
		rt_log("ERROR nmg_reverse_face(fu=x%x), orientation=%d.\n",
			fu, fu->orientation );
	}

	if (rt_g.NMG_debug & DEBUG_BASIC)  {
		rt_log("nmg_reverse_face(fu=x%x) fumate=x%x\n", fu, fumate);
	}
}

/*
 *			N M G _ F A C E _ F I X _ R A D I A L _ P A R I T Y
 *
 *  Around an edge, consider all the edgeuses that belong to a single shell.
 *  The faceuses pertaining to those edgeuses must maintain the appropriate
 *  parity with their radial faceuses, so that OT_SAME is always radial to
 *  OT_SAME, and OT_OPPOSITE is always radial to OT_OPPOSITE.
 *
 *  If a radial edgeuse is encountered that belongs to *this* face, then
 *  it might not have been processed by this routine yet, and is ignored
 *  for the purposes of checking parity.
 *
 *  When moving faces between shells, sometimes this parity relationship
 *  needs to be fixed, which can be easily accomplished by exchanging
 *  the incorrect edgeuse with it's mate in the radial edge linkages.
 *
 *  XXX Note that this routine will not work right in the presence of
 *  XXX dangling faces.
 *
 *  Note that this routine can't be used incrementally, because
 *  after an odd number (like one) of faceuses have been "fixed",
 *  there is an inherrent parity error, which will cause wrong
 *  decisions to be made.  Therefore, *all* faces have to be moved from
 *  one shell to another before the radial parity can be "fixed".
 *  Even then, this isn't going to work right unless we are given
 *  a list of all the "suspect" faceuses, so the initial parity
 *  value can be properly established.
 *
 *  XXXX I am of the opinion this routine is neither useful nor correct
 *  XXXX in it's present form, except for limited special cases.
 *
 *  The 'tol' arg is used strictly for printing purposes.
 *
 *  Returns -
 *	count of number of edges fixed.
 */
int
nmg_face_fix_radial_parity( fu, tol )
struct faceuse		*fu;
CONST struct rt_tol	*tol;
{
	struct loopuse *lu;
	struct edgeuse *eu;
	struct faceuse *fu2;
	struct shell	*s;
	long		count = 0;

	NMG_CK_FACEUSE( fu );
	s = fu->s_p;
	NMG_CK_SHELL(s);
	RT_CK_TOL(tol);

	/* Make sure we are now dealing with the OT_SAME faceuse */
	if( fu->orientation == OT_SAME )
		fu2 = fu;
	else
		fu2 = fu->fumate_p;

	for( RT_LIST_FOR( lu , loopuse , &fu2->lu_hd ) )  {
		NMG_CK_LOOPUSE( lu );

		/* skip loops of a single vertex */
		if( RT_LIST_FIRST_MAGIC( &lu->down_hd ) != NMG_EDGEUSE_MAGIC )
			continue;

		/*
		 *  Consider every edge of this loop.
		 *  Initial sequencing is:
		 *    before(radial), eu, eumate, after(radial)
		 */
		for( RT_LIST_FOR( eu , edgeuse , &lu->down_hd ) )  {
			struct edgeuse	*eumate;
			struct edgeuse	*before;
			struct edgeuse	*sbefore;	/* searched before */
			struct edgeuse	*after;

			eumate = eu->eumate_p;
			before = eu->radial_p;
			after = eumate->radial_p;
			NMG_CK_EDGEUSE(eumate);
			NMG_CK_EDGEUSE(before);
			NMG_CK_EDGEUSE(after);

			/* If no other edgeuses around edge, done. */
			if( before == eumate )
				continue;

			/*
			 *  Search in 'before' direction, until it's in
			 *  same shell.  Also ignore edgeuses from this FACE,
			 *  as they may not have been fixed yet.
			 */
			for( sbefore = before;
			     sbefore != eu && sbefore != eumate;
			     sbefore = sbefore->eumate_p->radial_p
			)  {
				struct faceuse	*bfu;
				if( nmg_find_s_of_eu(sbefore) != s )  continue;

				bfu = nmg_find_fu_of_eu(sbefore);
				/* If edgeuse isn't part of a faceuse, skip */
				if( !bfu )  continue;
				if( bfu->f_p == fu->f_p )  continue;
				/* Found a candidate */
				break;
			}

			/* If search found no others from this shell, done. */
			if( sbefore == eu || sbefore == eumate )
				continue;

			/*
			 *  'eu' is in an OT_SAME faceuse.
			 *  If the first faceuse in the 'before' direction
			 *  from this shell is OT_SAME, no fix is required.
			 */
			if( sbefore->up.lu_p->up.fu_p->orientation == OT_SAME )
				continue;

#if 0
rt_log("sbefore eu=x%x, before=x%x, eu=x%x, eumate=x%x, after=x%x\n",
			sbefore, before, eu, eumate, after );
nmg_pr_fu_around_eu(eu, tol );
#endif
			/*
			 *  Rearrange order to be: before, eumate, eu, after.
			 *  NOTE: do NOT use sbefore here.
			 */
			before->radial_p = eumate;
			eumate->radial_p = before;

			after->radial_p = eu;
			eu->radial_p = after;
			count++;
			if( rt_g.NMG_debug & DEBUG_BASIC )  {
				rt_log("nmg_face_fix_radial_parity() exchanging eu=x%x & eumate=x%x on edge x%x\n",
					eu, eumate, eu->e_p);
			}
#if 0
			/* Can't do this incrementally, it blows up after 1st "fix" */
			if( rt_g.NMG_debug )  {
nmg_pr_fu_around_eu(eu, tol );
				if( nmg_check_radial( eu, tol ) )
					rt_bomb("nmg_face_fix_radial_parity(): nmg_check_radial failed\n");
			}
#endif
		}
	}

	if (rt_g.NMG_debug & DEBUG_BASIC)  {
		rt_log("nmg_face_fix_radial_parity(fu=x%x) returns %d\n", fu, count);
	}

	return count;
}

/*
 *			N M G _ M V _ F U _ B E T W E E N _ S H E L L S
 *
 *  Move faceuse from 'src' shell to 'dest' shell.
 */
void
nmg_mv_fu_between_shells( dest, src, fu )
struct shell		*dest;
register struct shell	*src;
register struct faceuse	*fu;
{
	register struct faceuse	*fumate;

	NMG_CK_FACEUSE(fu);
	fumate = fu->fumate_p;
	NMG_CK_FACEUSE(fumate);

	if (fu->s_p != src) {
		rt_log("nmg_mv_fu_between_shells(dest=x%x, src=x%x, fu=x%x), fu->s_p=x%x isnt src shell\n",
			dest, src, fu, fu->s_p );
		rt_bomb("fu->s_p isnt source shell\n");
	}
	if (fumate->s_p != src) {
		rt_log("nmg_mv_fu_between_shells(dest=x%x, src=x%x, fu=x%x), fumate->s_p=x%x isn't src shell\n",
			dest, src, fu, fumate->s_p );
		rt_bomb("fumate->s_p isnt source shell\n");
	}

	/* Remove fu from src shell */
	RT_LIST_DEQUEUE( &fu->l );
	if( RT_LIST_IS_EMPTY( &src->fu_hd ) )  {
		/* This was the last fu in the list, bad news */
		rt_log("nmg_mv_fu_between_shells(dest=x%x, src=x%x, fu=x%x), fumate=x%x not in src shell\n",
			dest, src, fu, fumate );
		rt_bomb("src shell emptied before finding fumate\n");
	}

	/* Remove fumate from src shell */
	RT_LIST_DEQUEUE( &fumate->l );

	/*
	 * Add fu and fumate to dest shell,
	 * preserving implicit OT_SAME, OT_OPPOSITE order
	 */
	if( fu->orientation == OT_SAME )  {
		RT_LIST_APPEND( &dest->fu_hd, &fu->l );
		RT_LIST_APPEND( &fu->l, &fumate->l );
	} else {
		RT_LIST_APPEND( &dest->fu_hd, &fumate->l );
		RT_LIST_APPEND( &fumate->l, &fu->l );
	}

	fu->s_p = dest;
	fumate->s_p = dest;

	if (rt_g.NMG_debug & DEBUG_BASIC)  {
		rt_log("nmg_mv_fu_between_shells(dest=x%x, src=x%x, fu=x%x)\n", dest , src , fu);
	}
}

/*
 *			N M G _ M O V E _ F U _ F U
 *
 *  Move everything from the source faceuse into the destination faceuse.
 *  Exists as a support routine for nmg_jf() only.
 */
static void
nmg_move_fu_fu( dest_fu, src_fu )
register struct faceuse	*dest_fu;
register struct faceuse	*src_fu;
{
	register struct loopuse	*lu;

	NMG_CK_FACEUSE(dest_fu);
	NMG_CK_FACEUSE(src_fu);

	if( dest_fu->orientation != src_fu->orientation )
		rt_bomb("nmg_move_fu_fu: differing orientations\n");

	/* Move all loopuses from src to dest faceuse */
	while( RT_LIST_WHILE( lu, loopuse, &src_fu->lu_hd ) )  {
		RT_LIST_DEQUEUE( &(lu->l) );
		RT_LIST_INSERT( &(dest_fu->lu_hd), &(lu->l) );
		lu->up.fu_p = dest_fu;
	}
	/* The src_fu is invalid here, with an empty lu_hd list */

	if (rt_g.NMG_debug & DEBUG_BASIC)  {
		rt_log("nmg_move_fu_fu(dest_fu=x%x, src_fu=x%x)\n", dest_fu , src_fu);
	}
}

/*
 *			N M G _ J F
 *
 *  Join two faces together by
 *  moving everything from the source faceuse and mate into the
 *  destination faceuse and mate, taking into account face orientations.
 *  The source face is destroyed by this operation.
 */
void
nmg_jf(dest_fu, src_fu)
register struct faceuse	*dest_fu;
register struct faceuse	*src_fu;
{
	NMG_CK_FACEUSE(dest_fu);
	NMG_CK_FACEUSE(src_fu);

	if (rt_g.NMG_debug & DEBUG_BASIC)  {
		rt_log("nmg_jf(dest_fu=x%x, src_fu=x%x)\n", dest_fu, src_fu);
	}

	if( src_fu->f_p == dest_fu->f_p )
		rt_bomb("nmg_jf() src and dest faces are the same\n");

	if( dest_fu->orientation == src_fu->orientation )  {
		nmg_move_fu_fu(dest_fu, src_fu);
		nmg_move_fu_fu(dest_fu->fumate_p, src_fu->fumate_p);
	} else {
		nmg_move_fu_fu(dest_fu, src_fu->fumate_p);
		nmg_move_fu_fu(dest_fu->fumate_p, src_fu);
	}
	/* The src_fu is invalid here, having an empty lu_hd list */
	nmg_kfu(src_fu);

}

/*
 *			N M G _ D U P _ F A C E
 *
 *  Construct a duplicate of a face into the shell 's'.
 *  The vertex geometry is copied from the source face into topologically
 *  distinct (new) vertex and vertex_g structs.
 *  They will start out being geometricly coincident, but it is anticipated
 *  that the caller will modify the geometry, e.g. as in an extrude operation.
 *
 *  It is the caller's responsibility to re-bound the new face after
 *  making any modifications.
 */
struct faceuse *
nmg_dup_face(fu, s)
struct faceuse *fu;
struct shell *s;
{
	struct loopuse *new_lu, *lu;
	struct faceuse *new_fu = (struct faceuse *)NULL;
	struct model	*m;
	long		**trans_tbl;

	NMG_CK_FACEUSE(fu);
	NMG_CK_FACE(fu->f_p);
	NMG_CK_SHELL(s);

	m = nmg_find_model( (long *)s );
	/* Needs double space, because model will grow as dup proceeds */
	trans_tbl = (long **)rt_calloc(m->maxindex*2, sizeof(long *),
			"nmg_dup_face trans_tbl");

	for (RT_LIST_FOR(lu, loopuse, &fu->lu_hd)) {
		if (new_fu) {
			new_lu = nmg_dup_loop(lu, &new_fu->l.magic, trans_tbl);
		} else {
			new_lu = nmg_dup_loop(lu, &s->l.magic, trans_tbl);
			new_fu = nmg_mf(new_lu);
		}
	}

	if (fu->f_p->fg_p) {
		plane_t		n;
		if( fu->orientation == OT_SAME )  {
			NMG_GET_FU_PLANE( n, fu );
		} else {
			NMG_GET_FU_PLANE( n, fu->fumate_p );
		}
		nmg_face_g(new_fu, n);
	}
	new_fu->orientation = fu->orientation;
	new_fu->fumate_p->orientation = fu->fumate_p->orientation;

	rt_free((char *)trans_tbl, "nmg_dup_face trans_tbl");

	if (rt_g.NMG_debug & DEBUG_BASIC)  {
		rt_log("nmg_dup_face(fu=x%x, s=x%x) new_fu=x%x\n",
			fu, s, new_fu);
	}

	return(new_fu);
}

/************************************************************************
 *									*
 *				LOOP Routines				*
 *									*
 ************************************************************************/

/*
 *			N M G _ J L
 *
 *  Join two loops together which share a common edge,
 *  such that both occurances of the common edge are deleted.
 *  This routine always leaves "lu" intact, and kills the loop
 *  radial to "eu" (after stealing all it's edges).
 *
 *  Either both loops must be of the same orientation, or then
 *  first loop must be OT_SAME, and the second loop must be OT_OPPOSITE.
 *  Joining OT_SAME & OT_OPPOSITE always gives an OT_SAME result.
 *  Since "lu" must survive, it must be the OT_SAME one.
 */
void
nmg_jl(lu, eu)
struct loopuse *lu;
struct edgeuse *eu;
{
	struct loopuse	*lu2;
	struct edgeuse	*eu_r;		/* use of shared edge in lu2 */
	struct edgeuse	*nexteu;

	NMG_CK_LOOPUSE(lu);

	NMG_CK_EDGEUSE(eu);
	NMG_CK_EDGEUSE(eu->eumate_p);
	eu_r = eu->radial_p;
	NMG_CK_EDGEUSE(eu_r);
	NMG_CK_EDGEUSE(eu_r->eumate_p);

	if (rt_g.NMG_debug & DEBUG_BASIC)  {
		rt_log("nmg_jl(lu=x%x, eu=x%x)\n", lu, eu);
	}

	if (eu->up.lu_p != lu)
		rt_bomb("nmg_jl: edgeuse is not child of loopuse?\n");

	lu2 = eu_r->up.lu_p;
	if (lu2->l.magic != NMG_LOOPUSE_MAGIC)
		rt_bomb("nmg_jl: radial edgeuse not part of loopuse\n");

	if (lu2 == lu)
		rt_bomb("nmg_jl: trying to join a loop to itself\n");

	if (lu->up.magic_p != lu2->up.magic_p)
		rt_bomb("nmg_jl: loopuses do not share parent\n");

	if (lu2->orientation != lu->orientation)  {
		if( lu->orientation != OT_SAME || lu2->orientation != OT_OPPOSITE )  {
			rt_log("nmg_jl: lu2 = %s, lu = %s\n",
				nmg_orientation(lu2->orientation),
				nmg_orientation(lu->orientation) );
			rt_bomb("nmg_jl: can't join loops of different orientation!\n");
		} else {
			/* Consuming an OPPOSITE into a SAME is OK */
		}
	}

	if (eu->radial_p->eumate_p->radial_p->eumate_p != eu ||
	    eu->eumate_p->radial_p->eumate_p->radial_p != eu)
	    	rt_bomb("nmg_jl: edgeuses must be sole uses of edge to join loops\n");

	/*
	 * Remove all the edgeuses "ahead" of our radial and insert them
	 * "behind" the current edgeuse.
	 * Operates on lu and lu's mate simultaneously.
	 */
	nexteu = RT_LIST_PNEXT_CIRC(edgeuse, eu_r);
	while (nexteu != eu_r) {
		RT_LIST_DEQUEUE(&nexteu->l);
		RT_LIST_INSERT(&eu->l, &nexteu->l);
		nexteu->up.lu_p = eu->up.lu_p;

		RT_LIST_DEQUEUE(&nexteu->eumate_p->l);
		RT_LIST_APPEND(&eu->eumate_p->l, &nexteu->eumate_p->l);
		nexteu->eumate_p->up.lu_p = eu->eumate_p->up.lu_p;

		nexteu = RT_LIST_PNEXT_CIRC(edgeuse, eu_r);
	}

	/*
	 * The other loop just has the one (shared) edgeuse left in it.
	 * Delete the other loop (and it's mate).
	 */
	nmg_klu(lu2);

	/*
	 * Kill the one remaining use of the (formerly) "shared" edge in lu
	 * and voila: one contiguous loop.
	 */
	if( nmg_keu(eu) )  rt_bomb("nmg_jl() loop vanished?\n");

}

/*
 *			N M G _ J O I N _ 2 L O O P S
 *
 *  Intended to join an interior and exterior loop together,
 *  by building a bridge between the two indicated vertices.
 *
 *  This routine can be used to join two exterior loops which do not
 *  overlap, and it can also be used to join an exterior loop with
 *  a loop of oposite orientation that lies entirely within it.
 *  This restriction is important, but not checked for.
 *
 *  If the two vertexuses reference distinct vertices, then two new
 *  edges are built to bridge the loops together.
 *  If the two vertexuses share the same vertex, then it is even easier.
 *
 *  Returns the replacement for vu2.
 */
struct vertexuse *
nmg_join_2loops( vu1, vu2 )
struct vertexuse	*vu1;
struct vertexuse	*vu2;
{
	struct edgeuse	*eu1, *eu2;
	struct edgeuse	*new_eu;
	struct edgeuse	*first_new_eu;
	struct edgeuse	*second_new_eu;
	struct edgeuse	*final_eu2;
	struct loopuse	*lu1, *lu2;
	int		new_orient;

	NMG_CK_VERTEXUSE(vu1);
	NMG_CK_VERTEXUSE(vu2);
	eu1 = vu1->up.eu_p;
	eu2 = vu2->up.eu_p;
	NMG_CK_EDGEUSE(eu1);
	NMG_CK_EDGEUSE(eu2);
	lu1 = eu1->up.lu_p;
	lu2 = eu2->up.lu_p;
	NMG_CK_LOOPUSE(lu1);
	NMG_CK_LOOPUSE(lu2);

	if (rt_g.NMG_debug & DEBUG_BASIC)  {
		rt_log("nmg_join_2loops( vu1=x%x, vu2=x%x )\n", vu1, vu2 );
	}

	if( lu1 == lu2 || lu1->l_p == lu2->l_p )
		rt_bomb("nmg_join_2loops: can't join loop to itself\n");

	if( lu1->up.fu_p != lu2->up.fu_p )
		rt_bomb("nmg_join_2loops: can't join loops in different faces\n");

	/* Joining same & opp gives same.  */
	if( lu1->orientation != lu2->orientation )  {
		new_orient = OT_SAME;
	} else {
		new_orient = lu1->orientation;
	}

	if( vu1->v_p != vu2->v_p )  {
		/*
		 *  Start by taking a jaunt from vu1 to vu2 and back.
		 */

		/* insert 0 length edge, before eu1 */
		first_new_eu = nmg_eins(eu1);

		/* split the new edge, and connect it to vertex 2 */
		second_new_eu = nmg_eusplit( vu2->v_p, first_new_eu );
		first_new_eu = RT_LIST_PPREV_CIRC(edgeuse, second_new_eu);

		/* Make the two new edgeuses share just one edge */
		nmg_moveeu( second_new_eu, first_new_eu );

		/* first_new_eu is eu that enters shared vertex */
		vu1 = second_new_eu->vu_p;
	} else {
		second_new_eu = eu1;
		first_new_eu = RT_LIST_PPREV_CIRC(edgeuse, second_new_eu);
		NMG_CK_EDGEUSE(second_new_eu);
	}
	/* second_new_eu is eu that departs from shared vertex */
	vu2 = second_new_eu->vu_p;	/* replacement for original vu2 */
	if( vu1->v_p != vu2->v_p )  rt_bomb("nmg_join_2loops: jaunt failed\n");

	/*
	 *  Gobble edges off of loop2 (starting with eu2),
	 *  and insert them into loop1,
	 *  between first_new_eu and second_new_eu.
	 *  The final edge from loop 2 will then be followed by
	 *  second_new_eu.
	 */
	final_eu2 = RT_LIST_PPREV_CIRC(edgeuse, eu2 );
	while( RT_LIST_NON_EMPTY( &lu2->down_hd ) )  {
		eu2 = RT_LIST_PNEXT_CIRC(edgeuse, final_eu2);

		RT_LIST_DEQUEUE(&eu2->l);
		RT_LIST_INSERT(&second_new_eu->l, &eu2->l);
		eu2->up.lu_p = lu1;

		RT_LIST_DEQUEUE(&eu2->eumate_p->l);
		RT_LIST_APPEND(&second_new_eu->eumate_p->l, &eu2->eumate_p->l);
		eu2->eumate_p->up.lu_p = lu1->lumate_p;
	}

	lu1->orientation = lu1->lumate_p->orientation = new_orient;

	/* Kill entire (null) loop associated with lu2 */
	nmg_klu(lu2);

	nmg_veu( &lu1->down_hd, &lu1->l.magic );	/* XXX sanity */

	return vu2;
}

/* XXX These should be included in nmg_join_2loops, or be called by it */
/*
 *			N M G _ J O I N _ S I N G V U _ L O O P
 *
 *  vu1 is in a regular loop, vu2 is in a loop of a single vertex
 *  A jaunt is taken from vu1 to vu2 and back to vu1, and the
 *  old loop at vu2 is destroyed.
 *  Return is the new vu that replaces vu2.
 */
struct vertexuse *
nmg_join_singvu_loop( vu1, vu2 )
struct vertexuse	*vu1, *vu2;
{
    	struct edgeuse	*eu1;
	struct edgeuse	*first_new_eu, *second_new_eu;
	struct loopuse	*lu2;

	NMG_CK_VERTEXUSE( vu1 );
	NMG_CK_VERTEXUSE( vu2 );

	if (rt_g.NMG_debug & DEBUG_BASIC)  {
		rt_log("nmg_join_singvu_loop( vu1=x%x, vu2=x%x )\n", vu1, vu2 );
	}

	if( *vu2->up.magic_p != NMG_LOOPUSE_MAGIC ||
	    *vu1->up.magic_p != NMG_EDGEUSE_MAGIC )  rt_bomb("nmg_join_singvu_loop bad args\n");

	if( vu1->v_p == vu2->v_p )  rt_bomb("nmg_join_singvu_loop same vertex\n");

    	/* Take jaunt from vu1 to vu2 and back */
    	eu1 = vu1->up.eu_p;
    	NMG_CK_EDGEUSE(eu1);

    	/* Insert 0 length edge */
    	first_new_eu = nmg_eins(eu1);
	/* split the new edge, and connect it to vertex 2 */
	second_new_eu = nmg_eusplit( vu2->v_p, first_new_eu );
	first_new_eu = RT_LIST_PLAST_CIRC(edgeuse, second_new_eu);
	/* Make the two new edgeuses share just one edge */
	nmg_moveeu( second_new_eu, first_new_eu );

	/* Kill loop lu2 associated with vu2 */
	lu2 = vu2->up.lu_p;
	NMG_CK_LOOPUSE(lu2);
	nmg_klu(lu2);

	return second_new_eu->vu_p;
}

/*
 *			N M G _ J O I N _ 2 S I N G V U _ L O O P S
 *
 *  Both vertices are part of single vertex loops.
 *  Converts loop on vu1 into a real loop that connects them together,
 *  with a single edge (two edgeuses).
 *  Loop on vu2 is killed.
 *  Returns replacement vu for vu2.
 *  Does not change the orientation.
 */
struct vertexuse *
nmg_join_2singvu_loops( vu1, vu2 )
struct vertexuse	*vu1, *vu2;
{
    	struct edgeuse	*eu1;
	struct edgeuse	*first_new_eu, *second_new_eu;
	struct loopuse	*lu1, *lu2;

	if (rt_g.NMG_debug & DEBUG_BASIC)  {
		rt_log("nmg_join_2singvu_loops( vu1=x%x, vu2=x%x )\n", vu1, vu2 );
	}

	NMG_CK_VERTEXUSE( vu1 );
	NMG_CK_VERTEXUSE( vu2 );

	if( *vu2->up.magic_p != NMG_LOOPUSE_MAGIC ||
	    *vu1->up.magic_p != NMG_LOOPUSE_MAGIC )  rt_bomb("nmg_join_2singvu_loops bad args\n");

	if( vu1->v_p == vu2->v_p )  rt_bomb("nmg_join_2singvu_loops same vertex\n");

    	/* Take jaunt from vu1 to vu2 and back */
	/* Make a 0 length edge on vu1 */
	first_new_eu = nmg_meonvu(vu1);
	/* split the new edge, and connect it to vertex 2 */
	second_new_eu = nmg_eusplit( vu2->v_p, first_new_eu );
	first_new_eu = RT_LIST_PLAST_CIRC(edgeuse, second_new_eu);
	/* Make the two new edgeuses share just one edge */
	nmg_moveeu( second_new_eu, first_new_eu );

	/* Kill loop lu2 associated with vu2 */
	lu2 = vu2->up.lu_p;
	NMG_CK_LOOPUSE(lu2);
	nmg_klu(lu2);

	lu1 = vu1->up.eu_p->up.lu_p;
	NMG_CK_LOOPUSE(lu1);

	return second_new_eu->vu_p;
}

/*			N M G _ C U T _ L O O P
 *
 *	Divide a loop of edges between two vertexuses.
 *
 *	Make a new loop between the two vertexes, and split it and
 *	the loop of the vertexuses at the same time.
 *
 *
 *		BEFORE					AFTER
 *
 *
 *     Va    eu1  vu1		Vb	       Va   eu1  vu1             Vb
 *	* <---------* <---------*		* <--------*  * <--------*
 *	|					|	      |		 
 *	|			^		|	   ^  |		 ^
 *	|	 Original	|		| Original |  |	   New   |
 *	|	   Loopuse	|		| Loopuse  |  |	 Loopuse |
 *	V			|		V          |  V	/	 |
 *				|		           |   /	 |
 *	*----------> *--------> *		*--------> *  *--------> *
 *     Vd	     vu2 eu2	Vc	       Vd             vu2  eu2   Vc
 *
 *  Returns the new loopuse pointer.  The new loopuse will contain "vu2"
 *  and the edgeuse associated with "vu2" as the FIRST edgeuse on the
 *  list of edgeuses.  The edgeuse for the new edge  (connecting
 *  the verticies indicated by vu1 and vu2) will be the LAST edgeuse on the
 *  new loopuse's list of edgeuses.
 *
 *  It is the caller's responsibility to re-bound the loops.
 *
 *  Both old and new loopuse will have orientation OT_UNSPEC.  It is the
 *  callers responsibility to determine what the orientations should be.
 *  This can be conveniently done with nmg_lu_reorient().
 *
 *  Here is a simple example of how the new loopuse might have a different
 *  orientation than the original one:
 *
 *
 *		F<----------------E
 *		|                 ^
 *		|                 |
 *		|      C--------->D
 *		|      ^          .
 *		|      |          .
 *		|      |          .
 *		|      B<---------A
 *		|                 ^
 *		v                 |
 *		G---------------->H
 *
 *  When nmg_cut_loop(A,D) is called, the new loop ABCD is clockwise,
 *  even though the original loop was counter-clockwise.
 *  There is no way to determine this without referring to the
 *  face normal and vertex geometry, which being a topology routine
 *  this routine shouldn't do.
 *
 *  Returns -
 *	NULL	Error
 *	lu	Loopuse of new loop, on success.
 */
struct loopuse *
nmg_cut_loop(vu1, vu2)
struct vertexuse *vu1, *vu2;
{
	struct loopuse *lu, *oldlu;
	struct edgeuse *eu1, *eu2, *eunext, *neweu, *eu;
	struct model	*m;
	FILE		*fd;
	char		name[32];
	static int	i=0;

	NMG_CK_VERTEXUSE(vu1);
	NMG_CK_VERTEXUSE(vu2);

	eu1 = vu1->up.eu_p;
	eu2 = vu2->up.eu_p;
	NMG_CK_EDGEUSE(eu1);
	NMG_CK_EDGEUSE(eu2);
	oldlu = eu1->up.lu_p;
	NMG_CK_LOOPUSE(oldlu);

	if (eu2->up.lu_p != oldlu) {
		rt_bomb("nmg_cut_loop() vertices not decendants of same loop\n");
	}

	if( vu1->v_p == vu2->v_p )  {
		/* The loops touch, use a different routine */
		lu = nmg_split_lu_at_vu( oldlu, vu1 );
		goto out;
	}

	NMG_CK_FACEUSE(oldlu->up.fu_p);
	m = oldlu->up.fu_p->s_p->r_p->m_p;
	NMG_CK_MODEL(m);

	if (rt_g.NMG_debug & DEBUG_CUTLOOP) {
		rt_log("\tnmg_cut_loop\n");
		if (rt_g.NMG_debug & DEBUG_PLOTEM) {
			long		*tab;
			tab = (long *)rt_calloc( m->maxindex, sizeof(long),
				"nmg_cut_loop flag[] 1" );

			(void)sprintf(name, "Before_cutloop%d.pl", ++i);
			rt_log("nmg_cut_loop() plotting %s\n", name);
			if ((fd = fopen(name, "w")) == (FILE *)NULL) {
				(void)perror(name);
				exit(-1);
			}

			nmg_pl_fu(fd, oldlu->up.fu_p, tab, 100, 100, 100);
			nmg_pl_fu(fd, oldlu->up.fu_p->fumate_p, tab, 100, 100, 100);
			(void)fclose(fd);
			rt_free( (char *)tab, "nmg_cut_loop flag[] 1" );
		}
	}

	/* make a new loop structure for the new loop & throw away
	 * the vertexuse we don't need
	 */
	lu = nmg_mlv(oldlu->up.magic_p, (struct vertex *)NULL, OT_UNSPEC );
	oldlu->orientation = oldlu->lumate_p->orientation = OT_UNSPEC;

	nmg_kvu(RT_LIST_FIRST(vertexuse, &lu->down_hd));
	nmg_kvu(RT_LIST_FIRST(vertexuse, &lu->lumate_p->down_hd));
	/* nmg_kvu() does RT_LIST_INIT() on down_hd */
	/* The loopuse is considered invalid until it gets some edges */

	/* move the edges into one of the uses of the new loop */
	for (eu = eu2 ; eu != eu1 ; eu = eunext) {
		eunext = RT_LIST_PNEXT_CIRC(edgeuse, &eu->l);

		RT_LIST_DEQUEUE(&eu->l);
		RT_LIST_INSERT(&lu->down_hd, &eu->l);
		RT_LIST_DEQUEUE(&eu->eumate_p->l);
		RT_LIST_APPEND(&lu->lumate_p->down_hd, &eu->eumate_p->l);
		eu->up.lu_p = lu;
		eu->eumate_p->up.lu_p = lu->lumate_p;
	}

	/* make a wire edge in the shell to "cap off" the new loop */
	neweu = nmg_me(eu1->vu_p->v_p, eu2->vu_p->v_p, nmg_find_s_of_eu(eu1));

	/* move the new edgeuse into the new loopuse */
	RT_LIST_DEQUEUE(&neweu->l);
	RT_LIST_INSERT(&lu->down_hd, &neweu->l);
	neweu->up.lu_p = lu;

	/* move the new edgeuse mate into the new loopuse mate */
	RT_LIST_DEQUEUE(&neweu->eumate_p->l);
	RT_LIST_APPEND(&lu->lumate_p->down_hd, &neweu->eumate_p->l);
	neweu->eumate_p->up.lu_p = lu->lumate_p;

	/* now we go back and close up the loop we just ripped open */
	eunext = nmg_me(eu2->vu_p->v_p, eu1->vu_p->v_p, nmg_find_s_of_eu(eu1));

	RT_LIST_DEQUEUE(&eunext->l);
	RT_LIST_INSERT(&eu1->l, &eunext->l);
	RT_LIST_DEQUEUE(&eunext->eumate_p->l);
	RT_LIST_APPEND(&eu1->eumate_p->l, &eunext->eumate_p->l);
	eunext->up.lu_p = eu1->up.lu_p;
	eunext->eumate_p->up.lu_p = eu1->eumate_p->up.lu_p;

	/* make new edgeuses radial to each other, sharing the new edge */
	nmg_moveeu(neweu, eunext);

	nmg_ck_lueu(oldlu, "oldlu");
	nmg_ck_lueu(lu, "lu");	/*LABLABLAB*/


	if (rt_g.NMG_debug & DEBUG_CUTLOOP && rt_g.NMG_debug & DEBUG_PLOTEM) {
		long		*tab;
		tab = (long *)rt_calloc( m->maxindex, sizeof(long),
			"nmg_cut_loop flag[] 2" );

		(void)sprintf(name, "After_cutloop%d.pl", i);
		rt_log("nmg_cut_loop() plotting %s\n", name);
		if ((fd = fopen(name, "w")) == (FILE *)NULL) {
			(void)perror(name);
			exit(-1);
		}

		nmg_pl_fu(fd, oldlu->up.fu_p, tab, 100, 100, 100);
		nmg_pl_fu(fd, oldlu->up.fu_p->fumate_p, tab, 100, 100, 100);
		(void)fclose(fd);
		rt_free( (char *)tab, "nmg_cut_loop flag[] 2" );
	}
out:
	if (rt_g.NMG_debug & DEBUG_BASIC)  {
		rt_log("nmg_cut_loop(vu1=x%x, vu2=x%x) new lu=x%x\n", vu1, vu2, lu );
	}

	return lu;
}

/*
 *			N M G _ S P L I T _ L U _ A T _ V U
 *
 *  In a loop which has at least two distinct uses of a vertex,
 *  split off the edges from "split_vu" to the second occurance of
 *  the vertex into a new loop.
 *  It is the caller's responsibility to re-bound the loops.
 *
 *  The old and new loopuses will have orientation OT_UNSPEC.  It is the
 *  callers responsibility to determine what their orientations should be.
 *  This can be conveniently done with nmg_lu_reorient().
 *
 *  Here is an example:
 *
 *	              E<__________B <___________A
 *	              |           ^\            ^
 *	              |          /  \           |
 *	              |         /    \          |
 *	              |        /      v         |
 *	              |       D<_______C        |
 *	              v                         |
 *	              F________________________>G
 *	
 *  When nmg_split_lu_at_vu(lu, B) is called, the old loopuse continues to
 *  be counter clockwise and OT_SAME, but the new loopuse BCD is now
 *  clockwise.  It needs to be marked OT_OPPOSITE.  Without referring
 *  to the geometry, this can't be determined.
 *
 *  Intended primarily for use by nmg_split_touchingloops().
 *
 *  Returns -
 *	NULL	Error
 *	lu	Loopuse of new loop, on success.
 */
struct loopuse *
nmg_split_lu_at_vu( lu, split_vu )
struct loopuse		*lu;
struct vertexuse	*split_vu;
{
	struct edgeuse		*eu;
	struct vertexuse	*vu;
	struct loopuse		*newlu;
	struct loopuse		*newlumate;
	struct vertex		*split_v;
	int			iteration;

	split_v = split_vu->v_p;
	NMG_CK_VERTEX(split_v);

	eu = split_vu->up.eu_p;
	NMG_CK_EDGEUSE(eu);

	if( eu->up.lu_p != lu )  {
		/* Could not find indicated vertex */
		newlu = (struct loopuse *)0;		/* FAIL */
		goto out;
	}

	/* Make a new loop in the same face */
	lu->orientation = lu->lumate_p->orientation = OT_UNSPEC;
	newlu = nmg_mlv( lu->up.magic_p, (struct vertex *)NULL, OT_UNSPEC );
	NMG_CK_LOOPUSE(newlu);
	newlumate = newlu->lumate_p;
	NMG_CK_LOOPUSE(newlumate);

	/* Throw away unneeded lone vertexuse */
	nmg_kvu(RT_LIST_FIRST(vertexuse, &newlu->down_hd));
	nmg_kvu(RT_LIST_FIRST(vertexuse, &newlumate->down_hd));
	/* nmg_kvu() does RT_LIST_INIT() on down_hd */

	/* Move edges & mates into new loop until vertex is repeated */
	for( iteration=0; iteration < 10000; iteration++ )  {
		struct edgeuse	*eunext;
		eunext = RT_LIST_PNEXT_CIRC(edgeuse, &eu->l);

		RT_LIST_DEQUEUE(&eu->l);
		RT_LIST_INSERT(&newlu->down_hd, &eu->l);
		RT_LIST_DEQUEUE(&eu->eumate_p->l);
		RT_LIST_APPEND(&newlumate->down_hd, &eu->eumate_p->l);

		/* Change edgeuse & mate up pointers */
		eu->up.lu_p = newlu;
		eu->eumate_p->up.lu_p = newlumate;

		/* Advance to next edgeuse */
		eu = eunext;

		/* When split_v is encountered, stop */
		vu = eu->vu_p;
		NMG_CK_VERTEXUSE(vu);
		if( vu->v_p == split_v )  break;
	}
	if( iteration >= 10000 )  rt_bomb("nmg_split_lu_at_vu:  infinite loop\n");
out:
	if (rt_g.NMG_debug & DEBUG_BASIC)  {
		rt_log("nmg_split_lu_at_vu( lu=x%x, split_vu=x%x ) newlu=x%x\n",
			lu, split_vu, newlu );
	}
	return newlu;
}

/*
 *			N M G _ S P L I T _ T O U C H I N G L O O P S
 *
 *  Search through all the vertices in a loop.
 *  Whenever there are two distinct uses of one vertex in the loop,
 *  split off all the edges between them into a new loop.
 *
 *  Note that the call to nmg_split_lu_at_vu() will cause the split
 *  loopuses to be marked OT_UNSPEC.
 */
void
nmg_split_touchingloops( lu, tol )
struct loopuse		*lu;
CONST struct rt_tol	*tol;
{
	struct edgeuse		*eu;
	struct vertexuse	*vu;
	struct vertex		*v;

	NMG_CK_LOOPUSE(lu);
	RT_CK_TOL(tol);
	if (rt_g.NMG_debug & DEBUG_BASIC)  {
		rt_log("nmg_split_touchingloops( lu=x%x )\n", lu);
	}
top:
	if( RT_LIST_FIRST_MAGIC( &lu->down_hd ) != NMG_EDGEUSE_MAGIC )
		return;

	/* For each edgeuse, get vertexuse and vertex */
	for( RT_LIST_FOR( eu, edgeuse, &lu->down_hd ) )  {
		struct vertexuse	*tvu;

		vu = eu->vu_p;
		NMG_CK_VERTEXUSE(vu);
		v = vu->v_p;
		NMG_CK_VERTEX(v);

		/*
		 *  For each vertexuse on vertex list,
		 *  check to see if it points up to the this loop.
		 *  If so, then there is a duplicated vertex.
		 *  Ordinarily, the vertex list will be *very* short,
		 *  so this strategy is likely to be faster than
		 *  a table-based approach, for most cases.
		 */
		for( RT_LIST_FOR( tvu, vertexuse, &v->vu_hd ) )  {
			struct edgeuse		*teu;
			struct loopuse		*tlu;
			struct loopuse		*newlu;

			if( tvu == vu )  continue;
			if( *tvu->up.magic_p != NMG_EDGEUSE_MAGIC )  continue;
			teu = tvu->up.eu_p;
			NMG_CK_EDGEUSE(teu);
			if( *teu->up.magic_p != NMG_LOOPUSE_MAGIC )  continue;
			tlu = teu->up.lu_p;
			NMG_CK_LOOPUSE(tlu);
			if( tlu != lu )  continue;
			/*
			 *  Repeated vertex exists,
			 *  Split loop into two loops
			 */
			newlu = nmg_split_lu_at_vu( lu, vu );
			NMG_CK_LOOPUSE(newlu);
			NMG_CK_LOOP(newlu->l_p);
			nmg_loop_g(newlu->l_p, tol);

			/* Ensure there are no duplications in new loop */
			nmg_split_touchingloops(newlu, tol);

			/* There is no telling where we will be in the
			 * remainder of original loop, check 'em all.
			 */
			goto top;
		}
	}
}

/*
 *			N M G _ J O I N _ T O U C H I N G L O O P S
 *
 *  Search through all the vertices in a loopuse that belongs to a faceuse.
 *  Whenever another loopuse in the same faceuse refers to one of this
 *  loop's vertices, the two loops touch at (at least) that vertex.
 *  Join them together.
 */
int
nmg_join_touchingloops( lu )
struct loopuse	*lu;
{
	struct faceuse		*fu;
	struct edgeuse		*eu;
	struct vertexuse	*vu;
	struct vertex		*v;
	int			count = 0;

	if (rt_g.NMG_debug & DEBUG_BASIC)  {
		rt_log("nmg_join_touchingloops( lu=x%x )\n", lu);
	}
	NMG_CK_LOOPUSE(lu);
	fu = lu->up.fu_p;
	NMG_CK_FACEUSE(fu);

top:
	if( RT_LIST_FIRST_MAGIC( &lu->down_hd ) != NMG_EDGEUSE_MAGIC )
		return count;

	/* For each edgeuse, get vertexuse and vertex */
	for( RT_LIST_FOR( eu, edgeuse, &lu->down_hd ) )  {
		struct vertexuse	*tvu;

		vu = eu->vu_p;
		NMG_CK_VERTEXUSE(vu);
		v = vu->v_p;
		NMG_CK_VERTEX(v);

		/*
		 *  For each vertexuse on vertex list,
		 *  check to see if it points up to an edgeuse in a
		 *  different loopuse in the same faceuse.
		 *  If so, we touch.
		 */
		for( RT_LIST_FOR( tvu, vertexuse, &v->vu_hd ) )  {
			struct edgeuse		*teu;
			struct loopuse		*tlu;
			struct loopuse		*newlu;

			if( tvu == vu )  continue;
			if( *tvu->up.magic_p != NMG_EDGEUSE_MAGIC )  continue;
			teu = tvu->up.eu_p;
			NMG_CK_EDGEUSE(teu);
			if( *teu->up.magic_p != NMG_LOOPUSE_MAGIC )  continue;
			tlu = teu->up.lu_p;
			NMG_CK_LOOPUSE(tlu);
			if( tlu == lu )  {
				/* We touch ourselves at another vu? */
				rt_log("INFO: nmg_join_touchingloops() lu=x%x touches itself at vu1=x%x, vu2=x%x, skipping\n",
					lu, vu, tvu );
				continue;
			}
			if( *tlu->up.magic_p != NMG_FACEUSE_MAGIC )  continue;
			if( tlu->up.fu_p != fu )  continue;	/* wrong faceuse */
			/*
			 *  Loop 'lu' touches loop 'tlu' at this vertex,
			 *  join them.
			 *  XXX Is there any advantage to searching for
			 *  XXX a potential shared edge at this point?
			 *  XXX Call nmg_simplify_loop()?
			 */
rt_log("nmg_join_touchingloops(): lu=x%x, vu=x%x, tvu=x%x\n", lu, vu, tvu);
			tvu = nmg_join_2loops( vu, tvu );
			NMG_CK_VERTEXUSE(tvu);
			count++;
			goto top;
		}
	}
	return count;
}

/*			N M G _ S I M P L I F Y _ L O O P
 *
 *	combine adjacent loops within the same parent that touch along
 *	a common edge into a single loop, with the edge eliminated.
 */
void
nmg_simplify_loop(lu)
struct loopuse *lu;
{
	struct edgeuse *eu, *eu_r, *tmpeu;

	NMG_CK_LOOPUSE(lu);
	if (rt_g.NMG_debug & DEBUG_BASIC)  {
		rt_log("nmg_simplify_loop(lu=x%x)\n", lu);
	}

	if (RT_LIST_FIRST_MAGIC(&lu->down_hd) != NMG_EDGEUSE_MAGIC)
		return;

	eu = RT_LIST_FIRST(edgeuse, &lu->down_hd);
	while (RT_LIST_NOT_HEAD(eu, &lu->down_hd) ) {

		NMG_CK_EDGEUSE(eu);

		eu_r = eu->radial_p;
		NMG_CK_EDGEUSE(eu_r);

		/* if the radial edge is part of a loop, and the loop of
		 * the other edge is a part of the same object (face)
		 * as the loop containing the current edge, and my
		 * edgeuse mate is radial to my radial`s edgeuse
		 * mate, and the radial edge is a part of a loop other
		 * than the one "eu" is a part of 
		 * then this is a "worthless" edge between these two loops.
		 */
		if (*eu_r->up.magic_p == NMG_LOOPUSE_MAGIC &&
		    eu_r->up.lu_p->up.magic_p == lu->up.magic_p &&
		    eu->eumate_p->radial_p == eu->radial_p->eumate_p &&
		    eu_r->up.lu_p != lu) {

		    	if( eu_r->up.lu_p->orientation != lu->orientation &&
		    	   (lu->orientation != OT_SAME ||
			    eu_r->up.lu_p->orientation != OT_OPPOSITE) )  {
				/* Does not meet requirements of nmg_jl(),
				 * skip it.
				 */
			    	eu = RT_LIST_PNEXT(edgeuse, eu);
				continue;
			}

		    	/* save a pointer to where we've already been
		    	 * so that when eu becomes an invalid pointer, we
		    	 * still know where to pick up from.
		    	 */
		    	tmpeu = RT_LIST_PLAST(edgeuse, eu);

			nmg_jl(lu, eu);

		    	/* Since all the new edges will have been appended
		    	 * after tmpeu, we can pick up processing with the
		    	 * edgeuse immediately after tmpeu
		    	 */
		    	eu = tmpeu;

		    	if (rt_g.NMG_debug &(DEBUG_PLOTEM|DEBUG_PL_ANIM) &&
			    *lu->up.magic_p == NMG_FACEUSE_MAGIC ) {
		    	    	static int fno=0;

				nmg_pl_2fu("After_joinloop%d.pl", fno++,
				    lu->up.fu_p, lu->up.fu_p->fumate_p, 0);
					
		    	}
		}
		eu = RT_LIST_PNEXT(edgeuse, eu);
	}
}


/*
 *			N M G _ K I L L _ S N A K E S
 *
 *  Removes "snake" or "disconnected crack" edges from loopuse.
 *
 *  Returns -
 *	0	If all went well
 *	1	If the loopuse is now empty and needs to be killed.
 */
int
nmg_kill_snakes(lu)
struct loopuse *lu;
{
	struct edgeuse *eu, *eu_r;
	struct vertexuse *vu;
	struct vertex *v;

	NMG_CK_LOOPUSE(lu);
	if (rt_g.NMG_debug & DEBUG_BASIC)  {
		rt_log("nmg_kill_snakes(lu=x%x)\n", lu);
	}
	if (RT_LIST_FIRST_MAGIC(&lu->down_hd) != NMG_EDGEUSE_MAGIC)
		return 0;

	eu = RT_LIST_FIRST(edgeuse, &lu->down_hd);
	while (RT_LIST_NOT_HEAD(eu, &lu->down_hd) ) {

		NMG_CK_EDGEUSE(eu);

		eu_r = eu->radial_p;
		NMG_CK_EDGEUSE(eu_r);

		/* if the radial edge is a part of the same loop, and
		 * this edge is not used by anyplace else, and the radial edge
		 * is also the next edge, this MAY be the tail of a snake!
		 */

		if (*eu_r->up.magic_p == NMG_LOOPUSE_MAGIC &&
		    eu_r->up.lu_p == eu->up.lu_p &&
		    eu->eumate_p->radial_p == eu->radial_p->eumate_p &&
		    RT_LIST_PNEXT_CIRC(edgeuse, eu) == eu_r) {

		    	/* if there are no other uses of the vertex
		    	 * between these two edgeuses, then this is
		    	 * indeed the tail of a snake
		    	 */
			v = eu->eumate_p->vu_p->v_p;
			vu = RT_LIST_FIRST(vertexuse, &v->vu_hd);
			while (RT_LIST_NOT_HEAD(vu, &v->vu_hd) &&
			      (vu->up.eu_p == eu->eumate_p ||
			       vu->up.eu_p == eu_r) )
				vu = RT_LIST_PNEXT(vertexuse, vu);

			if (! RT_LIST_NOT_HEAD(vu, &v->vu_hd) ) {
				/* this is the tail of a snake! */
				(void)nmg_keu(eu_r);
				if( nmg_keu(eu) )  return 1; /* kill lu */
				if( RT_LIST_IS_EMPTY( &lu->down_hd ) )
					return 1;	/* loopuse is empty */
				eu = RT_LIST_FIRST(edgeuse, &lu->down_hd);

			    	if (rt_g.NMG_debug &(DEBUG_PLOTEM|DEBUG_PL_ANIM) &&
				    *lu->up.magic_p == NMG_FACEUSE_MAGIC ) {
			    	    	static int fno=0;

					nmg_pl_2fu("After_joinloop%d.pl", fno++,
					    lu->up.fu_p, lu->up.fu_p->fumate_p, 0);

			    	}


			} else
				eu = RT_LIST_PNEXT(edgeuse, eu);
		} else
			eu = RT_LIST_PNEXT(edgeuse, eu);
	}
	return 0;	/* All is well, loop still has edges */
}

/*
 *			N M G _ M V _ L U _ B E T W E E N _ S H E L L S
 *
 *  Move a wire-loopuse from one shell to another.
 *  Note that this routine can not be used on loops in faces.
 */
void
nmg_mv_lu_between_shells( dest, src, lu )
struct shell		*dest;
register struct shell	*src;
register struct loopuse	*lu;
{
	register struct loopuse	*lumate;

	NMG_CK_LOOPUSE(lu);
	lumate = lu->lumate_p;
	NMG_CK_LOOPUSE(lumate);

	if( lu->up.s_p != src )  {
		rt_log("nmg_mv_lu_between_shells(dest=x%x, src=x%x, lu=x%x), lu->up.s_p=x%x isn't source shell\n",
			dest, src, lu, lu->up.s_p );
		rt_bomb("lu->up.s_p isn't source shell\n");
	}
	if( lumate->up.s_p != src )  {
		rt_log("nmg_mv_lu_between_shells(dest=x%x, src=x%x, lu=x%x), lumate->up.s_p=x%x isn't source shell\n",
			dest, src, lu, lumate->up.s_p );
		rt_bomb("lumate->up.s_p isn't source shell\n");
	}

	/* Remove lu from src shell */
	RT_LIST_DEQUEUE( &lu->l );
	if( RT_LIST_IS_EMPTY( &src->lu_hd ) )  {
		/* This was the last lu in the list */
		rt_log("nmg_mv_lu_between_shells(dest=x%x, src=x%x, lu=x%x), lumate=x%x not in src shell\n",
			dest, src, lu, lumate );
		rt_bomb("src shell emptied before finding lumate\n");
	}

	/* Remove lumate from src shell */
	RT_LIST_DEQUEUE( &lumate->l );

	/* Add lu and lumate to dest shell */
	RT_LIST_APPEND( &dest->lu_hd, &lu->l );
	RT_LIST_APPEND( &lu->l, &lumate->l );

	lu->up.s_p = dest;
	lumate->up.s_p = dest;

	if (rt_g.NMG_debug & DEBUG_BASIC)  {
		rt_log("nmg_mv_lu_between_shells(dest=x%x, src=x%x, lu=x%x)\n",
			dest , src , lu);
	}
}

/*
 *			N M G _ M O V E L T O F
 *
 *	move first pair of shell wire loopuses out to become a genuine loop
 *	in an existing face.
 * XXX This routine is not used anywhere, and may not work.
 */
void nmg_moveltof(fu, s)
struct faceuse *fu;
struct shell *s;
{
	struct loopuse	*lu1, *lu2;

	NMG_CK_SHELL(s);
	NMG_CK_FACEUSE(fu);
	if (fu->s_p != s) {
		rt_log("nmg_moveltof() in %s at %d. Cannot move loop to face in another shell\n",
		    __FILE__, __LINE__);
	}
	lu1 = RT_LIST_FIRST(loopuse, &s->lu_hd);
	NMG_CK_LOOPUSE(lu1);
	RT_LIST_DEQUEUE( &lu1->l );

	lu2 = RT_LIST_FIRST(loopuse, &s->lu_hd);
	NMG_CK_LOOPUSE(lu2);
	RT_LIST_DEQUEUE( &lu2->l );

	RT_LIST_APPEND( &fu->lu_hd, &lu1->l );
	RT_LIST_APPEND( &fu->fumate_p->lu_hd, &lu2->l );
	/* XXX What about loopuse "up" pointers? */

	if (rt_g.NMG_debug & DEBUG_BASIC)  {
		rt_log("nmg_moveltof(fu=x%x, s=x%x)\n",
			fu , s );
	}
}

/*
 *			N M G _ D U P _ L O O P
 *
 *  A support routine for nmg_dup_face()
 */
struct loopuse *
nmg_dup_loop(lu, parent, trans_tbl)
struct loopuse *lu;
long	*parent;		/* fu or shell ptr */
long	**trans_tbl;
{
	struct loopuse *new_lu = (struct loopuse *)NULL;
	struct vertexuse *new_vu = (struct vertexuse *)NULL;
	struct vertexuse *old_vu = (struct vertexuse *)NULL;
	struct vertex *old_v = (struct vertex *)NULL;
	struct vertex	*new_v;
	struct edgeuse *new_eu = (struct edgeuse *)NULL;
	struct edgeuse *tbl_eu = (struct edgeuse *)NULL;
	struct edgeuse *eu = (struct edgeuse *)NULL;
	int i=1;

	NMG_CK_LOOPUSE(lu);

	/* if loop is just a vertex, that's simple to duplicate */
	if (RT_LIST_FIRST_MAGIC(&lu->down_hd) == NMG_VERTEXUSE_MAGIC) {
		old_vu = RT_LIST_FIRST(vertexuse, &lu->down_hd);
		old_v = old_vu->v_p;

		/* Obtain new duplicate of old vertex.  May be null 1st time. */
		new_v = NMG_INDEX_GETP(vertex, trans_tbl, old_v);
		new_lu = nmg_mlv(parent, new_v, lu->orientation);
		if( new_v )  {
			/* the new vertex already exists in the new model */
			rt_log("nmg_dup_loop() existing vertex in new model\n");
			return (struct loopuse *)NULL;
		}
		/* nmg_mlv made a new vertex */
		rt_log("nmg_dup_loop() new vertex in new model\n");

		new_vu = RT_LIST_FIRST(vertexuse, &new_lu->down_hd);
		new_v = new_vu->v_p;
		/* Give old_v entry a pointer to new_v */
		NMG_INDEX_ASSIGN( trans_tbl, old_v, (long *)new_v );
		if (old_v->vg_p) {
			/* Build a different vertex_g with same coordinates */
			nmg_vertex_gv(new_v, old_v->vg_p->coord);
		}
		if (rt_g.NMG_debug & DEBUG_BASIC)  {
			rt_log("nmg_dup_loop(lu=x%x, parent=x%x, trans_tbl=x%x) new_lu=x%x\n",
				lu , parent , trans_tbl , new_lu );
		}
		return new_lu;
	}

	/* This loop is an edge-loop.  This is a little more work
	 * First order of business is to duplicate the vertex/edge makeup.
	 */
	for (RT_LIST_FOR(eu, edgeuse, &lu->down_hd)) {

		NMG_CK_EDGEUSE(eu);
		NMG_CK_VERTEXUSE(eu->vu_p);
		NMG_CK_VERTEX(eu->vu_p->v_p);
		old_v = eu->vu_p->v_p;

		/* Obtain new duplicate of old vertex.  May be null 1st time. */
		new_v = NMG_INDEX_GETP(vertex, trans_tbl, old_v);
		if (new_lu == (struct loopuse *)NULL) {
			/* this is the first edge in the new loop */
			new_lu = nmg_mlv(parent, new_v, lu->orientation);
			new_vu = RT_LIST_FIRST(vertexuse, &new_lu->down_hd);

			NMG_CK_VERTEXUSE(new_vu);
			NMG_CK_VERTEX(new_vu->v_p);

			if( !new_v )  {
				/* Give old_v entry a pointer to new_v */
				NMG_INDEX_ASSIGN( trans_tbl, old_v,
					(long *)new_vu->v_p );
			}
			new_v = new_vu->v_p;

			new_eu = nmg_meonvu(new_vu);
		} else {
			/* not the first edge in new loop */
			new_eu = RT_LIST_LAST(edgeuse, &new_lu->down_hd);
			NMG_CK_EDGEUSE(new_eu);

			new_eu = nmg_eusplit(new_v, new_eu);
			new_vu = new_eu->vu_p;

			if( !new_v )  {
				/* Give old_v entry a pointer to new_v */
				NMG_INDEX_ASSIGN( trans_tbl, old_v,
					(long *)new_vu->v_p );
			}
			new_v = new_vu->v_p;
		}
		/* Build a different vertex_g with same coordinates */
		if (old_v->vg_p) {
			NMG_CK_VERTEX_G(old_v->vg_p);
			nmg_vertex_gv(new_v, old_v->vg_p->coord);
		}

		/* Prepare to glue edges */
		/* Use old_e as subscript, to get 1st new_eu (for new_e) */
		/* Use old_eu to get mapped new_eu */
		tbl_eu = NMG_INDEX_GETP(edgeuse, trans_tbl, eu->e_p );
		if( !tbl_eu )  {
			/* Establishes map from old edge to new edge(+use) */
			NMG_INDEX_ASSIGN( trans_tbl, eu->e_p, (long *)new_eu );
		}
		NMG_INDEX_ASSIGN( trans_tbl, eu, (long *)new_eu );
	}

#if 0
/* XXX untested */
	/* All vertex structs are shared.  Make shared edges be shared */
	for(RT_LIST_FOR(eu, edgeuse, &lu->down_hd)) {
		/* Use old_e as subscript, to get 1st new_eu (for new_e) */
		/* Use old_eu to get mapped new_eu */
		tbl_eu = NMG_INDEX_GETP(edgeuse, trans_tbl, eu->e_p );
		new_eu = NMG_INDEX_GETP(edgeuse, trans_tbl, eu );
		if( tbl_eu->e_p == new_eu->e_p )  continue;

		/* new_eu isn't sharing edge with tbl_eu, join them */
		/* XXX Is radial relationship preserved (enough)? */
		nmg_moveeu( tbl_eu, new_eu );
	}
#endif

	/* Now that we've got all the right topology created and the
	 * vertex geometries are in place we can create the edge geometries.
	 * XXX This ought to be optional, as most callers will immediately
	 * XXX change the vertex geometry anyway (e.g. by extrusion dist).
	 */
	for(RT_LIST_FOR(eu, edgeuse, &lu->down_hd)) {
		new_eu = NMG_INDEX_GETP(edgeuse, trans_tbl, eu );
		if( new_eu->e_p->eg_p )  continue;
		nmg_edge_g(new_eu->e_p);
	}

	if (rt_g.NMG_debug & DEBUG_BASIC)  {
		rt_log("nmg_dup_loop(lu=x%x, parent=x%x, trans_tbl=x%x) new_lu=x%x\n",
			lu , parent , trans_tbl , new_lu );
	}
	return (new_lu);
}

/*
 *			N M G _ S E T _ L U _ O R I E N T A T I O N
 *
 *  Set this loopuse and mate's orientation to be SAME or OPPOSITE
 *  from the orientation of the faceuse they each reside in.
 */
void
nmg_set_lu_orientation( lu, is_opposite )
struct loopuse	*lu;
int		is_opposite;
{
	NMG_CK_LOOPUSE(lu);
	NMG_CK_LOOPUSE(lu->lumate_p);
	if (rt_g.NMG_debug & DEBUG_BASIC)  {
		rt_log("nmg_set_lu_orientation(lu=x%x, %s)\n",
			lu, is_opposite?"OT_OPPOSITE":"OT_SAME");
	}
	if( is_opposite )  {
		/* Interior (crack) loop */
		lu->orientation = OT_OPPOSITE;
		lu->lumate_p->orientation = OT_OPPOSITE;
	} else {
		/* Exterior loop */
		lu->orientation = OT_SAME;
		lu->lumate_p->orientation = OT_SAME;
	}
}

/*
 *			N M G _ L U _ R E O R I E N T
 *
 *  Based upon a geometric calculation, reorient a loop and it's mate,
 *  if the stored orientation differs from the geometric one.
 */
void
nmg_lu_reorient( lu, tol )
struct loopuse		*lu;
CONST struct rt_tol	*tol;
{
	struct faceuse	*fu;
	int	ccw;
	int	geom_orient;
	plane_t	norm;

	NMG_CK_LOOPUSE(lu);
	RT_CK_TOL(tol);
	fu = lu->up.fu_p;
	NMG_CK_FACEUSE(fu);
	if( fu->orientation != OT_SAME )  fu = fu->fumate_p;

	if (rt_g.NMG_debug & DEBUG_BASIC)  {
		rt_log("nmg_lu_reorient(lu=x%x, tol)\n", lu);
	}

	/* Get OT_SAME faceuse's normal */
	NMG_GET_FU_PLANE( norm, fu );

	ccw = nmg_loop_is_ccw( lu, norm, tol );
	if( ccw == 0 )  {
		int	class;
		/* Loop does not have 3 linearly independent vertices, can't tell. */
		if (rt_g.NMG_debug & DEBUG_BASIC)
			rt_log("nmg_lu_reorient:  unable to determine orientation from geometry\n");
		class = nmg_class_lu_fu( lu, tol );
		switch( class )  {
		case NMG_CLASS_AinB:
			/* An interior "hole crack" */
			geom_orient = OT_OPPOSITE;
			break;
		case NMG_CLASS_AoutB:
			/* An exterior "solid crack" */
			geom_orient = OT_SAME;
			break;
		case NMG_CLASS_AonBshared:
			/* ALL vu's touch other loops in face, should have been joined. */
			rt_bomb("nmg_lu_reorient() lu is ON another lu, should have been joined\n");
		default:
			rt_bomb("nmg_lu_reorient() bad class from nmg_class_lu_fu()\n");
		}
rt_log("nmg_lu_reorient() class=%s, orient=%s\n", nmg_class_name(class), nmg_orientation(geom_orient) );
	} else {
		if( ccw > 0 )  {
			geom_orient = OT_SAME;	/* same as face (OT_SAME faceuse) */
		} else {
			geom_orient = OT_OPPOSITE;
		}
	}

	if( lu->orientation == geom_orient )  return;
	if( rt_g.NMG_debug & DEBUG_BASIC )  {
		rt_log("nmg_lu_reorient(x%x):  changing orientation: %s to %s\n",
			lu, nmg_orientation(lu->orientation),
			nmg_orientation(geom_orient) );
	}

	lu->orientation = geom_orient;
	lu->lumate_p->orientation = geom_orient;
}

/************************************************************************
 *									*
 *				EDGE Routines				*
 *									*
 ************************************************************************/

/*
 *			N M G _ E U S P L I T
 *
 *	Split an edgeuse by inserting a vertex into middle of the edgeuse.
 *
 *	Make a new edge, and a vertex.  If v is non-null it is taken as a
 *	pointer to an existing vertex to use as the start of the new edge.
 *	If v is null, then a new vertex is created for the begining of the
 *	new edge.
 *
 *	In either case, the new edge will exist as the "next" edgeuse after
 *	the edgeuse passed as a parameter.
 *
 *  Explicit return -
 *	edgeuse of new edge, starting at v.
 *
 *  List on entry -
 *
 *		       oldeu
 *		  .------------->
 *		 /
 *		A =============== B (edge)
 *				 /
 *		  <-------------.
 *		      oldeumate
 *
 *  List on return -
 *
 *		     oldeu(cw)    eu1
 *		    .------->   .----->
 *		   /           /
 *	   (edge) A ========= V ~~~~~~~ B (new edge)
 *			     /         /
 *		    <-------.   <-----.
 *		       mate	 mate
 */
struct edgeuse *
nmg_eusplit(v, oldeu)
struct vertex *v;
struct edgeuse *oldeu;
{
	struct edgeuse	*eu1,
			*eu2,
			*oldeumate;
	struct shell *s;
	struct loopuse	*lu;

	NMG_CK_EDGEUSE(oldeu);
	if (v) {
		NMG_CK_VERTEX(v);
	}
	oldeumate = oldeu->eumate_p;
	NMG_CK_EDGEUSE( oldeumate );

	/* if this edge has uses other than this edge and its mate, we must
	 * separate these two edgeuses from the existing edge, and create
	 * a new edge for them.  Then we can insert a new vertex in this
	 * new edge without fear of damaging some other object.
	 */
	if (oldeu->radial_p != oldeumate)
		nmg_unglueedge(oldeu);

	if (*oldeu->up.magic_p == NMG_SHELL_MAGIC) {
		s = oldeu->up.s_p;
		NMG_CK_SHELL(s);

		/*
		 *  Make an edge from the new vertex ("V") to vertex at
		 *  other end of the edge given ("B").
		 *  The new vertex "V" may be NULL, which will cause the
		 *  shell's lone vertex to be used, or a new one obtained.
		 *  New edges will be placed at head of shell's edge list.
		 */
		eu1 = nmg_me(v, oldeumate->vu_p->v_p, s);
		eu2 = eu1->eumate_p;

		/*
		 *  The situation is now:
		 *
		 *      eu1			       oldeu
		 *  .----------->		  .------------->
		 * /				 /
		 *V ~~~~~~~~~~~~~ B (new edge)	A =============== B (edge)
		 *		 /				 /
		 *  <-----------.		  <-------------.
		 *      eu2			      oldeumate
		 */

		/* Make oldeumate start at "V", not "B" */
		nmg_movevu(oldeumate->vu_p, eu1->vu_p->v_p);

		/*
		 *  Enforce rigid ordering in shell's edge list:
		 *	oldeu, oldeumate, eu1, eu2
		 *  This is to keep edges & mates "close to each other".
		 */
		if( RT_LIST_PNEXT(edgeuse, oldeu) != oldeumate )  {
			RT_LIST_DEQUEUE( &oldeumate->l );
			RT_LIST_APPEND( &oldeu->l, &oldeumate->l );
		}
		RT_LIST_DEQUEUE( &eu1->l );
		RT_LIST_DEQUEUE( &eu2->l );
		RT_LIST_APPEND( &oldeumate->l, &eu1->l );
		RT_LIST_APPEND( &eu1->l, &eu2->l );

		/*
		 *	     oldeu(cw)    eu1
		 *	    .------->   .----->
		 *	   /           /
		 * (edge) A ========= V ~~~~~~~ B (new edge)
		 *		     /         /
		 *	    <-------.   <-----.
		 *	    oldeumate     eu2
		 */
		goto out;
	}
	else if (*oldeu->up.magic_p != NMG_LOOPUSE_MAGIC) {
		rt_log("nmg_eusplit() in %s at %d invalid edgeuse parent\n",
			__FILE__, __LINE__);
		rt_bomb("nmg_eusplit\n");
	}

	/* now we know we are in a loop */

	lu = oldeu->up.lu_p;
	NMG_CK_LOOPUSE(lu);

	/* get a parent shell pointer so we can make a new edge */
	if (*lu->up.magic_p == NMG_SHELL_MAGIC)
		s = lu->up.s_p;
	else if (*lu->up.magic_p == NMG_FACEUSE_MAGIC)
		s = lu->up.fu_p->s_p;
	else
		rt_bomb("nmg_eusplit() bad lu->up\n");
	NMG_CK_SHELL(s);

	/* Make a new wire edge in the shell */
	if (v) {
		/* An edge on the single vertex "V" */
		eu1 = nmg_me(v, v, s);
		eu2 = eu1->eumate_p;
	} else {
		/* Form a wire edge between two new vertices */
		eu1 = nmg_me((struct vertex *)NULL, (struct vertex *)NULL, s);
		eu2 = eu1->eumate_p;
		/* Make both ends of edge use same vertex.
		 * The second vertex is freed automaticly.
		 */
		nmg_movevu(eu2->vu_p, eu1->vu_p->v_p);
	}

	/*
	 *  The current situation is now:
	 *
	 *	      eu1				       oldeu
	 *	  .------------->			  .------------->
	 *	 /					 /
	 *	V ~~~~~~~~~~~~~~~ V (new edge)		A =============== B (edge)
	 *			 /					 /
	 *	  <-------------.			  <-------------.
	 *	      eu2				      oldeumate
	 *
	 *  Goals:
	 *  eu1 will become the mate to oldeumate on the existing edge.
	 *  eu2 will become the mate of oldeu on the new edge.
	 */
	RT_LIST_DEQUEUE( &eu1->l );
	RT_LIST_DEQUEUE( &eu2->l );
	RT_LIST_APPEND( &oldeu->l, &eu1->l );
	RT_LIST_APPEND( &oldeumate->l, &eu2->l );

	/*
	 *  The situation is now:
	 *
	 *		       oldeu      eu1			>>>loop>>>
	 *		    .------->   .----->
	 *		   /           /
	 *	   (edge) A ========= V ~~~~~~~ B (new edge)
	 *			     /         /
	 *		    <-------.   <-----.	
	 *		       eu2      oldeumate		<<<loop<<<
	 */

	/* Copy parentage (loop affiliation) and orientation */
	eu1->up.magic_p = oldeu->up.magic_p;
	eu1->orientation = oldeu->orientation;
	eu1->eua_p = (struct edgeuse_a *)NULL;

	eu2->up.magic_p = oldeumate->up.magic_p;
	eu2->orientation = oldeumate->orientation;
	eu2->eua_p = (struct edgeuse_a *)NULL;

	/* Build mate relationship */
	eu1->eumate_p = oldeumate;
	oldeumate->eumate_p = eu1;
	eu2->eumate_p = oldeu;
	oldeu->eumate_p = eu2;

	/*  Build radial relationship.
	 *  Simple only because this edge has no other uses.
	 */
	eu1->radial_p = oldeumate;
	oldeumate->radial_p = eu1;
	eu2->radial_p = oldeu;
	oldeu->radial_p = eu2;

	/* Associate oldeumate with new edge, and eu2 with old edge. */
	oldeumate->e_p = eu1->e_p;
	eu2->e_p = oldeu->e_p;

out:
	if (rt_g.NMG_debug & DEBUG_BASIC)  {
		rt_log("nmg_eusplit(v=x%x, eu=x%x) new_eu=x%x, mate=x%x\n",
			v, oldeu, eu1, eu1->eumate_p );
	}
	return(eu1);
}

/*
 *			N M G _ E S P L I T
 *
 *	Split an edge by inserting a vertex into middle of *all* of the
 *	uses of this edge, and combine the new edgeuses together onto the
 *	new edge.
 *	A more powerful version of nmg_eusplit(), which does only one use.
 *
 *	Makes a new edge, and a vertex.  If v is non-null it is taken as a
 *	pointer to an existing vertex to use as the start of the new edge.
 *	If v is null, then a new vertex is created for the begining of the
 *	new edge.
 *
 *	In either case, the new edgeuse will exist as the "next" edgeuse
 *	after the edgeuse passed as a parameter.
 *
 *	Note that eu->e_p changes value upon return, because the old
 *	edge is incrementally replaced by two new ones.
 *
 *  Explicit return -
 *	Pointer to the edgeuse which starts at the newly created
 *	vertex (V), and runs to B.
 *
 *  Implicit returns -
 *	ret_eu->vu_p->v_p gives the new vertex ("v", if non-null), and
 *	ret_eu->e_p is the new edge that runs from V to B.
 *
 *	The new vertex created will also be eu->eumate_p->vu_p->v_p.
 *
 *  Edge on entry -
 *
 *			eu
 *		  .------------->
 *		 /
 *		A =============== B (edge)
 *				 /
 *		  <-------------.
 *		    eu->eumate_p
 *
 *  Edge on return -
 *
 *			eu	  ret_eu
 *		    .------->   .--------->
 *		   /           /
 *	(newedge) A ========= V ~~~~~~~~~~~ B (new edge)
 *			     /             /
 *		    <-------.   <---------.
 */
struct edgeuse *
nmg_esplit(v, eu)
struct vertex	*v;		/* New vertex, to go in middle */
struct edgeuse	*eu;
{
	struct edge	*e;	/* eu->e_p */
	struct edgeuse	*teuX,	/* radial edgeuse of eu */
			*teuY,	/* new edgeuse (next of teuX) */
			*neu1, *neu2; /* new (split) edgeuses */
	int 		notdone=1;
	struct vertex	*vA, *vB;	/* start and end of eu */

	neu1 = neu2 = (struct edgeuse *)NULL;

	NMG_CK_EDGEUSE(eu);
	e = eu->e_p;
	NMG_CK_EDGE(e);

	NMG_CK_VERTEXUSE(eu->vu_p);
	vA = eu->vu_p->v_p;
	NMG_CK_VERTEX(vA);

	NMG_CK_EDGEUSE(eu->eumate_p);
	NMG_CK_VERTEXUSE(eu->eumate_p->vu_p);
	vB = eu->eumate_p->vu_p->v_p;
	NMG_CK_VERTEX(vB);

	/* vA and vB are the endpoints of the original edge "e" */
	if( vA == vB )  {
		rt_log("WARNING: nmg_esplit() on edge from&to v=x%x\n", vA);
		rt_bomb("nmg_esplit() of edge running from&to same v\n");
	}
	if( v && ( v == vA || v == vB ) )  {
		rt_log("WARNING: nmg_esplit(v=x%x) vertex is already an edge vertex\n", v);
		rt_bomb("nmg_esplit() new vertex is already an edge vertex\n");
	}

	/* one at a time, we peel out & split an edgeuse pair of this edge.
	 * when we split an edge that didn't need to be peeled out, we know
	 * we've split the last edge
	 */
	do {
		/* Peel two temporary edgeuses off the original edge */
		teuX = eu->radial_p;
		/* teuX runs from vA to vB */
		teuY = nmg_eusplit(v, teuX);
		/* Now, teuX runs from vA to v, teuY runs from v to vB */
		NMG_CK_EDGEUSE(teuX);
		NMG_CK_EDGEUSE(teuY);
		NMG_TEST_EDGEUSE(teuX);
		NMG_TEST_EDGEUSE(teuY);
		
		if (!v)  {
			/* If "v" parameter was NULL and this is the
			 * first time through, take note of "v" where
			 * "e" was just split at.
			 */
			v = teuY->vu_p->v_p;
			NMG_CK_VERTEX(v);
		}

		if (teuY->e_p == e || teuX->e_p == e) notdone = 0;
		
		/*  Are the two edgeuses going in same or opposite directions?
		 *  Move the newly created temporary edge (teuX, teuY)
		 *  onto the new edge (neu1, neu2).
		 *  On first pass, just take note of the new edge & edgeuses.
		 */
		NMG_CK_VERTEX(teuX->vu_p->v_p);
		if (teuX->vu_p->v_p == vA) {
			if (neu1) {
				nmg_moveeu(neu1, teuX);
				nmg_moveeu(neu2, teuY);
			}
			neu1 = teuX->eumate_p;
			neu2 = teuY->eumate_p;
		} else if (teuX->vu_p->v_p == vB) {
			if (neu1) {
				nmg_moveeu(neu2, teuX);
				nmg_moveeu(neu1, teuY);
			}
			neu2 = teuX->eumate_p;
			neu1 = teuY->eumate_p;
		} else {
			rt_log("nmg_esplit(v=x%x, e=x%x)\n", v, e);
			rt_log("nmg_esplit: teuX->vu_p->v_p=x%x, vA=x%x, vB=x%x\n", teuX->vu_p->v_p, vA, vB );
			rt_bomb("nmg_esplit() teuX->vu_p->v_p is neither vA nor vB\n");
		}
	} while (notdone);
	/* Here, "e" pointer is invalid -- it no longer exists */

	/* Find an edgeuse that runs from v to vB */
	if( neu2->vu_p->v_p == v && neu2->eumate_p->vu_p->v_p == vB )  {
		if (rt_g.NMG_debug & DEBUG_BASIC)  {
			rt_log("nmg_esplit(v=x%x, eu=x%x) neu2=x%x\n",
				v, eu, neu2);
		}
		return neu2;
	}

	rt_bomb("nmg_esplit() unable to find eu starting at new v\n");
}

/*
 *			N M G _ E B R E A K
 *
 *  Like nmg_esplit(), split an edge into two parts, but where the
 *  two resultant parts share the original edge geometry.
 *  If the original edge had no edge geometry, then none is created here.
 *
 *  The return is the return of nmg_esplit().
 */
struct edgeuse *
nmg_ebreak(v, eu)
struct vertex	*v;
struct edgeuse	*eu;
{
	struct edgeuse	*new_eu;
	struct edge_g	*eg;

	NMG_CK_EDGEUSE(eu);
	eg = eu->e_p->eg_p;

	/* nmg_esplit() will delete eu->e_p, so if geom is present, save it! */
	if( eg )  {
		NMG_CK_EDGE_G(eg);
		eg->usage++;
	}

	new_eu = nmg_esplit(v, eu);	/* Do the hard work */
	NMG_CK_EDGEUSE(eu);
	NMG_CK_EDGEUSE(new_eu);

	if( eu->e_p == new_eu->e_p )  rt_bomb("nmb_ebreak() same edges?\n");

	/* If there wasn't any edge geometry before, nothing more to do */
	if( !eg )
	{
		if (rt_g.NMG_debug & DEBUG_BASIC)  {
			rt_log("nmg_ebreak( v=x%x, eu=x%x ) new_eu=x%x\n", v, eu, new_eu);
		}
		return new_eu;
	}

	/* Make sure the two edges share the same geometry. */
	NMG_CK_EDGE_G(eg);

	/* Both these edges should be fresh, without geometry yet. */
	if( eu->e_p->eg_p )   {
		rt_log("old eg=x%x, new_eg=x%x\n", eg, eu->e_p->eg_p);
		nmg_pr_eg(eg, "old_");
		nmg_pr_eg(eu->e_p->eg_p, "new_");
		rt_bomb("nmg_ebreak() eu grew geometry?\n");
	}
	eu->e_p->eg_p = eg;		/* eg->usage++ was done above */

	/* Sometimes new_eu still has the previous edge geometry on it */
	if( new_eu->e_p->eg_p )  {
		if( new_eu->e_p->eg_p != eg )
			rt_bomb("nmg_ebreak() new_eu grew geometry?\n");
		/* new_eu retained the previous geometry (why?) */
	} else {
		nmg_use_edge_g( new_eu->e_p, eg );
	}

	if (rt_g.NMG_debug & DEBUG_BASIC)  {
		rt_log("nmg_ebreak( v=x%x, eu=x%x ) new_eu=x%x\n", v, eu, new_eu);
	}
	return new_eu;

	/* XXX It would be much nicer to have a list of edges sharing
	 * XXX edge geometry, rather than a count...
	 */
}

/*
 *			N M G _ E 2 B R E A K
 *
 *  Given two edges that are known to intersect someplace other than
 *  at any of their endpoints, break both of them and
 *  insert a shared vertex.
 *  Return a pointer to the new vertex.
 */
struct vertex *
nmg_e2break( eu1, eu2 )
struct edgeuse	*eu1;
struct edgeuse	*eu2;
{
	struct vertex		*v;
	struct edgeuse		*new_eu;

	NMG_CK_EDGEUSE(eu1);
	NMG_CK_EDGEUSE(eu2);

	new_eu = nmg_ebreak( NULL, eu1 );
	v = new_eu->vu_p->v_p;
	NMG_CK_VERTEX(v);
	(void)nmg_ebreak( v, eu2 );

	if (rt_g.NMG_debug & DEBUG_BASIC)  {
		rt_log("nmg_e2break( eu1=x%x, eu2=x%x ) v=x%x\n", eu1, eu2, v);
	}
	return v;
}
/*
 *			N M G _ U N B R E A K _ E D G E
 *
 *  Undoes the effect of an unwanted nmg_ebreak().
 *
 *  Eliminate the vertex between this edgeuse and the next edge,
 *  on all edgeuses radial to this edgeuse's edge.
 *  The edge geometry must be shared, and all uses of the vertex
 *  to be disposed of must originate from this edge pair.
 *
 *		     eu1          eu2
 *		*----------->*----------->*
 *		A.....e1.....B.....e2.....C
 *		*<-----------*<-----------*
 *		    eu1mate      eu2mate
 *
 *  If successful, the vertex B, the edge e2, and all the edgeuses
 *  radial to eu2 (including eu2) will have all been killed.
 *  The radial ordering around e1 will not change.
 *
 *		     eu1
 *		*------------------------>*
 *		A.....e1..................C
 *		*<------------------------*
 *		    eu1mate
 *
 *
 *  No new topology structures are created by this operation.
 *
 *  Returns -
 *	0	OK, edge unbroken
 *	<0	failure, nothing changed
 */
int
nmg_unbreak_edge( eu1_first )
struct edgeuse	*eu1_first;
{
	struct edgeuse	*eu1;
	struct edgeuse	*eu2;
	struct edge	*e1;
	struct edge_g	*eg;
	struct vertexuse *vu;
	struct vertex	*vb = 0;
	struct vertex	*vc;
	struct shell	*s1;
	int		ret = 0;

	NMG_CK_EDGEUSE( eu1_first );
	e1 = eu1_first->e_p;
	NMG_CK_EDGE( e1 );

	eg = e1->eg_p;
	if( !eg )  {
		rt_log( "nmg_unbreak_edge: no geometry for edge1 x%x\n" , e1 );
		ret = -1;
		goto out;
	}
	NMG_CK_EDGE_G(eg);

	/* if the edge geometry doesn't have at least two uses, this
	 * is not a candidate for unbreaking */		
	if( eg->usage < 2 )  {
		ret = -2;
		goto out;
	}

	s1 = nmg_find_s_of_eu(eu1_first);
	NMG_CK_SHELL(s1);

	eu1 = eu1_first;
	eu2 = RT_LIST_PNEXT_CIRC( edgeuse , eu1 );
	if( eu2->e_p->eg_p != eg )  {
		rt_log( "nmg_unbreak_edge: second eu geometry x%x does not match geometry x%x of edge1 x%x\n" ,
			eu2->e_p->eg_p, eg, e1 );
		ret = -3;
		goto out;
	}

	vb = eu2->vu_p->v_p;		/* middle vertex (B) */
	vc = eu2->eumate_p->vu_p->v_p;	/* end vertex (C) */

	/* all uses of this vertex must be for this edge geometry, otherwise
	 * it is not a candidate for deletion */
	for( RT_LIST_FOR( vu , vertexuse , &vb->vu_hd ) )  {
		NMG_CK_VERTEXUSE(vu);
		if( *(vu->up.magic_p) != NMG_EDGEUSE_MAGIC )  {
			/* vertex is referred to by a self-loop */
			if( vu->up.lu_p->orientation == OT_BOOLPLACE )  {
				/* This kind is transient, and safe to ignore */
				continue;
			}
			ret = -4;
			goto out;
		}
		NMG_CK_EDGE(vu->up.eu_p->e_p);
		if( vu->up.eu_p->e_p->eg_p != eg )  {
			ret = -5;
			goto out;
		}
	}

	/* visit all the edgeuse pairs radial to eu1 */
	for(;;)  {
		/* revector eu1mate's start vertex from B to C */
		nmg_movevu( eu1->eumate_p->vu_p , vc );

		/* Now kill off the unnecessary eu2 associated w/ cur eu1 */
		eu2 = RT_LIST_PNEXT_CIRC( edgeuse, eu1 );
		NMG_CK_EDGEUSE(eu2);
		if( eu2->e_p->eg_p != eg )  {
			rt_bomb("nmg_unbreak_edge:  eu2 geometry is wrong\n");
		}
		if( nmg_keu( eu2 ) )
			rt_bomb( "nmg_unbreak_edge: edgeuse parent is now empty!!\n" );

		eu1 = eu1->eumate_p->radial_p;
		if( eu1 == eu1_first )  break;
	}
out:
	if (rt_g.NMG_debug & DEBUG_BASIC)  {
		rt_log("nmg_unbreak_edge(eu=x%x, vb=x%x) ret = %d\n",
			eu1_first, vb, ret);
	}
	return ret;
}

/*
 *			N M G _ E I N S
 *
 *	Insert a new (zero length) edge at the begining of (ie, before)
 *	an existing edgeuse
 *	Perhaps this is what nmg_esplit and nmg_eusplit should have been like?
 *
 *	Before:
 *	.--A--> .--eu-->
 *		 \
 *		  >.
 *		 /
 *	  <-A'--. <-eu'-.
 *
 *
 *	After:
 *
 *               eu1     eu
 *	.--A--> .---> .--eu-->
 *		 \   /
 *		  >.<
 *		 /   \
 *	  <-A'--. <---. <-eu'--.
 *	          eu2     eumate
 */
struct edgeuse *
nmg_eins(eu)
struct edgeuse *eu;
{
	struct edgeuse	*eumate;
	struct edgeuse	*eu1, *eu2;
	struct shell	*s;

	NMG_CK_EDGEUSE(eu);
	eumate = eu->eumate_p;
	NMG_CK_EDGEUSE(eumate);

	if (*eu->up.magic_p == NMG_SHELL_MAGIC) {
		s = eu->up.s_p;
		NMG_CK_SHELL(s);
	}
	else {
		struct loopuse *lu;
		
		lu = eu->up.lu_p;
		NMG_CK_LOOPUSE(lu);
		if (*lu->up.magic_p == NMG_SHELL_MAGIC) {
			s = lu->up.s_p;
			NMG_CK_SHELL(s);
		} else {
			struct faceuse *fu;
			fu = lu->up.fu_p;
			NMG_CK_FACEUSE(fu);
			s = fu->s_p;
			NMG_CK_SHELL(s);
		}
	}

	eu1 = nmg_me(eu->vu_p->v_p, eu->vu_p->v_p, s);
	eu2 = eu1->eumate_p;

	if (*eu->up.magic_p == NMG_LOOPUSE_MAGIC) {
		RT_LIST_DEQUEUE( &eu1->l );
		RT_LIST_DEQUEUE( &eu2->l );

		RT_LIST_INSERT( &eu->l, &eu1->l );
		RT_LIST_APPEND( &eumate->l, &eu2->l );

		eu1->up.lu_p = eu->up.lu_p;
		eu2->up.lu_p = eumate->up.lu_p;
	}
	else {
		rt_bomb("nmg_eins() Cannot yet insert null edge in shell\n");
	}
	if (rt_g.NMG_debug & DEBUG_BASIC)  {
		rt_log("nmg_eins(eu=x%x) eu1=x%x\n", eu, eu1);
	}
	return(eu1);
}

/*
 *			N M G _ M V _ E U _ B E T W E E N _ S H E L L S
 *
 *  Move a wire edgeuse and it's mate from one shell to another.
 */
void
nmg_mv_eu_between_shells( dest, src, eu )
struct shell		*dest;
register struct shell	*src;
register struct edgeuse	*eu;
{
	register struct edgeuse	*eumate;

	NMG_CK_EDGEUSE(eu);
	eumate = eu->eumate_p;
	NMG_CK_EDGEUSE(eumate);

	if (eu->up.s_p != src) {
		rt_log("nmg_mv_eu_between_shells(dest=x%x, src=x%x, eu=x%x), eu->up.s_p=x%x isnt src shell\n",
			dest, src, eu, eu->up.s_p );
		rt_bomb("eu->up.s_p isnt source shell\n");
	}
	if (eumate->up.s_p != src) {
		rt_log("nmg_mv_eu_between_shells(dest=x%x, src=x%x, eu=x%x), eumate->up.s_p=x%x isn't src shell\n",
			dest, src, eu, eumate->up.s_p );
		rt_bomb("eumate->up.s_p isnt source shell\n");
	}

	/* Remove eu from src shell */
	RT_LIST_DEQUEUE( &eu->l );
	if( RT_LIST_IS_EMPTY( &src->eu_hd ) )  {
		/* This was the last eu in the list, bad news */
		rt_log("nmg_mv_eu_between_shells(dest=x%x, src=x%x, eu=x%x), eumate=x%x not in src shell\n",
			dest, src, eu, eumate );
		rt_bomb("src shell emptied before finding eumate\n");
	}

	/* Remove eumate from src shell */
	RT_LIST_DEQUEUE( &eumate->l );

	/* Add eu and eumate to dest shell */
	RT_LIST_APPEND( &dest->eu_hd, &eu->l );
	RT_LIST_APPEND( &eu->l, &eumate->l );

	eu->up.s_p = dest;
	eumate->up.s_p = dest;

	if (rt_g.NMG_debug & DEBUG_BASIC)  {
		rt_log("nmg_mv_eu_between_shells( dest=x%x, src=x%x, eu=x%x ) new_eu=x%x\n",
			dest, src, eu);
	}
}

/*
 *			N M G _ M O V E _ E G
 *
 *  For every edge in shell 's', change all occurances of edge geometry
 *  structure 'old_eg' to be 'new_eg'.
 */
void
nmg_move_eg( old_eg, new_eg, s )
struct edge_g	*old_eg;
struct edge_g	*new_eg;
struct shell	*s;
{
	struct  faceuse		*fu;
	struct face		*f;
	struct loopuse		*lu;
	struct loop		*l;
	register struct edgeuse		*eu;
	register struct edge		*e;

	NMG_CK_EDGE_G(old_eg);
	NMG_CK_EDGE_G(new_eg);
	NMG_CK_SHELL(s);
	if (rt_g.NMG_debug & DEBUG_BASIC)  {
		rt_log("nmg_move_eg( old_eg=x%x, new_eg=x%x, s=x%x )\n",
			old_eg, new_eg, s );
	}

	/* Faces in shell */
	for( RT_LIST_FOR( fu, faceuse, &s->fu_hd ) )  {
		NMG_CK_FACEUSE(fu);
		f = fu->f_p;
		NMG_CK_FACE(f);
		/* Loops in face */
		for( RT_LIST_FOR( lu, loopuse, &fu->lu_hd ) )  {
			NMG_CK_LOOPUSE(lu);
			l = lu->l_p;
			NMG_CK_LOOP(l);
			if( RT_LIST_FIRST_MAGIC(&lu->down_hd) == NMG_VERTEXUSE_MAGIC )  {
				/* Loop of Lone vertex */
				continue;
			}
			for( RT_LIST_FOR( eu, edgeuse, &lu->down_hd ) )  {
				NMG_CK_EDGEUSE(eu);
				e = eu->e_p;
				NMG_CK_EDGE(e);
				if(e->eg_p == old_eg)  {
					nmg_use_edge_g( e, new_eg );
				}
			}
		}
	}
	/* Wire loops in shell */
	for( RT_LIST_FOR( lu, loopuse, &s->lu_hd ) )  {
		NMG_CK_LOOPUSE(lu);
		l = lu->l_p;
		NMG_CK_LOOP(l);
		if( RT_LIST_FIRST_MAGIC(&lu->down_hd) == NMG_VERTEXUSE_MAGIC )  {
			/* Wire loop of Lone vertex */
			continue;
		}
		for( RT_LIST_FOR( eu, edgeuse, &lu->down_hd ) )  {
			NMG_CK_EDGEUSE(eu);
			e = eu->e_p;
			NMG_CK_EDGE(e);
			if(e->eg_p == old_eg)  {
				nmg_use_edge_g( e, new_eg );
			}
		}
	}
	/* Wire edges in shell */
	for( RT_LIST_FOR( eu, edgeuse, &s->eu_hd ) )  {
		NMG_CK_EDGEUSE(eu);
		e = eu->e_p;
		NMG_CK_EDGE(e);
		if(e->eg_p == old_eg)  {
			nmg_use_edge_g( e, new_eg );
		}
	}
}

/************************************************************************
 *									*
 *				VERTEX Routines				*
 *									*
 ************************************************************************/

/*
 *			N M G _ M V _ V U _ B E T W E E N _ S H E L L S
 *
 *  If this shell had a single vertexuse in it, move it to the other
 *  shell, but "promote" it to a "loop of a single vertex" along the way.
 */
void
nmg_mv_vu_between_shells( dest, src, vu )
struct shell		*dest;
register struct shell	*src;
register struct vertexuse	*vu;
{
	struct loopuse *lu;
	NMG_CK_VERTEXUSE( vu );
	NMG_CK_VERTEX( vu->v_p );

	if (rt_g.NMG_debug & DEBUG_BASIC)  {
		rt_log("nmg_mv_vu_between_shells( dest_s=x%x, src_s=x%x, vu=x%x )\n",
			dest, src, vu);
	}
	lu = nmg_mlv( &(dest->l.magic), vu->v_p, OT_SAME );
	if (vu->v_p->vg_p) {
		NMG_CK_VERTEX_G(vu->v_p->vg_p);
	}
	nmg_kvu( vu );
}
