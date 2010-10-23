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

/// @file character_defs.h

#include "egoboo_typedef.h"

#include "file_formats/cap_file.h"

#if defined(__cplusplus)
extern "C"
{
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// Attack directions
#define ATK_FRONT  0x0000
#define ATK_RIGHT  0x4000
#define ATK_BEHIND 0x8000
#define ATK_LEFT   0xC000

#define MAP_TURN_OFFSET 0x8000

#define MAXXP           200000                      ///< Maximum experience
#define MAXMONEY        9999                        ///< Maximum money
#define SHOP_IDENTIFY   200                         ///< Maximum value for identifying shop items

#define MAX_CAP    MAX_PROFILE

#define INFINITE_WEIGHT          (( Uint32 )0xFFFFFFFF)
#define MAX_WEIGHT               (( Uint32 )0xFFFFFFFE)

#define GRABSIZE            135.0f //90.0f             ///< Grab tolerance
#define NOHIDE              127                        ///< Don't hide
#define SEEINVISIBLE        128                        ///< Cutoff for invisible characters
#define RESPAWN_ANYTIME     0xFF                       ///< Code for respawnvalid...

#define RAISE               12                  ///< Helps correct z level

/// The possible methods for characters to determine what direction they are facing
    typedef enum e_turn_modes
    {
        TURNMODE_VELOCITY = 0,                       ///< Character gets rotation from velocity (normal)
        TURNMODE_ACCELERATION,                              ///< For watch towers, look towards waypoint
        TURNMODE_SPIN,                               ///< For spinning objects
        TURNMODE_WATCHTARGET,                        ///< For combat intensive AI
        TURNMODE_FLYING_JUMP,
        TURNMODE_FLYING_PLATFORM,
        TURNMODE_COUNT
    }
    TURN_MODE;

#define MANARETURNSHIFT     44                    ///< ChrObjList.get_data(ichr).manareturn/MANARETURNSHIFT = mana regen per second

#define TURN_SPEED             0.1f                  ///< Cutoff for turning or same direction
#define WATCH_SPEED            0.1f                  ///< Tolerance for TURNMODE_ACCELERATION
#define FLYING_SPEED           1.0f                  ///< Tolerance for TURNMODE_FLY_*
#define SPINRATE            200                      ///< How fast spinners spin

// The vertex offsets for the various grips
    enum e_grip_offset
    {
        GRIP_ORIGIN    =               0,                ///< Spawn attachments at the center
        GRIP_LAST      =               1,                ///< Spawn particles at the last vertex
        GRIP_LEFT      = ( 1 * GRIP_VERTS ),             ///< Left weapon grip starts  4 from last
        GRIP_RIGHT     = ( 2 * GRIP_VERTS ),             ///< Right weapon grip starts 8 from last

        // aliases
        GRIP_INVENTORY =               GRIP_ORIGIN,
        GRIP_ONLY      =               GRIP_LEFT
    };
    typedef enum e_grip_offset grip_offset_t;

    grip_offset_t slot_to_grip_offset( slot_t slot );
    slot_t        grip_offset_to_slot( grip_offset_t grip );

#define PITDEPTH            -60                     ///< Depth to kill character
#define NO_SKIN_OVERRIDE    -1                      ///< For import
#define HURTDAMAGE           256                    ///< Minimum damage for hurt animation

// Dismounting
#define DISMOUNTZVEL        16
#define DISMOUNTZVELFLY     4
#define PHYS_DISMOUNT_TIME  (TICKS_PER_SEC*0.05f)          ///< time delay for full object-object interaction (approximately 0.05 second)

// Knockbacks
#define REEL                7600.0f     ///< Dampen for melee knock back
#define REELBASE            0.35f

// Water
#define RIPPLETOLERANCE     60          ///< For deep water
#define SPLASHTOLERANCE     10
#define RIPPLEAND           15          ///< How often ripples spawn

// Stats
#define LOWSTAT             256                     ///< Worst...
#define PERFECTSTAT         (60*256)                ///< Maximum stat without magic effects
#define PERFECTBIG          (100*256)               ///< Perfect life or mana...
#define HIGHSTAT            (100*256)               ///< Absolute max adding enchantments as well

// Throwing
#define THROWFIX            30.0f                    ///< To correct thrown velocities
#define MINTHROWVELOCITY    15.0f
#define MAXTHROWVELOCITY    75.0f

// Inventory
#define MAXNUMINPACK        6                       ///< Max number of items to carry in pack
#define PACKDELAY           25                      ///< Time before inventory rotate again
#define GRABDELAY           25                      ///< Time before grab again

// Z velocity
#define FLYDAMPEN            0.001f                    ///< Leveling rate for fliers
#define JUMP_DELAY           20                      ///< Time between jumps
#define JUMP_SPEED_WATER     25                        ///< How good we jump in water
#define JUMP_NUMBER_INFINITE 255                     ///< Flying character
#define SLIDETOLERANCE        10                      ///< Stick to ground better
#define PLATADD              -10                     ///< Height add...
#define PLATASCEND           0.10f                     ///< Ascension rate
#define PLATKEEP             0.90f                     ///< Retention rate
#define MOUNTTOLERANCE       (PLATTOLERANCE)
#define STOPBOUNCING         0.1f // 1.0f                ///< To make objects stop bouncing
#define DROPZVEL             7
#define DROPXYVEL            12

// Timer resets
#define DAMAGETILETIME      32                            ///< Invincibility time
#define DAMAGETIME          32                            ///< Invincibility time
#define DEFENDTIME          24                            ///< Invincibility time
#define BORETIME            generate_randmask( 255, 511 ) ///< IfBored timer
#define CAREFULTIME         50                            ///< Friendly fire timer
#define SIZETIME            100                           ///< Time it takes to resize a character

/// Bits used to control options for the ego_chr::get_name() function
    enum e_chr_name_bits
    {
        CHRNAME_NONE     = 0,               ///< no options
        CHRNAME_ARTICLE  = ( 1 << 0 ),      ///< use an article (a, an, the)
        CHRNAME_DEFINITE = ( 1 << 1 ),      ///< if set, choose "the" else "a" or "an"
        CHRNAME_CAPITAL  = ( 1 << 2 )       ///< capitalize the name
    };

    enum e_chr_movement_idx
    {
        CHR_MOVEMENT_STOP  = 0,
        CHR_MOVEMENT_SNEAK,
        CHR_MOVEMENT_WALK,
        CHR_MOVEMENT_RUN,
        CHR_MOVEMENT_COUNT
    };

    enum e_chr_movement_bits
    {
        CHR_MOVEMENT_NONE  = 0,
        CHR_MOVEMENT_BITS_STOP  = 1 << CHR_MOVEMENT_STOP,
        CHR_MOVEMENT_BITS_SNEAK = 1 << CHR_MOVEMENT_SNEAK,
        CHR_MOVEMENT_BITS_WALK  = 1 << CHR_MOVEMENT_WALK,
        CHR_MOVEMENT_BITS_RUN   = 1 << CHR_MOVEMENT_RUN
    };

//------------------------------------
// Team variables
//------------------------------------
    enum e_team_types
    {
        TEAM_EVIL            = ( 'E' - 'A' ),        ///< Evil team
        TEAM_GOOD            = ( 'G' - 'A' ),        ///< Good team
        TEAM_NULL            = ( 'N' - 'A' ),        ///< Null or Neutral team
        TEAM_ZIPPY           = ( 'Z' - 'A' ),        ///< Zippy Team?
        TEAM_DAMAGE,                                 ///< For damage tiles
        TEAM_MAX
    };

#define NOLEADER            0xFFFF                   ///< If the team has no leader...

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
}
#endif

#define character_defs_h
