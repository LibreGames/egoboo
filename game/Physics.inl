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
