#pragma once

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

///
/// @file
/// @brief Md2 Model display routines
/// @details Adapted from "Tactics - MD2_Model.h" by Jonathan Fischer
///   A class for loading/using Quake 2 and Egoboo md2 models.
///   Creating/destroying objects of this class is done in the same fashion as
///   Textures, so see Texture.h for details.

#include "id_md2.h"
#include <SDL_opengl.h>
#include <memory.h>

#pragma pack(push,1)
  typedef struct s_ego_md2_vertex
  {
    float x, y, z;
    unsigned normal;  ///< index to id-normal array
  } MD2_Vertex_t;

  typedef struct s_ego_md2_texcoord
  {
    float s, t;
  } MD2_TexCoord_t;

  typedef struct s_ego_md2_frame
  {
    char name[16];
    float bbmin[3], bbmax[3];    ///< axis-aligned bounding box limits
    MD2_Vertex_t *vertices;
  } MD2_Frame_t;
#pragma pack(pop)

typedef md2_triangle MD2_Triangle_t;
typedef md2_skinname MD2_SkinName_t;

struct s_ego_md2_glcommand
{
  struct s_ego_md2_glcommand * next;
  GLenum          gl_mode;
  signed int      command_count;
  md2_gldata    * data;
};
typedef struct s_ego_md2_glcommand MD2_GLCommand_t;

void MD2_GLCommand_construct(MD2_GLCommand_t * m);
void MD2_GLCommand_destruct(MD2_GLCommand_t * m);

MD2_GLCommand_t * MD2_GLCommand_new( void );
MD2_GLCommand_t * MD2_GLCommand_new_vector(int n);
void MD2_GLCommand_delete(MD2_GLCommand_t * m);
void MD2_GLCommand_delete_vector(MD2_GLCommand_t * v, int n);


struct s_ego_md2_model
{
  int m_numVertices;
  int m_numTexCoords;
  int m_numTriangles;
  int m_numSkins;
  int m_numFrames;
  int m_numCommands;

  MD2_SkinName_t  *m_skins;
  MD2_TexCoord_t  *m_texCoords;
  MD2_Triangle_t  *m_triangles;
  MD2_Frame_t     *m_frames;
  MD2_GLCommand_t *m_commands;
};
typedef struct s_ego_md2_model MD2_Model_t;

void md2_construct(MD2_Model_t * m);
void md2_destruct(MD2_Model_t * m);

MD2_Model_t * md2_new( void );
MD2_Model_t * md2_new_vector(int n);
void md2_delete(MD2_Model_t * m);
void md2_delete_vector(MD2_Model_t * v, int n);


MD2_Model_t * md2_load(EGO_CONST char * szFilename, MD2_Model_t* m);

void md2_deallocate(MD2_Model_t * m);

//char * rip_md2_frame_name( MD2_Model_t * m, int frame );
void md2_scale_model(MD2_Model_t * pmd2, float scale);

INLINE EGO_CONST int md2_get_numVertices(MD2_Model_t * m)  { return m->m_numVertices; }
INLINE EGO_CONST int md2_get_numTexCoords(MD2_Model_t * m) { return m->m_numTexCoords; }
INLINE EGO_CONST int md2_get_numTriangles(MD2_Model_t * m) { return m->m_numTriangles; }
INLINE EGO_CONST int md2_get_numSkins(MD2_Model_t * m)     { return m->m_numSkins; }
INLINE EGO_CONST int md2_get_numFrames(MD2_Model_t * m)    { return m->m_numFrames; }

INLINE EGO_CONST MD2_SkinName_t  *md2_get_SkinNames(MD2_Model_t * m) { return m->m_skins;     }
INLINE EGO_CONST MD2_TexCoord_t  *md2_get_TexCoords(MD2_Model_t * m) { return m->m_texCoords; }
INLINE EGO_CONST MD2_Triangle_t  *md2_get_Triangles(MD2_Model_t * m) { return m->m_triangles; }
INLINE EGO_CONST MD2_Frame_t     *md2_get_Frames   (MD2_Model_t * m) { return m->m_frames;    }
INLINE EGO_CONST MD2_GLCommand_t *md2_get_Commands (MD2_Model_t * m) { return m->m_commands;  }
