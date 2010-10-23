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

/// @file file_formats/pip_file.h
/// @details routines for reading and writing the particle profile file "part*.txt"

#include "egoboo_typedef.h"

#if defined(__cplusplus)
extern "C"
{
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// Pre-defined global particle types
    enum e_global_pips
    {
        PIP_COIN1 = 0,                                 ///< Coins are the first particles loaded
        PIP_COIN5,
        PIP_COIN25,
        PIP_COIN100,
        PIP_WEATHER4,                                  ///< Weather particles
        PIP_WEATHER5,                                  ///< Weather particle finish
        PIP_SPLASH,                                    ///< Water effects are next
        PIP_RIPPLE,
        PIP_DEFEND,                                     ///< Defend particle
        GLOBAL_PIP_COUNT
    };

/// particle types / sprite dosplay modes
    enum e_sprite_mode
    {
        SPRITE_LIGHT = 0,                         ///< Magic effect particle
        SPRITE_SOLID,                             ///< Sprite particle
        SPRITE_ALPHA                              ///< Smoke particle
    };

/// dynamic lighting modes
    enum e_dyna_mode
    {
        DYNA_MODE_OFF   = 0,
        DYNA_MODE_ON,
        DYNA_MODE_LOCAL
    };

// #define MAXFALLOFF 1400

/// Possible methods for computing the position and orientation of the quad used to display particle sprites
    enum e_prt_orientations
    {
        ORIENTATION_B = 0,   ///< billboard
        ORIENTATION_X,       ///< put particle up along the world or body-fixed x-axis
        ORIENTATION_Y,       ///< put particle up along the world or body-fixed y-axis
        ORIENTATION_Z,       ///< put particle up along the world or body-fixed z-axis
        ORIENTATION_V,       ///< vertical, like a candle
        ORIENTATION_H        ///< horizontal, like a plate
    };
    typedef enum e_prt_orientations prt_ori_t;

// The special damage effects for particles
    enum e_damage_fx
    {
        DAMFX_NONE           = 0,                       ///< Damage effects
        DAMFX_ARMO           = ( 1 << 1 ),              ///< Armor piercing
        DAMFX_NBLOC          = ( 1 << 2 ),              ///< Cannot be blocked by shield
        DAMFX_ARRO           = ( 1 << 3 ),              ///< Only hurts the one it's attached to
        DAMFX_TURN           = ( 1 << 4 ),              ///< Turn to attached direction
        DAMFX_TIME           = ( 1 << 5 )
    };

/// Turn values specifying corrections to the rotation of particles
    enum e_particle_direction
    {
        prt_v = 0x0000,    ///< particle is vertical on the bitmap
        prt_r = 0x2000,    ///< particle is diagonal (rotated 45 degrees to the right = 8192)
        prt_h = 0x4000,    ///< particle is horizontal (rotated 90 degrees = 16384)
        prt_l = 0xE000,    ///< particle is diagonal (rotated 45 degrees to the right = 8192)
        prt_u = 0xFFFF     ///< particle is of unknown orientation
    };
    typedef enum e_particle_direction particle_direction_t;

//--------------------------------------------------------------------------------------------
    struct s_dynalight_info
    {
        Uint8   mode;                ///< when is it?
        Uint8   on;                  ///< is it on now?

        float   level;               ///< intensity
        float   level_add;           ///< intensity changes

        float   falloff;             ///< range
        float   falloff_add;         ///< range changes
    };
    typedef struct s_dynalight_info dynalight_info_t;

//--------------------------------------------------------------------------------------------
// Particle template
//--------------------------------------------------------------------------------------------

/// The definition of a particle profile
    struct s_pip_data
    {
        EGO_PROFILE_STUFF;

        char    comment[1024];                ///< the first line of the file has a comment line

        // spawning
        bool_t  force;                        ///< Force spawn?
        Uint8   type;                         ///< Transparency mode
        Uint8   numframes;                    ///< Number of frames
        Uint8   image_base;                   ///< Starting image
        IPair   image_add;                    ///< Frame rate
        int     time;                         ///< Time until end
        IPair   rotate_pair;                  ///< Rotation
        Sint16  rotate_add;                   ///< Rotation rate
        Uint16  size_base;                    ///< Size
        Sint16  size_add;                     ///< Size rate
        Sint8   soundspawn;                   ///< Beginning sound
        Uint16  facingadd;                    ///< Facing
        IPair   facing_pair;                  ///< Facing
        IPair   spacing_hrz_pair;             ///< Spacing
        IPair   spacing_vrt_pair;             ///< Altitude
        IPair   vel_hrz_pair;                 ///< Shot velocity
        IPair   vel_vrt_pair;                 ///< Up velocity
        bool_t  newtargetonspawn;             ///< Get new target?
        bool_t  needtarget;                   ///< Need a target?
        bool_t  startontarget;                ///< Start on target?

        // ending conditions
        bool_t  end_water;                     ///< End if underwater
        bool_t  end_bump;                      ///< End if bumped
        bool_t  end_ground;                    ///< End if on ground
        bool_t  end_wall;                      ///< End if hit a wall
        bool_t  end_lastframe;                 ///< End on last frame
        Uint8   end_spawn_amount;              ///< Spawn amount
        Uint16  end_spawn_facingadd;           ///< Spawn in circle
        int     end_spawn_pip;                 ///< Spawn type ( local )
        Sint8   end_sound;                     ///< Ending sound
        Sint8   end_sound_floor;               ///< Floor sound
        Sint8   end_sound_wall;                ///< Ricochet sound

        // bumping
        Sint8   bump_money;                   ///< Value of particle
        Uint32  bump_size;                    ///< Bounding box size
        Uint32  bump_height;                  ///< Bounding box height

        // "bump particle" spawning
        Uint8   bumpspawn_amount;            ///< Spawn amount
        int     bumpspawn_pip;               ///< Spawn type ( global )

        // continuous spawning
        Uint16  contspawn_delay;              ///< Spawn timer
        Uint8   contspawn_amount;             ///< Spawn amount
        Uint16  contspawn_facingadd;          ///< Spawn in circle
        int     contspawn_pip;                ///< Spawn type ( local )

        // damage
        FRange  damage;                       ///< Damage
        Uint8   damagetype;                   ///< Damage type
        int     dazetime;                     ///< Daze
        int     grogtime;                     ///< Drunkeness
        Uint32  damfx;                        ///< Damage effects
        bool_t  intdamagebonus;               ///< Add intelligence as damage bonus
        bool_t  wisdamagebonus;               ///< Add wisdom as damage bonus
        bool_t  spawnenchant;                 ///< Spawn enchant?
        bool_t  onlydamagefriendly;           ///< Only friends?
        bool_t  friendlyfire;                 ///< Friendly fire
        bool_t  hateonly;                     ///< Only hit hategroup
        bool_t  causepancake;                 ///< @todo Not implemented!!
        Uint16  lifedrain;                      ///< Steal this much life
        Uint16  manadrain;                      ///< Steal this much mana

        // homing
        bool_t   homing;                       ///< Homing?
        FACING_T targetangle;                  ///< To find target
        float    homingaccel;                  ///< Acceleration rate
        float    homingfriction;               ///< Deceleration rate
        int      zaimspd;                      ///< [ZSPD] For Z aiming
        bool_t   rotatetoface;                 ///< Arrows/Missiles
        bool_t   targetcaster;                 ///< Target caster?

        // physics
        float   spdlimit;                     ///< Speed limit
        float   dampen;                       ///< Bounciness
        bool_t  allowpush;                    ///< Allow particle to push characters around

        dynalight_info_t dynalight;           ///< Dynamic lighting info

        prt_ori_t orientation;                ///< the way the particle orientation is calculated for display

        // debugging parameters
        int prt_request_count;                ///< a way to tell how popular this particle is
        int prt_create_count;                 ///< if this number is significantly less than the prt_request_count, there is a problem.
    };

    typedef struct s_pip_data pip_data_t;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
    extern particle_direction_t prt_direction[256];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

    pip_data_t * load_one_pip_file_vfs( const char *szLoadName, pip_data_t * ppip );

    pip_data_t * pip_init( pip_data_t * ppip );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#if defined(__cplusplus)
}
#endif

#define _pip_file_h

