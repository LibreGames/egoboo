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

INLINE ACTION_INFO * action_info_new( ACTION_INFO * a);

INLINE ANIM_INFO * anim_info_new( ANIM_INFO * a );

INLINE WP_LIST * wp_list_new(WP_LIST * w, vect3 * pos);
INLINE bool_t    wp_list_advance(WP_LIST * wl);
INLINE bool_t    wp_list_add(WP_LIST * wl, float x, float y);
INLINE float     wp_list_x( WP_LIST * wl );
INLINE float     wp_list_y( WP_LIST * wl );

INLINE AI_STATE * ai_state_new(AI_STATE * a, Uint16 ichr);
INLINE AI_STATE * ai_state_renew(AI_STATE * a, Uint16 ichr);

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


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE WP_LIST * wp_list_new(WP_LIST * w, vect3 * pos)
{
  w->tail = 0;
  w->head = 1;

  if(NULL == pos)
  {
    w->pos[0].x = w->pos[0].y = 0;
  }
  else
  {
    w->pos[0].x = pos->x;
    w->pos[0].y = pos->y;
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
INLINE AI_STATE * ai_state_new(AI_STATE * a, Uint16 ichr)
{
  int tnc;

  CHR * pchr;
  MAD * pmad;
  CAP * pcap;

  if(NULL == a) return NULL;

  memset(a, 0, sizeof(AI_STATE));

  if( !VALID_CHR_RANGE(ichr) ) return NULL;

  pchr = ChrList + ichr;

  if( !VALID_MDL_RANGE(pchr->model) ) return NULL;

  pmad = MadList + pchr->model;
  pcap = CapList + pchr->model;

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

  a->lip_fp8 = 0;
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
  VData_Blended * retval = calloc(sizeof(VData_Blended), 1);
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

  v->Vertices = calloc( sizeof(vect3), verts);
  v->Normals  = calloc( sizeof(vect3), verts);
  v->Colors   = calloc( sizeof(vect4), verts);
  v->Texture  = calloc( sizeof(vect2), verts);
  v->Ambient  = calloc( sizeof(float), verts);
}
