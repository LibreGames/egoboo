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
/// @file bbox.h
/// @brief A small "library" for dealing with various bounding boxes

#include "egoboo_math.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// axis aligned bounding box
struct ego_aabb
{
    float mins[3];
    float maxs[3];
};

//--------------------------------------------------------------------------------------------

/// Level 0 character "bumper"
/// The simplest collision volume, equivalent to the old-style collision data
/// stored in data.txt
struct ego_bumper
{
    float  size;        ///< Size of bumpers
    float  size_big;     ///< For octagonal bumpers
    float  height;      ///< Distance from head to toe
};

//--------------------------------------------------------------------------------------------

/// The various axes for the octagonal bounding box
enum e_octagonal_axes
{
    OCT_X, OCT_Y, OCT_XY, OCT_YX, OCT_Z, OCT_COUNT
};

/// a "vector" that measures distances based on the axes of an octagonal bounding box
typedef float oct_vec_base_t[OCT_COUNT];

struct ego_oct_vec
{
    oct_vec_base_t v;

    ego_oct_vec( ) { clear( this ); }

    ego_oct_vec( oct_vec_base_t & vals ) { clear( this ); SDL_memmove( &v, &vals, sizeof( v ) ); }

    ego_oct_vec( fvec3_t & vec ) { clear( this ); ctor_this( this, vec ); }

    float & operator []( size_t index ) { return v[index]; }

    const float & operator []( size_t index ) const  { return v[index]; }

    static ego_oct_vec * ctor_this( ego_oct_vec * ovec, fvec3_t pos );

private:

    static ego_oct_vec * clear( ego_oct_vec * ptr )
    {
        if ( NULL == ptr ) return NULL;

        SDL_memset( ptr, 0, sizeof( *ptr ) );

        return ptr;
    }
};

//--------------------------------------------------------------------------------------------

/// generic octagonal bounding box
/// to be used for the Level 1 character "bumper"
/// The best possible octagonal bounding volume. A generalization of the old octagonal bounding box
/// values in data.txt. Computed on the fly.

struct ego_oct_bb
{
    ego_oct_vec mins,  maxs;

    ego_oct_bb() : mins(), maxs() {}

    static ego_oct_bb * ctor_this( ego_oct_bb * pobb );
    static bool_t       do_union( ego_oct_bb & src1, ego_oct_bb & src2, ego_oct_bb   * pdst );
    static bool_t       do_intersection( ego_oct_bb & src1, ego_oct_bb & src2, ego_oct_bb   * pdst );
    static bool_t       empty( ego_oct_bb & src1 );

    static void         downgrade( ego_oct_bb * psrc, ego_bumper bump_stt, ego_bumper bump_base, ego_bumper * p_bump, ego_oct_bb   * pdst );
    static bool_t       add_vector( const ego_oct_bb & src, const fvec3_base_t vec, ego_oct_bb   * pdst );

    static egoboo_rv intersect_index( int index, ego_oct_bb & src1, ego_oct_vec & opos1, ego_oct_vec & ovel1, ego_oct_bb & src2, ego_oct_vec & opos2, ego_oct_vec & ovel2, float *tmin, float *tmax );
    static egoboo_rv intersect_index_close( int index, ego_oct_bb & src1, ego_oct_vec & opos1, ego_oct_vec & ovel1, ego_oct_bb & src2, ego_oct_vec & opos2, ego_oct_vec & ovel2, float *tmin, float *tmax );
};

//--------------------------------------------------------------------------------------------

struct ego_lod_aabb
{
    int    sub_used;
    float  weight;

    bool_t used;
    int    level;
    int    address;

    ego_aabb  bb;

    ego_lod_aabb() { used = bfalse; sub_used = level = address = 0; weight = 0.0f; }
};

//--------------------------------------------------------------------------------------------
struct ego_aabb_lst
{
    int            count;
    ego_lod_aabb * list;

    ego_aabb_lst() { clear( this ); }
    ~ego_aabb_lst() { dtor_this( this ); }

    static const ego_aabb_lst   * ctor_this( ego_aabb_lst   * lst );
    static const ego_aabb_lst   * dtor_this( ego_aabb_lst   * lst );
    static const ego_aabb_lst   * renew( ego_aabb_lst   * lst );
    static const ego_aabb_lst   * alloc( ego_aabb_lst   * lst, int count );

private:

    static ego_aabb_lst * clear( ego_aabb_lst * ptr )
    {
        if ( NULL == ptr ) return NULL;

        SDL_memset( ptr, 0, sizeof( *ptr ) );

        return ptr;
    }
};

//--------------------------------------------------------------------------------------------
struct ego_aabb_ary
{
    int         count;
    ego_aabb_lst   * list;

    ego_aabb_ary() { clear( this ); }
    ~ego_aabb_ary() { dtor_this( this ); }

    static ego_aabb_ary * ctor_this( ego_aabb_ary * ary ) { ary = dtor_this( ary ); ary = clear( ary ); return ary; }
    static ego_aabb_ary * dtor_this( ego_aabb_ary * ary );
    static ego_aabb_ary * renew( ego_aabb_ary * ary );
    static ego_aabb_ary * alloc( ego_aabb_ary * ary, int count );

private:
    static ego_aabb_ary * clear( ego_aabb_ary * ptr )
    {
        if ( NULL == ptr ) return ptr;

        SDL_memset( ptr, 0, sizeof( *ptr ) );

        return ptr;
    }
};

//--------------------------------------------------------------------------------------------

/// @details A convex poly representation of an object volume
struct ego_OVolume
{
    int      lod;             ///< the level of detail (LOD) of this volume
    bool_t   needs_shape;     ///< is the shape data valid?
    bool_t   needs_position;  ///< Is the position data valid?

    ego_oct_bb   oct;

    ego_OVolume() { clear( this ); }

    static ego_OVolume do_merge( ego_OVolume * pv1, ego_OVolume * pv2 );
    static ego_OVolume do_intersect( ego_OVolume * pv1, ego_OVolume * pv2 );
    static bool_t      draw( ego_OVolume * cv, bool_t draw_square, bool_t draw_diamond );
    static bool_t      shift( ego_OVolume * cv_src, fvec3_t * pos_src, ego_OVolume *cv_dst );
    static bool_t      unshift( ego_OVolume * cv_src, fvec3_t * pos_src, ego_OVolume *cv_dst );
    static bool_t      refine( ego_OVolume * pov, fvec3_t * pcenter, float * pvolume );

private:
    static ego_OVolume * clear( ego_OVolume * ptr )
    {
        if ( NULL == ptr ) return NULL;

        ptr->lod = -1;
        ptr->needs_shape = ptr->needs_position = bfalse;

        return ptr;
    }
};

//--------------------------------------------------------------------------------------------
struct ego_OTree
{
    ego_OVolume leaf[8];
};

//--------------------------------------------------------------------------------------------

/// @details A convex polygon representation of the collision of two objects
struct ego_CVolume
{
    float          volume;
    fvec3_t        center;
    ego_OVolume    ov;
    ego_OTree    * tree;

    static bool_t ctor_this( ego_CVolume * pcv, ego_OVolume * pva, ego_OVolume * pvb );
    static bool_t refine( ego_CVolume * pcv );
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// type conversion routines

bool_t bumper_to_oct_bb_0( ego_bumper src, ego_oct_bb   * pdst );
bool_t bumper_to_oct_bb_1( ego_bumper src, fvec3_t vel, ego_oct_bb   * pdst );

int    oct_bb_to_points( ego_oct_bb   * pbmp, fvec4_t pos[], size_t pos_count );
void   points_to_oct_bb( ego_oct_bb   * pbmp, fvec4_t pos[], size_t pos_count );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define _egoboo_bbox_h
