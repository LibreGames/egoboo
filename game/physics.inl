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

/// @file physics.inl
/// @details Put all the functions that need to be fast in this file
///
/// @notes
/// The test functions are designed to detect an interaction with the "least possible" computation.
/// Don't spoil the optimization by calling a test_interaction* function and then a get_depth* function
///
/// The numbers attached to these functions signify the level of precision that is used in calculating the
/// collision. The smallest number, 0, indicates the least precise collision and is equivalent to the "old" Egoboo method.
///
/// The _close_ keyword indicates that you are checking for something like whether a character is able to
/// stand on a platform or something where it will tend to fall off if it starts to step off the edge.
///
/// Use the test_platform flag if you want to test whether the objects are close enough for some platform interaction.
/// You could determine whether this should be set to btrue by determining whether either of the objects was a platform
/// and whether the other object could use the platform.
///
/// If you definitely are going to need the depth info, make sure to use the get_depth* functions with the break_out
/// flag set to bfalse. Setting break_out to btrue will make the function faster in the case that there is no collision,
/// but it will leave some of the "depth vector" uncalculated, which might leave it with uninitialized data.

#include "physics.h"

#include "egoboo_math.inl"

//--------------------------------------------------------------------------------------------
// FORWARD DECLARATIONS
//--------------------------------------------------------------------------------------------

INLINE bool_t test_interaction_0( ego_bumper bump_a, fvec3_t pos_a, ego_bumper bump_b, fvec3_t pos_b, int test_platform );
INLINE bool_t test_interaction_1( ego_oct_bb & cv_a,   fvec3_t pos_a, ego_bumper bump_b, fvec3_t pos_b, int test_platform );
INLINE bool_t test_interaction_2( ego_oct_bb & cv_a,   fvec3_t pos_a, ego_oct_bb & cv_b, fvec3_t pos_b, int test_platform );
INLINE bool_t test_interaction_close_0( ego_bumper bump_a, fvec3_t pos_a, ego_bumper bump_b, fvec3_t pos_b, int test_platform );
INLINE bool_t test_interaction_close_1( ego_oct_bb & cv_a,   fvec3_t pos_a, ego_bumper bump_b, fvec3_t pos_b, int test_platform );
INLINE bool_t test_interaction_close_2( ego_oct_bb & cv_a,   fvec3_t pos_a, ego_oct_bb & cv_b, fvec3_t pos_b, int test_platform );

INLINE bool_t get_depth_0( ego_bumper bump_a, fvec3_t pos_a, ego_bumper bump_b, fvec3_t pos_b, bool_t break_out, ego_oct_vec & depth );
INLINE bool_t get_depth_1( ego_oct_bb & cv_a,   fvec3_t pos_a, ego_bumper bump_b, fvec3_t pos_b, bool_t break_out, ego_oct_vec & depth );
INLINE bool_t get_depth_2( ego_oct_bb & cv_a,   fvec3_t pos_a, ego_oct_bb & cv_b, fvec3_t pos_b, bool_t break_out, ego_oct_vec & depth );
INLINE bool_t get_depth_close_0( ego_bumper bump_a, fvec3_t pos_a, ego_bumper bump_b, fvec3_t pos_b, bool_t break_out, ego_oct_vec & depth );
INLINE bool_t get_depth_close_1( ego_oct_bb & cv_a,   fvec3_t pos_a, ego_bumper bump_b, fvec3_t pos_b, bool_t break_out, ego_oct_vec & depth );
INLINE bool_t get_depth_close_2( ego_oct_bb & cv_a,   fvec3_t pos_a, ego_oct_bb & cv_b, fvec3_t pos_b, bool_t break_out, ego_oct_vec & depth );

INLINE void phys_data_blank_accumulators( ego_phys_data * pdata );

INLINE bool_t phys_data_accumulate_apos_coll( ego_phys_data * pdata, const fvec3_base_t acc );
INLINE bool_t phys_data_accumulate_apos_plat( ego_phys_data * pdata, const fvec3_base_t acc );
INLINE bool_t phys_data_accumulate_avel( ego_phys_data * pdata, const fvec3_base_t acc );

INLINE bool_t phys_data_accumulate_apos_coll_index( ego_phys_data * pdata, const float acc, const int index );
INLINE bool_t phys_data_accumulate_apos_plat_index( ego_phys_data * pdata, const float acc, const int index );
INLINE bool_t phys_data_accumulate_avel_index( ego_phys_data * pdata, const float acc, const int index );

//--------------------------------------------------------------------------------------------
// IMPLEMENTATION
//--------------------------------------------------------------------------------------------
INLINE bool_t test_interaction_close_0( ego_bumper bump_a, fvec3_t pos_a, ego_bumper bump_b, fvec3_t pos_b, int test_platform )
{
    /// @details BB@> Test whether two objects could interact based on the "collision bounding box"
    ///               This version is for character-particle collisions

    ego_oct_bb   cv_a, cv_b;

    // convert the bumpers to the correct format
    bumper_to_oct_bb_0( bump_a, &cv_a );
    bumper_to_oct_bb_0( bump_b, &cv_b );

    return test_interaction_close_2( cv_a, pos_a, cv_b, pos_b, test_platform );
}

//--------------------------------------------------------------------------------------------
INLINE bool_t test_interaction_0( ego_bumper bump_a, fvec3_t pos_a, ego_bumper bump_b, fvec3_t pos_b, int test_platform )
{
    /// @details BB@> Test whether two objects could interact based on the "collision bounding box"
    ///               This version is for character-particle collisions

    ego_oct_bb   cv_a, cv_b;

    // convert the bumpers to the correct format
    bumper_to_oct_bb_0( bump_a, &cv_a );
    bumper_to_oct_bb_0( bump_b, &cv_b );

    return test_interaction_2( cv_a, pos_a, cv_b, pos_b, test_platform );
}

//--------------------------------------------------------------------------------------------
INLINE bool_t test_interaction_close_1( ego_oct_bb & cv_a, fvec3_t pos_a, ego_bumper bump_b, fvec3_t pos_b, int test_platform )
{
    /// @details BB@> Test whether two objects could interact based on the "collision bounding box"
    ///               This version is for character-particle collisions

    ego_oct_bb   cv_b;

    // convert the bumper to the correct format
    bumper_to_oct_bb_0( bump_b, &cv_b );

    return test_interaction_close_2( cv_a, pos_a, cv_b, pos_b, test_platform );
}

//--------------------------------------------------------------------------------------------
INLINE bool_t test_interaction_1( ego_oct_bb & cv_a, fvec3_t pos_a, ego_bumper bump_b, fvec3_t pos_b, int test_platform )
{
    /// @details BB@> Test whether two objects could interact based on the "collision bounding box"
    ///               This version is for character-particle collisions

    ego_oct_bb   cv_b;

    // convert the bumper to the correct format
    bumper_to_oct_bb_0( bump_b, &cv_b );

    return test_interaction_2( cv_a, pos_a, cv_b, pos_b, test_platform );
}

//--------------------------------------------------------------------------------------------
INLINE bool_t test_interaction_close_2( ego_oct_bb & cv_a, fvec3_t pos_a, ego_oct_bb & cv_b, fvec3_t pos_b, int test_platform )
{
    /// @details BB@> Test whether two objects could interact based on the "collision bounding box"
    ///               This version is for character-character collisions

    int cnt;
    float depth;
    ego_oct_vec oa, ob;

    // translate the positions to oct_vecs
    ego_oct_vec::ctor( &oa, pos_a );
    ego_oct_vec::ctor( &ob, pos_b );

    // calculate the depth
    for ( cnt = 0; cnt < OCT_Z; cnt++ )
    {
        float ftmp1 = MIN(( ob[cnt] + cv_b.maxs[cnt] ) - oa[cnt], oa[cnt] - ( ob[cnt] + cv_b.mins[cnt] ) );
        float ftmp2 = MIN(( oa[cnt] + cv_a.maxs[cnt] ) - ob[cnt], ob[cnt] - ( oa[cnt] + cv_a.mins[cnt] ) );
        depth = MAX( ftmp1, ftmp2 );
        if ( depth <= 0.0f ) return bfalse;
    }

    // treat the z coordinate the same as always
    depth = MIN( cv_b.maxs[OCT_Z] + ob[OCT_Z], cv_a.maxs[OCT_Z] + oa[OCT_Z] ) -
            MAX( cv_b.mins[OCT_Z] + ob[OCT_Z], cv_a.mins[OCT_Z] + oa[OCT_Z] );

    return test_platform ? ( depth > -PLATTOLERANCE ) : ( depth > 0.0f );
}

//--------------------------------------------------------------------------------------------
INLINE bool_t test_interaction_2( ego_oct_bb & cv_a, fvec3_t pos_a, ego_oct_bb & cv_b, fvec3_t pos_b, int test_platform )
{
    /// @details BB@> Test whether two objects could interact based on the "collision bounding box"
    ///               This version is for character-character collisions

    int cnt;
    ego_oct_vec oa, ob;
    float depth;

    // translate the positions to oct_vecs
    ego_oct_vec::ctor( &oa, pos_a );
    ego_oct_vec::ctor( &ob, pos_b );

    // calculate the depth
    for ( cnt = 0; cnt < OCT_Z; cnt++ )
    {
        depth  = MIN( cv_b.maxs[cnt] + ob[cnt], cv_a.maxs[cnt] + oa[cnt] ) -
                 MAX( cv_b.mins[cnt] + ob[cnt], cv_a.mins[cnt] + oa[cnt] );

        if ( depth <= 0.0f ) return bfalse;
    }

    // treat the z coordinate the same as always
    depth = MIN( cv_b.maxs[OCT_Z] + ob[OCT_Z], cv_a.maxs[OCT_Z] + oa[OCT_Z] ) -
            MAX( cv_b.mins[OCT_Z] + ob[OCT_Z], cv_a.mins[OCT_Z] + oa[OCT_Z] );

    return ( 0 != test_platform ) ? ( depth > -PLATTOLERANCE ) : ( depth > 0.0f );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE bool_t get_depth_close_0( ego_bumper bump_a, fvec3_t pos_a, ego_bumper bump_b, fvec3_t pos_b, bool_t break_out, ego_oct_vec & depth )
{
    /// @details BB@> Estimate the depth of collision based on the "collision bounding box"
    ///               This version is for character-particle collisions

    ego_oct_bb   cv_a, cv_b;

    // convert the bumpers to the correct format
    bumper_to_oct_bb_0( bump_a, &cv_a );
    bumper_to_oct_bb_0( bump_b, &cv_b );

    return get_depth_close_2( cv_a, pos_a, cv_b, pos_b, break_out, depth );
}

//--------------------------------------------------------------------------------------------
INLINE bool_t get_depth_0( ego_bumper bump_a, fvec3_t pos_a, ego_bumper bump_b, fvec3_t pos_b, bool_t break_out, ego_oct_vec & depth )
{
    /// @details BB@> Estimate the depth of collision based on the "collision bounding box"
    ///               This version is for character-particle collisions

    ego_oct_bb   cv_a, cv_b;

    // convert the bumpers to the correct format
    bumper_to_oct_bb_0( bump_a, &cv_a );
    bumper_to_oct_bb_0( bump_b, &cv_b );

    // convert the bumper to the correct format
    bumper_to_oct_bb_0( bump_b, &cv_b );

    return get_depth_2( cv_a, pos_a, cv_b, pos_b, break_out, depth );
}

//--------------------------------------------------------------------------------------------
INLINE bool_t get_depth_close_1( ego_oct_bb & cv_a, fvec3_t pos_a, ego_bumper bump_b, fvec3_t pos_b, bool_t break_out, ego_oct_vec & depth )
{
    /// @details BB@> Estimate the depth of collision based on the "collision bounding box"
    ///               This version is for character-particle collisions

    ego_oct_bb   cv_b;

    // convert the bumper to the correct format
    bumper_to_oct_bb_0( bump_b, &cv_b );

    return get_depth_close_2( cv_a, pos_a, cv_b, pos_b, break_out, depth );
}

//--------------------------------------------------------------------------------------------
INLINE bool_t get_depth_1( ego_oct_bb & cv_a, fvec3_t pos_a, ego_bumper bump_b, fvec3_t pos_b, bool_t break_out, ego_oct_vec & depth )
{
    /// @details BB@> Estimate the depth of collision based on the "collision bounding box"
    ///               This version is for character-particle collisions

    ego_oct_bb   cv_b;

    // convert the bumper to the correct format
    bumper_to_oct_bb_0( bump_b, &cv_b );

    return get_depth_2( cv_a, pos_a, cv_b, pos_b, break_out, depth );
}

//--------------------------------------------------------------------------------------------
INLINE bool_t get_depth_close_2( ego_oct_bb & cv_a, fvec3_t pos_a, ego_oct_bb & cv_b, fvec3_t pos_b, bool_t break_out, ego_oct_vec & depth )
{
    /// @details BB@> Estimate the depth of collision based on the "collision bounding box"
    ///               This version is for character-character collisions

    int cnt;
    ego_oct_vec oa, ob;
    bool_t valid;

    // translate the positions to oct_vecs
    ego_oct_vec::ctor( &oa, pos_a );
    ego_oct_vec::ctor( &ob, pos_b );

    // calculate the depth
    valid = btrue;
    for ( cnt = 0; cnt < OCT_Z; cnt++ )
    {
        float ftmp1 = MIN(( ob[cnt] + cv_b.maxs[cnt] ) - oa[cnt], oa[cnt] - ( ob[cnt] + cv_b.mins[cnt] ) );
        float ftmp2 = MIN(( oa[cnt] + cv_a.maxs[cnt] ) - ob[cnt], ob[cnt] - ( oa[cnt] + cv_a.mins[cnt] ) );
        depth[cnt] = MAX( ftmp1, ftmp2 );

        if ( depth[cnt] <= 0.0f )
        {
            valid = bfalse;
            if ( break_out ) return bfalse;
        }
    }

    // treat the z coordinate the same as always
    depth[OCT_Z]  = MIN( cv_b.maxs[OCT_Z] + ob[OCT_Z], cv_a.maxs[OCT_Z] + oa[OCT_Z] ) -
                    MAX( cv_b.mins[OCT_Z] + ob[OCT_Z], cv_a.mins[OCT_Z] + oa[OCT_Z] );

    if ( depth[OCT_Z] <= 0.0f )
    {
        valid = bfalse;
        if ( break_out ) return bfalse;
    }

    // scale the diagonal components so that they are actually distances
    depth[OCT_XY] *= INV_SQRT_TWO;
    depth[OCT_YX] *= INV_SQRT_TWO;

    return valid;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t get_depth_2( ego_oct_bb & cv_a, fvec3_t pos_a, ego_oct_bb & cv_b, fvec3_t pos_b, bool_t break_out, ego_oct_vec & depth )
{
    /// @details BB@> Estimate the depth of collision based on the "collision bounding box"
    ///               This version is for character-character collisions

    int cnt;
    ego_oct_vec oa, ob;
    bool_t valid;

    // translate the positions to oct_vecs
    ego_oct_vec::ctor( &oa, pos_a );
    ego_oct_vec::ctor( &ob, pos_b );

    // calculate the depth
    valid = btrue;
    for ( cnt = 0; cnt < OCT_COUNT; cnt++ )
    {
        depth[cnt]  = MIN( cv_b.maxs[cnt] + ob[cnt], cv_a.maxs[cnt] + oa[cnt] ) -
                      MAX( cv_b.mins[cnt] + ob[cnt], cv_a.mins[cnt] + oa[cnt] );

        if ( depth[cnt] <= 0.0f )
        {
            valid = bfalse;
            if ( break_out ) return bfalse;
        }
    }

    // scale the diagonal components so that they are actually distances
    depth[OCT_XY] *= INV_SQRT_TWO;
    depth[OCT_YX] *= INV_SQRT_TWO;

    return valid;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE void phys_data_blank_accumulators( ego_phys_data * pdata )
{
    if ( NULL == pdata ) return;

    fvec3_self_clear( pdata->apos_plat.v );
    fvec3_self_clear( pdata->apos_coll.v );
    fvec3_self_clear( pdata->avel.v );
}

//--------------------------------------------------------------------------------------------
INLINE bool_t phys_data_accumulate_avel( ego_phys_data * pdata, const fvec3_base_t acc )
{
    if ( NULL == pdata ) return bfalse;

    //if( fvec3_length_abs(acc) > 100.0f )
    //    acc[kX] = 0.0f;

    return fvec3_self_sum( pdata->avel.v, acc );
}

//--------------------------------------------------------------------------------------------
INLINE bool_t phys_data_accumulate_apos_coll( ego_phys_data * pdata, const fvec3_base_t acc )
{
    if ( NULL == pdata ) return bfalse;

    //if( fvec3_length_abs(acc) > 100.0f )
    //    acc[kX] = 0.0f;

    return fvec3_self_sum( pdata->apos_coll.v, acc );
}

//--------------------------------------------------------------------------------------------
INLINE bool_t phys_data_accumulate_apos_plat( ego_phys_data * pdata, const fvec3_base_t acc )
{
    if ( NULL == pdata ) return bfalse;

    //if( fvec3_length_abs(acc) > 100.0f )
    //    acc[kX] = 0.0f;

    return fvec3_self_sum( pdata->apos_plat.v, acc );
}

//--------------------------------------------------------------------------------------------
INLINE bool_t phys_data_accumulate_avel_index( ego_phys_data * pdata, const float acc, const int index )
{
    bool_t retval = bfalse;

    if ( NULL == pdata ) return bfalse;

    if ( 0.0f == acc ) return btrue;

    //if( ABS(acc) > 100.0f )
    //    acc = 0.0f;

    if ( index >= 0 && index <= kZ )
    {
        pdata->avel.v[index] += acc;
        retval = btrue;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t phys_data_accumulate_apos_coll_index( ego_phys_data * pdata, const float acc, const int index )
{
    bool_t retval = bfalse;

    if ( NULL == pdata ) return bfalse;

    if ( 0.0f == acc ) return btrue;

    //if( ABS(acc) > 100.0f )
    //    acc = 0.0f;

    if ( index >= 0 && index <= kZ )
    {
        pdata->apos_coll.v[index] += acc;
        retval = btrue;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t phys_data_accumulate_apos_plat_index( ego_phys_data * pdata, const float acc, const int index )
{
    bool_t retval = bfalse;

    if ( NULL == pdata ) return bfalse;

    if ( 0.0f == acc ) return btrue;

    //if( ABS(acc) > 100.0f )
    //    acc = 0.0f;

    if ( index >= 0 && index <= kZ )
    {
        pdata->apos_plat.v[index] += acc;
        retval = btrue;
    }

    return retval;
}
