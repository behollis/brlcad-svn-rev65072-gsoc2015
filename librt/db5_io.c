/*
 *			D B 5 _ I O . C
 *
 *  Handle import/export and IO of v5 database objects.
 *
 *  Author -
 *	Michael John Muuss
 *  
 *  Source -
 *	The U. S. Army Research Laboratory
 *	Aberdeen Proving Ground, Maryland  21005-5068  USA
 *  
 *  Distribution Status -
 *	Public Domain, Distribution Unlimited.
 */
#ifndef lint
static char RCSid[] = "@(#)$Header$ (ARL)";
#endif

#include "conf.h"

#include <stdio.h>
#ifdef USE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include "machine.h"
#include "bu.h"
#include "vmath.h"
#include "bn.h"
#include "db5.h"
#include "raytrace.h"

#include "./debug.h"

BU_EXTERN(CONST unsigned char *db5_get_raw_internal_ptr, (struct db5_raw_internal *rip, unsigned char * CONST ip));

/*
 *			D B 5 _ H E A D E R _ I S _ V A L I D
 *
 *  Verify that this is a valid header for a BRL-CAD v5 database.
 *
 *  Returns -
 *	0	Not valid v5 header
 *	1	Valid v5 header
 */
int
db5_header_is_valid( hp )
CONST unsigned char *hp;
{
	CONST struct db5_ondisk_header *odp = (CONST struct db5_ondisk_header *)hp;

	if( odp->db5h_magic1 != DB5HDR_MAGIC1 )  return 0;
	if( hp[7] != DB5HDR_MAGIC2 )  return 0;

	/* hflags */
	if( (odp->db5h_hflags & DB5HDR_HFLAGS_DLI_MASK) != DB5HDR_HFLAGS_DLI_HEADER_OBJECT )
		return 0;
	if( (odp->db5h_hflags & DB5HDR_HFLAGS_NAME_PRESENT) )  return 0;
	if( ((odp->db5h_hflags & DB5HDR_HFLAGS_OBJECT_WIDTH_MASK) >> DB5HDR_HFLAGS_OBJECT_WIDTH_SHIFT)
	    != DB5HDR_WIDTHCODE_8BIT )  return 0;

	/* iflags */
	if( (odp->db5h_iflags & DB5HDR_IFLAGS_ZZZ_MASK) != DB5HDR_IFLAGS_ZZZ_UNCOMPRESSED )  return 0;
	if( odp->db5h_iflags & DB5HDR_IFLAGS_ATTRIBUTES_PRESENT )  return 0;
	if( odp->db5h_iflags & DB5HDR_IFLAGS_BODY_PRESENT )  return 0;
	if( ((odp->db5h_iflags & DB5HDR_IFLAGS_INTERIOR_WIDTH_MASK) >> DB5HDR_IFLAGS_INTERIOR_WIDTH_SHIFT)
	    != DB5HDR_WIDTHCODE_8BIT )  return 0;

	/* major and minor type */
	if( odp->db5h_major_type != DB5HDR_MAJORTYPE_RESERVED )  return 0;
	if( odp->db5h_minor_type != 0 )  return 0;

	/* Check length, known to be 8-bit.  Header len=1 8-byte chunk. */
	if( hp[5] != 1 )  return 0;

	/* Ensure pad is zero */
	if( hp[6] != 0 )  return 0;

	return 1;		/* valid */
}

/*
 *			D B 5 _ S E L E C T _ L E N G T H _ E N C O D I N G
 *
 *  Given a number to encode, decide which is the smallest encoding format
 *  which will contain it.
 */
int
db5_select_length_encoding( len )
long len;
{
	if( len <= 255 )  return DB5HDR_WIDTHCODE_8BIT;
	if( len <= 65535 )  return DB5HDR_WIDTHCODE_16BIT;
	if( len < 0x7ffffffe )  return DB5HDR_WIDTHCODE_32BIT;
	return DB5HDR_WIDTHCODE_64BIT;
}

/*
 *			D B 5 _ D E C O D E _ L E N G T H
 *
 *  Given a variable-width length field in network order (XDR),
 *  store it in *lenp.
 *
 *  Note that for object_length the returned number needs to be
 *  multiplied by 8, while for the other lengths, it is already a byte count.
 *
 *  Returns -
 *	The number of bytes of input that were decoded.
 */
int
db5_decode_length( lenp, cp, format )
long			*lenp;
CONST unsigned char	*cp;
int			format;
{
	switch( format )  {
	case DB5HDR_WIDTHCODE_8BIT:
		*lenp = (*cp);
		return 1;
	case DB5HDR_WIDTHCODE_16BIT:
		*lenp = BU_GSHORT(cp);
		return 2;
	case DB5HDR_WIDTHCODE_32BIT:
		*lenp = BU_GLONG(cp);
		return 4;
	case DB5HDR_WIDTHCODE_64BIT:
#if 0
		if( sizeof(long) >= 8 )  {
			*lenp = BU_GLONGLONG(cp);
			return 8;
		}
#endif
		bu_bomb("db5_decode_length(): encountered 64-bit length on 32-bit machine\n");
	}
	bu_bomb("db5_decode_length(): unknown width code\n");
	/* NOTREACHED */
}

/*
 *			D B 5 _ E N C O D E _ L E N G T H
 *
 *  Given a value and a variable-width format spec,
 *  store it in network order (XDR).
 *
 *  Returns -
 *	pointer to next available byte.
 */
unsigned char *
db5_encode_length( cp, val, format )
unsigned char	*cp;
long		val;
int		format;
{
	switch( format )  {
	case DB5HDR_WIDTHCODE_8BIT:
		*cp = val & 0xFF;
		return cp+1;
	case DB5HDR_WIDTHCODE_16BIT:
		return bu_pshort( cp, (short)val );
	case DB5HDR_WIDTHCODE_32BIT:
		return bu_plong( cp, val );
	case DB5HDR_WIDTHCODE_64BIT:
		bu_bomb("db5_encode_length(): encountered 64-bit length\n");
	}
	bu_bomb("db5_encode_length(): unknown width code\n");
	/* NOTREACHED */
}

/*
 *			D B 5 _ C R A C K _ D I S K _ H E A D E R
 *
 *  Returns -
 *	0 on success
 *	-1 on error
 */
int
db5_crack_disk_header( rip, cp )
struct db5_raw_internal		*rip;
CONST unsigned char		*cp;
{
	if( cp[0] != DB5HDR_MAGIC1 )  return 0;

	/* hflags */
	rip->h_dli = (cp[1] & DB5HDR_HFLAGS_DLI_MASK);
	rip->h_object_width = (cp[1] & DB5HDR_HFLAGS_OBJECT_WIDTH_MASK) >>
		DB5HDR_HFLAGS_OBJECT_WIDTH_SHIFT;
	rip->h_name_present = (cp[1] & DB5HDR_HFLAGS_NAME_PRESENT);

	/* iflags */
	rip->i_object_width = (cp[2] & DB5HDR_IFLAGS_INTERIOR_WIDTH_MASK) >>
		DB5HDR_IFLAGS_INTERIOR_WIDTH_SHIFT;
	rip->i_attributes_present = (cp[2] & DB5HDR_IFLAGS_ATTRIBUTES_PRESENT);
	rip->i_body_present = (cp[2] & DB5HDR_IFLAGS_BODY_PRESENT);
	rip->i_zzz = (cp[2] & DB5HDR_IFLAGS_ZZZ_MASK);

	rip->major_type = cp[3];
	rip->minor_type = cp[4];

	if(rt_g.debug&DEBUG_DB) bu_log("db5_crack_disk_header() h_dli=%d, h_object_width=%d, h_name_present=%d, i_object_width=%d, i_attributes_present=%d, i_body_present=%d, i_zzz=%d, major=%d, minor=%d\n",
		rip->h_dli,
		rip->h_object_width,
		rip->h_name_present,
		rip->i_object_width,
		rip->i_attributes_present,
		rip->i_body_present,
		rip->i_zzz,
		rip->major_type,
		rip->minor_type );

	return 0;
}

/*
 *			D B 5 _ G E T _ R A W _ I N T E R N A L _ P T R
 *
 *  Returns -
 *	on success, pointer to first unused byte
 *	NULL, on error
 */
CONST unsigned char *
db5_get_raw_internal_ptr( rip, ip )
struct db5_raw_internal		*rip;
unsigned char		* CONST ip;
{
	CONST unsigned char	*cp = ip;

	if( db5_crack_disk_header( rip, cp ) < 0 )  return NULL;
	cp += sizeof(struct db5_ondisk_header);

	cp += db5_decode_length( &rip->object_length, cp, rip->h_object_width );
	rip->object_length <<= 3;	/* cvt 8-byte chunks to byte count */

	/* Verify trailing magic number */
	if( ip[rip->object_length-1] != DB5HDR_MAGIC2 )  {
		bu_log("db5_get_raw_internal_ptr() bad magic2\n");
		return NULL;
	}

	/* Grab name, if present */
	if( rip->h_name_present )  {
		cp += db5_decode_length( &rip->name_length, cp, rip->i_object_width );
		rip->name = (char *)cp;		/* discard CONST */
		cp += rip->name_length;
	} else {
		rip->name_length = 0;
		rip->name = NULL;
	}

	/* Point to object interior, if present */
	if( rip->i_attributes_present || rip->i_body_present )  {
		/* interior_length will include any pad bytes but not magic2 */
		/* Because it may be compressed, we don't know exact len yet */
		rip->interior_length = cp - rip->buf - 1;
		rip->interior = (unsigned char *)cp;	/* discard CONST */
	} else {
		rip->interior_length = 0;
		rip->interior = NULL;
	}

	rip->buf = NULL;	/* no buffer needs freeing */

	return ip + rip->object_length;
}

/*
 *			D B 5 _ G E T _ R A W _ I N T E R N A L _ F P
 *
 *  Returns -
 *	0 on success
 *	-1 on EOF
 *	-2 on error
 */
int
db5_get_raw_internal_fp( rip, fp )
struct db5_raw_internal	*rip;
FILE			*fp;
{
	struct db5_ondisk_header	header;
	unsigned char			lenbuf[8];
	int				count;
	int				used;
	long				want, got;
	unsigned char			*cp;

	if( fread( (unsigned char *)&header, sizeof header, 1, fp ) != 1  )  {
		if( feof(fp) )  return -1;
		bu_log("db5_get_raw_internal_fp(): fread header error\n");
		return -2;
	}
	if( db5_crack_disk_header( rip, (unsigned char *)&header ) < 0 )
		return -2;
	used = sizeof(header);

	switch( rip->h_object_width )  {
	case DB5HDR_WIDTHCODE_8BIT:
		count = 1;
		break;
	case DB5HDR_WIDTHCODE_16BIT:
		count = 2;
		break;
	case DB5HDR_WIDTHCODE_32BIT:
		count = 4;
		break;
	case DB5HDR_WIDTHCODE_64BIT:
		count = 8;
	}
	if( fread( lenbuf, count, 1, fp )  != 1 )  {
		bu_log("db5_get_raw_internal_fp(): fread lenbuf error\n");
		return -2;
	}
	used += db5_decode_length( &rip->object_length, lenbuf, rip->h_object_width );
	rip->object_length <<= 3;	/* cvt 8-byte chunks to byte count */

	/* Now that we finally know how large the object is, get it all */
	rip->buf = (unsigned char *)bu_malloc( rip->object_length, "raw v5 object" );

	*((struct db5_ondisk_header *)rip->buf) = header;	/* struct copy */
	bcopy( lenbuf, rip->buf+sizeof(header), count );

	cp = rip->buf+used;
	want = rip->object_length-used;
	BU_ASSERT_LONG( want, >, 0 );
	if( (got = fread( cp, 1, want, fp )) != want ) {
		bu_log("db5_get_raw_internal_fp(), want=%ld, got=%ld, database is too short\n",
			want, got );
		return -2;
	}

	/* Verify trailing magic number */
	if( rip->buf[rip->object_length-1] != DB5HDR_MAGIC2 )  {
		bu_log("db5_get_raw_internal_fp() bad magic2\n");
		return -2;
	}

	/* Grab name, if present */
	if( rip->h_name_present )  {
		cp += db5_decode_length( &rip->name_length, cp, rip->i_object_width );
		rip->name = (char *)cp;
		cp += rip->name_length;
	} else {
		rip->name_length = 0;
		rip->name = NULL;
	}

	/* Point to object interior, if present */
	if( rip->i_attributes_present || rip->i_body_present )  {
		/* interior_length will include any pad bytes but not magic2 */
		/* Because it may be compressed, we don't know exact len yet */
		rip->interior_length = cp - rip->buf - 1;
		rip->interior = cp;
	} else {
		rip->interior_length = 0;
		rip->interior = NULL;
	}

	return 0;		/* success */
}

/*
 *			D B 5 _ E X P O R T _ O B J E C T 3
 *
 *  A routine for merging together the three optional
 *  parts of an object into the final on-disk format.
 *  Results in extra data copies, but serves as a starting point for testing.
 *  Any of name, attrib, and body may be null.
 */
void
db5_export_object3( out, dli, name, attrib, body, major, minor, zzz )
struct bu_external		*out;			/* output */
int				dli;
CONST char			*name;
CONST struct bu_external	*attrib;
CONST struct bu_external	*body;
int				major;
int				minor;
int				zzz;		/* compression, someday */
{
	struct db5_ondisk_header *odp;
	register unsigned char	*cp;
	long	namelen = 0;
	long	need;
	long	ineed = 0;	/* sizes of internal parts */
	int	h_width, i_width;	
	long	togo;

	/*
	 *  First, compute an upper bound on the size buffer needed.
	 *  Over-estimate on the length fields just to keep it simple.
	 */
	need = sizeof(struct db5_ondisk_header);
	need += 8;	/* for object_length */
	if( name )  {
		namelen = strlen(name) + 1;	/* includes null */
		need += namelen + 8;
		ineed = namelen;
	}
	if( attrib )  {
		BU_CK_EXTERNAL(attrib);
		need += attrib->ext_nbytes + 8;
		ineed |= attrib->ext_nbytes;
	}
	if( body )  {
		BU_CK_EXTERNAL(body);
		need += body->ext_nbytes + 8;
		ineed |= body->ext_nbytes;
	}
	need += 8;	/* pad and magic2 */

	/* Allocate the buffer for the combined external representation */
	out->ext_magic = BU_EXTERNAL_MAGIC;
	out->ext_nbytes = 0;
	out->ext_buf = bu_malloc( need, "external object3" );

	/* Determine encoding for the two kinds of length fields */
	h_width = db5_select_length_encoding( (need+7)>>3 );
	i_width = db5_select_length_encoding( ineed );

	/* prepare combined external object */
	odp = (struct db5_ondisk_header *)out->ext_buf;
	odp->db5h_magic1 = DB5HDR_MAGIC1;

	/* hflags */
	odp->db5h_hflags = (h_width << DB5HDR_HFLAGS_OBJECT_WIDTH_SHIFT) |
			(dli & DB5HDR_HFLAGS_DLI_MASK);
	if( name )  odp->db5h_hflags |= DB5HDR_HFLAGS_NAME_PRESENT;

	/* iflags */
	odp->db5h_iflags = i_width << DB5HDR_IFLAGS_INTERIOR_WIDTH_SHIFT;
	if( attrib )  odp->db5h_iflags |= DB5HDR_IFLAGS_ATTRIBUTES_PRESENT;
	if( body )  odp->db5h_iflags |= DB5HDR_IFLAGS_BODY_PRESENT;
	odp->db5h_iflags |= zzz & DB5HDR_IFLAGS_ZZZ_MASK;

	if( zzz )  bu_bomb("db5_export_object3: compression not supported yet\n");

	/* Object_Type */
	odp->db5h_major_type = major;
	odp->db5h_minor_type = minor;

	/* Build up the rest of the record */
	cp = ((unsigned char *)out->ext_buf) + sizeof(struct db5_ondisk_header);
	cp = db5_encode_length( cp, 0L, h_width );	/* will be replaced below */

	if( name )  {
		cp = db5_encode_length( cp, namelen, i_width );
		bcopy( name, cp, namelen );	/* includes null */
		cp += namelen;
	}

	if( attrib )  {
		cp = db5_encode_length( cp, attrib->ext_nbytes, i_width );
		bcopy( attrib->ext_buf, cp, attrib->ext_nbytes );
		cp += attrib->ext_nbytes;
	}

	if( body )  {
		cp = db5_encode_length( cp, body->ext_nbytes, i_width );
		bcopy( body->ext_buf, cp, body->ext_nbytes );
		cp += body->ext_nbytes;
	}

	togo = cp - ((unsigned char *)out->ext_buf) + 1;
	togo &= 7;
	if( togo != 0 )  {
		togo = 8 - togo;
		while( togo-- > 0 )  *cp++ = '\0';
	}
	*cp++ = DB5HDR_MAGIC2;

	/* Verify multiple of 8 */
	togo = cp - ((unsigned char *)out->ext_buf);
	BU_ASSERT_LONG( togo&7, ==, 0 );

	/* Finally, go back to the header and write the actual object length */
	cp = ((unsigned char *)out->ext_buf) + sizeof(struct db5_ondisk_header);
	cp = db5_encode_length( cp, togo>>3, h_width );

	out->ext_nbytes = togo;
}

/*
 *			D B 5 _ E X P O R T _ A T T R I B U T E S
 *
 *  One attempt at encoding attribute-value information in the external
 *  format.
 *  This may not be the best or only way to do it, but it gets things
 *  started, for testing.
 *
 *  The on-disk encoding is:
 *
 *	aname1 NULL value1 NULL ... anameN NULL valueN NULL NULL
 */
void
db5_export_attributes( ext, avp )
struct bu_external		*ext;
CONST struct bu_attribute_value_pair	*avp;
{
	int	need = 0;
	CONST struct bu_attribute_value_pair	*avpp;
	char	*cp;

	/* First pass -- determine how much space is required */
	for( avpp = avp; avpp->name != NULL; avpp++ )  {
		need += strlen( avpp->name ) + strlen( avpp->value ) + 2;
	}
	need += 1;		/* for final null */

	ext->ext_magic = BU_EXTERNAL_MAGIC;
	ext->ext_nbytes = need;
	ext->ext_buf = bu_malloc( need, "external attributes" );

	/* Second pass -- store in external form */
	cp = (char *)ext->ext_buf;
	for( avpp = avp; avpp->name != NULL; avpp++ )  {
		need = strlen( avpp->name ) + 1;
		bcopy( avpp->name, cp, need );
		cp += need;

		need = strlen( avpp->value ) + 1;
		bcopy( avpp->value, cp, need );
		cp += need;
	}
	*cp++ = '\0';
	need = cp - ((char *)ext->ext_buf);
	BU_ASSERT_LONG( need, ==, ext->ext_nbytes );
}

/*
 *			D B 5 _ F W R I T E _ I D E N T
 *
 *  Create a header for a v5 database.
 *  This routine has the same calling sequence as db_fwrite_ident()
 *  which makes a v4 database header.
 *
 *  In the v5 database, two database objects must be created to
 *  match the semantics of what was in the v4 header:
 *
 *  First, a database header object.
 *
 *  Second, an attribute-only object named "_GLOBAL" which
 *  contains the attributes "title=" and "units=".
 *
 * Returns -
 *	 0	Success
 *	-1	Fatal Error
 */
int
db5_fwrite_ident( fp, title, local2mm )
FILE		*fp;
CONST char	*title;
double		local2mm;
{
	struct bu_attribute_value_pair avp[3];
	struct bu_vls		units;
	struct bu_external	out;
	struct bu_external	attr;

	/* First, write the header object */
	db5_export_object3( &out, DB5HDR_HFLAGS_DLI_HEADER_OBJECT,
		NULL, NULL, NULL,
		DB5HDR_MAJORTYPE_RESERVED, 0,
		DB5HDR_IFLAGS_ZZZ_UNCOMPRESSED );
	bu_fwrite_external( fp, &out );
	bu_free_external( &out );

	/* Second, create the attribute-only object */
	bu_vls_init( &units );
	bu_vls_printf( &units, "%.25f", local2mm );

	avp[0].name = "title";
	avp[0].value = (char *)title;		/* un-CONST */
	avp[1].name = "units";
	avp[1].value = bu_vls_addr(&units);
	avp[2].name = NULL;
	avp[2].value = NULL;

	db5_export_attributes( &attr, avp );
	db5_export_object3( &out, DB5HDR_HFLAGS_DLI_APPLICATION_DATA_OBJECT,
		"_GLOBAL", &attr, NULL,
		DB5HDR_MAJORTYPE_ATTRIBUTE_ONLY, 0,
		DB5HDR_IFLAGS_ZZZ_UNCOMPRESSED);
	bu_fwrite_external( fp, &out );
	bu_free_external( &out );
	bu_free_external( &attr );

	bu_vls_free( &units );

	return 0;
}

/*
 *			R T _ D B _ P U T _ I N T E R N A L 5
 *
 *  Convert the internal representation of a solid to the external one,
 *  and write it into the database.
 *  On success only, the internal representation is freed.
 *
 *  Returns -
 *	<0	error
 *	 0	success
 */
int
rt_db_put_internal5( dp, dbip, ip )
struct directory	*dp;
struct db_i		*dbip;
struct rt_db_internal	*ip;
{
	struct bu_external	ext;
	int			ret;

	BU_INIT_EXTERNAL(&ext);
	RT_CK_DB_INTERNAL( ip );

	/* Scale change on export is 1.0 -- no change */
	ret = ip->idb_meth->ft_export( &ext, ip, 1.0, dbip );
	if( ret < 0 )  {
		bu_log("rt_db_put_internal(%s):  solid export failure\n",
			dp->d_namep);
		db_free_external( &ext );
		return -2;		/* FAIL */
	}

	if( db_put_external( &ext, dp, dbip ) < 0 )  {
		db_free_external( &ext );
		return -1;		/* FAIL */
	}

    	if( ip->idb_ptr )  ip->idb_meth->ft_ifree( ip );

	RT_INIT_DB_INTERNAL(ip);
	db_free_external( &ext );
	return 0;			/* OK */
}

/*
 *			R T _ F W R I T E _ I N T E R N A L 5
 *
 *  Put an object in internal format out onto a file in external format.
 *  Used by LIBWDB.
 *
 *  Can't really require a dbip parameter, as many callers won't have one.
 *
 *  Returns -
 *	0	OK
 *	<0	error
 */
int
rt_fwrite_internal5( fp, name, ip, conv2mm, attr )
FILE				*fp;
CONST char			*name;
CONST struct rt_db_internal	*ip;
double				conv2mm;
CONST struct bu_attribute_value_pair	*attr;
{
	struct bu_external	body;
	struct bu_external	ext;
	int			major, minor;

	RT_CK_DB_INTERNAL(ip);
	RT_CK_FUNCTAB( ip->idb_meth );

	if( ip->idb_meth->ft_export( &body, ip, conv2mm, NULL /*dbip*/ ) < 0 )  {
		bu_log("rt_fwrite_internal5(%s): solid export failure\n",
			name );
		db_free_external( &body );
		return -2;				/* FAIL */
	}
	BU_CK_EXTERNAL( &body );

	/* What about combinations? */
	major = DB5HDR_MAJORTYPE_BRLCAD_GEOMETRY;
	minor = ip->idb_type;	/* XXX not necessarily v5 numbers. */

	db5_export_object3( &ext, DB5HDR_HFLAGS_DLI_APPLICATION_DATA_OBJECT,
		name, attr, &body,
		major, minor,
		DB5HDR_IFLAGS_ZZZ_UNCOMPRESSED);
	db_free_external( &body );

	if( db_fwrite_external( fp, name, &ext ) < 0 )  {
		bu_log("rt_fwrite_internal(%s): db_fwrite_external() error\n",
			name );
		db_free_external( &ext );
		return -3;
	}
	db_free_external( &ext );
	return 0;

}
