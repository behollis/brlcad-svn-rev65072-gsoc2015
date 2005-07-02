/*                     P A C K . C
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
 */
/** @file pack.c
 *                     P A C K . C
 *
 *  Common Library - Parsing and Packing
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

#include "pack.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "umath.h"
#include "texture.h"
#include "tienet.h"
#include "brlcad_config.h"


#define K *1024
#define M *1024 K


void	common_pack_write(void **dest, int *ind, void *src, int size);

int	common_pack(common_db_t *db, void **app_data, char *proj);
void	common_pack_camera(common_db_t *db, void **app_data, int *app_ind);
void	common_pack_env(common_db_t *db, void **app_data, int *app_ind);
void	common_pack_prop(void **app_data, int *app_ind, char *filename);
void	common_pack_texture(void **app_data, int *app_ind, char *filename);

void	common_pack_mesh(common_db_t *db, void **app_data, int *app_ind, char *filename);
void	common_pack_mesh_adrt(common_db_t *db, void **app_data, int *app_ind, char *filename);


int	common_pack_app_size;
int	common_pack_app_mem;
int	common_pack_trinum;


void common_pack_write(void **dest, int *ind, void *src, int size) {
  if((int)(*ind + size) > (int)common_pack_app_size)
    common_pack_app_size = *ind + size;

  if(common_pack_app_size > common_pack_app_mem) {
    common_pack_app_mem = common_pack_app_size + (16 M);
    *dest = realloc(*dest, common_pack_app_mem);
  }

  memcpy(&(((char *)*dest)[*ind]), src, size);
  *ind += size;
}


int common_pack(common_db_t *db, void **app_data, char *proj) {
  short s;
  int app_ind, i;

  common_pack_app_size = 0;
  common_pack_app_mem = 0;

  common_pack_trinum = 0;

  /* Initialize app_data */
  app_ind = 0;
  (*app_data) = NULL;

  /* VERSION */
  s = 0;
  common_pack_write(app_data, &app_ind, &s, sizeof(short));

  /* ENVIRONMENT DATA */
  common_pack_env(db, app_data, &app_ind);

  /* CAMERA DATA */
  common_pack_camera(db, app_data, &app_ind);

  /* PROPERTY DATA */
  common_pack_prop(app_data, &app_ind, db->env.properties_file);

  /* TEXTURE DATA */
  common_pack_texture(app_data, &app_ind, db->env.textures_file);

  /* MESH DATA */
  common_pack_mesh(db, app_data, &app_ind, db->env.geometry_file);

  *app_data = realloc(*app_data, common_pack_app_size);
  return(common_pack_app_size);
}


void common_pack_camera(common_db_t *db, void **app_data, int *app_ind) {
  short s;
  int marker, size;

  s = COMMON_PACK_CAMERA;
  common_pack_write(app_data, app_ind, &s, sizeof(short));
  marker = *app_ind;
  *app_ind += sizeof(int);


  common_pack_write(app_data, app_ind, &db->anim.frame_list[0].pos, sizeof(TIE_3));
  common_pack_write(app_data, app_ind, &db->anim.frame_list[0].focus, sizeof(TIE_3));
  common_pack_write(app_data, app_ind, &db->anim.frame_list[0].tilt, sizeof(tfloat));
  common_pack_write(app_data, app_ind, &db->anim.frame_list[0].fov, sizeof(tfloat));
  common_pack_write(app_data, app_ind, &db->anim.frame_list[0].dof, sizeof(tfloat));


  size = *app_ind - marker - sizeof(int);
  common_pack_write(app_data, &marker, &size, sizeof(int));
  *app_ind = marker + size;
}


void common_pack_env(common_db_t *db, void **app_data, int *app_ind) {
  short s;
  int marker, size;


  s = COMMON_PACK_ENV;
  common_pack_write(app_data, app_ind, &s, sizeof(short));
  marker = *app_ind;
  *app_ind += sizeof(int);


  s = COMMON_PACK_ENV_RM;
  common_pack_write(app_data, app_ind, &s, sizeof(short));
  common_pack_write(app_data, app_ind, &db->env.rm, sizeof(int));

  switch(db->env.rm) {
    case RENDER_METHOD_NORMAL:
      break;

    case RENDER_METHOD_PHONG:
      break;

    case RENDER_METHOD_PATH:
      common_pack_write(app_data, app_ind, &((render_path_t *)db->env.render.data)->samples, sizeof(render_path_t));
      break;

    case RENDER_METHOD_KELOS:
      break;

    case RENDER_METHOD_PLANE:
      break;

    case RENDER_METHOD_FLAT:
      break;

    default:
      break;
  }

  s = COMMON_PACK_ENV_IMAGESIZE;
  common_pack_write(app_data, app_ind, &s, sizeof(short));
  common_pack_write(app_data, app_ind, &db->env.img_w, sizeof(int));
  common_pack_write(app_data, app_ind, &db->env.img_h, sizeof(int));
  common_pack_write(app_data, app_ind, &db->env.img_hs, sizeof(int));


  size = *app_ind - marker - sizeof(int);
  common_pack_write(app_data, &marker, &size, sizeof(int));
  *app_ind = marker + size;
}


void common_pack_prop(void **app_data, int *app_ind, char *filename) {
  FILE *fh;
  common_prop_t def_prop;
  short s;
  char c, line[256], name[256], *token;
  int marker, size, prop_num;


  s = COMMON_PACK_PROP;
  common_pack_write(app_data, app_ind, &s, sizeof(short));
  marker = *app_ind;
  *app_ind += sizeof(int);


  fh = fopen(filename, "r");
  if(!fh) {
    printf("Properties file: %s does not exist, exiting...\n", filename);
    exit(1);
  }

  prop_num = 0;
  while(fgets(line, 256, fh)) {
    token = strtok(line, ",");
    if(!strcmp("properties", token)) {

      if(prop_num) {
        /* pack name */
        c = strlen(name) + 1;
        common_pack_write(app_data, app_ind, &c, sizeof(char));
        common_pack_write(app_data, app_ind, name, c);
        /* pack properties data */
        common_pack_write(app_data, app_ind, &def_prop, sizeof(common_prop_t));
      }

      token = strtok(NULL, ",");
      /* strip off newline */
      if(token[strlen(token)-1] == '\n') token[strlen(token)-1] = 0;
      strcpy(name, token);

      /* set defaults */
      def_prop.color.v[0] = 0.8;
      def_prop.color.v[1] = 0.8;
      def_prop.color.v[2] = 0.8;
      def_prop.density = 0.5;
      def_prop.gloss = 0.5;
      def_prop.emission = 0.0;
      def_prop.ior = 1.0;

      prop_num++;

    } else if(!strcmp("color", token)) {

      token = strtok(NULL, ",");
      def_prop.color.v[0] = atof(token);
      token = strtok(NULL, ",");
      def_prop.color.v[1] = atof(token);
      token = strtok(NULL, ",");
      def_prop.color.v[2] = atof(token);

    } else if(!strcmp("density", token)) {

      token = strtok(NULL, ",");
      def_prop.density = atof(token);

    } else if(!strcmp("gloss", token)) {

      token = strtok(NULL, ",");
      def_prop.gloss = atof(token);

    } else if(!strcmp("emission", token)) {

      token = strtok(NULL, ",");
      def_prop.emission = atof(token);

    } else if(!strcmp("ior", token)) {

      token = strtok(NULL, ",");
      def_prop.ior = atof(token);

    }
  }

  if(prop_num) {
    /* pack name */
    c = strlen(name) + 1;
    common_pack_write(app_data, app_ind, &c, sizeof(char));
    common_pack_write(app_data, app_ind, name, c);
    /* pack properties data */
    common_pack_write(app_data, app_ind, &def_prop, sizeof(common_prop_t));
  }


  size = *app_ind - marker - sizeof(int);
  common_pack_write(app_data, &marker, &size, sizeof(int));
  *app_ind = marker + size;
}


void common_pack_texture(void **app_data, int *app_ind, char *filename) {
  FILE *fh;
  char c, line[256], *token;
  short s;
  int marker, size;


  s = COMMON_PACK_TEXTURE;
  common_pack_write(app_data, app_ind, &s, sizeof(short));
  marker = *app_ind;
  *app_ind += sizeof(int);


  fh = fopen(filename, "r");
  if(!fh) {
    printf("Textures file: %s does not exist, exiting...\n", filename);
    exit(1);
  }


  while(fgets(line, 256, fh)) {
    token = strtok(line, ",");
    if(!strcmp("texture", token)) {
      token = strtok(NULL, ",");
      if(!strcmp("stack", token)) {
        s = TEXTURE_STACK;
        common_pack_write(app_data, app_ind, &s, sizeof(short));

        /* name */
        token = strtok(NULL, ",");
        if(token[strlen(token)-1] == '\n') token[strlen(token)-1] = 0;

        c = strlen(token) + 1;
        common_pack_write(app_data, app_ind, &c, sizeof(char));
        common_pack_write(app_data, app_ind, token, c);
      } else if(!strcmp("mix", token)) {
        tfloat coef;

        s = TEXTURE_MIX;
        common_pack_write(app_data, app_ind, &s, sizeof(short));

        /* name */
        token = strtok(NULL, ",");
        c = strlen(token) + 1;
        common_pack_write(app_data, app_ind, &c, sizeof(char));
        common_pack_write(app_data, app_ind, token, c);

        /* texture 1 */
        token = strtok(NULL, ",");
        c = strlen(token) + 1;
        common_pack_write(app_data, app_ind, &c, sizeof(char));
        common_pack_write(app_data, app_ind, token, c);

        /* texture 2 */
        token = strtok(NULL, ",");
        c = strlen(token) + 1;
        common_pack_write(app_data, app_ind, &c, sizeof(char));
        common_pack_write(app_data, app_ind, token, c);
/*
        sscanf(strstr(tag, "mode"), "mode=\"%[^\"]", ident);
        c = strlen(ident);
        common_pack_write(app_data, app_ind, &c, sizeof(char));
        common_pack_write(app_data, app_ind, ident, c);
*/

        /* coefficient */
        token = strtok(NULL, ",");
        if(token[strlen(token)-1] == '\n') token[strlen(token)-1] = 0;
        coef = atof(token);
        common_pack_write(app_data, app_ind, &coef, sizeof(tfloat));
      }
    } else if(!strcmp("blend", token)) {
      TIE_3 color1, color2;

      s = TEXTURE_BLEND;
      common_pack_write(app_data, app_ind, &s, sizeof(short));

      /* color 1 */
      token = strtok(NULL, ",");
      color1.v[0] = atof(token);
      token = strtok(NULL, ",");
      color1.v[1] = atof(token);
      token = strtok(NULL, ",");
      color1.v[2] = atof(token);

      /* color 2 */
      token = strtok(NULL, ",");
      color2.v[0] = atof(token);
      token = strtok(NULL, ",");
      color2.v[1] = atof(token);
      token = strtok(NULL, ",");
      if(token[strlen(token)-1] == '\n') token[strlen(token)-1] = 0;
      color2.v[2] = atof(token);

      common_pack_write(app_data, app_ind, &color1, sizeof(TIE_3));
      common_pack_write(app_data, app_ind, &color2, sizeof(TIE_3));
    } else if(!strcmp("bump", token)) {
      TIE_3 coef;

      s = TEXTURE_BUMP;
      common_pack_write(app_data, app_ind, &s, sizeof(short));

      /* coefficient */
      token = strtok(NULL, ",");
      coef.v[0] = atof(token);
      token = strtok(NULL, ",");
      coef.v[1] = atof(token);
      token = strtok(NULL, ",");
      if(token[strlen(token)-1] == '\n') token[strlen(token)-1] = 0;
      coef.v[2] = atof(token);

      common_pack_write(app_data, app_ind, &coef, sizeof(TIE_3));
    } else if(!strcmp("checker", token)) {
      int tile;

      s = TEXTURE_CHECKER;
      common_pack_write(app_data, app_ind, &s, sizeof(short));

      /* tile */
      token = strtok(NULL, ",");
      if(token[strlen(token)-1] == '\n') token[strlen(token)-1] = 0;
      tile = atoi(token);

      common_pack_write(app_data, app_ind, &tile, sizeof(int));
    } else if(!strcmp("camo", token)) {
      tfloat size;
      int octaves, absolute;
      TIE_3 color1, color2, color3;

      s = TEXTURE_CAMO;
      common_pack_write(app_data, app_ind, &s, sizeof(short));

      /* size */
      token = strtok(NULL, ",");
      size = atof(token);

      /* octaves */
      token = strtok(NULL, ",");
      octaves = atoi(token);

      /* absolute */
      token = strtok(NULL, ",");
      absolute = atoi(token);

      /* color 1 */
      token = strtok(NULL, ",");
      color1.v[0] = atof(token);
      token = strtok(NULL, ",");
      color1.v[1] = atof(token);
      token = strtok(NULL, ",");
      color1.v[2] = atof(token);

      /* color 2 */
      token = strtok(NULL, ",");
      color2.v[0] = atof(token);
      token = strtok(NULL, ",");
      color2.v[1] = atof(token);
      token = strtok(NULL, ",");
      color2.v[2] = atof(token);

      /* color 3 */
      token = strtok(NULL, ",");
      color3.v[0] = atof(token);
      token = strtok(NULL, ",");
      color3.v[1] = atof(token);
      token = strtok(NULL, ",");
      if(token[strlen(token)-1] == '\n') token[strlen(token)-1] = 0;
      color3.v[2] = atof(token);

      common_pack_write(app_data, app_ind, &size, sizeof(tfloat));
      common_pack_write(app_data, app_ind, &octaves, sizeof(int));
      common_pack_write(app_data, app_ind, &absolute, sizeof(int));
      common_pack_write(app_data, app_ind, &color1, sizeof(TIE_3));
      common_pack_write(app_data, app_ind, &color2, sizeof(TIE_3));
      common_pack_write(app_data, app_ind, &color3, sizeof(TIE_3));
    } else if(!strcmp("clouds", token)) {
      tfloat size;
      int octaves, absolute;
      TIE_3 scale, translate;

      s = TEXTURE_CLOUDS;
      common_pack_write(app_data, app_ind, &s, sizeof(short));

      /* size */
      token = strtok(NULL, ",");
      size = atof(token);

      /* octaves */
      token = strtok(NULL, ",");
      octaves = atoi(token);

      /* absolute */
      token = strtok(NULL, ",");
      absolute = atoi(token);

      /* scale */
      token = strtok(NULL, ",");
      scale.v[0] = atof(token);
      token = strtok(NULL, ",");
      scale.v[1] = atof(token);
      token = strtok(NULL, ",");
      scale.v[2] = atof(token);

      /* translate */
      token = strtok(NULL, ",");
      translate.v[0] = atof(token);
      token = strtok(NULL, ",");
      translate.v[1] = atof(token);
      token = strtok(NULL, ",");
      if(token[strlen(token)-1] == '\n') token[strlen(token)-1] = 0;
      translate.v[2] = atof(token);

      common_pack_write(app_data, app_ind, &size, sizeof(tfloat));
      common_pack_write(app_data, app_ind, &octaves, sizeof(int));
      common_pack_write(app_data, app_ind, &absolute, sizeof(int));
      common_pack_write(app_data, app_ind, &scale, sizeof(TIE_3));
      common_pack_write(app_data, app_ind, &translate, sizeof(TIE_3));
    } else if(!strcmp("image", token)) {
/*
      char file[64];
      image = SDL_LoadBMP(file);
      if(image) {
        s = TEXTURE_IMAGE;
        common_pack_write(app_data, app_ind, &s, sizeof(short));
        common_pack_write(app_data, app_ind, &(image->w), sizeof(short));
        common_pack_write(app_data, app_ind, &(image->h), sizeof(short));
        common_pack_write(app_data, app_ind, image->pixels, 3*image->w*image->h);
      }
*/
    } else if(!strcmp("gradient", token)) {
      int axis;

      s = TEXTURE_GRADIENT;
      common_pack_write(app_data, app_ind, &s, sizeof(short));

      /* axis */
      token = strtok(NULL, ",");
      if(token[strlen(token)-1] == '\n') token[strlen(token)-1] = 0;
      axis = atoi(token);

      common_pack_write(app_data, app_ind, &axis, sizeof(int));
    }
  }

  fclose(fh);


  size = *app_ind - marker - sizeof(int);
  common_pack_write(app_data, &marker, &size, sizeof(int));
  *app_ind = marker + size;
}


void common_pack_mesh(common_db_t *db, void **app_data, int *app_ind, char *filename) {
  common_pack_mesh_adrt(db, app_data, app_ind, filename);
}


void common_pack_mesh_adrt(common_db_t *db, void **app_data, int *app_ind, char *filename) {
  FILE *fh;
  TIE_3 v;
  char c, meshname[256], texturename[256];
  short s, endian;
  int face[3], marker_size, marker_trinum, size, i, j, k, n, num, matrixind, end;


  fh = fopen(filename, "rb");
  if(!fh) {
    printf("ADRT geometry file: %s does not exist, exiting...\n", filename);
    exit(1);
  }

  s = COMMON_PACK_MESH;
  common_pack_write(app_data, app_ind, &s, sizeof(short));

  /* Marker for size of all mesh data */
  marker_size = *app_ind;
  *app_ind += sizeof(int);

  /* Marker for total number of triangles */
  marker_trinum = *app_ind;
  *app_ind += sizeof(int);

  /* Get End Position */
  fseek(fh, 0, SEEK_END);
  end = ftell(fh);
  fseek(fh, 0, SEEK_SET);

  /* Check Endian */
  fread(&endian, sizeof(short), 1, fh);
  endian = endian == 1 ? 0 : 1;

  /* Check Geometry Revision */
  fread(&s, sizeof(short), 1, fh);

  while(ftell(fh) != end) {
    /* Mesh Name */
    fread(&c, sizeof(char), 1, fh);
    fread(meshname, sizeof(char), c, fh);
    meshname[(int)(c++)] = 0;
    common_pack_write(app_data, app_ind, &c, sizeof(char));
    common_pack_write(app_data, app_ind, meshname, c);

    /* Texture/Properties Name */
    fread(&c, sizeof(char), 1, fh);
    fread(texturename, sizeof(char), c, fh);
    texturename[(int)(c++)] = 0;
    common_pack_write(app_data, app_ind, &c, sizeof(char));
    common_pack_write(app_data, app_ind, texturename, c);

    /* Pack Number of Vertices */
    fread(&num, sizeof(int), 1, fh);
    if(endian) tienet_flip(&num, &num, sizeof(int));
    common_pack_write(app_data, app_ind, &num, sizeof(int));

    /* Pack Vertices */
    for(i = 0; i < num; i++) {
      fread(&v, sizeof(TIE_3), 1, fh);
      common_pack_write(app_data, app_ind, &v, sizeof(TIE_3));
    }

    /* Pack Number of Faces */
    fread(&num, sizeof(int), 1, fh);
    if(endian) tienet_flip(&num, &num, sizeof(int));
    common_pack_write(app_data, app_ind, &num, sizeof(int));
    common_pack_trinum += num;

    /* Pack Faces */
    for(i = 0; i < num; i++) {
      fread(face, sizeof(int), 3, fh);
      common_pack_write(app_data, app_ind, face, sizeof(int) * 3);
    }

    /* Determine if Mesh has a Transformation Matrix assigned to it */
    matrixind = -1;
    for(n = 0; n < db->anim.frame_list[0].tnum; n++)
      if(!strcmp(meshname, db->anim.frame_list[0].tlist[n].mesh_name))
        matrixind = n;

    /* Write Matrix */
    if(matrixind >= 0) {
      common_pack_write(app_data, app_ind, db->anim.frame_list[0].tlist[matrixind].matrix, sizeof(tfloat)*16);
    } else{
      tfloat matrix[16];
      math_mat_ident(matrix, 4);
      common_pack_write(app_data, app_ind, matrix, sizeof(tfloat)*16);
    }
  }

  fclose(fh);

  /* pack the total number of triangles */
  common_pack_write(app_data, &marker_trinum, &common_pack_trinum, sizeof(int));

  /* pack the size of the mesh data */
  size = *app_ind - marker_size - sizeof(int); /* make sure you're not counting the data size integer */
  common_pack_write(app_data, &marker_size, &size, sizeof(int));

  *app_ind = marker_size + size;
}
