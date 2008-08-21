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
/// @brief Egoboo String Utilities.
/// @details String manipulation functions.

#include "egoboo_types.h"
#include "egoboo_config.h"

#include <string.h>
#include <ctype.h>

void str_trim( char *pStr );
char * str_decode( char *strout, size_t insize, char * strin );
char * str_encode( char *strout, size_t insize, char * strin );
char * str_convert_slash_net( char * str, size_t size );
char * str_convert_slash_sys( char * str, size_t size );

char * str_append_slash(char * str, size_t size);
char * str_append_slash_net(char * str, size_t size);

