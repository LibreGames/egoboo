/*******************************************************************************
*  PARTICLE.C                                                                  *
*    - EGOBOO-Editor                                                           *     
*                                                                              *
*    - [...]                                                                   *
*      (c) 2013 Paul Mueller <muellerp61@bluewin.ch>                           *
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


/*******************************************************************************
* INCLUDES					                                                *
*******************************************************************************/

#include <math.h>
#include <memory.h> 
#include <stdio.h>


// Egoboo headers
#include "sdlgl3d.h"     /* Create, handle a particle as object         */
#include "sdlglcfg.h"    /* Read egoboo text files eg. passage, spawn   */
// #include "editfile.h"    /* Get the work-directoires                    */
#include "egodefs.h"     // MAPENV_T: Description of the tiles enviro   */
#include "idsz.h"
#include "misc.h"        // Random values


// Own header
#include "particle.h"

/*******************************************************************************
* DEFINES								                                   *
*******************************************************************************/

#define PRT_STOP_BOUNCING   0.04         // To make particles stop bouncing
#define MAX_PIP_TYPE       1024

/*******************************************************************************
* TYPEDEFS								                                   *
*******************************************************************************/


/// The definition of a particle profile
typedef struct
{
    // Internal info
    int id;                 // Number of profile
    unsigned int flags;     // All booleans packed into one integer
    // spawning
    char  type;                     ///< Transparency mode
    int   image_base;               ///< Starting image
    int   numframes;                ///< Number of frames
    int   image_add[2];             ///< Frame rate : base, randomness 
                                    /// ( 0 to 1000 ) / ( 0, 1, 3, 7, 15, 31, 63... )
    int   rotate_pair[2];           ///< Rotation: base, add
                                    /// ( 0 to 65535 ) / ( 0, 1, 3, 7, 15, 31, 63... ) 
    short int rotate_add;           ///< Rotation rate ( 0 to 1000 )
                                    ///@todo: Normalize at loading time to angle
    int       size_base;            ///< Size   ( 0 to 65535 )
                                    ///@todo: Normalize at loading time
    short int size_add;             ///< Size increase rate ( 0 to 1000 )
                                    ///@todo: Normalize at loading time
    int       time;                 ///< Time until end ( 1 to 1000, 0 )

    int     soundspawn;             ///< Beginning sound
    unsigned short  facingadd;      ///< Facing
    int   facing_pair[2];           ///< Facing : base, rand
    int   spacing_hrz_pair[2];      ///< Spacing : base, rand
    int   spacing_vrt_pair[2];      ///< Altitude : base, rand
    int   vel_hrz_pair[2];          ///< Shot velocity : base, rand
    int   vel_vrt_pair[2];          ///< Up velocity : base, rand
    char  newtargetonspawn;         ///< Get new target?
    char  needtarget;               ///< Need a target?
    char  startontarget;            ///< Start on target?

    // ending conditions
    unsigned char  end_spawn_amount;    ///< Spawn amount
    unsigned short end_spawn_facingadd; ///< Spawn in circle
    int   end_spawn_pip;            ///< Spawn type ( local )
    int   end_sound;                ///< Ending sound
    int   end_sound_floor;          ///< Floor sound
    int   end_sound_wall;           ///< Ricochet sound

    // bumping
    int  bump_money;                ///< Value of particle
    int  bump_size;                 ///< Bounding box size
    int  bump_height;               ///< Bounding box height

    // "bump particle" spawning
    unsigned char bumpspawn_amount; ///< Spawn amount
    int     bumpspawn_pip;          ///< Spawn type ( global )

    // continuous spawning
    unsigned short contspawn_time;          ///< Spawn timer (Number of frames)
    unsigned char  contspawn_amount;        ///< Spawn amount
    unsigned short contspawn_facingadd;     ///< Spawn in circle
    int     contspawn_pip;                  ///< Spawn type ( local )

    // damage
    float damage[2];                    ///< Damage range
    char  damagetype;                   ///< Damage type
    int   dazetime;                     ///< Daze
    int   grogtime;                     ///< Drunkeness
    int   damfx;                        ///< Damage effects
    char  intdamagebonus;               ///< Add intelligence as damage bonus
    char  wisdamagebonus;               ///< Add wisdom as damage bonus
    char  lifedrain;                    ///< Steal this much life
    char  manadrain;                    ///< Steal this much mana

    // homing
    int    targetangle;                  ///< To find target  -- FACING_T
    float  homingaccel;                  ///< Acceleration rate
    float  homingfriction;               ///< Deceleration rate
    int    zaimspd;                      ///< [ZSPD] For Z aiming

    // physics
    float   spdlimit;                     ///< Speed limit
    float   dampen;                       ///< Bounciness
    char    allowpush;                    ///< Allow particle to push characters around    

    DYNALIGHT_INFO_T dynalight;           ///< Dynamic lighting info
    
    IDSZ_T exp_idsz[5];         /// Particle expansion IDSZs */
    // Data for internal use and statistics
    int request_count;
    int create_count;
    
} PIP_T;


/*******************************************************************************
* DATA									                                       *
*******************************************************************************/

// Load-Buffer for 'raw' data which is converted into 'packed' data
static char BoolVal[33];        /* Boolean values */
static char ExpIdsz[16][18 + 1]; /* Particle expansion IDSZs ( with a colon in front ) */

// Particle description itself
static PIP_T PrtDesc;

static SDLGLCFG_NAMEDVALUE PrtVal[] =
{
    { SDLGLCFG_VAL_BOOLEAN, &BoolVal[0] },      // force spawn
    { SDLGLCFG_VAL_ONECHAR, &PrtDesc.type },
    { SDLGLCFG_VAL_INT,    &PrtDesc.image_base },
    { SDLGLCFG_VAL_INT,    &PrtDesc.numframes },
    { SDLGLCFG_VAL_INT,    &PrtDesc.image_add[0] },
    { SDLGLCFG_VAL_INT,    &PrtDesc.image_add[1] },
    { SDLGLCFG_VAL_INT,    &PrtDesc.rotate_pair[0] },
    { SDLGLCFG_VAL_INT,    &PrtDesc.rotate_pair[1] },
    { SDLGLCFG_VAL_SHORT,  &PrtDesc.rotate_add },
    { SDLGLCFG_VAL_INT,    &PrtDesc.size_base },
    { SDLGLCFG_VAL_SHORT,  &PrtDesc.size_add },
    { SDLGLCFG_VAL_FLOAT,  &PrtDesc.spdlimit },
    { SDLGLCFG_VAL_USHORT, &PrtDesc.facingadd },
    { SDLGLCFG_VAL_BOOLEAN, &BoolVal[1] },
    { SDLGLCFG_VAL_BOOLEAN, &BoolVal[2] },
    { SDLGLCFG_VAL_BOOLEAN, &BoolVal[3] },
    { SDLGLCFG_VAL_BOOLEAN, &BoolVal[4] },
    { SDLGLCFG_VAL_INT, &PrtDesc.time },
    { SDLGLCFG_VAL_FLOAT, &PrtDesc.dampen },
    { SDLGLCFG_VAL_INT, &PrtDesc.bump_money },
    { SDLGLCFG_VAL_INT, &PrtDesc.bump_size },
    { SDLGLCFG_VAL_INT, &PrtDesc.bump_height },
    { SDLGLCFG_VAL_FPAIR, PrtDesc.damage },
    { SDLGLCFG_VAL_ONECHAR, &PrtDesc.damagetype },
    { SDLGLCFG_VAL_BOOLEAN, &PrtDesc.dynalight.on },
    { SDLGLCFG_VAL_FLOAT, &PrtDesc.dynalight.level },
    { SDLGLCFG_VAL_FLOAT, &PrtDesc.dynalight.falloff },
    { SDLGLCFG_VAL_INT, &PrtDesc.facing_pair[0] },
    { SDLGLCFG_VAL_INT, &PrtDesc.facing_pair[1] },
    { SDLGLCFG_VAL_INT, &PrtDesc.spacing_hrz_pair[0] },
    { SDLGLCFG_VAL_INT, &PrtDesc.spacing_hrz_pair[1] },
    { SDLGLCFG_VAL_INT, &PrtDesc.spacing_vrt_pair[0] },
    { SDLGLCFG_VAL_INT, &PrtDesc.spacing_vrt_pair[1] },
    { SDLGLCFG_VAL_INT, &PrtDesc.vel_hrz_pair[0] },
    { SDLGLCFG_VAL_INT, &PrtDesc.vel_hrz_pair[1] },
    { SDLGLCFG_VAL_INT, &PrtDesc.vel_vrt_pair[0] },
    { SDLGLCFG_VAL_INT, &PrtDesc.vel_vrt_pair[1] },
    { SDLGLCFG_VAL_USHORT, &PrtDesc.contspawn_time },
    { SDLGLCFG_VAL_UCHAR, &PrtDesc.contspawn_amount },
    { SDLGLCFG_VAL_USHORT, &PrtDesc.contspawn_facingadd },
    { SDLGLCFG_VAL_INT, &PrtDesc.contspawn_pip },
    { SDLGLCFG_VAL_UCHAR, &PrtDesc.end_spawn_amount },
    { SDLGLCFG_VAL_USHORT, &PrtDesc.end_spawn_facingadd },
    { SDLGLCFG_VAL_INT, &PrtDesc.end_spawn_pip },
    { SDLGLCFG_VAL_UCHAR, &PrtDesc.bumpspawn_amount },
    { SDLGLCFG_VAL_INT, &PrtDesc.bumpspawn_pip },
    { SDLGLCFG_VAL_INT, &PrtDesc.dazetime },
    { SDLGLCFG_VAL_INT, &PrtDesc.grogtime },
    { SDLGLCFG_VAL_BOOLEAN, &BoolVal[6] },
    { SDLGLCFG_VAL_BOOLEAN, &BoolVal[7] },
    { SDLGLCFG_VAL_BOOLEAN, &BoolVal[8] },
    { SDLGLCFG_VAL_BOOLEAN, &BoolVal[9] },
    { SDLGLCFG_VAL_BOOLEAN, &BoolVal[10] }, // targetcaster
    { SDLGLCFG_VAL_BOOLEAN, &BoolVal[11] }, // startontarget
    { SDLGLCFG_VAL_BOOLEAN, &BoolVal[12] },
    { SDLGLCFG_VAL_INT, &PrtDesc.soundspawn },
    { SDLGLCFG_VAL_INT, &PrtDesc.end_sound },
    { SDLGLCFG_VAL_BOOLEAN, &BoolVal[13] },
    { SDLGLCFG_VAL_BOOLEAN, &BoolVal[14] }, // hateonly
    { SDLGLCFG_VAL_BOOLEAN, &BoolVal[15] }, // newtargetonspawn
    { SDLGLCFG_VAL_INT, &PrtDesc.targetangle },
    { SDLGLCFG_VAL_BOOLEAN, &BoolVal[16] }, // homing
    { SDLGLCFG_VAL_FLOAT, &PrtDesc.homingfriction },
    { SDLGLCFG_VAL_FLOAT, &PrtDesc.homingaccel },
    { SDLGLCFG_VAL_BOOLEAN, &BoolVal[17] }, // rotatetoface
    { SDLGLCFG_VAL_BOOLEAN, &BoolVal[18] },     // respawnonhit
    { SDLGLCFG_VAL_CHAR,   &PrtDesc.manadrain },
    { SDLGLCFG_VAL_CHAR,   &PrtDesc.lifedrain },
    // IDSZ values (maximum 12 different IDSZs)
    { SDLGLCFG_VAL_STRING, ExpIdsz[0], 18 },
    { SDLGLCFG_VAL_STRING, ExpIdsz[1], 18 },
    { SDLGLCFG_VAL_STRING, ExpIdsz[2], 18 },
    { SDLGLCFG_VAL_STRING, ExpIdsz[3], 18 },
    { SDLGLCFG_VAL_STRING, ExpIdsz[4], 18 },
    { SDLGLCFG_VAL_STRING, ExpIdsz[5], 18 },
    { SDLGLCFG_VAL_STRING, ExpIdsz[6], 18 },
    { SDLGLCFG_VAL_STRING, ExpIdsz[7], 18 },
    { SDLGLCFG_VAL_STRING, ExpIdsz[8], 18 },
    { SDLGLCFG_VAL_STRING, ExpIdsz[9], 18 },
    { SDLGLCFG_VAL_STRING, ExpIdsz[10], 18 },
    { SDLGLCFG_VAL_STRING, ExpIdsz[11], 18 },
    { SDLGLCFG_VAL_STRING, ExpIdsz[12], 18 },
    { SDLGLCFG_VAL_STRING, ExpIdsz[13], 18 },
    { 0 }
};

// List of the global particle file names
static char *prt_file_names[] =
{
    "",
    "1money.txt",   // PIP_COIN1 = 1, ///< Coins are the first particles loaded
    "5money.txt",   // PIP_COIN5,
    "25money.txt",  // PIP_COIN25,
    "100money.txt", // PIP_COIN100,
    "-",            // PIP_WEATHER4,                ///< Weather particles
    "-",            // PIP_WEATHER5,                ///< Weather particle finish
    "splash.txt",   // PIP_SPLASH,                  ///< Water effects are next
    "ripple.txt",   // PIP_RIPPLE,
    "defend.txt",   // PIP_DEFEND,                  ///< Defend particle                                       
    ""              // GLOBAL_PIP_COUNT
};

static PIP_T PrtDescList[MAX_PIP_TYPE + 2];

/*******************************************************************************
* CODE									                                   *
*******************************************************************************/

/*
 * Name:
 *     particlePlaySound
 * Description:
 *     This function plays a sound effect for a particle
 * Input:
 *     sound:       Number of sound to play
 */
static void particlePlaySound(int sound)
{
    if(sound > 0)
    {
        // @todo: soundPlay(sound);
        // sound_play_chunk(prt_get_pos_v_const( pprt ), pro_get_chunk( pprt->profile_ref, sound ))
        // Sound-Functions check itself forthe validity of a sound
        // @note: Check for 'g_wavelist[sound]'
    }
}

/*
 * Name:
 *     particleSpawnOne
 * Description:
 *     Spawns a particle as an object. Returns the number of the object created on success. 
 * Input:
 *     pip_no:    Use this particle type for creation of object
 *     pemit *:   Spawned by this object (Holds spawn position)
 *     target_no: Particle has to follow the 'emitter_no' or targetting this one
 *     modifier:  Precision of aiming 70..100%, strength multiplier for damage
 *     zpos:      Modifier for zpos, if any 
 * Output:
 *     > 0: Number of the object created
 */
 static int particleSpawnOne(int pip_no, SDLGL3D_OBJECT *pemit, char target_no, char modifier, float zpos)
 {  
    int obj_no;
    float size, velocity, offsetfacing, posz;
    float facing, spacing_xy, spacing_z, vel_z;
    SDLGL3D_OBJECT new_obj;
    PIP_T *ppip;


    // @port: Replaces: 'prt_config_do_init'
    // @note: The particle is a pure object. The only difference is additional handling after movement
    if(NULL == pemit || 0 == pip_no)
    {
        // no spawner, no type, no particle
        return 0;
    }       

    // Get the particle profile
    ppip = &PrtDescList[pip_no];
        

    // Copy all its data into the new object
    memcpy(&new_obj, pemit, sizeof(SDLGL3D_OBJECT));

    // New object is a particle of given type
    new_obj.obj_type = EGOMAP_OBJ_PART;
    new_obj.type_no  = pip_no;

    /* Setup the objects values */
    // Set character attachments ( pdata->chr_attach == 0 means none )
    if(target_no)
    {
        // Set 'attached_to'
        new_obj.target_obj = pemit->id;
    }

    // Initial spawning of this particle
    // Correct loc_facing
    ///< Facing rate     ( 0 to 1000 ) at movement
    facing      = ppip->facing_pair[0] * 360.0 / 65535.0;      // Facing base: ( 0 to 65535 )
    facing     += (float)miscRandVal(ppip->facing_pair[1]) * 360.0 / 65535.0;
    spacing_xy  = ppip->spacing_hrz_pair[0]; // Spacing : ( 0 to 100 )
    spacing_xy += (float)miscRandVal(ppip->spacing_hrz_pair[1]) * 100.0 / 65535.0;
    spacing_z   = ppip->spacing_vrt_pair[0]; // Altitude : ( -100 to 100 )
    spacing_z  += (float)miscRandVal(ppip->spacing_vrt_pair[1]) * 100.0 / 65535.0;
    velocity   = ppip->vel_hrz_pair[0];     ///< Shot velocity : ( 0 to 100 )
    velocity   += (float)miscRandVal(ppip->vel_hrz_pair[1]) * 100.0 / 65535.0;
    vel_z       = ppip->vel_vrt_pair[0];     ///< Up velocity : ( -100 to 100 )
    vel_z      += (float)miscRandVal(ppip->vel_vrt_pair[1]) * 100.0 / 65535.0;

    // Targeting... 
    posz = pemit->pos[2];
    
    if(ppip->flags & PRT_NEWTARGETONSPAWN)
    {
        if(ppip->flags & PRT_TARGETCASTER)
        {
            // Set the target to the caster
            new_obj.target_obj = pemit->id;
        }
        else
        {
            // Find a target
            // @todo: Correct 'facing' for dexterity...
            /*
                new_obj.target_obj = find_target(pos -> x,  pos -> y, facing,
                                             ppip->targetangle, ppip->onlydamagefriendly, 0, team, characterorigin, oldtarget);
                if(pprt->target[cnt] != 0 && ppip->homing == 0)
                {
                    facing = facing-glouseangle;
                }
             */
            offsetfacing = 0;
            /*
             pchar = charGet(pemit->owner_no);
            if(pchar->dex[CHARSTAT_ACT] < CHAR_PERFECTSTAT)
                {
                    // Correct facing for randomness
                    offsetfacing = facing;  // Divided by PERFECTSTAT
                    offsetfacing /= (CHAR_PERFECTSTAT - pchar->dex[CHARSTAT_ACT]);
                }
                if(ppip->zaimspd != 0 && new_obj.target_obj > 0)
                {
                    // These aren't velocities...  This is to do aiming on the Z axis
                    if(velocity > 0)
                    {
                        posz = pemit->pos[2];
                        
                        ptarget = sdlgl3dGetObject(new_obj.target_obj);
                        vel[0] = ptarget.pos[0] - pemit->pos[0];
                        vel[1] = ptarget.pos[1] - pemit->pos[1];
                        Vel.x = CharPos[pprt->target[cnt]].pos.x - pos -> x;
                        Vel.y = CharPos[pprt->target[cnt]].pos.y - pos -> y;
                        tvel = sqrt(vel[0]*vel[0]+vel[1]*vel[1])/velocity;  // This is the number of steps...
                        if(tvel > 0)
                        {
                            vel[2] = (ptarget->pos[2]+(ptarget->bradius)-vel[2])/tvel;  // This is the zvel alteration
                            if(vel[2] < -(ppip->zaimspd / 2)) 
                            {
                                posz = -(ppip->zaimspd / 2);
                            }
                            if(vel[2] > ppip->zaimspd)
                            {
                                posz = ppip->zaimspd;
                            }
                        }
                    }
                }
            */
        }
        // Does it go away?
        if(new_obj.target_obj == 0 && (ppip->flags & PRT_NEEDTARGET))
        {
            return 0;
        }
        // Start on top of target
        if(new_obj.target_obj != 0 && (ppip->flags & PRT_STARTONTARGET))
        {
            new_obj.pos[0] = pemit->pos[0];
            new_obj.pos[1] = pemit->pos[1];
        }
    }
    else
    {
        offsetfacing = pemit->rot[2];
        offsetfacing += (float)miscRandVal(ppip->facing_pair[1]) * 180.0 / 65535.0;
    }

    // @todo: Correct 'facing' for dexterity...
    facing += offsetfacing;
    new_obj.rot[2] += facing;

    // Adjust position in the original objects direction
    new_obj.pos[0] += new_obj.dir[0] * spacing_xy;
    new_obj.pos[1] += new_obj.dir[1] * spacing_xy;

    // Adjust position on Z-Axis
    new_obj.pos[2] += spacing_z;

    if(zpos != 0.0)
    {
        new_obj.pos[2] = zpos;
    }
    else if(posz != 0)
    {
        new_obj.pos[2] = posz;
    }

    // General Velocity
    new_obj.speed  = velocity;
    new_obj.zspeed = vel_z;

    // Set the turn-velocity
    ///< ( -100 to 100 ), ( 0, 1, 3, 7, 15, 31, 63... )
    new_obj.turnvel = (float)ppip->facingadd * 360.0 / 65535.0;

    // Set the bounding box
    // Only set ppip->bump_height if > 0
    size = (float)ppip->size_base * 20.0 / 65535.0; // half the size
    new_obj.bradius = size;
    new_obj.bbox[0][0] = -size;
    new_obj.bbox[0][1] = -size;
    new_obj.bbox[0][2] = -size; // (ppip->bump_height / 2);
    new_obj.bbox[1][0] = +size;
    new_obj.bbox[1][1] = +size;
    new_obj.bbox[1][2] = +size; // (ppip->bump_height / 2);

    // Info about animation, First / Current and Last frame
    new_obj.base_frame = ppip->image_base;
    new_obj.cur_frame  = ppip->image_base;
    new_obj.end_frame  = ppip->image_base + ppip->numframes - 1;
    new_obj.anim_speed = (float)ppip->image_add[0] / 1000.0;    // Framerat in ticks/sec
    new_obj.anim_clock = new_obj.anim_speed;                    // Set clock for countdown
    new_obj.spawn_time = (float)ppip->contspawn_time / 50.0;    // 50 Frames / Second
    new_obj.life_time  = (float)ppip->time / 50.0;              // Life in frames (max. 20 Sec.)

    if((ppip->flags & PRT_END_LASTFRAME) && ppip->numframes > 1)
    {
        if(ppip->time == 0)
        {
            // Part time is set to 1 cycle
            new_obj.life_time = 0.02f * ppip->numframes;
        }
        else
        {
            // Part time is used to give number of cycles
            new_obj.life_time = 0.02f * (float)ppip->time * ppip->numframes;
        }
    }

    // Other general info-flags
    new_obj.tags = ppip->flags;     // PRT_...
    new_obj.size = 1.0;

    // count all the requests for this particle type
    ppip->request_count++;

    // Create new object with 'auto-movement'
    obj_no = sdlgl3dCreateObject(&new_obj, SDLGL3D_MOVE_3D);

    if(obj_no > 0)
    {
        // count all the successful spawns of this particle
        ppip->create_count++;
    }

    // return the number of the object, if any is created
    return obj_no;
}

/*
 * Name:
 *     particleDoEndSpawn
 * Description:
 *     Does all of the end of life care for the particle
 * Input:
 *     pprt *: This particle-object
 *     ppip *: This is its profile
 * Output:
 *     > 0: Number of the object created
 */
static int particleDoEndSpawn(SDLGL3D_OBJECT *pprt, PIP_T *ppip)
{
    int tnc, obj_no;
    int endspawn_count;
    float facing;


    endspawn_count = 0;
    
    if (!pprt->type_no) return endspawn_count;    

    // Spawn new particles if time for old one is up
    if (ppip->end_spawn_amount > 0 && ppip->end_spawn_pip > -1 )
    {
        facing = 0.0;
        
        // All object data is copied from 'parent' to 'child'
        for (tnc = 0; tnc < ppip->end_spawn_amount; tnc++)
        {
            facing += ppip->end_spawn_facingadd;
            
            // we have determined the absolute pip reference when the particle was spawned
            // so, set the profile reference to INVALID_PRO_REF, so that the
            // value of ppip->endspawn_lpip will be used directly
            obj_no = particleSpawnOne(ppip->end_spawn_pip, pprt, 0, 100, 0);

            if (obj_no > 0)
            {
                endspawn_count++;
            }
            
            // @todo: Add facing angle to object
        }

        // we have already spawned these particles, so set this amount to
        // zero in case we are not actually calling 'particleEndInGame()'
        // this time around.        
    }

    return endspawn_count;
}

/*
 * Name:
 *     particleEndInGame
 * Description:
 *     This function causes the game to end a particle and mark it as a ghost.
 *     A 'ghost' has to be killed by the game itself
 * Input:
 *     obj_no:  Do update on this object
 *     hit_env: Has hit environement ENV_WALL or ENV_BOTTOM or 0 
 *     // @todo: enviro *: Pointer on description of environment for physics 
 * Output:
 *     0: Particle is killed by movement,
 *        // Caller should detach its object from map and destroy it  
 */
static char particleEndInGame(SDLGL3D_OBJECT *pprt, PIP_T *ppip)
{
    // the object is waiting to be killed, so
    // do all of the end of life care for the particle
    particleDoEndSpawn(pprt, ppip);

    /*
    if ( SPAWNNOCHARACTER != pprt->endspawn_characterstate )
    {
        child = spawn_one_character( prt_get_pos_v_const( pprt ), pprt->profile_ref, pprt->team, 0, pprt->facing, NULL, INVALID_CHR_REF );
        if ( DEFINED_CHR( child ) )
        {
            chr_t * pchild = ChrList_get_ptr( child );

            chr_set_ai_state( pchild , pprt->endspawn_characterstate );
            pchild->ai.owner = pprt->owner_ref;
        }
    }
    */

    if(NULL != ppip)
    {
        particlePlaySound(ppip->end_sound);
    }
        
    // Object is killed by game
    return 0;
}

/*
 * Name:
 *     particleGetFreePip
 * Description:
 *     Returns the number of the first free buffer for a particle profile
 * Input:
 *     None
 * Output:
 *     >0: Number of profile
 */
static int particleGetFreePip(void)
{
    int pip_no;
    
    
    for(pip_no = GLOBAL_PIP_COUNT; pip_no < MAX_PIP_TYPE; pip_no++)
    {
        if(PrtDescList[pip_no].id == 0)
        {
            return pip_no;
        }
    }
    
    return 0;
}


/*
 * Name:
 *     particleLoadOne
 * Description:
 *     Loads one particle profile
 * Input:
 *     filename *:
 *     pip_no:       Number of profile to fill with given data
 *     first_pip_no: Number of first particle of consecutive loaded PIPS
 * Output:
 *     Profile could be read yes/no
 */
static char particleLoadOne(char *filename, int pip_no, int first_pip_no)
{
    int i;
    PIP_T *ppip;


    // Do the 'raw' loading
    if(! sdlglcfgEgobooValues(filename, PrtVal, 0))
    {
        return 0;
    }

    ppip = &PrtDescList[pip_no];

    // Fill in the 'direct' loaded data
    memcpy(ppip, &PrtDesc, sizeof(PIP_T));

    // Sign it as 'used', number of array index
    ppip -> id = pip_no;
    
    // 'Translate' raw to profile data
    // Change booleans to flags
    for (i = 0; i < 32; i++)
    {
        if(BoolVal[i] != 0)
        {
            ppip -> flags |= (int)(1 << i);
        }
    }
    
    // Set number of local PIP for continuous spawning
    ppip -> contspawn_pip += first_pip_no;
    
    // Change idsz-strings to idsz-values
    for(i = 0; i < 14; i++)
    {
        // @todo: Convert IDSZ's to Particle values
    }

    return 1;
}

/* ========================================================================== */
/* ============================= PUBLIC FUNCTION(S) ========================= */
/* ========================================================================== */

/*
 * Name:
 *     particleInit
 * Description:
 *     Reset the buffer for particle types, except this one for the global particles
 * Input:
 *     None 
 */
void particleInit(void)
{
    // Reset index-counter: Start after global Particles
    memset(&PrtDescList[GLOBAL_PIP_COUNT], 0, MAX_PIP_TYPE - GLOBAL_PIP_COUNT);
}

/*
 * Name:
 *     particleLoad
 * Description:
 *     Loads multiple particles in consecutive order
 * Input:
 *     fdir *:    Directory to lad particle descriptors from
 *     pip_cnt *: Where to return the number of pips loaded 
 * Output:
 *     > 0: Number of first in 'PrtDescList', if any
 */
int particleLoad(char *fdir, int *pip_cnt)
{   
    char fname[512];
    int first_pip_no, cnt, num_pip;


    // Init counter of loaded particles
    *pip_cnt = 0;

    first_pip_no = particleGetFreePip();

    if(first_pip_no > 0)
    {
        // Load the first (basic) particle
        sprintf(fname, "%spart0.txt", fdir);

        if(particleLoadOne(fname, first_pip_no, first_pip_no))
        {
            // Now load the other particles
            num_pip = 1;

            for(cnt = 1; cnt < 20; cnt++)
            {
                // A maximum of 20 particles
                if(first_pip_no + cnt < MAX_PIP_TYPE)
                {
                    // Still buffer left for particle profiles
                    sprintf(fname, "%spart%d.txt", fdir, cnt);

                    if(particleLoadOne(fname, first_pip_no + cnt, first_pip_no))
                    {
                        num_pip++;
                    }
                    else
                    {
                        break;
                    }
                }
            }

            *pip_cnt = num_pip;
        }
        else
        {
            first_pip_no = 0;
        }
    }

    return first_pip_no;
}

/*
 * Name:
 *     particleLoadGlobals
 * Description:
 *
 * Input:
 *     None
 * Output:
 *     > 0: All particle-types are loaded
 */
int particleLoadGlobals(void)
{
    int pip_no;
    char *fname;
    
    
    for(pip_no = 1; pip_no < GLOBAL_PIP_COUNT; pip_no++)
    {
        fname = prt_file_names[pip_no];
        
        if(fname[0] != 0 && fname[0] != '-')
        {
            // @todo: Load global particles
            // fname = editfileMakeFileName();
            particleLoadOne(fname, pip_no, 0);
        }
    }
    
    return 1;
}


/* ======= Game-Functions ===== */

/*
 * Name:
 *     particleSpawn
 * Description:
 *     Spawns a particle as an object. Returns the number of the object created on success. 
 * Input:
 *     pip_no:      Use this particle type for creation of object
 *     emitter_no:  Spawned by this object (Holds spawn position)
 *     attached_to: Particle has to follow the 'emitter_no'
 *     modifier:    Precision of aiming 70..100%, strength multiplier for damage
 * Output:
 *     > 0: Number of the object created
 */
 int particleSpawn(int pip_no, int emitter_no, char attached_to, char modifier)
 {  
    SDLGL3D_OBJECT *orig_obj;


    // @port: Replaces: 'prt_config_do_init'
    // @note: The particle is a pure object. The only difference is additional handling after movement
    if(0 == emitter_no || 0 == pip_no)
    {
        // no spawner, no type, no particle
        return 0;
    }

    // Get the spawning object
    orig_obj = sdlgl3dGetObject(emitter_no);
     
    // return the number of the object, if any is created
    return particleSpawnOne(pip_no, orig_obj, attached_to, modifier, 0.0);
 }

/* ============ Particle functions ========== */


/*
 * Name:
 *     particleOnMove
 * Description:
 *     Does the additional update needed on a particle object while moving. 
 *     It'ss assumed that the particle has not collided for damage.
 *     Animation is done by the general object movement code  
 * Input:
 *     pobj *:    Do update on this object
 *     penviro *: Info about the local environment 
 * Output:
 *     0: Destroyed by movement, particle has to be destroyed by caller
 */
char particleOnMove(SDLGL3D_OBJECT *pobj, MAPENV_T *penviro)
{
    SDLGL3D_OBJECT *ptarget;
    PIP_T *ppip;
    float facing;
    int tnc, part_obj;
    
    
    ppip = &PrtDescList[pobj->type_no];     // Particle profile

    /*
    pobj->rot[2] = ppip->rotate_add;                    // Rotates around Z-Axis
    pobj->size   += (float)ppip->size_add / 65635.0;    // Change size
    */

    // Change dynalight values
    /*
    pobj->dynalight_level   += ppip->dynalight.level_add;
    pobj->dynalight_falloff += ppip->dynalight.falloff_add;
    */

    // Make it sit on the floor...  Shift is there to correct for sprite size
    // level = prtlevel[cnt]+(prtsize[cnt]>>9);

    // Do homing
    if((pobj->tags & PRT_HOMING) && pobj->target_obj != 0)
    {
        ptarget = sdlgl3dGetObject(pobj->target_obj);

        if (!(pobj->tags & PRT_ATTACHEDTO))
        {
            pobj->pos[0] += ((ptarget->pos[0] - pobj->pos[0]) * ppip->homingaccel) * ppip->homingfriction;
            pobj->pos[1] += ((ptarget->pos[1] - pobj->pos[1]) * ppip->homingaccel) * ppip->homingfriction;
            pobj->pos[2] += ((ptarget->pos[2] + (ptarget->bbox[1][2] / 2) - pobj->pos[2]) * ppip->homingaccel);
        }

        if (pobj->tags & PRT_ROTATETOFACE)
        {
            // Turn to face target
            facing = atan2(ptarget->pos[1] - pobj->pos[1], ptarget->pos[0] - pobj->pos[0]);
            facing += 180;
            pobj->rot[2] = facing;
        }
    }

    // Do speed limit on Z ??
    /*
    if (pobj->dir[2] < -ppip->spdlimit)
    {
        pobj->dir[2] = -ppip->spdlimit;
    }
    */
    
    // Spawn new particles if continually spawning
    if(ppip->contspawn_amount > 0)
    {
        // Clock is counted down by general object movement code
        if(pobj->spawn_time < 0)
        {

            pobj->spawn_time += (float)ppip->contspawn_time / 50.0;
            // Skip one spawning
            if(pobj->spawn_time <= 0.0)
            {
                // Spawn at next call
                pobj->spawn_time += (float)ppip->contspawn_time / 50.0;
            }

            facing = pobj->rot[2];
            tnc = 0;

            while(tnc < ppip->contspawn_amount)
            {
                part_obj = particleSpawnOne(ppip->contspawn_pip, pobj, 0, 0, 0);

                /*
                if(part_obj != 0 && ppip->contspawn_facingadd != 0)
                {
                    ptarget = sdlgl3dGetObject(part_obj);

                    facing += ppip->contspawn_facingadd;
                    ptarget->rot[2] += facing;
                    // Hack to fix velocity
                    ptarget->speed += pobj->speed;
                }
                */

                tnc++;
            }

        }
    }


    // Check underwater
    if (pobj->pos[2] < penviro -> waterdouselevel && (penviro->fx & MPDFX_WATER) && (ppip->flags & PRT_END_WATER))
    {
        if(penviro -> water_surface_level == 0.0)
        {
            // Mark 'zpos' as valid for override
            penviro -> water_surface_level = 0.01;
        }

        particleSpawnOne(PIP_RIPPLE, pobj, 0, 0, penviro -> water_surface_level);

        // Check for disaffirming character
        if((pobj->tags & PRT_ATTACHEDTO) && pobj->target_obj != 0)
        {
            // @todo: Disaffirm the whole character
            // disaffirm_attached_particles(pobj->target_obj);
        }
        else
        {
            return 0;
        }
    }

    return 1;
}

/*
 * Name:
 *     particleOnBump
 * Description:
 *     Does the action needed if a particle collided with something
 * Input:
 *     pobj *:    Handle this object
 *     reason:    Of collision EGOMAP_HIT_...
 *     penviro *: Pointer on description of environment for physics 
 * Output:
 *     0: Particle is killed by movement, caller should detach it from linked list
 */
char particleOnBump(SDLGL3D_OBJECT *pobj, int reason, MAPENV_T *penviro)
{   
    PIP_T *ppip;
    
    
    ppip = &PrtDescList[pobj -> obj_type]; // Particle type;
    
    
    // handle the sound and collision
    
    if(reason == EGOMAP_HIT_WALL)
    {
        // Play the sound for hitting the wall [FSND]
        particlePlaySound(ppip->end_sound_wall);
        // handle the collision
        if(pobj->tags & (PRT_END_WALL | PRT_END_BUMP))
        {
            return particleEndInGame(pobj, ppip);
        }
        else
        {
            // @todo: Only if not attached to character
            // @todo: Take collision angle into account
            /* Change direction */
            pobj->dir[0] = -pobj->dir[0];
            pobj->dir[1] = -pobj->dir[1];
            pobj->dir[2] = -pobj->dir[2];
            pobj->speed *= ppip->dampen;

            // Gravity is done by general object moving code
            // pobj->dir[2] += penviro->gravity;
        }
    }
    else if(reason == EGOMAP_HIT_FLOOR)
    {
        pobj->speed *= penviro->noslipfriction;
        
        if (pobj->dir[2] < 0)
        { 
            /* If heading towards bottom */
            if (pobj->speed > -PRT_STOP_BOUNCING)
            {
                // Make it not bounce
                pobj->speed = 0.0;
            }
            else
            {
                // Make it bounce
                pobj->dir[2] = -pobj->dir[2];
                pobj->speed *= ppip->dampen;                
            }
        }
        
        // Play the sound for hitting the floor [FSND]
        particlePlaySound(ppip->end_sound_floor);
        // handle the collision
        if(pobj->tags & PRT_END_GROUND)
        {
            return particleEndInGame(pobj, ppip);
        }
    }

    return 1;   // Particle is still alive, keep object
}


