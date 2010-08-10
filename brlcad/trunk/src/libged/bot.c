/*                         B O T . C
 * BRL-CAD
 *
 * Copyright (c) 2008-2010 United States Government as represented by
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
/** @file bot.c
 *
 * Bot command for simple bot primitive operations.
 *
 */

#include "common.h"

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "bio.h"

#include "rtgeom.h"

#include "./ged_private.h"


int
ged_bot(struct ged *gedp, int argc, const char *argv[])
{
    struct directory *bot_dp;
    struct rt_db_internal intern;
    struct rt_bot_internal *bot;
    char *cmd = argv[0];
    char *primitive = argv[argc - 1];
    char prop = '\0';
    static const char *usage = "get (faces|orientation|type|vertices) bot\n";

    GED_CHECK_DATABASE_OPEN(gedp, GED_ERROR);
    GED_CHECK_READ_ONLY(gedp, GED_ERROR);
    GED_CHECK_ARGC_GT_0(gedp, argc, GED_ERROR);

    /* initialize result */
    bu_vls_trunc(&gedp->ged_result_str, 0);

    /* must be wanting help */
    if (argc < 3) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", cmd, usage);
	return GED_ERROR;
    }

    /* get bot */
    GED_DB_LOOKUP(gedp, bot_dp, primitive, LOOKUP_NOISY, GED_ERROR & GED_QUIET);
    GED_DB_GET_INTERNAL(gedp, &intern,  bot_dp, bn_mat_identity, &rt_uniresource, GED_ERROR);

    if (intern.idb_major_type != DB5_MAJORTYPE_BRLCAD || intern.idb_minor_type != DB5_MINORTYPE_BRLCAD_BOT) {
	bu_vls_printf(&gedp->ged_result_str, "%s: %s is not a BOT solid!", cmd, primitive);
	return GED_ERROR;
    }

    bot = (struct rt_bot_internal *)intern.idb_ptr;
    RT_BOT_CK_MAGIC(bot);

    /* run subcommand */
    char *sub = argv[1];
    char *arg = argv[2];
    size_t len = strlen(sub);

    if (strncmp(sub, "get", len) == 0) {
	int propVal = rt_bot_propget(bot, arg);

	if (propVal != -1) {
	    bu_vls_printf(&gedp->ged_result_str, "%d", propVal);
	} else {
	    bu_vls_printf(&gedp->ged_result_str, "%s: %s is not a valid argument!", sub, arg);
	    return GED_ERROR;
	}
    } else {
	bu_vls_printf(&gedp->ged_result_str, "%s: %s is not a known subcommand!", cmd, sub);
	return GED_ERROR;
    }

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
