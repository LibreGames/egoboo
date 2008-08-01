//********************************************************************************************
//* Egoboo - ogl_include.c
//*
//* Implements the most basic code that Egoboo uses to interface with OpenGL
//*
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

#include "ogl_include.h"

#include "Log.h"
#include "egoboo_math.inl"

//--------------------------------------------------------------------------------------------
// MN This probably should be replaced by a call to gluLookAt, don't see why we need to make our own...
GLmatrix ViewMatrix( const vect3 from,     // camera location
                     const vect3 at,        // camera look-at target
                     const vect3 world_up,  // world�s up, usually 0, 0, 1
                     const float roll )     // clockwise roll around viewing direction, in radians
{

  GLmatrix view;

  ATTRIB_PUSH( "ViewMatrix", GL_TRANSFORM_BIT );
  {
    glMatrixMode( GL_MODELVIEW );
    glPushMatrix();
    glLoadIdentity();
    glScalef( -1, 1, 1 );

    if ( roll > .001 )
    {
      matrix_4x4 stupid_intermediate_matrix = RotateZ( roll );
      glMultMatrixf( stupid_intermediate_matrix.v );
    }

    gluLookAt( from.x, from.y, from.z, at.x, at.y, at.z, world_up.x, world_up.y, world_up.z );

    glGetFloatv( GL_MODELVIEW_MATRIX, view.v );

    glPopMatrix();
  }
  ATTRIB_POP( "ViewMatrix" );

  return view;
}

//--------------------------------------------------------------------------------------------
GLmatrix ProjectionMatrix( const float near_plane,    // distance to near clipping plane
                           const float far_plane,      // distance to far clipping plane
                           const float fov )           // field of view angle, in radians
{
  GLmatrix proj;

  ATTRIB_PUSH( "ProjectionMatrix", GL_TRANSFORM_BIT );
  {
    GLint viewport[ 4 ];

    //use OpenGl to create our projection matrix
    glGetIntegerv( GL_VIEWPORT, viewport );

    glMatrixMode( GL_PROJECTION );
    glPushMatrix();
    glLoadIdentity();
    gluPerspective( fov, ( float )( viewport[ 2 ] - viewport[ 0 ] ) / ( float )( viewport[ 3 ] - viewport[ 1 ] ), near_plane, far_plane );
    glGetFloatv( GL_PROJECTION_MATRIX, proj.v );
    glPopMatrix();
  }
  ATTRIB_POP( "ProjectionMatrix" );

  return proj;
}