#pragma once

#include "object.h"
#include "char.h"

//--------------------------------------------------------------------------------------------
INLINE const CHR_REF team_get_sissy( TEAM_REF iteam )
{
  if ( !VALID_TEAM( iteam ) ) return MAXCHR;

  TeamList[iteam].sissy = VALIDATE_CHR( TeamList[iteam].sissy );
  return TeamList[iteam].sissy;
};

//--------------------------------------------------------------------------------------------
INLINE const CHR_REF team_get_leader( TEAM_REF iteam )
{
  if ( !VALID_TEAM( iteam ) ) return MAXCHR;

  TeamList[iteam].leader = VALIDATE_CHR( TeamList[iteam].leader );
  return TeamList[iteam].leader;
};

//--------------------------------------------------------------------------------------------
INLINE const SLOT grip_to_slot( GRIP g )
{
  SLOT s = SLOT_NONE;

  switch ( g )
  {
    case GRIP_ORIGIN: s = SLOT_NONE; break;
    case GRIP_LAST:   s = SLOT_NONE; break;
    case GRIP_RIGHT:  s = SLOT_RIGHT; break;

      // TODO : differentiate between GRIP_SADDLE and GRIP_LEFT
    case GRIP_LEFT:   s = SLOT_LEFT;  break;
      // case GRIP_SADDLE: s = SLOT_SADDLE; break;

    case GRIP_NONE:   s = SLOT_NONE; break;

    default:
      //try to do this mathematically

      if ( 0 == ( g % GRIP_SIZE ) )
      {
        s = ( g / GRIP_SIZE ) - 1;
        if ( s <  0          ) s = SLOT_NONE;
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
INLINE const GRIP slot_to_grip( SLOT s )
{
  GRIP g = SLOT_NONE;

  switch ( s )
  {
    case SLOT_LEFT:      g = GRIP_LEFT;   break;
    case SLOT_RIGHT:     g = GRIP_RIGHT;  break;
    case SLOT_SADDLE:    g = GRIP_SADDLE; break;

    case SLOT_NONE:      g = GRIP_ORIGIN; break;
    case SLOT_INVENTORY: g = GRIP_INVENTORY; break;

    default:
      //try to do this mathematically

      g = (s + 1) * GRIP_SIZE;
      if ( g > GRIP_RIGHT ) g = GRIP_ORIGIN;
  }

  return g;
};

//--------------------------------------------------------------------------------------------
INLINE const Uint16 slot_to_offset( SLOT s )
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
INLINE const Uint16 slot_to_latch( Uint16 object, SLOT s )
{
  Uint16 latch = LATCHBUTTON_NONE;
  bool_t in_hand = bfalse;

  if ( VALID_CHR( object ) )
    in_hand = chr_using_slot( object, s );

  switch ( s )
  {
    case SLOT_LEFT:  latch = in_hand ? LATCHBUTTON_LEFT  : LATCHBUTTON_ALTLEFT;  break;
    case SLOT_RIGHT: latch = in_hand ? LATCHBUTTON_RIGHT : LATCHBUTTON_ALTRIGHT; break;
  };

  return latch;
};



//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE BData * bdata_new(BData * b)
{
  if(NULL == b) return NULL;

  memset(b, 0, sizeof(BData));
  b->valid = bfalse;

  return b;
};

//--------------------------------------------------------------------------------------------
INLINE bool_t bdata_delete(BData * b)
{
  if(NULL == b || !b->valid) return bfalse;

  FREE(b->cv_tree);
  b->valid = bfalse;

  return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE BData * bdata_renew(BData * b)
{
  if(NULL==b) return NULL;

  bdata_delete(b);
  return bdata_new(b);
}