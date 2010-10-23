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

/// @file passage.h
/// @Passages and doors and whatnot.  Things that impede your progress!

#include "egoboo_typedef.h"

#include "file_formats/passage_file.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct ego_script_state;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define MAX_PASS             256                     ///< Maximum number of passages ( mul 32 )
#define MAX_SHOP             MAX_PASS
#define CLOSETOLERANCE       2                       ///< For closing doors

#define SHOP_NOOWNER 0xFFFF        ///< Shop has no owner
#define SHOP_STOLEN  0xFFFF        ///< Someone stole a item

/// The pre-defined orders for communicating with shopkeepers
enum e_shop_orders
{
    SHOP_BUY       = 0,
    SHOP_SELL,
    SHOP_NOAFFORD,
    SHOP_THEFT
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// Passages

struct ego_passage : public s_passage_data
{
    static ego_passage * init( ego_passage * ppass, passage_data_t * pdata );
    static bool_t  do_open( ego_passage * ppass );
    static void    flash( ego_passage * ppass, Uint8 color );
    static bool_t  point_is_in( ego_passage * ppass, float xpos, float ypos );
    static bool_t  object_is_in( ego_passage * ppass, float xpos, float ypos, float radius );
    static CHR_REF who_is_blocking( ego_passage * ppass, const CHR_REF & isrc, IDSZ idsz, BIT_FIELD targeting_bits, IDSZ require_item );
    static bool_t  check_music( ego_passage * ppass );
    static bool_t  close( ego_passage * ppass );
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

extern t_cpp_stack< ego_passage, MAX_PASS  > PassageStack;

#define VALID_PASSAGE_RANGE( IPASS ) ( ((IPASS) >= 0) && ((IPASS) <   MAX_PASS) )
#define VALID_PASSAGE( IPASS )       ( VALID_PASSAGE_RANGE( IPASS ) && ((IPASS) <  PassageStack.count) )
#define INVALID_PASSAGE( IPASS )     ( !VALID_PASSAGE( IPASS ) )

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// The data defining a shop
struct ego_shop
{
    PASS_REF passage;  ///< The passage number
    CHR_REF  owner;    ///< Who gets the gold?
};

extern t_cpp_stack< ego_shop, MAX_SHOP  > ShopStack;

#define VALID_SHOP_RANGE( ISHOP ) ( ((ISHOP) >= 0) && ((ISHOP) <   MAX_SHOP) )
#define VALID_SHOP( ISHOP )       ( VALID_SHOP_RANGE( ISHOP ) && ((ISHOP) <  ShopStack.count) )
#define INVALID_SHOP( ISHOP )     ( !VALID_SHOP( ISHOP ) )

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// prototypes
void   passage_system_clear();

void   activate_passages_file_vfs();

void     PassageStack_check_music();
void     PassageStack_add_one( ego_passage * pdata );
bool_t   PassageStack_open( const PASS_REF & ipassage );
bool_t   PassageStack_close_one( const PASS_REF & ipassage );
void     PassageStack_flash( const PASS_REF & ipassage, Uint8 color );
CHR_REF  PassageStack_who_is_blocking( const PASS_REF & passage, const CHR_REF & isrc, IDSZ idsz, BIT_FIELD targeting_bits, IDSZ require_item );
bool_t   PassageStack_point_is_inside( const PASS_REF & ipassage, float xpos, float ypos );
bool_t   PassageStack_object_is_inside( const PASS_REF & ipassage, float xpos, float ypos, float radius );

void     ShopStack_add_one( const CHR_REF & owner, const PASS_REF & ipassage );
CHR_REF  ShopStack_find_owner( int ix, int iy );
