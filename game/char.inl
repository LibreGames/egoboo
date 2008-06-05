#pragma once

#include "char.h"
#include "game.h"

#include <assert.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------


//INLINE const bool_t chr_in_pack( CHR_REF character );
//INLINE const bool_t chr_attached( CHR_REF character );
//INLINE const bool_t chr_has_inventory( CHR_REF character );
//INLINE const bool_t chr_is_invisible( CHR_REF character );
//INLINE const bool_t chr_using_slot( CHR_REF character, SLOT slot );
//
//INLINE const CHR_REF chr_get_nextinpack( CHR_REF ichr );
//INLINE const CHR_REF chr_get_onwhichplatform( CHR_REF ichr );
//INLINE const CHR_REF chr_get_inwhichpack( CHR_REF ichr );
//INLINE const CHR_REF chr_get_attachedto( CHR_REF ichr );
//INLINE const CHR_REF chr_get_holdingwhich( CHR_REF ichr, SLOT slot );
//
//INLINE const CHR_REF chr_get_aitarget( Chr * pchr );
//INLINE const CHR_REF chr_get_aiowner( Chr * pchr );
//INLINE const CHR_REF chr_get_aichild( Chr * pchr );
//INLINE const CHR_REF chr_get_aiattacklast( Chr * pchr );
//INLINE const CHR_REF chr_get_aibumplast( Chr * pchr );
//INLINE const CHR_REF chr_get_aihitlast( Chr * pchr );

INLINE ACTION_INFO * action_info_new( ACTION_INFO * a);

INLINE ANIM_INFO * anim_info_new( ANIM_INFO * a );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

INLINE const bool_t chr_attached( Chr lst[], size_t lst_size, CHR_REF chr_ref )
{
  if ( !VALID_CHR(lst, chr_ref ) ) return bfalse;

  lst[chr_ref].attachedto = VALIDATE_CHR(lst, lst[chr_ref].attachedto );
  if(!VALID_CHR(lst, chr_ref)) lst[chr_ref].inwhichslot = SLOT_NONE;

  return VALID_CHR(lst, lst[chr_ref].attachedto );
};

//--------------------------------------------------------------------------------------------
INLINE const bool_t chr_in_pack( Chr lst[], size_t lst_size, CHR_REF chr_ref )
{
  CHR_REF inwhichpack = chr_get_inwhichpack( lst, lst_size, chr_ref );
  return VALID_CHR(lst, inwhichpack );
}

//--------------------------------------------------------------------------------------------
INLINE const bool_t chr_has_inventory( Chr lst[], size_t lst_size, CHR_REF chr_ref )
{
  bool_t retval = bfalse;
  CHR_REF nextinpack = chr_get_nextinpack( lst, lst_size, chr_ref );

  if ( VALID_CHR(lst, nextinpack ) )
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
INLINE const bool_t chr_is_invisible( Chr lst[], size_t lst_size, CHR_REF chr_ref )
{
  if ( !VALID_CHR(lst, chr_ref ) ) return btrue;

  return FP8_MUL( lst[chr_ref].alpha_fp8, lst[chr_ref].light_fp8 ) <= INVISIBLE;
};

//--------------------------------------------------------------------------------------------
INLINE const bool_t chr_using_slot( Chr lst[], size_t lst_size, CHR_REF chr_ref, SLOT slot )
{
  CHR_REF inslot = chr_get_holdingwhich( lst, lst_size, chr_ref, slot );

  return VALID_CHR(lst, inslot );
};


//--------------------------------------------------------------------------------------------
INLINE const CHR_REF chr_get_nextinpack( Chr lst[], size_t lst_size, CHR_REF chr_ref )
{
  CHR_REF nextinpack = MAXCHR;

  if ( !VALID_CHR(lst, chr_ref ) ) return MAXCHR;

#if defined(_DEBUG) || !defined(NDEBUG)
  nextinpack = lst[chr_ref].nextinpack;
  if ( MAXCHR != nextinpack && !lst[chr_ref].on )
  {
    // this is an invalid configuration that may indicate a corrupted list
    nextinpack = lst[nextinpack].nextinpack;
    if ( VALID_CHR(lst, nextinpack ) )
    {
      // the list is definitely corrupted
      assert( bfalse );
    }
  }
#endif

  lst[chr_ref].nextinpack = VALIDATE_CHR(lst, lst[chr_ref].nextinpack );
  return lst[chr_ref].nextinpack;
};

//--------------------------------------------------------------------------------------------
INLINE const CHR_REF chr_get_onwhichplatform( Chr lst[], size_t lst_size, CHR_REF chr_ref )
{
  if ( !VALID_CHR(lst, chr_ref ) ) return MAXCHR;

  lst[chr_ref].onwhichplatform = VALIDATE_CHR(lst, lst[chr_ref].onwhichplatform );
  return lst[chr_ref].onwhichplatform;
};

//--------------------------------------------------------------------------------------------
INLINE const CHR_REF chr_get_holdingwhich( Chr lst[], size_t lst_size, CHR_REF chr_ref, SLOT slot )
{
  

  if ( !VALID_CHR(lst, chr_ref ) || slot >= SLOT_COUNT ) return MAXCHR;

#if defined(_DEBUG) || !defined(NDEBUG)
  {
    CHR_REF inslot;
    inslot = lst[chr_ref].holdingwhich[slot];
    if ( MAXCHR != inslot )
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
};

//--------------------------------------------------------------------------------------------
INLINE const CHR_REF chr_get_inwhichpack( Chr lst[], size_t lst_size, CHR_REF chr_ref )
{
  if ( !VALID_CHR(lst, chr_ref ) ) return MAXCHR;

  lst[chr_ref].inwhichpack = VALIDATE_CHR(lst, lst[chr_ref].inwhichpack );
  return lst[chr_ref].inwhichpack;
};

//--------------------------------------------------------------------------------------------
INLINE const CHR_REF chr_get_attachedto( Chr lst[], size_t lst_size, CHR_REF chr_ref )
{
  if ( !VALID_CHR(lst, chr_ref ) ) return MAXCHR;

#if defined(_DEBUG) || !defined(NDEBUG)

  {
    CHR_REF holder;

    if( MAXCHR != lst[chr_ref].attachedto )
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
  if( !VALID_CHR(lst, lst[chr_ref].attachedto ) ) lst[chr_ref].inwhichslot = SLOT_NONE;
  return lst[chr_ref].attachedto;
};

//--------------------------------------------------------------------------------------------
INLINE const CHR_REF chr_get_aitarget( Chr lst[], size_t lst_size, Chr * pchr )
{
  if ( NULL==pchr || !pchr->on ) return MAXCHR;

  pchr->aistate.target = VALIDATE_CHR( lst, pchr->aistate.target );
  return pchr->aistate.target;
};

//--------------------------------------------------------------------------------------------
INLINE const CHR_REF chr_get_aiowner( Chr lst[], size_t lst_size, Chr * pchr )
{
  if ( NULL==pchr || !pchr->on ) return MAXCHR;

  pchr->aistate.owner = VALIDATE_CHR(lst, pchr->aistate.owner );
  return pchr->aistate.owner;
};

//--------------------------------------------------------------------------------------------
INLINE const CHR_REF chr_get_aichild( Chr lst[], size_t lst_size, Chr * pchr )
{
  if ( NULL==pchr || !pchr->on ) return MAXCHR;

  pchr->aistate.child = VALIDATE_CHR(lst, pchr->aistate.child );
  return pchr->aistate.child;
};

//--------------------------------------------------------------------------------------------
INLINE const CHR_REF chr_get_aiattacklast( Chr lst[], size_t lst_size, Chr * pchr )
{
  if ( NULL==pchr || !pchr->on ) return MAXCHR;

  pchr->aistate.attacklast = VALIDATE_CHR(lst, pchr->aistate.attacklast );
  return pchr->aistate.attacklast;
};

//--------------------------------------------------------------------------------------------
INLINE const CHR_REF chr_get_aibumplast( Chr lst[], size_t lst_size, Chr * pchr )
{
  if ( NULL==pchr || !pchr->on ) return MAXCHR;

  pchr->aistate.bumplast = VALIDATE_CHR(lst, pchr->aistate.bumplast );
  return pchr->aistate.bumplast;
};

//--------------------------------------------------------------------------------------------
INLINE const CHR_REF chr_get_aihitlast( Chr lst[], size_t lst_size, Chr * pchr )
{
  if ( NULL==pchr || !pchr->on ) return MAXCHR;

  pchr->aistate.hitlast = VALIDATE_CHR(lst, pchr->aistate.hitlast );
  return pchr->aistate.hitlast;
};


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
INLINE bool_t wp_list_advance(WP_LIST * wl)
{
  // BB > return value of btrue means wp_list is empty

  bool_t retval = bfalse;

  if(NULL == wl) return retval;

  if( wl->tail != wl->head )
  {
    // advance the tail and let it wrap around
    wl->tail = (wl->tail + 1) % MAXWAY;
  }

  if ( wl->tail == wl->head )
  {
    retval = btrue;
  }

  return retval;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t wp_list_add(WP_LIST * wl, float x, float y)
{
  // BB > add a point to the waypoint list. 
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
};

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
};

//--------------------------------------------------------------------------------------------
INLINE float wp_list_y( WP_LIST * wl ) 
{ 
  if(NULL == wl) return 0.0f; 

  return wl->pos[wl->tail].y; 
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE AI_STATE * ai_state_new(CGame * gs, AI_STATE * a, CHR_REF ichr)
{
  int tnc;

  Chr * pchr;
  Mad * pmad;
  Cap * pcap;

  if(NULL == a) return NULL;

  memset(a, 0, sizeof(AI_STATE));

  if( !VALID_MDL_RANGE(ichr) ) return NULL;
  pchr = gs->ChrList + ichr;

  if( !VALID_MDL_RANGE(pchr->model) ) return NULL;

  pmad = gs->MadList + pchr->model;
  pcap = gs->CapList + pchr->model;

  a->type    = pmad->ai;
  a->alert   = ALERT_SPAWNED;
  a->state   = pcap->stateoverride;
  a->content = pcap->contentoverride;
  a->target  = ichr;
  a->owner   = ichr;
  a->child   = ichr;
  a->time    = 0;

  tnc = 0;
  while ( tnc < MAXSTOR )
  {
    a->x[tnc] = 0;
    a->y[tnc] = 0;
    tnc++;
  }

  wp_list_new( &(a->wp), &(pchr->pos) );

  a->morphed = bfalse;

  a->latch.x = 0;
  a->latch.y = 0;
  a->latch.b = 0;
  a->turnmode = TURNMODE_VELOCITY;

  a->bumplast   = ichr;
  a->attacklast = MAXCHR;
  a->hitlast    = ichr;

  a->trgvel.x = 0;
  a->trgvel.y = 0;
  a->trgvel.z = 0;

  return a;
};


//--------------------------------------------------------------------------------------------
INLINE AI_STATE * ai_state_renew(AI_STATE * a, Uint16 ichr)
{
  if(NULL == a) return NULL;

  if( !VALID_CHR_RANGE(ichr) )
  {
    memset(a, 0, sizeof(AI_STATE));
    return NULL;
  }

  a->alert = ALERT_NONE;
  a->target = ichr;
  a->time = 0;
  a->trgvel.x = 0;
  a->trgvel.y = 0;
  a->trgvel.z = 0;

  return a;
};

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
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE void VData_Blended_construct(VData_Blended * v)
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
INLINE void VData_Blended_destruct(VData_Blended * v)
{
  VData_Blended_Deallocate(v);
};


//--------------------------------------------------------------------------------------------
INLINE void VData_Blended_Deallocate(VData_Blended * v)
{
  if(NULL == v) return;

  FREE( v->Vertices );
  FREE( v->Normals );
  FREE( v->Colors );
  FREE( v->Texture );
  FREE( v->Ambient );
}

//--------------------------------------------------------------------------------------------
INLINE VData_Blended * VData_Blended_new()
{
  VData_Blended * retval = (VData_Blended*)calloc(1, sizeof(VData_Blended));
  if(NULL != retval)
  {
    VData_Blended_construct(retval);
  };
  return retval;
};

//--------------------------------------------------------------------------------------------
INLINE void VData_Blended_delete(VData_Blended * v)
{
  if(NULL != v) return;

  VData_Blended_destruct(v);
  FREE(v);
};

//--------------------------------------------------------------------------------------------
INLINE void VData_Blended_Allocate(VData_Blended * v, size_t verts)
{
  if(NULL == v) return;

  VData_Blended_destruct(v);

  v->Vertices = (vect3*)calloc( verts, sizeof(vect3));
  v->Normals  = (vect3*)calloc( verts, sizeof(vect3));
  v->Colors   = (vect4*)calloc( verts, sizeof(vect4));
  v->Texture  = (vect2*)calloc( verts, sizeof(vect2));
  v->Ambient  = (float*)calloc( verts, sizeof(float));
}
