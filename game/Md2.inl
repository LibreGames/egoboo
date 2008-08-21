#pragma once

#include "Md2.h"


INLINE EGO_CONST int md2_get_numVertices(MD2_Model_t * m)  { return m->m_numVertices; }
INLINE EGO_CONST int md2_get_numTexCoords(MD2_Model_t * m) { return m->m_numTexCoords; }
INLINE EGO_CONST int md2_get_numTriangles(MD2_Model_t * m) { return m->m_numTriangles; }
INLINE EGO_CONST int md2_get_numSkins(MD2_Model_t * m)     { return m->m_numSkins; }
INLINE EGO_CONST int md2_get_numFrames(MD2_Model_t * m)    { return m->m_numFrames; }

INLINE EGO_CONST MD2_SkinName_t  *md2_get_Skin    (MD2_Model_t * m, int index);
INLINE EGO_CONST MD2_Frame_t     *md2_get_Frame   (MD2_Model_t * m, int index);
INLINE EGO_CONST MD2_Triangle_t  *md2_get_Triangle(MD2_Model_t * m, int index);

INLINE EGO_CONST MD2_SkinName_t  *md2_get_SkinNames(MD2_Model_t * m) { return m->m_skins;     }
INLINE EGO_CONST MD2_TexCoord_t  *md2_get_TexCoords(MD2_Model_t * m) { return m->m_texCoords; }
INLINE EGO_CONST MD2_Triangle_t  *md2_get_Triangles(MD2_Model_t * m) { return m->m_triangles; }
INLINE EGO_CONST MD2_Frame_t     *md2_get_Frames   (MD2_Model_t * m) { return m->m_frames;    }
INLINE EGO_CONST MD2_GLCommand_t *md2_get_Commands (MD2_Model_t * m) { return m->m_commands;  }


INLINE EGO_CONST MD2_SkinName_t *md2_get_Skin(MD2_Model_t * m, int index)
{
  if(index >= 0 && index < m->m_numSkins)
  {
    return m->m_skins + index;
  }
  return NULL;
}

INLINE EGO_CONST MD2_Frame_t *md2_get_Frame(MD2_Model_t * m, int index)
{
  if(index >= 0 && index < m->m_numFrames)
  {
    return m->m_frames + index;
  }
  return NULL;
}

INLINE EGO_CONST MD2_Triangle_t  *md2_get_Triangle(MD2_Model_t * m, int index)
{
  if(index >= 0 && index < m->m_numTriangles)
  {
    return m->m_triangles + index;
  }
  return NULL;
}
