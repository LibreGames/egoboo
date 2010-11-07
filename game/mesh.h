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

//--------------------------------------------------------------------------------------------
struct ego_mpd;

struct mesh_wall_data;
struct ego_mpd_info;

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

/// The data describing an Egoboo tile
struct ego_tile_info
{
    Uint8   type;                              ///< Tile type
    Uint16  img;                               ///< Get texture from this
    size_t  vrtstart;                          ///< Which vertex to start at

    bool_t   fanoff;                            ///< display this tile?
    bool_t   inrenderlist;                      ///< Is the tile going to be rendered this frame?
    ego_sint inrenderlist_frame;                ///< What was the frame number the last time this tile was rendered?
    bool_t   needs_lighting_update;             ///< Has this tile been tagged for a lighting update?

    ego_oct_bb         oct;                        ///< the octagonal bounding box for this tile
    normal_cache_t ncache;                     ///< the normals at the corners of this tile
    light_cache_t  lcache;                     ///< the light at the corners of this tile
    light_cache_t  d1_cache;                   ///< the estimated change in the light at the corner of the tile
    light_cache_t  d2_cache;                   ///< the estimated change in the light at the corner of the tile
};

ego_tile_info * ego_tile_info_alloc();
ego_tile_info * ego_tile_info_init( ego_tile_info * ptr );

ego_tile_info * ego_tile_info_alloc_ary( const size_t count );
ego_tile_info * ego_tile_info_init_ary( ego_tile_info * ptr, const size_t count );

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

    ego_grid_info() { fx = 0; twist = 0; a = l = 0; }
};

//--------------------------------------------------------------------------------------------
struct ego_grid_mem
{
    friend struct ego_mpd;

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

    ego_grid_mem() { clear( this ); ctor_this( this ); }
    ~ego_grid_mem() { dtor_this( this ); }

    static ego_grid_mem * ctor_this( ego_grid_mem * pmem );
    static ego_grid_mem * dtor_this( ego_grid_mem * pmem );

protected:

    static bool_t alloc( ego_grid_mem * pmem, ego_mpd_info * pinfo );
    static bool_t dealloc( ego_grid_mem * pmem );

    static ego_grid_mem * make_fanstart( ego_grid_mem * pgmem, ego_mpd_info * pinfo );

private:
    static ego_grid_mem * clear( ego_grid_mem * pmem )
    {
        if ( NULL == pmem ) return pmem;

        SDL_memset( pmem, 0, sizeof( *pmem ) );

        return pmem;
    }
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// A wrapper for the dynamically allocated mesh memory
struct ego_tile_mem
{
    ego_aabb           bbox;                             ///< bounding box for the entire mesh

    // the per-tile info
    size_t         tile_count;                       ///< number of tiles
    ego_tile_info* tile_list;                        ///< tile command info

    // the per-vertex info to be presented to OpenGL
    size_t          vert_count;                        ///< number of vertices
    GLXvector3f   * plst;                              ///< the position list
    GLXvector2f   * tlst;                              ///< the texture coordinate list
    GLXvector3f   * clst;                              ///< the color list (for lighting the mesh)
    GLXvector3f   * nlst;                              ///< the normal list

    ego_tile_mem() { clear( this ); ctor_this( this ); };
    ~ego_tile_mem() { dtor_this( this ); };

    static ego_tile_mem *  ctor_this( ego_tile_mem * pmem );
    static ego_tile_mem *  dtor_this( ego_tile_mem * pmem );
    static bool_t          dealloc( ego_tile_mem * pmem );
    static bool_t          alloc( ego_tile_mem * pmem, ego_mpd_info * pinfo );

    static bool_t          interpolate_vertex( ego_tile_mem * pmem, const int fan, const float pos[], float * plight );

private:
    static ego_tile_mem * clear( ego_tile_mem * ptr )
    {
        if ( NULL == ptr ) return ptr;

        ptr->tile_count = 0;
        ptr->tile_list = NULL;

        ptr->vert_count = 0;
        ptr->plst = NULL;
        ptr->tlst = NULL;
        ptr->clst = NULL;
        ptr->nlst = NULL;

        return ptr;
    }
};

//--------------------------------------------------------------------------------------------

/// The generic parameters describing an ego_mpd
struct ego_mpd_info
{
    size_t          vertcount;                         ///< For SDL_malloc

    size_t          tiles_x;                          ///< Size in tiles
    size_t          tiles_y;
    Uint32          tiles_count;                      ///< Number of tiles

    ego_mpd_info() { clear( this ); ctor_this( this ); }
    ~ego_mpd_info() { dtor_this( this ); }

    static ego_mpd_info * ctor_this( ego_mpd_info * pinfo );
    static ego_mpd_info * dtor_this( ego_mpd_info * pinfo );

    static ego_mpd_info * init( ego_mpd_info * pinfo, const int numvert, const size_t tiles_x, const size_t tiles_y );

private:

    static ego_mpd_info * clear( ego_mpd_info * ptr )
    {
        if ( NULL == ptr ) return NULL;

        SDL_memset( ptr, 0, sizeof( *ptr ) );

        return ptr;
    }
};

//--------------------------------------------------------------------------------------------

/// Egoboo's representation of the .mpd mesh file
struct ego_mpd
{
    ego_mpd_info  info;
    ego_tile_mem  tmem;
    ego_grid_mem  gmem;

    fvec2_t       tileoff[MAXTILETYPE];     ///< Tile texture offset

    static int mpdfx_tests;
    static int bound_tests;
    static int pressure_tests;

    ego_mpd()  { clear( this ); ctor_this( this ); };
    ~ego_mpd() { dtor_this( this ); }

    static ego_mpd * create( ego_mpd * pmesh, const int tiles_x, const int tiles_y );
    static bool_t      destroy( ego_mpd ** pmesh );

    static ego_mpd * ctor_this( ego_mpd * pmesh );
    static ego_mpd * dtor_this( ego_mpd * pmesh );
    static ego_mpd * renew( ego_mpd * pmesh );

    //---- loading functions
    static bool_t    convert( ego_mpd * pmesh_dst, const mpd_t * pmesh_src );
    static ego_mpd * finalize( ego_mpd * pmesh );

    //---- wall interaction
    static BIT_FIELD hit_wall( ego_mpd * pmesh, const float pos[], const float radius, const BIT_FIELD bits, float nrm[], float * pressure );
    static bool_t    test_wall( ego_mpd * pmesh, const float pos[], const float radius, const BIT_FIELD bits, mesh_wall_data * private_data );
    static fvec2_t   get_diff( ego_mpd * pmesh, const float pos[], const float radius, const float center_pressure, BIT_FIELD bits );
    static float     get_pressure( ego_mpd * pmesh, const float pos[], const float radius, const BIT_FIELD bits );

    static float     get_max_vertex_0( ego_mpd * pmesh, const int grid_x, const int grid_y );
    static float     get_max_vertex_1( ego_mpd * pmesh, const int grid_x, const int grid_y, const float xmin, const float ymin, const float xmax, const float ymax );

    //---- texture stuff
    static bool_t    set_texture( ego_mpd * pmesh, const Uint16 tile, const Uint16 image );
    static bool_t    update_texture( ego_mpd * pmesh, const Uint32 tile );
    static bool_t    make_texture( ego_mpd * pmesh );

    //---- lighting
    static float     light_corners( ego_mpd * pmesh, const int itile, const float mesh_lighting_keep );
    static bool_t    test_corners( ego_mpd * pmesh, const int itile, const float threshold );
    static bool_t    grid_light_one_corner( ego_mpd * pmesh, const int fan, const float height, float nrm[], float * plight );
    static bool_t    remove_ambient( ego_mpd * pmesh );
    static bool_t    light_one_corner( ego_mpd * pmesh, const int itile, const GLXvector3f pos, GLXvector3f nrm, float * plight );
    static bool_t    test_one_corner( const ego_mpd * pmesh, GLXvector3f pos, float * pdelta );

    //---- INLINE functions

    static INLINE float  get_level( const ego_mpd * pmesh, const float x, const float y );
    static INLINE Uint32 get_block( const ego_mpd * pmesh, const float pos_x, const float pos_y );
    static INLINE Uint32 get_tile( const ego_mpd * pmesh, const float pos_x, const float pos_y );

    static INLINE Uint32 get_block_int( const ego_mpd * pmesh, const int block_x, const int block_y );
    static INLINE Uint32 get_tile_int( const ego_mpd * pmesh, const int grid_x,  const int grid_y );

    static INLINE BIT_FIELD test_fx( ego_mpd * pmesh, const Uint32 itile, const BIT_FIELD flags );
    static INLINE bool_t    clear_fx( ego_mpd * pmesh, const Uint32 itile, const BIT_FIELD flags );
    static INLINE bool_t    add_fx( ego_mpd * pmesh, const Uint32 itile, const BIT_FIELD flags );

    static INLINE BIT_FIELD has_some_mpdfx( BIT_FIELD mpdfx, const BIT_FIELD test );
    static INLINE bool_t    grid_is_valid( const ego_mpd * pmpd, const Uint32 id );

protected:

    static void   init_tile_offset( ego_mpd * pmesh );
    static void   make_vrtstart( ego_mpd * pmesh );

    // some twist/normal functions
    static bool_t make_normals( ego_mpd * pmesh );

    static bool_t make_bbox( ego_mpd * pmesh );

    static bool_t      recalc_twist( ego_mpd * pmesh );

private:
    static ego_mpd * ctor_1( ego_mpd * pmesh, const int tiles_x, const int tiles_y );

    static ego_mpd * clear( ego_mpd * pmesh )
    {
        if ( NULL == pmesh ) return pmesh;

        init_tile_offset( pmesh );

        return pmesh;
    }

};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// loading/saving
ego_mpd * mesh_load( const char *modname, ego_mpd * pmesh );

void   mesh_make_twist();

Uint8 cartman_get_fan_twist( ego_mpd * pmesh, const Uint32 tile );
