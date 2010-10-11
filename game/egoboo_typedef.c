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

/// @file egoboo_typedef.c
/// @brief Implementation of the support functions for Egoboo's special datatypes
/// @details

#include "egoboo_typedef.h"
#include "egoboo_math.inl"

#include "log.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Uint32 ego_object_guid = 0;
Uint32 ego_object_spawn_depth = 0;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// implement simple type arrays
IMPLEMENT_DYNAMIC_ARY( char_ary,   char );
IMPLEMENT_DYNAMIC_ARY( short_ary,  short );
IMPLEMENT_DYNAMIC_ARY( int_ary,    int );
IMPLEMENT_DYNAMIC_ARY( float_ary,  float );
IMPLEMENT_DYNAMIC_ARY( double_ary, double );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
const char * undo_idsz( IDSZ idsz )
{
    /// @details ZZ@> This function takes an integer and makes a text IDSZ out of it.

    static char value_string[5] = {"NONE"};

    if ( idsz == IDSZ_NONE )
    {
        strncpy( value_string, "NONE", SDL_arraysize( value_string ) );
    }
    else
    {
        // Bad! both function return and return to global variable!
        value_string[0] = (( idsz >> 15 ) & 0x1F ) + 'A';
        value_string[1] = (( idsz >> 10 ) & 0x1F ) + 'A';
        value_string[2] = (( idsz >> 5 ) & 0x1F ) + 'A';
        value_string[3] = (( idsz ) & 0x1F ) + 'A';
        value_string[4] = 0;
    }

    return value_string;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t frect_union( const frect_t * src1, const frect_t * src2, frect_t * dst )
{
    if ( NULL == dst ) return bfalse;

    if ( NULL == src1 && NULL == src2 )
    {
        memset( dst, 0, sizeof( frect_t ) );
    }
    else if ( NULL == src1 )
    {
        memmove( src2, dst, sizeof( frect_t ) );
    }
    else if ( NULL == src2 )
    {
        memmove( src1, dst, sizeof( frect_t ) );
    }
    else
    {
        float x1, y1, x2, y2;
        float x3, y3, x4, y4;

        x1 = src1->x;
        y1 = src1->y;
        x2 = src1->x + src1->w;
        y2 = src1->y + src1->h;

        x3 = src2->x;
        y3 = src2->y;
        x4 = src2->x + src2->w;
        y4 = src2->y + src2->h;

        x1 = MIN( x1, x3 );
        y1 = MIN( y1, y3 );
        x2 = MAX( x2, x4 );
        y2 = MAX( y2, y4 );

        dst->x = x1;
        dst->y = y1;
        dst->w = x2 - x1;
        dst->h = y2 - y1;
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t irect_point_inside( ego_irect_t * prect, int   ix, int   iy )
{
    if ( NULL == prect ) return bfalse;

    if ( ix < prect->xmin || ix > prect->xmax + 1 ) return bfalse;
    if ( iy < prect->ymin || iy > prect->ymax + 1 ) return bfalse;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t frect_point_inside( ego_frect_t * prect, float fx, float fy )
{
    if ( NULL == prect ) return bfalse;

    if ( fx < prect->xmin || fx > prect->xmax ) return bfalse;
    if ( fy < prect->ymin || fy > prect->ymax ) return bfalse;

    return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void latch_input_init( latch_input_t * platch )
{
    if ( NULL == platch ) return;

    memset( platch, 0, sizeof( *platch ) );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void pair_to_range( IPair pair, FRange * prange )
{
    /// @details ZZ@> convert from a pair to a range

    if ( pair.base < 0 )
    {
        log_warning( "We got a randomization error again! (Base is less than 0)\n" );
    }

    if ( pair.rand < 0 )
    {
        log_warning( "We got a randomization error again! (rand is less than 0)\n" );
    }

    if ( NULL != prange )
    {
        float fFrom, fTo;

        fFrom = SFP8_TO_FLOAT( pair.base );
        fTo   = SFP8_TO_FLOAT( pair.base + pair.rand );

        prange->from = MIN( fFrom, fTo );
        prange->to   = MAX( fFrom, fTo );
    }
}

//--------------------------------------------------------------------------------------------
void range_to_pair( FRange range, IPair * ppair )
{
    /// @details ZZ@> convert from a range to a pair

    if ( range.from > range.to )
    {
        log_warning( "We got a range error! (to is less than from)\n" );
    }

    if ( NULL != ppair )
    {
        float fFrom, fTo;

        fFrom = MIN( range.from, range.to );
        fTo   = MAX( range.from, range.to );

        ppair->base = FLOAT_TO_SFP8( fFrom );
        ppair->rand = FLOAT_TO_SFP8( fTo - fFrom );
    }
}

//--------------------------------------------------------------------------------------------
void ints_to_range( int ibase, int irand, FRange * prange )
{
    IPair pair_tmp;

    pair_tmp.base = ibase;
    pair_tmp.rand = irand;

    pair_to_range( pair_tmp, prange );
}

//--------------------------------------------------------------------------------------------
void floats_to_pair( float vmin, float vmax, IPair * ppair )
{
    FRange range_tmp;

    range_tmp.from = vmin;
    range_tmp.to   = vmax;

    range_to_pair( range_tmp, ppair );
}

