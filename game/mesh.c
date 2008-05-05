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

#include "mesh.h"
#include "Log.h"
#include "char.h"
#include "particle.h"

#include "cartman.h"
#include "egoboo_utility.h"
#include "egoboo.h"

MESH_INFO   mesh;
MESH_FAN    Mesh_Fan[MAXMESHFAN];
MESH_TILE   Mesh_Tile[MAXTILETYPE];
MESH_MEMORY Mesh_Mem;

BUMPLIST bumplist = {bfalse, 0};

Uint32  Mesh_Block_X[( MAXMESHSIZEY/4 ) +1];
Uint32  Mesh_Fan_X[MAXMESHSIZEY];                         // Which fan to start a row with

//--------------------------------------------------------------------------------------------
bool_t allocate_bumplist(int blocks)
{
  bumplist.chr_list = calloc(MAXCHR, sizeof(BUMPLIST_NODE));
  bumplist.prt_list = calloc(MAXPRT, sizeof(BUMPLIST_NODE));

  return bumplist_allocate( &bumplist, blocks );
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


//--------------------------------------------------------------------------------------------
Uint32 mesh_get_fan( vect3 pos )
{
  // BB > find the tile under <pos.x,pos.y>, but MAKE SURE we have the right tile.

  Uint32 ivert, testfan = INVALID_FAN;
  float minx, maxx, miny, maxy;
  int i, ix, iy;
  bool_t bfound = bfalse;

  if ( !mesh_check( pos.x, pos.y ) )
    return testfan;

  ix = MESH_FLOAT_TO_FAN( pos.x );
  iy = MESH_FLOAT_TO_FAN( pos.y );

  testfan = ix + Mesh_Fan_X[iy];

  ivert = Mesh_Fan[testfan].vrt_start;
  minx = maxx = Mesh_Mem.vrt_x[ivert];
  miny = maxy = Mesh_Mem.vrt_y[ivert];
  for ( i = 1;i < 4;i++, ivert++ )
  {
    minx = MIN( minx, Mesh_Mem.vrt_x[ivert] );
    maxx = MAX( maxx, Mesh_Mem.vrt_x[ivert] );

    miny = MIN( miny, Mesh_Mem.vrt_y[ivert] );
    maxy = MAX( maxy, Mesh_Mem.vrt_y[ivert] );
  };

  if ( pos.x < minx ) { ix--; bfound = btrue; }
  else if ( pos.x > maxx ) { ix++; bfound = btrue; }
  if ( pos.y < miny ) { iy--; bfound = btrue; }
  else if ( pos.y > maxy ) { iy++; bfound = btrue; }

  if ( ix < 0 || iy < 0 || ix > mesh.size_x || iy > mesh.size_y )
    testfan = INVALID_FAN;
  else if ( bfound )
    testfan = ix + Mesh_Fan_X[iy];

  return testfan;
};

//--------------------------------------------------------------------------------------------
Uint32 mesh_get_block( vect3 pos )
{
  // BB > find the block under <x,y>

  return mesh_convert_block( MESH_FLOAT_TO_BLOCK( pos.x ), MESH_FLOAT_TO_BLOCK( pos.y ) );
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
void mesh_set_colora( int fan_x, int fan_y, int color )
{
  Uint32 cnt, fan, vert, numvert;

  fan = mesh_convert_fan( fan_x, fan_y );
  if ( INVALID_FAN == fan ) return;

  vert = Mesh_Fan[fan].vrt_start;
  cnt = 0;
  numvert = Mesh_Cmd[Mesh_Fan[fan].type].vrt_count;
  while ( cnt < numvert )
  {
    Mesh_Mem.vrt_ar_fp8[vert] = color;
    Mesh_Mem.vrt_ag_fp8[vert] = color;
    Mesh_Mem.vrt_ab_fp8[vert] = color;
    vert++;
    cnt++;
  }
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
bool_t mesh_clear_fan_bits( int fan_x, int fan_y, Uint32 bits )
{
  bool_t retval = bfalse;
  Uint32 fan;

  fan = mesh_convert_fan( fan_x, fan_y );
  if ( INVALID_FAN == fan ) return retval;

  retval = mesh_has_some_bits( fan, bits );

  Mesh_Fan[fan].fx &= ~bits;

  return retval;
};

//--------------------------------------------------------------------------------------------
bool_t mesh_add_fan_bits( int fan_x, int fan_y, Uint32 bits )
{
  bool_t retval = bfalse;
  Uint32 fan;

  fan = mesh_convert_fan( fan_x, fan_y );
  if ( INVALID_FAN == fan ) return retval;

  retval = MISSING_BITS( Mesh_Fan[fan].fx, bits );

  Mesh_Fan[fan].fx |= bits;

  return retval;
};

//--------------------------------------------------------------------------------------------
bool_t mesh_set_fan_bits( int fan_x, int fan_y, Uint32 bits )
{
  bool_t retval = bfalse;
  Uint32 fan;

  fan = mesh_convert_fan( fan_x, fan_y );
  if ( INVALID_FAN == fan ) return retval;

  retval = ( Mesh_Fan[fan].fx != bits );

  Mesh_Fan[fan].fx = bits;

  return retval;
};

//--------------------------------------------------------------------------------------------
int mesh_bump_tile( int fan_x, int fan_y )
{
  Uint32 fan;

  fan = mesh_convert_fan( fan_x, fan_y );
  if ( INVALID_FAN == fan ) return 0;

  Mesh_Fan[fan].tile++;

  return Mesh_Fan[fan].tile;
}

//--------------------------------------------------------------------------------------------
Uint32 mesh_get_tile( int fan_x, int fan_y )
{
  Uint32 fan;

  fan = mesh_convert_fan( fan_x, fan_y );
  if ( INVALID_FAN == fan ) return INVALID_TILE;

  return Mesh_Fan[fan].tile;
}

//--------------------------------------------------------------------------------------------
bool_t mesh_set_tile( int fan_x, int fan_y, Uint32 become )
{
  bool_t retval = bfalse;
  Uint32 fan;

  fan = mesh_convert_fan( fan_x, fan_y );
  if ( INVALID_FAN == fan ) return retval;

  if ( become != 0 )
  {
    Mesh_Fan[fan].tile = become;
    retval = btrue;
  }

  return retval;
}

//--------------------------------------------------------------------------------------------
Uint32 mesh_convert_fan( int fan_x, int fan_y )
{
  // BB > convert <fan_x,fan_y> to a fanblock

  if ( fan_x < 0 || fan_x > mesh.size_x || fan_y < 0 || fan_y > mesh.size_y ) return INVALID_FAN;

  return fan_x + Mesh_Fan_X[fan_y];
};

//--------------------------------------------------------------------------------------------
Uint32 mesh_convert_block( int block_x, int block_y )
{
  // BB > convert <block_x,block_y> to a fanblock

  if ( block_x < 0 || block_x > ( mesh.size_x >> 2 ) || block_y < 0 || block_y > ( mesh.size_y >> 2 ) ) return INVALID_FAN;

  return block_x + Mesh_Block_X[block_y];
};

//--------------------------------------------------------------------------------------------
float mesh_clip_x( float x )
{
  if ( x <      0.0f )  x = 0.0f;
  if ( x > mesh.edge_x )  x = mesh.edge_x;

  return x;
}

//--------------------------------------------------------------------------------------------
float mesh_clip_y( float y )
{
  if ( y <      0.0f )  y = 0.0f;
  if ( y > mesh.edge_y )  y = mesh.edge_y;

  return y;
}

//--------------------------------------------------------------------------------------------
int mesh_clip_fan_x( int ix )
{
  if ( ix < 0 )  ix = 0;
  if ( ix > mesh.size_x - 1 )  ix = mesh.size_x - 1;

  return ix;
};

//--------------------------------------------------------------------------------------------
int mesh_clip_fan_y( int iy )
{
  if ( iy < 0 )  iy = 0;
  if ( iy > mesh.size_y - 1 )  iy = mesh.size_y - 1;

  return iy;
};

//--------------------------------------------------------------------------------------------
int mesh_clip_block_x( int ix )
{
  if ( ix < 0 )  ix = 0;
  if ( ix > ( mesh.size_x >> 2 ) - 1 )  ix = ( mesh.size_x >> 2 ) - 1;

  return ix;
};

//--------------------------------------------------------------------------------------------
int mesh_clip_block_y( int iy )
{
  if ( iy < 0 )  iy = 0;
  if ( iy > ( mesh.size_y >> 2 ) - 1 )  iy = ( mesh.size_y >> 2 ) - 1;

  return iy;
};

//--------------------------------------------------------------------------------------------
bool_t mesh_check( float x, float y )
{
  if ( x < 0 || x > mesh.edge_x ) return bfalse;
  if ( y < 0 || y > mesh.edge_x ) return bfalse;

  return btrue;
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

//--------------------------------------------------------------------------------------------
Uint32 mesh_get_bits( int fan, Uint32 bits )
{
  if ( INVALID_FAN == fan ) return 0;

  return Mesh_Fan[fan].fx & bits;
};

//--------------------------------------------------------------------------------------------
bool_t mesh_has_some_bits( int fan, Uint32 bits )
{
  if ( INVALID_FAN == fan ) return 0;

  return HAS_SOME_BITS( Mesh_Fan[fan].fx, bits );
};

//--------------------------------------------------------------------------------------------
bool_t mesh_has_no_bits( int fan, Uint32 bits )
{
  if ( INVALID_FAN == fan ) return 0;

  return HAS_NO_BITS( Mesh_Fan[fan].fx, bits );
};

//--------------------------------------------------------------------------------------------
bool_t mesh_has_all_bits( int fan, Uint32 bits )
{
  if ( INVALID_FAN == fan ) return 0;

  return HAS_ALL_BITS( Mesh_Fan[fan].fx, bits );
};

//--------------------------------------------------------------------------------------------
float mesh_fraction_x( float x )
{
  return x / mesh.edge_x;
};

//--------------------------------------------------------------------------------------------
float mesh_fraction_y( float y )
{
  return y / mesh.edge_y;
};

//--------------------------------------------------------------------------------------------
bool_t mesh_in_renderlist( int fan )
{
  if ( INVALID_FAN == fan ) return bfalse;

  return Mesh_Fan[fan].inrenderlist;
};

//--------------------------------------------------------------------------------------------
void mesh_remove_renderlist( int fan )
{
  Mesh_Fan[fan].inrenderlist = bfalse;
};

//--------------------------------------------------------------------------------------------
void mesh_add_renderlist( int fan )
{
  Mesh_Fan[fan].inrenderlist = btrue;
};

//--------------------------------------------------------------------------------------------
Uint8 mesh_get_twist( int fan )
{
  Uint8 retval = 0x77;

  if ( INVALID_FAN != fan )
  {
    retval = Mesh_Fan[fan].twist;
  }

  return retval;
};

//--------------------------------------------------------------------------------------------
BUMPLIST * bumplist_new(BUMPLIST * b)
{
  if(NULL == b) return NULL;

  memset(b, 0, sizeof(BUMPLIST));

  return b;
};

//--------------------------------------------------------------------------------------------
void bumplist_delete(BUMPLIST * b)
{
  if(NULL == b) return;

  b->valid = bfalse;

  if(0 == b->num_blocks) return;

  b->num_blocks = 0;

  FREE(b->num_chr);
  FREE(b->chr);

  FREE(b->num_prt);
  FREE(b->prt);
}

//--------------------------------------------------------------------------------------------
BUMPLIST * bumplist_renew(BUMPLIST * b)
{
  if(NULL == b) return NULL;

  bumplist_delete(b);
  return bumplist_new(b);
}

//--------------------------------------------------------------------------------------------
bool_t bumplist_allocate(BUMPLIST * b, int size)
{
  if(NULL == b) return bfalse;

  if(size <= 0)
  {
    bumplist_renew(b);
  }
  else
  {
    b->num_chr = calloc(size, sizeof(Uint16));
    b->chr     = calloc(size, sizeof(Uint16));

    b->num_prt = calloc(size, sizeof(Uint16));
    b->prt     = calloc(size, sizeof(Uint16));

    if(NULL != b->num_chr && NULL != b->chr && NULL != b->num_prt && NULL != b->prt)
    {
      b->valid      = btrue;
      b->num_blocks = size;
    }
  }

  return btrue;
};

//--------------------------------------------------------------------------------------------
bool_t bumplist_insert_chr(BUMPLIST * b, Uint32 block, CHR_REF chr_ref)
{
  // BB > insert a character into the bumplist at fanblock.

  if(NULL == b || !b->valid) return bfalse;

  b->chr_list[chr_ref].ref  = chr_ref;
  b->chr_list[chr_ref].next = b->chr[block];

  b->chr[block] = chr_ref;
  b->num_chr[block]++;

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t bumplist_insert_prt(BUMPLIST * b, Uint32 block, PRT_REF prt_ref)
{
  // BB > insert a particle into the bumplist at fanblock.

  if(NULL == b || !b->valid) return bfalse;

  b->prt_list[prt_ref].ref  = prt_ref;
  b->prt_list[prt_ref].next = b->prt[block];

  b->prt[block] = prt_ref;
  b->num_prt[block]++;

  return btrue;
}


//--------------------------------------------------------------------------------------------
CHR_REF bumplist_get_next_chr( BUMPLIST * b, CHR_REF chr_ref )
{
  Uint32  nodenext;
  CHR_REF bumpnext;

  if(NULL == b || !b->valid) return MAXPRT;

  nodenext = b->chr_list[chr_ref].next;
  bumpnext = b->chr_list[nodenext].ref;

  // scan until we find the next valid particle
  while( MAXPRT != nodenext && !VALID_PRT(bumpnext) )
  {
    nodenext = b->chr_list[bumpnext].next;
    bumpnext = b->chr_list[nodenext].ref;
  }

  return bumpnext;
};

//--------------------------------------------------------------------------------------------
PRT_REF bumplist_get_next_prt( BUMPLIST * b, PRT_REF prt_ref )
{
  Uint32  nodenext;
  PRT_REF bumpnext;

  if(NULL == b || !b->valid) return MAXPRT;

  nodenext = b->prt_list[prt_ref].next;
  bumpnext = b->prt_list[nodenext].ref;

  // scan until we find the next valid particle
  while( MAXPRT != nodenext && !VALID_PRT(bumpnext) )
  {
    nodenext = b->prt_list[bumpnext].next;
    bumpnext = b->prt_list[nodenext].ref;
  }

  return bumpnext;

};

//--------------------------------------------------------------------------------------------
CHR_REF    bumplist_get_chr_head(BUMPLIST * b, Uint32 block)
{
  if(NULL == b || !b->valid) return MAXCHR;
  if(block > b->num_blocks)  return MAXCHR;

  return b->chr[block];
};

//--------------------------------------------------------------------------------------------
PRT_REF    bumplist_get_prt_head(BUMPLIST * b, Uint32 block)
{
  if(NULL == b || !b->valid) return MAXPRT;
  if(block > b->num_blocks)  return MAXPRT;

  return b->prt[block];
}

//--------------------------------------------------------------------------------------------
bool_t bumplist_clear( BUMPLIST * b )
{
  Uint32 cnt;

  if(NULL == b || !b->valid) return bfalse;
  if( 0 == b->num_blocks)    return btrue;

  for ( cnt = 0; cnt < b->num_blocks; cnt++ )
  {
    b->valid        = bfalse;

    b->num_chr[cnt] = 0;
    b->chr[cnt]     = MAXCHR;

    b->num_prt[cnt] = 0;
    b->prt[cnt]     = MAXPRT;
  }

  for ( cnt = 0; cnt < MAXCHR; cnt++ )
  {
    b->chr_list[cnt].next = MAXCHR;
    b->chr_list[cnt].ref  = cnt;
  }

  for ( cnt = 0; cnt < MAXPRT; cnt++ )
  {
    b->prt_list[cnt].next = MAXCHR;
    b->prt_list[cnt].ref  = cnt;
  }

  return btrue;

};