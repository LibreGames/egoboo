/*******************************************************************************
*  PLATFORM.H                                                                  *
*      - Contains platform and compiler dependant stuff                        *
*                                                                              *
*  EGOBOO - Game                                                               *
*      (c) The Egoboo Team                                                     *
*                                                                              *
*  This program is free software; you can redistribute it and/or modify        *
*  it under the terms of the GNU General Public License as published by        *
*  the Free Software Foundation; either version 2 of the License, or           *
*  (at your option) any later version.                                         *
*                                                                              *
*  This program is distributed in the hope that it will be useful,             *
*  but WITHOUT ANY WARRANTY; without even the implied warranty of              *
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               *
*  GNU Library General Public License for more details.                        *
*                                                                              *
*  You should have received a copy of the GNU General Public License           *
*  along with this program; if not, write to the Free Software                 *
*  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  *
*******************************************************************************/

#ifndef _PLATFORM_H_
#define _PLATFORM_H_

/*******************************************************************************
* VISUAL STUDIO RELATED STUFF								                   *
*******************************************************************************/
#if defined _MSC_VER

//This prevents a depecrated warning in Visual Studio 2010
#	define strlwr(STR) _strlwr(STR)

#endif

/*******************************************************************************
* LINUX PLATFORM            								                   *
*******************************************************************************/
#if defined(__unix__) || defined(__unix) || defined(_unix) || defined(unix)

/// map all of these to __unix__
#    if !defined(__unix__)
#        define __unix__
#    endif

#    include <unistd.h>

/// make this function work cross-platform
#    define stricmp  strcasecmp

// Slashes are normal in Unix systems
#	define SLASH_STR "/"
#	define SLASH_CHR '/'

#endif

/*******************************************************************************
* WINDOWS PLATFORM          								                   *
*******************************************************************************/
#if defined(WIN32) || defined(_WIN32) || defined (__WIN32) || defined(__WIN32__)

// map all of these possibilities to WIN32
#    if !defined(WIN32)
#        define WIN32
#    endif

/// Speeds up compile times a bit.  We don't need everything in windows.h
#	define WIN32_LEAN_AND_MEAN

// Slashes are backwards in windows
#	define SLASH_STR "\\"
#	define SLASH_CHR '\\'

#endif




#endif /* _PLATFORM_H_	*/