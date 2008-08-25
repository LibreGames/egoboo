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
/// @brief Header files for math operations.
/// @details The name's pretty self explanatory, doncha think?

/**> HEADER FILES <**/
#include <math.h>

#include "egoboo_types.h"
#include "egoboo_config.h"

#define HAS_SOME_BITS(XX,YY) (0 != ((XX)&(YY)))
#define HAS_ALL_BITS(XX,YY)  ((YY) == ((XX)&(YY)))
#define HAS_NO_BITS(XX,YY)   (0 == ((XX)&(YY)))
#define MISSING_BITS(XX,YY)  (HAS_SOME_BITS(XX,YY) && !HAS_ALL_BITS(XX,YY))

#define SQRT_TWO            1.4142135623730950488016887242097f
#define INV_SQRT_TWO        0.70710678118654752440084436210485f
#define PI                  3.1415926535897932384626433832795f
#define TWO_PI              6.283185307179586476925286766559f
#define INV_TWO_PI          0.15915494309189533576888376337251f
#define PI_OVER_TWO         1.5707963267948966192313216916398f
#define PI_OVER_FOUR        0.78539816339744830961566084581988f
#define SHORT_TO_RAD        (TWO_PI / (float)(1<<16))
#define RAD_TO_SHORT        ((float)(1<<16) / TWO_PI)
#define RAD_TO_BYTE         ((float)(1<< 8) / TWO_PI)
#define DEG_TO_RAD          0.017453292519943295769236907684886f
#define RAD_TO_DEG          57.295779513082320876798154814105

#ifndef UINT32_SIZE
#define UINT32_SIZE         0x100000000
#endif

#ifndef UINT32_MAX
#define UINT32_MAX          0xFFFFFFFF
#endif

#ifndef SINT32_MAX
#define SINT32_MAX          0x7FFFFFFF
#endif

#ifndef UINT16_SIZE
#define UINT16_SIZE         0x10000
#endif

#ifndef UINT16_MAX
#define UINT16_MAX          0xFFFF
#endif

#ifndef SINT16_MAX
#define SINT16_MAX          0x7FFF
#endif

#ifndef UINT8_SIZE
#define UINT8_SIZE          0x100
#endif

#ifndef UINT8_MAX
#define UINT8_MAX           0xFF
#endif

#ifndef SINT8_MAX
#define SINT8_MAX           0x7F
#endif

#define RAD_TO_TURN(XX)     ((Uint16)(((XX) + PI) * RAD_TO_SHORT))
#define TURN_TO_RAD(XX)     (((float)(XX))*SHORT_TO_RAD - PI)

#define FP8_TO_FLOAT(XX)   ( (float)(XX)/(float)(1<<8) )
#define FLOAT_TO_FP8(XX)   ( (Uint32)((XX)*(float)(1<<8)) )

#define FP8_TO_INT(XX)     ( (XX) >> 8 )                      // fast version of XX / 256
#define INT_TO_FP8(XX)     ( (XX) << 8 )                      // fast version of XX * 256
#define FP8_MUL(XX, YY)    ( ((XX)*(YY)) >> 8 )
#define FP8_DIV(XX, YY)    ( ((XX)<<8) / (YY) )

/* Neither Linux nor Mac OS X seem to have MIN and MAX defined, so if they
 * haven't already been found, define them here. */
#ifndef MAX
#define MAX(a,b) ( ((a)>(b))? (a):(b) )
#endif

#ifndef MIN
#define MIN(a,b) ( ((a)>(b))? (b):(a) )
#endif

#ifndef SGN
#define SGN(a) ( ((a)>0) ? 1 : -1 )
#endif

#ifndef CLIP
#define CLIP(A, B, C) MIN(MAX(A,B),C)
#endif

#ifndef ABS
#define ABS(X)  (((X) > 0) ? (X) : -(X))
#endif


/**> MACROS <**/
#define CNV(i,j) v[4*i+j]
#define CopyMatrix( pMatrixSource, pMatrixDest ) memcpy( (pMatrixDest), (pMatrixSource), sizeof( matrix_4x4 ) )

#define INT_TO_BOOL(XX) (0!=(XX))

/**> DATA STRUCTURES <**/
#pragma pack(push,1)
  typedef struct matrix_4x4_t { float v[16]; } matrix_4x4;
  typedef union vector2_t { float _v[2]; struct { float x, y; }; struct { float u, v; }; struct { float s, t; }; } vect2;
  typedef union vector3_t { float v[3]; struct { float x, y, z; }; struct { float r, g, b; }; } vect3;
  typedef union vector3_ui08_t { Uint8 v[3]; struct { Uint8 x, y, z; }; struct { Uint8 r, g, b; }; } vect3_ui08;
  typedef union vector3_ui16_t { Uint16 v[3]; struct { Uint16 x, y, z; }; struct { Uint16 r, g, b; }; } vect3_ui16;
  typedef union vector3_ui32_t { Uint32 v[3]; struct { Uint32 x, y, z; }; struct { Uint32 r, g, b; }; } vect3_ui32;
  typedef union vector3_si32_t { Sint32 v[3]; struct { Sint32 x, y, z; }; struct { Sint32 r, g, b; }; } vect3_si32;
  typedef union vector4_t { float v[4]; struct { float x, y, z, w; }; struct { float r, g, b, a; }; } vect4;
  typedef vect4 quaternion;
#pragma pack(pop)

#define ZERO_VECT2 { {0,0} }
#define ZERO_VECT3 { {0,0,0} }
#define ZERO_VECT4 { {0,0,0,0} }

#define VECT2(XX,YY) { {XX,YY} }
#define VECT3(XX,YY,ZZ) { {XX,YY,ZZ} }
#define VECT4(XX,YY,ZZ,WW) { {XX,YY,ZZ,WW} }

/**> GLOBAL VARIABLES <**/
#define TRIGTABLE_SIZE (1<<14)
#define TRIGTABLE_MASK (TRIGTABLE_SIZE-1)
#define TRIGTABLE_SHIFT (TRIGTABLE_SIZE>>2)       // TRIGTABLE_SIZE/4 == TWO_PI/4 == PI_OVER_2

extern float turntosin[TRIGTABLE_SIZE];           ///< Convert chrturn>>2...  to sine
extern float turntocos[TRIGTABLE_SIZE];           ///< Convert chrturn>>2...  to cosine

/**> FUNCTION PROTOTYPES <**/
void make_turntosin( void );
matrix_4x4 ViewMatrix( EGO_CONST vect3 from, EGO_CONST vect3 at, EGO_CONST vect3 world_up, EGO_CONST float roll );
matrix_4x4 ProjectionMatrix( EGO_CONST float near_plane, EGO_CONST float far_plane, EGO_CONST float fov );

//--------------------------------------------------------------------------------------------
struct s_aa_bbox
{
  int    sub_used;
  float  weight;

  bool_t used;
  int    level;
  int    address;

  vect3  mins;
  vect3  maxs;
};

typedef struct s_aa_bbox AA_BBOX;

//--------------------------------------------------------------------------------------------
struct s_bbox_list
{
  int       count;
  AA_BBOX * list;
};
typedef struct s_bbox_list BBOX_LIST;

//--------------------------------------------------------------------------------------------
struct s_bbox_array
{
  int         count;
  BBOX_LIST * list;
};
typedef struct s_bbox_array BBOX_ARY;
