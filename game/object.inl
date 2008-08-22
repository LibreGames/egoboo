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

#include "object.h"
#include "game.h"

#include "char.inl"
#include "egoboo_types.inl"

//--------------------------------------------------------------------------------------------
INLINE CHR_REF team_get_sissy( Game_t * gs, TEAM_REF iteam )
{
  if ( !VALID_TEAM_RANGE( iteam ) ) return INVALID_CHR;

  gs->TeamList[iteam].sissy = VALIDATE_CHR( gs->ChrList, gs->TeamList[iteam].sissy );
  return gs->TeamList[iteam].sissy;
};

//--------------------------------------------------------------------------------------------
INLINE CHR_REF team_get_leader( Game_t * gs, TEAM_REF iteam )
{
  if ( !VALID_TEAM_RANGE( iteam ) ) return INVALID_CHR;

  gs->TeamList[iteam].leader = VALIDATE_CHR( gs->ChrList, gs->TeamList[iteam].leader );
  return gs->TeamList[iteam].leader;
};

//--------------------------------------------------------------------------------------------
INLINE EGO_CONST SLOT grip_to_slot( GRIP g )
{
  SLOT s = SLOT_NONE;

  switch ( g )
  {
    case GRIP_ORIGIN: s = SLOT_NONE; break;
    case GRIP_LAST:   s = SLOT_NONE; break;
    case GRIP_RIGHT:  s = SLOT_RIGHT; break;

      /// @todo  differentiate between GRIP_SADDLE and GRIP_LEFT
    case GRIP_LEFT:   s = SLOT_LEFT;  break;
      // case GRIP_SADDLE: s = SLOT_SADDLE; break;

    case GRIP_NONE:   s = SLOT_NONE; break;

    default:
      //try to do this mathematically

      if ( 0 == ( g % GRIP_SIZE ) )
      {
        s = (SLOT)(( g / GRIP_SIZE ) - 1);
        /* if ( s <  0          ) s = SLOT_NONE; */
        if ( s >= SLOT_COUNT ) s = SLOT_NONE;
      }
      else
      {
        s = SLOT_NONE;
      }

      break;
  };

  return s;
};


//--------------------------------------------------------------------------------------------
INLINE EGO_CONST GRIP slot_to_grip( SLOT s )
{
  GRIP g = GRIP_ORIGIN;

  switch ( s )
  {
    case SLOT_LEFT:      g = GRIP_LEFT;   break;
    case SLOT_RIGHT:     g = GRIP_RIGHT;  break;
    case SLOT_SADDLE:    g = GRIP_SADDLE; break;

    case SLOT_NONE:      g = GRIP_ORIGIN; break;
    case SLOT_INVENTORY: g = GRIP_INVENTORY; break;

    default:
      //try to do this mathematically

      g = (GRIP)((s + 1) * GRIP_SIZE);
      if ( g > GRIP_RIGHT ) g = GRIP_ORIGIN;
  }

  return g;
};

//--------------------------------------------------------------------------------------------
INLINE EGO_CONST Uint16 slot_to_offset( SLOT s )
{
  Uint16 o = 1;

  switch ( s )
  {
    case SLOT_LEFT:      o = GRIP_LEFT;   break;
    case SLOT_RIGHT:     o = GRIP_RIGHT;  break;
    case SLOT_SADDLE:    o = GRIP_SADDLE; break;

    case SLOT_NONE:      o = GRIP_LAST; break;
    case SLOT_INVENTORY: o = GRIP_LAST; break;

    default:
      //try to do this mathematically
      o = s * GRIP_SIZE + GRIP_SIZE;
      if ( o > GRIP_RIGHT ) o = GRIP_LAST;
  }

  return o;
};

//--------------------------------------------------------------------------------------------
INLINE EGO_CONST Uint16 slot_to_latch( PChr_t lst, size_t count, CHR_REF object, SLOT s )
{
  Uint16 latch = LATCHBUTTON_NONE;
  bool_t in_hand = bfalse;

  if ( ACTIVE_CHR( lst, object ) )
    in_hand = chr_using_slot( lst, count, object, s );

  switch ( s )
  {
    case SLOT_LEFT:  latch = in_hand ? LATCHBUTTON_LEFT  : LATCHBUTTON_ALTLEFT;  break;
    case SLOT_RIGHT: latch = in_hand ? LATCHBUTTON_RIGHT : LATCHBUTTON_ALTRIGHT; break;
    default: /* do nothing */ break;
  };

  return latch;
};



//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE BData_t * BData_new(BData_t * b)
{
  if(NULL == b) return NULL;

  memset(b, 0, sizeof(BData_t));
  b->valid = bfalse;

  return b;
};

//--------------------------------------------------------------------------------------------
INLINE bool_t BData_delete(BData_t * b)
{
  if(NULL == b || !b->valid) return bfalse;

  EGOBOO_DELETE(b->cv_tree);
  b->valid = bfalse;

  return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE BData_t * BData_renew(BData_t * b)
{
  if(NULL ==b) return NULL;

  BData_delete(b);
  return BData_new(b);
}
