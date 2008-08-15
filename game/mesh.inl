#pragma once

#include "mesh.h"
#include "char.h"
#include "particle.h"
#include "game.h"

#include "egoboo_types.inl"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE const BUMPLIST_NODE * bumplist_node_new(BUMPLIST_NODE * n)
{
  if(NULL == n) return NULL;

  n->next = INVALID_BUMPLIST_NODE;
  n->ref  = INVALID_BUMPLIST_NODE;

  return n;
}

//--------------------------------------------------------------------------------------------
INLINE const Uint32 bumplist_node_get_ref(BUMPLIST_NODE * n)
{
  if(NULL == n) return INVALID_BUMPLIST_NODE;

  return n->ref;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE const BUMPLIST * bumplist_new(BUMPLIST * b)
{
  if(NULL == b) return NULL;

  bumplist_delete(b);

  memset(b, 0, sizeof(BUMPLIST));

  EKEY_PNEW(b, BUMPLIST);

  return b;
};

//--------------------------------------------------------------------------------------------
INLINE const bool_t bumplist_delete(BUMPLIST * b)
{
  if(NULL == b) return bfalse;

  if(!EKEY_PVALID(b)) return btrue;
  EKEY_PINVALIDATE(b);

  if(!b->allocated) return btrue;
  b->allocated = bfalse;

  if(0 == b->num_blocks) return btrue;
  b->num_blocks = 0;

  b->free_count = 0;
  EGOBOO_DELETE(b->free_lst);
  EGOBOO_DELETE(b->node_lst);

  EGOBOO_DELETE(b->num_chr);
  EGOBOO_DELETE(b->chr_ref);

  EGOBOO_DELETE(b->num_prt);
  EGOBOO_DELETE(b->prt_ref);

  return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE const BUMPLIST * bumplist_renew(BUMPLIST * b)
{
  if(NULL == b) return NULL;

  bumplist_delete(b);
  return bumplist_new(b);
}

//--------------------------------------------------------------------------------------------
INLINE const bool_t bumplist_clear(BUMPLIST * b)
{
  Uint32 i;

  if( !EKEY_PVALID(b) || !b->allocated ) return bfalse;

  // initialize the data
  for(i=0; i < b->num_blocks; i++)
  {
    b->num_chr[i] = 0;
    b->chr_ref[i].next = INVALID_BUMPLIST_NODE;
    b->chr_ref[i].ref  = REF_TO_INT(INVALID_CHR);

    b->num_prt[i] = 0;
    b->prt_ref[i].next = INVALID_BUMPLIST_NODE;
    b->prt_ref[i].ref  = REF_TO_INT(INVALID_PRT);
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE const bool_t bumplist_allocate(BUMPLIST * b, int size)
{
  if( !EKEY_PVALID(b) ) return bfalse;

  // deallocate the list if necessary
  if(size < 0)
  {
    bumplist_renew(b);
    return btrue;
  }

  // re-allocate blocks if necessary
  if (size > (int)b->num_blocks)
  {
    bumplist_renew(b);

    b->num_chr = EGOBOO_NEW_ARY( Uint16, size );
    b->chr_ref = EGOBOO_NEW_ARY( BUMPLIST_NODE, size );

    b->num_prt = EGOBOO_NEW_ARY( Uint16, size );
    b->prt_ref = EGOBOO_NEW_ARY( BUMPLIST_NODE, size );

    if(NULL != b->num_chr && NULL != b->chr_ref && NULL != b->num_prt && NULL != b->prt_ref)
    {
      b->num_blocks = size;
      b->allocated  = btrue;
    }
  }

  // clear the bumplist
  bumplist_clear(b);

  return btrue;
};

//--------------------------------------------------------------------------------------------
INLINE const Uint32 bumplist_get_free(BUMPLIST * b)
{
  if(NULL == b || b->free_count<=0) return INVALID_BUMPLIST_NODE;

  b->free_count--;
  return b->free_lst[b->free_count];
}

//--------------------------------------------------------------------------------------------
INLINE const bool_t bumplist_return_free(BUMPLIST * b, Uint32 ref)
{
  if(!EKEY_PVALID(b) || ref >= b->free_max) return bfalse;

  b->free_lst[b->free_count] = ref;
  b->free_count++;

  return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE const bool_t bumplist_insert_chr(BUMPLIST * b, Uint32 block, CHR_REF chr_ref)
{
  // BB > insert a character into the bumplist at fanblock.

  Uint32 ref;

  if(!EKEY_PVALID(b)) return bfalse;

  ref = bumplist_get_free(b);
  if( INVALID_BUMPLIST_NODE == ref ) return bfalse;

  // place this as the first node in the list
  b->node_lst[ref].ref  = REF_TO_INT(chr_ref);
  b->node_lst[ref].next = b->chr_ref[block].next;

  // make the list point to our new node
  b->chr_ref[block].next = ref;
  b->chr_ref[block].ref  = REF_TO_INT(INVALID_CHR);

  // increase the count for this block
  b->num_chr[block]++;

  return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE const bool_t bumplist_insert_prt(BUMPLIST * b, Uint32 block, PRT_REF prt_ref)
{
  // BB > insert a particle into the bumplist at fanblock.

  Uint32 ref;

  if(!EKEY_PVALID(b)) return bfalse;

  ref = bumplist_get_free(b);
  if( INVALID_BUMPLIST_NODE == ref ) return bfalse;

  // place this as the first node in the list
  b->node_lst[ref].ref  = REF_TO_INT(prt_ref);
  b->node_lst[ref].next = b->prt_ref[block].next;

  // make the list point to out new node
  b->prt_ref[block].next = ref;
  b->prt_ref[block].ref  = REF_TO_INT(INVALID_PRT);

  // increase the count for this block
  b->num_chr[block]++;

  return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE const Uint32 bumplist_get_next( BUMPLIST * b, Uint32 node )
{
  if(!EKEY_PVALID(b) || INVALID_BUMPLIST_NODE == node) return INVALID_BUMPLIST_NODE;

  return b->node_lst[node].next;
}

//--------------------------------------------------------------------------------------------
INLINE const Uint32 bumplist_get_next_chr( Game_t * gs, BUMPLIST * b, Uint32 node )
{
  Uint32  nodenext;
  CHR_REF bumpnext;

  if(!EKEY_PVALID(b) || INVALID_BUMPLIST_NODE == node) return INVALID_BUMPLIST_NODE;

  nodenext = b->node_lst[node].next;
  bumpnext = b->node_lst[nodenext].ref;

  while( INVALID_BUMPLIST_NODE != nodenext && !ACTIVE_CHR(gs->ChrList, bumpnext) )
  {
    nodenext = b->node_lst[node].next;
    bumpnext = b->node_lst[nodenext].ref;
  }

  return nodenext;
}

//--------------------------------------------------------------------------------------------
INLINE const Uint32 bumplist_get_next_prt( Game_t * gs, BUMPLIST * b, Uint32 node )
{
  Uint32  nodenext;
  PRT_REF bumpnext;

  if(!EKEY_PVALID(b) || INVALID_BUMPLIST_NODE == node) return INVALID_BUMPLIST_NODE;

  nodenext = b->node_lst[node].next;
  bumpnext = b->node_lst[nodenext].ref;

  while( INVALID_BUMPLIST_NODE != nodenext && !ACTIVE_PRT(gs->PrtList, bumpnext) )
  {
    nodenext = b->node_lst[node].next;
    bumpnext = b->node_lst[nodenext].ref;
  }

  return nodenext;
}

//--------------------------------------------------------------------------------------------
INLINE const Uint32 bumplist_get_chr_head(BUMPLIST * b, Uint32 block)
{
  if(!EKEY_PVALID(b)) return INVALID_BUMPLIST_NODE;
  if(block > b->num_blocks)  return INVALID_BUMPLIST_NODE;

  return b->chr_ref[block].next;
};

//--------------------------------------------------------------------------------------------
INLINE const Uint32 bumplist_get_prt_head(BUMPLIST * b, Uint32 block)
{
  if(!EKEY_PVALID(b)) return INVALID_BUMPLIST_NODE;
  if(block > b->num_blocks)  return INVALID_BUMPLIST_NODE;

  return b->prt_ref[block].next;
}

//--------------------------------------------------------------------------------------------
INLINE const Uint32 bumplist_get_chr_count(BUMPLIST * b, Uint32 block)
{
  if(!EKEY_PVALID(b)) return INVALID_BUMPLIST_NODE;
  if(block > b->num_blocks)  return INVALID_BUMPLIST_NODE;

  return b->num_chr[block];
};

//--------------------------------------------------------------------------------------------
INLINE const Uint32 bumplist_get_prt_count(BUMPLIST * b, Uint32 block)
{
  if(!EKEY_PVALID(b)) return INVALID_BUMPLIST_NODE;
  if(block > b->num_blocks)  return INVALID_BUMPLIST_NODE;

  return b->num_prt[block];
}

//--------------------------------------------------------------------------------------------
INLINE const Uint32 bumplist_get_ref(BUMPLIST * b, Uint32 node)
{
  if(!EKEY_PVALID(b)) return INVALID_BUMPLIST_NODE;
  if(node > b->free_max)    return INVALID_BUMPLIST_NODE;

  return b->node_lst[node].ref;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE const bool_t mesh_fan_is_in_renderlist( MeshTile_t * mf_list, int fan )
{
  if ( INVALID_FAN == fan ) return bfalse;

  return mf_list[fan].inrenderlist;
};

//--------------------------------------------------------------------------------------------
INLINE void mesh_fan_remove_renderlist( MeshTile_t * mf_list, int fan )
{
  mf_list[fan].inrenderlist = bfalse;
};

//--------------------------------------------------------------------------------------------
INLINE void mesh_fan_add_renderlist( MeshTile_t * mf_list, int fan )
{
  mf_list[fan].inrenderlist = btrue;
};


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE const float mesh_clip_x( MeshInfo_t * mi, float x )
{
  if ( x <      0.0f )  x = 0.0f;
  if ( x > mi->edge_x )  x = mi->edge_x;

  return x;
}

//--------------------------------------------------------------------------------------------
INLINE const float mesh_clip_y( MeshInfo_t * mi, float y )
{
  if ( y <      0.0f )  y = 0.0f;
  if ( y > mi->edge_y )  y = mi->edge_y;

  return y;
}

//--------------------------------------------------------------------------------------------
INLINE const int mesh_clip_fan_x( MeshInfo_t * mi, int ix )
{
  if ( ix < 0 )  ix = 0;
  if ( ix > mi->size_x - 1 )  ix = mi->size_x - 1;

  return ix;
};

//--------------------------------------------------------------------------------------------
INLINE const int mesh_clip_fan_y( MeshInfo_t * mi, int iy )
{
  if ( iy < 0 )  iy = 0;
  if ( iy > mi->size_y - 1 )  iy = mi->size_y - 1;

  return iy;
};

//--------------------------------------------------------------------------------------------
INLINE const int mesh_clip_block_x( MeshInfo_t * mi, int ix )
{
  if ( ix < 0 )  ix = 0;
  if ( ix > ( mi->size_x >> 2 ) - 1 )  ix = ( mi->size_x >> 2 ) - 1;

  return ix;
};

//--------------------------------------------------------------------------------------------
INLINE const int mesh_clip_block_y( MeshInfo_t * mi, int iy )
{
  if ( iy < 0 )  iy = 0;
  if ( iy > ( mi->size_y >> 2 ) - 1 )  iy = ( mi->size_y >> 2 ) - 1;

  return iy;
};

//--------------------------------------------------------------------------------------------
INLINE const bool_t mesh_check( MeshInfo_t * mi, float x, float y )
{
  if ( x < 0 || x > mi->edge_x ) return bfalse;
  if ( y < 0 || y > mi->edge_y ) return bfalse;

  return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE const bool_t mesh_check_fan( MeshInfo_t * mi, int fan_x, int fan_y )
{
  if ( fan_x < 0 || fan_x >= mi->size_x ) return bfalse;
  if ( fan_y < 0 || fan_y >= mi->size_y ) return bfalse;

  return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE void mesh_set_colora( Mesh_t * pmesh, int fan_x, int fan_y, int color )
{
  Uint32 cnt, fan, vert, numvert;

  MeshInfo_t * mi      = &(pmesh->Info);
  MeshTile_t * mf_list = pmesh->Mem.tilelst;

  fan = mesh_convert_fan( mi, fan_x, fan_y );
  if ( INVALID_FAN == fan ) return;

  vert = mf_list[fan].vrt_start;
  cnt = 0;
  numvert = pmesh->TileDict[mf_list[fan].type].vrt_count;
  while ( cnt < numvert )
  {
    pmesh->Mem.vrt_ar_fp8[vert] = color;
    pmesh->Mem.vrt_ag_fp8[vert] = color;
    pmesh->Mem.vrt_ab_fp8[vert] = color;
    vert++;
    cnt++;
  }
}

//--------------------------------------------------------------------------------------------
INLINE const Uint32 mesh_get_fan( Mesh_t * pmesh, vect3 pos )
{
  // BB > find the tile under <pos.x,pos.y>, but MAKE SURE we have the right tile.

  Uint32 ivert, testfan = INVALID_FAN;
  float minx, maxx, miny, maxy;
  int i, ix, iy;
  bool_t bfound = bfalse;

  MeshInfo_t  * mi      = &(pmesh->Info);
  MeshMem_t   * mm      = &(pmesh->Mem);
  MeshTile_t  * mf_list = mm->tilelst;

  if ( !mesh_check( mi, pos.x, pos.y ) )
    return testfan;

  ix = MESH_FLOAT_TO_FAN( pos.x );
  iy = MESH_FLOAT_TO_FAN( pos.y );

  testfan = mesh_convert_fan( mi, ix, iy );
  if(INVALID_FAN == testfan) return testfan;

  ivert = mf_list[testfan].vrt_start;
  minx = maxx = mm->vrt_x[ivert];
  miny = maxy = mm->vrt_y[ivert];
  for ( i = 1;i < 4;i++, ivert++ )
  {
    minx = MIN( minx, mm->vrt_x[ivert] );
    maxx = MAX( maxx, mm->vrt_x[ivert] );

    miny = MIN( miny, mm->vrt_y[ivert] );
    maxy = MAX( maxy, mm->vrt_y[ivert] );
  };

  if ( pos.x < minx ) { ix--; bfound = btrue; }
  else if ( pos.x > maxx ) { ix++; bfound = btrue; }

  if ( pos.y < miny ) { iy--; bfound = btrue; }
  else if ( pos.y > maxy ) { iy++; bfound = btrue; }

  testfan = mesh_convert_fan( mi, ix, iy );

  return testfan;
};

//--------------------------------------------------------------------------------------------
INLINE const Uint32 mesh_get_block( MeshInfo_t * mi, vect3 pos )
{
  // BB > find the block under <x,y>

  return mesh_convert_block( mi, MESH_FLOAT_TO_BLOCK( pos.x ), MESH_FLOAT_TO_BLOCK( pos.y ) );
};



//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE const bool_t mesh_fan_clear_bits( Mesh_t * pmesh, int fan_x, int fan_y, Uint32 bits )
{
  bool_t retval = bfalse;
  Uint32 fan;

  MeshTile_t  * mf_list = pmesh->Mem.tilelst;
  MeshInfo_t  * mi      = &(pmesh->Info);

  fan = mesh_convert_fan( mi, fan_x, fan_y );
  if ( INVALID_FAN == fan ) return retval;

  retval = mesh_has_some_bits( mf_list, fan, bits );

  mf_list[fan].fx &= ~bits;

  return retval;
};

//--------------------------------------------------------------------------------------------
INLINE const bool_t mesh_fan_add_bits( Mesh_t * pmesh, int fan_x, int fan_y, Uint32 bits )
{
  bool_t retval = bfalse;
  Uint32 fan;

  MeshTile_t  * mf_list = pmesh->Mem.tilelst;
  MeshInfo_t * mi      = &(pmesh->Info);

  fan = mesh_convert_fan( mi, fan_x, fan_y );
  if ( INVALID_FAN == fan ) return retval;

  retval = MISSING_BITS( mf_list[fan].fx, bits );

  mf_list[fan].fx |= bits;

  return retval;
};

//--------------------------------------------------------------------------------------------
INLINE const bool_t mesh_fan_set_bits( Mesh_t * pmesh, int fan_x, int fan_y, Uint32 bits )
{
  bool_t retval = bfalse;
  Uint32 fan;

  MeshTile_t  * mf_list = pmesh->Mem.tilelst;
  MeshInfo_t * mi      = &(pmesh->Info);

  fan = mesh_convert_fan( mi, fan_x, fan_y );
  if ( INVALID_FAN == fan ) return retval;

  retval = ( mf_list[fan].fx != bits );

  mf_list[fan].fx = bits;

  return retval;
};


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE const int mesh_bump_tile( Mesh_t * pmesh, int fan_x, int fan_y )
{
  Uint32 fan;

  MeshTile_t  * mf_list = pmesh->Mem.tilelst;
  MeshInfo_t * mi      = &(pmesh->Info);

  fan = mesh_convert_fan( mi, fan_x, fan_y );
  if ( INVALID_FAN == fan ) return 0;

  mf_list[fan].tile++;

  return mf_list[fan].tile;
}

//--------------------------------------------------------------------------------------------
INLINE const Uint16 mesh_get_tile( Mesh_t * pmesh, int fan_x, int fan_y )
{
  Uint32 fan;

  MeshTile_t * mf_list = pmesh->Mem.tilelst;
  MeshInfo_t * mi       = &(pmesh->Info);

  fan = mesh_convert_fan( mi, fan_x, fan_y );
  if ( INVALID_FAN == fan ) return INVALID_TILE;

  return mf_list[fan].tile;
}

//--------------------------------------------------------------------------------------------
INLINE const bool_t mesh_set_tile( Mesh_t * pmesh, int fan_x, int fan_y, Uint32 become )
{
  bool_t retval = bfalse;
  Uint32 fan;

  MeshTile_t  * mf_list = pmesh->Mem.tilelst;
  MeshInfo_t * mi      = &(pmesh->Info);

  fan = mesh_convert_fan( mi, fan_x, fan_y );
  if ( INVALID_FAN == fan ) return retval;

  if ( become != 0 )
  {
    mf_list[fan].tile = become;
    retval = btrue;
  }

  return retval;
}

//--------------------------------------------------------------------------------------------
INLINE const Uint32 mesh_convert_fan( MeshInfo_t * mi, int fan_x, int fan_y )
{
  // BB > convert <fan_x,fan_y> to a fanblock

  if ( fan_x < 0 || fan_x >= mi->size_x || fan_y < 0 || fan_y >= mi->size_y ) return INVALID_FAN;

  return fan_x + mi->Fan_X[fan_y];
};

//--------------------------------------------------------------------------------------------
INLINE const Uint32 mesh_convert_block( MeshInfo_t * mi, int block_x, int block_y )
{
  // BB > convert <block_x,block_y> to a fanblock

  if ( block_x < 0 || block_x > ( mi->size_x >> 2 ) || block_y < 0 || block_y > ( mi->size_y >> 2 ) ) return INVALID_FAN;

  return block_x + mi->Block_X[block_y];
};


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE const Uint32 mesh_get_bits( MeshTile_t * mf_list, int fan, Uint32 bits )
{
  if ( INVALID_FAN == fan ) return 0;

  return mf_list[fan].fx & bits;
};

//--------------------------------------------------------------------------------------------
INLINE const bool_t mesh_has_some_bits( MeshTile_t * mf_list, int fan, Uint32 bits )
{
  if ( INVALID_FAN == fan ) return 0;

  return HAS_SOME_BITS( mf_list[fan].fx, bits );
};

//--------------------------------------------------------------------------------------------
INLINE const bool_t mesh_has_no_bits( MeshTile_t * mf_list, int fan, Uint32 bits )
{
  if ( INVALID_FAN == fan ) return 0;

  return HAS_NO_BITS( mf_list[fan].fx, bits );
};

//--------------------------------------------------------------------------------------------
INLINE const bool_t mesh_has_all_bits( MeshTile_t * mf_list, int fan, Uint32 bits )
{
  if ( INVALID_FAN == fan ) return 0;

  return HAS_ALL_BITS( mf_list[fan].fx, bits );
};

//--------------------------------------------------------------------------------------------
INLINE const float mesh_fraction_x( MeshInfo_t * mi, float x )
{
  return x / mi->edge_x;
};

//--------------------------------------------------------------------------------------------
INLINE const float mesh_fraction_y( MeshInfo_t * mi, float y )
{
  return y / mi->edge_y;
};


//--------------------------------------------------------------------------------------------
INLINE const Uint8 mesh_get_twist( MeshTile_t * mf_list, int fan )
{
  Uint8 retval = 0x77;

  if ( INVALID_FAN != fan )
  {
    retval = mf_list[fan].twist;
  }

  return retval;
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE Mesh_t * Mesh_new( Mesh_t * pmesh )
{
  memset( pmesh, 0, sizeof(Mesh_t) );

  MeshMem_new(&(pmesh->Mem), 0, 0 );
  memcpy( pmesh->TileDict, gTileDict, sizeof(TileDictionary_t));

  return pmesh;
};

INLINE bool_t Mesh_delete( Mesh_t * pmesh )
{
  if(NULL == pmesh) return bfalse;

  MeshMem_delete(&(pmesh->Mem));				                       //Free the mesh memory
  memset( pmesh->TileDict, 0, sizeof(pmesh->TileDict) );

  return bfalse;
};