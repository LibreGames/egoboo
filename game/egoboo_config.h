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

/// \file egoboo_config.h
/// \brief System-dependent global parameters.
///   \todo  some of this stuff is compiler dependent, rather than system dependent.

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// compilation flags

// object pre-allocations
#define MAX_CHR            512             ///< Maximum number of characters
#define MAX_ENC            200             ///< Maximum number of enchantments
#define MAX_PRT           2048             ///< Maximum number of particles
#define MAX_DYNA            64             ///< Maximum number of dynamic lights

#define MAX_TEXTURE        (MAX_CHR * 4)     ///< Maximum number of textures
#define MAX_ICON           (MAX_TEXTURE + 4) ///< Maximum number of icons

#define MAX_SKIN             4               ///< The maxumum number of skins per model. This must remain hard coded at 4 for the moment.

/// profile pre-allocations
#define MAX_PROFILE        256          ///< Maximum number of object profiles
#define MAX_AI             MAX_PROFILE  ///< Maximum number of scripts

/// per-object pre-allocations
#define MAX_WAVE             30        ///< Maximum number of *.wav/*.ogg per object
#define MAX_PIP_PER_PROFILE  13        ///< Maximum number of part*.txt per object
#define MAX_PIP             (MAX_PROFILE * MAX_PIP_PER_PROFILE)

// the basic debugging switch
#if defined(_DEBUG) && !defined(NDEBUG)
#    define EGO_DEBUG 1
#else
#    define EGO_DEBUG 0
#endif

// this line can be added to get some debugging features enabled in release mode
// the compiler will complain
//#define EGO_DEBUG 0

//--- experimental game features
#undef  OLD_CAMERA_MODE       ///< Use the old camera style
#undef  ENABLE_BODY_GRAB      ///< Enable the grabbing of bodies?
#undef  USE_LUA_CONSOLE       ///< LUA support for the console

#define CLIP_LIGHT_FANS      ///< is the light_fans() function going to be throttled?
#define CLIP_ALL_LIGHT_FANS  ///< a switch for selecting how the fans will be updated

//---- graphics feedback
#undef  RENDER_HMAP           ///< render the mesh's heightmap?
#undef  DEBUG_MESH_NORMALS    ///< render the mesh normals
#undef  DEBUG_CHR_BBOX        ///< display the all character bounding boxes
#undef  DEBUG_PRT_BBOX        ///< display the all particle bounding boxes

//---- generic debugging
#define LOG_TO_CONSOLE        ///< dump all log info to file and to the console. Only useful if your compiler generates console for program output. Otherwise the results will end up in a file called stdout.txt
#define DEBUG_OBJECT_SPAWN    ///< Log debug info for every object spawned
#define TEST_NAN_RESULT       ///< Test the result of certain math operations
#undef  DEBUG_WAYPOINTS      ///< display error messages when adding waypoints

/// How much script debugging.
///    0 -- debugging off ( requires EGO_DEBUG )
/// >= 1 -- Log the amount of script time that every object uses (requires EGO_DEBUG and PROFILE_LEVEL)
/// >= 2 -- Log the amount of time that every single script command uses (requires EGO_DEBUG and PROFILE_LEVEL)
/// >= 3 -- decompile every script (requires EGO_DEBUG)
#define DEBUG_SCRIPT_LEVEL 0

//---- list debugging
#define USE_HASH              ///< use stdext::hash_map and stdext::hash_set instead of std::map and std::set
#undef  DEBUG_LIST            ///< Perform some extra steps to make sure the data in the XxxObjList do not become corrupt
#undef  DEBUG_CPP_LISTS

//---- performance feedback
#undef  BSP_INFO             ///< Print info about the BSP/octree state
#undef  RENDERLIST_INFO      ///< Print info for the currently rendered mesh

/// How much profiling
///    0 -- profiling off ( does not require EGO_DEBUG )
/// >= 1 -- calculate profiling
#define PROFILE_LEVEL                    0
#define PROFILE_DISPLAY (PROFILE_LEVEL > 0)    ///< Display the results for the performance profiling
#undef  PROFILE_INIT                           ///< Display the results for the performance profiling of the rendering initialization
#undef  PROFILE_RENDER                         ///< Display the results for the performance profiling of the generic rendering
#undef  PROFILE_MESH                           ///< Display the results for the performance profiling of the mesh rendering sub-system

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// do the includes last so that the compile switches are always set
#include "egoboo_platform.h"
#include "egoboo_endian.h"

#define _egoboo_config_h

