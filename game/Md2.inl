#pragma once

#include "Md2.h"

INLINE const int md2_get_numVertices(MD2_Model * m)  { return m->m_numVertices; }
INLINE const int md2_get_numTexCoords(MD2_Model * m) { return m->m_numTexCoords; }
INLINE const int md2_get_numTriangles(MD2_Model * m) { return m->m_numTriangles; }
INLINE const int md2_get_numSkins(MD2_Model * m)     { return m->m_numSkins; }
INLINE const int md2_get_numFrames(MD2_Model * m)    { return m->m_numFrames; }

INLINE const MD2_SkinName  *md2_get_Skin    (MD2_Model * m, int index);
INLINE const MD2_Frame     *md2_get_Frame   (MD2_Model * m, int index);
INLINE const MD2_Triangle  *md2_get_Triangle(MD2_Model * m, int index);

INLINE const MD2_SkinName  *md2_get_SkinNames(MD2_Model * m) { return m->m_skins;     }
INLINE const MD2_TexCoord  *md2_get_TexCoords(MD2_Model * m) { return m->m_texCoords; }
INLINE const MD2_Triangle  *md2_get_Triangles(MD2_Model * m) { return m->m_triangles; }
INLINE const MD2_Frame     *md2_get_Frames   (MD2_Model * m) { return m->m_frames;    }
INLINE const MD2_GLCommand *md2_get_Commands (MD2_Model * m) { return m->m_commands;  }


INLINE const MD2_SkinName *md2_get_Skin(MD2_Model * m, int index)
{
  if(index >= 0 && index < m->m_numSkins)
  {
    return m->m_skins + index;
  }
  return NULL;
}

INLINE const MD2_Frame *md2_get_Frame(MD2_Model * m, int index)
{
  if(index >= 0 && index < m->m_numFrames)
  {
    return m->m_frames + index;
  }
  return NULL;
}

INLINE const MD2_Triangle  *md2_get_Triangle(MD2_Model * m, int index)
{
  if(index >= 0 && index < m->m_numTriangles)
  {
    return m->m_triangles + index;
  }
  return NULL;
}