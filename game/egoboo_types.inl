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
  if(!n->initialized) return btrue;

  n->data        = NULL;
  n->initialized = bfalse;
  
  return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE HashNode * HashNode_new(HashNode * n, void * data)
{
  if(NULL == n) return n;
  if(n->initialized) HashNode_delete(n);

  memset(n, 0, sizeof(HashNode));
  n->data = data;
  n->initialized = btrue;

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
  if(NULL == n) return lst;
  n->next = NULL;

  if(lst == NULL) return n;

  n->next = n->next;
  lst->next = n;

  return lst;
}

//--------------------------------------------------------------------------------------------
INLINE HashNode * HashNode_insert_before(HashNode lst[], HashNode * n)
{
  if(NULL == n) return lst;
  n->next = NULL;

  if(lst == NULL) return n;

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

