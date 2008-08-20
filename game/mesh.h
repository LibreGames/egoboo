#pragma once

#include "egoboo_types.h"
#include "egoboo_math.h"

struct sGame;
struct s_water_info;
struct sPhysicsData;

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

  Uint32   num_blocks;              // Number of collision areas

  Uint32          free_count;
  Uint32          free_max;
  Uint32        * free_lst;
  BUMPLIST_NODE * node_lst;

  BUMPLIST_NODE * chr_ref;                 // For character collisions
  Uint16        * num_chr;                 // Number on the block

  BUMPLIST_NODE  * prt_ref;                 // For particle collisions
  Uint16         * num_prt;                 // Number on the block
};
typedef struct s_bumplist BUMPLIST;

INLINE const BUMPLIST * bumplist_new(BUMPLIST * b);
INLINE const bool_t     bumplist_delete(BUMPLIST * b);
INLINE const BUMPLIST * bumplist_renew(BUMPLIST * b);
INLINE const bool_t     bumplist_allocate(BUMPLIST * b, int size);
INLINE const bool_t     bumplist_clear( BUMPLIST * b );

INLINE const Uint32     bumplist_get_free(BUMPLIST * b);

INLINE const bool_t     bumplist_insert_chr(BUMPLIST * b, Uint32 block, CHR_REF chr_ref);
INLINE const bool_t     bumplist_insert_prt(BUMPLIST * b, Uint32 block, PRT_REF prt_ref);

INLINE const Uint32     bumplist_get_chr_head(BUMPLIST * b, Uint32 block);
INLINE const Uint32     bumplist_get_prt_head(BUMPLIST * b, Uint32 block);
INLINE const Uint32     bumplist_get_chr_count(BUMPLIST * b, Uint32 block );
INLINE const Uint32     bumplist_get_prt_count(BUMPLIST * b, Uint32 block );

INLINE const Uint32     bumplist_get_next(BUMPLIST * b, Uint32 node );
INLINE const Uint32     bumplist_get_ref(BUMPLIST * b, Uint32 node );
INLINE const Uint32     bumplist_get_next_prt(struct sGame * gs, BUMPLIST * b, Uint32 node );
INLINE const Uint32     bumplist_get_next_chr(struct sGame * gs, BUMPLIST * b, Uint32 node );

#define MAPID                           0x4470614d     // The string 'MapD'

#define MAXMESHFAN                      (512*512)      // Terrain mesh size
#define MAXMESHSIZEY                    1024           // Max fans in y direction
#define BYTESFOREACHVERTEX              (3*sizeof(float) + 3*sizeof(Uint8) + 3*sizeof(float))
#define MAXMESHVERTICES                 16             // Fansquare vertices
#define MAXMESHTYPE                     64             // Number of fansquare command types
#define MAXMESHCOMMAND                  4              // Draw up to 4 fans
#define MAXMESHCOMMANDENTRIES           32             // Fansquare command list size
#define MAXMESHCOMMANDSIZE              32             // Max trigs in each command
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
  MPDFX_REF                     =      0,         // 0 This tile is drawn 1st
  MPDFX_NOREFLECT               = 1 << 0,         // 0 This tile IS reflected in the floors
  MPDFX_SHINY                   = 1 << 1,         // 1 Draw reflection of characters
  MPDFX_ANIM                    = 1 << 2,         // 2 Animated tile ( 4 frame )
  MPDFX_WATER                   = 1 << 3,         // 3 Render water above surface ( Water details are set per module )
  MPDFX_WALL                    = 1 << 4,         // 4 Wall ( Passable by ghosts, particles )
  MPDFX_IMPASS                  = 1 << 5,         // 5 Impassable
  MPDFX_DAMAGE                  = 1 << 6,         // 6 Damage
  MPDFX_SLIPPY                  = 1 << 7          // 7 Ice or normal
};
typedef enum e_mpd_effect_bits MPDFX_BITS;

#define SLOPE                           800     // Slope increments for terrain normals
#define SLIDE                           .04         // Acceleration for steep hills
#define SLIDEFIX                        .08         // To make almost flat surfaces flat

enum e_fan_type
{
  GROUND_1,  //  0  Two Faced Ground...
  GROUND_2,   //  1  Two Faced Ground...
  GROUND_3,   //  2  Four Faced Ground...
  GROUND_4,   //  3  Eight Faced Ground...
  PILLAR_1,   //  4  Ten Face Pillar...
  PILLAR_2,   //  5  Eighteen Faced Pillar...
  BLANK_1,    //  6  Blank...
  BLANK_2,    //  7  Blank...
  WALL_WE,    //  8  Six Faced Wall (WE)...
  WALL_NS,    //  9  Six Faced Wall (NS)...
  BLANK_3,    //  10  Blank...
  BLANK_4,    //  11  Blank...
  WALL_W,     //  12  Eight Faced Wall (W)...
  WALL_N,     //  13  Eight Faced Wall (N)...
  WALL_E,     //  14  Eight Faced Wall (E)...
  WALL_S,     //  15  Eight Faced Wall (S)...
  CORNER_WS,  //  16  Ten Faced Wall (WS)...
  CORNER_NW,  //  17  Ten Faced Wall (NW)...
  CORNER_NE,  //  18  Ten Faced Wall (NE)...
  CORNER_ES,  //  19  Ten Faced Wall (ES)...
  ALCOVE_WSE, //  20  Twelve Faced Wall (WSE)...
  ALCOVE_NWS, //  21  Twelve Faced Wall (NWS)...
  ALCOVE_ENW, //  22  Twelve Faced Wall (ENW)...
  ALCOVE_SEN, //  23  Twelve Faced Wall (SEN)...
  STAIR_WE,   //  24  Twelve Faced Stair (WE)...
  STAIR_NS    //  25  Twelve Faced Stair (NS)...
};
typedef enum e_fan_type FAN_TYPE;

struct sMeshInfo
{
  bool_t  exploremode;                      // Explore mode?

  int     vert_count;                       // Total mesh vertices

  int     tiles_x;                          // Mesh size in tiles
  int     tiles_y;
  int     tile_count;

  int     blocks_x;
  int     blocks_y;
  int     block_count;

  float   edge_x;                                          // floating point size of mesh
  float   edge_y;                                          //

  Uint16  last_texture;                                    // Last texture used

  Uint32  Block_X[(MAXMESHSIZEY >> 2) +1];
  Uint32  Tile_X[MAXMESHSIZEY];                         // Which fan to start a row with

  BUMPLIST bumplist;
};
typedef struct sMeshInfo MeshInfo_t;

struct sMeshTile
{
  Uint8   type;                               // Command type
  Uint8   fx;                                 // Special effects flags
  Uint8   twist;                              //
  bool_t  inrenderlist;                       //
  Uint16  tile;                               // Get texture from this
  Uint32  vrt_start;                          // Which vertex to start at
  AA_BBOX bbox;                               // what is the minimum extent of the fan
};
typedef struct sMeshTile MeshTile_t;

struct sMeshMem
{
  egoboo_key_t ekey;

  int     vrt_count;
  void *  base;                                                 // For malloc

  float*  vrt_x;                                           // Vertex position
  float*  vrt_y;                                           //
  float*  vrt_z;                                           // Vertex elevation

  Uint8*  vrt_ar_fp8;                                           // Vertex base light
  Uint8*  vrt_ag_fp8;                                           // Vertex base light
  Uint8*  vrt_ab_fp8;                                           // Vertex base light

  float*  vrt_lr_fp8;                                           // Vertex light
  float*  vrt_lg_fp8;                                           // Vertex light
  float*  vrt_lb_fp8;                                           // Vertex light

  int         tile_count;
  MeshTile_t  * tilelst;
};
typedef struct sMeshMem MeshMem_t;

MeshMem_t * MeshMem_new(MeshMem_t * mem, int vertcount, int fancount);
bool_t      MeshMem_delete(MeshMem_t * mem);

struct sTileDefinition
{
  Uint8   cmd_count;                  // Number of commands
  Uint8   cmd_size[MAXMESHCOMMAND];   // Entries in each command
  Uint16  vrt[MAXMESHCOMMANDENTRIES]; // Fansquare vertex list

  Uint8   vrt_count;                  // Number of vertices
  Uint8   ref[MAXMESHVERTICES];       // Lighting references
  vect2   tx[MAXMESHVERTICES];        // Relative texture coordinates
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

INLINE Mesh_t * Mesh_new( Mesh_t * pmesh );
INLINE bool_t   Mesh_delete( Mesh_t * pmesh );

struct s_mesh_tile_txbox
{
  vect2   off;                          // Tile texture offset
};
typedef struct s_mesh_tile_txbox TILE_TXBOX;

extern TILE_TXBOX gTileTxBox[MAXTILETYPE];

bool_t TileDictionary_load(TileDictionary_t * pdict);
void mesh_make_fanstart(MeshInfo_t * mi);
void mesh_make_twist();
bool_t mesh_load( Mesh_t * pmesh, char *modname );


bool_t mesh_calc_normal_fan( Mesh_t * pmesh, struct sPhysicsData * phys, int fan, vect3 * pnrm, vect3 * ppos );
bool_t mesh_calc_normal_pos( Mesh_t * pmesh, struct sPhysicsData * phys, int fan, vect3 pos, vect3 * pnrm );
bool_t mesh_calc_normal( Mesh_t * pmesh, struct sPhysicsData * phys, vect3 pos, vect3 * pnrm );

float mesh_get_level( MeshMem_t * mm, Uint32 fan, float x, float y, bool_t waterwalk, struct s_water_info * wi );


Uint32 mesh_hitawall( Mesh_t * pmesh, vect3 pos, float tiles_x, float tiles_y, Uint32 collision_bits, vect3 * nrm );

INLINE const Uint32 mesh_get_fan( Mesh_t * pmesh, vect3 pos );
INLINE const Uint32 mesh_get_block( MeshInfo_t * mi, vect3 pos );

INLINE void mesh_set_colora( Mesh_t * pmesh, int fan_x, int fan_y, int color );

INLINE const bool_t mesh_fan_clear_bits( Mesh_t * pmesh,  int fan_x, int fan_y, Uint32 bits );
INLINE const bool_t mesh_fan_add_bits( Mesh_t * pmesh,  int fan_x, int fan_y, Uint32 bits );
INLINE const bool_t mesh_fan_set_bits( Mesh_t * pmesh, int fan_x, int fan_y, Uint32 bits );

INLINE const int   mesh_bump_tile( Mesh_t * pmesh, int fan_x, int fan_y );
INLINE const Uint16 mesh_get_tile( Mesh_t * pmesh, int fan_x, int fan_y );
INLINE const bool_t mesh_set_tile( Mesh_t * pmesh, int fan_x, int fan_y, Uint32 become );

INLINE const Uint32 mesh_convert_fan( MeshInfo_t * mi, int fan_x, int fan_y );
INLINE const Uint32 mesh_convert_block( MeshInfo_t * mi, int block_x, int block_y );

INLINE const float mesh_fraction_x( MeshInfo_t * mi, float x );
INLINE const float mesh_fraction_y( MeshInfo_t * mi, float y );

INLINE const bool_t mesh_fan_is_in_renderlist(  MeshTile_t * mf_list, int fan );
INLINE       void   mesh_fan_remove_renderlist( MeshTile_t * mf_list, int fan );
INLINE       void   mesh_fan_add_renderlist( MeshTile_t * mf_list, int fan );

INLINE const float mesh_clip_x( MeshInfo_t * mi, float x );
INLINE const float mesh_clip_y( MeshInfo_t * mi, float y );
INLINE const int mesh_clip_fan_x( MeshInfo_t * mi, int fan_x );
INLINE const int mesh_clip_fan_y( MeshInfo_t * mi, int fan_y );
INLINE const int mesh_clip_block_x( MeshInfo_t * mi, int block_x );
INLINE const int mesh_clip_block_y( MeshInfo_t * mi, int block_y );

INLINE const bool_t mesh_check( MeshInfo_t * mi, float x, float y );
INLINE const bool_t mesh_check_fan( MeshInfo_t * mi, int fan_x, int fan_y );

INLINE const Uint32 mesh_test_bits( MeshTile_t * mf_list, int fan, Uint32 bits );
INLINE const bool_t mesh_has_some_bits( MeshTile_t * mf_list, int fan, Uint32 bits );
INLINE const bool_t mesh_has_no_bits( MeshTile_t * mf_list, int fan, Uint32 bits );
INLINE const bool_t mesh_has_all_bits( MeshTile_t * mf_list, int fan, Uint32 bits );

INLINE const Uint8 mesh_get_twist( MeshTile_t * mf_list, int fan );

bool_t mesh_reset_bumplist(MeshInfo_t * mi);
