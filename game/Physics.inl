#pragma once

#include "Physics.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
INLINE CPhysicsData * CPhysicsData_new(CPhysicsData * phys)
{
  if(NULL == phys) return phys;

  CPhysicsData_delete(phys);

  memset(phys, 0, sizeof(CPhysicsData));

  EKEY_PNEW(phys, CPhysicsData);

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
INLINE bool_t CPhysicsData_delete(CPhysicsData * phys)
{
  if(NULL == phys) return bfalse;
  if( !EKEY_PVALID(phys) ) return btrue;

  EKEY_PINVALIDATE( phys );

  return btrue;
}

//--------------------------------------------------------------------------------------------
INLINE CPhysicsData * CPhysicsData_renew(CPhysicsData * phys)
{
  CPhysicsData_delete(phys);
  return CPhysicsData_new(phys);
}
