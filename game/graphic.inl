#pragma once

#include "graphic.h"
#include "egoboo_types.inl"

//--------------------------------------------------------------------------------------------
INLINE EGO_CONST bool_t bbox_gl_draw(AA_BBOX * pbbox)
{
  vect3 * pmin, * pmax;

  if(NULL == pbbox) return bfalse;

  glPushMatrix();
  {
    pmin = &(pbbox->mins);
    pmax = &(pbbox->maxs);

    // !!!! there must be an optimized way of doing this !!!!

    glBegin(GL_QUADS);
    {
      // Front Face
      glVertex3f(pmin->x, pmin->y, pmax->z);
      glVertex3f(pmax->x, pmin->y, pmax->z);
      glVertex3f(pmax->x, pmax->y, pmax->z);
      glVertex3f(pmin->x, pmax->y, pmax->z);

      // Back Face
      glVertex3f(pmin->x, pmin->y, pmin->z);
      glVertex3f(pmin->x, pmax->y, pmin->z);
      glVertex3f(pmax->x, pmax->y, pmin->z);
      glVertex3f(pmax->x, pmin->y, pmin->z);

      // Top Face
      glVertex3f(pmin->x, pmax->y, pmin->z);
      glVertex3f(pmin->x, pmax->y, pmax->z);
      glVertex3f(pmax->x, pmax->y, pmax->z);
      glVertex3f(pmax->x, pmax->y, pmin->z);

      // Bottom Face
      glVertex3f(pmin->x, pmin->y, pmin->z);
      glVertex3f(pmax->x, pmin->y, pmin->z);
      glVertex3f(pmax->x, pmin->y, pmax->z);
      glVertex3f(pmin->x, pmin->y, pmax->z);

      // Right face
      glVertex3f(pmax->x, pmin->y, pmin->z);
      glVertex3f(pmax->x, pmax->y, pmin->z);
      glVertex3f(pmax->x, pmax->y, pmax->z);
      glVertex3f(pmax->x, pmin->y, pmax->z);

      // Left Face
      glVertex3f(pmin->x, pmin->y, pmin->z);
      glVertex3f(pmin->x, pmin->y, pmax->z);
      glVertex3f(pmin->x, pmax->y, pmax->z);
      glVertex3f(pmin->x, pmax->y, pmin->z);
    }
    glEnd();
  }
  glPopMatrix();

  return btrue;
}
