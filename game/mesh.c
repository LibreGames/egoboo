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
/// @brief Raw MPD loader
/// @details This part handles MPD mesh related stuff.

#include "mesh.inl"

#include "Log.h"
#include "file_common.h"

#include "egoboo_utility.h"
#include "egoboo.h"

#include "char.inl"
#include "particle.inl"
#include "egoboo_types.inl"

TILE_TXBOX       gTileTxBox[MAXTILETYPE];
TileDictionary_t gTileDict;
TWIST_ENTRY      twist_table[256];

#define TX_FUDGE 0.5f

//--------------------------------------------------------------------------------------------
bool_t mesh_reset_bumplist(MeshInfo_t * mi)
{
  size_t i;

  BUMPLIST  * pbump  = &(mi->bumplist);

  if(!pbump->allocated) return bfalse;

  bumplist_clear(pbump);

  for(i=0; i<pbump->free_max; i++)
  {
    pbump->free_lst[i] = i;
    bumplist_node_new( pbump->node_lst + i );
  }
  pbump->free_count = pbump->free_max;

  //pbump->initialized = btrue;
  pbump->filled      = bfalse;

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t mesh_allocate_bumplist(MeshInfo_t * mi, int blocks)
{
  BUMPLIST * pbump  = &(mi->bumplist);

  if(!EKEY_PVALID(pbump)) bumplist_new(pbump);

  if( bumplist_allocate(pbump, blocks) )
  {
    // set up nodes and the list of free nodes
    pbump->free_max = 8*(CHRLST_COUNT + PRTLST_COUNT);
    pbump->free_lst = EGOBOO_NEW_ARY( Uint32, pbump->free_max );
    pbump->node_lst = EGOBOO_NEW_ARY( BUMPLIST_NODE, pbump->free_max );

    // set this to 0 until the list has been set up
    pbump->free_count = 0;

    mesh_reset_bumplist(mi);
  }

  return btrue;
};

//--------------------------------------------------------------------------------------------
bool_t mesh_load( Mesh_t * pmesh, char *modname )
{
  /// @details ZZ@> This function loads the "LEVEL.MPD" file

  FILE* fileread;
  STRING newloadname;
  int itmp, cnt;
  float ftmp;
  Uint32 fan;
  int vert, vrt;

  MeshMem_t   * mem     = &(pmesh->Mem);
  MeshInfo_t  * mi      = &(pmesh->Info);
  MeshTile_t  * mf_list;

  snprintf( newloadname, sizeof( newloadname ), "%s%s" SLASH_STRING "%s", modname, CData.gamedat_dir, CData.mesh_file );
  fileread = fs_fileOpen( PRI_NONE, NULL, newloadname, "rb" );
  if ( NULL == fileread ) return bfalse;

  fread( &itmp, 4, 1, fileread );  if (SDL_SwapLE32( itmp ) != MAPID ) return bfalse;
  fread( &itmp, 4, 1, fileread );  mi->vert_count = SDL_SwapLE32( itmp );
  fread( &itmp, 4, 1, fileread );  mi->tiles_x    = SDL_SwapLE32( itmp );
  fread( &itmp, 4, 1, fileread );  mi->tiles_y    = SDL_SwapLE32( itmp );
  mi->tile_count = mi->tiles_x * mi->tiles_y;

  mi->blocks_x = mi->tiles_x >> 2;
  mi->blocks_y = mi->tiles_y >> 2;
  if( 0 != (mi->tiles_x & 0x03) ) mi->blocks_x++;   // check for multiples of 4
  if( 0 != (mi->tiles_y & 0x03) ) mi->blocks_y++;
  mi->block_count = mi->blocks_x * mi->blocks_y;

  if ( !MeshMem_new(mem, mi->vert_count, mi->tile_count) )
  {
    log_error( "mesh_load() - \n\tUnable to initialize Mesh Memory. MPD file %s has too many vertices.\n", modname );
    return bfalse;
  }

  // wait until fanlist is allocated!
  mf_list = mem->tilelst;

  mi->edge_x = mi->tiles_x * 128;
  mi->edge_y = mi->tiles_y * 128;

  // allocate the bumplist
  mesh_allocate_bumplist( mi, mi->block_count );

  // Load fan data
  for ( fan = 0; fan < mi->tile_count;  fan++)
  {
    fread( &itmp, 4, 1, fileread );
    itmp = SDL_SwapLE32( itmp );

    mf_list[fan].type = itmp >> 24;
    mf_list[fan].fx   = itmp >> 16;
    mf_list[fan].tile = itmp;
  }

  // Load fan data
  for ( fan = 0; fan < mi->tile_count; fan++ )
  {
    fread( &itmp, 1, 1, fileread );
    mf_list[fan].twist = itmp;
  }

  // Load vertex fan_x data
  for( cnt = 0;  cnt < mi->vert_count; cnt++ )
  {
    fread( &ftmp, 4, 1, fileread );
    mem->vrt_x[cnt] = SwapLE_float( ftmp );
  }

  // Load vertex fan_y data
  for( cnt = 0; cnt < mi->vert_count; cnt++ )
  {
    fread( &ftmp, 4, 1, fileread );
    mem->vrt_y[cnt] = SwapLE_float( ftmp );
  }

  // Load vertex z data
  cnt = 0;
  for( cnt = 0; cnt < mi->vert_count; cnt++ )
  {
    fread( &ftmp, 4, 1, fileread );
    mem->vrt_z[cnt] = SwapLE_float( ftmp )  / 16.0;  // Cartman uses 4 bit fixed point for Z
  }

  // Load vertex lighting data

  for ( cnt = 0; cnt < mi->vert_count; cnt++ )
  {
    fread( &itmp, 1, 1, fileread );

    mem->vrt_ar_fp8[cnt] = 0; // itmp;
    mem->vrt_ag_fp8[cnt] = 0; // itmp;
    mem->vrt_ab_fp8[cnt] = 0; // itmp;

    mem->vrt_lr_fp8[cnt] = 0;
    mem->vrt_lg_fp8[cnt] = 0;
    mem->vrt_lb_fp8[cnt] = 0;
  }

  fs_fileClose( fileread );

  mesh_make_fanstart( mi );

  vert = 0;
  for ( fan = 0; fan < mem->tile_count; fan++ )
  {
    int vrtcount = pmesh->TileDict[mf_list[fan].type].vrt_count;
    int vrtstart = vert;

    mf_list[fan].vrt_start = vrtstart;

    mf_list[fan].bbox.mins.x = mf_list[fan].bbox.maxs.x = mem->vrt_x[vrtstart];
    mf_list[fan].bbox.mins.y = mf_list[fan].bbox.maxs.y = mem->vrt_y[vrtstart];
    mf_list[fan].bbox.mins.z = mf_list[fan].bbox.maxs.z = mem->vrt_z[vrtstart];

    for ( vrt = vrtstart + 1; vrt < vrtstart + vrtcount; vrt++ )
    {
      mf_list[fan].bbox.mins.x = MIN( mf_list[fan].bbox.mins.x, mem->vrt_x[vrt] );
      mf_list[fan].bbox.mins.y = MIN( mf_list[fan].bbox.mins.y, mem->vrt_y[vrt] );
      mf_list[fan].bbox.mins.z = MIN( mf_list[fan].bbox.mins.z, mem->vrt_z[vrt] );

      mf_list[fan].bbox.maxs.x = MAX( mf_list[fan].bbox.maxs.x, mem->vrt_x[vrt] );
      mf_list[fan].bbox.maxs.y = MAX( mf_list[fan].bbox.maxs.y, mem->vrt_y[vrt] );
      mf_list[fan].bbox.maxs.z = MAX( mf_list[fan].bbox.maxs.z, mem->vrt_z[vrt] );
    }

    vert += vrtcount;
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t TileDictionary_load(TileDictionary_t * pdict)
{
  /// @details ZZ@> This function loads fan types for the terrain

  int cnt, entry;
  int numfantype, fantype, bigfantype, vertices;
  int numcommand, command, commandsize;
  FILE* fileread;

  // Initialize all mesh types to 0
  entry = 0;
  while ( entry < MAXMESHTYPE )
  {
    (*pdict)[entry].vrt_count = 0;
    (*pdict)[entry].cmd_count = 0;
    entry++;
  }


  // Open the file and go to it
  log_info("Loading fan types of the terrain... ");
  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.fans_file );
  fileread = fs_fileOpen( PRI_NONE, NULL, CStringTmp1, "r" );
  if ( NULL == fileread )
  {
    log_message("Failed!\n");
    return bfalse;
  }
  log_message("Succeeded!\n");

  fantype    = 0;               // 32 x 32 tiles
  bigfantype = MAXMESHTYPE / 2; // 64 x 64 tiles
  numfantype = fget_next_int( fileread );
  for ( /* nothing */; fantype < numfantype; fantype++, bigfantype++ )
  {
    vertices                       =
    (*pdict)[   fantype].vrt_count =
    (*pdict)[bigfantype].vrt_count = fget_next_int( fileread );  // Dupe

    for ( cnt = 0; cnt < vertices; cnt++ )
    {
      (*pdict)[   fantype].ref[cnt] =
      (*pdict)[bigfantype].ref[cnt] = fget_next_int( fileread );

      (*pdict)[   fantype].tx[cnt].u =
      (*pdict)[bigfantype].tx[cnt].u = fget_next_float( fileread );

      (*pdict)[   fantype].tx[cnt].v =
      (*pdict)[bigfantype].tx[cnt].v = fget_next_float( fileread );
    }

    numcommand                     =
    (*pdict)[   fantype].cmd_count =
    (*pdict)[bigfantype].cmd_count = fget_next_int( fileread );  // Dupe

    for ( entry = 0, command = 0; command < numcommand; command++ )
    {
      commandsize                            =
      (*pdict)[   fantype].cmd_size[command] =
      (*pdict)[bigfantype].cmd_size[command] = fget_next_int( fileread );  // Dupe

      for ( cnt = 0; cnt < commandsize; cnt++ )
      {
        (*pdict)[   fantype].vrt[entry] =
        (*pdict)[bigfantype].vrt[entry] = fget_next_int( fileread );  // Dupe
        entry++;
      }
    }

  }
  fs_fileClose( fileread );



  // Correct silly Cartman 32-pixel-wide textures to Egoboo's 256 pixel wide textures

  for ( cnt = 0; cnt < MAXMESHTYPE / 2; cnt++ )
  {
    for ( entry = 0; entry < (*pdict)[cnt].vrt_count; entry++ )
    {
      (*pdict)[cnt].tx[entry].u = ( TX_FUDGE + (*pdict)[cnt].tx[entry].u * ( 31.0f - TX_FUDGE ) ) / 256.0f;
      (*pdict)[cnt].tx[entry].v = ( TX_FUDGE + (*pdict)[cnt].tx[entry].v * ( 31.0f - TX_FUDGE ) ) / 256.0f;
    }

    // blank the unused values
    for ( /* nothing */; entry < MAXMESHVERTICES; entry++ )
    {
      (*pdict)[cnt].tx[entry].u = -1.0f;
      (*pdict)[cnt].tx[entry].v = -1.0f;
    }
  }

  // Correct silly Cartman 64-pixel-wide textures to Egoboo's 256 pixel wide textures
  for ( cnt = MAXMESHTYPE / 2; cnt < MAXMESHTYPE; cnt++ )
  {
    for ( entry = 0; entry < (*pdict)[cnt].vrt_count; entry++ )
    {
      (*pdict)[cnt].tx[entry].u = ( TX_FUDGE  + (*pdict)[cnt].tx[entry].u * ( 63.0f - TX_FUDGE ) ) / 256.0f;
      (*pdict)[cnt].tx[entry].v = ( TX_FUDGE  + (*pdict)[cnt].tx[entry].v * ( 63.0f - TX_FUDGE ) ) / 256.0f;
    }

    // blank the unused values
    for ( /* nothing */; entry < MAXMESHVERTICES; entry++ )
    {
      (*pdict)[cnt].tx[entry].u = -1.0f;
      (*pdict)[cnt].tx[entry].v = -1.0f;
    }
  }

  // Make tile texture offsets
  // 64 tiles per texture, 4 textures
  for ( cnt = 0; cnt < MAXTILETYPE; cnt++ )
  {
    gTileTxBox[cnt].off.u = ( ( cnt >> 0 ) & 7 ) / 8.0f;
    gTileTxBox[cnt].off.v = ( ( cnt >> 3 ) & 7 ) / 8.0f;
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
void mesh_make_fanstart(MeshInfo_t * mi)
{
  /// @details ZZ@> This function builds a look up table to ease calculating the
  ///     fan number given an x,y pair

  int cnt;

  for ( cnt = 0; cnt < mi->tiles_y; cnt++ )
  {
    mi->Tile_X[cnt] = mi->tiles_x * cnt;
  }

  for ( cnt = 0; cnt < mi->blocks_y; cnt++ )
  {
    mi->Block_X[cnt] = mi->blocks_x * cnt;
  }
}

//--------------------------------------------------------------------------------------------
void mesh_make_twist()
{
  /// @details ZZ@> This function precomputes surface normals and steep hill acceleration for
  ///     the mesh

  int cnt;
  int x, y;
  float fx, fy, fz, ftmp;
  float xslide, yslide;


  for ( cnt = 0; cnt < 256; cnt++ )
  {
    y = cnt >> 4;
    x = cnt & 15;

    fy = y - 7;  // -7 to 8
    fx = x - 7;  // -7 to 8
    twist_table[cnt].ud = 32768 + fy * SLOPE;
    twist_table[cnt].lr = 32768 + fx * SLOPE;

    ftmp = fx * fx + fy * fy;
    if ( ftmp > 121.0f )
    {
      fz = 0.0f;
      ftmp = sqrt( ftmp );
      fx /= ftmp;
      fy /= ftmp;
    }
    else
    {
      fz = sqrt( 1.0f - ftmp / 121.0f );
      fx /= 11.0f;
      fy /= 11.0f;
    }

    xslide = fx;
    yslide = fy;

    twist_table[cnt].nrm.x =  fx;
    twist_table[cnt].nrm.y = -fy;
    twist_table[cnt].nrm.z =  fz;
    twist_table[cnt].nrm = Normalize(twist_table[cnt].nrm);

    twist_table[cnt].flat = fz > ( 1.0 - SLIDEFIX );
  }
}

//--------------------------------------------------------------------------------------------
bool_t mesh_calc_normal_fan( Mesh_t * pmesh, PhysicsData_t * phys, Uint32 fan, vect3 * pnrm, vect3 * ppos )
{
  bool_t retval = bfalse;
  Uint32 cnt;
  vect3 normal, position;

  MeshMem_t * mm = &(pmesh->Mem);

  if ( INVALID_TILE == fan )
  {
    normal.x = 0.0f;
    normal.y = 0.0f;
    normal.z = MESH_FAN_TO_FLOAT( 1 );

    VectorClear( position.v );
  }
  else
  {
    float dzdx, dzdy, dx, dy;
    float z0, z1, z2, z3;
    int vrtstart = mm->tilelst[fan].vrt_start;

    VectorClear( position.v );
    for ( cnt = 0; cnt < 4; cnt++ )
    {
      position.x += mm->vrt_x[vrtstart + cnt];
      position.y += mm->vrt_y[vrtstart + cnt];
      position.z += mm->vrt_z[vrtstart + cnt];
    };
    position.x /= 4.0f;
    position.y /= 4.0f;
    position.z /= 4.0f;

    dx = 1;
    if ( mm->vrt_x[vrtstart + 0] > mm->vrt_x[vrtstart + 1] ) dx = -1;

    dy = 1;
    if ( mm->vrt_y[vrtstart + 0] > mm->vrt_y[vrtstart + 3] ) dy = -1;

    z0 = mm->vrt_z[vrtstart + 0];
    z1 = mm->vrt_z[vrtstart + 1];
    z2 = mm->vrt_z[vrtstart + 2];
    z3 = mm->vrt_z[vrtstart + 3];

    // find the derivatives of the height function used to find level
    dzdx = 0.5f * ( z1 - z0 + z2 - z3 );
    dzdy = 0.5f * ( z3 - z0 + z2 - z1 );

    // use these to compute the normal
    normal.x = -dy * dzdx;
    normal.y = -dx * dzdy;
    normal.z =  dx * dy * MESH_FAN_TO_FLOAT( 1 );

    // orient the normal in the proper direction
    if ( normal.z * phys->gravity > 0.0f )
    {
      normal.x *= -1.0f;
      normal.y *= -1.0f;
      normal.z *= -1.0f;
    };
  };


  // make sure that the normal is not trivial
  retval = ( ABS( normal.x ) + ABS( normal.y ) + ABS( normal.z ) ) > 0.0f;

  if ( NULL != pnrm )
  {
    *pnrm = Normalize(normal);
  };

  if ( NULL != ppos )
  {
    *ppos = position;
  };

  return retval;
};


//--------------------------------------------------------------------------------------------
bool_t mesh_calc_normal_pos( Mesh_t * pmesh, PhysicsData_t * phys, Uint32 fan, vect3 pos, vect3 * pnrm )
{
  bool_t retval = bfalse;
  vect3 normal;

  MeshMem_t * mm = &(pmesh->Mem);

  if ( INVALID_TILE == fan )
  {
    normal.x = 0.0f;
    normal.y = 0.0f;
    normal.z = 1.0f;
  }
  else
  {
    float dzdx, dzdy, dx, dy;
    float z0, z1, z2, z3;
    float fx, fy;
    int vrtstart = mm->tilelst[fan].vrt_start;

    dx = 1;
    if ( mm->vrt_x[vrtstart + 0] > mm->vrt_x[vrtstart + 1] ) dx = -1;

    dy = 1;
    if ( mm->vrt_y[vrtstart + 0] > mm->vrt_y[vrtstart + 3] ) dy = -1;

    z0 = mm->vrt_z[vrtstart + 0];
    z1 = mm->vrt_z[vrtstart + 1];
    z2 = mm->vrt_z[vrtstart + 2];
    z3 = mm->vrt_z[vrtstart + 3];

    pos.x = MESH_FLOAT_TO_FAN( pos.x );
    pos.y = MESH_FLOAT_TO_FAN( pos.y );

    fx = pos.x - (( int ) pos.x );
    fy = pos.y - (( int ) pos.y );

    // find the derivatives of the height function used to find level

    dzdx = (( 1.0f - fy ) * ( z1 - z0 ) + fy * ( z2 - z3 ) );
    dzdy = (( 1.0f - fx ) * ( z3 - z0 ) + fx * ( z2 - z1 ) );

    // use these to compute the normal
    normal.x = -dy * dzdx;
    normal.y = -dx * dzdy;
    normal.z =  dx * dy * MESH_FAN_TO_FLOAT( 1 );

    // orient the normal in the proper direction
    if ( normal.z*phys->gravity > 0.0f )
    {
      normal.x *= -1.0f;
      normal.y *= -1.0f;
      normal.z *= -1.0f;
    };

  };


  // make sure that the normal is not trivial
  retval = ABS( normal.x ) + ABS( normal.y ) + ABS( normal.z ) > 0.0f;

  if ( NULL != pnrm )
  {
    *pnrm = Normalize(normal);
  };

  return retval;
};

//--------------------------------------------------------------------------------------------
bool_t mesh_calc_normal( Mesh_t * pmesh, PhysicsData_t * phys, vect3 pos, vect3 * pnrm )
{
  bool_t retval = bfalse;
  Uint32 fan;
  vect3 normal;

  MeshMem_t * mm = &(pmesh->Mem);

  fan = mesh_get_fan( pmesh, pos );
  if ( INVALID_FAN == fan )
  {
    normal.x = 0.0f;
    normal.y = 0.0f;
    normal.z = 1.0f;
  }
  else
  {
    float dzdx, dzdy, dx, dy;
    float z0, z1, z2, z3;
    float fx, fy;
    int vrtstart = mm->tilelst[fan].vrt_start;

    dx = 1;
    if ( mm->vrt_x[vrtstart + 0] > mm->vrt_x[vrtstart + 1] ) dx = -1;

    dy = 1;
    if ( mm->vrt_y[vrtstart + 0] > mm->vrt_y[vrtstart + 3] ) dy = -1;

    z0 = mm->vrt_z[vrtstart + 0];
    z1 = mm->vrt_z[vrtstart + 1];
    z2 = mm->vrt_z[vrtstart + 2];
    z3 = mm->vrt_z[vrtstart + 3];

    pos.x = MESH_FLOAT_TO_FAN( pos.x );
    pos.y = MESH_FLOAT_TO_FAN( pos.y );

    fx = pos.x - (( int ) pos.x );
    fy = pos.y - (( int ) pos.y );

    // find the derivatives of the height function used to find level

    dzdx = (( 1.0f - fy ) * ( z1 - z0 ) + fy * ( z2 - z3 ) );
    dzdy = (( 1.0f - fx ) * ( z3 - z0 ) + fx * ( z2 - z1 ) );

    // use these to compute the normal
    normal.x = -dy * dzdx;
    normal.y = -dx * dzdy;
    normal.z =  dx * dy * MESH_FAN_TO_FLOAT( 1 );

    // orient the normal in the proper direction
    if ( normal.z * phys->gravity > 0.0f )
    {
      normal.x *= -1.0f;
      normal.y *= -1.0f;
      normal.z *= -1.0f;
    };

    normal = Normalize( normal );
  };


  // make sure that the normal is not trivial
  retval = ABS( normal.x ) + ABS( normal.y ) + ABS( normal.z ) > 0.0f;

  if ( NULL != pnrm )
  {
    *pnrm = Normalize(normal);
  };

  return retval;
};

//---------------------------------------------------------------------------------------------
float mesh_get_level( MeshMem_t * mm, Uint32 fan, float x, float y, bool_t waterwalk, WATER_INFO * wi )
{
  /// @details ZZ@> This function returns the height of a point within a mesh fan, precise
  ///     If waterwalk is nonzero and the fan is wi->y, then the level returned is the
  ///     level of the Water.

  float z0, z1, z2, z3;         // Height of each fan corner
  float zleft, zright, zdone;   // Weighted height of each side
  float fx, fy;

  if ( INVALID_FAN == fan ) return 0.0f;

  x = MESH_FLOAT_TO_FAN( x );
  y = MESH_FLOAT_TO_FAN( y );

  fx = x - (( int ) x );
  fy = y - (( int ) y );


  z0 = mm->vrt_z[mm->tilelst[fan].vrt_start + 0];
  z1 = mm->vrt_z[mm->tilelst[fan].vrt_start + 1];
  z2 = mm->vrt_z[mm->tilelst[fan].vrt_start + 2];
  z3 = mm->vrt_z[mm->tilelst[fan].vrt_start + 3];

  zleft = ( z0 * ( 1.0f - fy ) + z3 * fy );
  zright = ( z1 * ( 1.0f - fy ) + z2 * fy );
  zdone = ( zleft * ( 1.0f - fx ) + zright * fx );

  if ( waterwalk )
  {
    if ( NULL!=wi && wi->surfacelevel > zdone && mesh_has_some_bits( mm->tilelst, fan, MPDFX_WATER ) && wi->iswater )
    {
      return wi->surfacelevel;
    }
  }

  return zdone;
}

//--------------------------------------------------------------------------------------------
static bool_t MeshMem_dealloc_verts(MeshMem_t * mem)
{
  if(NULL ==mem) return bfalse;

  EGOBOO_DELETE ( mem->base );

  mem->vrt_count  = 0;

  mem->vrt_x      = NULL;
  mem->vrt_y      = NULL;
  mem->vrt_z      = NULL;

  mem->vrt_ar_fp8 = NULL;
  mem->vrt_ag_fp8 = NULL;
  mem->vrt_ab_fp8 = NULL;

  mem->vrt_lr_fp8 = NULL;
  mem->vrt_lg_fp8 = NULL;
  mem->vrt_lb_fp8 = NULL;

  return btrue;
}

//--------------------------------------------------------------------------------------------
static bool_t MeshMem_alloc_verts(MeshMem_t * mem, int vertcount)
{
  if(NULL == mem || 0 == vertcount) return bfalse;

  if(vertcount<0) vertcount = CData.maxtotalmeshvertices;
  if(mem->vrt_count > vertcount) return btrue;

  MeshMem_dealloc_verts(mem);

  mem->vrt_count = 0;
  mem->base = calloc( vertcount, BYTESFOREACHVERTEX );
  if ( NULL == mem->base ) return bfalse;

  mem->vrt_count  = vertcount;

  mem->vrt_x      = (float *)mem->base;
  mem->vrt_y      = mem->vrt_x + vertcount;
  mem->vrt_z      = mem->vrt_y + vertcount;

  mem->vrt_ar_fp8 = (Uint8*) (mem->vrt_z + vertcount);
  mem->vrt_ag_fp8 = mem->vrt_ar_fp8 + vertcount;
  mem->vrt_ab_fp8 = mem->vrt_ag_fp8 + vertcount;

  mem->vrt_lr_fp8 = (float *)(mem->vrt_ab_fp8 + vertcount);
  mem->vrt_lg_fp8 = mem->vrt_lr_fp8 + vertcount;
  mem->vrt_lb_fp8 = mem->vrt_lg_fp8 + vertcount;

  return btrue;
}

//--------------------------------------------------------------------------------------------
static bool_t MeshMem_dealloc_fans(MeshMem_t * mem)
{
  if(NULL ==mem) return bfalse;

  EGOBOO_DELETE ( mem->tilelst );

  mem->tile_count  = 0;

  return btrue;
}

//--------------------------------------------------------------------------------------------
static bool_t MeshMem_alloc_fans(MeshMem_t * mem, int fancount)
{
  if(NULL == mem || 0 == fancount) return bfalse;

  if(fancount<0) fancount = MAXMESHFAN;
  if(mem->tile_count > fancount) return btrue;

  MeshMem_dealloc_fans(mem);

  mem->tile_count = 0;
  mem->tilelst = EGOBOO_NEW_ARY( MeshTile_t, fancount );
  if ( NULL == mem->tilelst ) return bfalse;

  mem->tile_count  = fancount;

  return btrue;
}

//--------------------------------------------------------------------------------------------
MeshMem_t * MeshMem_new(MeshMem_t * mem, int vertcount, int fancount)
{
  /// @details ZZ@> This function gets a load of memory for the terrain mesh

  if(NULL == mem) return mem;

  MeshMem_delete( mem );

  memset( mem, 0, sizeof(MeshMem_t) );

  EKEY_PNEW( mem, MeshMem_t );

  if(fancount <0) fancount  = MAXMESHFAN;

  if(vertcount >= 0 || fancount >= 0)
  {
    MeshMem_alloc_verts(mem, vertcount);
    MeshMem_alloc_fans (mem, fancount );
  }

  return mem;
}

//--------------------------------------------------------------------------------------------
bool_t MeshMem_delete(MeshMem_t * mem)
{
  if(NULL == mem) return bfalse;
  if( !EKEY_PVALID(mem) ) return btrue;

  EKEY_PINVALIDATE(mem);

  MeshMem_dealloc_verts( mem );
  MeshMem_dealloc_fans ( mem );

  return btrue;
}



//--------------------------------------------------------------------------------------------
void set_fan_colorl( Game_t * gs, int fan_x, int fan_y, int color )
{
  Uint32 cnt, fan, vert, numvert;

  Mesh_t * pmesh = Game_getMesh(gs);

  fan = mesh_convert_fan( &(pmesh->Info), fan_x, fan_y );
  if ( INVALID_FAN == fan ) return;

  vert = pmesh->Mem.tilelst[fan].vrt_start;
  cnt = 0;
  numvert = pmesh->TileDict[pmesh->Mem.tilelst[fan].type].vrt_count;
  while ( cnt < numvert )
  {
    pmesh->Mem.vrt_lr_fp8[vert] = color;
    pmesh->Mem.vrt_lg_fp8[vert] = color;
    pmesh->Mem.vrt_lb_fp8[vert] = color;
    vert++;
    cnt++;
  }
}

//--------------------------------------------------------------------------------------------
Uint32 mesh_hitawall( Mesh_t * pmesh, vect3 pos, float tiles_x, float tiles_y, Uint32 collision_bits, vect3 * nrm )
{
  /// @details ZZ@> This function returns nonzero if \<pos.x, pos.y\> is in an invalid tile

  vect3 loc_pos;
  int fan_x, fan_x_min, fan_x_max, fan_y, fan_y_min, fan_y_max;
  Uint32 fan, pass = 0;

  if(NULL != nrm)
  {
    VectorClear(nrm->v);
  }

  fan_x_min = ( pos.x - tiles_x < 0 ) ? 0 : MESH_FLOAT_TO_FAN( pos.x - tiles_x );
  fan_x_max = ( pos.x + tiles_x < 0 ) ? 0 : MESH_FLOAT_TO_FAN( pos.x + tiles_x );

  fan_y_min = ( pos.y - tiles_y < 0 ) ? 0 : MESH_FLOAT_TO_FAN( pos.y - tiles_y );
  fan_y_max = ( pos.y + tiles_y < 0 ) ? 0 : MESH_FLOAT_TO_FAN( pos.y + tiles_y );

  for ( fan_x = fan_x_min; fan_x <= fan_x_max; fan_x++ )
  {
    for ( fan_y = fan_y_min; fan_y <= fan_y_max; fan_y++ )
    {
      Uint32 bits;
      float  lerp;

      loc_pos.x = MESH_FAN_TO_INT( fan_x ) + ( 1 << 6 );
      loc_pos.y = MESH_FAN_TO_INT( fan_y ) + ( 1 << 6 );
      loc_pos.z = pos.z;

      fan = mesh_get_fan( pmesh, loc_pos );
      if ( INVALID_FAN != fan )
      {
        bits = pmesh->Mem.tilelst[ fan ].fx;
        pass |= bits;

        if( HAS_SOME_BITS(bits, collision_bits) && NULL != nrm)
        {
          //level = mesh_get_level(fan, loc_pos.x, loc_pos.y, bfalse);

          //lerp = (level + PLATTOLERANCE - loc_pos.z) / PLATTOLERANCE;
          //lerp = CLIP(lerp, 0, 1);

          //if(lerp>0)
          {
            lerp = 1.0f;
            nrm->x += lerp * twist_table[ pmesh->Mem.tilelst[ fan ].twist ].nrm.x;
            nrm->y += lerp * twist_table[ pmesh->Mem.tilelst[ fan ].twist ].nrm.y;
            nrm->z += lerp * twist_table[ pmesh->Mem.tilelst[ fan ].twist ].nrm.z;
          }
        }
      }
    };
  };

  if( HAS_SOME_BITS(pass, collision_bits) && NULL != nrm)
  {
    *nrm = Normalize(*nrm);
  }

  return HAS_SOME_BITS(pass, collision_bits);
}

//--------------------------------------------------------------------------------------------
MeshInfo_t * MeshInfo_new(MeshInfo_t * mi)
{
  if(NULL == mi) return mi;

  MeshInfo_delete( mi );

  memset( mi, 0, sizeof(MeshInfo_t) );

  EKEY_PNEW( mi, MeshInfo_t );

  bumplist_new( &(mi->bumplist) );

  return mi;
};

//--------------------------------------------------------------------------------------------
bool_t MeshInfo_delete(MeshInfo_t * mi)
{
  if(NULL == mi) return bfalse;

  if( !EKEY_PVALID(mi) ) return btrue;

  bumplist_delete( &(mi->bumplist) );

  EKEY_PINVALIDATE( mi );

  memset(mi, 0, sizeof(MeshInfo_t));

  return btrue;
}
