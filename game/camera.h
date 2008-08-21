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
/// @brief The camera object.

#include "ogl_include.h"

#define FOV                             40           ///< Field of view
#define CAMKEYTURN                      5            ///< Keyboard camera rotation
#define CAMJOYTURN                      5            ///< Joystick camera rotation

// Multi cam
#define MINZOOM                         100          ///< Camera distance
#define MAXZOOM                         600         //
#define MINZADD                         100          ///< Camera height
#define MAXZADD                         3000

//Camera control stuff

struct sCamera
{
  bool_t    exploremode;     ///< Explore mode?

  vect3     pos;             ///< Camera position
  float     zoom;            ///< Distance from the trackee
  vect3     trackvel;        ///< Change in trackee position
  vect3     centerpos;       ///< Move character to side before tracking
  vect3     trackpos;        ///< Trackee position
  float     tracklevel;      //
  float     zadd;            ///< Camera height above terrain
  float     zaddgoto;        ///< Desired z position
  float     zgoto;           //
  float     turn_lr;         ///< Camera rotations
  float     turn_lr_one;
  float     turnadd;         ///< Turning rate
  float     sustain;         ///< Turning rate falloff
  float     roll;            //

  int       swing;           ///< Camera swingin'
  int       swingrate;       //
  float     swingamp;        //

  // for floor reflections
  float     tracklevel_stt;
  bool_t    tracklevel_stt_valid;


  GLmatrix  mView;           ///< View Matrix
  GLmatrix  mProjection;     ///< Projection Matrix
  GLmatrix  mProjectionBig;  ///< Larger projection matrix for frustum culling
};

typedef struct sCamera Camera_t;

extern Camera_t GCamera;

//extern float                   cornerx[4];                 ///< Render area corners
//extern float                   cornery[4];                 //
//extern int                     cornerlistlowtohighy[4];    ///< Ordered list
//extern int                     cornerlowx;                 ///< Render area extremes
//extern int                     cornerhighx;                //
//extern int                     cornerlowy;                 //
//extern int                     cornerhighy;                //

void camera_move( float dUpdate );
void camera_reset( void );
