/*                         M A T E R . H
 * BRL-CAD
 *
 * Copyright (c) 1985-2008 United States Government as represented by
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
/** @addtogroup librt */
/** @{ */
/** @file mater.h
 *
 * @brief
 *  Information about mapping between region IDs and material
 *  information (colors and outboard database "handles").
 *
 *  @author
 *	Michael John Muuss
 *
 */

#include "bu.h"

#ifndef RT_EXPORT
#if defined(_WIN32) && !defined(__CYGWIN__) && defined(BRLCAD_DLL)
#ifdef RT_EXPORT_DLL
#define RT_EXPORT __declspec(dllexport)
#else
#define RT_EXPORT __declspec(dllimport)
#endif
#else
#define RT_EXPORT
#endif
#endif

struct mater {
	short		mt_low;		/**< @brief bounds of region IDs, inclusive */
	short		mt_high;
	unsigned char	mt_r;		/**< @brief color */
	unsigned char	mt_g;
	unsigned char	mt_b;
	long		mt_daddr;	/**< @brief db address, for updating */
	struct mater	*mt_forw;	/**< @brief next in chain */
};
#define MATER_NULL	((struct mater *)0)
#define MATER_NO_ADDR	(-1L)		/**< @brief invalid mt_daddr */

RT_EXPORT extern struct mater *rt_material_head; /**< @brief defined in mater.c */
RT_EXPORT BU_EXTERN(void rt_insert_color,
		       (struct mater *newp));
/** @} */
/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
