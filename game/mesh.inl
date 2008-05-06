#pragma once

#include "mesh.h"
#include "char.h"
#include "particle.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

INLINE void mesh_set_colora( int fan_x, int fan_y, int color );

INLINE const Uint32 mesh_get_fan( vect3 pos );
INLINE const Uint32 mesh_get_block( vect3 pos );

INLINE const int    mesh_bump_tile( int fan_x, int fan_y );
INLINE const Uint16 mesh_get_tile( int fan_x, int fan_y );
INLINE const bool_t mesh_set_tile( int fan_x, int fan_y, Uint32 become );
INLINE const Uint32 mesh_convert_fan( int fan_x, int fan_y );
INLINE const Uint32 mesh_convert_block( int block_x, int block_y );
INLINE const float  mesh_fraction_x( float x );
INLINE const float  mesh_fraction_y( float y );
INLINE const bool_t mesh_check( float x, float y );
INLINE const Uint32 mesh_test_bits( int fan, Uint32 bits );
INLINE const bool_t mesh_has_some_bits( int fan, Uint32 bits );
INLINE const bool_t mesh_has_no_bits( int fan, Uint32 bits );
INLINE const bool_t mesh_has_all_bits( int fan, Uint32 bits );
INLINE const Uint8  mesh_get_twist( int fan );

INLINE const bool_t mesh_fan_is_in_renderlist( int fan );
INLINE       void   mesh_fan_remove_renderlist( int fan );
INLINE       void   mesh_fan_add_renderlist( int fan );

INLINE const float  mesh_clip_x( float x );
INLINE const float  mesh_clip_y( float y );
INLINE const int    mesh_clip_fan_x( int fan_x );
INLINE const int    mesh_clip_fan_y( int fan_y );
INLINE const int    mesh_clip_block_x( int block_x );
INLINE const int    mesh_clip_block_y( int block_y );

INLINE const bool_t mesh_fan_clear_bits( int fan_x, int fan_y, Uint32 bits );
INLINE const bool_t mesh_fan_add_bits( int fan_x, int fan_y, Uint32 bits );
INLINE const bool_t mesh_fan_set_bits( int fan_x, int fan_y, Uint32 bits );

INLINE const BUMPLIST * bumplist_new(BUMPLIST * b);
INLINE const void       bumplist_delete(BUMPLIST * b);
INLINE const BUMPLIST * bumplist_renew(BUMPLIST * b);
INLINE const bool_t     bumplist_allocate(BUMPLIST * b, int size);
INLINE const bool_t     bumplist_insert_chr(BUMPLIST * b, Uint32 block, CHR_REF chr_ref);
INLINE const bool_t     bumplist_insert_prt(BUMPLIST * b, Uint32 block, PRT_REF prt_ref);
INLINE const CHR_REF    bumplist_get_next_chr(BUMPLIST * b, CHR_REF ichr );
INLINE const PRT_REF    bumplist_get_next_prt(BUMPLIST * b, PRT_REF iprt );
INLINE const CHR_REF    bumplist_get_chr_head(BUMPLIST * b, Uint32 block);
INLINE const PRT_REF    bumplist_get_prt_head(BUMPLIST * b, Uint32 block);
INLINE const bool_t     bumplist_clear( BUMPLIST * b );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE const BUMPLIST * bumplist_new(BUMPLIST * b)
{
  if(NULL == b) return NULL;

  memset(b, 0, sizeof(BUMPLIST));

  return b;
};

//--------------------------------------------------------------------------------------------
INLINE const void bumplist_delete(BUMPLIST * b)
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
INLINE const BUMPLIST * bumplist_renew(BUMPLIST * b)
{
  if(NULL == b) return NULL;

  bumplist_delete(b);
  return bumplist_new(b);
}

//--------------------------------------------------------------------------------------------
INLINE const bool_t bumplist_allocate(BUMPLIST * b, int size)
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
INLINE const bool_t bumplist_insert_chr(BUMPLIST * b, Uint32 block, CHR_REF chr_ref)
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
INLINE const bool_t bumplist_insert_prt(BUMPLIST * b, Uint32 block, PRT_REF prt_ref)
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
INLINE const CHR_REF bumplist_get_next_chr( BUMPLIST * b, CHR_REF chr_ref )
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
INLINE const PRT_REF bumplist_get_next_prt( BUMPLIST * b, PRT_REF prt_ref )
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
INLINE const CHR_REF    bumplist_get_chr_head(BUMPLIST * b, Uint32 block)
{
  if(NULL == b || !b->valid) return MAXCHR;
  if(block > b->num_blocks)  return MAXCHR;

  return b->chr[block];
};

//--------------------------------------------------------------------------------------------
INLINE const PRT_REF    bumplist_get_prt_head(BUMPLIST * b, Uint32 block)
{
  if(NULL == b || !b->valid) return MAXPRT;
  if(block > b->num_blocks)  return MAXPRT;

  return b->prt[block];
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE const bool_t bumplist_clear( BUMPLIST * b )
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


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE const bool_t mesh_fan_is_in_renderlist( int fan )
{
  if ( INVALID_FAN == fan ) return bfalse;

  return Mesh_Fan[fan].inrenderlist;
};

//--------------------------------------------------------------------------------------------
INLINE void mesh_fan_remove_renderlist( int fan )
{
  Mesh_Fan[fan].inrenderlist = bfalse;
};

//--------------------------------------------------------------------------------------------
INLINE void mesh_fan_add_renderlist( int fan )
{
  Mesh_Fan[fan].inrenderlist = btrue;
};


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE const float mesh_clip_x( float x )
{
  if ( x <      0.0f )  x = 0.0f;
  if ( x > mesh.edge_x )  x = mesh.edge_x;

  return x;
}

//--------------------------------------------------------------------------------------------
INLINE const float mesh_clip_y( float y )
{
  if ( y <      0.0f )  y = 0.0f;
  if ( y > mesh.edge_y )  y = mesh.edge_y;

  return y;
}

//--------------------------------------------------------------------------------------------
INLINE const int mesh_clip_fan_x( int ix )
{
  if ( ix < 0 )  ix = 0;
  if ( ix > mesh.size_x - 1 )  ix = mesh.size_x - 1;

  return ix;
};

//--------------------------------------------------------------------------------------------
INLINE const int mesh_clip_fan_y( int iy )
{
  if ( iy < 0 )  iy = 0;
  if ( iy > mesh.size_y - 1 )  iy = mesh.size_y - 1;

  return iy;
};

//--------------------------------------------------------------------------------------------
INLINE const int mesh_clip_block_x( int ix )
{
  if ( ix < 0 )  ix = 0;
  if ( ix > ( mesh.size_x >> 2 ) - 1 )  ix = ( mesh.size_x >> 2 ) - 1;

  return ix;
};

//--------------------------------------------------------------------------------------------
INLINE const int mesh_clip_block_y( int iy )
{
  if ( iy < 0 )  iy = 0;
  if ( iy > ( mesh.size_y >> 2 ) - 1 )  iy = ( mesh.size_y >> 2 ) - 1;

  return iy;
};

//--------------------------------------------------------------------------------------------
INLINE const bool_t mesh_check( float x, float y )
{
  if ( x < 0 || x > mesh.edge_x ) return bfalse;
  if ( y < 0 || y > mesh.edge_x ) return bfalse;

  return btrue;
}


//--------------------------------------------------------------------------------------------
INLINE void mesh_set_colora( int fan_x, int fan_y, int color )
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
INLINE const Uint32 mesh_get_fan( vect3 pos )
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
INLINE const Uint32 mesh_get_block( vect3 pos )
{
  // BB > find the block under <x,y>

  return mesh_convert_block( MESH_FLOAT_TO_BLOCK( pos.x ), MESH_FLOAT_TO_BLOCK( pos.y ) );
};



//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE const bool_t mesh_fan_clear_bits( int fan_x, int fan_y, Uint32 bits )
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
INLINE const bool_t mesh_fan_add_bits( int fan_x, int fan_y, Uint32 bits )
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
INLINE const bool_t mesh_fan_set_bits( int fan_x, int fan_y, Uint32 bits )
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
//--------------------------------------------------------------------------------------------
INLINE const int mesh_bump_tile( int fan_x, int fan_y )
{
  Uint32 fan;

  fan = mesh_convert_fan( fan_x, fan_y );
  if ( INVALID_FAN == fan ) return 0;

  Mesh_Fan[fan].tile++;

  return Mesh_Fan[fan].tile;
}

//--------------------------------------------------------------------------------------------
INLINE const Uint16 mesh_get_tile( int fan_x, int fan_y )
{
  Uint32 fan;

  fan = mesh_convert_fan( fan_x, fan_y );
  if ( INVALID_FAN == fan ) return INVALID_TILE;

  return Mesh_Fan[fan].tile;
}

//--------------------------------------------------------------------------------------------
INLINE const bool_t mesh_set_tile( int fan_x, int fan_y, Uint32 become )
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
INLINE const Uint32 mesh_convert_fan( int fan_x, int fan_y )
{
  // BB > convert <fan_x,fan_y> to a fanblock

  if ( fan_x < 0 || fan_x > mesh.size_x || fan_y < 0 || fan_y > mesh.size_y ) return INVALID_FAN;

  return fan_x + Mesh_Fan_X[fan_y];
};

//--------------------------------------------------------------------------------------------
INLINE const Uint32 mesh_convert_block( int block_x, int block_y )
{
  // BB > convert <block_x,block_y> to a fanblock

  if ( block_x < 0 || block_x > ( mesh.size_x >> 2 ) || block_y < 0 || block_y > ( mesh.size_y >> 2 ) ) return INVALID_FAN;

  return block_x + Mesh_Block_X[block_y];
};


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE const Uint32 mesh_get_bits( int fan, Uint32 bits )
{
  if ( INVALID_FAN == fan ) return 0;

  return Mesh_Fan[fan].fx & bits;
};

//--------------------------------------------------------------------------------------------
INLINE const bool_t mesh_has_some_bits( int fan, Uint32 bits )
{
  if ( INVALID_FAN == fan ) return 0;

  return HAS_SOME_BITS( Mesh_Fan[fan].fx, bits );
};

//--------------------------------------------------------------------------------------------
INLINE const bool_t mesh_has_no_bits( int fan, Uint32 bits )
{
  if ( INVALID_FAN == fan ) return 0;

  return HAS_NO_BITS( Mesh_Fan[fan].fx, bits );
};

//--------------------------------------------------------------------------------------------
INLINE const bool_t mesh_has_all_bits( int fan, Uint32 bits )
{
  if ( INVALID_FAN == fan ) return 0;

  return HAS_ALL_BITS( Mesh_Fan[fan].fx, bits );
};

//--------------------------------------------------------------------------------------------
INLINE const float mesh_fraction_x( float x )
{
  return x / mesh.edge_x;
};

//--------------------------------------------------------------------------------------------
INLINE const float mesh_fraction_y( float y )
{
  return y / mesh.edge_y;
};


//--------------------------------------------------------------------------------------------
INLINE const Uint8 mesh_get_twist( int fan )
{
  Uint8 retval = 0x77;

  if ( INVALID_FAN != fan )
  {
    retval = Mesh_Fan[fan].twist;
  }

  return retval;
};