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
//*    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
//*
//********************************************************************************************

/* Egoboo - egoboostrutil.c
 * String manipulation functions.  Not currently in use.
 */

#include "egoboo_strutil.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// TrimStr remove all space and tabs in the beginning and at the end of the string
void TrimStr( char *pStr )
{
    Sint32 DebPos, CurPos, EndPos = 0;
    if ( pStr == NULL )
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

//--------------------------------------------------------------------------------------------
char * str_decode( char *strout, size_t insize, char * strin )
{
    /// @details BB> str_decode converts a string from "storage mode" to an actual string

    char *pin = strin, *pout = strout, *plast = pout + insize;
    if ( NULL == strin || NULL == strout || 0 == insize ) return NULL;

    while ( pout < plast && '\0' != *pin )
    {
        *pout = *pin;
        if      ( '_' == *pout ) *pout = ' ';
        else if ( '~' == *pout ) *pout = '\t';

        pout++;
        pin++;
    };
    if ( pout < plast ) *pout = '\0';

    return strout;
}

//--------------------------------------------------------------------------------------------
char * str_encode( char *strout, size_t insize, char * strin )
{
    /// @details BB> str_encode converts an actual string to "storage mode"

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
}

//--------------------------------------------------------------------------------------------
void make_newloadname( char *modname, char *appendname, char *newloadname )
{
    // ZZ> This function takes some names and puts 'em together
    int cnt, tnc;
    char ctmp;

    cnt = 0;
    ctmp = modname[cnt];

    while ( ctmp != 0 )
    {
        newloadname[cnt] = ctmp;
        cnt++;
        ctmp = modname[cnt];
    }

    tnc = 0;
    ctmp = appendname[tnc];

    while ( ctmp != 0 )
    {
        newloadname[cnt] = ctmp;
        cnt++;
        tnc++;
        ctmp = appendname[tnc];
    }

    newloadname[cnt] = 0;
}

