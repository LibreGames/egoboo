/* Egoboo - Md2.h
 * This code is not currently in use.
 */

//********************************************************************************************
//*
//*    This file is part of Egoboo.
//*
//*    Egoboo is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Egoboo is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

/* Adapted from "Tactics - MD2_Model.h" by Jonathan Fischer
 * A class for loading/using Quake 2 and Egoboo md2 models.
 *
 * Creating/destroying objects of this class is done in the same fashion as
 * Textures, so see Texture.h for details.
 */

#pragma once

#include "id_md2.h"
#include <SDL_opengl.h>
#include <memory.h>

#pragma pack(push,1)
  typedef struct ego_md2_vertex_t
  {
    float x, y, z;
    unsigned normal;  // index to id-normal array
  } MD2_Vertex;

  typedef struct ego_md2_texcoord_t
  {
    float s, t;
  } MD2_TexCoord;

  typedef struct ego_md2_frame_t
  {
    char name[16];
    float bbmin[3], bbmax[3];    // axis-aligned bounding box limits
    MD2_Vertex *vertices;
  } MD2_Frame;
#pragma pack(pop)

typedef md2_triangle MD2_Triangle;
typedef md2_skinname MD2_SkinName;

struct ego_md2_glcommand_t
{
  struct ego_md2_glcommand_t * next;
  GLenum          gl_mode;
  signed int      command_count;
  md2_gldata    * data;
};
typedef struct ego_md2_glcommand_t MD2_GLCommand;

void MD2_GLCommand_construct(MD2_GLCommand * m);
void MD2_GLCommand_destruct(MD2_GLCommand * m);

MD2_GLCommand * MD2_GLCommand_new();
MD2_GLCommand * MD2_GLCommand_new_vector(int n);
void MD2_GLCommand_delete(MD2_GLCommand * m);
void MD2_GLCommand_delete_vector(MD2_GLCommand * v, int n);


struct ego_md2_model_t;

void md2_construct(struct ego_md2_model_t * m);
void md2_destruct(struct ego_md2_model_t * m);

struct ego_md2_model_t * md2_new();
struct ego_md2_model_t * md2_new_vector(int n);
void md2_delete(struct ego_md2_model_t * m);
void md2_delete_vector(struct ego_md2_model_t * v, int n);


struct ego_md2_model_t * md2_load(const char * szFilename, struct ego_md2_model_t* m);

void md2_deallocate(struct ego_md2_model_t * m);



//char * rip_md2_frame_name( struct ego_md2_model_t * m, int frame );
void md2_scale_model(struct ego_md2_model_t * pmd2, float scale);

INLINE const int md2_get_numVertices(struct ego_md2_model_t * m);
INLINE const int md2_get_numTexCoords(struct ego_md2_model_t * m);
INLINE const int md2_get_numTriangles(struct ego_md2_model_t * m);
INLINE const int md2_get_numSkins(struct ego_md2_model_t * m);
INLINE const int md2_get_numFrames(struct ego_md2_model_t * m);

INLINE const MD2_SkinName  *md2_get_SkinNames(struct ego_md2_model_t * m);
INLINE const MD2_TexCoord  *md2_get_TexCoords(struct ego_md2_model_t * m);
INLINE const MD2_Triangle  *md2_get_Triangles(struct ego_md2_model_t * m);
INLINE const MD2_Frame     *md2_get_Frames   (struct ego_md2_model_t * m);
INLINE const MD2_GLCommand *md2_get_Commands (struct ego_md2_model_t * m);

INLINE const MD2_SkinName  *md2_get_Skin     (struct ego_md2_model_t * m, int index);
INLINE const MD2_Frame     *md2_get_Frame    (struct ego_md2_model_t * m, int index);
INLINE const MD2_Triangle  *md2_get_Triangle (struct ego_md2_model_t * m, int index);
