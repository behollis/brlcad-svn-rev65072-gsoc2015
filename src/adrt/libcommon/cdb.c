/*                     C D B . C
 *
 * @file cdb.c
 *
 * BRL-CAD
 *
 * Copyright (C) 2002-2005 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 *
 *  Comments -
 *      Common Library - Database loader
 *
 *  Author -
 *      Justin L. Shumaker
 *
 *  Source -
 *      The U. S. Army Research Laboratory
 *      Aberdeen Proving Ground, Maryland  21005-5068  USA
 *
 * $Id$
 */

#include "cdb.h"
#include <string.h>
#include "canim.h"
#include "env.h"


int common_db_load(common_db_t *db, char *path);


int common_db_load(common_db_t *db, char *path) {
  char fpath[255];

  /* Parse path out of proj and chdir to it */
  strcpy(fpath, path);
  if(path[strlen(path)-1] != '/')
    strcat(fpath, "/");

  chdir(fpath);

  /* Load Environment Data */
  common_env_init(&db->env);
  common_env_read(&db->env);
  common_env_prep(&db->env);
  
  /* Load Animation Data */
  common_anim_read(&db->anim);

  return(0);
}
