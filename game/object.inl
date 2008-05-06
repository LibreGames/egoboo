#pragma once

#include "object.h"
#include "char.h"

//--------------------------------------------------------------------------------------------
INLINE const CHR_REF team_get_sissy( TEAM_REF iteam )
{
  if ( !VALID_TEAM( iteam ) ) return MAXCHR;

  TeamList[iteam].sissy = VALIDATE_CHR( TeamList[iteam].sissy );
  return TeamList[iteam].sissy;
};

//--------------------------------------------------------------------------------------------
INLINE const CHR_REF team_get_leader( TEAM_REF iteam )
{
  if ( !VALID_TEAM( iteam ) ) return MAXCHR;

  TeamList[iteam].leader = VALIDATE_CHR( TeamList[iteam].leader );
  return TeamList[iteam].leader;
};