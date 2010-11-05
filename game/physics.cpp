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

/// \file physics.c

#include "physics.inl"

#include "game.h"

#include "char.inl"
#include "particle.inl"
#include "mesh.inl"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
float   hillslide       =  1.00f;
float   slippyfriction  =  1.00f;
float   airfriction     =  0.91f;
float   waterfriction   =  0.80f;
float   noslipfriction  =  0.91f;
float   gravity         = STANDARD_GRAVITY;
float   platstick       =  0.50f;
fvec3_t windspeed       = ZERO_VECT3;
fvec3_t waterspeed      = ZERO_VECT3;

static int breadcrumb_guid = 0;

const float air_friction = 0.9868f;
const float ice_friction = 0.9738f;  // the square of air_friction

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static bool_t  phys_apply_normal_acceleration( fvec3_base_t acc, fvec3_base_t nrm, float para_factor, float perp_factor, fvec3_t * pnrm_acc );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t phys_estimate_chr_chr_normal( ego_oct_vec & opos_a, ego_oct_vec & opos_b, ego_oct_vec & odepth, float exponent, fvec3_base_t nrm )
{
    bool_t retval;

    // is everything valid?
    if ( NULL == nrm ) return bfalse;

    // initialize the vector
    nrm[kX] = nrm[kY] = nrm[kZ] = 0.0f;

    if ( odepth[OCT_X] <= 0.0f )
    {
        odepth[OCT_X] = 0.0f;
    }
    else
    {
        float sgn = opos_b[OCT_X] - opos_a[OCT_X];
        sgn = sgn > 0 ? -1 : 1;

        nrm[kX] += sgn / POW( odepth[OCT_X] / PLATTOLERANCE, exponent );
    }

    if ( odepth[OCT_Y] <= 0.0f )
    {
        odepth[OCT_Y] = 0.0f;
    }
    else
    {
        float sgn = opos_b[OCT_Y] - opos_a[OCT_Y];
        sgn = sgn > 0 ? -1 : 1;

        nrm[kY] += sgn / POW( odepth[OCT_Y] / PLATTOLERANCE, exponent );
    }

    if ( odepth[OCT_XY] <= 0.0f )
    {
        odepth[OCT_XY] = 0.0f;
    }
    else
    {
        float sgn = opos_b[OCT_XY] - opos_a[OCT_XY];
        sgn = sgn > 0 ? -1 : 1;

        nrm[kX] += sgn / POW( odepth[OCT_XY] / PLATTOLERANCE, exponent );
        nrm[kY] += sgn / POW( odepth[OCT_XY] / PLATTOLERANCE, exponent );
    }

    if ( odepth[OCT_YX] <= 0.0f )
    {
        odepth[OCT_YX] = 0.0f;
    }
    else
    {
        float sgn = opos_b[OCT_YX] - opos_a[OCT_YX];
        sgn = sgn > 0 ? -1 : 1;
        nrm[kX] -= sgn / POW( odepth[OCT_YX] / PLATTOLERANCE, exponent );
        nrm[kY] += sgn / POW( odepth[OCT_YX] / PLATTOLERANCE, exponent );
    }

    if ( odepth[OCT_Z] <= 0.0f )
    {
        odepth[OCT_Z] = 0.0f;
    }
    else
    {
        float sgn = opos_b[OCT_Z] - opos_a[OCT_Z];

        sgn = sgn > 0 ? -1 : 1;

        nrm[kZ] += sgn / POW( exponent * odepth[OCT_Z] / PLATTOLERANCE, exponent );
    }

    retval = bfalse;
    if ( fvec3_length_abs( nrm ) > 0.0f )
    {
        fvec3_t vtmp = fvec3_normalize( nrm );
        SDL_memcpy( nrm, vtmp.v, sizeof( fvec3_base_t ) );
        retval = btrue;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
egoboo_rv ego_oct_bb::intersect_index( int index, ego_oct_bb & src1, ego_oct_vec & opos1, ego_oct_vec & ovel1, ego_oct_bb & src2, ego_oct_vec & opos2, ego_oct_vec & ovel2, float *tmin, float *tmax )
{
    float diff;
    float time[4];

    if ( NULL == tmin || NULL == tmax ) return rv_error;

    if ( index < 0 || index >= OCT_COUNT ) return rv_error;

    diff = ovel2[index] - ovel1[index];
    if ( diff == 0.0f ) return rv_fail;

    time[0] = (( src1.mins[index] + opos1[index] ) - ( src2.mins[index] + opos2[index] ) ) / diff;
    time[1] = (( src1.mins[index] + opos1[index] ) - ( src2.maxs[index] + opos2[index] ) ) / diff;
    time[2] = (( src1.maxs[index] + opos1[index] ) - ( src2.mins[index] + opos2[index] ) ) / diff;
    time[3] = (( src1.maxs[index] + opos1[index] ) - ( src2.maxs[index] + opos2[index] ) ) / diff;

    *tmin = SDL_min( SDL_min( time[0], time[1] ), SDL_min( time[2], time[3] ) );
    *tmax = SDL_max( SDL_max( time[0], time[1] ), SDL_max( time[2], time[3] ) );

    // normalize the results for the diagonal directions
    if ( OCT_XY == index || OCT_YX == index )
    {
        *tmin *= INV_SQRT_TWO;
        *tmax *= INV_SQRT_TWO;
    }

    if ( *tmax < *tmin ) return rv_fail;

    return rv_success;
}

//--------------------------------------------------------------------------------------------
egoboo_rv ego_oct_bb::intersect_index_close( int index, ego_oct_bb & src1, ego_oct_vec & opos1, ego_oct_vec & ovel1, ego_oct_bb & src2, ego_oct_vec & opos2, ego_oct_vec & ovel2, float *tmin, float *tmax )
{
    egoboo_rv retval = rv_error;
    float     diff;

    if ( NULL == tmin || NULL == tmax ) return rv_error;

    if ( index < 0 || index >= OCT_COUNT ) return rv_error;

    diff = ovel2[index] - ovel1[index];
    if ( diff == 0.0f ) return rv_fail;

    if ( OCT_Z != index )
    {
        // in the horizontal distances, it is just the depth between the center of one object
        // and the edge of the other.

        float time[8];
        float tmp_min1, tmp_max1;
        float tmp_min2, tmp_max2;

        time[0] = ( opos1[index] - ( src2.mins[index] + opos2[index] ) ) / diff;
        time[1] = ( opos1[index] - ( src2.maxs[index] + opos2[index] ) ) / diff;
        time[2] = ( opos1[index] - ( src2.mins[index] + opos2[index] ) ) / diff;
        time[3] = ( opos1[index] - ( src2.maxs[index] + opos2[index] ) ) / diff;
        tmp_min1 = SDL_min( SDL_min( time[0], time[1] ), SDL_min( time[2], time[3] ) );
        tmp_max1 = SDL_max( SDL_max( time[0], time[1] ), SDL_max( time[2], time[3] ) );

        time[4] = (( src1.mins[index] + opos1[index] ) - opos2[index] ) / diff;
        time[5] = (( src1.mins[index] + opos1[index] ) - opos2[index] ) / diff;
        time[6] = (( src1.maxs[index] + opos1[index] ) - opos2[index] ) / diff;
        time[7] = (( src1.maxs[index] + opos1[index] ) - opos2[index] ) / diff;
        tmp_min2 = SDL_min( SDL_min( time[4], time[5] ), SDL_min( time[6], time[7] ) );
        tmp_max2 = SDL_max( SDL_max( time[4], time[5] ), SDL_max( time[6], time[7] ) );

        *tmin = SDL_min( tmp_min1, tmp_min2 );
        *tmax = SDL_max( tmp_max1, tmp_max2 );

        // normalize the results for the diagonal directions
        if ( OCT_XY == index || OCT_YX == index )
        {
            *tmin *= INV_SQRT_TWO;
            *tmax *= INV_SQRT_TWO;
        }

        retval = rv_success;
    }
    else
    {
        // there is no special treatment in the z direction
        retval = ego_oct_bb::intersect_index( index, src1, opos1, ovel1, src2, opos2, ovel2, tmin, tmax );
    }

    if ( *tmax < *tmin ) return rv_fail;

    return rv_success;
}

//--------------------------------------------------------------------------------------------
bool_t phys_intersect_oct_bb( ego_oct_bb & src1_orig, fvec3_t pos1, fvec3_t vel1, ego_oct_bb & src2_orig, fvec3_t pos2, fvec3_t vel2, int test_platform, ego_oct_bb   * pdst, float *tmin, float *tmax )
{
    /// \author BB
    /// \details  A test to determine whether two "fast moving" objects are interacting within a frame.
    ///               Designed to determine whether a bullet particle will interact with character.

    ego_oct_bb    src1, src2;
    ego_oct_bb    exp1, exp2;
    ego_oct_vec opos1, opos2;
    ego_oct_vec ovel1, ovel2;

    int    cnt, index;
    bool_t found;
    float  local_tmin, local_tmax;
    float  tmp_min, tmp_max;

    int    failure_count = 0;
    bool_t failure[OCT_COUNT];

    // handle optional parameters
    if ( NULL == tmin ) tmin = &local_tmin;
    if ( NULL == tmax ) tmax = &local_tmax;

    // convert the position and velocity vectors to octagonal format
    ego_oct_vec::ctor_this( &ovel1, vel1 );
    ego_oct_vec::ctor_this( &opos1, pos1 );

    ego_oct_vec::ctor_this( &ovel2, vel2 );
    ego_oct_vec::ctor_this( &opos2, pos2 );

    // cycle through the coordinates to see when the two volumes might coincide
    found = bfalse;
    *tmin = *tmax = -1.0f;
    if ( 0.0f == fvec3_dist_abs( vel1.v, vel2.v ) )
    {
        // no relative motion, so avoid the loop to save time
        failure_count = OCT_COUNT;
    }
    else
    {
        for ( index = 0; index < OCT_COUNT; index++ )
        {
            egoboo_rv retval;

            if ( PHYS_CLOSE_TOLERANCE_NONE != test_platform )
            {
                retval = ego_oct_bb::intersect_index_close( index, src1_orig, opos1, ovel1, src2_orig, opos2, ovel2, &tmp_min, &tmp_max );
            }
            else
            {
                retval = ego_oct_bb::intersect_index( index, src1_orig, opos1, ovel1, src2_orig, opos2, ovel2, &tmp_min, &tmp_max );
            }

            if ( rv_fail == retval )
            {
                // This case will only occur if the objects are not moving relative to each other.

                failure[index] = btrue;
                failure_count++;
            }
            else if ( rv_success == retval )
            {
                failure[index] = bfalse;

                if ( !found )
                {
                    *tmin = tmp_min;
                    *tmax = tmp_max;
                    found = btrue;
                }
                else
                {
                    *tmin = SDL_max( *tmin, tmp_min );
                    *tmax = SDL_min( *tmax, tmp_max );
                }

                if ( *tmax < *tmin ) return bfalse;
            }
        }
    }

    if ( OCT_COUNT == failure_count )
    {
        // No relative motion on any axis.
        // Just say that they are interacting for the whole frame

        *tmin = 0.0f;
        *tmax = 1.0f;
    }
    else
    {
        // some relative motion found

        // if the objects do not interact this frame let the caller know
        if ( *tmin > 1.0f || *tmax < 0.0f ) return bfalse;

        // limit the negative values of time to the start of the module
        if (( *tmin ) + update_wld < 0 ) *tmin = - Sint32( update_wld ) ;
    }

    if ( NULL != pdst )
    {
        // clip the interaction time to just one frame
        tmp_min = CLIP( *tmin, 0.0f, 1.0f );
        tmp_max = CLIP( *tmax, 0.0f, 1.0f );

        // shift the source bounding boxes to be centered on the given positions
        ego_oct_bb::add_vector( src1_orig, pos1.v, &src1 );
        ego_oct_bb::add_vector( src2_orig, pos2.v, &src2 );

        // determine the expanded collision volumes for both objects
        phys_expand_oct_bb( src1, vel1, tmp_min, tmp_max, &exp1 );
        phys_expand_oct_bb( src2, vel2, tmp_min, tmp_max, &exp2 );

        // determine the intersection of these two volumes
        ego_oct_bb::do_intersection( exp1, exp2, pdst );

        // check to see if there is any possibility of interaction at all
        for ( cnt = 0; cnt < OCT_COUNT; cnt++ )
        {
            if ( pdst->mins[cnt] >= pdst->maxs[cnt] ) return bfalse;
        }
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t phys_expand_oct_bb( ego_oct_bb & src, fvec3_t vel, float tmin, float tmax, ego_oct_bb   * pdst )
{
    /// \author BB
    /// \details  use the velocity of an object and its ego_oct_bb   to determine the
    ///               amount of territory that an object will cover in the range [tmin,tmax].
    ///               One update equals [tmin,tmax] == [0,1].

    float abs_vel;
    ego_oct_bb   tmp_min, tmp_max;

    abs_vel = fvec3_length_abs( vel.v );
    if ( 0.0f == abs_vel )
    {
        if ( NULL != pdst )
        {
            *pdst = src;
        }
        return btrue;
    }

    // determine the bounding volume at t == tmin
    if ( 0.0f == tmin )
    {
        tmp_min = src;
    }
    else
    {
        fvec3_t pos_min;

        pos_min.x = vel.x * tmin;
        pos_min.y = vel.y * tmin;
        pos_min.z = vel.z * tmin;

        // adjust the bounding box to take in the position at the next step
        if ( !ego_oct_bb::add_vector( src, pos_min.v, &tmp_min ) ) return bfalse;
    }

    // determine the bounding volume at t == tmax
    if ( tmax == 0.0f )
    {
        tmp_max = src;
    }
    else
    {
        fvec3_t pos_max;

        pos_max.x = vel.x * tmax;
        pos_max.y = vel.y * tmax;
        pos_max.z = vel.z * tmax;

        // adjust the bounding box to take in the position at the next step
        if ( !ego_oct_bb::add_vector( src, pos_max.v, &tmp_max ) ) return bfalse;
    }

    // determine bounding box for the range of times
    if ( !ego_oct_bb::do_union( tmp_min, tmp_max, pdst ) ) return bfalse;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t phys_expand_chr_bb( ego_chr * pchr, float tmin, float tmax, ego_oct_bb   * pdst )
{
    /// \author BB
    /// \details  use the object velocity to figure out where the volume that the character will
    ///               occupy during this update. Use the loser prt_cv and include extra height if
    ///               it is a platform.

    ego_oct_bb   tmp_oct1, tmp_oct2;

    if ( !PROCESSING_PCHR( pchr ) ) return bfalse;

    // copy the volume
    // the "platform" version was expanded in the z direction, but only if it was needed
    tmp_oct1 = pchr->chr_max_cv;

    // add in the current position to the bounding volume
    ego_oct_bb::add_vector( tmp_oct1, pchr->pos.v, &tmp_oct2 );

    // stretch the bounding volume to cover the path of the object
    return phys_expand_oct_bb( tmp_oct2, pchr->vel, tmin, tmax, pdst );
}

//--------------------------------------------------------------------------------------------
bool_t phys_expand_prt_bb( ego_prt * pprt, float tmin, float tmax, ego_oct_bb   * pdst )
{
    /// \author BB
    /// \details  use the object velocity to figure out where the volume that the particle will
    ///               occupy during this update

    ego_oct_bb   tmp_oct;

    if ( !PROCESSING_PPRT( pprt ) ) return bfalse;

    // add in the current position to the bounding volume
    {
        fvec3_t _tmp_vec = ego_prt::get_pos( pprt );
        ego_oct_bb::add_vector( pprt->prt_cv, _tmp_vec.v, &tmp_oct );
    }

    // stretch the bounding volume to cover the path of the object
    return phys_expand_oct_bb( tmp_oct, pprt->vel, tmin, tmax, pdst );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_breadcrumb * breadcrumb_init_chr( ego_breadcrumb * bc, ego_chr * pchr )
{
    if ( NULL == bc ) return bc;

    SDL_memset( bc, 0, sizeof( ego_breadcrumb ) );
    bc->time = update_wld;

    if ( NULL == pchr ) return bc;

    bc->bits   = pchr->stoppedby;
    bc->radius = pchr->bump_1.size;
    bc->pos.x  = ( FLOOR( pchr->pos.x / GRID_SIZE ) + 0.5f ) * GRID_SIZE;
    bc->pos.y  = ( FLOOR( pchr->pos.y / GRID_SIZE ) + 0.5f ) * GRID_SIZE;
    bc->pos.z  = pchr->pos.z;

    bc->grid   = ego_mpd::get_tile( PMesh, bc->pos.x, bc->pos.y );
    bc->valid  = ( 0 == ego_mpd::test_wall( PMesh, bc->pos.v, bc->radius, bc->bits, NULL ) );

    bc->id = breadcrumb_guid++;

    return bc;
}

//--------------------------------------------------------------------------------------------
ego_breadcrumb * breadcrumb_init_prt( ego_breadcrumb * bc, ego_prt * pprt )
{
    BIT_FIELD bits = 0;
    ego_pip * ppip;

    if ( NULL == bc ) return bc;

    SDL_memset( bc, 0, sizeof( ego_breadcrumb ) );
    bc->time = update_wld;

    if ( NULL == pprt ) return bc;

    ppip = ego_prt::get_ppip( GET_REF_PPRT( pprt ) );
    if ( NULL == ppip ) return bc;

    bits = MPDFX_IMPASS;
    if ( 0 != ppip->bump_money ) ADD_BITS( bits, MPDFX_WALL );

    bc->bits   = bits;
    bc->radius = pprt->bump_real.size;

    bc->pos = ego_prt::get_pos( pprt );
    bc->pos.x  = ( FLOOR( bc->pos.x / GRID_SIZE ) + 0.5f ) * GRID_SIZE;
    bc->pos.y  = ( FLOOR( bc->pos.y / GRID_SIZE ) + 0.5f ) * GRID_SIZE;

    bc->grid   = ego_mpd::get_tile( PMesh, bc->pos.x, bc->pos.y );
    bc->valid  = ( 0 == ego_mpd::test_wall( PMesh, bc->pos.v, bc->radius, bc->bits, NULL ) );

    bc->id = breadcrumb_guid++;

    return bc;
}

//--------------------------------------------------------------------------------------------
int breadcrumb_cmp( const void * lhs, const void * rhs )
{
    // comparison to sort from oldest to newest
    int retval;

    ego_breadcrumb * bc_lhs = ( ego_breadcrumb * )lhs;
    ego_breadcrumb * bc_rhs = ( ego_breadcrumb * )rhs;

    retval = ego_sint( bc_rhs->time ) - ego_sint( bc_lhs->time );

    if ( 0 == retval )
    {
        retval = ego_sint( bc_rhs->id ) - ego_sint( bc_lhs->id );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t breadcrumb_list_full( ego_breadcrumb_list *  lst )
{
    if ( NULL == lst || !lst->on ) return btrue;

    lst->count = CLIP( lst->count, 0, MAX_BREADCRUMB );

    return ( lst->count >= MAX_BREADCRUMB );
}

//--------------------------------------------------------------------------------------------
bool_t breadcrumb_list_empty( ego_breadcrumb_list * lst )
{
    if ( NULL == lst || !lst->on ) return btrue;

    lst->count = CLIP( lst->count, 0, MAX_BREADCRUMB );

    return ( 0 == lst->count );
}

//--------------------------------------------------------------------------------------------
void breadcrumb_list_compact( ego_breadcrumb_list * lst )
{
    int cnt, tnc;

    if ( NULL == lst || !lst->on ) return;

    // compact the list of breadcrumbs
    for ( cnt = 0, tnc = 0; cnt < lst->count; cnt ++ )
    {
        ego_breadcrumb * bc_src = lst->lst + cnt;
        ego_breadcrumb * bc_dst = lst->lst + tnc;

        if ( bc_src->valid )
        {
            if ( bc_src != bc_dst )
            {
                SDL_memcpy( bc_dst, bc_src, sizeof( ego_breadcrumb ) );
            }

            tnc++;
        }
    }
    lst->count = tnc;
}

//--------------------------------------------------------------------------------------------
void breadcrumb_list_validate( ego_breadcrumb_list * lst )
{
    int cnt, invalid_cnt;

    if ( NULL == lst || !lst->on ) return;

    // invalidate all bad breadcrumbs
    for ( cnt = 0, invalid_cnt = 0; cnt < lst->count; cnt ++ )
    {
        ego_breadcrumb * bc = lst->lst + cnt;

        if ( !bc->valid )
        {
            invalid_cnt++;
        }
        else
        {
            if ( 0 != ego_mpd::test_wall( PMesh, bc->pos.v, bc->radius, bc->bits, NULL ) )
            {
                bc->valid = bfalse;
                invalid_cnt++;
            }
        }
    }

    // clean up the list
    if ( invalid_cnt > 0 )
    {
        breadcrumb_list_compact( lst );
    }

    // sort the values from lowest to highest
    if ( lst->count > 1 )
    {
        SDL_qsort( lst->lst, lst->count, sizeof( ego_breadcrumb ), breadcrumb_cmp );
    }
}

//--------------------------------------------------------------------------------------------
ego_breadcrumb * breadcrumb_list_last_valid( ego_breadcrumb_list * lst )
{
    ego_breadcrumb * retval = NULL;

    if ( NULL == lst || !lst->on ) return NULL;

    breadcrumb_list_validate( lst );

    if ( !breadcrumb_list_empty( lst ) )
    {
        retval = lst->lst + 0;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
ego_breadcrumb * breadcrumb_list_newest( ego_breadcrumb_list * lst )
{
    int cnt;

    Uint32         old_time = 0xFFFFFFFF;
    ego_breadcrumb * old_ptr = NULL;

    if ( NULL == lst || !lst->on ) return NULL;

    for ( cnt = 0; cnt < lst->count; cnt ++ )
    {
        ego_breadcrumb * bc = lst->lst + cnt;

        if ( !bc->valid ) continue;

        if ( NULL == old_ptr )
        {
            old_ptr  = bc;
            old_time = bc->time;

            break;
        }
    }

    for ( cnt++; cnt < lst->count; cnt++ )
    {
        int tmp;
        ego_breadcrumb * bc = lst->lst + cnt;

        if ( !bc->valid ) continue;

        tmp = breadcrumb_cmp( old_ptr, bc );

        if ( tmp < 0 )
        {
            old_ptr  = bc;
            old_time = bc->time;

            break;
        }
    }

    return old_ptr;
}

//--------------------------------------------------------------------------------------------
ego_breadcrumb * breadcrumb_list_oldest( ego_breadcrumb_list * lst )
{
    int cnt;

    Uint32         old_time = 0xFFFFFFFF;
    ego_breadcrumb * old_ptr = NULL;

    if ( NULL == lst || !lst->on ) return NULL;

    for ( cnt = 0; cnt < lst->count; cnt ++ )
    {
        ego_breadcrumb * bc = lst->lst + cnt;

        if ( !bc->valid ) continue;

        if ( NULL == old_ptr )
        {
            old_ptr  = bc;
            old_time = bc->time;

            break;
        }
    }

    for ( cnt++; cnt < lst->count; cnt++ )
    {
        int tmp;
        ego_breadcrumb * bc = lst->lst + cnt;

        if ( !bc->valid ) continue;

        tmp = breadcrumb_cmp( old_ptr, bc );

        if ( tmp > 0 )
        {
            old_ptr  = bc;
            old_time = bc->time;

            break;
        }
    }

    return old_ptr;
}

//--------------------------------------------------------------------------------------------
ego_breadcrumb * breadcrumb_list_oldest_grid( ego_breadcrumb_list * lst, Uint32 match_grid )
{
    int cnt;

    Uint32         old_time = 0xFFFFFFFF;
    ego_breadcrumb * old_ptr = NULL;

    if ( NULL == lst || !lst->on ) return NULL;

    for ( cnt = 0; cnt < lst->count; cnt ++ )
    {
        ego_breadcrumb * bc = lst->lst + cnt;

        if ( !bc->valid ) continue;

        if (( NULL == old_ptr ) && ( bc->grid == match_grid ) )
        {
            old_ptr  = bc;
            old_time = bc->time;

            break;
        }
    }

    for ( cnt++; cnt < lst->count; cnt++ )
    {
        int tmp;

        ego_breadcrumb * bc = lst->lst + cnt;

        if ( !bc->valid ) continue;

        tmp = breadcrumb_cmp( old_ptr, bc );

        if (( tmp > 0 ) && ( bc->grid == match_grid ) )
        {
            old_ptr  = bc;
            old_time = bc->time;

            break;
        }
    }

    return old_ptr;
}

//--------------------------------------------------------------------------------------------
ego_breadcrumb * breadcrumb_list_alloc( ego_breadcrumb_list * lst )
{
    ego_breadcrumb * retval = NULL;

    if ( breadcrumb_list_full( lst ) )
    {
        breadcrumb_list_compact( lst );
    }

    if ( breadcrumb_list_full( lst ) )
    {
        retval = breadcrumb_list_oldest( lst );
    }
    else
    {
        retval = lst->lst + lst->count;
        lst->count++;
        retval->id = breadcrumb_guid++;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t breadcrumb_list_add( ego_breadcrumb_list * lst, ego_breadcrumb * pnew )
{
    int cnt, invalid_cnt;

    bool_t retval;
    ego_breadcrumb * pold, *ptmp;

    if ( NULL == lst || !lst->on ) return bfalse;

    if ( NULL == pnew ) return bfalse;

    for ( cnt = 0, invalid_cnt = 0; cnt < lst->count; cnt ++ )
    {
        ego_breadcrumb * bc = lst->lst + cnt;

        if ( !bc->valid )
        {
            invalid_cnt++;
            break;
        }
    }

    if ( invalid_cnt > 0 )
    {
        breadcrumb_list_compact( lst );
    }

    // find the newest tile with the same grid position
    ptmp = breadcrumb_list_newest( lst );
    if ( NULL != ptmp && ptmp->valid )
    {
        if ( ptmp->grid == pnew->grid )
        {
            if ( INVALID_TILE == ptmp->grid )
            {
                // both are off the map, so determine the difference in distance
                if ( SDL_abs( ptmp->pos.x - pnew->pos.x ) < GRID_SIZE && SDL_abs( ptmp->pos.y - pnew->pos.y ) < GRID_SIZE )
                {
                    // not far enough apart
                    pold = ptmp;
                }
            }
            else
            {
                // the newest is on the same tile == the object hasn't moved
                pold = ptmp;
            }
        }
    }

    if ( breadcrumb_list_full( lst ) )
    {
        // the list is full, so we have to reuse an element

        // try the oldest element at this grid position
        pold = breadcrumb_list_oldest_grid( lst, pnew->grid );

        if ( NULL == pold )
        {
            // not found, so find the oldest breadcrumb
            pold = breadcrumb_list_oldest( lst );
        }
    }
    else
    {
        // the list is not full, so just allocate an element as normal

        pold = breadcrumb_list_alloc( lst );
    }

    // assign the data to the list element
    retval = bfalse;
    if ( NULL != pold )
    {
        *pold = *pnew;

        retval = btrue;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t phys_apply_normal_acceleration( fvec3_base_t acc, fvec3_base_t nrm, float para_factor, float perp_factor, fvec3_t * pnrm_acc )
{
    fvec3_t sum = ZERO_VECT3;

    if ( NULL == acc || NULL == nrm ) return bfalse;

    // clear the normal acceleration here
    fvec3_self_clear( pnrm_acc->v );

    // if there is no acceleration, there is nothing to do
    if ( 0.0f == fvec3_length_abs( acc ) ) return btrue;

    // if the object scale factors are 1, there is nothing to do
    if ( 1.0f == perp_factor && 1.0f == para_factor ) return btrue;

    // make a shortcut for the simple case of both scale factors being zero
    if ( 0.0f == perp_factor && 0.0 == para_factor )
    {
        fvec3_self_clear( sum.v );
    }
    else
    {
        float   dot;
        fvec3_t para = ZERO_VECT3, perp = ZERO_VECT3;

        if ( 0.0f == fvec3_length_abs( nrm ) )
        {
            nrm[kX] = nrm[kY] = 0.0f;
            nrm[kZ] = -SGN( gravity );
        }

        // perpendicular to the ground
        if ( 1.0f == SDL_abs( nrm[kZ] ) )
        {
            dot = acc[kZ] * nrm[kZ];

            perp.x = 0.0f;
            perp.y = 0.0f;
            perp.z = dot;

            // parallel to the ground
            para.x = acc[kX];
            para.y = acc[kY];
            para.z = acc[kZ] - perp.z;
        }
        else
        {
            // the vector parallel to the normal is perpendicular to the surface
            dot = fvec3_decompose( acc, nrm, perp.v, para.v );
        }

        // kill the acceleration perpendicular to the ground to take the net effect of the
        // ground's "normal force" into account
        if ( dot < 0.0f && 1.0f != perp_factor )
        {
            fvec3_self_scale( perp.v, perp_factor );
        }

        // scale the parallel vector
        if ( 1.0f != para_factor )
        {
            fvec3_self_scale( para.v, para_factor );
        }

        // calculate the new acceleration
        sum = fvec3_add( para.v, perp.v );
    }

    // make a shortcut for the simple case
    if ( NULL == pnrm_acc )
    {
        acc[kX] = sum.v[kX];
        acc[kY] = sum.v[kY];
        acc[kZ] = sum.v[kZ];
    }
    else
    {
        // calculate the effective "normal acceleration"
        *pnrm_acc = fvec3_sub( sum.v, acc );

        // apply the normal acceleration
        fvec3_self_sum( acc, pnrm_acc->v );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t phys_data_integrate_accumulators( fvec3_t * ppos, fvec3_t * pvel, ego_phys_data * pdata, float dt )
{
    fvec3_t loc_pos = ZERO_VECT3;
    fvec3_t loc_vel = ZERO_VECT3;
    fvec3_t displacement = ZERO_VECT3;

    if ( NULL == pdata ) return bfalse;
    if ( 0.0f == dt ) return btrue;

    // handle optional parameters
    if ( NULL == ppos ) ppos = &loc_pos;
    if ( NULL == pvel ) pvel = &loc_vel;

    // sum the displacements
    displacement = fvec3_add( pdata->apos_plat.v, pdata->apos_coll.v );

    //if( fvec3_length_abs(displacement.v) > 100.0f )
    //{
    //    displacement.x = 0.0f;
    //}

    // integrate the position
    ppos->x += pvel->x * dt + displacement.x;
    ppos->y += pvel->y * dt + displacement.y;
    ppos->z += pvel->z * dt + displacement.z;

    //if( fvec3_length_abs(pdata->avel.v) > 100.0f )
    //{
    //    pdata->avel.x = 0.0f;
    //}

    // integrate the velocity
    pvel->x += pdata->avel.x * dt;
    pvel->y += pdata->avel.y * dt;
    pvel->z += pdata->avel.z * dt;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t phys_data_apply_normal_acceleration( ego_phys_data * pphys, fvec3_t nrm, float para_factor, float perp_factor, fvec3_t * pnrm_acc )
{
    /// \author BB
    /// \details  break the acceleration up into parts that are parallel and perpendicular to the floor
    /// and partially kill the acceleration perpendicular to the ground.
    /// This takes the net effect of the ground's "normal force" into account.

    /// \note BB@> don't apply this to the platform accumulator since it has some special "physics"
    ///     like sucking an object onto the platform, which keeps object attached
    ///     when a platform starts to descend

    if ( NULL == pphys ) return bfalse;

    // use a short-cut if the vectors are going to be scaled to 0.0f, anyway
    if ( 0.0f == para_factor && 0.0f == perp_factor )
    {
        fvec3_self_clear( pphys->apos_coll.v );
        // fvec3_self_clear( pphys->apos_plat.v );
    }
    else
    {

        phys_apply_normal_acceleration( pphys->apos_coll.v, nrm.v, para_factor, perp_factor, NULL );
        // phys_apply_normal_acceleration( pphys->apos_plat.v, nrm.v, para_factor, perp_factor, NULL     );
    }

    // can't use the same short-cut this calculation because we my still need to know nrm_acc
    if ( 0.0f == para_factor && 0.0f == perp_factor && NULL == pnrm_acc )
    {
        fvec3_self_clear( pphys->avel.v );
    }
    else
    {
        phys_apply_normal_acceleration( pphys->avel.v, nrm.v, para_factor, perp_factor, pnrm_acc );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
// OBSOLETE CODE
//--------------------------------------------------------------------------------------------

//bool_t ego_oct_bb::intersect_close( ego_oct_bb & src1, fvec3_t pos1, fvec3_t vel1, ego_oct_bb & src2, fvec3_t pos2, fvec3_t vel2, int test_platform, ego_oct_bb   * pdst, float *tmin, float *tmax )
//{
//    /// \author BB
//    /// \details  A test to determine whether two "fast moving" objects are interacting within a frame.
//    ///               Designed to determine whether a bullet particle will interact with character.
//
//    ego_oct_bb   exp1, exp2;
//    ego_oct_bb   intersection;
//
//    ego_oct_vec opos1, opos2;
//    ego_oct_vec ovel1, ovel2;
//
//    int    cnt, index;
//    bool_t found;
//    float  tolerance;
//    float  local_tmin, local_tmax;
//
//    // handle optional parameters
//    if ( NULL == tmin ) tmin = &local_tmin;
//    if ( NULL == tmax ) tmax = &local_tmax;
//
//    // do the objects interact at the very beginning of the update?
//    if ( test_interaction_2( src1, pos2, src2, pos2, test_platform ) )
//    {
//        if ( NULL != pdst )
//        {
//            ego_oct_bb::do_intersection( src1, src2, pdst );
//        }
//
//        return btrue;
//    }
//
//    // convert the position and velocity vectors to octagonal format
//    ego_oct_vec::ctor_this( ovel1, vel1 );
//    ego_oct_vec::ctor_this( opos1, pos1 );
//
//    ego_oct_vec::ctor_this( ovel2, vel2 );
//    ego_oct_vec::ctor_this( opos2, pos2 );
//
//    // cycle through the coordinates to see when the two volumes might coincide
//    found = bfalse;
//    *tmin = *tmax = -1.0f;
//    for ( index = 0; index < OCT_COUNT; index ++ )
//    {
//        egoboo_rv retval;
//        float tmp_min, tmp_max;
//
//        retval = ego_oct_bb::intersect_index_close( index, src1, opos1, ovel1, src2, opos2, ovel2, test_platform, &tmp_min, &tmp_max );
//        if ( rv_fail == retval ) return bfalse;
//
//        if ( rv_success == retval )
//        {
//            if ( !found )
//            {
//                *tmin = tmp_min;
//                *tmax = tmp_max;
//                found = btrue;
//            }
//            else
//            {
//                *tmin = SDL_max( *tmin, tmp_min );
//                *tmax = SDL_min( *tmax, tmp_max );
//            }
//        }
//
//        if ( *tmax < *tmin ) return bfalse;
//    }
//
//    // if the objects do not interact this frame let the caller know
//    if ( *tmin > 1.0f || *tmax < 0.0f ) return bfalse;
//
//    // determine the expanded collision volumes for both objects
//    phys_expand_oct_bb( src1, vel1, *tmin, *tmax, &exp1 );
//    phys_expand_oct_bb( src2, vel2, *tmin, *tmax, &exp2 );
//
//    // determine the intersection of these two volumes
//    ego_oct_bb::do_intersection( exp1, exp2, &intersection );
//
//    // check to see if there is any possibility of interaction at all
//    for ( cnt = 0; cnt < OCT_Z; cnt++ )
//    {
//        if ( intersection.mins[cnt] > intersection.maxs[cnt] ) return bfalse;
//    }
//
//    tolerance = ( 0 == test_platform ) ? 0.0f : PLATTOLERANCE;
//    if ( intersection.mins[OCT_Z] > intersection.maxs[OCT_Z] + tolerance ) return bfalse;
//
//    return btrue;
//}
//
