//********************************************************************************************
//* Egoboo - egoboo_math.inl
//*
//* Inlined math functions.
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

#pragma once

#include "egoboo_math.h"
#include "egoboo_types.inl"

//---------------------------------------------------------------------------------------------
INLINE void turn_to_vec( Uint16 turn, float * dx, float * dy )
{
  *dx = -turntocos[turn>>2];
  *dy = -turntosin[turn>>2];
}

//---------------------------------------------------------------------------------------------
INLINE Uint16 vec_to_turn( float dx, float dy )
{
  return RAD_TO_TURN( atan2( dy, dx ) );
}

//---------------------------------------------------------------------------------------------
INLINE vect3 VSub( vect3 A, vect3 B )
{
  vect3 tmp;
  tmp.x = A.x - B.x; tmp.y = A.y - B.y; tmp.z = A.z - B.z;
  return ( tmp );
}

//---------------------------------------------------------------------------------------------
INLINE vect3 Normalize( vect3 vec )
{
  vect3 tmp = vec;
  float len;

  len = vec.x * vec.x + vec.y * vec.y + vec.z * vec.z;
  if ( len == 0.0f )
  {
    tmp.x = tmp.y = tmp.z = 0.0f;
  }
  else
  {
    len = sqrt( len );
    tmp.x /= len;
    tmp.y /= len;
    tmp.z /= len;
  };

  return ( tmp );
}

//---------------------------------------------------------------------------------------------
INLINE vect3 CrossProduct( vect3 A, vect3 B )
{
  vect3 tmp;
  tmp.x = A.y * B.z - A.z * B.y;
  tmp.y = A.z * B.x - A.x * B.z;
  tmp.z = A.x * B.y - A.y * B.x;
  return ( tmp );
}

//---------------------------------------------------------------------------------------------
INLINE float DotProduct( vect3 A, vect3 B )
{ return ( A.x*B.x + A.y*B.y + A.z*B.z ); }

//---------------------------------------------------------------------------------------------
INLINE vect4 VSub4( vect4 A, vect4 B )
{
  vect4 tmp;
  tmp.x = A.x - B.x; tmp.y = A.y - B.y; tmp.z = A.z - B.z;
  return ( tmp );
}

//---------------------------------------------------------------------------------------------
INLINE vect4 Normalize4( vect4 vec )
{
  vect4 tmp = vec;
  float len;

  len = ( vec.x * vec.x + vec.y * vec.y + vec.z * vec.z );
  if ( len == 0.0f )
  {
    tmp.x = tmp.y = tmp.z = 0.0f;
  }
  else
  {
    len = sqrt( len );
    tmp.x /= len;
    tmp.y /= len;
    tmp.z /= len;
  };

  return ( tmp );
}

//---------------------------------------------------------------------------------------------
INLINE vect4 CrossProduct4( vect4 A, vect4 B )
{
  vect4 tmp;
  tmp.x = A.y * B.z - A.z * B.y;
  tmp.y = A.z * B.x - A.x * B.z;
  tmp.z = A.x * B.y - A.y * B.x;
  return ( tmp );
}

//---------------------------------------------------------------------------------------------
INLINE float DotProduct4( vect4 A, vect4 B )
{ return ( A.x*B.x + A.y*B.y + A.z*B.z ); }

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
INLINE quaternion QuatConjugate(quaternion q1)
{
  quaternion q2;

  q2.x = -q1.x;
  q2.y = -q1.y;
  q2.z = -q1.z;
  q2.w =  q1.w;

  return q2;
}

//---------------------------------------------------------------------------------------------
INLINE quaternion QuatNormalize(quaternion q1)
{
  quaternion q2;
  float len2;

  len2 = q1.x*q1.x + q1.y*q1.y + q1.z*q1.z + q1.w*q1.w;
  if(0.0f == len2)
  {
    q2.x = q2.y = q2.z = q2.w = 0.0f;
  }
  else
  {
    float len = sqrt(len2);
    q2.x = q1.x / len;
    q2.y = q1.y / len;
    q2.z = q1.z / len;
    q2.w = q1.w / len;
  }

  return q2;
}

//---------------------------------------------------------------------------------------------
INLINE quaternion QuatMultiply(quaternion q1, quaternion q2)
{
  quaternion q3;

  q3.x = (q1.x*q2.w + q1.w*q2.x) + (q1.y*q2.z - q1.z*q2.y);
  q3.y = (q1.y*q2.w + q1.w*q2.y) + (q1.z*q2.x - q1.x*q2.z);
  q3.z = (q1.z*q2.w + q1.w*q2.z) + (q1.x*q2.y - q1.y*q2.x);
  q3.w = q1.w*q2.w - q1.x*q2.x - q1.y*q2.y - q1.z*q2.z;

  return q3;
}

//---------------------------------------------------------------------------------------------
INLINE quaternion QuatDotprod(quaternion q1, quaternion q2)
{
  quaternion q3;

  q3.x = (q1.x*q2.w - q1.w*q2.x) - (q1.y*q2.z - q1.z*q2.y);
  q3.y = (q1.y*q2.w - q1.w*q2.y) - (q1.z*q2.x - q1.x*q2.z);
  q3.z = (q1.z*q2.w - q1.w*q2.z) - (q1.x*q2.y - q1.y*q2.x);
  q3.w = q1.w*q2.w + q1.x*q2.x + q1.y*q2.y + q1.z*q2.z;

  return q3;
}

//---------------------------------------------------------------------------------------------
INLINE quaternion QuatTransform(quaternion q1, quaternion q2, quaternion q3)
{
  quaternion qtmp;

  qtmp = QuatDotprod(q2,q3);
  qtmp = QuatMultiply(q1,qtmp);

  return qtmp;
}

//---------------------------------------------------------------------------------------------
INLINE quaternion QuatConvert(matrix_4x4 m)
{
  quaternion quat = {{0,0,0,1}};  // the funky {{,,,}} thing is because the vectors are declared as structs of unions (nested typed == nested {}'s)
  float  tr, s, s2;
  int    i, j, k;
  int nxt[3] = {1, 2, 0};

  tr = m.CNV(0,0) + m.CNV(1, 1) + m.CNV(2, 2);

  // check the diagonal
  if (tr > 0.0)
  {
    s = sqrt (tr + 1.0);
    quat.w = s / 2.0;
    s = 0.5 / s;
    quat.x = (m.CNV(2, 1) - m.CNV(1, 2)) * s;
    quat.y = (m.CNV(0, 2) - m.CNV(2, 0)) * s;
    quat.z = (m.CNV(1, 0) - m.CNV(0, 1)) * s;
  }
  else
  {
    // diagonal is negative
    i = 0;
    if (m.CNV(1, 1) > m.CNV(0, 0)) i = 1;
    if (m.CNV(2, 2) > m.CNV(i, i)) i = 2;
    j = nxt[i];
    k = nxt[j];

    s2 = 1.0 + m.CNV(i, i) - m.CNV(j, j) - m.CNV(k, k);
    if(s2 <= 0.0f) return quat;

    s = sqrt(s2);

    quat.v[i] = s * 0.5f;

    s = 0.5f / s;
    quat.v[3] = (m.CNV(k, j) - m.CNV(j, k)) * s;
    quat.v[j] = (m.CNV(j, i) + m.CNV(i, j)) * s;
    quat.v[k] = (m.CNV(k, i) + m.CNV(i, k)) * s;
  }

  return quat;
}


//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
INLINE matrix_4x4 IdentityMatrix()
{
  matrix_4x4 tmp;

  tmp.CNV( 0, 0 ) = 1; tmp.CNV( 1, 0 ) = 0; tmp.CNV( 2, 0 ) = 0; tmp.CNV( 3, 0 ) = 0;
  tmp.CNV( 0, 1 ) = 0; tmp.CNV( 1, 1 ) = 1; tmp.CNV( 2, 1 ) = 0; tmp.CNV( 3, 1 ) = 0;
  tmp.CNV( 0, 2 ) = 0; tmp.CNV( 1, 2 ) = 0; tmp.CNV( 2, 2 ) = 1; tmp.CNV( 3, 2 ) = 0;
  tmp.CNV( 0, 3 ) = 0; tmp.CNV( 1, 3 ) = 0; tmp.CNV( 2, 3 ) = 0; tmp.CNV( 3, 3 ) = 1;
  return ( tmp );
}

//--------------------------------------------------------------------------------------------
INLINE matrix_4x4 ZeroMatrix( void )
{
  matrix_4x4 ret;
  int i, j;

  for ( i = 0; i < 4; i++ )
    for ( j = 0; j < 4; j++ )
      ret.CNV( i, j ) = 0;

  return ret;
}

//--------------------------------------------------------------------------------------------
INLINE matrix_4x4 MatrixTranspose( EGO_CONST matrix_4x4 a )
{
  matrix_4x4 ret;
  int i, j;

  for ( i = 0; i < 4; i++ )
    for ( j = 0; j < 4; j++ )
      ret.CNV( i, j ) = a.CNV( j, i );

  return ret;
}

//--------------------------------------------------------------------------------------------
INLINE matrix_4x4 MatrixMult( EGO_CONST matrix_4x4 a, EGO_CONST matrix_4x4 b )
{
  matrix_4x4 ret;
  int i, j, k;

  for ( i = 0; i < 4; i++ )
    for ( j = 0; j < 4; j++ )
    {
      ret.CNV( i, j ) = 0.0f;
      for ( k = 0; k < 4; k++ )
        ret.CNV( i, j ) += a.CNV( k, j ) * b.CNV( i, k );
    };

  return ret;
}

//--------------------------------------------------------------------------------------------
INLINE matrix_4x4 Translate( EGO_CONST float dx, EGO_CONST float dy, EGO_CONST float dz )
{
  matrix_4x4 ret = IdentityMatrix();
  ret.CNV( 3, 0 ) = dx;
  ret.CNV( 3, 1 ) = dy;
  ret.CNV( 3, 2 ) = dz;
  return ret;
}

//--------------------------------------------------------------------------------------------
INLINE matrix_4x4 RotateX( EGO_CONST float rads )
{
  float cosine = ( float ) cos( rads );
  float sine   = ( float ) sin( rads );

  matrix_4x4 ret = IdentityMatrix();
  ret.CNV( 1, 1 ) =  cosine;
  ret.CNV( 2, 2 ) =  cosine;
  ret.CNV( 1, 2 ) = -sine;
  ret.CNV( 2, 1 ) =  sine;
  return ret;
}

//--------------------------------------------------------------------------------------------
INLINE matrix_4x4 RotateY( EGO_CONST float rads )
{
  float cosine = ( float ) cos( rads );
  float sine   = ( float ) sin( rads );
  matrix_4x4 ret = IdentityMatrix();
  ret.CNV( 0, 0 ) =  cosine;  //0,0
  ret.CNV( 2, 2 ) =  cosine;  //2,2
  ret.CNV( 0, 2 ) =  sine;    //0,2
  ret.CNV( 2, 0 ) = -sine;    //2,0
  return ret;
}

//--------------------------------------------------------------------------------------------
INLINE matrix_4x4 RotateZ( EGO_CONST float rads )
{
  float cosine = ( float ) cos( rads );
  float sine   = ( float ) sin( rads );
  matrix_4x4 ret = IdentityMatrix();
  ret.CNV( 0, 0 ) =  cosine;  //0,0
  ret.CNV( 1, 1 ) =  cosine;  //1,1
  ret.CNV( 0, 1 ) = -sine;    //0,1
  ret.CNV( 1, 0 ) =  sine;    //1,0
  return ret;
}

//--------------------------------------------------------------------------------------------
INLINE matrix_4x4 ScaleXYZ( EGO_CONST float sizex, EGO_CONST float sizey, EGO_CONST float sizez )
{
  matrix_4x4 ret = IdentityMatrix();
  ret.CNV( 0, 0 ) = sizex;   //0,0
  ret.CNV( 1, 1 ) = sizey;   //1,1
  ret.CNV( 2, 2 ) = sizez;   //2,2
  return ret;
}

//--------------------------------------------------------------------------------------------
INLINE matrix_4x4 ScaleXYZRotateXYZTranslate( EGO_CONST float sizex, EGO_CONST float sizey, EGO_CONST float sizez, Uint16 turnz, Uint16 turnx, Uint16 turny, float tx, float ty, float tz )
{
  matrix_4x4 ret;

  float cx = turntocos[(turnx >> 2) & TRIGTABLE_MASK];
  float sx = turntosin[(turnx >> 2) & TRIGTABLE_MASK];

  float cy = turntocos[(turny >> 2) & TRIGTABLE_MASK];
  float sy = turntosin[(turny >> 2) & TRIGTABLE_MASK];

  float cz = turntocos[(turnz >> 2) & TRIGTABLE_MASK];
  float sz = turntosin[(turnz >> 2) & TRIGTABLE_MASK];

  float sxsy = sx * sy;
  float cxsy = cx * sy;
  float sxcy = sx * cy;
  float cxcy = cx * cy;

  ret.CNV( 0, 0 ) = sizex * ( cy * cz );    //0,0
  ret.CNV( 0, 1 ) = sizex * ( sxsy * cz + cx * sz );  //0,1
  ret.CNV( 0, 2 ) = sizex * ( -cxsy * cz + sx * sz );  //0,2
  ret.CNV( 0, 3 ) = 0;      //0,3

  ret.CNV( 1, 0 ) = sizey * ( -cy * sz );    //1,0
  ret.CNV( 1, 1 ) = sizey * ( -sxsy * sz + cx * cz );  //1,1
  ret.CNV( 1, 2 ) = sizey * ( cxsy * sz + sx * cz );  //1,2
  ret.CNV( 1, 3 ) = 0;      //1,3

  ret.CNV( 2, 0 ) = sizez * ( sy );  //2,0
  ret.CNV( 2, 1 ) = sizez * ( -sxcy );    //2,1
  ret.CNV( 2, 2 ) = sizez * ( cxcy );    //2,2
  ret.CNV( 2, 3 ) = 0;      //2,3

  ret.CNV( 3, 0 ) = tx;      //3,0
  ret.CNV( 3, 1 ) = ty;      //3,1
  ret.CNV( 3, 2 ) = tz;      //3,2
  ret.CNV( 3, 3 ) = 1;      //3,3
  return ret;
}

//--------------------------------------------------------------------------------------------
INLINE matrix_4x4 FourPoints( vect4 ori, vect4 wid, vect4 frw, vect4 up, float scale )
{
  matrix_4x4 tmp;

  wid.x -= ori.x;  frw.x -= ori.x;  up.x -= ori.x;
  wid.y -= ori.y;  frw.y -= ori.y;  up.y -= ori.y;
  wid.z -= ori.z;  frw.z -= ori.z;  up.z -= ori.z;

  // fix -x scaling on the input
  wid.x *= -1.0; // HUK
  wid.y *= -1.0; // HUK
  wid.z *= -1.0; // HUK

  wid = Normalize4( wid );
  frw = Normalize4( frw );
  up  = Normalize4( up );

  tmp.CNV( 0, 0 ) = scale * wid.x;       //0,0
  tmp.CNV( 0, 1 ) = scale * wid.y;       //0,1
  tmp.CNV( 0, 2 ) = scale * wid.z;       //0,2
  tmp.CNV( 0, 3 ) = 0;  //0,3

  tmp.CNV( 1, 0 ) = scale * frw.x;       //1,0
  tmp.CNV( 1, 1 ) = scale * frw.y;       //1,1
  tmp.CNV( 1, 2 ) = scale * frw.z;       //1,2
  tmp.CNV( 1, 3 ) = 0;  //1,3

  tmp.CNV( 2, 0 ) = scale * up.x;       //2,0
  tmp.CNV( 2, 1 ) = scale * up.y;       //2,1
  tmp.CNV( 2, 2 ) = scale * up.z;       //2,2
  tmp.CNV( 2, 3 ) = 0;       //2,3

  tmp.CNV( 3, 0 ) = ori.x;       //3,0
  tmp.CNV( 3, 1 ) = ori.y;       //3,1
  tmp.CNV( 3, 2 ) = ori.z;       //3,2
  tmp.CNV( 3, 3 ) = 1;       //3,3

  return tmp;
}

//----------------------------------------------------
INLINE matrix_4x4 MatrixConvert(quaternion q)
{
  matrix_4x4 tmp;

  q = QuatNormalize(q);

  tmp.CNV(0,0) = 1.0f - 2.0f*(q.y*q.y + q.z*q.z);
  tmp.CNV(0,1) =        2.0f*(q.x*q.y + q.w*q.z);
  tmp.CNV(0,2) =        2.0f*(q.x*q.z - q.w*q.y);
  tmp.CNV(0,3) = 0.0f;

  tmp.CNV(1,0) =        2.0f*(q.x*q.y - q.w*q.z);
  tmp.CNV(1,1) = 1.0f - 2.0f*(q.x*q.x + q.z*q.z);
  tmp.CNV(1,2) =        2.0f*(q.w*q.x + q.y*q.z);
  tmp.CNV(1,3) = 0.0f;

  tmp.CNV(2,0) =        2.0f*(q.w*q.y + q.x*q.z);
  tmp.CNV(2,1) =        2.0f*(q.y*q.z - q.w*q.x);
  tmp.CNV(2,2) = 1.0f - 2.0f*(q.x*q.x + q.y*q.y);
  tmp.CNV(2,3) = 0.0f;

  tmp.CNV(3,0) = 0.0f;
  tmp.CNV(3,1) = 0.0f;
  tmp.CNV(3,2) = 0.0f;
  tmp.CNV(3,3) = 1.0f;

  return tmp;
}

//----------------------------------------------------
INLINE void Transform4_Full( float pre_scale, float post_scale, matrix_4x4 *pMatrix, vect4 pSourceV[], vect4 pDestV[], Uint32 NumVertor )
{
  Uint32   cnt;
  vect4 *psrc, *pdst;

  psrc = pSourceV;
  pdst = pDestV;
  for (cnt = 0; cnt < NumVertor; cnt++, psrc++, pdst++)
  {
    pdst->x = psrc->x * pMatrix->v[0] + psrc->y * pMatrix->v[4] + psrc->z * pMatrix->v[ 8];
    pdst->y = psrc->x * pMatrix->v[1] + psrc->y * pMatrix->v[5] + psrc->z * pMatrix->v[ 9];
    pdst->z = psrc->x * pMatrix->v[2] + psrc->y * pMatrix->v[6] + psrc->z * pMatrix->v[10];
    pdst->w = psrc->x * pMatrix->v[3] + psrc->y * pMatrix->v[7] + psrc->z * pMatrix->v[11];

    if(1.0f != pre_scale)
    {
      pdst->x *= pre_scale;
      pdst->y *= pre_scale;
      pdst->z *= pre_scale;
    };

    if(1.0f == psrc->w)
    {
      pdst->x += pMatrix->v[12];
      pdst->y += pMatrix->v[13];
      pdst->z += pMatrix->v[14];
      pdst->w += pMatrix->v[15];
    }
    else if(0.0f != psrc->w)
    {
      pdst->x += psrc->w * pMatrix->v[12];
      pdst->y += psrc->w * pMatrix->v[13];
      pdst->z += psrc->w * pMatrix->v[14];
      pdst->w += psrc->w * pMatrix->v[15];
    }

    if(1.0f != post_scale)
    {
      pdst->x *= post_scale;
      pdst->y *= post_scale;
      pdst->z *= post_scale;
    };
  }
}

//----------------------------------------------------
INLINE void Transform4( float pre_scale, float post_scale, matrix_4x4 *pMatrix, vect4 pSourceV[], vect4 pDestV[], Uint32 NumVertor )
{
  Uint32   cnt;
  vect4 *psrc, *pdst;
  float scale = pre_scale * post_scale;

  psrc = pSourceV;
  pdst = pDestV;
  for (cnt = 0; cnt < NumVertor; cnt++, psrc++, pdst++)
  {
    pdst->x = psrc->x * pMatrix->v[0] + psrc->y * pMatrix->v[4] + psrc->z * pMatrix->v[ 8];
    pdst->y = psrc->x * pMatrix->v[1] + psrc->y * pMatrix->v[5] + psrc->z * pMatrix->v[ 9];
    pdst->z = psrc->x * pMatrix->v[2] + psrc->y * pMatrix->v[6] + psrc->z * pMatrix->v[10];
    pdst->w = psrc->x * pMatrix->v[3] + psrc->y * pMatrix->v[7] + psrc->z * pMatrix->v[11];

    if(1.0f != scale)
    {
      pdst->x *= scale;
      pdst->y *= scale;
      pdst->z *= scale;
    }
  }
}

//----------------------------------------------------
INLINE void Translate4( matrix_4x4 *pMatrix, vect4 pSourceV[], vect4 pDestV[], Uint32 NumVertor )
{
  Uint32   cnt;
  vect4 *psrc, *pdst;

  psrc = pSourceV;
  pdst = pDestV;
  for (cnt = 0; cnt < NumVertor; cnt++, psrc++, pdst++)
  {
    pdst->x = psrc->x + pMatrix->v[12];
    pdst->y = psrc->y + pMatrix->v[13];
    pdst->z = psrc->z + pMatrix->v[14];
    pdst->w = psrc->w + pMatrix->v[15];
  }
}

//----------------------------------------------------
INLINE void Transform3_Full( float pre_scale, float post_scale, matrix_4x4 *pMatrix, vect3 pSourceV[], vect3 pDestV[], Uint32 NumVertor )
{
  Uint32   cnt;
  vect3 *psrc, *pdst;

  psrc = pSourceV;
  pdst = pDestV;
  for (cnt = 0; cnt < NumVertor; cnt++, psrc++, pdst++)
  {
    pdst->x = psrc->x * pMatrix->v[0] + psrc->y * pMatrix->v[4] + psrc->z * pMatrix->v[ 8];
    pdst->y = psrc->x * pMatrix->v[1] + psrc->y * pMatrix->v[5] + psrc->z * pMatrix->v[ 9];
    pdst->z = psrc->x * pMatrix->v[2] + psrc->y * pMatrix->v[6] + psrc->z * pMatrix->v[10];

    if(1.0f != pre_scale)
    {
      pdst->x *= pre_scale;
      pdst->y *= pre_scale;
      pdst->z *= pre_scale;
    }

    pdst->x += pMatrix->v[12];
    pdst->y += pMatrix->v[13];
    pdst->z += pMatrix->v[14];

    if(1.0f != post_scale)
    {
      pdst->x *= post_scale;
      pdst->y *= post_scale;
      pdst->z *= post_scale;
    }
  }

}

//----------------------------------------------------
INLINE void Transform3( float pre_scale, float post_scale, matrix_4x4 *pMatrix, vect3 pSourceV[], vect3 pDestV[], Uint32 NumVertor )
{
  Uint32   cnt;
  vect3 *psrc, *pdst;
  float scale = pre_scale * post_scale;

  psrc = pSourceV;
  pdst = pDestV;
  for (cnt = 0; cnt < NumVertor; cnt++, psrc++, pdst++)
  {
    pdst->x = psrc->x * pMatrix->v[0] + psrc->y * pMatrix->v[4] + psrc->z * pMatrix->v[ 8];
    pdst->y = psrc->x * pMatrix->v[1] + psrc->y * pMatrix->v[5] + psrc->z * pMatrix->v[ 9];
    pdst->z = psrc->x * pMatrix->v[2] + psrc->y * pMatrix->v[6] + psrc->z * pMatrix->v[10];

    if(1.0f != scale)
    {
      pdst->x *= scale;
      pdst->y *= scale;
      pdst->z *= scale;
    }
  }
}

//----------------------------------------------------
INLINE void Translate3( matrix_4x4 *pMatrix, vect3 pSourceV[], vect3 pDestV[], Uint32 NumVertor )
{
  Uint32 cnt;
  vect3 *psrc, *pdst;

  psrc = pSourceV;
  pdst = pDestV;
  for (cnt = 0; cnt < NumVertor; cnt++, psrc++, pdst++)
  {
    pdst->x = psrc->x + pMatrix->v[12];
    pdst->y = psrc->y + pMatrix->v[13];
    pdst->z = psrc->z + pMatrix->v[14];
  }
}

//----------------------------------------------------
INLINE void VectorClear( float v[] )
{
  v[0] = v[1] = v[2] = 0.0f;
}

//----------------------------------------------------
INLINE void VectorClear4( float v[] )
{
  v[0] = v[1] = v[2] = 0.0f;
  v[3] = 1.0f;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t matrix_compare_3x3(matrix_4x4 * pm1, matrix_4x4 * pm2)
{
  // BB > compare two 3x3 matrices to see if the transformation is the same

  int i,j;

  for(i=0;i<3;i++)
  {
    for(j=0;j<3;j++)
    {
      if( (*pm1).CNV(i,j) != (*pm2).CNV(i,j) ) return bfalse;
    };
  };

  return btrue;
}


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE EGO_CONST BBOX_LIST * bbox_list_new(BBOX_LIST * lst)
{
  if(NULL == lst) return NULL;
  memset(lst, 0, sizeof(BBOX_LIST));
  return lst;
}

//--------------------------------------------------------------------------------------------
INLINE EGO_CONST BBOX_LIST * bbox_list_delete(BBOX_LIST * lst)
{
  if(NULL == lst) return NULL;

  if( lst->count > 0 )
  {
    EGOBOO_DELETE(lst->list);
  }

  lst->count = 0;
  lst->list  = NULL;

  return lst;
}

//--------------------------------------------------------------------------------------------
INLINE EGO_CONST BBOX_LIST * bbox_list_renew(BBOX_LIST * lst)
{
  if(NULL == lst) return NULL;

  bbox_list_delete(lst);
  return bbox_list_new(lst);
}

//--------------------------------------------------------------------------------------------
INLINE EGO_CONST BBOX_LIST * bbox_list_alloc(BBOX_LIST * lst, int count)
{
  if(NULL == lst) return NULL;

  bbox_list_delete(lst);

  if(count>0)
  {
    lst->list = EGOBOO_NEW_ARY( AA_BBOX, count );
    if(NULL != lst->list)
    {
      lst->count = count;
    }
  }

  return lst;
}

//--------------------------------------------------------------------------------------------
INLINE EGO_CONST BBOX_LIST * bbox_list_realloc(BBOX_LIST * lst, int count)
{
  // check for bad list
  if(NULL == lst) return NULL;

  // check for no change in the count
  if(count == lst->count) return lst;

  // check another dumb case
  if(count==0)
  {
    return bbox_list_delete(lst);
  }


  lst->list = (AA_BBOX *)realloc(lst->list, count * sizeof(AA_BBOX));
  if(NULL == lst->list)
  {
    lst->count = 0;
  }
  else
  {
    lst->count = count;
  }


  return lst;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE EGO_CONST BBOX_ARY * bbox_ary_new(BBOX_ARY * ary)
{
  if(NULL == ary) return NULL;
  memset(ary, 0, sizeof(BBOX_ARY));
  return ary;
}

//--------------------------------------------------------------------------------------------
INLINE EGO_CONST BBOX_ARY * bbox_ary_delete(BBOX_ARY * ary)
{
  int i;

  if(NULL == ary) return NULL;

  if(NULL !=ary->list)
  {
    for(i=0; i<ary->count; i++)
    {
      bbox_list_delete(ary->list + i);
    }

    EGOBOO_DELETE(ary->list);
  }

  ary->count = 0;
  ary->list = NULL;

  return ary;
}

//--------------------------------------------------------------------------------------------
INLINE EGO_CONST BBOX_ARY * bbox_ary_renew(BBOX_ARY * ary)
{
  if(NULL == ary) return NULL;
  bbox_ary_delete(ary);
  return bbox_ary_new(ary);
}

//--------------------------------------------------------------------------------------------
INLINE EGO_CONST BBOX_ARY * bbox_ary_alloc(BBOX_ARY * ary, int count)
{
  if(NULL == ary) return NULL;

  bbox_ary_delete(ary);

  if(count>0)
  {
    ary->list = EGOBOO_NEW_ARY( BBOX_LIST, count );
    if(NULL != ary->list)
    {
      ary->count = count;
    }
  }

  return ary;
}

//--------------------------------------------------------------------------------------------
INLINE Uint32 ego_rand_32(Uint32 * seed)
{
  if(NULL == seed)
    return rand();

  *seed = (Uint32)0x000019660D * (*seed) + (Uint32)0x3C6EF35F;

  return *seed;
}

//--------------------------------------------------------------------------------------------
INLINE Uint16 ego_rand_16(Uint16 * seed)
{
  if(NULL == seed) return rand() & 0xFFFFL ;

  *seed = (Uint16)0x00FD * (*seed) + (Uint16)0x3F03;

  return *seed;
}

//--------------------------------------------------------------------------------------------
INLINE Uint8 ego_rand_8(Uint8 * seed)
{
  if(NULL == seed) return rand() & 0xFFL;

  *seed = (Uint8)0x0D * (*seed) + (Uint8)0x33;

  return *seed;
}
