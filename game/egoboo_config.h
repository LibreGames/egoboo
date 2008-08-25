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
/// @brief System-dependent global parameters.
///   @todo  move more of the typical config stuff to this file.
///   @todo  add in linux and mac stuff.
///   @todo  some of this stuff is compiler dependent, rather than system dependent.


#pragma once

#define EGOBOO_CONFIG

#include <SDL_endian.h>

// deal with gcc's the warnings about const on return types in C
#ifdef __cplusplus
#    define EGO_CONST const
#else
#    define EGO_CONST
#endif


// localize the inline keyword to the compiler
#if defined(_MSC_VER)

    // In MS visual C, the "inline" keyword seems to be depreciated. Must to be promoted to "_inline" of "__inline"
#    define INLINE __inline

    // Turn off warnings that we don't care about.

#    pragma warning(disable : 4090) // '=' : different 'EGO_CONST' qualifiers (totally unimportant in C)
#    pragma warning(disable : 4200) // zero-sized array in struct/union (used in the md2 loader)
#    pragma warning(disable : 4201) // nameless struct/union (nameless unions and nameless structs used in defining the vector structs)
#    pragma warning(disable : 4204) // non-constant aggregate initializer (used to simplify some vector initializations)
#    pragma warning(disable : 4244) // conversion from 'double' to 'float'
#    pragma warning(disable : 4305) // truncation from 'double' to 'float'

#    ifndef _DEBUG
#        pragma warning(disable : 4554) // possibly operator precendence error
#    endif

    // Visual C defines snprintf as _snprintf; that's kind of a pain, so redefine it here
#    define snprintf _snprintf

    // Visual C does not have native support for vsnprintf, it is implemented as _vsnprintf
#    define vsnprintf _vsnprintf

#else

#    define INLINE static inline

#endif

#define NET_SLASH_STRING "/"
#define NET_SLASH_CHAR   '/'

// end-of-string character. assume standard null terminated string
#define EOS '\0'
#define NULL_STRING { EOS }

#define EMPTY_CSTR(PSTR) ((NULL!=PSTR) && (EOS == PSTR[0]))
#define VALID_CSTR(PSTR) ((NULL!=PSTR) && (EOS != PSTR[0]))

#ifdef WIN32

    // Speeds up compile times a bit.  We don't need everything in windows.h
#    define WIN32_LEAN_AND_MEAN

    // special win32 macro that lets windows know that you are going to be
    // starting from a console.  This is useful because you can get real-time
    // output to the screen just by using printf()!
#    ifdef _CONSOLE
#        define CONSOLE_MODE
#    else
#        undef  CONSOLE_MODE
#    endif

#    define SLASH_STRING "\\"
#    define SLASH_CHAR   '\\'

#else

#    define SLASH_STRING "/"
#    define SLASH_CHAR   '/'

#endif

#undef DEBUG_PROFILE
#undef DEBUG_ATTRIB
#undef DEBUG_MESH_NORMALS
#undef DEBUG_CHR_NORMALS
#undef DEBUG_BBOX

#undef DEBUG_UPDATE_CLAMP
#undef DEBUG_MESHFX
#undef DEBUG_CVOLUME
