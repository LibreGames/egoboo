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
/// @brief 
/// @details functions that will be declared inside the base class

#include "Clock.h"

#include "egoboo_config.h"
#include "egoboo_types.h"
#include "egoboo_math.inl"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

INLINE egoboo_key_t * egoboo_key_create(Uint32 itype, void * pdata);
INLINE bool_t         egoboo_key_destroy(egoboo_key_t ** pkey);
INLINE egoboo_key_t * egoboo_key_new(egoboo_key_t * pkey, Uint32 itype, void * pdata);
INLINE bool_t         egoboo_key_validate(egoboo_key_t * pkey);
INLINE bool_t         egoboo_key_invalidate(egoboo_key_t * pkey);
INLINE bool_t         egoboo_key_valid(egoboo_key_t * pkey);
INLINE void *         egoboo_key_get_data(egoboo_key_t * pkey, Uint32 type);

//INLINE CList * CList_new(CList * lst, size_t count, size_t size);
//INLINE bool_t  CList_delete(CList * lst);
//INLINE void  * CList_getData(CList * lst, int index);

#if SDL_BYTEORDER != SDL_LIL_ENDIAN
    INLINE float SwapLE_float( float val );
#endif

    
INLINE HashNode_t * HashNode_create(void * data);
INLINE bool_t       HashNode_destroy(HashNode_t **);
INLINE HashNode_t * HashNode_insert_after (HashNode_t lst[], HashNode_t * n);
INLINE HashNode_t * HashNode_insert_before(HashNode_t lst[], HashNode_t * n);
INLINE HashNode_t * HashNode_remove_after (HashNode_t lst[]);
INLINE HashNode_t * HashNode_remove       (HashNode_t lst[]);

INLINE HashList_t * HashList_create(int size);
INLINE bool_t     HashList_destroy(HashList_t **);

INLINE BSP_node_t * BSP_node_new( BSP_node_t * t, void * data, int type );
INLINE bool_t     BSP_node_delete( BSP_node_t * t );

INLINE BSP_leaf_t * BSP_leaf_new( BSP_leaf_t * L, size_t size );
INLINE bool_t       BSP_leaf_delete( BSP_leaf_t * L );
INLINE bool_t       BSP_leaf_insert( BSP_leaf_t * L, BSP_node_t * n );

INLINE BSP_tree_t * BSP_tree_new( BSP_tree_t * t, Sint32 dim, Sint32 depth);
INLINE bool_t     BSP_tree_delete( BSP_tree_t * t );

INLINE Sint32 BSP_tree_count_nodes(Sint32 dim, Sint32 depth);

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------


#if SDL_BYTEORDER == SDL_LIL_ENDIAN

#    define SwapLE_float

#else

    INLINE float SwapLE_float( float val )
    {
      FCONVERT convert;

      convert.f = val;
      convert.i = SDL_SwapLE32(convert.i);

      return convert.f;
    };

#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE bool_t HashNode_delete(HashNode_t * n)
{
  if(NULL == n) return bfalse;
  if( !EKEY_PVALID( n ) ) return btrue;

  n->data        = NULL;
  egoboo_key_invalidate(&(n->ekey));

  return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE HashNode_t * HashNode_new(HashNode_t * n, void * data)
{
  if(NULL == n) return n;
  if( EKEY_PVALID( n ) )
  {
    HashNode_delete(n);
  }

  memset(n, 0, sizeof(HashNode_t));

  EKEY_PNEW( n, HashNode_t );

  n->data = data;

  return n;
}

//--------------------------------------------------------------------------------------------
INLINE HashNode_t * HashNode_create(void * data)
{
  HashNode_t * n = EGOBOO_NEW( HashNode_t );
  if(NULL == n) return n;

  return HashNode_new(n, data);
}

//--------------------------------------------------------------------------------------------
INLINE bool_t HashNode_destroy(HashNode_t ** pn)
{
  bool_t retval = bfalse;

  if(NULL == pn || NULL == *pn) return bfalse;

  retval = HashNode_delete(*pn);

  EGOBOO_DELETE(*pn);

  return retval;
}

//--------------------------------------------------------------------------------------------
INLINE HashNode_t * HashNode_insert_after(HashNode_t lst[], HashNode_t * n)
{
  if( NULL == n ) return lst;
  n->next = NULL;

  if( NULL == lst ) return n;

  n->next = n->next;
  lst->next = n;

  return lst;
}

//--------------------------------------------------------------------------------------------
INLINE HashNode_t * HashNode_insert_before(HashNode_t lst[], HashNode_t * n)
{
  if( NULL == n ) return lst;
  n->next = NULL;

  if( NULL == lst ) return n;

  n->next = lst;

  return n;
}

//--------------------------------------------------------------------------------------------
INLINE HashNode_t * HashNode_remove_after(HashNode_t lst[])
{
  HashNode_t * n;

  if(NULL == lst) return NULL;

  n = lst->next;
  if(NULL == n) return lst;

  lst->next = n->next;
  n->next   = NULL;

  return lst;
}

//--------------------------------------------------------------------------------------------
INLINE HashNode_t * HashNode_remove(HashNode_t lst[])
{
  HashNode_t * n;

  if(NULL == lst) return NULL;

  n = lst->next;
  if(NULL == n) return NULL;

  lst->next = NULL;

  return n;
}


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE bool_t HashList_deallocate(HashList_t * lst)
{
  if(NULL == lst) return bfalse;
  if(0 == lst->allocated) return btrue;

  EGOBOO_DELETE(lst->subcount);
  EGOBOO_DELETE(lst->sublist);
  lst->allocated = 0;

  return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t HashList_allocate(HashList_t * lst, int size)
{
  if(NULL == lst) return bfalse;

  HashList_deallocate(lst);

  lst->subcount = EGOBOO_NEW_ARY(int, size);
  if(NULL == lst->subcount)
  {
    return bfalse;
  }

  lst->sublist = EGOBOO_NEW_ARY( HashNode_t *, size );
  if(NULL == lst->sublist)
  {
    EGOBOO_DELETE(lst->subcount);
    return bfalse;
  }

  lst->allocated = size;

  return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE HashList_t * HashList_new(HashList_t * lst, int size)
{
  if(NULL == lst) return NULL;

  if(size<0) size = 256;
  HashList_allocate(lst, size);

  return lst;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t HashList_delete(HashList_t * lst)
{
  if(NULL == lst) return bfalse;

  HashList_deallocate(lst);

  return btrue;
}
//--------------------------------------------------------------------------------------------
INLINE HashList_t * HashList_create(int size)
{
  return HashList_new( EGOBOO_NEW( HashList_t ), size);
}

//--------------------------------------------------------------------------------------------
INLINE bool_t HashList_destroy(HashList_t ** plst)
{
  bool_t retval = bfalse;

  if(NULL == plst || NULL == *plst) return bfalse;

  retval = HashList_delete(*plst);

  EGOBOO_DELETE(*plst);

  return retval;
}



//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

INLINE Uint32 make_key_32(Uint32 * seed)
{
  return ego_rand_32(seed);
}

INLINE Uint32 make_key_16(Uint16 * seed)
{
  Uint32 ret = ego_rand_16(seed);
  ret        = ego_rand_16(seed) | (ret << 16);

  return ret;
}

INLINE Uint32 make_key_8(Uint8 * seed)
{
  Uint32 ret = ego_rand_8(seed);
  ret        = ego_rand_8(seed) | (ret << 8);
  ret        = ego_rand_8(seed) | (ret << 8);
  ret        = ego_rand_8(seed) | (ret << 8);

  return ret;
}

INLINE egoboo_key_t * egoboo_key_create(Uint32 itype, void * pdata)
{
  /// @details BB@> dynamically allocate and initialize a new key

  egoboo_key_t * ptmp, * pkey;

  pkey = EGOBOO_NEW( egoboo_key_t );
  if(NULL == pkey) return pkey;

  // initialize the key
  ptmp = egoboo_key_new(pkey, itype, pdata);
  if(NULL == ptmp)
  {
    EGOBOO_DELETE(pkey);
  }

  // let everyone know that this data is on the heap, not the stack
  pkey->dynamic = btrue;

  return pkey;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t egoboo_key_destroy(egoboo_key_t ** ppkey)
{
  /// @details BB@> de-initialize the key and free it if necessary

  if( NULL == ppkey || NULL == *ppkey) return bfalse;

  if( !egoboo_key_invalidate( *ppkey ) ) return bfalse;

  // do the actual freeing. make sure never to free memory that is on the stack.
  if((*ppkey)->dynamic)
  {
    EGOBOO_DELETE(*ppkey);
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE egoboo_key_t * egoboo_key_new(egoboo_key_t * pkey, Uint32 itype, void * pdata)
{
  /// @details BB@> initialize the key

  if(NULL == pkey) return pkey;

  // initialize the key. pkey->dynamic is automatically set to bfalse
  memset(pkey, 0, sizeof(egoboo_key_t));

  // set the proper values
  if( egoboo_key_validate(pkey) )
  {
    pkey->data_type = itype;
    pkey->pdata     = pdata;
  }

  return pkey;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t egoboo_key_invalidate(egoboo_key_t * pkey)
{
  /// @details BB@> de-initialize the key

  if(NULL == pkey) return bfalse;

  if( !egoboo_key_valid(pkey) ) return bfalse;

  //PROFILE_BEGIN(ekey);

  // make sure that the key is invalid
  pkey->v1 = pkey->v2 = 0;

  //PROFILE_END2( ekey );

  return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t egoboo_key_validate(egoboo_key_t * pkey)
{
  /// @details BB@> de-initialize the key

  //static Uint32 new_id_32 = (Uint32)(~0);
  //static Uint16 new_id_16 = (Uint16)(~0);
  static Uint8  new_id_8  = (Uint8 )(~0);

  if(NULL == pkey) return bfalse;

  if( egoboo_key_valid(pkey) ) return btrue;

  //PROFILE_BEGIN(ekey);

  // make sure that the key is invalid
  //pkey->v1 = make_key_32(&new_id_32);
  //pkey->v2 = make_key_32(&new_id_32);

  //pkey->v1 = make_key_16(&new_id_16);
  //pkey->v2 = make_key_16(&new_id_16);

  pkey->v1 = make_key_8(&new_id_8);
  pkey->v2 = make_key_8(&new_id_8);


  PROFILE_END2(ekey);

  return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t egoboo_key_valid(egoboo_key_t * pkey)
{
  /// @details BB@> verify that a "key" is valid.

  Uint32 val_32;
  //Uint32 seed_32;
  //Uint16 seed_16;
  Uint8  seed_8;

  if(NULL == pkey) return bfalse;

  //PROFILE_BEGIN(ekey);

  //seed_32 = pkey->v1;
  //val_32 = ego_rand_32(&seed_32);

  //seed_16 = pkey->v1 & 0xFFFF;
  //val_32  = make_key_16( &seed_16 );

  seed_8 = pkey->v1 & 0xFF;
  val_32  = make_key_8( &seed_8 );

  PROFILE_END2( ekey );

  return (val_32 == pkey->v2);
}

//--------------------------------------------------------------------------------------------
INLINE void * egoboo_key_get_data(egoboo_key_t * pkey, Uint32 type)
{
  /// @details BB@> grab the data associated with the key, if and only if the key is valid and the
  ///      data type matches the key data

  if(NULL == pkey) return NULL;

  // if the type doesn't match, don't bother
  if(type != pkey->data_type) return NULL;

  // do this test second so we avoid the math operations until necessary
  if( !egoboo_key_valid(pkey) ) return NULL;

  // we have a match
  return pkey->pdata;

}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE BSP_node_t * BSP_node_new( BSP_node_t * n, void * data, int type )
{
  if(NULL == n) return n;

  BSP_node_delete( n );

  memset(n, 0, sizeof(BSP_node_t));

  if(NULL == data) return n;

  n->data_type = type;
  n->data      = data;

  EKEY_PNEW( n, BSP_node_t );

  return n;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t BSP_node_delete( BSP_node_t * n )
{
  bool_t retval;

  if( NULL == n ) return bfalse;
  if( !EKEY_PVALID(n) ) return btrue;

  n->data_type = -1;
  n->data      = NULL;

  EKEY_PINVALIDATE( n );

  return retval;
}


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE BSP_leaf_t * BSP_leaf_new( BSP_leaf_t * L, size_t count )
{
  if(NULL == L) return L;

  BSP_leaf_delete( L );

  memset(L, 0, sizeof(BSP_leaf_t));

  L->children = EGOBOO_NEW_ARY(BSP_leaf_t*, count);
  if(NULL != L->children)
  {
    L->child_count = count;
  }

  EKEY_PNEW( L, BSP_leaf_t );

  return L;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t BSP_leaf_delete( BSP_leaf_t * L )
{
  bool_t retval;

  if( NULL == L ) return bfalse;
  if( !EKEY_PVALID(L) ) return btrue;

  EGOBOO_DELETE(L->children);
  L->child_count = 0;

  EKEY_PINVALIDATE( L );

  return retval;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t BSP_leaf_insert( BSP_leaf_t * L, BSP_node_t * n )
{
  if( !EKEY_PVALID(L) || !EKEY_PVALID(n) ) return bfalse;

  n->next  = L->nodes;
  L->nodes = n;

  return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE bool_t BSP_tree_allocate( BSP_tree_t * t, size_t count, size_t children )
{
  size_t i;

  if(NULL == t) return bfalse;
  if(NULL != t->leaf_list || t->leaf_count > 0) return bfalse;

  // allocate the nodes
  t->leaf_list = EGOBOO_NEW_ARY( BSP_leaf_t, count );
  if(NULL == t->leaf_list) return bfalse;

  // initialize the nodes
  t->leaf_count = count;
  for(i=0; i<count; i++)
  {
    BSP_leaf_new(t->leaf_list + i, children);
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t BSP_tree_deallocate( BSP_tree_t * t )
{
  size_t i;

  if( NULL == t ) return bfalse;
  if( !EKEY_PVALID(t) ) return btrue;
  if( NULL == t->leaf_list || 0 == t->leaf_count ) return btrue;

  // delete the nodes
  for(i=0; i<t->leaf_count; i++)
  {
    BSP_leaf_delete(t->leaf_list + i);
  }

  EGOBOO_DELETE(t->leaf_list);
  t->leaf_count = 0;

  return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t BSP_tree_construct(BSP_tree_t * t)
{
  int i, j, k;
  int children, current_node;

  if(!EKEY_PVALID(t)) return bfalse;
  if(0 == t->leaf_count || NULL == t->leaf_list) return bfalse;

  children = 2 << t->dimensions;
  t->leaf_list[0].parent = NULL;
  current_node = 0;
  for(i=0; i<t->depth; i++)
  {
    int node_at_depth, free_node;

    node_at_depth = 1 << (i * t->dimensions);
    free_node = BSP_tree_count_nodes(t->dimensions, i);
    for(j=0; j<node_at_depth; j++)
    {
      for(k=0;k<children;k++)
      {
        t->leaf_list[free_node].parent = t->leaf_list + current_node;
        t->leaf_list[current_node].children[k] = t->leaf_list + free_node;
        free_node++;
      }
      current_node++;
    }
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE BSP_tree_t * BSP_tree_new( BSP_tree_t * t, Sint32 dim, Sint32 depth)
{
  Sint32 count, children;

  if(NULL == t) return t;

  BSP_tree_delete( t );

  memset(t, 0, sizeof(BSP_tree_t));

  count = BSP_tree_count_nodes(dim, depth);
  if(count < 0) return t;

  children = 2 << dim;
  if( !BSP_tree_allocate(t, count, children) ) return t;

  t->dimensions = dim;
  t->depth      = depth;

  EKEY_PNEW( t, BSP_tree_t );

  BSP_tree_construct(t);

  return t;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t BSP_tree_delete( BSP_tree_t * t )
{
  bool_t retval;

  if( NULL == t ) return bfalse;
  if( !EKEY_PVALID(t) ) return btrue;

  retval = BSP_tree_deallocate( t );

  EKEY_PINVALIDATE( t );

  return retval;
}

//--------------------------------------------------------------------------------------------
INLINE Sint32 BSP_tree_count_nodes(Sint32 dim, Sint32 depth)
{
  int itmp;
  Sint32 node_count;
  Uint32 numer, denom;

  itmp = dim * (depth+1);
  if( itmp > 31 ) return -1;

  numer = (1 << itmp) - 1;
  denom = (1 <<  dim) - 1;
  node_count    = numer / denom;

  return node_count;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t BSP_tree_insert( BSP_tree_t * t, BSP_leaf_t * L, BSP_node_t * n, int index )
{
  if( !EKEY_PVALID(t) || !EKEY_PVALID(L) || !EKEY_PVALID(n) ) return bfalse;
  if( index > (int)L->child_count ) return bfalse;

  if( index >= 0 && NULL != L->children[index])
  {
    // inserting a node into the child
    return BSP_leaf_insert(L->children[index], n);
  }

  if(index < 0 || NULL == t->leaf_list)
  {
    // inserting a node into this leaf node
    // this can either occur because someone requested it (index < 0)
    // OR because there are no more free nodes
    return BSP_leaf_insert(L, n);
  }
  else
  {
    // the requested L->children[index] slot is empty. grab a pre-allocated
    // BSP_leaf_t from the free list in the BSP_tree_t structure an insert it in
    // this child node

    L->children[index] = t->leaf_list;
    t->leaf_list = t->leaf_list->parent;           // parent is doubling for "next"

    BSP_leaf_new( L->children[index], 2 << t->dimensions );

    return BSP_leaf_insert(L->children[index], n);
  }

  // something went wrong
  return bfalse;
}




////--------------------------------------------------------------------------------------------
////--------------------------------------------------------------------------------------------
//INLINE CList * CList_new(CList * lst, size_t count, size_t size)
//{
//  if(NULL == lst) return NULL;
//
//  CList_delete(lst);
//
//  memset(lst, 0, sizeof(CList));
//
//  EKEY_PNEW(lst, CList);
//
//  lst->data = calloc(count, size);
//  if(NULL != lst->data)
//  {
//    lst->count = count;
//    lst->size  = size;
//  }
//
//  return lst;
//}
//
////--------------------------------------------------------------------------------------------
//INLINE bool_t CList_delete(CList * lst)
//{
//  if(NULL == lst) return bfalse;
//  if(!EKEY_PVALID(lst)) return btrue;
//
//  if(lst->count>0 && lst->size>0 && NULL != lst->data)
//  {
//    EGOBOO_DELETE(lst->data);
//    lst->count = 0;
//    lst->size = 0;
//  }
//
//  EKEY_PINVALIDATE( lst );
//
//  return btrue;
//}
//
////--------------------------------------------------------------------------------------------
//INLINE void  * CList_getData(CList * lst, int index)
//{
//  Uint8 * ptr;
//
//  if(!EKEY_PVALID(lst) || NULL == lst->data) return NULL;
//
//  ptr = (Uint8*)(lst->data);
//
//  ptr += index * lst->size;
//
//  return (void *)ptr;
//}

