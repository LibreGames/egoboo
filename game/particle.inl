#pragma once

#include "particle.h"
#include "char.h"
#include "game.h"

//--------------------------------------------------------------------------------------------
CHR_REF prt_get_owner( Game_t * gs, PRT_REF iprt )
{
  if ( !ACTIVE_PRT( gs->PrtList, iprt ) ) return INVALID_CHR;

  gs->PrtList[iprt].owner = VALIDATE_CHR( gs->ChrList, gs->PrtList[iprt].owner );
  return gs->PrtList[iprt].owner;
};

//--------------------------------------------------------------------------------------------
CHR_REF prt_get_target( Game_t * gs, PRT_REF iprt )
{
  if ( !ACTIVE_PRT( gs->PrtList, iprt ) ) return INVALID_CHR;

  gs->PrtList[iprt].target = VALIDATE_CHR( gs->ChrList, gs->PrtList[iprt].target );
  return gs->PrtList[iprt].target;
};

//--------------------------------------------------------------------------------------------
CHR_REF prt_get_attachedtochr( Game_t * gs, PRT_REF iprt )
{
  if ( !ACTIVE_PRT( gs->PrtList, iprt ) ) return INVALID_CHR;

  gs->PrtList[iprt].attachedtochr = VALIDATE_CHR( gs->ChrList, gs->PrtList[iprt].attachedtochr );
  return gs->PrtList[iprt].attachedtochr;
};