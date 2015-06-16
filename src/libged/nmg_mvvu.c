/*                         N M G _ M V V U . C
 * BRL-CAD
 *
 * Copyright (c) 2008-2014 United States Government as represented by
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
/** @file libged/nmg_mvvu.c
 *
 * The nmg_mvvu command.
 *
 */

#include "common.h"

#include <string.h>

#include "bu/cmd.h"
#include "rt/geom.h"

#include "./ged_private.h"

int
ged_nmg_mvvu(struct ged *gedp, int argc, const char *argv[])
{
	struct rt_db_internal internal;
	struct directory *dp;
	struct model* m;
	const char* name;
	struct nmgregion* r;
	struct shell* s;
	struct vertexuse* vu;

	static const char *usage = "nmg_name";

	GED_CHECK_DATABASE_OPEN(gedp, GED_ERROR);
	GED_CHECK_READ_ONLY(gedp, GED_ERROR);
	GED_CHECK_ARGC_GT_0(gedp, argc, GED_ERROR);

	if (argc != 2) {
		bu_vls_printf(gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
		return GED_HELP;
	}

	/* attempt to resolve and verify */
	name = argv[1];

	if ( (dp=db_lookup(gedp->ged_wdbp->dbip, name, LOOKUP_QUIET))
		== RT_DIR_NULL ) {
		bu_vls_printf(gedp->ged_result_str, "%s does not exist\n", name);
		return GED_ERROR;
	}

	if (rt_db_get_internal(&internal, dp, gedp->ged_wdbp->dbip,
		bn_mat_identity, &rt_uniresource) < 0) {
		bu_vls_printf(gedp->ged_result_str, "rt_db_get_internal() error\n");
		return GED_ERROR;
	}

	if (internal.idb_type != ID_NMG) {
		bu_vls_printf(gedp->ged_result_str, "%s is not an NMG solid\n", name);
		rt_db_free_internal(&internal);
		return GED_ERROR;
	}

	m = (struct model *)internal.idb_ptr;
	NMG_CK_MODEL(m);

	/* For now, let's just add vertexuses for the first
	*  encountered region's first shell.
	*  TODO: take shell from CLI
	*/
	r = BU_LIST_FIRST(nmgregion, &m->r_hd);
	NMG_CK_REGION(r);

	s = BU_LIST_FIRST(shell, &r->s_hd);
	NMG_CK_SHELL(s);

	/* Make vertex / vertexuse with first shell as parent structure.
	 * Note that this does not add an instance of a vertex, only
	 * a NULL pointer cast to a struct vertex_g *  */
	vu = (struct vertexuse*)nmg_mvvu((struct shell*)s, m);

	/* Now add the new vertexuse to the model's first region / shell. */
	s->vu_p = vu;

	return GED_OK;
}

/*
 * Local Variables:
 * tab-width: 8
 * mode: C
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
