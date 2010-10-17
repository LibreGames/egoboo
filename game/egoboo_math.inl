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

/// @file egoboo_math.inl
/// @brief
/// @details Almost all of the math functions are intended to be inlined for maximum speed

#include "egoboo_math.h"
#include "log.h"
#include "ogl_include.h"
#include "ogl_debug.h"

#include <math.h>
#include <float.h>

//--------------------------------------------------------------------------------------------
// FORWARD DECLARATIONS
//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
extern "C" {
#endif

// conversion functions
INLINE FACING_T vec_to_facing( float dx, float dy );
INLINE void     facing_to_vec( FACING_T facing, float * dx, float * dy );

// rotation functions
INLINE int terp_dir( FACING_T majordir, FACING_T minordir, int weight );

// limiting functions
INLINE void getadd( int min, int value, int max, int* valuetoadd );
INLINE void fgetadd( float min, float value, float max, float* valuetoadd );

// random functions
INLINE int generate_irand_pair( IPair num );
INLINE int generate_irand_range( FRange num );
INLINE int generate_randmask( int base, int mask );

// vector functions
INLINE bool_t  fvec2_self_clear( fvec2_base_t A );
INLINE bool_t  fvec2_base_copy( fvec2_base_t A, fvec2_base_t B );
INLINE bool_t  fvec2_base_assign( fvec2_base_t A, fvec2_t B );
INLINE float   fvec2_length( const fvec2_base_t A );
INLINE float   fvec2_length_abs( const fvec2_base_t A );
INLINE bool_t  fvec2_self_scale( fvec2_base_t A, const float B );
INLINE fvec2_t fvec2_sub( const fvec2_base_t A, const fvec2_base_t B );
INLINE fvec2_t fvec2_normalize( const fvec2_base_t vec );
INLINE float   fvec2_cross_product( const fvec2_base_t A, const fvec2_base_t B );
INLINE float   fvec2_dot_product( const fvec2_base_t A, const fvec2_base_t B );
INLINE float   fvec3_dist_abs( const fvec3_base_t A, const fvec3_base_t B );

INLINE bool_t  fvec3_self_clear( fvec3_base_t A );
INLINE bool_t  fvec3_base_copy( fvec3_base_t A, fvec3_base_t B );
INLINE bool_t  fvec3_base_assign( fvec3_base_t A, fvec3_t B );
INLINE bool_t  fvec3_self_scale( fvec3_base_t A, const float B );
INLINE bool_t  fvec3_self_sum( fvec3_base_t A, const fvec3_base_t B );
INLINE bool_t  fvec3_self_normalize( fvec3_base_t A );
INLINE bool_t  fvec3_self_normalize_to( fvec3_base_t A, float B );
INLINE float   fvec3_length_2( const fvec3_base_t A );
INLINE float   fvec3_length( const fvec3_base_t A );
INLINE float   fvec3_length_abs( const fvec3_base_t A );
INLINE float   fvec3_dot_product( const fvec3_base_t A, const fvec3_base_t B );
INLINE float   fvec3_dist_abs( const fvec3_base_t A, const fvec3_base_t B );
INLINE fvec3_t fvec3_scale( const fvec3_base_t A, const float B );
INLINE fvec3_t fvec3_normalize( const fvec3_base_t A );
INLINE fvec3_t fvec3_add( const fvec3_base_t A, const fvec3_base_t B );
INLINE fvec3_t fvec3_sub( const fvec3_base_t A, const fvec3_base_t B );
INLINE fvec3_t fvec3_cross_product( const fvec3_base_t A, const fvec3_base_t B );
INLINE float   fvec3_decompose( const fvec3_base_t A, const fvec3_base_t NRM, fvec3_base_t PARA, fvec3_base_t PERP );

INLINE bool_t fvec4_self_clear( fvec4_t * A );

// matrix functions
INLINE fmat_4x4_t IdentityMatrix( void );
INLINE fmat_4x4_t ZeroMatrix( void );
INLINE fmat_4x4_t MatrixMult( const fmat_4x4_t a, const fmat_4x4_t b );
INLINE fmat_4x4_t Translate( const float dx, const float dy, const float dz );
INLINE fmat_4x4_t RotateX( const float rads );
INLINE fmat_4x4_t RotateY( const float rads );
INLINE fmat_4x4_t RotateZ( const float rads );
INLINE fmat_4x4_t ScaleXYZ( const float sizex, const float sizey, const float sizez );
INLINE fmat_4x4_t FourPoints( const fvec4_base_t ori, const fvec4_base_t wid, const fvec4_base_t frw, const fvec4_base_t upx, const float scale );
INLINE fmat_4x4_t ViewMatrix( const fvec3_base_t   from, const fvec3_base_t   at, const fvec3_base_t   world_up, const float roll );
INLINE fmat_4x4_t ProjectionMatrix( const float near_plane, const float far_plane, const float fov );
INLINE void       TransformVertices( const fmat_4x4_t *pMatrix, const fvec4_t *pSourceV, fvec4_t *pDestV, const Uint32 NumVertor );

INLINE fvec3_t   mat_getChrUp( const fmat_4x4_t mat );
INLINE fvec3_t   mat_getChrForward( const fmat_4x4_t mat );
INLINE fvec3_t   mat_getChrRight( const fmat_4x4_t mat );
INLINE fvec3_t   mat_getCamUp( const fmat_4x4_t mat );
INLINE fvec3_t   mat_getCamRight( const fmat_4x4_t mat );
INLINE fvec3_t   mat_getCamForward( const fmat_4x4_t mat );
INLINE fvec3_t   mat_getTranslate( const fmat_4x4_t mat );
INLINE float *   mat_getTranslate_v( const fmat_4x4_t mat );

#if defined(__cplusplus)
};
#endif

//--------------------------------------------------------------------------------------------
// CONVERSION FUNCTIONS
//--------------------------------------------------------------------------------------------
INLINE FACING_T vec_to_facing( float dx, float dy )
{
    return ( FACING_T )(( ATAN2( dy, dx ) + PI ) * RAD_TO_TURN );
}

//--------------------------------------------------------------------------------------------
INLINE void facing_to_vec( FACING_T facing, float * dx, float * dy )
{
    TURN_T turn = TO_TURN( facing - 0x8000 );

    if ( NULL != dx )
    {
        *dx = turntocos[ turn ];
    }

    if ( NULL != dy )
    {
        *dy = turntosin[ turn ];
    }
}

//--------------------------------------------------------------------------------------------
// ROTATION FUNCTIONS
//--------------------------------------------------------------------------------------------
INLINE int terp_dir( FACING_T majordir, FACING_T minordir, int weight )
{
    /// @details ZZ@> This function returns a direction between the major and minor ones, closer
    ///    to the major.

    int diff;

    // Align major direction with 0
    diff = ( int )minordir - ( int )majordir;

    if ( diff <= -( int )0x8000L )
    {
        diff += ( int )0x00010000L;
    }
    else if ( diff >= ( int )0x8000L )
    {
        diff -= ( int )0x00010000L;
    }

    return diff / weight;
}

//--------------------------------------------------------------------------------------------
// LIMITING FUNCTIONS
//--------------------------------------------------------------------------------------------
INLINE void getadd( int min, int value, int max, int* valuetoadd )
{
    /// @details ZZ@> This function figures out what value to add should be in order
    ///    to not overflow the min and max bounds

    int newvalue;

    newvalue = value + ( *valuetoadd );
    if ( newvalue < min )
    {
        // Increase valuetoadd to fit
        *valuetoadd = min - value;
        if ( *valuetoadd > 0 )  *valuetoadd = 0;

        return;
    }
    if ( newvalue > max )
    {
        // Decrease valuetoadd to fit
        *valuetoadd = max - value;
        if ( *valuetoadd < 0 )  *valuetoadd = 0;
    }
}

//--------------------------------------------------------------------------------------------
INLINE void fgetadd( float min, float value, float max, float* valuetoadd )
{
    /// @details ZZ@> This function figures out what value to add should be in order
    ///    to not overflow the min and max bounds

    float newvalue;

    newvalue = value + ( *valuetoadd );
    if ( newvalue < min )
    {
        // Increase valuetoadd to fit
        *valuetoadd = min - value;
        if ( *valuetoadd > 0 )  *valuetoadd = 0;

        return;
    }
    if ( newvalue > max )
    {
        // Decrease valuetoadd to fit
        *valuetoadd = max - value;
        if ( *valuetoadd < 0 )  *valuetoadd = 0;
    }
}

//--------------------------------------------------------------------------------------------
// RANDOM FUNCTIONS
//--------------------------------------------------------------------------------------------
INLINE int generate_irand_pair( IPair num )
{
    /// @details ZZ@> This function generates a random number

    int tmp = 0;
    int irand = RANDIE;

    tmp = num.base;
    if ( num.rand > 1 )
    {
        tmp += irand % num.rand;
    }

    return tmp;
}

//--------------------------------------------------------------------------------------------
INLINE int generate_irand_range( FRange num )
{
    /// @details ZZ@> This function generates a random number

    IPair loc_pair;

    range_to_pair( num, &loc_pair );

    return generate_irand_pair( loc_pair );
}

//--------------------------------------------------------------------------------------------
INLINE int generate_randmask( int base, int mask )
{
    /// @details ZZ@> This function generates a random number
    int tmp;
    int irand = RANDIE;

    tmp = base;
    if ( mask > 0 )
    {
        tmp += irand & mask;
    }

    return tmp;
}

//--------------------------------------------------------------------------------------------
// VECTOR FUNCTIONS
//--------------------------------------------------------------------------------------------
INLINE bool_t fvec2_self_clear( fvec2_base_t A )
{
    if ( NULL == A ) return bfalse;

    A[kX] = A[kY] = 0.0f;

    return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t fvec2_base_copy( fvec2_base_t A, fvec2_base_t B )
{
    if ( NULL == A ) return bfalse;

    if ( NULL == B ) return fvec2_self_clear( A );

    A[kX] = B[kX];
    A[kY] = B[kY];

    return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t  fvec2_base_assign( fvec2_base_t A, fvec2_t B )
{
    if ( NULL == A ) return bfalse;

    A[kX] = B.v[kX];
    A[kY] = B.v[kY];

    return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t fvec2_self_scale( fvec2_base_t A, const float B )
{
    if ( NULL == A ) return bfalse;

    A[kX] *= B;
    A[kY] *= B;

    return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE float fvec2_length_abs( const fvec2_base_t A )
{
    if ( NULL == A ) return 0.0f;

    return ABS( A[kX] ) + ABS( A[kY] );
}

//--------------------------------------------------------------------------------------------
INLINE float fvec2_length( const fvec2_base_t A )
{
    float A2;

    if ( NULL == A ) return 0.0f;

    A2 = A[kX] * A[kX] + A[kY] * A[kY];

    return SQRT( A2 );
}

//--------------------------------------------------------------------------------------------
INLINE fvec2_t fvec2_sub( const fvec2_base_t A, const fvec2_base_t B )
{
    fvec2_t tmp;

    tmp.x = A[kX] - B[kX];
    tmp.y = A[kY] - B[kY];

    return tmp;
}

//--------------------------------------------------------------------------------------------
INLINE float fvec2_dist_abs( const fvec2_base_t A, const fvec2_base_t B )
{
    return ABS( A[kX] - B[kX] ) + ABS( A[kY] - B[kY] );
}

//--------------------------------------------------------------------------------------------
INLINE fvec2_t fvec2_scale( const fvec2_base_t A, const float B )
{
    fvec2_t tmp = ZERO_VECT2;

    if ( NULL == A || 0.0f == B ) return tmp;

    tmp.v[kX] = A[kX] * B;
    tmp.v[kY] = A[kY] * B;

    return tmp;
}

//--------------------------------------------------------------------------------------------
INLINE fvec2_t fvec2_normalize( const fvec2_base_t vec )
{
    fvec2_t tmp = ZERO_VECT2;

    if ( ABS( vec[kX] ) + ABS( vec[kY] ) > 0 )
    {
        float len2 = vec[kX] * vec[kX] + vec[kY] * vec[kY];
        float inv_len = 1.0f / SQRT( len2 );
        LOG_NAN( inv_len );

        tmp.x = vec[kX] * inv_len;
        LOG_NAN( tmp.x );

        tmp.y = vec[kY] * inv_len;
        LOG_NAN( tmp.y );
    }

    return tmp;
}

//--------------------------------------------------------------------------------------------
INLINE float fvec2_cross_product( const fvec2_base_t A, const fvec2_base_t B )
{
    return A[kX] * B[kY] - A[kY] * B[kX];
}

//--------------------------------------------------------------------------------------------
INLINE float   fvec2_dot_product( const fvec2_base_t A, const fvec2_base_t B )
{
    return A[kX]*B[kX] + A[kY]*B[kY];
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE bool_t fvec3_self_clear( fvec3_base_t A )
{
    if ( NULL == A ) return bfalse;

    A[kX] = A[kY] = A[kZ] = 0.0f;

    return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t fvec3_base_copy( fvec3_base_t A, fvec3_base_t B )
{
    if ( NULL == A ) return bfalse;

    if ( NULL == B ) return fvec3_self_clear( A );

    A[kX] = B[kX];
    A[kY] = B[kY];
    A[kZ] = B[kZ];

    return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t  fvec3_base_assign( fvec3_base_t A, fvec3_t B )
{
    if ( NULL == A ) return bfalse;

    A[kX] = B.v[kX];
    A[kY] = B.v[kY];
    A[kZ] = B.v[kZ];

    return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t fvec3_self_scale( fvec3_base_t A, const float B )
{
    if ( NULL == A ) return bfalse;

    A[kX] *= B;
    A[kY] *= B;
    A[kZ] *= B;

    return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t fvec3_self_sum( fvec3_base_t A, const fvec3_base_t B )
{
    if ( NULL == A || NULL == B ) return bfalse;

    A[kX] += B[kX];
    A[kY] += B[kY];
    A[kZ] += B[kZ];

    return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE float fvec3_length_abs( const fvec3_base_t A )
{
    if ( NULL == A ) return 0.0f;

    return ABS( A[kX] ) + ABS( A[kY] ) + ABS( A[kZ] );
}

//--------------------------------------------------------------------------------------------
INLINE float fvec3_length_2( const fvec3_base_t A )
{
    float A2;

    if ( NULL == A ) return 0.0f;

    A2 = A[kX] * A[kX] + A[kY] * A[kY] + A[kZ] * A[kZ];

    return A2;
}

//--------------------------------------------------------------------------------------------
INLINE float fvec3_length( const fvec3_base_t A )
{
    float A2;

    if ( NULL == A ) return 0.0f;

    A2 = A[kX] * A[kX] + A[kY] * A[kY] + A[kZ] * A[kZ];

    return SQRT( A2 );
}

//--------------------------------------------------------------------------------------------
INLINE fvec3_t fvec3_add( const fvec3_base_t A, const fvec3_base_t B )
{
    fvec3_t tmp;

    tmp.x = A[kX] + B[kX];
    tmp.y = A[kY] + B[kY];
    tmp.z = A[kZ] + B[kZ];

    return tmp;
}

//--------------------------------------------------------------------------------------------
INLINE fvec3_t fvec3_sub( const fvec3_base_t A, const fvec3_base_t B )
{
    fvec3_t tmp;

    tmp.x = A[kX] - B[kX];
    tmp.y = A[kY] - B[kY];
    tmp.z = A[kZ] - B[kZ];

    return tmp;
}

//--------------------------------------------------------------------------------------------
INLINE fvec3_t fvec3_scale( const fvec3_base_t A, const float B )
{
    fvec3_t tmp = ZERO_VECT3;

    if ( NULL == A || 0.0f == B ) return tmp;

    tmp.v[kX] = A[kX] * B;
    tmp.v[kY] = A[kY] * B;
    tmp.v[kZ] = A[kZ] * B;

    return tmp;
}

//--------------------------------------------------------------------------------------------
INLINE fvec3_t fvec3_normalize( const fvec3_base_t vec )
{
    float len2, inv_len;
    fvec3_t tmp = ZERO_VECT3;

    if ( NULL == vec ) return tmp;

    if ( 0.0f == fvec3_length_abs( vec ) ) return tmp;

    len2 = vec[kX] * vec[kX] + vec[kY] * vec[kY] + vec[kZ] * vec[kZ];
    inv_len = 1.0f / SQRT( len2 );
    LOG_NAN( inv_len );

    tmp.x = vec[kX] * inv_len;
    tmp.y = vec[kY] * inv_len;
    tmp.z = vec[kZ] * inv_len;

    return tmp;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t  fvec3_self_normalize( fvec3_base_t A )
{
    if ( NULL == A ) return bfalse;

    if ( 0.0f != fvec3_length_abs( A ) )
    {
        float len2 = A[kX] * A[kX] + A[kY] * A[kY] + A[kZ] * A[kZ];
        float inv_len = 1.0f / SQRT( len2 );
        LOG_NAN( inv_len );

        A[kX] *= inv_len;
        A[kY] *= inv_len;
        A[kZ] *= inv_len;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t fvec3_self_normalize_to( fvec3_base_t vec, float B )
{
    if ( NULL == vec ) return bfalse;

    if ( 0.0f == B )
    {
        fvec3_self_clear( vec );
        return btrue;
    }

    if ( 0.0f != fvec3_length_abs( vec ) )
    {
        float len2 = vec[kX] * vec[kX] + vec[kY] * vec[kY] + vec[kZ] * vec[kZ];
        float inv_len = B / SQRT( len2 );
        LOG_NAN( inv_len );

        vec[kX] *= inv_len;
        vec[kY] *= inv_len;
        vec[kZ] *= inv_len;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE fvec3_t fvec3_cross_product( const fvec3_base_t A, const fvec3_base_t B )
{
    fvec3_t   tmp;

    tmp.x = A[kY] * B[kZ] - A[kZ] * B[kY];
    tmp.y = A[kZ] * B[kX] - A[kX] * B[kZ];
    tmp.z = A[kX] * B[kY] - A[kY] * B[kX];

    return tmp;
}

//--------------------------------------------------------------------------------------------
INLINE float fvec3_decompose( const fvec3_base_t A, const fvec3_base_t NRM, fvec3_base_t PARA, fvec3_base_t PERP )
{
    /// BB@> the normal (NRM) is assumed to be normalized. Try to get this as optimized as possible.

    float dot;

    // error trapping
    if ( NULL == A || NULL == NRM ) return 0.0f;

    // if this is true, there is no reason to run this function
    dot = fvec3_dot_product( A, NRM );

    if ( 0.0f == dot )
    {
        // handle optional parameters
        if ( NULL == PARA && NULL == PERP )
        {
            // no point in doing anything
            return 0.0f;
        }
        else if ( NULL == PARA )
        {
            PERP[kX] = A[kX];
            PERP[kY] = A[kY];
            PERP[kZ] = A[kZ];
        }
        else if ( NULL == PERP )
        {
            PARA[kX] = 0.0f;
            PARA[kY] = 0.0f;
            PARA[kZ] = 0.0f;
        }
        else
        {
            PARA[kX] = 0.0f;
            PARA[kY] = 0.0f;
            PARA[kZ] = 0.0f;

            PERP[kX] = A[kX];
            PERP[kY] = A[kY];
            PERP[kZ] = A[kZ];
        }
    }
    else
    {
        // handle optional parameters
        if ( NULL == PARA && NULL == PERP )
        {
            // no point in doing anything
            return 0.0f;
        }
        else if ( NULL == PARA )
        {
            PERP[kX] = A[kX] - dot * NRM[kX];
            PERP[kY] = A[kY] - dot * NRM[kY];
            PERP[kZ] = A[kZ] - dot * NRM[kZ];
        }
        else if ( NULL == PERP )
        {
            PARA[kX] = dot * NRM[kX];
            PARA[kY] = dot * NRM[kY];
            PARA[kZ] = dot * NRM[kZ];
        }
        else
        {
            PARA[kX] = dot * NRM[kX];
            PARA[kY] = dot * NRM[kY];
            PARA[kZ] = dot * NRM[kZ];

            PERP[kX] = A[kX] - PARA[kX];
            PERP[kY] = A[kY] - PARA[kY];
            PERP[kZ] = A[kZ] - PARA[kZ];
        }
    }

    return dot;
}

//--------------------------------------------------------------------------------------------
INLINE float fvec3_dist_abs( const fvec3_base_t A, const fvec3_base_t B )
{
    return ABS( A[kX] - B[kX] ) + ABS( A[kY] - B[kY] ) + ABS( A[kZ] - B[kZ] );
}

//--------------------------------------------------------------------------------------------
INLINE float   fvec3_dot_product( const fvec3_base_t A, const fvec3_base_t B )
{
    return A[kX]*B[kX] + A[kY]*B[kY] + A[kZ]*B[kZ];
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE bool_t fvec4_self_clear( fvec4_t * A )
{
    if ( NULL == A ) return bfalse;

    A->x = A->y = A->z = 0.0f;
    A->w = 1.0f;

    return btrue;
}

//--------------------------------------------------------------------------------------------
// MATIX FUNCTIONS
//--------------------------------------------------------------------------------------------
INLINE fvec3_t mat_getTranslate( const fmat_4x4_t mat )
{
    fvec3_t   pos;

    pos.x = mat.CNV( 3, 0 );
    pos.y = mat.CNV( 3, 1 );
    pos.z = mat.CNV( 3, 2 );

    return pos;
}

//--------------------------------------------------------------------------------------------
INLINE float * mat_getTranslate_v( const fmat_4x4_t mat )
{
    static fvec3_t pos;

    pos.x = mat.CNV( 3, 0 );
    pos.y = mat.CNV( 3, 1 );
    pos.z = mat.CNV( 3, 2 );

    return pos.v;
}

//--------------------------------------------------------------------------------------------
INLINE fvec3_t mat_getChrUp( const fmat_4x4_t mat )
{
    fvec3_t   up;

    // for a character
    up.x = mat.CNV( 2, 0 );
    up.y = mat.CNV( 2, 1 );
    up.z = mat.CNV( 2, 2 );

    return up;
}

//--------------------------------------------------------------------------------------------
INLINE fvec3_t mat_getChrForward( const fmat_4x4_t mat )
{
    fvec3_t   right;

    // for a character
    right.x = -mat.CNV( 0, 0 );
    right.y = -mat.CNV( 0, 1 );
    right.z = -mat.CNV( 0, 2 );

    return right;
}

//--------------------------------------------------------------------------------------------
INLINE fvec3_t mat_getChrRight( const fmat_4x4_t mat )
{
    fvec3_t   frw;

    // for a character's matrix
    frw.x = mat.CNV( 1, 0 );
    frw.y = mat.CNV( 1, 1 );
    frw.z = mat.CNV( 1, 2 );

    return frw;
}

//--------------------------------------------------------------------------------------------
INLINE fvec3_t mat_getCamUp( const fmat_4x4_t mat )
{
    fvec3_t   up;

    // for the camera
    up.x = -mat.CNV( 0, 1 );
    up.y = -mat.CNV( 1, 1 );
    up.z = -mat.CNV( 2, 1 );

    return up;
}

//--------------------------------------------------------------------------------------------
INLINE fvec3_t mat_getCamRight( const fmat_4x4_t mat )
{
    fvec3_t   right;

    // for the camera
    right.x = mat.CNV( 0, 0 );
    right.y = mat.CNV( 1, 0 );
    right.z = mat.CNV( 2, 0 );

    return right;
}

//--------------------------------------------------------------------------------------------
INLINE fvec3_t mat_getCamForward( const fmat_4x4_t mat )
{
    fvec3_t   frw;

    // for the camera
    frw.x = mat.CNV( 0, 2 );
    frw.y = mat.CNV( 1, 2 );
    frw.z = mat.CNV( 2, 2 );

    return frw;
}

//--------------------------------------------------------------------------------------------
INLINE fmat_4x4_t IdentityMatrix()
{
    fmat_4x4_t tmp;

    tmp.CNV( 0, 0 ) = 1; tmp.CNV( 1, 0 ) = 0; tmp.CNV( 2, 0 ) = 0; tmp.CNV( 3, 0 ) = 0;
    tmp.CNV( 0, 1 ) = 0; tmp.CNV( 1, 1 ) = 1; tmp.CNV( 2, 1 ) = 0; tmp.CNV( 3, 1 ) = 0;
    tmp.CNV( 0, 2 ) = 0; tmp.CNV( 1, 2 ) = 0; tmp.CNV( 2, 2 ) = 1; tmp.CNV( 3, 2 ) = 0;
    tmp.CNV( 0, 3 ) = 0; tmp.CNV( 1, 3 ) = 0; tmp.CNV( 2, 3 ) = 0; tmp.CNV( 3, 3 ) = 1;

    return( tmp );
}

//--------------------------------------------------------------------------------------------
INLINE fmat_4x4_t ZeroMatrix( void )
{
    // initializes matrix to zero

    fmat_4x4_t ret;
    int i, j;

    for ( i = 0; i < 4; i++ )
    {
        for ( j = 0; j < 4; j++ )
        {
            ret.CNV( i, j ) = 0;
        }
    }

    return ret;
}

//--------------------------------------------------------------------------------------------
INLINE fmat_4x4_t MatrixMult( const fmat_4x4_t a, const fmat_4x4_t b )
{
    fmat_4x4_t ret = ZERO_MAT_4X4;
    int i, j, k;

    for ( i = 0; i < 4; i++ )
    {
        for ( j = 0; j < 4; j++ )
        {
            for ( k = 0; k < 4; k++ )
            {
                ret.CNV( i, j ) += a.CNV( k, j ) * b.CNV( i, k );
            }
        }
    }

    return ret;
}

//--------------------------------------------------------------------------------------------
INLINE fmat_4x4_t Translate( const float dx, const float dy, const float dz )
{
    fmat_4x4_t ret = IdentityMatrix();

    ret.CNV( 3, 0 ) = dx;
    ret.CNV( 3, 1 ) = dy;
    ret.CNV( 3, 2 ) = dz;

    return ret;
}

//--------------------------------------------------------------------------------------------
INLINE fmat_4x4_t RotateX( const float rads )
{
    float cosine = COS( rads );
    float sine = SIN( rads );

    fmat_4x4_t ret = IdentityMatrix();

    ret.CNV( 1, 1 ) = cosine;
    ret.CNV( 2, 2 ) = cosine;
    ret.CNV( 1, 2 ) = -sine;
    ret.CNV( 2, 1 ) = sine;

    return ret;
}

//--------------------------------------------------------------------------------------------
INLINE fmat_4x4_t RotateY( const float rads )
{
    float cosine = COS( rads );
    float sine = SIN( rads );

    fmat_4x4_t ret = IdentityMatrix();

    ret.CNV( 0, 0 ) = cosine; // 0,0
    ret.CNV( 2, 2 ) = cosine; // 2,2
    ret.CNV( 0, 2 ) = sine; // 0,2
    ret.CNV( 2, 0 ) = -sine; // 2,0

    return ret;
}

//--------------------------------------------------------------------------------------------
INLINE fmat_4x4_t RotateZ( const float rads )
{
    float cosine = COS( rads );
    float sine = SIN( rads );

    fmat_4x4_t ret = IdentityMatrix();

    ret.CNV( 0, 0 ) = cosine; // 0,0
    ret.CNV( 1, 1 ) = cosine; // 1,1
    ret.CNV( 0, 1 ) = -sine; // 0,1
    ret.CNV( 1, 0 ) = sine; // 1,0

    return ret;
}

//--------------------------------------------------------------------------------------------
INLINE fmat_4x4_t ScaleXYZ( const float sizex, const float sizey, const float sizez )
{
    fmat_4x4_t ret = IdentityMatrix();

    ret.CNV( 0, 0 ) = sizex; // 0,0
    ret.CNV( 1, 1 ) = sizey; // 1,1
    ret.CNV( 2, 2 ) = sizez; // 2,2

    return ret;
}

//--------------------------------------------------------------------------------------------
INLINE fmat_4x4_t ScaleXYZRotateXYZTranslate_SpaceFixed( const float scale_x, const float scale_y, const float scale_z, const Uint16 turn_z, const Uint16 turn_x, const Uint16 turn_y, const float translate_x, const float translate_y, const float translate_z )
{
    fmat_4x4_t ret;

    float cx = turntocos[ turn_x & TRIG_TABLE_MASK ];
    float sx = turntosin[ turn_x & TRIG_TABLE_MASK ];
    float cy = turntocos[ turn_y & TRIG_TABLE_MASK ];
    float sy = turntosin[ turn_y & TRIG_TABLE_MASK ];
    float cz = turntocos[ turn_z & TRIG_TABLE_MASK ];
    float sz = turntosin[ turn_z & TRIG_TABLE_MASK ];

    ret.CNV( 0, 0 ) = scale_x * ( cz * cy );
    ret.CNV( 0, 1 ) = scale_x * ( cz * sy * sx + sz * cx );
    ret.CNV( 0, 2 ) = scale_x * ( sz * sx - cz * sy * cx );
    ret.CNV( 0, 3 ) = 0.0f;

    ret.CNV( 1, 0 ) = scale_y * ( -sz * cy );
    ret.CNV( 1, 1 ) = scale_y * ( -sz * sy * sx + cz * cx );
    ret.CNV( 1, 2 ) = scale_y * ( sz * sy * cx + cz * sx );
    ret.CNV( 1, 3 ) = 0.0f;

    ret.CNV( 2, 0 ) = scale_z * ( sy );
    ret.CNV( 2, 1 ) = scale_z * ( -cy * sx );
    ret.CNV( 2, 2 ) = scale_z * ( cy * cx );
    ret.CNV( 2, 3 ) = 0.0f;

    ret.CNV( 3, 0 ) = translate_x;
    ret.CNV( 3, 1 ) = translate_y;
    ret.CNV( 3, 2 ) = translate_z;
    ret.CNV( 3, 3 ) = 1.0f;

    return ret;
}

//--------------------------------------------------------------------------------------------
INLINE fmat_4x4_t ScaleXYZRotateXYZTranslate_BodyFixed( const float scale_x, const float scale_y, const float scale_z, const Uint16 turn_z, const Uint16 turn_x, const Uint16 turn_y, const float translate_x, const float translate_y, const float translate_z )
{
    /// @details BB@> Transpose the SpaceFixed representation and invert the angles to get the BodyFixed representation

    fmat_4x4_t ret;

    float cx = turntocos[ turn_x & TRIG_TABLE_MASK ];
    float sx = turntosin[ turn_x & TRIG_TABLE_MASK ];
    float cy = turntocos[ turn_y & TRIG_TABLE_MASK ];
    float sy = turntosin[ turn_y & TRIG_TABLE_MASK ];
    float cz = turntocos[ turn_z & TRIG_TABLE_MASK ];
    float sz = turntosin[ turn_z & TRIG_TABLE_MASK ];

    //ret.CNV( 0, 0 ) = scale_x * ( cz * cy);
    //ret.CNV( 0, 1 ) = scale_x * ( sz * cy);
    //ret.CNV( 0, 2 ) = scale_x * (-sy);
    //ret.CNV( 0, 3 ) = 0.0f;

    //ret.CNV( 1, 0 ) = scale_y * (-sz * cx + cz * sy * sx);
    //ret.CNV( 1, 1 ) = scale_y * ( cz * cx + sz * sy * sx);
    //ret.CNV( 1, 2 ) = scale_y * ( cy * sx);
    //ret.CNV( 1, 3 ) = 0.0f;

    //ret.CNV( 2, 0 ) = scale_z * ( sz * sx + cz * sy * cx);
    //ret.CNV( 2, 1 ) = scale_z * (-cz * sx + sz * sy * cx);
    //ret.CNV( 2, 2 ) = scale_z * ( cy * cx);
    //ret.CNV( 2, 3 ) = 0.0f;

    ret.CNV( 0, 0 ) = scale_x * ( cz * cy - sz * sy * sx );
    ret.CNV( 0, 1 ) = scale_x * ( sz * cy + cz * sy * sx );
    ret.CNV( 0, 2 ) = scale_x * ( -cx * sy );
    ret.CNV( 0, 3 ) = 0.0f;

    ret.CNV( 1, 0 ) = scale_y * ( -sz * cx );
    ret.CNV( 1, 1 ) = scale_y * ( cz * cx );
    ret.CNV( 1, 2 ) = scale_y * ( sx );
    ret.CNV( 1, 3 ) = 0.0f;

    ret.CNV( 2, 0 ) = scale_z * ( cz * sy + sz * sx * cy );
    ret.CNV( 2, 1 ) = scale_z * ( sz * sy - cz * sx * cy );
    ret.CNV( 2, 2 ) = scale_z * ( cy * cx );
    ret.CNV( 2, 3 ) = 0.0f;

    ret.CNV( 3, 0 ) = translate_x;
    ret.CNV( 3, 1 ) = translate_y;
    ret.CNV( 3, 2 ) = translate_z;
    ret.CNV( 3, 3 ) = 1.0f;

    return ret;
}

//--------------------------------------------------------------------------------------------
INLINE fmat_4x4_t FourPoints( const fvec4_base_t ori, const fvec4_base_t wid, const fvec4_base_t frw, const fvec4_base_t up, const float scale )
{
    fmat_4x4_t tmp;

    fvec3_t vWid, vFor, vUp;

    vWid.x = wid[kX] - ori[kX];
    vWid.y = wid[kY] - ori[kY];
    vWid.z = wid[kZ] - ori[kZ];

    vUp.x = up[kX] - ori[kX];
    vUp.y = up[kY] - ori[kY];
    vUp.z = up[kZ] - ori[kZ];

    vFor.x = frw[kX] - ori[kX];
    vFor.y = frw[kY] - ori[kY];
    vFor.z = frw[kZ] - ori[kZ];

    fvec3_self_normalize( vWid.v );
    fvec3_self_normalize( vUp.v );
    fvec3_self_normalize( vFor.v );

    tmp.CNV( 0, 0 ) = -scale * vWid.x;  // HUK
    tmp.CNV( 0, 1 ) = -scale * vWid.y;  // HUK
    tmp.CNV( 0, 2 ) = -scale * vWid.z;  // HUK
    tmp.CNV( 0, 3 ) = 0.0f;

    tmp.CNV( 1, 0 ) = scale * vFor.x;
    tmp.CNV( 1, 1 ) = scale * vFor.y;
    tmp.CNV( 1, 2 ) = scale * vFor.z;
    tmp.CNV( 1, 3 ) = 0.0f;

    tmp.CNV( 2, 0 ) = scale * vUp.x;
    tmp.CNV( 2, 1 ) = scale * vUp.y;
    tmp.CNV( 2, 2 ) = scale * vUp.z;
    tmp.CNV( 2, 3 ) = 0.0f;

    tmp.CNV( 3, 0 ) = ori[kX];
    tmp.CNV( 3, 1 ) = ori[kY];
    tmp.CNV( 3, 2 ) = ori[kZ];
    tmp.CNV( 3, 3 ) = 1.0f;

    return tmp;
}

//--------------------------------------------------------------------------------------------
INLINE fmat_4x4_t ViewMatrix( const fvec3_base_t   from,     // camera location
                              const fvec3_base_t   at,        // camera look-at target
                              const fvec3_base_t   world_up,  // world’s up, usually 0, 0, 1
                              const float roll )         // clockwise roll around
//   viewing direction,
//   in radians
{
    /// @details MN@> This probably should be replaced by a call to gluLookAt(),
    ///               don't see why we need to make our own...

    fmat_4x4_t view = IdentityMatrix();
    fvec3_t   up, right, view_dir, temp;

    temp     = fvec3_sub( at, from );
    view_dir = fvec3_normalize( temp.v );
    right    = fvec3_cross_product( world_up, view_dir.v );
    up       = fvec3_cross_product( view_dir.v, right.v );
    fvec3_self_normalize( right.v );
    fvec3_self_normalize( up.v );

    view.CNV( 0, 0 ) = right.x;
    view.CNV( 1, 0 ) = right.y;
    view.CNV( 2, 0 ) = right.z;
    view.CNV( 0, 1 ) = up.x;
    view.CNV( 1, 1 ) = up.y;
    view.CNV( 2, 1 ) = up.z;
    view.CNV( 0, 2 ) = view_dir.x;
    view.CNV( 1, 2 ) = view_dir.y;
    view.CNV( 2, 2 ) = view_dir.z;
    view.CNV( 3, 0 ) = -fvec3_dot_product( right.v,    from );
    view.CNV( 3, 1 ) = -fvec3_dot_product( up.v,       from );
    view.CNV( 3, 2 ) = -fvec3_dot_product( view_dir.v, from );

    if ( roll != 0.0f )
    {
        // MatrixMult function shown above
        view = MatrixMult( RotateZ( -roll ), view );
    }

    return view;
}

//--------------------------------------------------------------------------------------------
INLINE fmat_4x4_t ProjectionMatrix( const float near_plane,    // distance to near clipping plane
                                    const float far_plane,      // distance to far clipping plane
                                    const float fov )           // field of view angle, in radians
{
    /// @details MN@> Again, there is a gl function for this, glFrustum or gluPerspective...
    ///               does this account for viewport ratio?

    fmat_4x4_t ret = ZERO_MAT_4X4;

    float c = COS( fov * 0.5f );
    float s = SIN( fov * 0.5f );
    float Q = s / ( 1.0f - near_plane / far_plane );

    ret.CNV( 0, 0 ) = c;         // 0,0
    ret.CNV( 1, 1 ) = c;         // 1,1
    ret.CNV( 2, 2 ) = Q;         // 2,2
    ret.CNV( 3, 2 ) = -Q * near_plane; // 3,2
    ret.CNV( 2, 3 ) = s;         // 2,3

    return ret;
}

//----------------------------------------------------
INLINE void  TransformVertices( const fmat_4x4_t *pMatrix, const fvec4_t *pSourceV, fvec4_t *pDestV, const Uint32 NumVertor )
{
    /// @details  GS@> This is just a MulVectorMatrix for now. The W division and screen size multiplication
    ///                must be done afterward.
    ///
    /// BB@> the matrix transformation for OpenGL vertices. Some minor optimizations.
    ///      The value pSourceV->w is assumed to be constant for all of the elements of pSourceV

    Uint32    cnt;
    fvec4_t * SourceIt = ( fvec4_t * )pSourceV;

    if ( 1.0f == SourceIt->w )
    {
        for ( cnt = 0; cnt < NumVertor; cnt++ )
        {
            pDestV->x = SourceIt->x * pMatrix->v[0] + SourceIt->y * pMatrix->v[4] + SourceIt->z * pMatrix->v[ 8] + pMatrix->v[12];
            pDestV->y = SourceIt->x * pMatrix->v[1] + SourceIt->y * pMatrix->v[5] + SourceIt->z * pMatrix->v[ 9] + pMatrix->v[13];
            pDestV->z = SourceIt->x * pMatrix->v[2] + SourceIt->y * pMatrix->v[6] + SourceIt->z * pMatrix->v[10] + pMatrix->v[14];
            pDestV->w = SourceIt->x * pMatrix->v[3] + SourceIt->y * pMatrix->v[7] + SourceIt->z * pMatrix->v[11] + pMatrix->v[15];

            pDestV++;
            SourceIt++;
        }
    }
    else if ( 0.0f == SourceIt->w )
    {
        for ( cnt = 0; cnt < NumVertor; cnt++ )
        {
            pDestV->x = SourceIt->x * pMatrix->v[0] + SourceIt->y * pMatrix->v[4] + SourceIt->z * pMatrix->v[ 8];
            pDestV->y = SourceIt->x * pMatrix->v[1] + SourceIt->y * pMatrix->v[5] + SourceIt->z * pMatrix->v[ 9];
            pDestV->z = SourceIt->x * pMatrix->v[2] + SourceIt->y * pMatrix->v[6] + SourceIt->z * pMatrix->v[10];
            pDestV->w = SourceIt->x * pMatrix->v[3] + SourceIt->y * pMatrix->v[7] + SourceIt->z * pMatrix->v[11];

            pDestV++;
            SourceIt++;
        }
    }
    else
    {
        for ( cnt = 0; cnt < NumVertor; cnt++ )
        {
            pDestV->x = SourceIt->x * pMatrix->v[0] + SourceIt->y * pMatrix->v[4] + SourceIt->z * pMatrix->v[8]  + SourceIt->w * pMatrix->v[12];
            pDestV->y = SourceIt->x * pMatrix->v[1] + SourceIt->y * pMatrix->v[5] + SourceIt->z * pMatrix->v[9]  + SourceIt->w * pMatrix->v[13];
            pDestV->z = SourceIt->x * pMatrix->v[2] + SourceIt->y * pMatrix->v[6] + SourceIt->z * pMatrix->v[10] + SourceIt->w * pMatrix->v[14];
            pDestV->w = SourceIt->x * pMatrix->v[3] + SourceIt->y * pMatrix->v[7] + SourceIt->z * pMatrix->v[11] + SourceIt->w * pMatrix->v[15];

            pDestV++;
            SourceIt++;
        }
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define egoboo_math_inl