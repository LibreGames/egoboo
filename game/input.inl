#pragma once

#include "input.h"
#include "network.h"
#include "char.h"
#include "graphic.h"
#include "game.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

typedef struct control_data_t
{
  Uint32 value;             // The scancode or mask
  bool_t is_key;            // Is it a key?
} CONTROL_DATA;

CONTROL_DATA control_list[INPUT_COUNT][CONTROL_COUNT];

//--------------------------------------------------------------------------------------------
INLINE bool_t key_is_pressed( int keycode )
{
  // ZZ> This function returns btrue if the given control is pressed...

  if ( Get_CGui()->net_messagemode )  return bfalse;

  if ( keyb.state )
    return SDLKEYDOWN( keycode );
  else
    return bfalse;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t control_key_is_pressed( CONTROL control )
{
  // ZZ> This function returns btrue if the given control is pressed...

  if ( control_list[INPUT_KEYB][control].is_key )
    return key_is_pressed( control_list[INPUT_KEYB][control].value );
  else
    return bfalse;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t control_mouse_is_pressed( CONTROL control )
{
  // ZZ> This function returns btrue if the given control is pressed...
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
  // ZZ> This function returns btrue if the given control is pressed...

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
INLINE CHR_REF PlaList_get_character( CGame * gs, PLA_REF iplayer )
{
  if ( !VALID_PLA( gs->PlaList, iplayer ) ) return MAXCHR;

  gs->PlaList[iplayer].chr_ref = VALIDATE_CHR( gs->ChrList, gs->PlaList[iplayer].chr_ref );
  return gs->PlaList[iplayer].chr_ref;
};