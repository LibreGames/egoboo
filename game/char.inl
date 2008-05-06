#pragma once

#include "char.h"

#include <assert.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------


INLINE const bool_t chr_in_pack( CHR_REF character );
INLINE const bool_t chr_attached( CHR_REF character );
INLINE const bool_t chr_has_inventory( CHR_REF character );
INLINE const bool_t chr_is_invisible( CHR_REF character );
INLINE const bool_t chr_using_slot( CHR_REF character, SLOT slot );

INLINE const CHR_REF chr_get_nextinpack( CHR_REF ichr );
INLINE const CHR_REF chr_get_onwhichplatform( CHR_REF ichr );
INLINE const CHR_REF chr_get_inwhichpack( CHR_REF ichr );
INLINE const CHR_REF chr_get_attachedto( CHR_REF ichr );
INLINE const CHR_REF chr_get_holdingwhich( CHR_REF ichr, SLOT slot );

INLINE const CHR_REF chr_get_aitarget( CHR_REF ichr );
INLINE const CHR_REF chr_get_aiowner( CHR_REF ichr );
INLINE const CHR_REF chr_get_aichild( CHR_REF ichr );
INLINE const CHR_REF chr_get_aiattacklast( CHR_REF ichr );
INLINE const CHR_REF chr_get_aibumplast( CHR_REF ichr );
INLINE const CHR_REF chr_get_aihitlast( CHR_REF ichr );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

INLINE const bool_t chr_attached( CHR_REF chr_ref )
{
  if ( !VALID_CHR( chr_ref ) ) return bfalse;

  ChrList[chr_ref].attachedto = VALIDATE_CHR( ChrList[chr_ref].attachedto );
  if(!VALID_CHR(chr_ref)) ChrList[chr_ref].inwhichslot = SLOT_NONE;

  return VALID_CHR( ChrList[chr_ref].attachedto );
};

//--------------------------------------------------------------------------------------------
INLINE const bool_t chr_in_pack( CHR_REF chr_ref )
{
  CHR_REF inwhichpack = chr_get_inwhichpack( chr_ref );
  return VALID_CHR( inwhichpack );
}

//--------------------------------------------------------------------------------------------
INLINE const bool_t chr_has_inventory( CHR_REF chr_ref )
{
  bool_t retval = bfalse;
  CHR_REF nextinpack = chr_get_nextinpack( chr_ref );

  if ( VALID_CHR( nextinpack ) )
  {
    retval = btrue;
  }
#if defined(_DEBUG) || !defined(NDEBUG)
  else
  {
    assert( ChrList[chr_ref].numinpack == 0 );
  }
#endif

  return retval;
};

//--------------------------------------------------------------------------------------------
INLINE const bool_t chr_is_invisible( CHR_REF chr_ref )
{
  if ( !VALID_CHR( chr_ref ) ) return btrue;

  return FP8_MUL( ChrList[chr_ref].alpha_fp8, ChrList[chr_ref].light_fp8 ) <= INVISIBLE;
};

//--------------------------------------------------------------------------------------------
INLINE const bool_t chr_using_slot( CHR_REF chr_ref, SLOT slot )
{
  CHR_REF inslot = chr_get_holdingwhich( chr_ref, slot );

  return VALID_CHR( inslot );
};


//--------------------------------------------------------------------------------------------
INLINE const CHR_REF chr_get_nextinpack( CHR_REF chr_ref )
{
  CHR_REF nextinpack = MAXCHR;

  if ( !VALID_CHR( chr_ref ) ) return MAXCHR;

#if defined(_DEBUG) || !defined(NDEBUG)
  nextinpack = ChrList[chr_ref].nextinpack;
  if ( MAXCHR != nextinpack && !ChrList[chr_ref].on )
  {
    // this is an invalid configuration that may indicate a corrupted list
    nextinpack = ChrList[nextinpack].nextinpack;
    if ( VALID_CHR( nextinpack ) )
    {
      // the list is definitely corrupted
      assert( bfalse );
    }
  }
#endif

  ChrList[chr_ref].nextinpack = VALIDATE_CHR( ChrList[chr_ref].nextinpack );
  return ChrList[chr_ref].nextinpack;
};

//--------------------------------------------------------------------------------------------
INLINE const CHR_REF chr_get_onwhichplatform( CHR_REF chr_ref )
{
  if ( !VALID_CHR( chr_ref ) ) return MAXCHR;

  ChrList[chr_ref].onwhichplatform = VALIDATE_CHR( ChrList[chr_ref].onwhichplatform );
  return ChrList[chr_ref].onwhichplatform;
};

//--------------------------------------------------------------------------------------------
INLINE const CHR_REF chr_get_holdingwhich( CHR_REF chr_ref, SLOT slot )
{
  CHR_REF inslot;

  if ( !VALID_CHR( chr_ref ) || slot >= SLOT_COUNT ) return MAXCHR;

#if defined(_DEBUG) || !defined(NDEBUG)
  inslot = ChrList[chr_ref].holdingwhich[slot];
  if ( MAXCHR != inslot )
  {
    CHR_REF holder = ChrList[inslot].attachedto;

    if ( chr_ref != holder )
    {
      // invalid configuration
      assert( bfalse );
    }
  };
#endif

  ChrList[chr_ref].holdingwhich[slot] = VALIDATE_CHR( ChrList[chr_ref].holdingwhich[slot] );
  return ChrList[chr_ref].holdingwhich[slot];
};

//--------------------------------------------------------------------------------------------
INLINE const CHR_REF chr_get_inwhichpack( CHR_REF chr_ref )
{
  if ( !VALID_CHR( chr_ref ) ) return MAXCHR;

  ChrList[chr_ref].inwhichpack = VALIDATE_CHR( ChrList[chr_ref].inwhichpack );
  return ChrList[chr_ref].inwhichpack;
};

//--------------------------------------------------------------------------------------------
INLINE const CHR_REF chr_get_attachedto( CHR_REF chr_ref )
{
  CHR_REF holder;

  if ( !VALID_CHR( chr_ref ) ) return MAXCHR;

#if defined(_DEBUG) || !defined(NDEBUG)

  if( MAXCHR != ChrList[chr_ref].attachedto )
  {
    SLOT slot = ChrList[chr_ref].inwhichslot;
    if(slot != SLOT_INVENTORY)
    {
      assert(SLOT_NONE != slot);
      holder = ChrList[chr_ref].attachedto;
      assert( ChrList[holder].holdingwhich[slot] == chr_ref );
    };
  }
  else
  {
    assert(SLOT_NONE == ChrList[chr_ref].inwhichslot);
  };
#endif

  ChrList[chr_ref].attachedto = VALIDATE_CHR( ChrList[chr_ref].attachedto );
  if( !VALID_CHR( ChrList[chr_ref].attachedto ) ) ChrList[chr_ref].inwhichslot = SLOT_NONE;
  return ChrList[chr_ref].attachedto;
};

//--------------------------------------------------------------------------------------------
INLINE const CHR_REF chr_get_aitarget( CHR_REF chr_ref )
{
  if ( !VALID_CHR( chr_ref ) ) return MAXCHR;

  ChrList[chr_ref].aistate.target = VALIDATE_CHR( ChrList[chr_ref].aistate.target );
  return ChrList[chr_ref].aistate.target;
};

//--------------------------------------------------------------------------------------------
INLINE const CHR_REF chr_get_aiowner( CHR_REF chr_ref )
{
  if ( !VALID_CHR( chr_ref ) ) return MAXCHR;

  ChrList[chr_ref].aistate.owner = VALIDATE_CHR( ChrList[chr_ref].aistate.owner );
  return ChrList[chr_ref].aistate.owner;
};

//--------------------------------------------------------------------------------------------
INLINE const CHR_REF chr_get_aichild( CHR_REF chr_ref )
{
  if ( !VALID_CHR( chr_ref ) ) return MAXCHR;

  ChrList[chr_ref].aistate.child = VALIDATE_CHR( ChrList[chr_ref].aistate.child );
  return ChrList[chr_ref].aistate.child;
};

//--------------------------------------------------------------------------------------------
INLINE const CHR_REF chr_get_aiattacklast( CHR_REF chr_ref )
{
  if ( !VALID_CHR( chr_ref ) ) return MAXCHR;

  ChrList[chr_ref].aistate.attacklast = VALIDATE_CHR( ChrList[chr_ref].aistate.attacklast );
  return ChrList[chr_ref].aistate.attacklast;
};

//--------------------------------------------------------------------------------------------
INLINE const CHR_REF chr_get_aibumplast( CHR_REF chr_ref )
{
  if ( !VALID_CHR( chr_ref ) ) return MAXCHR;

  ChrList[chr_ref].aistate.bumplast = VALIDATE_CHR( ChrList[chr_ref].aistate.bumplast );
  return ChrList[chr_ref].aistate.bumplast;
};

//--------------------------------------------------------------------------------------------
INLINE const CHR_REF chr_get_aihitlast( CHR_REF chr_ref )
{
  if ( !VALID_CHR( chr_ref ) ) return MAXCHR;

  ChrList[chr_ref].aistate.hitlast = VALIDATE_CHR( ChrList[chr_ref].aistate.hitlast );
  return ChrList[chr_ref].aistate.hitlast;
};
