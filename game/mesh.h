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
/// @file
/// @brief Egoboo Mesh ( .mpd ) file Loading
/// @details Definitions for raw reading of the mpd file.

#include "egoboo_types.h"
#include "egoboo_math.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct sGame;
struct s_water_info;
struct sPhysicsData;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct s_twist_entry
{
  Uint32       lr;           ///< For surface normal of mesh
  Uint32       ud;           //
  vect3        nrm;          ///< For sliding down steep hills
  bool_t       flat;         //
};
typedef struct s_twist_entry TWIST_ENTRY;

extern TWIST_ENTRY twist_table[256];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define INVALID_BUMPLIST_NODE (~(Uint32)0)

struct s_bumplist_node
{
  Uint32 ref;
  Uint32 next;
};
typedef struct s_bumplist_node BUMPLIST_NODE;

struct s_bumplist
{
  egoboo_key_t ekey;
  bool_t     allocated;
  bool_t     filled;

  Uint32   num_blocks;              ///< Number of collision areas

  Uint32          free_count;
  Uint32          free_max;
  Uint32        * free_lst;
  BUMPLIST_NODE * node_lst;

  BUMPLIST_NODE * chr_ref;                 ///< For character collisions
  Uint16        * num_chr;                 ///< Number on the block

  BUMPLIST_NODE  * prt_ref;                 ///< For particle collisions
  Uint16         * num_prt;                 ///< Number on the block
};
typedef struct s_bumplist BUMPLIST;



//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define MAPID                           0x4470614d      ///< The string 'MapD'

#define MAXMESHFAN                      (512*512)      // Terrain mesh size
#define MAXMESHSIZEY                    1024            ///< Max fans in y direction
#define BYTESFOREACHVERTEX              (3*sizeof(float) + 3*sizeof(Uint8) + 3*sizeof(float))
#define MAXMESHVERTICES                 16              ///< Fansquare vertices
#define MAXMESHTYPE                     64              ///< Number of fansquare command types
#define MAXMESHCOMMAND                  4               ///< Draw up to 4 fans
#define MAXMESHCOMMANDENTRIES           32              ///< Fansquare command list size
#define MAXMESHCOMMANDSIZE              32              ///< Max trigs in each command
#define MAXTILETYPE                     (64*4)         // Max number of tile images
#define MAXMESHRENDER                   (1024*8)       // Max number of tiles to draw
#define INVALID_TILE                    ((Uint16)(~(Uint16)0))   // Don't draw the fansquare if tile = this
#define INVALID_FAN                     ((Uint32)(~(Uint32)0))   // Character not on a fan ( maybe )

#define FAN_BITS 7
#define FAN_MASK (1-(1<<FAN_BITS))
#define MESH_FAN_TO_INT(XX)    ( (XX) << FAN_BITS )
#define MESH_INT_TO_FAN(XX)    ( (XX) >> FAN_BITS )
#define MESH_FAN_TO_FLOAT(XX)  ( (float)(XX) * (float)(1<<FAN_BITS) )
#define MESH_FLOAT_TO_FAN(XX)  ( (XX) / (float)(1<<FAN_BITS))

#define BLOCK_BITS 9
#define MESH_BLOCK_TO_INT(XX)    ( (XX) << BLOCK_BITS )
#define MESH_INT_TO_BLOCK(XX)    ( (XX) >> BLOCK_BITS )
#define MESH_BLOCK_TO_FLOAT(XX)  ( (float)(XX) * (float)(1<<BLOCK_BITS) )
#define MESH_FLOAT_TO_BLOCK(XX)  ( (XX) / (float)(1<<BLOCK_BITS))

enum e_mpd_effect_bits
{
  MPDFX_REF                     =      0,         ///< This tile is drawn 1st
  MPDFX_NOREFLECT               = 1 << 0,         ///< This tile IS reflected in the floors
  MPDFX_SHINY                   = 1 << 1,         ///< Draw reflection of characters
  MPDFX_ANIM                    = 1 << 2,         ///< Animated tile ( 4 frame )
  MPDFX_WATER                   = 1 << 3,         ///< Render water above surface ( Water details are set per module )
  MPDFX_WALL                    = 1 << 4,         ///< Wall ( Passable by ghosts, particles )
  MPDFX_IMPASS                  = 1 << 5,         ///< Impassable
  MPDFX_DAMAGE                  = 1 << 6,         ///< Damage
  MPDFX_SLIPPY                  = 1 << 7          ///< Ice or normal
};
typedef enum e_mpd_effect_bits MPDFX_BITS;

#define SLOPE                           800      ///< Slope increments for terrain normals
#define SLIDE                           .04          ///< Acceleration for steep hills
#define SLIDEFIX                        .08          ///< To make almost flat surfaces flat

enum e_fan_type
{
  GROUND_1,   ///<  0  Two Faced Ground...
  GROUND_2,   ///<  1  Two Faced Ground...
  GROUND_3,   ///<  2  Four Faced Ground...
  GROUND_4,   ///<  3  Eight Faced Ground...
  PILLAR_1,   ///<  4  Ten Face Pillar...
  PILLAR_2,   ///<  5  Eighteen Faced Pillar...
  BLANK_1,    ///<  6  Blank...
  BLANK_2,    ///<  7  Blank...
  WALL_WE,    ///<  8  Six Faced Wall (WE)...
  WALL_NS,    ///<  9  Six Faced Wall (NS)...
  BLANK_3,    ///<  10  Blank...
  BLANK_4,    ///<  11  Blank...
  WALL_W,     ///<  12  Eight Faced Wall (W)...
  WALL_N,     ///<  13  Eight Faced Wall (N)...
  WALL_E,     ///<  14  Eight Faced Wall (E)...
  WALL_S,     ///<  15  Eight Faced Wall (S)...
  CORNER_WS,  ///<  16  Ten Faced Wall (WS)...
  CORNER_NW,  ///<  17  Ten Faced Wall (NW)...
  CORNER_NE,  ///<  18  Ten Faced Wall (NE)...
  CORNER_ES,  ///<  19  Ten Faced Wall (ES)...
  ALCOVE_WSE, ///<  20  Twelve Faced Wall (WSE)...
  ALCOVE_NWS, ///<  21  Twelve Faced Wall (NWS)...
  ALCOVE_ENW, ///<  22  Twelve Faced Wall (ENW)...
  ALCOVE_SEN, ///<  23  Twelve Faced Wall (SEN)...
  STAIR_WE,   ///<  24  Twelve Faced Stair (WE)...
  STAIR_NS    ///<  25  Twelve Faced Stair (NS)...
};
typedef enum e_fan_type FAN_TYPE;

struct sMeshInfo
{
  egoboo_key_t ekey;

  int     vert_count;                       ///< Total mesh vertices

  int     tiles_x;                          ///< Mesh size in tiles
  int     tiles_y;
  size_t  tile_count;

  int     blocks_x;
  int     blocks_y;
  size_t  block_count;

  float   edge_x;                                          ///< floating point size of mesh
  float   edge_y;                                          //

  Uint16  last_texture;                                    ///< Last texture used

  Uint32  Block_X[(MAXMESHSIZEY >> 2) +1];
  Uint32  Tile_X[MAXMESHSIZEY];                         ///< Which fan to start a row with

  BUMPLIST bumplist;
};
typedef struct sMeshInfo MeshInfo_t;

MeshInfo_t * MeshInfo_new(MeshInfo_t *);
bool_t       MeshInfo_delete(MeshInfo_t *);

struct sMeshTile
{
  Uint8   type;                               ///< Command type
  Uint8   fx;                                 ///< Special effects flags
  Uint8   twist;                              //
  bool_t  inrenderlist;                       //
  Uint16  tile;                               ///< Get texture from this
  Uint32  vrt_start;                          ///< Which vertex to start at
  AA_BBOX bbox;                               ///< what is the minimum extent of the fan
};
typedef struct sMeshTile MeshTile_t;

struct sMeshMem
{
  egoboo_key_t ekey;

  int     vrt_count;
  void *  base;                                                 ///< For malloc

  float*  vrt_x;                                           ///< Vertex position
  float*  vrt_y;                                           //
  float*  vrt_z;                                           ///< Vertex elevation

  Uint8*  vrt_ar_fp8;                                           ///< Vertex base light
  Uint8*  vrt_ag_fp8;                                           ///< Vertex base light
  Uint8*  vrt_ab_fp8;                                           ///< Vertex base light

  float*  vrt_lr_fp8;                                           ///< Vertex light
  float*  vrt_lg_fp8;                                           ///< Vertex light
  float*  vrt_lb_fp8;                                           ///< Vertex light

  Uint32        tile_count;
  MeshTile_t  * tilelst;
};
typedef struct sMeshMem MeshMem_t;

MeshMem_t * MeshMem_new(MeshMem_t * mem, int vertcount, int fancount);
bool_t      MeshMem_delete(MeshMem_t * mem);

struct sTileDefinition
{
  Uint8   cmd_count;                  ///< Number of commands
  Uint8   cmd_size[MAXMESHCOMMAND];   ///< Entries in each command
  Uint16  vrt[MAXMESHCOMMANDENTRIES]; ///< Fansquare vertex list

  Uint8   vrt_count;                  ///< Number of vertices
  Uint8   ref[MAXMESHVERTICES];       ///< Lighting references
  vect2   tx[MAXMESHVERTICES];        ///< Relative texture coordinates
};
typedef struct sTileDefinition TileDefinition_t;

typedef TileDefinition_t TileDictionary_t[MAXMESHTYPE];

extern TileDictionary_t gTileDict;

// an encapsulating structore for the mesh
struct sMesh
{
  MeshInfo_t       Info;
  MeshMem_t        Mem;
  TileDictionary_t TileDict;
};
typedef struct sMesh Mesh_t;


struct s_mesh_tile_txbox
{
  vect2   off;                          ///< Tile texture offset
};
typedef struct s_mesh_tile_txbox TILE_TXBOX;

extern TILE_TXBOX gTileTxBox[MAXTILETYPE];

bool_t TileDictionary_load(TileDictionary_t * pdict);
void mesh_make_fanstart(MeshInfo_t * mi);
void mesh_make_twist( void );
bool_t mesh_load( Mesh_t * pmesh, char *modname );


bool_t mesh_calc_normal_fan( Mesh_t * pmesh, struct sPhysicsData * phys, Uint32 fan, vect3 * pnrm, vect3 * ppos );
bool_t mesh_calc_normal_pos( Mesh_t * pmesh, struct sPhysicsData * phys, Uint32 fan, vect3 pos, vect3 * pnrm );
bool_t mesh_calc_normal( Mesh_t * pmesh, struct sPhysicsData * phys, vect3 pos, vect3 * pnrm );

float mesh_get_level( MeshMem_t * mm, Uint32 fan, float x, float y, bool_t waterwalk, struct s_water_info * wi );


Uint32 mesh_hitawall( Mesh_t * pmesh, vect3 pos, float tiles_x, float tiles_y, Uint32 collision_bits, vect3 * nrm );

bool_t mesh_reset_bumplist(MeshInfo_t * mi);
