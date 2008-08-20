/*                         E X P A N D . C
 * BRL-CAD
 *
 * Copyright (c) 2008 United States Government as represented by
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
/** @file expand.c
 *
 * The expand command.
 *
 */

#include "common.h"

#include <string.h>

#include "bio.h"
#include "cmd.h"
#include "ged_private.h"

static void
ged_scrape_escapes_AppendResult(struct bu_vls	*result,
				char		*str);

int
ged_expand(struct ged *gedp, int argc, const char *argv[])
{
    register char *pattern;
    register struct directory *dp;
    register int i, whicharg;
    int regexp, nummatch, thismatch, backslashed;
    static const char *usage = "expression";

    GED_CHECK_DATABASE_OPEN(gedp, BRLCAD_ERROR);

    /* initialize result */
    bu_vls_trunc(&gedp->ged_result_str, 0);
    gedp->ged_result = GED_RESULT_NULL;
    gedp->ged_result_flags = 0;

    /* invalid command name */
    if (argc < 1) {
	bu_vls_printf(&gedp->ged_result_str, "Error: command name not provided");
	return BRLCAD_ERROR;
    }

    /* must be wanting help */
    if (argc == 1) {
	gedp->ged_result_flags |= GED_RESULT_FLAGS_HELP_BIT;
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_OK;
    }

    if (MAXARGS < argc) {
	bu_vls_printf(&gedp->ged_result_str, "Usage: %s %s", argv[0], usage);
	return BRLCAD_ERROR;
    }

    nummatch = 0;
    backslashed = 0;
    for (whicharg = 1; whicharg < argc; whicharg++) {
	/* If * ? or [ are present, this is a regular expression */
	pattern = (char *)argv[whicharg];
	regexp = 0;
	do {
	    if ((*pattern == '*' || *pattern == '?' || *pattern == '[') &&
		!backslashed) {
		regexp = 1;
		break;
	    }
	    if (*pattern == '\\' && !backslashed)
		backslashed = 1;
	    else
		backslashed = 0;
	} while (*pattern++);

	/* If it isn't a regexp, copy directly and continue */
	if (regexp == 0) {
	    if (nummatch > 0)
		bu_vls_printf(&gedp->ged_result_str, " ");
	    ged_scrape_escapes_AppendResult(&gedp->ged_result_str, (char *)argv[whicharg]);
	    ++nummatch;
	    continue;
	}

	/* Search for pattern matches.
	 * If any matches are found, we do not have to worry about
	 * '\' escapes since the match coming from dp->d_namep will be
	 * clean. In the case of no matches, just copy the argument
	 * directly.
	 */

	pattern = (char *)argv[whicharg];
	thismatch = 0;
	for (i = 0; i < RT_DBNHASH; i++) {
	    for (dp = gedp->ged_wdbp->dbip->dbi_Head[i]; dp != DIR_NULL; dp = dp->d_forw) {
		if (!db_regexp_match(pattern, dp->d_namep))
		    continue;
		/* Successful match */
		if (nummatch == 0)
		    bu_vls_printf(&gedp->ged_result_str, "%s", dp->d_namep);
		else
		    bu_vls_printf(&gedp->ged_result_str, " %s", dp->d_namep);
		++nummatch;
		++thismatch;
	    }
	}
	if (thismatch == 0) {
	    if (nummatch > 0)
		bu_vls_printf(&gedp->ged_result_str, " ");
	    ged_scrape_escapes_AppendResult(&gedp->ged_result_str, (char *)argv[whicharg]);
	}
    }

    return BRLCAD_OK;
}

static void
ged_scrape_escapes_AppendResult(struct bu_vls	*result,
				char		*str)
{
    char buf[2];
    buf[1] = '\0';

    while (*str) {
	buf[0] = *str;
	if (*str != '\\') {
	    bu_vls_printf(result, "%s", buf);
	} else if (*(str+1) == '\\') {
	    bu_vls_printf(result, "%s", buf);
	    ++str;
	}
	if (*str == '\0')
	    break;
	++str;
    }
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
