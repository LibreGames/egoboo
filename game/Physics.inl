#pragma once

#include "Physics.h"
#include "egoboo_types.inl"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
extern Uint32  cv_list_count;
extern CVolume_t cv_list[1000];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE PhysicsData_t * CPhysicsData_new(PhysicsData_t * phys)
{
  if(NULL == phys) return phys;

  CPhysicsData_delete(phys);

  memset(phys, 0, sizeof(PhysicsData_t));

  EKEY_PNEW(phys, PhysicsData_t);

  phys->hillslide       = 1.00f;
  phys->slippyfriction  = 1.00f;   //1.05 for Chevron
  phys->airfriction     = 0.95f;
  phys->waterfriction   = 0.85f;
  phys->noslipfriction  = 0.95f;
  phys->platstick       = 0.04f;
  phys->gravity         =-1.00f;

  return phys;
}

//--------------------------------------------------------------------------------------------
INLINE bool_t CPhysicsData_delete(PhysicsData_t * phys)
{
  if(NULL == phys) return bfalse;
  if( !EKEY_PVALID(phys) ) return btrue;

  EKEY_PINVALIDATE( phys );

  return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE PhysicsData_t * CPhysicsData_renew(PhysicsData_t * phys)
{
  CPhysicsData_delete(phys);
  return CPhysicsData_new(phys);
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE void cv_list_add( CVolume_t * cv)
{
  if(NULL == cv || cv_list_count > 1000) return;

  cv_list[cv_list_count++] = *cv;
};

//--------------------------------------------------------------------------------------------
INLINE void cv_list_clear()
{
  cv_list_count = 0;
};

//--------------------------------------------------------------------------------------------
INLINE void cv_list_draw()
{
  Uint32 cnt;

  for(cnt=0; cnt<cv_list_count; cnt++)
  {
    CVolume_draw( &(cv_list[cnt]), btrue, bfalse );
  };
}