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
/// @brief 
/// @details functions that will be declared inside the base class

#include "graphic.h"
#include "egoboo_types.inl"

INLINE EGO_CONST bool_t bbox_gl_draw(AA_BBOX * pbbox);

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
