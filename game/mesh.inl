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

/// @file mesh.inl

#include "mesh.h"

#include "egoboo_math.inl"

//--------------------------------------------------------------------------------------------
// IMPLEMENTATION
//--------------------------------------------------------------------------------------------
INLINE Uint32 ego_mpd::has_some_mpdfx( BIT_FIELD MPDFX, BIT_FIELD TEST )
{
    ego_mpd::mpdfx_tests++;
    return HAS_SOME_BITS( MPDFX, TEST );
}

//--------------------------------------------------------------------------------------------
INLINE bool_t ego_mpd::grid_is_valid( ego_mpd   * pmpd, Uint32 id )
{
    if ( NULL == pmpd ) return bfalse;

    ego_mpd::bound_tests++;

    if ( INVALID_TILE == id ) return bfalse;

    return id < pmpd->info.tiles_count;
}

//--------------------------------------------------------------------------------------------
INLINE float ego_mpd::get_level( ego_mpd   * pmesh, float x, float y )
{
    /// @details ZZ@> This function returns the height of a point within a mesh fan, precisely

    Uint32 tile;
    int ix, iy;

    float z0, z1, z2, z3;         // Height of each fan corner
    float zleft, zright, zdone;   // Weighted height of each side

    tile = ego_mpd::get_tile( pmesh, x, y );
    if ( !ego_mpd::grid_is_valid( pmesh, tile ) ) return 0;

    ix = x;
    iy = y;

    ix &= TILE_MASK;
    iy &= TILE_MASK;

    z0 = pmesh->tmem.plst[ pmesh->tmem.tile_list[tile].vrtstart + 0 ][ZZ];
    z1 = pmesh->tmem.plst[ pmesh->tmem.tile_list[tile].vrtstart + 1 ][ZZ];
    z2 = pmesh->tmem.plst[ pmesh->tmem.tile_list[tile].vrtstart + 2 ][ZZ];
    z3 = pmesh->tmem.plst[ pmesh->tmem.tile_list[tile].vrtstart + 3 ][ZZ];

    zleft  = ( z0 * ( GRID_SIZE - iy ) + z3 * iy ) / GRID_SIZE;
    zright = ( z1 * ( GRID_SIZE - iy ) + z2 * iy ) / GRID_SIZE;
    zdone  = ( zleft * ( GRID_SIZE - ix ) + zright * ix ) / GRID_SIZE;

    return zdone;
}

//--------------------------------------------------------------------------------------------
INLINE Uint32 ego_mpd::get_block( ego_mpd   * pmesh, float pos_x, float pos_y )
{
    Uint32 block = INVALID_BLOCK;

    if ( pos_x >= 0.0f && pos_x <= pmesh->gmem.edge_x && pos_y >= 0.0f && pos_y <= pmesh->gmem.edge_y )
    {
        int ix, iy;

        ix = pos_x;
        iy = pos_y;

        ix >>= BLOCK_BITS;
        iy >>= BLOCK_BITS;

        block = ego_mpd::get_block_int( pmesh, ix, iy );
    }

    return block;
}

//--------------------------------------------------------------------------------------------
INLINE Uint32 ego_mpd::get_tile( ego_mpd   * pmesh, float pos_x, float pos_y )
{
    Uint32 tile = INVALID_TILE;

    if ( pos_x >= 0.0f && pos_x < pmesh->gmem.edge_x && pos_y >= 0.0f && pos_y < pmesh->gmem.edge_y )
    {
        int ix, iy;

        ix = pos_x;
        iy = pos_y;

        ix >>= TILE_BITS;
        iy >>= TILE_BITS;

        tile = ego_mpd::get_tile_int( pmesh, ix, iy );
    }

    return tile;
}

//--------------------------------------------------------------------------------------------
INLINE Uint32 ego_mpd::get_block_int( ego_mpd   * pmesh, int block_x, int block_y )
{
    if ( NULL == pmesh ) return INVALID_BLOCK;

    if ( block_x < 0 || block_x >= pmesh->gmem.blocks_x )  return INVALID_BLOCK;
    if ( block_y < 0 || block_y >= pmesh->gmem.blocks_y )  return INVALID_BLOCK;

    return block_x + pmesh->gmem.blockstart[block_y];
}

//--------------------------------------------------------------------------------------------
INLINE Uint32 ego_mpd::get_tile_int( ego_mpd   * pmesh, int grid_x,  int grid_y )
{
    if ( NULL == pmesh ) return INVALID_TILE;

    if ( grid_x < 0 || grid_x >= pmesh->info.tiles_x )  return INVALID_TILE;
    if ( grid_y < 0 || grid_y >= pmesh->info.tiles_y )  return INVALID_TILE;

    return grid_x + pmesh->gmem.tilestart[grid_y];
}

//--------------------------------------------------------------------------------------------
INLINE bool_t ego_mpd::clear_fx( ego_mpd   * pmesh, Uint32 itile, BIT_FIELD flags )
{
    Uint32 old_flags;

    // test for mesh
    if ( NULL == pmesh ) return bfalse;

    // test for invalid tile
    ego_mpd::bound_tests++;
    if ( itile > pmesh->info.tiles_count ) return bfalse;

    // save a copy of the fx
    old_flags = pmesh->gmem.grid_list[itile].fx;

    // clear the wall and impass flags
    REMOVE_BITS( pmesh->gmem.grid_list[itile].fx, flags );

    // succeed only of something actually changed
    return 0 != ( old_flags & flags );
}

//--------------------------------------------------------------------------------------------
INLINE bool_t ego_mpd::add_fx( ego_mpd   * pmesh, Uint32 itile, BIT_FIELD flags )
{
    Uint32 old_flags;

    // test for mesh
    if ( NULL == pmesh ) return bfalse;

    // test for invalid tile
    ego_mpd::bound_tests++;
    if ( itile > pmesh->info.tiles_count ) return bfalse;

    // save a copy of the fx
    old_flags = pmesh->gmem.grid_list[itile].fx;

    // add in the flags
    pmesh->gmem.grid_list[itile].fx = old_flags | flags;

    // succeed only of something actually changed
    return 0 != ( old_flags & flags );
}

//--------------------------------------------------------------------------------------------
INLINE Uint32 ego_mpd::test_fx( ego_mpd   * pmesh, Uint32 itile, BIT_FIELD flags )
{
    // test for mesh
    if ( NULL == pmesh ) return 0;

    // test for invalid tile
    ego_mpd::bound_tests++;
    if ( itile > pmesh->info.tiles_count )
    {
        return flags & ( MPDFX_WALL | MPDFX_IMPASS );
    }

    // if the tile is actually labelled as FANOFF, ignore it completely
    if ( FANOFF == pmesh->tmem.tile_list[itile].img )
    {
        return 0;
    }

    return ego_mpd::has_some_mpdfx( pmesh->gmem.grid_list[itile].fx, flags );
}

