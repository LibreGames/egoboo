/* Egoboo - egobootypedef.h
 * Defines some basic types that are used throughout the game code.
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

#pragma once

#include "egoboo_config.h"

#include <SDL_endian.h>
#include <SDL_types.h>

typedef struct aa_bbox_t AA_BBOX;

typedef struct rect_sint32_t
{
  Sint32 left;
  Sint32 right;
  Sint32 top;
  Sint32 bottom;
} IRect;

typedef struct rect_float_t
{
  float left;
  float right;
  float top;
  float bottom;
} FRect;


typedef enum bool_e
{
  btrue  = ( 1 == 1 ),
  bfalse = ( !btrue )
} bool_t;


typedef char STRING[256];

//--------------------------------------------------------------------------------------------
typedef Uint32 IDSZ;

#ifndef MAKE_IDSZ
#define MAKE_IDSZ(idsz) ((IDSZ)((((idsz)[0]-'A') << 15) | (((idsz)[1]-'A') << 10) | (((idsz)[2]-'A') << 5) | (((idsz)[3]-'A') << 0)))
#endif


//--------------------------------------------------------------------------------------------
typedef struct pair_t
{
  Sint32 ibase;
  Uint32 irand;
} PAIR;

//--------------------------------------------------------------------------------------------
typedef struct range_t
{
  float ffrom, fto;
} RANGE;

//--------------------------------------------------------------------------------------------
typedef Uint16 CHR_REF;
typedef Uint16 TEAM_REF;
typedef Uint16 PRT_REF;
typedef Uint16 PLA_REF;

//--------------------------------------------------------------------------------------------
typedef union float_int_convert_u 
{ 
  float f; 
  Uint32 i; 
} FCONVERT;

#if SDL_BYTEORDER != SDL_LIL_ENDIAN
    INLINE float SwapLE_float( float val );
#endif

//--------------------------------------------------------------------------------------------
typedef struct bbox_list_t
{
  int       count;
  AA_BBOX * list;
} BBOX_LIST;

INLINE const BBOX_LIST * bbox_list_new(BBOX_LIST * lst);
INLINE const BBOX_LIST * bbox_list_delete(BBOX_LIST * lst);
INLINE const BBOX_LIST * bbox_list_renew(BBOX_LIST * lst);
INLINE const BBOX_LIST * bbox_list_alloc(BBOX_LIST * lst, int count);
INLINE const BBOX_LIST * bbox_list_realloc(BBOX_LIST * lst, int count);

//--------------------------------------------------------------------------------------------
typedef struct bbox_array_t
{
  int         count;
  BBOX_LIST * list;
} BBOX_ARY;

INLINE const BBOX_ARY * bbox_ary_new(BBOX_ARY * ary);
INLINE const BBOX_ARY * bbox_ary_delete(BBOX_ARY * ary);
INLINE const BBOX_ARY * bbox_ary_renew(BBOX_ARY * ary);
INLINE const BBOX_ARY * bbox_ary_alloc(BBOX_ARY * ary, int count);