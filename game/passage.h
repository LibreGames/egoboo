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
/// @brief Egoboo Passage and Shop Definitions

#include "egoboo_types.h"

struct sGame;

#define PASSLST_COUNT             256                      ///< Maximum number of passages ( mul 32 )
#define NOOWNER             INVALID_CHR

//Passages
struct sPassage
{
  egoboo_key_t ekey;

  IRect_t   area;            ///< Passage_t positions
  int     music;           ///< Music track appointed to the specific passage
  Uint32  mask;
  bool_t  open;            ///< Is the passage open?
  CHR_REF owner;           ///< Who controls the passage?
};
typedef struct sPassage Passage_t;

Passage_t * Passage_new(Passage_t *ppass);
bool_t      Passage_delete(Passage_t *ppass);
Passage_t * Passage_renew(Passage_t *ppass);

Uint32 PassList_add( struct sGame * gs,  int tlx, int tly, int brx, int bry, bool_t open, Uint32 mask );
void   PassList_load( struct sGame * gs, char *modname );

struct sShop
{
  egoboo_key_t ekey;
  PASS_REF passage;  ///< The passage number
  CHR_REF  owner;    ///< Who gets the gold?
};
typedef struct sShop Shop_t;

Shop_t * Shop_new(Shop_t *pshop);
bool_t   Shop_delete(Shop_t *pshop);
Shop_t * Shop_renew(Shop_t *pshop);

Uint32 ShopList_add( struct sGame * gs,  CHR_REF owner, PASS_REF passage );

// Passage control functions
bool_t  passage_open( struct sGame * gs,  PASS_REF passage );
bool_t  passage_close( struct sGame * gs,  Uint32 passage );

void    passage_check_music(struct sGame * gs );
void    passage_flash( struct sGame * gs,  PASS_REF passage, Uint8 color );

CHR_REF passage_search_blocking( struct sGame * gs,  PASS_REF passage );
CHR_REF passage_search_blocking_ID( struct sGame * gs,  PASS_REF passage, IDSZ idsz );
bool_t  passage_search_tile( struct sGame * gs, PASS_REF passage, Uint32 tiletype, Sint32 tmpx, Sint32 tmpy, Sint32 * pix, Sint32 * piy );
bool_t  passage_break_tiles( struct sGame * gs, PASS_REF passage, Uint16 starttile, Uint16 frames, Uint16 become, Uint32 meshfxor, Sint32 *pix, Sint32 *piy );

bool_t  passage_check    ( struct sGame * gs, CHR_REF ichr, PASS_REF pass, Uint16 * powner );
bool_t  passage_check_any( struct sGame * gs, CHR_REF ichr, PASS_REF pass, Uint16 * powner );
bool_t  passage_check_all( struct sGame * gs, CHR_REF ichr, PASS_REF pass, Uint16 * powner );


