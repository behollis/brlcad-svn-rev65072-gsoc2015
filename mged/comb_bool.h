/*
 *			C O M B _ B O O L . H
 *
 *  Author -
 *	Paul Tanenbaum
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
 *	This software is Copyright (C) 1995 by the United States Army
 *	in all countries except the USA.  All rights reserved.
 */

struct bool_tree_node
{
    long			    btn_magic;
    int				    btn_opn;
    union
    {
	char			*leaf_name;
	struct bool_tree_node	*operands[2];
    }				    btn_operands;
};
#define	BOOL_TREE_NODE_NULL	((struct bool_tree_node *) 0)

#define	OPN_NULL		0
#define	OPN_UNION		1
#define	OPN_INTERSECTION	2
#define	OPN_DIFFERENCE		3

#define	BT_LEFT			0
#define	BT_RIGHT		1

#define bt_opn(n)		((n) -> btn_opn)
#define bt_leaf_name(n)		((n) -> btn_operands.leaf_name)
#define bt_opd(n,d)		((n) -> btn_operands.operands[(d)])

#define bt_is_leaf(n)		(bt_opn((n)) == OPN_NULL)

struct tree_tail
{
    long		tt_magic;
    int			tt_opn;
    struct tree_tail	*tt_next;
};
#define	TREE_TAIL_NULL	((struct tree_tail *) 0)

/*			C H K M A G ( )
 *
 *	    Check and validate a structure pointer
 *
 *	This macro has three parameters: a pointer, the magic number
 *	expected at that location, and a string describing the expected
 *	structure type.
 */
#define	CHKMAG(p, m, _s)					\
    if ((p) == 0)						\
    {								\
	fprintf(stderr, "Error: Null %s pointer, file %s, line %d\n",	\
	    (_s), __FILE__, __LINE__);				\
	exit (0);						\
    }								\
    else if (*((long *)(p)) != (m))				\
    {								\
	fprintf(stderr,						\
	    "Error: Bad %s pointer x%x s/b x%x was x%x, file %s, line %d\n", \
	    (_s), (p), (m), *((long *)(p)), __FILE__, __LINE__);\
	exit (0);						\
    }
#define ZAPMAG(p)		(*((long *)(p)) = 0)
#define	BOOL_TREE_NODE_MAGIC	0x62746e64
#define	TREE_TAIL_MAGIC		0x74727461

extern struct bool_tree_node	*comb_bool_tree;

/*                      T A L L O C ( )
 *
 *                  Simple interface to malloc()
 *
 *      This macro has three parameters:  a pointer, a C data type,
 *      and a number of data objects.  Talloc() allocates enough
 *      memory to store n objects of type t.  It has the side-effect
 *      of causing l to point to the allocated storage.
 */
#define         talloc(l, t, n)                                         \
        if (((l) = (t *) malloc(n * sizeof(t))) == (t *) 0)             \
        {                                                               \
            fprintf(stderr, "%s:%d: Ran out of memory\n",		\
                    __FILE__, __LINE__);                                \
            exit(1);                                                    \
        }

/*
 *	
 */
extern struct bool_tree_node	*bt_create_internal (
				    int,
				    struct bool_tree_node *,
				    struct bool_tree_node *);
extern struct bool_tree_node	*bt_create_leaf (char*);
extern void			show_tree_infix (
				    struct bool_tree_node *,
				    int);
extern void			show_tree_lisp (struct bool_tree_node *);
#define show_tree(t,l)		if (l)				\
				{				\
				    show_tree_lisp((t));	\
				    printf("\n");		\
				}				\
				else				\
				    show_tree_infix((t), 0)
extern int			cvt_to_gift_bool (struct bool_tree_node *);
extern void			show_gift_bool (struct bool_tree_node *, int);
