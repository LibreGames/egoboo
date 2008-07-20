#pragma once

#include "egoboo_config.h"
#include "egoboo_types.h"
#include "egoboo_math.inl"

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
INLINE bool_t HashNode_delete(HashNode * n)
{
  if(NULL == n) return bfalse;
  if( !EKEY_PVALID( n ) ) return btrue;

  n->data        = NULL;
  egoboo_key_invalidate(&(n->ekey));
  
  return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE HashNode * HashNode_new(HashNode * n, void * data)
{
  if(NULL == n) return n;
  if( !EKEY_PVALID( n ) ) HashNode_delete(n);

  memset(n, 0, sizeof(HashNode));

  EKEY_PNEW( n, HashNode );  
  
  n->data = data;

  return n;
}

//--------------------------------------------------------------------------------------------
INLINE HashNode * HashNode_create(void * data)
{
  HashNode * n = calloc(1, sizeof(HashNode));
  if(NULL == n) return n;

  return HashNode_new(n, data);
}

//--------------------------------------------------------------------------------------------
INLINE bool_t HashNode_destroy(HashNode ** pn)
{
  bool_t retval = bfalse;

  if(NULL == pn || NULL == *pn) return bfalse;

  retval = HashNode_delete(*pn);

  FREE(*pn);

  return retval;
}

//--------------------------------------------------------------------------------------------
INLINE HashNode * HashNode_insert_after(HashNode lst[], HashNode * n)
{
  if( NULL == n ) return lst;
  n->next = NULL;

  if( NULL == lst ) return n;

  n->next = n->next;
  lst->next = n;

  return lst;
}

//--------------------------------------------------------------------------------------------
INLINE HashNode * HashNode_insert_before(HashNode lst[], HashNode * n)
{
  if( NULL == n ) return lst;
  n->next = NULL;

  if( NULL == lst ) return n;

  n->next = lst;

  return n;
}

//--------------------------------------------------------------------------------------------
INLINE HashNode * HashNode_remove_after(HashNode lst[])
{
  HashNode * n;

  if(NULL == lst) return NULL;

  n = lst->next;
  if(NULL == n) return lst;

  lst->next = n->next;
  n->next   = NULL;

  return lst;
}

//--------------------------------------------------------------------------------------------
INLINE HashNode * HashNode_remove(HashNode lst[])
{
  HashNode * n;

  if(NULL == lst) return NULL;

  n = lst->next;
  if(NULL == n) return NULL;

  lst->next = NULL;

  return n;
}


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE bool_t HashList_deallocate(HashList * lst)
{
  if(NULL == lst) return bfalse;
  if(0 == lst->allocated) return btrue;

  FREE(lst->subcount);
  FREE(lst->sublist);
  lst->allocated = 0;

  return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t HashList_allocate(HashList * lst, int size)
{
  if(NULL == lst) return bfalse;

  HashList_deallocate(lst);

  lst->subcount = calloc(size, sizeof(int));
  if(NULL == lst->subcount)
  {
    return bfalse;
  }

  lst->sublist = calloc(size, sizeof(HashNode *));
  if(NULL == lst->sublist)
  {
    FREE(lst->subcount);
    return bfalse;
  }

  lst->allocated = size;

  return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE HashList * HashList_new(HashList * lst, int size)
{
  if(NULL == lst) return NULL;

  if(size<0) size = 256;
  HashList_allocate(lst, size);

  return lst;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t     HashList_delete(HashList * lst)
{
  if(NULL == lst) return bfalse;

  HashList_deallocate(lst);
  
  return btrue;
}
//--------------------------------------------------------------------------------------------
INLINE HashList * HashList_create(int size)
{
  HashList * lst = calloc(1, sizeof(HashList));
  if(NULL == lst) return lst;

  return HashList_new(lst, size);
}

//--------------------------------------------------------------------------------------------
INLINE bool_t HashList_destroy(HashList ** plst)
{
  bool_t retval = bfalse;

  if(NULL == plst || NULL == *plst) return bfalse;

  retval = HashList_delete(*plst);

  FREE(*plst);

  return retval;
}



//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE egoboo_key * egoboo_key_create(Uint32 itype, void * pdata)
{
  // BB > dynamically allocate and initialize a new key

  egoboo_key * ptmp, * pkey = calloc(1, sizeof(egoboo_key));
  if(NULL == pkey) return pkey;

  // initialize the key
  ptmp = egoboo_key_new(pkey, itype, pdata);
  if(NULL == ptmp)
  {
    FREE(pkey);
  }

  // let everyone know that this data is on the heap, not the stack
  pkey->dynamic = btrue;

  return pkey;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t egoboo_key_destroy(egoboo_key ** ppkey)
{
  // BB > de-initialize the key and free it if necessary

  if( NULL == ppkey || NULL == *ppkey) return bfalse;
  
  if( !egoboo_key_invalidate( *ppkey ) ) return bfalse;

  // do the actual freeing. make sure never to free memory that is on the stack.
  if((*ppkey)->dynamic)
  {
    FREE(*ppkey);
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE egoboo_key * egoboo_key_new(egoboo_key * pkey, Uint32 itype, void * pdata)
{
  // BB > initialize the key

  if(NULL == pkey) return pkey;

  // initialize the key. pkey->dynamic is automatically set to bfalse
  memset(pkey, 0, sizeof(egoboo_key));

  // set the proper values
  if( egoboo_key_validate(pkey) )
  {
    pkey->data_type = itype;
    pkey->pdata     = pdata;
  }

  return pkey;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t egoboo_key_validate(egoboo_key * pkey)
{
  // BB > de-initialize the key

  static Uint32 new_id = 0;

  if(NULL == pkey) return bfalse;

  if( egoboo_key_valid(pkey) ) return btrue;

  // make sure that the key is invalid
  pkey->v1 = ego_rand(&new_id);
  pkey->v2 = ego_rand(&new_id);

  return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t egoboo_key_invalidate(egoboo_key * pkey)
{
  // BB > de-initialize the key

  if(NULL == pkey) return bfalse;

  if( !egoboo_key_valid(pkey) ) return bfalse;

  // make sure that the key is invalid
  pkey->v1 = pkey->v2 = 0;

  return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t egoboo_key_valid(egoboo_key * pkey)
{
  // BB > verify that a "key" is valid.

  Uint32 seed;
  bool_t retval;

  if(NULL == pkey) return bfalse;

  seed = pkey->v1;
  retval = (ego_rand(&seed) == pkey->v2);

  if(!retval)
  {
    int i=0;
  }

  return retval;
}

//--------------------------------------------------------------------------------------------
INLINE void * egoboo_key_get_data(egoboo_key * pkey, Uint32 type)
{
  // BB > grab the data associated with the key, if and only if the key is valid and the
  //      data type matches the key data

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
INLINE BSP_node * BSP_node_new( BSP_node * n, void * data )
{
  if(NULL == n) return n;

  BSP_leaf_delete( n );

  memset(n, 0, sizeof(BSP_node));

  if(NULL == data) return n;

  n->data = data;

  EKEY_PNEW( n, BSP_node );

  return n;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t BSP_node_delete( BSP_node * n )
{
  bool_t retval;

  if( NULL == n ) return bfalse;
  if( !EKEY_PVALID(n) ) return btrue;

  n->data = NULL;

  EKEY_PINVALIDATE( n );

  return retval;
}


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE BSP_leaf * BSP_leaf_new( BSP_leaf * L, int count )
{
  if(NULL == L) return L;

  BSP_leaf_delete( L );

  memset(L, 0, sizeof(BSP_leaf));

  L->children = calloc(count, sizeof(BSP_leaf*));
  if(NULL != L->children)
  {
    L->child_count = count;
  }

  EKEY_PNEW( L, BSP_leaf );

  return L;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t BSP_leaf_delete( BSP_leaf * L )
{
  bool_t retval;

  if( NULL == L ) return bfalse;
  if( !EKEY_PVALID(L) ) return btrue;

  FREE(L->children);
  L->child_count = 0;

  EKEY_PINVALIDATE( L );

  return retval;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t BSP_leaf_insert( BSP_leaf * L, BSP_node * n )
{
  if( !EKEY_PVALID(L) || !EKEY_PVALID(n) ) return bfalse;

  n->next  = L->nodes;
  L->nodes = n;

  return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE bool_t BSP_tree_allocate( BSP_tree * t, size_t count, size_t children )
{
  int i;

  if(NULL == t) return bfalse;
  if(NULL != t->leaf_list || t->leaf_count > 0) return bfalse;

  // allocate the nodes
  t->leaf_list = calloc(count, sizeof(BSP_leaf));
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
INLINE bool_t BSP_tree_deallocate( BSP_tree * t )
{
  int i;

  if( NULL == t ) return bfalse;
  if( !EKEY_PVALID(t) ) return btrue;
  if( NULL == t->leaf_list || 0 == t->leaf_count ) return btrue;

  // delete the nodes
  for(i=0; i<t->leaf_count; i++)
  {
    BSP_leaf_delete(t->leaf_list + i);
  }

  FREE(t->leaf_list);
  t->leaf_count = 0;

  return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t BSP_tree_construct(BSP_tree * t)
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
INLINE BSP_tree * BSP_tree_new( BSP_tree * t, Sint32 dim, Sint32 depth)
{
  Sint32 count, children;

  if(NULL == t) return t;

  BSP_tree_delete( t );

  memset(t, 0, sizeof(BSP_tree));

  count = BSP_tree_count_nodes(dim, depth);
  if(count < 0) return t;

  children = 2 << dim;
  if( !BSP_tree_allocate(t, count, children) ) return t;

  t->dimensions = dim;
  t->depth      = depth;

  EKEY_PNEW( t, BSP_tree );

  BSP_tree_construct(t);

  return t;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t BSP_tree_delete( BSP_tree * t )
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
INLINE bool_t BSP_tree_insert( BSP_tree * t, BSP_leaf * L, BSP_node * n, int index )
{
  if( !EKEY_PVALID(t) || !EKEY_PVALID(L) || !EKEY_PVALID(n) ) return bfalse;
  if(index > 0 || index > L->child_count) return bfalse;

  if( index >= 0 && NULL != L->children[index])
  {
    // inserting a node into the child
    return BSP_leaf_insert(L->children + index, n);
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
    // BSP_leaf from the free list in the BSP_tree structure an insert it in
    // this child node

    L->children[index] = t->leaf_list;
    t->leaf_list = t->leaf_list->parent;           // parent is doubling for "next"

    BSP_leaf_new( L->children[index], 2 << t->dimensions );

    return BSP_leaf_insert(L->children + index, n);
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
//    FREE(lst->data);
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

