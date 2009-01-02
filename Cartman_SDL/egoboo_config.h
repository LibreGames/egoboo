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

#define NET_SLASH_STRING "/"
#define NET_SLASH_CHAR   '/'

// end-of-string character. assume standard null terminated string
#define EOS '\0'
#define NULL_STRING { EOS }

#define EMPTY_CSTR(PSTR) ((NULL!=PSTR) && (EOS == PSTR[0]))
#define VALID_CSTR(PSTR) ((NULL!=PSTR) && (EOS != PSTR[0]))

#ifdef _MSC_VER
// Turn off warnings that we don't care about.
#    pragma warning(disable : 4305) // truncation from 'double' to 'float'
#    pragma warning(disable : 4244) // conversion from 'double' to 'float'
#    pragma warning(disable : 4554) // possibly operator precendence error
#    pragma warning(disable : 4201) // nonstandard extension used : nameless struct/union

     // the VisualC runtimes define snprintf as _snprintf; that's kind of a pain, so redefine it here
#    define snprintf  _snprintf
#    define vsnprintf _vsnprintf

#endif

#ifdef __unix__
#    include <unistd.h>
#endif

#ifdef WIN32
// Speeds up compile times a bit.  We don't need everything in windows.h
#    define WIN32_LEAN_AND_MEAN
#endif

#ifdef WIN32
#    define SLASH_STR "\\"
#    define SLASH_CHR '\\'
#else
#    define SLASH_STR "/"
#    define SLASH_CHR '/'
#endif
