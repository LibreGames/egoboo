#pragma once

#include "particle.h"
#include "char.h"
#include "game.h"

//--------------------------------------------------------------------------------------------
const CHR_REF prt_get_owner( GameState * gs, PRT_REF iprt )
{
  if ( !VALID_PRT( gs->PrtList, iprt ) ) return MAXCHR;

  gs->PrtList[iprt].owner = VALIDATE_CHR( gs->ChrList, gs->PrtList[iprt].owner );
  return gs->PrtList[iprt].owner;
};

//--------------------------------------------------------------------------------------------
const CHR_REF prt_get_target( GameState * gs, PRT_REF iprt )
{
  if ( !VALID_PRT( gs->PrtList, iprt ) ) return MAXCHR;

  gs->PrtList[iprt].target = VALIDATE_CHR( gs->ChrList, gs->PrtList[iprt].target );
  return gs->PrtList[iprt].target;
};

//--------------------------------------------------------------------------------------------
const CHR_REF prt_get_attachedtochr( GameState * gs, PRT_REF iprt )
{
  if ( !VALID_PRT( gs->PrtList, iprt ) ) return MAXCHR;

  gs->PrtList[iprt].attachedtochr = VALIDATE_CHR( gs->ChrList, gs->PrtList[iprt].attachedtochr );
  return gs->PrtList[iprt].attachedtochr;
};