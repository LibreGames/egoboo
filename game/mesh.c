/* Egoboo - Mesh.c
 * This part handles mesh related stuff.
 *
 */

/*
    This file is part of Egoboo.

    Egoboo is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Egoboo is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "mesh.inl"

#include "Log.h"

#include "egoboo_utility.h"
#include "egoboo.h"

#include "char.inl"
#include "particle.inl"
#include "egoboo_types.inl"


MESH_INFO   mesh;
MESH_FAN    Mesh_Fan[MAXMESHFAN];
MESH_TILE   Mesh_Tile[MAXTILETYPE];
MESH_MEMORY Mesh_Mem;

BUMPLIST bumplist = {bfalse, 0, 0 };

Uint32  Mesh_Block_X[( MAXMESHSIZEY/4 ) +1];
Uint32  Mesh_Fan_X[MAXMESHSIZEY];                         // Which fan to start a row with

//--------------------------------------------------------------------------------------------
bool_t reset_bumplist()
{
  int i;

  if(!bumplist.allocated) return bfalse;

  bumplist_clear(&bumplist);

  for(i=0; i<bumplist.free_max; i++)
  {
    bumplist.free_lst[i] = i;
    bumplist_node_new( bumplist.node_lst + i );
  }
  bumplist.free_count = bumplist.free_max;

  bumplist.initialized = btrue;
  bumplist.filled      = bfalse;
}

//--------------------------------------------------------------------------------------------
bool_t allocate_bumplist(int blocks)
{
  bool_t retval;
  int i;

  if( bumplist_allocate(&bumplist, blocks) )
  {
    // set up nodes and the list of free nodes
    bumplist.free_max   =
    bumplist.free_count = 8*(MAXCHR + MAXPRT);
    bumplist.free_lst = calloc( bumplist.free_count, sizeof(Uint32));
    bumplist.node_lst = calloc( bumplist.free_count, sizeof(BUMPLIST_NODE));

    reset_bumplist();
  }

  return btrue;
};

//--------------------------------------------------------------------------------------------
bool_t load_mesh( char *modname )
{
  // ZZ> This function loads the "LEVEL.MPD" file

  FILE* fileread;
  STRING newloadname;
  int itmp, cnt;
  float ftmp;
  int fan, fancount;
  int numvert, numfan;
  int vert, vrt;

  snprintf( newloadname, sizeof( newloadname ), "%s%s" SLASH_STRING "%s", modname, CData.gamedat_dir, CData.mesh_file );
  fileread = fs_fileOpen( PRI_NONE, NULL, newloadname, "rb" );
  if ( NULL == fileread ) return bfalse;

  fread( &itmp, 4, 1, fileread );  if (SDL_SwapLE32( itmp ) != MAPID ) return bfalse;
  fread( &itmp, 4, 1, fileread );  numvert     = SDL_SwapLE32( itmp );
  fread( &itmp, 4, 1, fileread );  mesh.size_x = SDL_SwapLE32( itmp );
  fread( &itmp, 4, 1, fileread );  mesh.size_y = SDL_SwapLE32( itmp );

  numfan = mesh.size_x * mesh.size_y;
  mesh.edge_x = mesh.size_x * 128;
  mesh.edge_y = mesh.size_y * 128;

  // MESHSIZEX MUST BE MULTIPLE OF 4
  if( mesh.size_x & 3 )
  {
    log_error( "Mesh size must be a multiple of 4 in the x direction" );
  }

  // allocate the bumplist
  allocate_bumplist( ( mesh.size_x >> 2 ) * ( mesh.size_y >> 2 ) );  

  // calculate the proper GWater.shift
  itmp = mesh.size_x >> 2;
  for(GWater.shift = 0; itmp != 0; GWater.shift++)
  {
    itmp >>= 1;
  };

  // Load fan data
  for ( fan = 0; fan < numfan;  fan++)
  {
    fread( &itmp, 4, 1, fileread );
    itmp = SDL_SwapLE32( itmp );

    Mesh_Fan[fan].type = itmp >> 24;
    Mesh_Fan[fan].fx   = itmp >> 16;
    Mesh_Fan[fan].tile = itmp;
  }

  // Load fan data
  for ( fan = 0; fan < numfan; fan++ )
  {
    fread( &itmp, 1, 1, fileread );
    Mesh_Fan[fan].twist = itmp;
  }

  // Load vertex fan_x data
  for( cnt = 0;  cnt < numvert; cnt++ )
  {
    fread( &ftmp, 4, 1, fileread );
    Mesh_Mem.vrt_x[cnt] = SwapLE_float( ftmp );
  }

  // Load vertex fan_y data
  for( cnt = 0; cnt < numvert; cnt++ )
  {
    fread( &ftmp, 4, 1, fileread );
    Mesh_Mem.vrt_y[cnt] = SwapLE_float( ftmp );
  }

  // Load vertex z data
  cnt = 0;
  for( cnt = 0; cnt < numvert; cnt++ )
  {
    fread( &ftmp, 4, 1, fileread );
    Mesh_Mem.vrt_z[cnt] = SwapLE_float( ftmp )  / 16.0;  // Cartman uses 4 bit fixed point for Z
  }

  // Load vertex lighting data
  
  for ( cnt = 0; cnt < numvert; cnt++ )
  {
    fread( &itmp, 1, 1, fileread );

    Mesh_Mem.vrt_ar_fp8[cnt] = 0; // itmp;
    Mesh_Mem.vrt_ag_fp8[cnt] = 0; // itmp;
    Mesh_Mem.vrt_ab_fp8[cnt] = 0; // itmp;

    Mesh_Mem.vrt_lr_fp8[cnt] = 0;
    Mesh_Mem.vrt_lg_fp8[cnt] = 0;
    Mesh_Mem.vrt_lb_fp8[cnt] = 0;
  }

  fs_fileClose( fileread );

  make_fanstart();

  vert = 0;
  fancount = mesh.size_y * mesh.size_x;
  for ( fan = 0; fan < fancount; fan++ )
  {
    int vrtcount = Mesh_Cmd[Mesh_Fan[fan].type].vrt_count;
    int vrtstart = vert;

    Mesh_Fan[fan].vrt_start = vrtstart;

    Mesh_Fan[fan].bbox.mins.x = Mesh_Fan[fan].bbox.maxs.x = Mesh_Mem.vrt_x[vrtstart];
    Mesh_Fan[fan].bbox.mins.y = Mesh_Fan[fan].bbox.maxs.y = Mesh_Mem.vrt_y[vrtstart];
    Mesh_Fan[fan].bbox.mins.z = Mesh_Fan[fan].bbox.maxs.z = Mesh_Mem.vrt_z[vrtstart];

    for ( vrt = vrtstart + 1; vrt < vrtstart + vrtcount; vrt++ )
    {
      Mesh_Fan[fan].bbox.mins.x = MIN( Mesh_Fan[fan].bbox.mins.x, Mesh_Mem.vrt_x[vrt] );
      Mesh_Fan[fan].bbox.mins.y = MIN( Mesh_Fan[fan].bbox.mins.y, Mesh_Mem.vrt_y[vrt] );
      Mesh_Fan[fan].bbox.mins.z = MIN( Mesh_Fan[fan].bbox.mins.z, Mesh_Mem.vrt_z[vrt] );

      Mesh_Fan[fan].bbox.maxs.x = MAX( Mesh_Fan[fan].bbox.maxs.x, Mesh_Mem.vrt_x[vrt] );
      Mesh_Fan[fan].bbox.maxs.y = MAX( Mesh_Fan[fan].bbox.maxs.y, Mesh_Mem.vrt_y[vrt] );
      Mesh_Fan[fan].bbox.maxs.z = MAX( Mesh_Fan[fan].bbox.maxs.z, Mesh_Mem.vrt_z[vrt] );
    }

    vert += vrtcount;
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
#define TX_FUDGE 0.5f

bool_t load_mesh_fans()
{
  // ZZ> This function loads fan types for the terrain

  int cnt, entry;
  int numfantype, fantype, bigfantype, vertices;
  int numcommand, command, commandsize;
  FILE* fileread;

  // Initialize all mesh types to 0
  entry = 0;
  while ( entry < MAXMESHTYPE )
  {
    Mesh_Cmd[entry].vrt_count = 0;
    Mesh_Cmd[entry].cmd_count = 0;
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
    Mesh_Cmd[   fantype].vrt_count = 
    Mesh_Cmd[bigfantype].vrt_count = fget_next_int( fileread );  // Dupe
    
    for ( cnt = 0; cnt < vertices; cnt++ )
    {
      Mesh_Cmd[   fantype].ref[cnt] =
      Mesh_Cmd[bigfantype].ref[cnt] = fget_next_int( fileread );

      Mesh_Cmd[   fantype].tx[cnt].u =
      Mesh_Cmd[bigfantype].tx[cnt].u = fget_next_float( fileread );

      Mesh_Cmd[   fantype].tx[cnt].v =
      Mesh_Cmd[bigfantype].tx[cnt].v = fget_next_float( fileread );
    }

    numcommand                     = 
    Mesh_Cmd[   fantype].cmd_count =
    Mesh_Cmd[bigfantype].cmd_count = fget_next_int( fileread );  // Dupe

    for ( entry = 0, command = 0; command < numcommand; command++ )
    {
      commandsize                            =
      Mesh_Cmd[   fantype].cmd_size[command] =
      Mesh_Cmd[bigfantype].cmd_size[command] = fget_next_int( fileread );  // Dupe

      for ( cnt = 0; cnt < commandsize; cnt++ )
      {
        Mesh_Cmd[   fantype].vrt[entry] = 
        Mesh_Cmd[bigfantype].vrt[entry] = fget_next_int( fileread );  // Dupe
        entry++;
      }
    }

  }
  fs_fileClose( fileread );



  // Correct silly Cartman 32-pixel-wide textures to Egoboo's 256 pixel wide textures
  
  for ( cnt = 0; cnt < MAXMESHTYPE / 2; cnt++ )
  {    
    for ( entry = 0; entry < Mesh_Cmd[cnt].vrt_count; entry++ )
    {
      Mesh_Cmd[cnt].tx[entry].u = ( TX_FUDGE + Mesh_Cmd[cnt].tx[entry].u * ( 31.0f - TX_FUDGE ) ) / 256.0f;
      Mesh_Cmd[cnt].tx[entry].v = ( TX_FUDGE + Mesh_Cmd[cnt].tx[entry].v * ( 31.0f - TX_FUDGE ) ) / 256.0f;
    }

    // blank the unused values
    for ( /* nothing */; entry < MAXMESHVERTICES; entry++ )
    {
      Mesh_Cmd[cnt].tx[entry].u = -1.0f;
      Mesh_Cmd[cnt].tx[entry].v = -1.0f;
    }
  }

  // Correct silly Cartman 64-pixel-wide textures to Egoboo's 256 pixel wide textures
  for ( cnt = MAXMESHTYPE / 2; cnt < MAXMESHTYPE; cnt++ )
  {
    for ( entry = 0; entry < Mesh_Cmd[cnt].vrt_count; entry++ )
    {
      Mesh_Cmd[cnt].tx[entry].u = ( TX_FUDGE  + Mesh_Cmd[cnt].tx[entry].u * ( 63.0f - TX_FUDGE ) ) / 256.0f;
      Mesh_Cmd[cnt].tx[entry].v = ( TX_FUDGE  + Mesh_Cmd[cnt].tx[entry].v * ( 63.0f - TX_FUDGE ) ) / 256.0f;
    }

    // blank the unused values
    for ( /* nothing */; entry < MAXMESHVERTICES; entry++ )
    {
      Mesh_Cmd[cnt].tx[entry].u = -1.0f;
      Mesh_Cmd[cnt].tx[entry].v = -1.0f;
    }
  }

  // Make tile texture offsets
  // 64 tiles per texture, 4 textures
  for ( cnt = 0; cnt < MAXTILETYPE; cnt++ )
  {
    Mesh_Tile[cnt].off.u = ( ( cnt >> 0 ) & 7 ) / 8.0f;
    Mesh_Tile[cnt].off.v = ( ( cnt >> 3 ) & 7 ) / 8.0f;
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
void make_fanstart()
{
  // ZZ> This function builds a look up table to ease calculating the
  //     fan number given an x,y pair

  int cnt;

  for ( cnt = 0; cnt < mesh.size_y; cnt++ )
  {
    Mesh_Fan_X[cnt] = mesh.size_x * cnt;
  }

  for ( cnt = 0; cnt < ( mesh.size_y >> 2 ); cnt++ )
  {
    Mesh_Block_X[cnt] = ( mesh.size_x >> 2 ) * cnt;
  }
}

//--------------------------------------------------------------------------------------------
void make_twist()
{
  // ZZ> This function precomputes surface normals and steep hill acceleration for
  //     the mesh

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
bool_t mesh_calc_normal_fan( int fan, vect3 * pnrm, vect3 * ppos )
{
  bool_t retval = bfalse;
  Uint32 cnt;
  vect3 normal, position;

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
    int vrtstart = Mesh_Fan[fan].vrt_start;

    VectorClear( position.v );
    for ( cnt = 0; cnt < 4; cnt++ )
    {
      position.x += Mesh_Mem.vrt_x[vrtstart + cnt];
      position.y += Mesh_Mem.vrt_y[vrtstart + cnt];
      position.z += Mesh_Mem.vrt_z[vrtstart + cnt];
    };
    position.x /= 4.0f;
    position.y /= 4.0f;
    position.z /= 4.0f;

    dx = 1;
    if ( Mesh_Mem.vrt_x[vrtstart + 0] > Mesh_Mem.vrt_x[vrtstart + 1] ) dx = -1;

    dy = 1;
    if ( Mesh_Mem.vrt_y[vrtstart + 0] > Mesh_Mem.vrt_y[vrtstart + 3] ) dy = -1;

    z0 = Mesh_Mem.vrt_z[vrtstart + 0];
    z1 = Mesh_Mem.vrt_z[vrtstart + 1];
    z2 = Mesh_Mem.vrt_z[vrtstart + 2];
    z3 = Mesh_Mem.vrt_z[vrtstart + 3];

    // find the derivatives of the height function used to find level
    dzdx = 0.5f * ( z1 - z0 + z2 - z3 );
    dzdy = 0.5f * ( z3 - z0 + z2 - z1 );

    // use these to compute the normal
    normal.x = -dy * dzdx;
    normal.y = -dx * dzdy;
    normal.z =  dx * dy * MESH_FAN_TO_FLOAT( 1 );

    // orient the normal in the proper direction
    if ( normal.z*gravity > 0.0f )
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
bool_t mesh_calc_normal_pos( int fan, vect3 pos, vect3 * pnrm )
{
  bool_t retval = bfalse;
  vect3 normal;

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
    int vrtstart = Mesh_Fan[fan].vrt_start;

    dx = 1;
    if ( Mesh_Mem.vrt_x[vrtstart + 0] > Mesh_Mem.vrt_x[vrtstart + 1] ) dx = -1;

    dy = 1;
    if ( Mesh_Mem.vrt_y[vrtstart + 0] > Mesh_Mem.vrt_y[vrtstart + 3] ) dy = -1;

    z0 = Mesh_Mem.vrt_z[vrtstart + 0];
    z1 = Mesh_Mem.vrt_z[vrtstart + 1];
    z2 = Mesh_Mem.vrt_z[vrtstart + 2];
    z3 = Mesh_Mem.vrt_z[vrtstart + 3];

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
    if ( normal.z*gravity > 0.0f )
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
bool_t mesh_calc_normal( vect3 pos, vect3 * pnrm )
{
  bool_t retval = bfalse;
  Uint32 fan;
  vect3 normal;

  fan = mesh_get_fan( pos );
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
    int vrtstart = Mesh_Fan[fan].vrt_start;

    dx = 1;
    if ( Mesh_Mem.vrt_x[vrtstart + 0] > Mesh_Mem.vrt_x[vrtstart + 1] ) dx = -1;

    dy = 1;
    if ( Mesh_Mem.vrt_y[vrtstart + 0] > Mesh_Mem.vrt_y[vrtstart + 3] ) dy = -1;

    z0 = Mesh_Mem.vrt_z[vrtstart + 0];
    z1 = Mesh_Mem.vrt_z[vrtstart + 1];
    z2 = Mesh_Mem.vrt_z[vrtstart + 2];
    z3 = Mesh_Mem.vrt_z[vrtstart + 3];

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
    if ( normal.z*gravity > 0.0f )
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
float mesh_get_level( Uint32 fan, float x, float y, bool_t waterwalk )
{
  // ZZ> This function returns the height of a point within a mesh fan, precise
  //     If waterwalk is nonzero and the fan is GWater.y, then the level returned is the
  //     level of the water.

  float z0, z1, z2, z3;         // Height of each fan corner
  float zleft, zright, zdone;   // Weighted height of each side
  float fx, fy;

  if ( INVALID_FAN == fan ) return 0.0f;

  x = MESH_FLOAT_TO_FAN( x );
  y = MESH_FLOAT_TO_FAN( y );

  fx = x - (( int ) x );
  fy = y - (( int ) y );


  z0 = Mesh_Mem.vrt_z[Mesh_Fan[fan].vrt_start + 0];
  z1 = Mesh_Mem.vrt_z[Mesh_Fan[fan].vrt_start + 1];
  z2 = Mesh_Mem.vrt_z[Mesh_Fan[fan].vrt_start + 2];
  z3 = Mesh_Mem.vrt_z[Mesh_Fan[fan].vrt_start + 3];

  zleft = ( z0 * ( 1.0f - fy ) + z3 * fy );
  zright = ( z1 * ( 1.0f - fy ) + z2 * fy );
  zdone = ( zleft * ( 1.0f - fx ) + zright * fx );

  if ( waterwalk )
  {
    if ( GWater.surfacelevel > zdone && mesh_has_some_bits( fan, MPDFX_WATER ) && GWater.iswater )
    {
      return GWater.surfacelevel;
    }
  }

  return zdone;
}

//--------------------------------------------------------------------------------------------
bool_t get_mesh_memory()
{
  // ZZ> This function gets a load of memory for the terrain mesh

  Mesh_Mem.base = ( float * ) malloc( CData.maxtotalmeshvertices * BYTESFOREACHVERTEX );
  if ( Mesh_Mem.base == NULL )
    return bfalse;

  Mesh_Mem.vrt_x = &Mesh_Mem.base[0];
  Mesh_Mem.vrt_y = &Mesh_Mem.vrt_x[CData.maxtotalmeshvertices];
  Mesh_Mem.vrt_z = &Mesh_Mem.vrt_y[CData.maxtotalmeshvertices];
  Mesh_Mem.vrt_ar_fp8 = ( Uint8 * ) & Mesh_Mem.vrt_z[CData.maxtotalmeshvertices];
  Mesh_Mem.vrt_ag_fp8 = &Mesh_Mem.vrt_ar_fp8[CData.maxtotalmeshvertices];
  Mesh_Mem.vrt_ab_fp8 = &Mesh_Mem.vrt_ag_fp8[CData.maxtotalmeshvertices];
  Mesh_Mem.vrt_lr_fp8 = &Mesh_Mem.vrt_ab_fp8[CData.maxtotalmeshvertices];
  Mesh_Mem.vrt_lg_fp8 = &Mesh_Mem.vrt_lr_fp8[CData.maxtotalmeshvertices];
  Mesh_Mem.vrt_lb_fp8 = &Mesh_Mem.vrt_lg_fp8[CData.maxtotalmeshvertices];
  return btrue;
}

//--------------------------------------------------------------------------------------------
void free_mesh_memory()
{
  FREE ( Mesh_Mem.base );
  memset(&Mesh_Mem, 0, sizeof(MESH_MEMORY));
}



//--------------------------------------------------------------------------------------------
void set_fan_colorl( int fan_x, int fan_y, int color )
{
  Uint32 cnt, fan, vert, numvert;

  fan = mesh_convert_fan( fan_x, fan_y );
  if ( INVALID_FAN == fan ) return;

  vert = Mesh_Fan[fan].vrt_start;
  cnt = 0;
  numvert = Mesh_Cmd[Mesh_Fan[fan].type].vrt_count;
  while ( cnt < numvert )
  {
    Mesh_Mem.vrt_lr_fp8[vert] = color;
    Mesh_Mem.vrt_lg_fp8[vert] = color;
    Mesh_Mem.vrt_lb_fp8[vert] = color;
    vert++;
    cnt++;
  }
}

//--------------------------------------------------------------------------------------------
Uint32 mesh_hitawall( vect3 pos, float size_x, float size_y, Uint32 collision_bits, vect3 * nrm )
{
  // ZZ> This function returns nonzero if <pos.x, pos.y> is in an invalid tile

  vect3 loc_pos;
  int fan_x, fan_x_min, fan_x_max, fan_y, fan_y_min, fan_y_max;
  Uint32 fan, pass = 0;

  if(NULL != nrm)
  {
    VectorClear(nrm->v);
  }

  fan_x_min = ( pos.x - size_x < 0 ) ? 0 : MESH_FLOAT_TO_FAN( pos.x - size_x );
  fan_x_max = ( pos.x + size_x < 0 ) ? 0 : MESH_FLOAT_TO_FAN( pos.x + size_x );

  fan_y_min = ( pos.y - size_y < 0 ) ? 0 : MESH_FLOAT_TO_FAN( pos.y - size_y );
  fan_y_max = ( pos.y + size_y < 0 ) ? 0 : MESH_FLOAT_TO_FAN( pos.y + size_y );

  for ( fan_x = fan_x_min; fan_x <= fan_x_max; fan_x++ )
  {
    for ( fan_y = fan_y_min; fan_y <= fan_y_max; fan_y++ )
    {
      Uint32 bits;
      float  level, lerp;

      loc_pos.x = MESH_FAN_TO_INT( fan_x ) + ( 1 << 6 );
      loc_pos.y = MESH_FAN_TO_INT( fan_y ) + ( 1 << 6 );
      loc_pos.z = pos.z;

      fan = mesh_get_fan( loc_pos );
      if ( INVALID_FAN != fan ) 
      {
        bits = Mesh_Fan[ fan ].fx;
        pass |= bits;

        if(0 != (bits & collision_bits) && NULL != nrm)
        {
          //level = mesh_get_level(fan, loc_pos.x, loc_pos.y, bfalse);

          //lerp = (level + PLATTOLERANCE - loc_pos.z) / PLATTOLERANCE;
          //lerp = CLIP(lerp, 0, 1);

          //if(lerp>0)
          {
            lerp = 1.0f;
            nrm->x += lerp * twist_table[ Mesh_Fan[ fan ].twist ].nrm.x;
            nrm->y += lerp * twist_table[ Mesh_Fan[ fan ].twist ].nrm.y;
            nrm->z += lerp * twist_table[ Mesh_Fan[ fan ].twist ].nrm.z;
          }
        }
      }
    };
  };

  if(0 != (pass & collision_bits) && NULL != nrm)
  {
    *nrm = Normalize(*nrm);
  }

  return pass & collision_bits;
}

