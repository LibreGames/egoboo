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
/// @brief Implementation of I/O utilities
/// @details

#include "egoboo_utility.h"

#include "Mad.h"
#include "Log.h"
#include "file_common.h"

#include "egoboo_strutil.h"

#include <assert.h>

#include "particle.inl"
#include "char.inl"
#include "egoboo_math.inl"
#include "egoboo_types.inl"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

EGO_CONST char * globalname = NULL;   // For debuggin' fgoto_colon
Uint16 randie[RANDIE_COUNT];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
DAMAGE fget_damage( FILE *fileread )
{
  char cTmp;
  DAMAGE retval = DAMAGE_NULL;
  if ( feof( fileread ) ) return retval;

  cTmp = fget_first_letter( fileread );
  switch ( toupper( cTmp ) )
  {
    case 'S': retval = DAMAGE_SLASH; break;
    case 'C': retval = DAMAGE_CRUSH; break;
    case 'P': retval = DAMAGE_POKE;  break;
    case 'H': retval = DAMAGE_HOLY;  break;
    case 'E': retval = DAMAGE_EVIL;  break;
    case 'F': retval = DAMAGE_FIRE;  break;
    case 'I': retval = DAMAGE_ICE;   break;
    case 'Z': retval = DAMAGE_ZAP;   break;
    case 'N': retval = DAMAGE_NULL;  break;
  };

  return retval;
};

//--------------------------------------------------------------------------------------------
DAMAGE fget_next_damage( FILE *fileread )
{
  DAMAGE retval = DAMAGE_NULL;

  if ( fgoto_colon_yesno( fileread ) )
  {
    retval = fget_damage( fileread );
  }

  return retval;
};

//--------------------------------------------------------------------------------------------
BLUD_LEVEL fget_blud( FILE *fileread )
{
  char cTmp;
  BLUD_LEVEL retval = BLUD_NORMAL;
  if ( feof( fileread ) ) return retval;

  cTmp = fget_first_letter( fileread );
  switch ( toupper( cTmp ) )
  {
    case 'T': retval = BLUD_NORMAL; break;
    case 'F': retval = BLUD_NONE;   break;
    case 'U': retval = BLUD_ULTRA;  break;
  };

  return retval;
};

//--------------------------------------------------------------------------------------------
BLUD_LEVEL fget_next_blud( FILE *fileread )
{
  BLUD_LEVEL retval = BLUD_NORMAL;

  if ( fgoto_colon_yesno( fileread ) )
  {
    retval = fget_blud( fileread );
  }

  return retval;
};

//--------------------------------------------------------------------------------------------
RESPAWN_MODE fget_respawn( FILE *fileread )
{
  char cTmp;
  RESPAWN_MODE retval = RESPAWN_NORMAL;
  if ( feof( fileread ) ) return retval;

  cTmp = fget_first_letter( fileread );
  switch ( toupper( cTmp ) )
  {
    case 'T': retval = RESPAWN_NORMAL;   break;
    case 'F': retval = RESPAWN_NONE;   break;
    case 'A': retval = RESPAWN_ANYTIME;  break;
  };

  return retval;
};

//--------------------------------------------------------------------------------------------
RESPAWN_MODE fget_next_respawn( FILE *fileread )
{
  RESPAWN_MODE retval = RESPAWN_NORMAL;

  if ( fgoto_colon_yesno( fileread ) )
  {
    retval = fget_respawn( fileread );
  }

  return retval;
};

//--------------------------------------------------------------------------------------------
DYNA_MODE fget_dynamode( FILE *fileread )
{
  char cTmp;
  DYNA_MODE retval = DYNA_OFF;
  if ( feof( fileread ) ) return retval;

  cTmp = fget_first_letter( fileread );
  switch ( toupper( cTmp ) )
  {
    case 'T': retval = DYNA_ON;   break;
    case 'F': retval = DYNA_OFF;   break;
    case 'L': retval = DYNA_LOCAL; break;
  };

  return retval;
};

//--------------------------------------------------------------------------------------------
DYNA_MODE fget_next_dynamode( FILE *fileread )
{
  DYNA_MODE retval = DYNA_OFF;
  if ( fgoto_colon_yesno( fileread ) )
  {
    retval = fget_dynamode( fileread );
  }

  return retval;
};


//--------------------------------------------------------------------------------------------
char * undo_idsz( IDSZ idsz )
{
  /// @details ZZ> This function takes an integer and makes an text IDSZ out of it.
  //     It will set valueidsz to "NONE" if the idsz is 0

  static char value_string[5] = {"NONE"};

  if ( idsz == IDSZ_NONE )
  {
    snprintf( value_string, sizeof( value_string ), "NONE" );
  }
  else
  {
    value_string[0] = (( idsz >> 15 ) & 31 ) + 'A';
    value_string[1] = (( idsz >> 10 ) & 31 ) + 'A';
    value_string[2] = (( idsz >> 5 ) & 31 ) + 'A';
    value_string[3] = (( idsz ) & 31 ) + 'A';
    value_string[4] = 0;
  }

  return value_string;
}

//--------------------------------------------------------------------------------------------
bool_t ftest_idsz( FILE* fileread )
{
  char cTmp;
  long pos;
  bool_t retval;

  pos = ftell(fileread);

  cTmp = fget_first_letter( fileread );
  retval = ( '[' == cTmp);

  fseek( fileread, 4, SEEK_CUR);
  cTmp = fget_first_letter( fileread );
  retval = retval && ( ']' == cTmp);

  // go back to the original position
  fseek( fileread, pos, SEEK_SET );

  return retval;
};

//--------------------------------------------------------------------------------------------
IDSZ fget_idsz( FILE* fileread )
{
  /// @details ZZ> This function reads and returns an IDSZ tag, or IDSZ_NONE if there wasn't one

  char sTemp[4], cTmp;
  IDSZ idsz = IDSZ_NONE;

  if ( feof( fileread ) ) return idsz;

  cTmp = fget_first_letter( fileread );
  if ( cTmp == '[' )
  {
    int cnt;
    for ( cnt = 0; cnt < 4; cnt++ ) sTemp[cnt] = fgetc( fileread );
    assert( ']' == fgetc( fileread ) );
    idsz = MAKE_IDSZ( sTemp );
  }

  return idsz;
}

//--------------------------------------------------------------------------------------------
IDSZ fget_next_idsz( FILE* fileread )
{
  /// @details ZZ> This function reads and returns an IDSZ tag, or IDSZ_NONE if there wasn't one

  IDSZ idsz = IDSZ_NONE;

  if ( fgoto_colon_yesno( fileread ) )
  {
    idsz = fget_idsz( fileread );
  };

  return idsz;
}

//--------------------------------------------------------------------------------------------
bool_t fget_pair_fp8( FILE* fileread, PAIR * ppair )
{
  /// @details ZZ> This function reads a damage/stat pair ( eg. 5-9 )

  char   cTmp;
  int    iBase, iRand;
  float  fBase, fRand;

  if ( feof( fileread ) ) return bfalse;

  fscanf( fileread, "%f", &fBase );   // The first number
  iBase = FLOAT_TO_FP8( fBase );

  cTmp = fget_first_letter( fileread );   // The hyphen
  if ( cTmp != '-' )
  {
    // second number is non-existant
    if ( NULL != ppair )
    {
      ppair->ibase = iBase;
      ppair->irand = 1;
    };
  }
  else
  {
    fscanf( fileread, "%f", &fRand );   // The second number
    iRand = FLOAT_TO_FP8( fRand );

    if ( NULL != ppair )
    {
      ppair->ibase = MIN( iBase, iRand );
      ppair->irand = ( MAX( iBase, iRand ) - ppair->ibase ) + 1;
    };
  };

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t fget_next_pair_fp8( FILE* fileread, PAIR * ppair )
{
  /// @details ZZ> This function reads the next damage/stat pair ( eg. 5-9 )

  bool_t retval = bfalse;

  if ( fgoto_colon_yesno( fileread ) )
  {
    retval = fget_pair_fp8( fileread, ppair );
  };

  return retval;

}

//--------------------------------------------------------------------------------------------
bool_t fget_pair( FILE* fileread, PAIR * ppair )
{
  /// @details ZZ> This function reads a damage/stat pair ( eg. 5-9 )

  char   cTmp;
  int    iBase, iRand;
  float  fBase, fRand;

  if ( feof( fileread ) ) return bfalse;

  fscanf( fileread, "%f", &fBase );   // The first number
  iBase = fBase;

  cTmp = fget_first_letter( fileread );   // The hyphen
  if ( cTmp != '-' )
  {
    // second number is non-existant
    if ( NULL != ppair )
    {
      ppair->ibase = iBase;
      ppair->irand = 1;
    };
  }
  else
  {
    fscanf( fileread, "%f", &fRand );   // The second number
    iRand = fRand;

    if ( NULL != ppair )
    {
      ppair->ibase = ppair->ibase = MIN( iBase, iRand );
      ppair->irand = ppair->irand = ( MAX( iBase, iRand ) - ppair->ibase ) + 1;
    };
  };

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t fget_next_pair( FILE* fileread, PAIR * ppair )
{
  /// @details ZZ> This function reads the next damage/stat pair ( eg. 5-9 )

  bool_t retval = bfalse;

  if ( fgoto_colon_yesno( fileread ) )
  {
    retval = fget_pair( fileread, ppair );
  };

  return retval;

}

//--------------------------------------------------------------------------------------------
bool_t undo_pair_fp8( PAIR * ppair, RANGE * prange )
{
  /// @details ZZ> This function generates a damage/stat pair ( eg. 3-6.5 )
  //     from the base and random values.  It set prange->ffrom and
  //     prange->fto

  bool_t bfound = bfalse;
  float fFrom = 0.0f, fTo = 0.0f;

  if ( NULL != ppair )
  {
    fFrom = FP8_TO_FLOAT( ppair->ibase );
    fTo   = FP8_TO_FLOAT( ppair->ibase + ppair->irand - 1 );
    if ( fFrom < 0.0 )  fFrom = 0.0;
    if ( fTo   < 0.0 )  fTo   = 0.0;
    bfound = btrue;
  };

  if ( NULL != prange )
  {
    prange->ffrom = fFrom;
    prange->fto   = fTo;
  }

  return bfound;
}

//--------------------------------------------------------------------------------------------
bool_t undo_pair( PAIR * ppair, RANGE * prange )
{
  /// @details ZZ> This function generates a damage/stat pair ( eg. 3-6.5 )
  //     from the base and random values.  It set prange->ffrom and
  //     prange->fto

  bool_t bfound = bfalse;
  float fFrom = 0.0f, fTo = 0.0f;

  if ( NULL != ppair )
  {
    fFrom = ppair->ibase;
    fTo   = ppair->ibase + ppair->irand - 1;
    if ( fFrom < 0.0 )  fFrom = 0.0;
    if ( fTo   < 0.0 )  fTo   = 0.0;
    bfound = btrue;
  };

  if ( NULL != prange )
  {
    prange->ffrom = fFrom;
    prange->fto   = fTo;
  }

  return bfound;
}

//--------------------------------------------------------------------------------------------
void ftruthf( FILE* filewrite, char* text, bool_t truth )
{
  /// @details ZZ> This function kinda mimics fprintf for the output of
  //     btrue bfalse statements

  fprintf( filewrite, text );
  if ( truth )
  {
    fprintf( filewrite, "TRUE\n" );
  }
  else
  {
    fprintf( filewrite, "FALSE\n" );
  }
}

//--------------------------------------------------------------------------------------------
void fdamagf( FILE* filewrite, char* text, DAMAGE damagetype )
{
  /// @details ZZ> This function kinda mimics fprintf for the output of
  //     SLASH CRUSH POKE HOLY EVIL FIRE ICE ZAP statements

  fprintf( filewrite, text );

  switch ( damagetype )
  {
    case DAMAGE_SLASH: fprintf( filewrite, "SLASH\n" ); break;
    case DAMAGE_CRUSH: fprintf( filewrite, "CRUSH\n" ); break;
    case DAMAGE_POKE:  fprintf( filewrite, "POKE\n" );  break;
    case DAMAGE_HOLY:  fprintf( filewrite, "HOLY\n" );  break;
    case DAMAGE_EVIL:  fprintf( filewrite, "EVIL\n" );  break;
    case DAMAGE_FIRE:  fprintf( filewrite, "FIRE\n" );  break;
    case DAMAGE_ICE:   fprintf( filewrite, "ICE\n" );   break;
    case DAMAGE_ZAP:   fprintf( filewrite, "ZAP\n" );   break;
    default:           fprintf( filewrite, "NONE\n" );  break;
  };
}

//--------------------------------------------------------------------------------------------
void factiof( FILE* filewrite, char* text, ACTION action )
{
  /// @details ZZ> This function kinda mimics fprintf for the output of
  //     SLASH CRUSH POKE HOLY EVIL FIRE ICE ZAP statements

  fprintf( filewrite, text );

  if ( action >= ACTION_DA && action <= ACTION_DD )
    fprintf( filewrite, "WALK\n" );
  else if ( action >= ACTION_UA && action <= ACTION_UD )
    fprintf( filewrite, "UNARMED\n" );
  else if ( action >= ACTION_TA && action <= ACTION_TD )
    fprintf( filewrite, "THRUST\n" );
  else if ( action >= ACTION_SA && action <= ACTION_SD )
    fprintf( filewrite, "SLASH\n" );
  else if ( action >= ACTION_CA && action <= ACTION_CD )
    fprintf( filewrite, "CHOP\n" );
  else if ( action >= ACTION_BA && action <= ACTION_BD )
    fprintf( filewrite, "BASH\n" );
  else if ( action >= ACTION_LA && action <= ACTION_LD )
    fprintf( filewrite, "LONGBOW\n" );
  else if ( action >= ACTION_XA && action <= ACTION_XD )
    fprintf( filewrite, "XBOW\n" );
  else if ( action >= ACTION_FA && action <= ACTION_FD )
    fprintf( filewrite, "FLING\n" );
  else if ( action >= ACTION_PA && action <= ACTION_PD )
    fprintf( filewrite, "PARRY\n" );
  else if ( action >= ACTION_ZA && action <= ACTION_ZD )
    fprintf( filewrite, "ZAP\n" );
}

//--------------------------------------------------------------------------------------------
void fgendef( FILE* filewrite, char* text, GENDER gender )
{
  /// @details ZZ> This function kinda mimics fprintf for the output of
  //     MALE FEMALE OTHER statements

  fprintf( filewrite, text );

  switch ( gender )
  {
    case GEN_MALE:   fprintf( filewrite, "MALE" ); break;
    case GEN_FEMALE: fprintf( filewrite, "FEMALE" ); break;
    default:
    case GEN_OTHER:  fprintf( filewrite, "OTHER" ); break;
  };

  fprintf( filewrite, "\n" );
}

//--------------------------------------------------------------------------------------------
void fpairof( FILE* filewrite, char* text, PAIR * ppair )
{
  /// @details ZZ> This function mimics fprintf in spitting out
  //     damage/stat pairs

  RANGE rng;

  fprintf( filewrite, text );
  if ( undo_pair_fp8( ppair, &rng ) )
  {
    fprintf( filewrite, "%4.2f-%4.2f\n", rng.ffrom, rng.fto );
  }
  else
  {
    fprintf( filewrite, "0\n" );
  }

}

//--------------------------------------------------------------------------------------------
void funderf( FILE* filewrite, char* text, char* usename )
{
  /// @details ZZ> This function mimics fprintf in spitting out
  //     a name with underscore spaces

  STRING tmpstr;

  str_encode( tmpstr, sizeof( tmpstr ), usename );
  fprintf( filewrite, "%s%s\n", text, tmpstr );
}

//--------------------------------------------------------------------------------------------
bool_t fget_message( FILE* fileread, MessageData_t * msglst )
{
  /// @details ZZ> This function loads a string into the message buffer, making sure it
  //     is null terminated.

  bool_t retval = bfalse;
  int cnt;
  char cTmp;
  STRING szTmp;

  if ( feof( fileread ) ) return retval;

  if ( msglst->total >= MAXTOTALMESSAGE ) return retval;

  if ( msglst->totalindex >= MESSAGEBUFFERSIZE )
  {
    msglst->totalindex = MESSAGEBUFFERSIZE - 1;
  }

  // a zero return value from fscanf() means that no fields were filled
  if ( 0 != fscanf( fileread, "%s", szTmp ) )
  {
    msglst->index[msglst->total] = msglst->totalindex;
    szTmp[255] = EOS;
    cTmp = szTmp[0];
    cnt = 1;
    while ( cTmp != 0 && msglst->totalindex < MESSAGEBUFFERSIZE - 1 )
    {
      if ( cTmp == '_' )  cTmp = ' ';
      msglst->text[msglst->totalindex] = cTmp;
      msglst->totalindex++;
      cTmp = szTmp[cnt];
      cnt++;
    }
    msglst->text[msglst->totalindex] = 0;  msglst->totalindex++;
    msglst->total++;

    retval = btrue;
  };

  return retval;
}

//--------------------------------------------------------------------------------------------
bool_t fget_next_message( FILE* fileread, MessageData_t * msglst )
{
  /// @details ZZ> This function loads a string into the message buffer, making sure it
  //     is null terminated.

  bool_t retval = bfalse;

  if ( fgoto_colon_yesno( fileread ) )
  {
    retval = fget_message( fileread, msglst );
  };

  return retval;
}

//--------------------------------------------------------------------------------------------
void fgoto_colon( FILE* fileread )
{
  /// @details BB> this calls fgoto_colon_buffer() with no buffer

  fgoto_colon_buffer( fileread, NULL, 0 );
}

//--------------------------------------------------------------------------------------------
bool_t fgoto_colon_yesno( FILE* fileread )
{
  /// @details BB> this calls fgoto_colon_yesno_buffer() with no buffer

  return fgoto_colon_yesno_buffer( fileread, NULL, 0 );
}

//--------------------------------------------------------------------------------------------
void fgoto_colon_buffer( FILE* fileread, char * buffer, size_t buffer_len )
{
  /// @details BB> This function scans the file for the next colon. If there is no colon,
  //     it forces the program to quit. It also stores any text
  //     before the colon in the optional array "buffer"

  if( !fgoto_colon_yesno_buffer( fileread, buffer, buffer_len ) )
  {
    // not enough colons in file!
    log_error( "There are not enough colons in file! (%s)\n", globalname );
  };

};

//--------------------------------------------------------------------------------------------
bool_t fgoto_colon_yesno_buffer( FILE* fileread, char * buffer, size_t buffer_len  )
{
  /// @details BB> This function scans the file for the next colon. If there is no colon,
  //     it returns bfalse. It also stores any text before the colon in the optional array "buffer"

  char cTmp, * pchar, * pchar_end;
  bool_t bfound = bfalse;

  pchar = pchar_end = buffer;
  if(pchar != NULL) pchar_end += buffer_len;

  while ( !feof( fileread ) )
  {
    cTmp = fgetc( fileread );
    if ( ':' == cTmp )
    {
      bfound = btrue;
      break;
    }
    else if ( EOF == cTmp )
    {
      // not enough colons in file!
      break;
    }
    else if ( '\n' ==  cTmp)
    {
      pchar = buffer;
      if(pchar < pchar_end)
      {
        *pchar = EOS;
      }
    }
    else
    {
      if(pchar < pchar_end)
      {
        *pchar++ = cTmp;
      }
    }
  }

  return bfound;
}

//--------------------------------------------------------------------------------------------
char fget_first_letter( FILE* fileread )
{
  /// @details ZZ> This function returns the next non-whitespace character

  char cTmp = EOS;
  bool_t bfound = bfalse;

  while ( !feof( fileread ) && !bfound )
  {
    cTmp = fgetc( fileread );
    bfound = isprint( cTmp ) && !isspace( cTmp );
  };

  return cTmp;
}

//--------------------------------------------------------------------------------------------
bool_t fget_name( FILE* fileread, char *szName, size_t lnName )
{
  /// @details ZZ> This function loads a string of up to MAXCAPNAMESIZE characters, parsing
  //     it for underscores.  The szName argument is rewritten with the null terminated
  //     string

  bool_t retval = bfalse;
  STRING szTmp;

  if ( feof( fileread ) ) return retval;

  // a zero return value from fscanf() means that no fields were filled
  if ( fget_string( fileread, szTmp, sizeof( szTmp ) ) )
  {
    str_decode( szName, lnName, szTmp );
    retval = btrue;
  };

  return retval;
}

//--------------------------------------------------------------------------------------------
bool_t fget_next_name( FILE* fileread, char *szName, size_t lnName )
{
  bool_t retval = bfalse;

  if ( fgoto_colon_yesno( fileread ) )
  {
    retval = fget_name( fileread, szName, lnName );
  };

  return retval;
};

//--------------------------------------------------------------------------------------------
int fget_int( FILE* fileread )
{
  int iTmp = 0;

  if ( feof( fileread ) ) return iTmp;

  fscanf( fileread, "%d", &iTmp );

  return iTmp;
};

//--------------------------------------------------------------------------------------------
int fget_next_int( FILE* fileread )
{
  int iTmp = 0;

  if ( fgoto_colon_yesno( fileread ) )
  {
    iTmp = fget_int( fileread );
  };

  return iTmp;
};

//--------------------------------------------------------------------------------------------
float fget_float( FILE* fileread )
{
  float fTmp = 0;

  if ( feof( fileread ) ) return fTmp;

  fscanf( fileread, "%f", &fTmp );

  return fTmp;
};

//--------------------------------------------------------------------------------------------
float fget_next_float( FILE* fileread )
{
  float fTmp = 0;

  if ( fgoto_colon_yesno( fileread ) )
  {
    fTmp = fget_float( fileread );
  };

  return fTmp;
};

//--------------------------------------------------------------------------------------------
Uint16 fget_fixed( FILE* fileread )
{
  float fTmp = 0;

  if ( feof( fileread ) ) return fTmp;

  fscanf( fileread, "%f", &fTmp );

  return FLOAT_TO_FP8( fTmp );
};

//--------------------------------------------------------------------------------------------
Uint16 fget_next_fixed( FILE* fileread )
{
  Uint16 iTmp = 0;

  if ( fgoto_colon_yesno( fileread ) )
  {
    iTmp = fget_fixed( fileread );
  };

  return iTmp;
};

//--------------------------------------------------------------------------------------------
bool_t fget_bool( FILE* fileread )
{
  bool_t bTmp = bfalse;
  char cTmp;

  if ( feof( fileread ) ) return bTmp;

  cTmp = fget_first_letter( fileread );
  switch ( toupper( cTmp ) )
  {
    case 'T': bTmp = btrue; break;
    case 'F': bTmp = bfalse; break;
  };

  return bTmp;
};

//--------------------------------------------------------------------------------------------
bool_t fget_next_bool( FILE* fileread )
{
  bool_t bTmp = 0;

  if ( fgoto_colon_yesno( fileread ) )
  {
    bTmp = fget_bool( fileread );
  };

  return bTmp;
};

//--------------------------------------------------------------------------------------------
GENDER fget_gender( FILE* fileread )
{
  char cTmp;
  GENDER retval = GEN_RANDOM;
  if ( feof( fileread ) ) return retval;

  cTmp = fget_first_letter( fileread );
  switch ( toupper( cTmp ) )
  {
    case 'F': retval = GEN_FEMALE; break;
    case 'M': retval = GEN_MALE;   break;
    case 'O': retval = GEN_OTHER;  break;
    case 'R': retval = GEN_RANDOM; break;
  }

  return retval;
};

//--------------------------------------------------------------------------------------------
GENDER fget_next_gender( FILE* fileread )
{
  GENDER retval = GEN_RANDOM;

  if ( fgoto_colon_yesno( fileread ) )
  {
    retval = fget_gender( fileread );
  };

  return retval;
};

//--------------------------------------------------------------------------------------------
ACTION fget_action( FILE* fileread )
{
  char cTmp;
  ACTION retval = ACTION_DA;
  if ( feof( fileread ) ) return retval;


  cTmp = fget_first_letter( fileread );
  return what_action( cTmp );
}

//--------------------------------------------------------------------------------------------
ACTION fget_next_action( FILE* fileread )
{
  ACTION retval = ACTION_DA;

  if ( fgoto_colon_yesno( fileread ) )
  {
    retval = fget_action( fileread );
  };

  return retval;
};

//--------------------------------------------------------------------------------------------
PRTALPHA fget_prttype( FILE * fileread )
{
  char cTmp;
  PRTALPHA retval = PRTTYPE_SOLID;

  if ( feof( fileread ) ) return retval;

  cTmp = fget_first_letter( fileread );
  switch ( toupper( cTmp ) )
  {
    case 'L': retval = PRTTYPE_LIGHT; break;
    case 'S': retval = PRTTYPE_SOLID; break;
    case 'T': retval = PRTTYPE_ALPHA; break;
  };

  return retval;
};

//--------------------------------------------------------------------------------------------
PRTALPHA fget_next_prttype( FILE * fileread )
{
  PRTALPHA retval = PRTTYPE_SOLID;

  if ( fgoto_colon_yesno( fileread ) )
  {
    retval = fget_prttype( fileread );
  };

  return retval;
};

//--------------------------------------------------------------------------------------------
bool_t fget_string( FILE* fileread, char *szLine, size_t lnLine )
{
  /// @details BB> this is a size-sensitive replacement for fscanf(fileread, "%s", &szLine)

  size_t len;
  char cTmp, *pout = szLine, *plast = szLine + lnLine;

  if ( feof( fileread ) ) return bfalse;
  if ( NULL == szLine || lnLine <= 1 ) return bfalse;

  // read past any initial spaces
  cTmp = EOS;
  while ( !feof( fileread ) )
  {
    cTmp = fgetc( fileread );
    if ( !isspace( cTmp ) ) break;
  }

  // stop at the next space or control character
  len = 0;
  while ( !feof( fileread ) && pout < plast )
  {
    if ( isspace( cTmp ) || !isprint( cTmp ) )
    {
      ungetc( cTmp, fileread );
      break;
    };
    *pout = cTmp;
    pout++;
    len++;
    cTmp = fgetc( fileread );
  };

  if ( pout < plast ) *pout = EOS;

  return len != 0;
}

//--------------------------------------------------------------------------------------------
bool_t fget_next_string( FILE* fileread, char *szLine, size_t lnLine )
{
  bool_t retval = bfalse;

  if ( fgoto_colon_yesno( fileread ) )
  {
    retval = fget_string( fileread, szLine, lnLine );
  };

  return retval;
}

//--------------------------------------------------------------------------------------------
EGO_CONST char * inherit_fname(EGO_CONST char * szObjPath, EGO_CONST char * szObject, EGO_CONST char *szFname )
{
  static STRING ret_fname;
  FILE * loc_pfile;
  STRING loc_fname, inherit_line;
  bool_t found;

  STRING ifile;
  STRING itype;
  Uint32 icrc = (~(Uint32)0);

  FS_FIND_INFO fs_finfo;

  fs_find_info_new( &fs_finfo );

  // blank the static string
  ret_fname[0] = EOS;

  if(NULL == szObject)
  {
    // do not search
    strcpy(ret_fname, szObjPath);
    str_append_slash(ret_fname, sizeof(ret_fname));
    strcat(ret_fname, szFname);
    return ret_fname;
  }

  strcpy(loc_fname, szObjPath);
  str_append_slash(loc_fname, sizeof(loc_fname));
  strcat(loc_fname, szObject);
  str_append_slash( loc_fname, sizeof(loc_fname) );
  strcat(loc_fname, "inherit.txt");
  loc_pfile = fs_fileOpen(PRI_NONE, "loc_fname()", loc_fname, "r");
  if(NULL == loc_pfile)
  {
    strcpy(ret_fname, szObjPath);
    str_append_slash(ret_fname, sizeof(ret_fname));
    strcat(ret_fname, szObject);
    str_append_slash(ret_fname, sizeof(ret_fname));
    strcat(ret_fname, szFname);

    return ret_fname;
  };

   // try to match the given file
  found = bfalse;
  while( fget_next_string( loc_pfile, ifile, sizeof(STRING) ) )
  {
    if( 0 == strcmp(ifile, szFname) )
    {
      // "-"
      fget_string( loc_pfile, inherit_line, sizeof(STRING) );

      // CRC
      icrc = fget_int( loc_pfile );

      // "-"
      fget_string( loc_pfile, inherit_line, sizeof(STRING) );

      // type
      fget_string( loc_pfile, itype, sizeof(STRING) );

      found = btrue;

      break;
    }
  }

  if(!found || 0 == strncmp( ifile, "data" , 4 ) )
  {
    strcpy(ret_fname, szObjPath);
    str_append_slash(ret_fname, sizeof(ret_fname));
    strcat(ret_fname, szObject);
    str_append_slash(ret_fname, sizeof(ret_fname));
    strcat(ret_fname, szFname);
  }
  else
  {
    EGO_CONST char * name_ptr = NULL;

    found = bfalse;

    // the shared resources have the CRC second so that they can be grouped by type
    sprintf(ret_fname, "*%u%s", icrc, itype);

    if( 0 == strcmp(".wav", itype) )
    {
      // search in the sounds directory

      name_ptr = fs_findFirstFile( &fs_finfo, "objects" SLASH_STRING "_sounds" SLASH_STRING, ret_fname, NULL);

      if(NULL != name_ptr)
      {
        sprintf(ret_fname, "objects" SLASH_STRING "_sounds" SLASH_STRING "%s", name_ptr);
        found = btrue;
      }

      fs_findClose(&fs_finfo);
    }
    else if ( 0 == strncmp( ifile, "icon" , 4 ) && 0 == strcmp(".bmp", itype) )
    {
      // search in the icons directory

      name_ptr = fs_findFirstFile( &fs_finfo, "objects" SLASH_STRING "_icons" SLASH_STRING, ret_fname, NULL);

      if(NULL != name_ptr)
      {
        sprintf(ret_fname, "objects" SLASH_STRING "_icons" SLASH_STRING "%s", name_ptr);
        found = btrue;
      }

      fs_findClose(&fs_finfo);
    }
    else if ( 0 == strncmp( ifile, "part" , 4 ) && 0 == strcmp(".txt", itype) )
    {
      // search in the particles directory

      name_ptr = fs_findFirstFile( &fs_finfo, "objects" SLASH_STRING "_particles" SLASH_STRING, ret_fname, NULL);

      if(NULL != name_ptr)
      {
        sprintf(ret_fname, "objects" SLASH_STRING "_particles" SLASH_STRING "%s", name_ptr);
        found = btrue;
      }

      fs_findClose(&fs_finfo);
    }
    else
    {
      STRING tmpfname;

      // number is first for the non-shared resources
      sprintf(ret_fname, "%u*%s", icrc, itype);

      strcpy(tmpfname, "objects");
      str_append_slash( tmpfname, sizeof(tmpfname) );
      strcat(tmpfname, szObject );
      str_append_slash( tmpfname, sizeof(tmpfname) );

      name_ptr = fs_findFirstFile( &fs_finfo, tmpfname, ret_fname, NULL);

      if(NULL != name_ptr)
      {
        sprintf(ret_fname, "%s%s", tmpfname, name_ptr);
        found = btrue;
      }

      fs_findClose(&fs_finfo);
    }

    if(!found)
    {
      log_warning("Could not find file <\"%s\", \"%s\", \"%s\"> with CRC %s\n", szObjPath, szObject, szFname, ret_fname );
    }

  };


  fs_fileClose(loc_pfile);

  return ret_fname;

}

//--------------------------------------------------------------------------------------------
retval_t util_calculateCRC(char * filename, Uint32 seed, Uint32 * pCRC)
{
  Uint32 tmpint, tmpCRC;
  FILE * pfile = NULL;

  if( NULL ==pCRC )
  {
    log_info("util_calculateCRC() - \n\tCalled with invalid parameters.\n");
    return rv_error;
  }

  if( !VALID_CSTR(filename) )
  {
    log_info("util_calculateCRC() - \n\tCalled null filename.\n");
    return rv_error;
  }

  pfile = fs_fileOpen(PRI_WARN, "util_calculateCRC()", filename, "rb");
  if(NULL ==pfile)
  {
    return rv_fail;
  }

  log_info("NET INFO: util_calculateCRC() - \n\t\"%s\" has CRC... ", filename);

  tmpCRC = seed;
  while(!feof(pfile))
  {
    if( 0!= fread(&tmpint, sizeof(Uint32), 1, pfile) )
    {
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
      tmpint = SDL_Swap32(tmpint);
#endif
      tmpCRC = ((tmpCRC + tmpint) * 0x0019660DL) + 0x3C6EF35FL;
    };
  }
  fs_fileClose(pfile);

  log_info("0x%08x\n", tmpCRC);

  *pCRC = tmpCRC;

  return rv_succeed;
}

//--------------------------------------------------------------------------------------------
Uint32 generate_unsigned( Uint32 * seed, PAIR * ppair )
{
  /// @details ZZ> This function generates a random number

  Uint32 itmp = 0;

  if ( NULL != ppair )
  {
    itmp = ppair->ibase;

    if ( ppair->irand > 1 )
    {
      itmp += ego_rand_32(seed) % ppair->irand;
    }
  }
  else
  {
    itmp = ego_rand_32(seed);
  }

  return itmp;
}

//--------------------------------------------------------------------------------------------
Sint32 generate_signed( Uint32 * seed, PAIR * ppair )
{
  /// @details ZZ> This function generates a random number

  Sint32 itmp = 0;

  if ( NULL != ppair )
  {
    itmp = ppair->ibase;

    if ( ppair->irand > 1 )
    {
      itmp += ego_rand_32(seed) % ppair->irand;
      itmp -= ppair->irand >> 1;
    }
  }
  else
  {
    itmp = ego_rand_32(seed);
  }

  return itmp;
}


//--------------------------------------------------------------------------------------------
Sint32 generate_dither( Uint32 * seed, PAIR * ppair, Uint16 strength_fp8 )
{
  /// @details ZZ> This function generates a random number

  Sint32 itmp = 0;

  if ( NULL != ppair && ppair->irand > 1 )
  {
    itmp = ego_rand_32(seed);
    itmp %= ppair->irand;
    itmp -= ppair->irand >> 1;

    if ( strength_fp8 != INT_TO_FP8( 1 ) )
    {
      itmp *= strength_fp8;
      itmp = FP8_TO_FLOAT( itmp );
    };

  };


  return itmp;

}


//--------------------------------------------------------------------------------------------
void make_randie()
{
  /// @details ZZ> This function makes the random number table

  int tnc, cnt;

  // seed the random number generator
  srand( time(NULL) );

  // Fill in the basic values
  cnt = 0;
  while ( cnt < RANDIE_COUNT )
  {
    randie[cnt] = rand() << 1;
    cnt++;
  }


  // Keep adjusting those values
  tnc = 0;
  while ( tnc < 20 )
  {
    cnt = 0;
    while ( cnt < RANDIE_COUNT )
    {
      randie[cnt] += ( Uint16 ) rand();
      cnt++;
    }
    tnc++;
  }
}


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t fput_next( FILE* filewrite, EGO_CONST char * comment )
{
  int written;
  if( NULL == filewrite) return bfalse;
  if( !VALID_CSTR(comment) ) comment = "";

  written = fprintf( filewrite, "\n%s: ", comment );

  return written > 0;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t fput_idsz( FILE* filewrite, IDSZ data )
{
  int written;

  if(NULL == filewrite) return bfalse;

  written = fprintf( filewrite, "[%s] ", undo_idsz(data) );

  return written > 0;
}



//--------------------------------------------------------------------------------------------
bool_t fput_next_idsz( FILE* filewrite, EGO_CONST char * comment, IDSZ data )
{
  if( !fput_next(filewrite, comment) ) return bfalse;
  fput_idsz(filewrite, data);

  return btrue;
}


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t fput_int( FILE* filewrite, int data )
{
  int written;
  if(NULL == filewrite) return bfalse;

  written = fprintf( filewrite, "%d ", data );

  return written > 0;
}


//--------------------------------------------------------------------------------------------
bool_t fput_next_int( FILE* filewrite, EGO_CONST char * comment, int data )
{
  if( !fput_next(filewrite, comment) ) return bfalse;
  fput_int(filewrite, data);

  return btrue;
}


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t fput_float( FILE* filewrite, float data )
{
  int written;
  if(NULL == filewrite) return bfalse;

  written = fprintf( filewrite, "%f ", data );

  return written > 0;
}



//--------------------------------------------------------------------------------------------
bool_t fput_next_float( FILE* filewrite, EGO_CONST char * comment, float data )
{
  if( !fput_next(filewrite, comment) ) return bfalse;
  fput_float(filewrite, data);

  return btrue;
}


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t fput_fixed( FILE* filewrite, Uint16 data )
{
  int written;
  if(NULL == filewrite) return bfalse;

  written = fprintf( filewrite, "%d ", FP8_TO_INT(data) );

  return written > 0;
}



//--------------------------------------------------------------------------------------------
bool_t fput_next_fixed( FILE* filewrite, EGO_CONST char * comment, Uint16 data )
{
  if( !fput_next(filewrite, comment) ) return bfalse;
  fput_fixed(filewrite, data);

  return btrue;
}


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t fput_bool( FILE* filewrite, bool_t data )
{
  int written;
  if(NULL == filewrite) return bfalse;

  written = fprintf( filewrite, "%s ", data ? "TRUE" : "FALSE" );

  return written > 0;
}



//--------------------------------------------------------------------------------------------
bool_t fput_next_bool( FILE* filewrite, EGO_CONST char * comment, bool_t data )
{
  if( !fput_next(filewrite, comment) ) return bfalse;
  fput_bool(filewrite, data);

  return btrue;
}


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t fput_gender( FILE* filewrite, enum e_gender data )
{
  int written;
  char * pchar;

  if(NULL == filewrite) return bfalse;

  pchar = "RANDOM";
  switch ( data )
  {
    case GEN_MALE:   pchar = "MALE";   break;
    case GEN_FEMALE: pchar = "FEMALE"; break;
    case GEN_OTHER:  pchar = "OTHER";  break;
    case GEN_RANDOM: pchar = "RANDOM"; break;
  };

  written = fprintf( filewrite, "%s ", pchar );

  return written > 0;
}



//--------------------------------------------------------------------------------------------
bool_t fput_next_gender( FILE* filewrite, EGO_CONST char * comment, enum e_gender data )
{
  if( !fput_next(filewrite, comment) ) return bfalse;
  fput_gender(filewrite, data);

  return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t fput_damage_char( FILE* filewrite, enum e_damage data )
{
  int written;
  char cTmp;

  if(NULL == filewrite) return bfalse;

  cTmp = 'N';
  switch ( data )
  {
    case DAMAGE_SLASH: cTmp = 'S'; break;
    case DAMAGE_CRUSH: cTmp = 'C'; break;
    case DAMAGE_POKE:  cTmp = 'P';  break;
    case DAMAGE_HOLY:  cTmp = 'H';  break;
    case DAMAGE_EVIL:  cTmp = 'E';  break;
    case DAMAGE_FIRE:  cTmp = 'F';  break;
    case DAMAGE_ICE:   cTmp = 'I';   break;
    case DAMAGE_ZAP:   cTmp = 'Z';   break;
    default:           cTmp = 'N';  break;
  };

  written = fprintf( filewrite, "%c ", cTmp );

  return written > 0;
}

//--------------------------------------------------------------------------------------------
bool_t fput_damage( FILE* filewrite, enum e_damage data )
{
  int written;
  char * pchar;

  if(NULL == filewrite) return bfalse;

  pchar = "NONE";
  switch ( data )
  {
    case DAMAGE_SLASH: pchar = "SLASH"; break;
    case DAMAGE_CRUSH: pchar = "CRUSH"; break;
    case DAMAGE_POKE:  pchar = "POKE";  break;
    case DAMAGE_HOLY:  pchar = "HOLY";  break;
    case DAMAGE_EVIL:  pchar = "EVIL";  break;
    case DAMAGE_FIRE:  pchar = "FIRE";  break;
    case DAMAGE_ICE:   pchar = "ICE";   break;
    case DAMAGE_ZAP:   pchar = "ZAP";   break;
    default:           pchar = "NONE";  break;
  };

  written = fprintf( filewrite, "%s ", pchar );

  return written > 0;
}



//--------------------------------------------------------------------------------------------
bool_t fput_next_damage( FILE* filewrite, EGO_CONST char * comment, enum e_damage data )
{
  if( !fput_next(filewrite, comment) ) return bfalse;
  fput_damage(filewrite, data);

  return btrue;
}


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t fput_blud( FILE* filewrite, enum e_blud_level data )
{
  int written;
  char * pchar;

  if(NULL == filewrite) return bfalse;

  pchar = "FALSE";
  switch ( data )
  {
    case BLUD_NORMAL: pchar = "TRUE"; break;
    case BLUD_NONE:   pchar = "FALSE"; break;
    case BLUD_ULTRA:  pchar = "ULTRABLUDY"; break;
  };

  written = fprintf( filewrite, "%s ", pchar );

  return written > 0;
}



//--------------------------------------------------------------------------------------------
bool_t fput_next_blud( FILE* filewrite, EGO_CONST char * comment, enum e_blud_level data )
{
  if( !fput_next(filewrite, comment) ) return bfalse;
  fput_blud(filewrite, data);

  return btrue;
}


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t fput_dynamode( FILE* filewrite, enum e_dyna_mode data )
{
  int written;
  char * pchar;

  if(NULL == filewrite) return bfalse;

  pchar = "FALSE";
  switch ( data )
  {
    case DYNA_ON:    pchar = "TRUE";  break;
    case DYNA_OFF:   pchar = "FALSE"; break;
    case DYNA_LOCAL: pchar = "LOCAL"; break;
  };

  written = fprintf( filewrite, "%s ", pchar );

  return written > 0;
}



//--------------------------------------------------------------------------------------------
bool_t fput_next_dynamode( FILE* filewrite, EGO_CONST char * comment, enum e_dyna_mode data )
{
  if( !fput_next(filewrite, comment) ) return bfalse;
  fput_dynamode(filewrite, data);

  return btrue;
}


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t fput_name( FILE* filewrite, char * data )
{
  int written;
  STRING szName;

  if(NULL == filewrite) return bfalse;

  str_encode( szName, sizeof(szName), data );

  written = fprintf( filewrite, "%s ", szName );

  return written > 0;
}



//--------------------------------------------------------------------------------------------
bool_t fput_next_name( FILE* filewrite, EGO_CONST char * comment, char * data )
{
  if( !fput_next(filewrite, comment) ) return bfalse;

  fput_name(filewrite, data);

  return btrue;
}


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t fput_string( FILE* filewrite, char * data )
{
  int written;

  if(NULL == filewrite) return bfalse;

  written = fprintf( filewrite, "%s ", data );

  return written > 0;
}



//--------------------------------------------------------------------------------------------
bool_t fput_next_string( FILE* filewrite, EGO_CONST char * comment, char * data )
{
  if( !fput_next(filewrite, comment) ) return bfalse;
  fput_string(filewrite, data);

  return btrue;
}


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t fput_alpha_type( FILE* filewrite, enum e_particle_alpha_type data )
{
  int written;
  char * pchar;

  if(NULL == filewrite) return bfalse;

  pchar = "SOLID";
  switch( data )
  {
    case PRTTYPE_LIGHT: pchar = "LIGHT"; break;
    case PRTTYPE_SOLID: pchar = "SOLID"; break;
    case PRTTYPE_ALPHA: pchar = "ALPHA"; break;
  }

  written = fprintf( filewrite, "%s ", pchar );

  return written > 0;
}



//--------------------------------------------------------------------------------------------
bool_t fput_next_alpha_type( FILE* filewrite, EGO_CONST char * comment, enum e_particle_alpha_type data )
{
  if( !fput_next(filewrite, comment) ) return bfalse;
  fput_alpha_type(filewrite, data);

  return btrue;
}


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t fput_action( FILE* filewrite, enum e_Action data )
{
  int written;
  char * pchar;

  if(NULL == filewrite) return bfalse;

 pchar = "DANCE";
 if ( data >= ACTION_DA && data <= ACTION_DD )
    pchar = "WALK";
  else if ( data >= ACTION_UA && data <= ACTION_UD )
    pchar = "UNARMED";
  else if ( data >= ACTION_TA && data <= ACTION_TD )
    pchar = "THRUST";
  else if ( data >= ACTION_SA && data <= ACTION_SD )
    pchar = "SLASH";
  else if ( data >= ACTION_CA && data <= ACTION_CD )
    pchar = "CHOP";
  else if ( data >= ACTION_BA && data <= ACTION_BD )
    pchar = "BASH";
  else if ( data >= ACTION_LA && data <= ACTION_LD )
    pchar = "LONGBOW";
  else if ( data >= ACTION_XA && data <= ACTION_XD )
    pchar = "XBOW";
  else if ( data >= ACTION_FA && data <= ACTION_FD )
    pchar = "FLING";
  else if ( data >= ACTION_PA && data <= ACTION_PD )
    pchar = "PARRY";
  else if ( data >= ACTION_ZA && data <= ACTION_ZD )
    pchar = "ZAP";

  written = fprintf( filewrite, "%s ", pchar );

  return written > 0;
}



//--------------------------------------------------------------------------------------------
bool_t fput_next_action( FILE* filewrite, EGO_CONST char * comment, enum e_Action data )
{
  if( !fput_next(filewrite, comment) ) return bfalse;
  fput_action(filewrite, data);

  return btrue;
}


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t fput_message( FILE* filewrite, MessageData_t * data, int num )
{
  int written, index;
  char * pchar;
  STRING szMessage;

  if(NULL == filewrite) return bfalse;

  index = data->index[num];

  pchar = "_";
  if(MAXTOTALMESSAGE != index)
  {
    pchar = data->text + index;
  }

  str_encode( szMessage, sizeof(szMessage), pchar );

  written = fprintf( filewrite, "%s", szMessage );

  return written > 0;
}



//--------------------------------------------------------------------------------------------
bool_t fput_next_message( FILE* filewrite, EGO_CONST char * comment, MessageData_t * data, int num )
{
  if( !fput_next(filewrite, comment) ) return bfalse;

  fput_message(filewrite, data, num);

  return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t fput_expansion( FILE* filewrite, char * idsz_string, int value )
{
  if(NULL == filewrite) return bfalse;

  fprintf( filewrite, "[%4s] ", idsz_string );
  fput_int ( filewrite, value );

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t fput_next_expansion( FILE* filewrite, EGO_CONST char * comment, char * idsz_string, int value )
{
  if( !fput_next(filewrite, comment) ) return bfalse;

  fput_expansion(filewrite, idsz_string, value);

  return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t fput_pair( FILE* filewrite, PAIR * data )
{
  RANGE rng;

  if(NULL == filewrite) return bfalse;

  if ( undo_pair_fp8( data, &rng ) )
  {
    fprintf( filewrite, "%4.2f-%4.2f ", rng.ffrom, rng.fto );
  }
  else
  {
    fprintf( filewrite, "0 " );
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t fput_next_pair( FILE* filewrite, EGO_CONST char * comment, PAIR * data )
{
  if( !fput_next(filewrite, comment) ) return bfalse;

  fput_pair(filewrite, data);

  return btrue;
}
