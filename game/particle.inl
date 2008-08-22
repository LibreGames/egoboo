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

#include "particle.h"
#include "game.h"

#include "char.inl"
#include "egoboo_types.inl"

//--------------------------------------------------------------------------------------------
INLINE CHR_REF prt_get_owner( Game_t * gs, PRT_REF iprt )
{
  if ( !ACTIVE_PRT( gs->PrtList, iprt ) ) return INVALID_CHR;

  gs->PrtList[iprt].owner = VALIDATE_CHR( gs->ChrList, gs->PrtList[iprt].owner );
  return gs->PrtList[iprt].owner;
};

//--------------------------------------------------------------------------------------------
INLINE CHR_REF prt_get_target( Game_t * gs, PRT_REF iprt )
{
  if ( !ACTIVE_PRT( gs->PrtList, iprt ) ) return INVALID_CHR;

  gs->PrtList[iprt].target = VALIDATE_CHR( gs->ChrList, gs->PrtList[iprt].target );
  return gs->PrtList[iprt].target;
}

//--------------------------------------------------------------------------------------------
INLINE CHR_REF prt_get_attachedtochr( Game_t * gs, PRT_REF iprt )
{
  if ( !ACTIVE_PRT( gs->PrtList, iprt ) ) return INVALID_CHR;

  gs->PrtList[iprt].attachedtochr = VALIDATE_CHR( gs->ChrList, gs->PrtList[iprt].attachedtochr );
  return gs->PrtList[iprt].attachedtochr;
}
