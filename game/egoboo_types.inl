#pragma once

#include "egoboo_config.h"
#include "egoboo_types.h"

#ifndef INLINE
#error
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE const BBOX_LIST * bbox_list_new(BBOX_LIST * lst)
{
  if(NULL == lst) return NULL;
  memset(lst, 0, sizeof(BBOX_LIST));
  return lst;
}

//--------------------------------------------------------------------------------------------
INLINE const BBOX_LIST * bbox_list_delete(BBOX_LIST * lst)
{
  if(NULL == lst) return NULL;

  if( lst->count > 0 )
  {
    FREE(lst->list);
  }

  lst->count = 0;
  lst->list  = NULL;

  return lst;
}

//--------------------------------------------------------------------------------------------
INLINE const BBOX_LIST * bbox_list_renew(BBOX_LIST * lst)
{
  if(NULL == lst) return NULL;

  bbox_list_delete(lst);
  return bbox_list_new(lst);
}

//--------------------------------------------------------------------------------------------
INLINE const BBOX_LIST * bbox_list_alloc(BBOX_LIST * lst, int count)
{
  if(NULL == lst) return NULL;

  bbox_list_delete(lst);

  if(count>0)
  {
    lst->list = (AA_BBOX*)calloc(count, sizeof(AA_BBOX));
    if(NULL != lst->list)
    {
      lst->count = count;
    }
  }

  return lst;
}

//--------------------------------------------------------------------------------------------
INLINE const BBOX_LIST * bbox_list_realloc(BBOX_LIST * lst, int count)
{
  // check for bad list
  if(NULL == lst) return NULL;

  // check for no change in the count
  if(count == lst->count) return lst;

  // check another dumb case
  if(count==0)
  {
    return bbox_list_delete(lst);
  }


  lst->list = (AA_BBOX *)realloc(lst->list, count * sizeof(AA_BBOX));
  if(NULL == lst->list)
  {
    lst->count = 0;
  }
  else
  {
    lst->count = count;
  }


  return lst;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE const BBOX_ARY * bbox_ary_new(BBOX_ARY * ary)
{
  if(NULL == ary) return NULL;
  memset(ary, 0, sizeof(BBOX_ARY));
  return ary;
}

//--------------------------------------------------------------------------------------------
INLINE const BBOX_ARY * bbox_ary_delete(BBOX_ARY * ary)
{
  int i;

  if(NULL == ary) return NULL;

  if(NULL!=ary->list)
  {
    for(i=0; i<ary->count; i++)
    {
      bbox_list_delete(ary->list + i);
    }

    FREE(ary->list);
  }

  ary->count = 0;
  ary->list = NULL;

  return ary;
}

//--------------------------------------------------------------------------------------------
INLINE const BBOX_ARY * bbox_ary_renew(BBOX_ARY * ary)
{
  if(NULL == ary) return NULL;
  bbox_ary_delete(ary);
  return bbox_ary_new(ary);
}

//--------------------------------------------------------------------------------------------
INLINE const BBOX_ARY * bbox_ary_alloc(BBOX_ARY * ary, int count)
{
  if(NULL == ary) return NULL;

  bbox_ary_delete(ary);

  if(count>0)
  {
    ary->list = (BBOX_LIST*)calloc(count, sizeof(BBOX_LIST));
    if(NULL != ary->list)
    {
      ary->count = count;
    }
  }

  return ary;
}

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

