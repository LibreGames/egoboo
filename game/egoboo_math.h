/* Egoboo - mathstuff.h
 * The name's pretty self explanatory, doncha think?
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

#pragma once

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
#define UINT32_SIZE         (1<<32)
#endif

#ifndef UINT32_MAX
#define UINT32_MAX          ((1<<32)-1)
#endif

#ifndef UINT16_SIZE
#define UINT16_SIZE         (1<<16)
#endif

#ifndef UINT16_MAX
#define UINT16_MAX          ((1<<16)-1)
#endif

#ifndef UINT8_SIZE
#define UINT8_SIZE          (1<< 8)
#endif

#ifndef UINT8_MAX
#define UINT8_MAX           ((1<< 8)-1)
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
  typedef union vector4_t { float v[4]; struct { float x, y, z, w; }; struct { float r, g, b, a; }; } vect4;
  typedef vect4 quaternion;
#pragma pack(pop)

/**> GLOBAL VARIABLES <**/
#define TRIGTABLE_SIZE (1<<14)
#define TRIGTABLE_MASK (TRIGTABLE_SIZE-1)
#define TRIGTABLE_SHIFT (TRIGTABLE_SIZE>>2)       // TRIGTABLE_SIZE/4 == TWO_PI/4 == PI_OVER_2

extern float turntosin[TRIGTABLE_SIZE];           // Convert chrturn>>2...  to sine
extern float turntocos[TRIGTABLE_SIZE];           // Convert chrturn>>2...  to cosine

/**> FUNCTION PROTOTYPES <**/
void make_turntosin( void );
matrix_4x4 ViewMatrix( const vect3 from, const vect3 at, const vect3 world_up, const float roll );
matrix_4x4 ProjectionMatrix( const float near_plane, const float far_plane, const float fov );

/**> 3 component vector <**/
INLINE vect3 VSub        ( vect3 A, vect3 B );
INLINE vect3 Normalize   ( vect3 A );
INLINE vect3 CrossProduct( vect3 A, vect3 B );
INLINE float DotProduct  ( vect3 A, vect3 B );

/**> 4 component vector <**/
INLINE vect4 VSub4        ( vect4 A, vect4 B );
INLINE vect4 Normalize4   ( vect4 A );
INLINE vect4 CrossProduct4( vect4 A, vect4 B );
INLINE float DotProduct4  ( vect4 A, vect4 B );

/**> quaternion <**/
INLINE quaternion QuatConjugate(quaternion q1);
INLINE quaternion QuatNormalize(quaternion q1);
INLINE quaternion QuatMultiply(quaternion q1, quaternion q2);
INLINE quaternion QuatDotprod(quaternion q1, quaternion q2);
INLINE quaternion QuatTransform(quaternion q1, quaternion q2, quaternion q3);
INLINE quaternion QuatConvert(matrix_4x4 m);

/**> 4x4 matrix <**/
INLINE matrix_4x4 IdentityMatrix( void );
INLINE matrix_4x4 ZeroMatrix( void );
INLINE matrix_4x4 MatrixTranspose( const matrix_4x4 a );
INLINE matrix_4x4 MatrixMult( const matrix_4x4 a, const matrix_4x4 b );
INLINE matrix_4x4 Translate( const float dx, const float dy, const float dz );
INLINE matrix_4x4 RotateX( const float rads );
INLINE matrix_4x4 RotateY( const float rads );
INLINE matrix_4x4 RotateZ( const float rads );
INLINE matrix_4x4 ScaleXYZ( const float sizex, const float sizey, const float sizez );
INLINE matrix_4x4 ScaleXYZRotateXYZTranslate( const float sizex, const float sizey, const float sizez, Uint16 turnz, Uint16 turnx, Uint16 turny, float tx, float ty, float tz );
INLINE matrix_4x4 FourPoints( vect4 ori, vect4 wid, vect4 forw, vect4 up, float scale );
INLINE matrix_4x4 MatrixConvert(quaternion q1);

INLINE void Transform4_Full( float pre_scale, float post_scale, matrix_4x4 *pMatrix, vect4 pSourceV[], vect4 pDestV[], Uint32 NumVertor );
INLINE void Transform4( float pre_scale, float post_scale, matrix_4x4 *pMatrix, vect4 pSourceV[], vect4 pDestV[], Uint32 NumVertor );
INLINE void Translate4( matrix_4x4 *pMatrix, vect4 pSourceV[], vect4 pDestV[], Uint32 NumVertor );

INLINE void Transform3_Full( float pre_scale, float post_scale, matrix_4x4 *pMatrix, vect3 pSourceV[], vect3 pDestV[], Uint32 NumVertor );
INLINE void Transform3( float pre_scale, float post_scale, matrix_4x4 *pMatrix, vect3 pSourceV[], vect3 pDestV[], Uint32 NumVertor );
INLINE void Translate3( matrix_4x4 *pMatrix, vect3 pSourceV[], vect3 pDestV[], Uint32 NumVertor );

INLINE Uint16 vec_to_turn( float dx, float dy );
INLINE void turn_to_vec( Uint16 turn, float * dx, float * dy );

INLINE void VectorClear( float v[] );
INLINE void VectorClear4( float v[] );

INLINE bool_t matrix_compare_3x3(matrix_4x4 * pm1, matrix_4x4 * pm2);


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

INLINE const BBOX_LIST * bbox_list_new(BBOX_LIST * lst);
INLINE const BBOX_LIST * bbox_list_delete(BBOX_LIST * lst);
INLINE const BBOX_LIST * bbox_list_renew(BBOX_LIST * lst);
INLINE const BBOX_LIST * bbox_list_alloc(BBOX_LIST * lst, int count);
INLINE const BBOX_LIST * bbox_list_realloc(BBOX_LIST * lst, int count);

//--------------------------------------------------------------------------------------------
struct s_bbox_array
{
  int         count;
  BBOX_LIST * list;
};
typedef struct s_bbox_array BBOX_ARY;

INLINE const BBOX_ARY * bbox_ary_new(BBOX_ARY * ary);
INLINE const BBOX_ARY * bbox_ary_delete(BBOX_ARY * ary);
INLINE const BBOX_ARY * bbox_ary_renew(BBOX_ARY * ary);
INLINE const BBOX_ARY * bbox_ary_alloc(BBOX_ARY * ary, int count);

INLINE Uint32 ego_rand(Uint32 * seed);
