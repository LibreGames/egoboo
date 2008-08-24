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

#include "input.h"
#include "Network.h"
#include "game.h"

#include "graphic.inl"
#include "char.inl"
#include "egoboo_types.inl"

extern KeyboardBuffer_t _keybuff;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_control_data
{
  Uint32 value;             // The scancode or mask
  bool_t is_key;            // Is it a key?
};

typedef struct s_control_data CONTROL_DATA;

CONTROL_DATA control_list[INPUT_COUNT][CONTROL_COUNT];

//--------------------------------------------------------------------------------------------
INLINE bool_t key_is_pressed( int keycode )
{
  /// @details ZZ@> This function returns btrue if the given control is pressed...

  if ( keyb.mode )  return bfalse;

  if ( keyb.state )
    return SDLKEYDOWN( keycode );
  else
    return bfalse;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t control_key_is_pressed( CONTROL control )
{
  /// @details ZZ@> This function returns btrue if the given control is pressed...

  if ( control_list[INPUT_KEYB][control].is_key )
    return key_is_pressed( control_list[INPUT_KEYB][control].value );
  else
    return bfalse;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t control_mouse_is_pressed( CONTROL control )
{
  /// @details ZZ@> This function returns btrue if the given control is pressed...
  bool_t retval = bfalse;

  if ( control_list[INPUT_MOUS][control].is_key )
  {
    retval = key_is_pressed( control_list[INPUT_MOUS][control].value );
  }
  else
  {
    retval = ( mous.latch.b == control_list[INPUT_MOUS][control].value );
  }

  return retval;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t control_joy_is_pressed( int joy_num, CONTROL control )
{
  /// @details ZZ@> This function returns btrue if the given control is pressed...

  INPUT_TYPE it;
  bool_t retval = bfalse;

  it = (joy_num == 0) ? INPUT_JOYA : INPUT_JOYB;

  if ( control_list[it][control].is_key )
  {
    retval = key_is_pressed( control_list[it][control].value );
  }
  else
  {
    retval = ( joy[joy_num].latch.b == control_list[it][control].value );
  }

  return retval;
}

//--------------------------------------------------------------------------------------------
INLINE CHR_REF PlaList_getRChr( Game_t * gs, PLA_REF iplayer )
{
  if ( !VALID_PLA( gs->PlaList, iplayer ) ) return INVALID_CHR;

  gs->PlaList[iplayer].chr_ref = VALIDATE_CHR( gs->ChrList, gs->PlaList[iplayer].chr_ref );
  return gs->PlaList[iplayer].chr_ref;
};
