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
//*    along with Egoboo.  If not, see <http:// www.gnu.org/licenses/>.
//*
//********************************************************************************************

/* Egoboo - egoboostrutil.h
 * String manipulation functions.  Not currently in use.
 */

#include "egoboo_typedef.h"
#include <string.h>
#include <ctype.h>

#define VALID_CSTR(X) ( ( NULL != (X) ) && ( '\0' != (X)[0] ) )

extern void TrimStr( char *pStr );

void   make_newloadname( const char *modname, const char *appendname, char *newloadname );

char * str_decode( char *strout, size_t insize, const char * strin );
char * str_encode( char *strout, size_t insize, const char * strin );

#define _EGOBOOSTRUTIL_H_

