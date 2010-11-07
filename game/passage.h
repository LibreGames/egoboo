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

/// \file passage.h
/// \details Passages and doors and whatnot.  Things that impede your progress!

#include "egoboo_typedef_cpp.h"

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

    static void    flash( ego_passage * ppass, const Uint8 color );

    /// \author ZF
    /// \details  returns btrue if the specified X and Y coordinates are within the passage
    static bool_t  point_is_in( ego_passage * ppass, const float xpos, const float ypos );

    /// \author ZF
    /// \details  returns btrue if an object of the given radius is PARTIALLY inside the passage
    static bool_t  object_is_in( ego_passage * ppass, const float xpos, const float ypos, const float radius );

    /// \author ZZ
    /// \details  searches for a specified object in a passage
    static CHR_REF who_is_blocking( ego_passage * ppass, const CHR_REF & isrc, IDSZ idsz, BIT_FIELD targeting_bits, IDSZ require_item );

    static bool_t  check_music( ego_passage * ppass );

    /// \author ZZ
    /// \details  makes a passage impassable, and returns btrue if it isn't blocked
    static bool_t  close( ego_passage * ppass );
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct passage_stack : public  t_stack< ego_passage, MAX_PASS >
{
    /// \author ZF
    /// \details  This function changes the music if, if needed
    void     check_music();

    /// \author ZZ
    /// \details  This function creates a passage area
    void     add_one( ego_passage * pdata );

    /// \author ZZ
    /// \details  This function makes a passage passable
    bool_t   open( const PASS_REF & ipassage );

    /// \author ZZ
    /// \details  This function makes a passage impassable
    bool_t   close_one( const PASS_REF & ipassage );

    /// \author ZZ
    /// \details  makes a passage flash white
    void     flash( const PASS_REF & ipassage, const Uint8 color );

    /// \author ZZ
    /// \details  Is any object "blocking" this passage
    CHR_REF  who_is_blocking( const PASS_REF & passage, const CHR_REF & isrc, const IDSZ idsz, const BIT_FIELD targeting_bits, const IDSZ require_item );

    /// \author ZZ
    /// \details  Is a point inside a passage
    bool_t   point_is_inside( const PASS_REF & ipassage, const float xpos, const float ypos );

    /// \author ZZ
    /// \details  Is an object completely inside a passage
    bool_t   object_is_inside( const PASS_REF & ipassage, const float xpos, const float ypos, const float radius );

    void     free_all();

protected:

    int  get_free();
};

extern passage_stack PassageStack;

#define VALID_PASSAGE( IPASS )       ( PassageStack.in_range_ref( IPASS ) && ((IPASS) <  PassageStack.count) )
#define INVALID_PASSAGE( IPASS )     ( !VALID_PASSAGE( IPASS ) )

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// The data defining a shop
struct ego_shop
{
    PASS_REF passage;  ///< The passage number
    CHR_REF  owner;    ///< Who gets the gold?
};

struct shop_stack : public t_stack< ego_shop, MAX_SHOP >
{
    void     free_all();
    int      get_free();

    void     add_one( const CHR_REF & owner, const PASS_REF & ipassage );
    CHR_REF  find_owner( const int ix, const int iy );
};

extern shop_stack ShopStack;

#define INVALID_SHOP( ISHOP )     ( !ShopStack.in_range_ref( ISHOP ) )

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// prototypes

/// \author ZZ
// \details  This function clears the passage list ( for doors )
void   passage_system_clear();

/// \author ZZ
/// \details  This function reads the passage file
void   activate_passages_file_vfs();
