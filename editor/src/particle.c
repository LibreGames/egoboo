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

#include <memory.h> 

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
* DEFINES								                                       *
*******************************************************************************/

#define MAX_PIP_TYPE    1024

/*******************************************************************************
* TYPEDEFS								                                       *
*******************************************************************************/

//--------------------------------------------------------------------------------------------
// Particle template
//--------------------------------------------------------------------------------------------

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
                                    /// generate_irand_pair( ppip->image_add );
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
    unsigned short contspawn_timer;         ///< Spawn timer
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

    PRT_ORI_T orientation;      ///< the way the particle orientation is calculated for display
    
    IDSZ_T exp_idsz[5];         /// Particle expansion IDSZs */
    // Data for internal use and statistics
    int request_count;
    int create_count;
    
} PIP_T;

typedef struct 
{
    int    obj_no;          // prt_ref
    SDLGL3D_OBJECT *pobj;   // SDLGL3D_OBJECT  prt_ptr

    int   pip_ref;
    PIP_T *pip_ptr;     // Pointer on its particle type
    
} PRT_BUNDLE_T;

/*******************************************************************************
* DATA									                                       *
*******************************************************************************/

// Load-Buffer for 'raw' data which is converted into 'packed' data
static char BoolVal[33];       /* Boolean values */
static char ExpIdsz[5][18 + 1]; /* Particle expansion IDSZs ( with a colon in front ) */

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
    { SDLGLCFG_VAL_USHORT, &PrtDesc.contspawn_timer },
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
    // IDSZ values
    { SDLGLCFG_VAL_STRING, ExpIdsz[0], 18 },
    { SDLGLCFG_VAL_STRING, ExpIdsz[1], 18 },
    { SDLGLCFG_VAL_STRING, ExpIdsz[2], 18 },
    { SDLGLCFG_VAL_STRING, ExpIdsz[3], 18 },
    { SDLGLCFG_VAL_STRING, ExpIdsz[4], 18 },
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
 *     particleLoadOne
 * Description:
 *     Loads one particle profile
 * Input:
 *     filename *:
 *     pip_no:      Number of profile to fill with given data 
 * Output:
 *     >0: Number of profile
 */
static int particleLoadOne(char *filename, int pip_no)
{
    int i;
    PIP_T *ppip;


    // Do the 'raw' loading
    sdlglcfgEgobooValues(filename, PrtVal, 0);

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
    
    // Change idsz-strings to idsz-values
    for(i = 0; i < 5; i++)
    {
        // @todo:
    }
    
    /* @todo:
        ppip->end_sound = CLIP( ppip->end_sound, INVALID_SOUND, MAX_WAVE );
        ppip->soundspawn = CLIP( ppip->soundspawn, INVALID_SOUND, MAX_WAVE );
     */

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
    /* @todo: Debug statistics
        fprintf( ftmp, "index == %d\tname == \"%s\"\tcreate_count == %d\trequest_count == %d\n", 
                ppip->id, ppip->name, ppip->create_count, ppip->request_count );
        "/debug/pip_usage.txt"
    */
    // Start after global Particles
    memset(&PrtDescList[GLOBAL_PIP_COUNT], 0, MAX_PIP_TYPE - GLOBAL_PIP_COUNT);    
}

/*
 * Name:
 *     particleLoad
 * Description:
 *     Loads a particle
 * Input:
 *     fname *: Name of file to load
 * Output:
 *     > 0: Number of Descriptor in 'PrtDescList'
 */
int particleLoad(char *fname)
{   
    int pip_no;


    for(pip_no = GLOBAL_PIP_COUNT; pip_no < MAX_PIP_TYPE; pip_no++)
    {
        if(PrtDescList[pip_no].id == 0)
        {
            break;
        }
    }

    if(pip_no >= MAX_PIP_TYPE)
    {
        // No Buffer left to read in the particle type
        return 0;
    }
    else
    {
        // @todo: Get the directory to read from
        if(*fname > 0)
        {
            particleLoadOne(fname, pip_no);
        }

        return pip_no;
    }
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
            particleLoadOne(fname, pip_no);
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
    float size;
    float facing, spacing_xy, spacing_z, vel_xy, vel_z;
    int obj_no;
    SDLGL3D_OBJECT new_obj, *orig_obj;
    PIP_T *ppip;


    // @port: Replaces: 'prt_config_do_init'
    // @note: The particle is a pure object. The only difference is additional handling after movement
    if(0 == emitter_no || 0 == pip_no)
    {
        // no spawner, no type, no particle
        return 0;
    }

    // Get the particle profile
    ppip = &PrtDescList[pip_no];

    // Get the spawning object
    orig_obj = sdlgl3dGetObject(emitter_no);

    // Copy all its data into the new object
    memcpy(&new_obj, orig_obj, sizeof(SDLGL3D_OBJECT));

    // New object is a particle of given type
    new_obj.obj_type = EGOMAP_OBJ_PART;
    new_obj.type_no  = pip_no;

    /* Setup the objects values */
    // Set character attachments ( pdata->chr_attach == 0 means none )
    if(attached_to)
    {
        // Set 'attached_to'
        new_obj.follow_obj = emitter_no;
    }

    // Initial spawning of this particle
    // Correct loc_facing
    ///< Facing rate     ( 0 to 1000 ) at movement
    facing      = ppip->facing_pair[0];      // Facing base: ( 0 to 65535 )
    facing     += (float)miscRandVal(ppip->facing_pair[1]) * 360.0 / 65535.0;
    spacing_xy  = ppip->spacing_hrz_pair[0]; // Spacing : ( 0 to 100 )
    spacing_xy += (float)miscRandVal(ppip->spacing_hrz_pair[1]) * 100.0 / 65535.0;
    spacing_z   = ppip->spacing_vrt_pair[0]; // Altitude : ( -100 to 100 )
    spacing_z  += (float)miscRandVal(ppip->spacing_vrt_pair[1]) * 100.0 / 65535.0;
    // Velocity data
    /*
    vel.x = -turntocos[ turn ] * velocity;
    vel.y = -turntosin[ turn ] * velocity;
    vel.z += generate_irand_pair( ppip->vel_vrt_pair ) - ( ppip->vel_vrt_pair.rand / 2 );
    */
    vel_xy      = ppip->vel_hrz_pair[0];     ///< Shot velocity : ( 0 to 100 )
    vel_xy     += (float)miscRandVal(ppip->vel_hrz_pair[1]) * 100.0 / 65535.0;    
    vel_z       = ppip->vel_vrt_pair[0];     ///< Up velocity : ( -100 to 100 )
    vel_z      += (float)miscRandVal(ppip->vel_vrt_pair[1]) * 100.0 / 65535.0;

    // @todo: Correct 'facing' for dexterity...
    new_obj.rot[2] = facing;
    // Adjust position in the original objects direction
    new_obj.pos[0] += new_obj.dir[0] * spacing_xy;
    new_obj.pos[1] += new_obj.dir[1] * spacing_xy;
    // Adjust position on Z-Axis
    new_obj.pos[2] += spacing_z;
    // General Velocity
    new_obj.speed  = vel_xy;
    new_obj.zspeed = vel_z;  
    

    // Set the turn-velocity
    ///< ( -100 to 100 ), ( 0, 1, 3, 7, 15, 31, 63... )
    new_obj.turnvel = (float)ppip->facingadd * 360.0 / 65535.0;

    // @todo: Set 'new_obj.user_flags'

    // Now set it on 'movement'
    new_obj.move_cmd = SDLGL3D_MOVE_FORWARD;

    // Set the bounding box
    size = (float)ppip->size_base * 60.0 / 65535.0; // half the size
    new_obj.bradius = size;
    new_obj.bbox[0][0] = -size;
    new_obj.bbox[0][1] = -size;
    new_obj.bbox[0][1] = -size;
    new_obj.bbox[1][0] = +size;
    new_obj.bbox[1][1] = +size;
    new_obj.bbox[1][1] = +size; 
    
    // count all the requests for this particle type
    ppip->request_count++;
    
    obj_no = sdlgl3dCreateObject(&new_obj);
    
    if(obj_no > 0)
    {
        // count all the successful spawns of this particle
        ppip->create_count++;
    }
    
    // return the number of the object, if any is created
    return obj_no;
 }

/* ============ Particle functions ========== */


/*
 * Name:
 *     particleUpdateOne
 * Description:
 *     Does the addtional update needed on a particle object. 
 *     It'ss assumed that the particle has not collided for damage  
 * Input:
 *     obj_no:  Do update on this object
 *     hit_env: Has hit environement ENV_WALL or ENV_BOTTOM or 0 
 *     // @todo: enviro *: Pointer on description of environment for physics 
 * Output:
 *     0: Particle is killed by movement,
 *        // Caller should detach its object from map and destroy it  
 */
char particleUpdateOne(int obj_no, int hit_env /* @todo: , MAPENV_T *penviro */)
{
    SDLGL3D_OBJECT *obj;
    PRT_BUNDLE_T prt_bundle;
    SDLGL3D_OBJECT  *loc_pprt;
    PIP_T           *loc_ppip;
    
    
    loc_pprt = sdlgl3dGetObject(obj_no);
    loc_ppip = &PrtDescList[loc_pprt -> obj_type]; // Particle type;
    
    // Set up the bundle
    prt_bundle.obj_no  = obj_no;
    prt_bundle.pobj    = loc_pprt;       // SDLGL3D_OBJECT  prt_ptr
    prt_bundle.pip_ref = loc_pprt -> obj_type;
    prt_bundle.pip_ptr = loc_ppip;
    
    
    /* @todo: move_one_particle( &prt_bdl ); */
    

    return 1;   // Particle is still alive, keep object
}

