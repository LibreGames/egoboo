//********************************************************************************************
//* Egoboo - egoboo_strutil.c
//*
//* String manipulation functions.
//*
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

#include "egoboo_strutil.h"
#include "egoboo_config.h"

// str_trim remove all space and tabs in the beginning and at the end of the string
void str_trim( char *pStr )
{
  Sint32 DebPos, EndPos, CurPos;

  if ( NULL == pStr  )
  {
    return;
  }
  // look for the first character in string
  DebPos = 0;
  while ( isspace( pStr[DebPos] ) && pStr[DebPos] != 0 )
  {
    DebPos++;
  }

  // look for the last character in string
  CurPos = DebPos;
  while ( pStr[CurPos] != 0 )
  {
    if ( !isspace( pStr[CurPos] ) )
    {
      EndPos = CurPos;
    }
    CurPos++;
  }

  if ( DebPos != 0 )
  {
    // shift string left
    for ( CurPos = 0; CurPos <= ( EndPos - DebPos ); CurPos++ )
    {
      pStr[CurPos] = pStr[CurPos + DebPos];
    }
    pStr[CurPos] = 0;
  }
  else
  {
    pStr[EndPos + 1] = 0;
  }
}

char * str_convert_underscores( char *strout, size_t insize, char * strin )
{
  char *pin = strin, *pout = strout, *plast = pout + insize;

  if ( NULL == strin || NULL == strout || 0 == insize ) return NULL;

  while ( pout < plast && '\0' != *pin )
  {
    *pout = *pin;
    if ( '_' == *pout ) *pout = ' ';
    pout++;
    pin++;
  };

  if ( pout < plast ) *pout = '\0';

  return strout;
};

char * str_convert_spaces( char *strout, size_t insize, char * strin )
{
  char chrlast = 0;
  char *pin = strin, *pout = strout, *plast = pout + insize;

  if ( NULL == strin || NULL == strout || 0 == insize ) return NULL;

  while ( pout < plast && '\0' != *pin )
  {
    if ( !isspace( *pin ) && isprint( *pin ) )
    {
      chrlast = *pout = tolower( *pin );
      pin++;
      pout++;
    }
    else if ( ' ' == *pin )
    {
      chrlast = *pout = '_';
      pin++;
      pout++;
    }
    else if ( '\t' == *pin )
    {
      chrlast = *pout = '~';
      pin++;
      pout++;
    }
    else if ( isspace( *pin ) )
    {
      chrlast = *pout = '_';
      pin++;
      pout++;
    }
    else if ( chrlast != '_' )
    {
      chrlast = *pout = '_';
      pin++;
      pout++;
    }
    else
    {
      pin++;
    }
  };

  if ( pout < plast ) *pout = '\0';

  return strout;
};


char * str_convert_net(char * str, size_t size)
{
  size_t i;

  if(NULL == str || '\0' == str[0]) return str;
  if(SLASH_CHAR == NET_SLASH_CHAR) return str;

  for(i=0; i < size; i++)
  {
    if(SLASH_CHAR == str[i])
    {
      str[i] = NET_SLASH_CHAR;
    }
  }

  return str;
}

char * str_convert_sys(char * str, size_t size)
{
  size_t i;

  if(NULL == str || '\0' == str[0]) return str;
  if(SLASH_CHAR == NET_SLASH_CHAR) return str;

  for(i=0; i < size; i++)
  {
    if(NET_SLASH_CHAR == str[i])
    {
      str[i] = SLASH_CHAR;
    }
  }

  return str;
}

char * str_append_net_slash(char * str, size_t size)
{
  size_t len;

  if(NULL == str || '\0' == str[0]) return str;

  len = strlen( str );
  if ( str[len-1] != '/' && str[len-1] != '\\' )
  {
    strncat(str, NET_SLASH_STRING, size);
  }

  return str;
}

char * str_append_slash(char * str, size_t size)
{
  size_t len;

  if(NULL == str || '\0' == str[0]) return NULL;

  len = strlen( str );
  if ( str[len-1] != '/' && str[len-1] != '\\' )
  {
    strncat(str, SLASH_STRING, size);
  }

  return str;
}