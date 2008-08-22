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

#include "char.h"
#include "game.h"

#include "egoboo_types.inl"

#include <assert.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------


INLINE ACTION_INFO * action_info_new( ACTION_INFO * a);

INLINE ANIM_INFO * anim_info_new( ANIM_INFO * a );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

INLINE bool_t chr_attached( PChr_t lst, size_t lst_size, CHR_REF chr_ref )
{
  if ( !ACTIVE_CHR(lst, chr_ref ) ) return bfalse;

  lst[chr_ref].attachedto = VALIDATE_CHR(lst, lst[chr_ref].attachedto );
  if(!ACTIVE_CHR(lst, chr_ref)) lst[chr_ref].inwhichslot = SLOT_NONE;

  return ACTIVE_CHR(lst, lst[chr_ref].attachedto );
};

//--------------------------------------------------------------------------------------------
INLINE bool_t chr_in_pack( PChr_t lst, size_t lst_size, CHR_REF chr_ref )
{
  CHR_REF inwhichpack = chr_get_inwhichpack( lst, lst_size, chr_ref );
  return ACTIVE_CHR(lst, inwhichpack );
}

//--------------------------------------------------------------------------------------------
INLINE bool_t chr_has_inventory( PChr_t lst, size_t lst_size, CHR_REF chr_ref )
{
  bool_t retval = bfalse;
  CHR_REF nextinpack = chr_get_nextinpack( lst, lst_size, chr_ref );

  if ( ACTIVE_CHR(lst, nextinpack ) )
  {
    retval = btrue;
  }
#if defined(_DEBUG) || !defined(NDEBUG)
  else
  {
    assert( lst[chr_ref].numinpack == 0 );
  }
#endif

  return retval;
};

//--------------------------------------------------------------------------------------------
INLINE bool_t chr_is_invisible( PChr_t lst, size_t lst_size, CHR_REF chr_ref )
{
  if ( !ACTIVE_CHR(lst, chr_ref ) ) return btrue;

  return FP8_MUL( lst[chr_ref].alpha_fp8, lst[chr_ref].light_fp8 ) <= INVISIBLE;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t chr_using_slot( PChr_t lst, size_t lst_size, CHR_REF chr_ref, SLOT slot )
{
  CHR_REF inslot = chr_get_holdingwhich( lst, lst_size, chr_ref, slot );

  return ACTIVE_CHR(lst, inslot );
}


//--------------------------------------------------------------------------------------------
INLINE CHR_REF chr_get_nextinpack( PChr_t lst, size_t lst_size, CHR_REF chr_ref )
{
  if ( !ACTIVE_CHR(lst, chr_ref ) ) return INVALID_CHR;

#if defined(_DEBUG) || !defined(NDEBUG)
  {
    CHR_REF nextinpack = INVALID_CHR;
    nextinpack = lst[chr_ref].nextinpack;
    if ( INVALID_CHR != nextinpack && !ACTIVE_CHR(lst, chr_ref) )
    {
      // this is an invalid configuration that may indicate a corrupted list
      nextinpack = lst[nextinpack].nextinpack;
      if ( ACTIVE_CHR(lst, nextinpack ) )
      {
        // the list is definitely corrupted
        assert( bfalse );
      }
    }
  }
#endif

  lst[chr_ref].nextinpack = VALIDATE_CHR(lst, lst[chr_ref].nextinpack );
  return lst[chr_ref].nextinpack;
}

//--------------------------------------------------------------------------------------------
INLINE CHR_REF chr_get_onwhichplatform( PChr_t lst, size_t lst_size, CHR_REF chr_ref )
{
  if ( !ACTIVE_CHR(lst, chr_ref ) ) return INVALID_CHR;

  lst[chr_ref].onwhichplatform = VALIDATE_CHR(lst, lst[chr_ref].onwhichplatform );
  return lst[chr_ref].onwhichplatform;
}

//--------------------------------------------------------------------------------------------
INLINE CHR_REF chr_get_holdingwhich( PChr_t lst, size_t lst_size, CHR_REF chr_ref, SLOT slot )
{


  if ( !ACTIVE_CHR(lst, chr_ref ) || slot >= SLOT_COUNT ) return INVALID_CHR;

#if defined(_DEBUG) || !defined(NDEBUG)
  {
    CHR_REF inslot;
    inslot = lst[chr_ref].holdingwhich[slot];
    if ( INVALID_CHR != inslot )
    {
      CHR_REF holder = lst[inslot].attachedto;

      if ( chr_ref != holder )
      {
        // invalid configuration
        assert( bfalse );
      }
    };
  }
#endif

  lst[chr_ref].holdingwhich[slot] = VALIDATE_CHR(lst, lst[chr_ref].holdingwhich[slot] );
  return lst[chr_ref].holdingwhich[slot];
}

//--------------------------------------------------------------------------------------------
INLINE CHR_REF chr_get_inwhichpack( PChr_t lst, size_t lst_size, CHR_REF chr_ref )
{
  if ( !ACTIVE_CHR(lst, chr_ref ) ) return INVALID_CHR;

  lst[chr_ref].inwhichpack = VALIDATE_CHR(lst, lst[chr_ref].inwhichpack );
  return lst[chr_ref].inwhichpack;
}

//--------------------------------------------------------------------------------------------
INLINE CHR_REF chr_get_attachedto( PChr_t lst, size_t lst_size, CHR_REF chr_ref )
{
  if ( !ACTIVE_CHR(lst, chr_ref ) ) return INVALID_CHR;

#if defined(_DEBUG) || !defined(NDEBUG)

  {
    CHR_REF holder;

    if( INVALID_CHR != lst[chr_ref].attachedto )
    {
      SLOT slot = lst[chr_ref].inwhichslot;
      if(slot != SLOT_INVENTORY)
      {
        assert(SLOT_NONE != slot);
        holder = lst[chr_ref].attachedto;
        assert( lst[holder].holdingwhich[slot] == chr_ref );
      };
    }
    else
    {
      assert(SLOT_NONE == lst[chr_ref].inwhichslot);
    };
  }
#endif

  lst[chr_ref].attachedto = VALIDATE_CHR(lst, lst[chr_ref].attachedto );
  if( !ACTIVE_CHR(lst, lst[chr_ref].attachedto ) ) lst[chr_ref].inwhichslot = SLOT_NONE;
  return lst[chr_ref].attachedto;
}

//--------------------------------------------------------------------------------------------
INLINE CHR_REF chr_get_aitarget( PChr_t lst, size_t lst_size, Chr_t * pchr )
{
  if ( !EKEY_PVALID(pchr) ) return INVALID_CHR;

  pchr->aistate.target = VALIDATE_CHR( lst, pchr->aistate.target );
  return pchr->aistate.target;
}

//--------------------------------------------------------------------------------------------
INLINE CHR_REF chr_get_aiowner( PChr_t lst, size_t lst_size, Chr_t * pchr )
{
  if ( !EKEY_PVALID(pchr) ) return INVALID_CHR;

  pchr->aistate.owner = VALIDATE_CHR(lst, pchr->aistate.owner );
  return pchr->aistate.owner;
}

//--------------------------------------------------------------------------------------------
INLINE CHR_REF chr_get_aichild( PChr_t lst, size_t lst_size, Chr_t * pchr )
{
  if ( !EKEY_PVALID(pchr) ) return INVALID_CHR;

  pchr->aistate.child = VALIDATE_CHR(lst, pchr->aistate.child );
  return pchr->aistate.child;
}

//--------------------------------------------------------------------------------------------
INLINE CHR_REF chr_get_aiattacklast( PChr_t lst, size_t lst_size, Chr_t * pchr )
{
  if ( !EKEY_PVALID(pchr) ) return INVALID_CHR;

  pchr->aistate.attacklast = VALIDATE_CHR(lst, pchr->aistate.attacklast );
  return pchr->aistate.attacklast;
}

//--------------------------------------------------------------------------------------------
INLINE CHR_REF chr_get_aibumplast( PChr_t lst, size_t lst_size, Chr_t * pchr )
{
  if ( !EKEY_PVALID(pchr) ) return INVALID_CHR;

  pchr->aistate.bumplast = VALIDATE_CHR(lst, pchr->aistate.bumplast );
  return pchr->aistate.bumplast;
}

//--------------------------------------------------------------------------------------------
INLINE CHR_REF chr_get_aihitlast( PChr_t lst, size_t lst_size, Chr_t * pchr )
{
  if ( !EKEY_PVALID(pchr) ) return INVALID_CHR;

  pchr->aistate.hitlast = VALIDATE_CHR(lst, pchr->aistate.hitlast );
  return pchr->aistate.hitlast;
}


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE WP_LIST * wp_list_new(WP_LIST * w, vect3 * pos)
{
  w->tail = w->head = 0;

  if(NULL == pos)
  {
    w->pos[w->head].x = w->pos[w->head].y = 0;
    w->head++;
  }
  else
  {
    w->pos[w->head].x = pos->x;
    w->pos[w->head].y = pos->y;
    w->head++;
  }

  return w;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t wp_list_clear(WP_LIST * w)
{
  if(NULL == w) return bfalse;

  w->head = w->tail = 0;

  return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t wp_list_advance(WP_LIST * wl)
{
  /// @details BB> return value of btrue means wp_list is empty

  if(NULL == wl || wp_list_empty(wl)) return bfalse;

  // advance the tail and let it wrap around
  wl->tail = (wl->tail + 1) % MAXWAY;

  return wp_list_empty(wl);
}

//--------------------------------------------------------------------------------------------
INLINE bool_t wp_list_add(WP_LIST * wl, float x, float y)
{
  /// @details BB> add a point to the waypoint list.
  //      returns bfalse if the list is full (?or should it advance the tail?)

  bool_t retval = bfalse;
  int    test;

  if(NULL == wl) return retval;

  test = (wl->head + 1) % MAXWAY;

  if(test == wl->tail) return bfalse;

  wl->pos[wl->head].x = x;
  wl->pos[wl->head].y = y;

  wl->head = test;

  return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t wp_list_empty( WP_LIST * wl )
{
  if(NULL == wl) return btrue;

  return (wl->head == wl->tail);
}

//--------------------------------------------------------------------------------------------
INLINE float wp_list_x( WP_LIST * wl )
{
  if(NULL == wl) return 0.0f;

  return wl->pos[wl->tail].x;
}

//--------------------------------------------------------------------------------------------
INLINE float wp_list_y( WP_LIST * wl )
{
  if(NULL == wl) return 0.0f;

  return wl->pos[wl->tail].y;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE bool_t ai_state_delete(AI_STATE * a)
{
  if(NULL == a) return bfalse;
  if(!EKEY_PVALID(a)) return btrue;

  EKEY_PINVALIDATE( a );

  return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE AI_STATE * ai_state_new(AI_STATE * a)
{
  if(NULL == a) return NULL;

  ai_state_delete(a);

  memset(a, 0, sizeof(AI_STATE));

  EKEY_PNEW(a, AI_STATE);

  a->type = AILST_COUNT;          // The AI script to run

  // "pointers" to various external data
  a->target     = INVALID_CHR;       // Who the AI is after
  a->oldtarget  = INVALID_CHR;       // The target the last time the script was run
  a->owner      = INVALID_CHR;       // The character's owner
  a->child      = INVALID_CHR;       // The character's child
  a->bumplast   = INVALID_CHR;       // Last character it was bumped by
  a->attacklast = INVALID_CHR;       // Last character it was attacked by
  a->hitlast    = INVALID_CHR;       // Last character it hit

  // other random stuff
  a->turnmode       = TURNMODE_VELOCITY;        // Turning mode

  Latch_clear( &(a->latch) );

  return a;
}

//--------------------------------------------------------------------------------------------
INLINE AI_STATE * ai_state_init(Game_t * gs, AI_STATE * a, CHR_REF ichr)
{
  int tnc;

  Obj_t * pobj;
  Chr_t * pchr;
  Mad_t * pmad;
  Cap_t * pcap;

  if( !EKEY_PVALID(a) ) return NULL;

  if( !VALID_CHR(gs->ChrList, ichr) ) return NULL;
  pchr = gs->ChrList + ichr;

  pchr->model = VALIDATE_OBJ(gs->ObjList, pchr->model);
  if( !VALID_OBJ(gs->ObjList, pchr->model) ) return NULL;
  pobj = gs->ObjList + pchr->model;

  if( !LOADED_MAD(gs->MadList, pobj->mad) ) return NULL;
  pmad = gs->MadList + pobj->mad;

  if( !LOADED_CAP(gs->CapList, pobj->cap) ) return NULL;
  pcap = gs->CapList + pobj->cap;

  a->type    = pobj->ai;
  a->alert   = ALERT_SPAWNED;
  a->state   = pcap->stateoverride;
  a->content = pcap->contentoverride;
  a->target  = ichr;
  a->owner   = ichr;
  a->child   = INVALID_CHR;
  a->time    = 0;

  tnc = 0;
  while ( tnc < STOR_COUNT )
  {
    a->x[tnc] = 0;
    a->y[tnc] = 0;
    tnc++;
  }

  wp_list_new( &(a->wp), &(pchr->ori.pos) );

  a->morphed = bfalse;

  Latch_clear( &(a->latch) );
  a->turnmode = TURNMODE_VELOCITY;

  a->bumplast   = ichr;
  a->attacklast = INVALID_CHR;
  a->hitlast    = ichr;

  a->trgvel.x = 0;
  a->trgvel.y = 0;
  a->trgvel.z = 0;

  return a;
}


//--------------------------------------------------------------------------------------------
INLINE AI_STATE * ai_state_reinit(AI_STATE * a, CHR_REF ichr)
{
  if( !EKEY_PVALID(a) ) return NULL;

  a->alert = ALERT_NONE;
  a->target = ichr;
  a->time = 0;
  a->trgvel.x = 0;
  a->trgvel.y = 0;
  a->trgvel.z = 0;

  return a;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE ACTION_INFO * action_info_new( ACTION_INFO * a)
{
  if(NULL == a) return NULL;

  a->ready = btrue;
  a->keep  = bfalse;
  a->loop  = bfalse;
  a->now   = ACTION_DA;
  a->next  = ACTION_DA;

  return a;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE ANIM_INFO * anim_info_new( ANIM_INFO * a )
{
  if(NULL == a) return NULL;

  a->ilip = 0;
  a->flip    = 0.0f;
  a->next    = a->last = 0;

  return a;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE void VData_Blended_construct(VData_Blended_t * v)
{
  if(NULL == v) return;

  v->Vertices = NULL;
  v->Normals  = NULL;
  v->Colors   = NULL;
  v->Texture  = NULL;
  v->Ambient  = NULL;

  v->frame0 = 0;
  v->frame1 = 0;
  v->vrtmin = 0;
  v->vrtmax = 0;
  v->lerp   = 0.0f;
  v->needs_lighting = btrue;
}

//--------------------------------------------------------------------------------------------
INLINE void VData_Blended_destruct(VData_Blended_t * v)
{
  VData_Blended_Deallocate(v);
}


//--------------------------------------------------------------------------------------------
INLINE void VData_Blended_Deallocate(VData_Blended_t * v)
{
  if(NULL == v) return;

  EGOBOO_DELETE( v->Vertices );
  EGOBOO_DELETE( v->Normals );
  EGOBOO_DELETE( v->Colors );
  EGOBOO_DELETE( v->Texture );
  EGOBOO_DELETE( v->Ambient );
}

//--------------------------------------------------------------------------------------------
INLINE VData_Blended_t * VData_Blended_new()
{
  VData_Blended_t * retval = EGOBOO_NEW( VData_Blended_t );
  if(NULL != retval)
  {
    VData_Blended_construct(retval);
  };
  return retval;
}

//--------------------------------------------------------------------------------------------
INLINE void VData_Blended_delete(VData_Blended_t * v)
{
  if(NULL != v) return;

  VData_Blended_destruct(v);
  EGOBOO_DELETE(v);
}

//--------------------------------------------------------------------------------------------
INLINE void VData_Blended_Allocate(VData_Blended_t * v, size_t verts)
{
  if(NULL == v) return;

  VData_Blended_destruct(v);

  v->Vertices = EGOBOO_NEW_ARY( vect3, verts );
  v->Normals  = EGOBOO_NEW_ARY( vect3, verts );
  v->Colors   = EGOBOO_NEW_ARY( vect4, verts );
  v->Texture  = EGOBOO_NEW_ARY( vect2, verts );
  v->Ambient  = EGOBOO_NEW_ARY( float, verts );
}
