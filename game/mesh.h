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

#include "file_formats/mpd_file.h"
#include "extensions/ogl_include.h"

#include "lighting.h"
#include "bsp.h"

//--------------------------------------------------------------------------------------------
struct ego_mpd;

struct mesh_wall_data;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define BLOCK_BITS    9
#define BLOCK_SIZE    ((float)(1<<(BLOCK_BITS)))

#define MAXMESHBLOCKY             (( MAXMESHTILEY >> (BLOCK_BITS-TILE_BITS) )+1)  ///< max blocks in the y direction

// mesh physics
#define SLIDE                           0.04f         ///< Acceleration for steep hills
#define SLIDEFIX                        0.08f         ///< To make almost flat surfaces flat
#define TWIST_FLAT                      119

#define TILE_UPPER_SHIFT                8
#define TILE_LOWER_MASK                 ((1 << TILE_UPPER_SHIFT)-1)
#define TILE_UPPER_MASK                 (~TILE_LOWER_MASK)

#define TILE_GET_LOWER_BITS(XX)         ( TILE_LOWER_MASK & (XX) )

#define TILE_GET_UPPER_BITS(XX)         (( TILE_UPPER_MASK & (XX) ) >> TILE_UPPER_SHIFT )
#define TILE_SET_UPPER_BITS(XX)         (( (XX) << TILE_UPPER_SHIFT ) & TILE_UPPER_MASK )

#define TILE_IS_FANOFF(XX)              ( FANOFF == (XX).img )

#define TILE_HAS_INVALID_IMAGE(XX)      HAS_SOME_BITS( TILE_UPPER_MASK, (XX).img )

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
typedef GLXvector3f normal_cache_t[4];
typedef float       light_cache_t[4];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// the BSP structure housing the mesh
struct mpd_BSP
{
    ego_oct_bb         volume;
    ego_BSP_leaf_ary_t nodes;
    ego_BSP_tree       tree;

    mpd_BSP();
    mpd_BSP( ego_mpd   * pmesh );
    ~mpd_BSP();

    static mpd_BSP root;

    static mpd_BSP * ctor( mpd_BSP * pbsp, ego_mpd * pmesh );
    static mpd_BSP * dtor( mpd_BSP * );
    static bool_t    alloc( mpd_BSP * pbsp, ego_mpd * pmesh );
    static bool_t    dealloc( mpd_BSP * pbsp );

    static bool_t    fill( mpd_BSP * pbsp );

    static int       collide( mpd_BSP * pbsp, ego_BSP_aabb   * paabb, ego_BSP_leaf_pary_t * colst );
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// The data describing an Egoboo tile
struct ego_tile_info
{
    Uint8   type;                              ///< Tile type
    Uint16  img;                               ///< Get texture from this
    size_t  vrtstart;                          ///< Which vertex to start at

    bool_t  fanoff;                            ///< display this tile?
    bool_t  inrenderlist;                      ///< Is the tile going to be rendered this frame?
    signed  inrenderlist_frame;                ///< What was the frame number the last time this tile was rendered?
    bool_t  needs_lighting_update;             ///< Has this tile been tagged for a lighting update?

    ego_oct_bb         oct;                        ///< the octagonal bounding box for this tile
    normal_cache_t ncache;                     ///< the normals at the corners of this tile
    light_cache_t  lcache;                     ///< the light at the corners of this tile
    light_cache_t  d1_cache;                   ///< the estimated change in the light at the corner of the tile
    light_cache_t  d2_cache;                   ///< the estimated change in the light at the corner of the tile
};

ego_tile_info * ego_tile_info_alloc();
ego_tile_info * ego_tile_info_init( ego_tile_info * ptr );

ego_tile_info * ego_tile_info_alloc_ary( size_t count );
ego_tile_info * ego_tile_info_init_ary( ego_tile_info * ptr, size_t count );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// The data describing an Egoboo grid
struct ego_grid_info
{
    Uint8           fx;                        ///< Special effects flags
    Uint8           twist;                     ///< The orientation of the tile

    // the lighting info in the upper left hand corner of a grid
    Uint8            a, l;                     ///< the raw mesh lighting... pretty much ignored
    ego_lighting_cache cache;                    ///< the per-grid lighting info

    ego_grid_info() { memset( this, 0, sizeof( *this ) ); }
};

//--------------------------------------------------------------------------------------------
struct ego_grid_mem
{
    int             grids_x;                          ///< Size in grids
    int             grids_y;
    size_t          grid_count;                       ///< how many grids

    int             blocks_x;                         ///< Size in blocks
    int             blocks_y;
    Uint32          blocks_count;                     ///< Number of blocks (collision areas)

    float           edge_x;                           ///< Limits
    float           edge_y;

    Uint32        * blockstart;                       ///< list of blocks that start each row
    Uint32        * tilestart;                        ///< list of tiles  that start each row

    // the per-grid info
    ego_grid_info* grid_list;                        ///< tile command info

    ego_grid_mem();
    ~ego_grid_mem();
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// A wrapper for the dynamically allocated mesh memory
struct ego_tile_mem
{
    ego_aabb           bbox;                             ///< bounding box for the entire mesh

    // the per-tile info
    size_t           tile_count;                       ///< number of tiles
    ego_tile_info* tile_list;                        ///< tile command info

    // the per-vertex info to be presented to OpenGL
    size_t          vert_count;                        ///< number of vertices
    GLXvector3f   * plst;                              ///< the position list
    GLXvector2f   * tlst;                              ///< the texture coordinate list
    GLXvector3f   * clst;                              ///< the color list (for lighting the mesh)
    GLXvector3f   * nlst;                              ///< the normal list

    ego_tile_mem();
    ~ego_tile_mem();
};

//--------------------------------------------------------------------------------------------

/// The generic parameters describing an ego_mpd
struct ego_mpd_info
{
    size_t          vertcount;                         ///< For malloc

    int             tiles_x;                          ///< Size in tiles
    int             tiles_y;
    Uint32          tiles_count;                      ///< Number of tiles

    ego_mpd_info() { memset( this, 0, sizeof( *this ) ); }
};

//--------------------------------------------------------------------------------------------

/// Egoboo's representation of the .mpd mesh file
struct ego_mpd
{
    ego_mpd_info  info;
    ego_tile_mem      tmem;
    ego_grid_mem      gmem;

    fvec2_t         tileoff[MAXTILETYPE];     ///< Tile texture offset

    ego_mpd();
    ~ego_mpd();
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

extern int mesh_mpdfx_tests;
extern int mesh_bound_tests;
extern int mesh_pressure_tests;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_mpd   * mesh_create( ego_mpd   * pmesh, int tiles_x, int tiles_y );
bool_t      mesh_destroy( ego_mpd   ** pmesh );

ego_mpd   * mesh_ctor( ego_mpd   * pmesh );
ego_mpd   * mesh_dtor( ego_mpd   * pmesh );
ego_mpd   * mesh_renew( ego_mpd   * pmesh );

// loading/saving
ego_mpd   * mesh_load( const char *modname, ego_mpd   * pmesh );

void   mesh_make_twist();

float  mesh_light_corners( ego_mpd   * pmesh, int itile, float mesh_lighting_keep );
bool_t mesh_test_corners( ego_mpd   * pmesh, int itile, float threshold );
bool_t mesh_interpolate_vertex( ego_tile_mem * pmem, int itile, float pos[], float * plight );

bool_t grid_light_one_corner( ego_mpd   * pmesh, int fan, float height, float nrm[], float * plight );

BIT_FIELD mesh_hit_wall( ego_mpd   * pmesh, float pos[], float radius, Uint32 bits, float nrm[], float * pressure );
bool_t mesh_test_wall( ego_mpd   * pmesh, float pos[], float radius, Uint32 bits, mesh_wall_data * private_data );

float mesh_get_max_vertex_0( ego_mpd   * pmesh, int grid_x, int grid_y );
float mesh_get_max_vertex_1( ego_mpd   * pmesh, int grid_x, int grid_y, float xmin, float ymin, float xmax, float ymax );

bool_t mesh_set_texture( ego_mpd   * pmesh, Uint16 tile, Uint16 image );
bool_t mesh_update_texture( ego_mpd   * pmesh, Uint32 tile );

fvec2_t mesh_get_diff( ego_mpd   * pmesh, float pos[], float radius, float center_pressure, BIT_FIELD bits );
float mesh_get_pressure( ego_mpd   * pmesh, float pos[], float radius, BIT_FIELD bits );

Uint8 cartman_get_fan_twist( ego_mpd   * pmesh, Uint32 tile );