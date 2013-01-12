/*******************************************************************************
*  PARTICLE.H                                                                  *
*    - EGOBOO-Editor                                                           *     
*                                                                              *
*    - Particle definitions and functions                                      *
*      (c)2013 Paul Mueller <muellerp61@bluewin.ch>                            *
*                                                                              *
*   This program is free software; you can redistribute it and/or modify       *
*   it under the terms of the GNU General Public License as published by       *
*   the Free Software Foundation; either version 2 of the License, or          *
*   (at your option) any later version.                                        *
*                                                                              *
*   This program is distributed in the hope that it will be useful,            *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of             *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
*   GNU Library General Public License for more details.                       *
*                                                                              *
*   You should have received a copy of the GNU General Public License          *
*   along with this program; if not, write to the Free Software                *
*   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. *
*******************************************************************************/

#ifndef _PARTICLE_H_
#define _PARTICLE_H_

/*******************************************************************************
* DEFINES								                                   *
*******************************************************************************/

// File booleans as flags
#define PRT_FORCE         0x0001    ///< Force spawn?
#define PRT_END_WATER     0x0002    ///< End if underwater
#define PRT_END_BUMP      0x0004    ///< End if bumped
#define PRT_END_GROUND    0x0008    ///< End if on ground
#define PRT_END_WALL      0x0010    ///< End if hit a wall
#define PRT_END_LASTFRAME 0x0020    ///< End on last frame  / BoolVal[4]
#define PRT_DYNALIGHT_ON  0x0040    // dynalight_on / BoolVal[5]
#define PRT_SPAWNENCHANT  0x0080    ///< Spawn enchant?  
#define PRT_CAUSEROLL     0x0100    ///< Cause roll?
#define PRT_CAUSEPANCAKE  0x0200    ///< \todo Not implemented!! Cause pancake
#define PRT_NEEDTARGET    0x0400    // needtarget
#define PRT_TARGETCASTER  0x0800    ///< Target caster?
#define PRT_STARTONTARGET 0x1000    ///< Start on target
#define PRT_ONLYDAMAGEFRIENDLY 0x2000   ///< Only friends?
#define PRT_FRIENDLYFIRE    0x4000      ///< Friendly fire
#define PRT_HATEONLY        0x8000      ///< Only hit hategroup
#define PRT_NEWTARGETONSPAWN 0x10000    ///< New target on spawn?
#define PRT_HOMING          0x20000     ///< Homing in on target?
#define PRT_ROTATETOFACE    0x40000     ///< Rotate to face direction? ///< Arrows/Missiles
#define PRT_RESPAWNONHIT    0x80000     ///< Respawn character on hit?

/*******************************************************************************
* ENUMS								                                        *
*******************************************************************************/

/// Pre-defined global particle types
enum
{
    PIP_COIN1 = 1,                                 ///< Coins are the first particles loaded
    PIP_COIN5,
    PIP_COIN25,
    PIP_COIN100,
    PIP_WEATHER4,                                  ///< Weather particles
    PIP_WEATHER5,                                  ///< Weather particle finish
    PIP_SPLASH,                                    ///< Water effects are next
    PIP_RIPPLE,
    PIP_DEFEND,                                     ///< Defend particle
    GLOBAL_PIP_COUNT
    
} E_GLOBAL_PIPS;

/// particle types / sprite display modes
enum
{
    SPRITE_LIGHT = 0,                         ///< Magic effect particle
    SPRITE_SOLID,                             ///< Sprite particle
    SPRITE_ALPHA                              ///< Smoke particle
    
} E_SPRITE_MODE;

/// dynamic lighting modes
enum
{
    DYNA_MODE_OFF   = 0,
    DYNA_MODE_ON,
    DYNA_MODE_LOCAL
    
} E_DYNA_MODE;

/// Possible methods for computing the position and orientation of the quad used to display particle sprites
typedef enum 
{
    ORIENTATION_B = 0,   ///< billboard
    ORIENTATION_X,       ///< put particle up along the world or body-fixed x-axis
    ORIENTATION_Y,       ///< put particle up along the world or body-fixed y-axis
    ORIENTATION_Z,       ///< put particle up along the world or body-fixed z-axis
    ORIENTATION_V,       ///< vertical, like a candle
    ORIENTATION_H        ///< horizontal, like a plate
    
} PRT_ORI_T;

// The special damage effects for particles
enum
{
    DAMFX_NONE           = 0,                       ///< Damage effects
    DAMFX_ARMO           = ( 1 << 1 ),              ///< Armor piercing
    DAMFX_NBLOC          = ( 1 << 2 ),              ///< Cannot be blocked by shield
    DAMFX_ARRO           = ( 1 << 3 ),              ///< Only hurts the one it's attached to
    DAMFX_TURN           = ( 1 << 4 ),              ///< Turn to attached direction
    DAMFX_TIME           = ( 1 << 5 )
    
} E_DAMAGE_FX;

/// Turn values specifying corrections to the rotation of particles
typedef enum
{
    prt_v = 0x0000,    ///< particle is vertical on the bitmap
    prt_r = 0x2000,    ///< particle is diagonal (rotated 45 degrees to the right = 8192)
    prt_h = 0x4000,    ///< particle is horizontal (rotated 90 degrees = 16384)
    prt_l = 0xE000,    ///< particle is diagonal (rotated 45 degrees to the right = 8192)
    prt_u = 0xFFFF     ///< particle is of unknown orientation
    
} PARTICLE_DIRECTION_T;

/*******************************************************************************
* TYPEDEFS							                                           *
*******************************************************************************/

typedef struct 
{
    char  mode;         ///< when is it?
    char  on;           ///< is it on now?

    float level;        ///< intensity
    float level_add;    ///< intensity changes

    float falloff;      ///< range
    float falloff_add;  ///< range changes
    
} DYNALIGHT_INFO_T;

/*******************************************************************************
* CODE 								                                           *
*******************************************************************************/


/* ============ File functions ============== */
int particleLoadGlobals(void);
int particleLoad(char *filename);

/* ======= Game-Functions ===== */
int  particleSpawn(int pip_no, int spawnobj_no, char attached_to, char modifier);

/* ============ Particle functions ========== */
char particleUpdateOne(int obj_no, int hit_env /* @todo: , MAPENV_T *penviro */);

#endif  /* #define _PARTICLE_H_ */