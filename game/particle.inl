#pragma once

#include "particle.h"
#include "char.h"

//--------------------------------------------------------------------------------------------
const CHR_REF prt_get_owner( PRT_REF iprt )
{
  if ( !VALID_PRT( iprt ) ) return MAXCHR;

  PrtList[iprt].owner = VALIDATE_CHR( PrtList[iprt].owner );
  return PrtList[iprt].owner;
};

//--------------------------------------------------------------------------------------------
const CHR_REF prt_get_target( PRT_REF iprt )
{
  if ( !VALID_PRT( iprt ) ) return MAXCHR;

  PrtList[iprt].target = VALIDATE_CHR( PrtList[iprt].target );
  return PrtList[iprt].target;
};

//--------------------------------------------------------------------------------------------
const CHR_REF prt_get_attachedtochr( PRT_REF iprt )
{
  if ( !VALID_PRT( iprt ) ) return MAXCHR;

  PrtList[iprt].attachedtochr = VALIDATE_CHR( PrtList[iprt].attachedtochr );
  return PrtList[iprt].attachedtochr;
};