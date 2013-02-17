/*******************************************************************************
*  CHAR.C                                                                      *
*    - EGOBOO-Editor                                                           *     
*                                                                              *
*    - Egoboo Characters                                                       *
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
* INCLUDES								                                       *
*******************************************************************************/

#include <stdio.h>      /* sscanf() */
#include <string.h>     /* strlwr() */

#include "sdlglcfg.h"   /* Read egoboo text files eg. passage, spawn    */
#include "editfile.h"   /* Get the work-directoires                     */
#include "egomap.h"     // Place objects on map


#include "idsz.h";
#include "misc.h"       /* Random treasure objects                      */
#include "egodefs.h"    /* MPDFX_IMPASS                                 */

// Own header
#include "char.h"

/*******************************************************************************
* DEFINES								                                       *
*******************************************************************************/

#define MAXCAPNAMESIZE     32   ///< Character class names

//Levels
#define MAXBASELEVEL        6   ///< Basic Levels 0-5
#define MAXLEVEL           20   ///< Absolute max level
#define CAP_MAX_PRT        10   /// Maximum particles for character profile

#define GRIP_VERTS          4

#define CAP_INFINITE_WEIGHT  0xFF
#define CAP_MAX_WEIGHT       (CAP_INFINITE_WEIGHT - 1)

#define CHAR_MAX_CAP    180
#define CHAR_MAX_SKIN     4
// Maximum of characters that can be loaded, for each an AI_STATE may be needed
#define CHAR_MAX        500

// Other maximum values
#define PERFECTSTAT   18    // wisdom, intelligence and so on...
#define PERFECTBIG  1000    // Maximum life/mana

// Clock-Times for special character clocks
// Times in seconds or in ticks ?
//Timer resets
#define DAMAGETILETIME      32                            ///< Invincibility time
#define DAMAGETIME          32                            ///< Invincibility time
#define DEFENDTIME          24                            ///< Invincibility time
#define BORETIME            320 // @todo((Uint16)generate_randmask( 255, 511 )) ///< IfBored timer
#define CAREFULTIME         50                            ///< Friendly fire timer
#define SIZETIME            100                           ///< Time it takes to resize a character

// team
#define TEAM_MAX            32  // Mum number of teams
#define TEAM_MAX_MEMBER     6   // Maximum members of a team 
// Spawn points
#define CHAR_MAXSPAWN    500      /* Maximum Lines in spawn list          */

/*******************************************************************************
* ENUMS								                                           *
*******************************************************************************/

/// The various ID strings that every character has
enum
{
    IDSZ_NONE   = 0,
    IDSZ_PARENT = 1,                             ///< Parent index
    IDSZ_TYPE,                                   ///< Self index
    IDSZ_SKILL,                                  ///< Skill index
    IDSZ_SPECIAL,                                ///< Special index
    IDSZ_HATE,                                   ///< Hate index
    IDSZ_VULNERABILITY,                          ///< Vulnerability index
    IDSZ_COUNT                                   ///< ID strings per character

} E_IDSZ;

/*******************************************************************************
* TYPEDEFS								                                       *
*******************************************************************************/

/// The character statistic data in the form used in data.txt
typedef struct
{
    int val[2];       /* from, to */
    int perlevel[2];  /* from, to */
    
} CAP_STAT_T;

// Definition of a team
typedef struct 
{
    int  members[TEAM_MAX_MEMBER];
    int  leader_no;                 // Index of leader in 'members'-array
    char morale;                    ///< Number of characters on team
    int  sissy_no;                  ///< Whoever called for help last Index of leader in 'members'-array
    unsigned int friends;           // Our friends as flags
    unsigned int foes;              // Our foes as flags
    
} TEAM_T;   

//--------------------------------------------------------------------------------------------

/// The character profile data, or cap (Type of Character)
/// The internal representation of the information in data.txt
typedef struct 
{
    char cap_name[MAXCAPNAMESIZE];             /// Name of this character profile (directory)
    // naming
    char class_name[MAXCAPNAMESIZE];           ///< Class name
    // skins
    char           skinname[CHAR_MAX_SKIN][MAXCAPNAMESIZE];   ///< Skin name
    unsigned short skincost[CHAR_MAX_SKIN];                   ///< Store prices
    float          maxaccel[CHAR_MAX_SKIN];                   ///< Acceleration for each skin
    unsigned char  skindressy;                           ///< Bits to tell whether the skins are "dressy"

    // overrides
    int           skin_override;                  ///< -1 or 0-3.. For import
    unsigned char level_override;                 ///< 0 for normal
    int           state_override;                 ///< 0 for normal
    int           content_override;               ///< 0 for normal

    unsigned int  idsz[IDSZ_COUNT];                   ///< ID strings

    // inventory
    unsigned char ammo_max;                        ///< Ammo stuff
    unsigned char ammo;
    short int     money;                          ///< Money

    // character stats
    unsigned char gender;                        ///< Gender

    // life
    CAP_STAT_T   life_stat;                     ///< Life statistics
    unsigned int life_return;                    ///< Life regeneration
    unsigned int life_heal;
    unsigned int life_spawn;                     ///< Life left from last module

    // mana
    CAP_STAT_T   mana_stat;             ///< Mana statistics
    CAP_STAT_T   mana_return_stat;      ///< Mana regeneration statistics
    CAP_STAT_T   mana_flow_stat;        ///< Mana channeling
    unsigned int mana_spawn;            ///< Mana left from last module

    CAP_STAT_T strength_stat;         ///< Strength
    CAP_STAT_T wisdom_stat;           ///< Wisdom
    CAP_STAT_T intelligence_stat;     ///< Intlligence
    CAP_STAT_T dexterity_stat;        ///< Dexterity

    // physics
    unsigned char weight;        ///< Weight
    float dampen;                        ///< Bounciness
    float bumpdampen;                    ///< Mass

    float size;                         ///< Scale of model
    float size_perlevel;                ///< Scale increases
    int   shadow_size;                  ///< Shadow size
    int   bump_size;                    ///< Bounding octagon
    char  bump_override_size;           ///< let bump_size override the measured object size
    int   bump_sizebig;                 ///< For octagonal bumpers
    char  bump_override_sizebig;        ///< let bump_sizebig override the measured object size
    int   bump_height;                  ///< the height of the object
    char  bump_override_height;         ///< let bump_height overrride the measured height of the object
    unsigned char stoppedby;            ///< Collision Mask

    // movement
    float jump;                         ///< Jump power
    char  jump_number;                  ///< Number of jumps ( Ninja )
    float anim_speed_sneak;             ///< Sneak threshold
    float anim_speed_walk;              ///< Walk threshold
    float anim_speed_run;               ///< Run threshold
    unsigned char fly_height;           ///< Fly height
    char  waterwalk;                    ///< Walk on water?

    // status graphics
    unsigned char life_color;           ///< Life bar color
    unsigned char mana_color;           ///< Mana bar color
    char draw_icon;                     ///< Draw icon

    // model graphics
    unsigned char flashand;                      ///< Flashing rate
    unsigned char alpha;                         ///< Transparency
    unsigned char light;                         ///< Light blending
    char          transferblend;                 ///< Transfer blending to rider/weapons
    unsigned char sheen;                         ///< How shiny it is ( 0-15 )
    char enviro;                        ///< Phong map this baby?
    int  uoffvel;                       ///< "horizontal" texture movement rate
    int  voffvel;                       ///< "vertical" texture movement rate
    char uniformlit;                    ///< Bad lighting?
    char reflect;                        ///< Draw the reflection
    char alwaysdraw;                     ///< Always render
    char forceshadow;                    ///< Draw a shadow?
    char ripple;                         ///< Spawn ripples?unsigned char        mana_color;                     ///< Mana bar color

    // attack blocking info
    unsigned short iframefacing;                  ///< Invincibility frame
    unsigned short iframeangle;
    unsigned short nframefacing;                  ///< Normal frame
    unsigned short nframeangle;

    // defense
    char          resistbumpspawn;                        ///< Don't catch fire
    unsigned char defense[CHAR_MAX_SKIN];                      ///< Defense for each skin
    unsigned char damagemodifier[DAMAGE_COUNT][CHAR_MAX_SKIN];

    // xp
    int   experience_forlevel[MAXLEVEL];    ///< Experience needed for next level
    float experience[2];                    ///< Starting experience
    unsigned short experience_worth;               ///< Amount given to killer/user
    float experience_exchange;            ///< Adds to worth
    float experience_rate[XP_COUNT];

    // sound
    char  sound_index[SOUND_COUNT];       ///< a map for soundX.wav to sound types

    // flags
    char isequipment;                    ///< Behave in silly ways
    char isitem;                         ///< Is it an item?
    char ismount;                        ///< Can you ride it?
    char isstackable;                    ///< Is it arrowlike?
    char invictus;                       ///< Is it invincible?
    char platform;                       ///< Can be stood on?
    char canuseplatforms;                ///< Can use platforms?
    char cangrabmoney;                   ///< Collect money?
    char canopenstuff;                   ///< Open chests/doors?
    char canbedazed;                     ///< Can it be dazed?
    char canbegrogged;                   ///< Can it be grogged?
    char istoobig;                       ///< Can't be put in pack
    char isranged;                       ///< Flag for ranged weapon
    char nameknown;                      ///< Is the class name known?
    char usageknown;                     ///< Is its usage known
    char cancarrytonextmodule;           ///< Take it with you?
    unsigned char damagetargettype;      ///< For AI DamageTarget
    char slotvalid[SLOT_COUNT];          ///< Left/Right hands valid
    char ridercanattack;                 ///< Rider attack?
    unsigned char kursechance;                    ///< Chance of being kursed
    char hidestate;                      ///< Don't draw when...
    char isvaluable;                     ///< Force to be valuable
    int  spelleffect_type;               ///< is the object that a spellbook generates

    // item usage
    char          needskillidtouse;               ///< Check IDSZ first?
    unsigned char weaponaction;                   ///< Animation needed to swing
    short int     manacost;                       ///< How much mana to use this object?
    unsigned char attack_attached;                ///< Do we have attack particles?
    char          attack_pip;                     ///< What kind of attack particles?
    char          attack_fast;                    ///< Ignores the default reload time?

    float str_bonus;                      ///< Strength     damage factor
    float wis_bonus;                      ///< Wisdom       damage factor
    float int_bonus;                      ///< Intelligence damage factor
    float dex_bonus;                      ///< dexterity    damage factor

    // special particle effects
    unsigned char attachedprt_amount;             ///< Number of sticky particles
    unsigned char attachedprt_reaffirmdamagetype; ///< Re-attach sticky particles? Relight that torch...
    int           attachedprt_pip;                ///< Which kind of sticky particle
    unsigned char gopoofprt_amount;               ///< Amount of poof particles
    short int     gopoofprt_facingadd;            ///< Angular spread of poof particles
    int           gopoofprt_pip;                  ///< Which poof particle
    unsigned char blud_valid;                     ///< Has blud? ( yuck )
    int           blud_pip;                       ///< What kind of blud?

    // skill system
    IDSZ_T       skills[MAX_IDSZ_MAP_SIZE];
    int          see_invisible_level;             ///< Can it see invisible?

    // random stuff
    char       stickybutt;      ///< Stick to the ground?
    // For graphics
    char       mdl_no;          /// < Display model for this character profile
    // particles for this profile
    int prt_list[CAP_MAX_PRT];
    // @todo: Possible other solution 'flexible' number of particles, 
    //        loaded in consecutive order
    int prt_first_no;           // Number of first particle
    int prt_cnt;                // Total number of particles
    
} CAP_T;


/*******************************************************************************
* DATA									                                   *
*******************************************************************************/

static EDITFILE_SPAWNPT_T SpawnPts[CHAR_MAXSPAWN + 2];    /* Holds data about chosen spawn point  */
// General info about teams
static TEAM_T TeamList[TEAM_MAX + 2];     
// Character profiles
static CAP_T  CapList[CHAR_MAX_CAP + 2];
// Characters itself, created from character-profiles
static CHAR_T CharList[CHAR_MAX + 2];

// Description for reading a cap-file
static SDLGLCFG_NAMEDVALUE CapVal[] =
{
    // Read in the real general data
    { SDLGLCFG_VAL_STRING, CapList[0].class_name, MAXCAPNAMESIZE },
    // Light cheat
    { SDLGLCFG_VAL_BOOLEAN, &CapList[0].uniformlit },
    // Ammo
    { SDLGLCFG_VAL_INT, &CapList[0].ammo_max },
    { SDLGLCFG_VAL_INT, &CapList[0].ammo },
    // Gender
    { SDLGLCFG_VAL_STRING, &CapList[0].gender },
     // Read in the icap stats
    { SDLGLCFG_VAL_INT, &CapList[0].life_color },
    { SDLGLCFG_VAL_INT, &CapList[0].mana_color },
    // Different stats
    { SDLGLCFG_VAL_IPAIR, &CapList[0].life_stat.val },
    { SDLGLCFG_VAL_IPAIR, &CapList[0].life_stat.perlevel  },

    { SDLGLCFG_VAL_IPAIR, &CapList[0].mana_stat.val },
    { SDLGLCFG_VAL_IPAIR, &CapList[0].mana_stat.perlevel  },
    
    { SDLGLCFG_VAL_IPAIR, &CapList[0].mana_return_stat.val },
    { SDLGLCFG_VAL_IPAIR, &CapList[0].mana_return_stat.perlevel  },
    
    { SDLGLCFG_VAL_IPAIR, &CapList[0].mana_flow_stat.val },
    { SDLGLCFG_VAL_IPAIR, &CapList[0].mana_flow_stat.perlevel  },
    
    { SDLGLCFG_VAL_IPAIR, &CapList[0].strength_stat.val },
    { SDLGLCFG_VAL_IPAIR, &CapList[0].strength_stat.perlevel  },

    { SDLGLCFG_VAL_IPAIR, &CapList[0].wisdom_stat.val },
    { SDLGLCFG_VAL_IPAIR, &CapList[0].wisdom_stat.perlevel  },

    { SDLGLCFG_VAL_IPAIR, &CapList[0].intelligence_stat.val },
    { SDLGLCFG_VAL_IPAIR, &CapList[0].intelligence_stat.perlevel  },

    { SDLGLCFG_VAL_IPAIR, &CapList[0].dexterity_stat.val },
    { SDLGLCFG_VAL_IPAIR, &CapList[0].dexterity_stat.perlevel  },

    // More physical attributes
    { SDLGLCFG_VAL_FLOAT, &CapList[0].size },
    { SDLGLCFG_VAL_FLOAT, &CapList[0].size_perlevel },
    { SDLGLCFG_VAL_INT, &CapList[0].shadow_size },
    { SDLGLCFG_VAL_INT, &CapList[0].bump_size },
    { SDLGLCFG_VAL_INT, &CapList[0].bump_height },
    { SDLGLCFG_VAL_FLOAT, &CapList[0].bumpdampen },
    { SDLGLCFG_VAL_INT, &CapList[0].weight },
    { SDLGLCFG_VAL_FLOAT, &CapList[0].jump },
    { SDLGLCFG_VAL_INT, &CapList[0].jump_number },
    { SDLGLCFG_VAL_FLOAT, &CapList[0].anim_speed_sneak },
    { SDLGLCFG_VAL_FLOAT, &CapList[0].anim_speed_walk },
    { SDLGLCFG_VAL_FLOAT, &CapList[0].anim_speed_run },
    { SDLGLCFG_VAL_INT, &CapList[0].fly_height },
    { SDLGLCFG_VAL_INT, &CapList[0].flashand },
    { SDLGLCFG_VAL_INT, &CapList[0].alpha },
    { SDLGLCFG_VAL_INT, &CapList[0].light },
    { SDLGLCFG_VAL_BOOLEAN, &CapList[0].transferblend },
    { SDLGLCFG_VAL_INT, &CapList[0].sheen },
    { SDLGLCFG_VAL_BOOLEAN, &CapList[0].enviro },
	{ 0 }
};


/*******************************************************************************
* CODE									                                   *
*******************************************************************************/

/*
 * Name:
 *     charSetupXPTable
 * Description:
 *     This calculates the xp needed to reach next level and stores it in an array for later use
 * Input:
 *     pcap *: POinter on character profile to setup XP-Table for
 * Output:
 *     >0: Number of character profile
 */
static char charSetupXPTable(CAP_T *pcap)
{
    char level;
    int xpneeded;


    // Calculate xp needed
    for (level = MAXBASELEVEL; level < MAXLEVEL; level++ )
    {
        xpneeded = pcap->experience_forlevel[MAXBASELEVEL - 1];
        xpneeded += ( level * level * level * 15 );
        xpneeded -= (( MAXBASELEVEL - 1 ) * ( MAXBASELEVEL - 1 ) * ( MAXBASELEVEL - 1 ) * 15 );
        pcap->experience_forlevel[level] = xpneeded;
    }
    
    return 1;
}



/*
 * Name:
 *     charNewCap
 * Description:
 *     Returns the number for an empty character profile slot, if available
 * Input:
 *     None
 * Output:
 *     >0: Number of character profile
 */
static int charNewCap(void)
{
    int i, cnt;
    CAP_T *pcap;

    
    // 0: Is an invalid cap    
    for(i = 1; i < CHAR_MAX_CAP; i++)
    {
        if(CapList[i].cap_name[0] == 0)
        {
            pcap = &CapList[i];

            // clear out all the data
            memset(pcap, 0, sizeof(CAP_T));

            // clear out the sounds
            for ( cnt = 0; cnt < SOUND_COUNT; cnt++ )
            {
                pcap->sound_index[cnt] = -1;
            }

            // Clear expansions...
            pcap->reflect = 1;
            pcap->isvaluable = -1;

            // either these will be overridden by data in the data.txt, or
            // they will be limited by the spawning character's max stats
            pcap->life_spawn = 10000;
            pcap->mana_spawn = 10000;

            // More stuff I forgot
            pcap->stoppedby  = MPDFX_IMPASS;
            
            // Now return its number            
            return i;
        }
    }
    
    // no empty slot left
    return 0;
}

/*
 * Name:
 *     charNewChar
 * Description:
 *     Returns the number for an empty character slot, if available
 * Input:
 *     None
 */
static int charNewChar(void)
{
    int i;

    
    for(i = 1; i < CHAR_MAX; i++)
    {
        // 0: Is emtpy, -1: Is deleted
        if(CharList[i].cap_no <= 0)
        {
            // clear out all the data
            memset(&CharList[i], 0, sizeof(CHAR_T));
            
            return i;
        }
    }

    return 0;
}

/*
 * Name:
 *     charReadCap
 * Description:
 *     Reads a character profile by name, if not already loaded.
 *     - For "PLAYER"-Slots an empty profile is created for override by imported characters
 *     - "unknown" slots are skipped
 *     - @todo; Support Random Treasure Names '%' if not in edit mode  
 * Input:
 *     pname *: Pointer on name of random treasure
 * Output:
 *     Number
 */
 static int charReadCap(char *objname)
 {
    char name_buf[32];
    int  player_no;
    int cno;
    // char *fdir;
    // char fname[512];
    CAP_T *pcap;

    
    // 1. PLAYER 1-4: Empty cap slots to be filled by 'saved game'-character profiles  
    if(strncmp(objname, "PLAYER", 6) == 0)
    {
        sscanf(objname, "%s %d", name_buf, &player_no);
        
        if(player_no > 0 && player_no < 5)
        {
            // Get the object name 
            strcpy(CapList[player_no].cap_name, objname);
            // Slot for this imported player
            return player_no;
        }
    }
    
    // Remove leading and trailing spaces
    sscanf(objname, "%s", objname);    
    
    // Compare lower case except names for random treasure
    if('%' != objname[0])
    {
        strlwr(objname);    
    }
    
    // Skip "unknown"
    if(strcmp(objname, "unknown") == 0)
    {
        return 0;
    }
    
    // @todo; Support Random Treasure '%' if not in edit mode

    // Only read if 'cap_name' not available yet
    for(cno = 5; cno < CHAR_MAX_CAP; cno++)
    {
        if(CapList[cno].cap_name[0] != 0)
        {
            if(strcmp(CapList[cno].cap_name, objname) == 0)
            {
                // Profile is already loaded
                return cno;
            }
        }
        else
        {
            break;  // End of list
        }
    }

    cno = charNewCap();

    if(cno > 4)
    {
        pcap = &CapList[cno];

        strcpy(pcap->cap_name, objname);  // For checking if profile and object are already loaded and editor
        /* @todo: Load random treasure object if in 'game-mode'
        if('%' == objname)
        {
        }        
        */
        /* @todo: Load the real data
        // Get the directory of this object        
        fdir = editfileMakeFileName(EDITFILE_OBJECTDIR, objname);
        
        // Read character profile        
        if(fdir[0] != 0)
        {
            // If object was found
            sprintf(fname, "%sdata.txt", fdir); 
            sdlglcfgEgobooValues(fname, CapVal, 0);

            // sprintf(fname, "%smessage.txt", fdir);
            // @todo: msgObjectLoad(char *fname)

            // @todo: Function for loading the naming data
            // sprintf(fname, "%snaming.txt", fdir);            
            // @todo: Load its particles
            // "part0.txt" - "part9.txt"
            // pcap->prt_first_no = particleLoad(fdir, &pcap->prt_cnt)
            //
            // sprintf(fname, "%stris.md2", fdir);
            // "icon0,bmp" - "icon4.bmp"
            // "tris0.bmp" - "tris4.bmp"
            // "sound0.wav" - "sound9.wav"
            // @todo: Load its scripts
            // sprintf(fname, "%sscript.txt", fdir);
            // charSetupXPTable(pcap);
        }
        else
        {
            // @todo: msgSend(MSG_LOG, MSG_GAME_OBJNOTFOUND, objname)
        }
        */
        // @todo: Replace this be 'real' code
        
        
        
        return cno;
    }

    return 0;
}


/* ================ Game functions ================== */

/*
 * Name:
 *     charDoLevelUp
 * Description:
 *     level gains are done here, but only once a second
 * Input:
 *     char_no: Number of character to do the level up for
 */
static void charDoLevelUp(const int char_no)
{
    CHAR_T *pchar;
    CAP_T *pcap;
    char curlevel;
    int  number;
    int xpcurrent, xpneeded;


    pchar = &CharList[char_no];
    pcap  = &CapList[pchar->cap_no];

    if (0 == pchar->cap_no) return;

    // Do level ups and stat changes
    curlevel = (char)(pchar->experience_level + 1);
    
    if (curlevel < MAXLEVEL)
    {
        xpcurrent = pchar->experience;
        xpneeded  = pcap->experience_forlevel[curlevel];
        
        if ( xpcurrent >= xpneeded )
        {
            // do the level up                
            pchar->experience_level++;
            // The character is ready to advance...
            // msgSend(0, character, MSG_LEVELUP);

            // Size
            pchar->fat_goto += pcap->size_perlevel * 0.25f;  // Limit this?
            pchar->fat_goto_time += SIZETIME;

            // Strength
            number = miscRandRange( pcap->strength_stat.perlevel );
            number += pchar->str[1];
            if ( number > PERFECTSTAT ) number = PERFECTSTAT;
            pchar->str[0] += (char)(number - pchar->str[1]);
            pchar->str[1] = (char)number;

            // Wisdom
            number = miscRandRange( pcap->wisdom_stat.perlevel );
            number += pchar->wis[1];
            if ( number > PERFECTSTAT ) number = PERFECTSTAT;
            pchar->wis[0] += (char)(number - pchar->wis[1]);
            pchar->wis[1] = (char)number;

            // Intelligence
            number = miscRandRange( pcap->intelligence_stat.perlevel );
            number += pchar->intel[1];
            if (number > PERFECTSTAT) number = PERFECTSTAT;
            pchar->intel[0] += (char)(number - pchar->intel[1]);
            pchar->intel[1] = (char)number;

            // Dexterity
            number = miscRandRange( pcap->dexterity_stat.perlevel );
            number += pchar->dex[1];
            if (number > PERFECTSTAT) number = PERFECTSTAT;
            pchar->dex[0] += (char)(number - pchar->dex[1]);
            pchar->dex[1] = (char)number;

            // Life
            number = miscRandRange( pcap->life_stat.perlevel );
            number += pchar->life[1];
            if (number > PERFECTBIG) number = PERFECTBIG;
            pchar->life[0] += (short int)(number - pchar->life[1]);
            pchar->life[1] = (short int)number;

            // Mana
            number = miscRandRange( pcap->mana_stat.perlevel );
            number += pchar->mana[1];
            if (number > PERFECTBIG) number = PERFECTBIG;
            pchar->mana[0] += (char)(number - pchar->mana[1]);
            pchar->mana[1] = (char)number;

            // Mana Return
            number = miscRandRange( pcap->mana_return_stat.perlevel );
            number += pchar->mana_return;
            if (number > PERFECTSTAT) number = PERFECTSTAT;
            pchar->mana_return = (char)number;

            // Mana Flow
            number = miscRandRange( pcap->mana_flow_stat.perlevel );
            number += pchar->mana_flow;
            if (number > PERFECTSTAT) number = PERFECTSTAT;
            pchar->mana_flow = (char)number;
        }
    }
}

/*
 * Name:
 *     charCreate
 * Description:
 *     Creates a new character.
 *     If needed, the character profile is loaded:
 *      - Looks up first in the modules directory
 *      - Second it looks that in the GOR
 * Input:
 *     spt *:     Create from this spawn point description
 * Output:
 *     Number of character, if any is created
 */
static int charCreate(EDITFILE_SPAWNPT_T *spt)
{
    int cap_no;
    int char_no;
    CAP_T  *pcap;
    CHAR_T *pchar;


    // @todo: Support Random Treasure
    cap_no = charReadCap(spt->obj_name);

    if(cap_no > 0)
    {
        char_no = charNewChar();

        if(char_no > 0)
        {
            pcap  = &CapList[cap_no];
            pchar = &CharList[char_no];

            // @todo: Complete creation of character profile
            // @todo: Change boolean values to flags
            pchar->id      = char_no;         /* Number of slot > 0: Is occupied      */
            pchar->cap_no  = cap_no;          /* Has this character profile           */
            pchar->mdl_no  = pcap->mdl_no;    /* Number of model for display          */
            pchar->icon_no = spt->skin;       /* Number of models icon for display    */
            pchar->skin_no = spt->skin;       /* Number of models skin for display    */
            // Attached to passage
            pchar->psg_no  = spt->pas;
            pchar->gender  = pcap->gender;            
            // Gender
            if('R' == pchar->gender)
            {
                // @todo: Set random gender
            }
            
            // Team stuff
            spt->team = (char)(spt->team - 'A'); // From 0 to ('Z' - 'A')
            pchar->team[0] = spt->team;          // Actual team
            pchar->team[1] = spt->team;          // Original team

            pchar->money = spt->money;   
            /* Info for editor: Name of object, for further naming: Name of it's class */
            pchar->obj_name   = pcap->cap_name;
            pchar->class_name = pcap->class_name;
            
            // Return number of slot
            return char_no;
        }
    }
    
    return 0;
}

/*
 * Name:
 *     charSpawnFromSpt
 * Description:
 *     Spawns an object on map, based on given spawn point
 * Input:
 *     spt *:  Description of spawn point
 */
 static void charSpawnFromSpt(EDITFILE_SPAWNPT_T *spt)
 {
    static inv_char = 0;   // Old character for inventory

    int char_no;
    int obj_no;             // 3D-Object generated
    char slot_no;           /* In inventory, if any */


    if(spt->view_dir == 'R')
    {
        slot_no = 0;    /* Also for mounts */
    }
    else if(spt->view_dir == 'L')
    {
        slot_no = 1;
    }
    else if(spt->view_dir == 'I')
    {
        slot_no = -1;
    }
    else
    {
        slot_no = 10;   /* Is a character on map */
    }

    char_no = charCreate(spt);

    if(char_no > 0)
    {
        if(slot_no == 10 && spt->x_pos > 0 && spt->y_pos > 0)
        {
            /* Inventory belongs to this character, if any */
            inv_char = char_no;
            // Is a character with an inventory -- Drop it to map
            obj_no = egomapPutChar(char_no, spt->x_pos, spt->y_pos, spt->z_pos, spt->view_dir, CharList[char_no].mdl_no);
            // Link back to object from character
            CharList[char_no].obj_no = obj_no;
        }
        else
        {
            // Is an inventory object -- put it there
            charInventoryAdd(inv_char, char_no, slot_no);
        }
        // Attach the character to a passage, if needed
        if(spt->pas > 0)
        {
            egomapPassageFunc(spt->pas, EGOMAP_PSGOWNER, char_no, NULL);
        }
    }
}

/* ========================================================================== */
/* ============================= PUBLIC FUNCTION(S) ========================= */
/* ========================================================================== */

/* ================= Basic character functions =============== */

/*
 * Name:
 *     charInit
 * Description:
 *     Initializes the character data buffers
 * Input:
 *     None
 */
void charInit(void) 
{       
    int i;
    unsigned int t_flag;
    
    
    // Clear the buffer for the character profiles
    memset(&CapList[0], 0, CHAR_MAX_CAP * sizeof(CAP_T)); 
    // Clear the buffer for the characters
    memset(&CharList[0], 0, CHAR_MAX * sizeof(CHAR_T)); 
    
    /* ----- Initalize the teams ------ */
    memset(&TeamList[0], 0, TEAM_MAX * sizeof(TEAM_T)); 
    for (i = 0; i < TEAM_MAX; i++)
    {
        t_flag = (1 << i);
        // Preset before 'alliance.txt' data is loaded
        TeamList[i].friends = t_flag;   // we are our friends
        TeamList[i].foes    = ~t_flag;  // all other are our foes
    }
    // The neutral team hates nobody
    TeamList[('N' - 'A')].foes = 0;
}

/*
 * Name:
 *     charGet
 * Description:
 *     Returns a pointer on a valid character, if available 
 * Input:
 *     char_no: Number of character
 * Output:
 *     Pointer on character
 */
CHAR_T *charGet(int char_no)
{
    // Return pointer on character description
    return &CharList[char_no];
} 

/*
 * Name:
 *     charSpawnAll
 * Description:
 *     Spawns all characters for the actual set module directory 
 * Input:
 *     None
 */
void charSpawnAll(void)
{
    EDITFILE_SPAWNPT_T *spt;
    int i;
    
    /* Load the spawn points */
    spt = &SpawnPts[1];
    editfileSpawn(spt, EDITFILE_ACT_LOAD, CHAR_MAXSPAWN);

    i = 1;
    while(spt->obj_name[0] != 0)
    {
        // Add data for editor -- Number of spawn point
        spt->rec_no = i;

        // Spawn the object
        charSpawnFromSpt(spt);

        spt++;
        i++;
    }
}

/* ================= Inventory functions ===================== */

/*
 * Name:
 *     charInventoryAdd
 * Description:
 *     This adds a new item into the specified inventory slot. Fails if there already is an item there.
 *     If the specified inventory slot is < 0, it will find the first free inventory slot.
 * Input:
 *     char_no: Number of character to remove item from
 *     item_no: Number of item 
 *     slot_no: Put to this slot, has to be 'absolute' (SLOT_COUNT .. INVEN_COUNT-1)
 * Output:
 *     Worked yes/no
 */
char charInventoryAdd(const int char_no, const int item_no, int slot_no)
{
    CHAR_T *pchar, *pitem;
    int i;


    pchar = &CharList[char_no];

    if(pchar->cap_no > 0 && CharList[item_no].cap_no > 0)
    {
        pitem = &CharList[item_no];

        // don't allow sub-inventories, e.g.
        if(pitem->attached_to > 0)
        {
            return 0;
        }

        //too big item?
        if(pitem->istoobig)
        {
            // SET_BIT( pitem->ai.alert, ALERTIF_NOTPUTAWAY );
            if(pchar->islocalplayer)
            {
                // @todo: Send message for display or to AI/Player
                // msgSend(char_no, MSG_TOOBIG, chr_get_name())
                // DisplayMsg_printf("%s is too big to be put away...", chr_get_name( item, CHRNAME_ARTICLE | CHRNAME_DEFINITE | CHRNAME_CAPITAL, NULL, 0 ) );
            }
            return 0;
        }

        if(slot_no < 0)
        {
            // Find an empty slot
            for(i = SLOT_COUNT; i < INVEN_COUNT; i++)
            {
                if(pchar->inventory[i].item_no == 0)
                {
                    slot_no = i;
                    break;
                }
            }
        }

        // @todo: Support stacked items (In hand too ?)
        // @todo: Support 'merge' items

        // don't override existing items
        if(slot_no < INVEN_COUNT)
        {
            if(pchar->inventory[slot_no].item_no == 0)
            {
                pchar->inventory[slot_no].item_no = item_no;
                // Link back to character
                // display code has to check slots 0 and 1 for display in 3D
                pitem->attached_to = char_no;

                // @todo: Message to player that it has picked up an item ?!
                // msgSend(char_no, MSG_ITEMFOUND, chr_get_name())
            }
        }
    }

    return 0;
}

/*
 * Name:
 *     charInventoryRemove
 * Description:
 *     This function removes the item specified in the inventory slot from the
 *     character's inventory. Note that you still have to handle it falling out to the map
 * Input:
 *     char_no: Number of character to remove item from
 *     slot_no: Remove it from this slot
 *     ignorekurse:  
 * Output:
 *     > 0: Number of character removed from inventory
 */
int charInventoryRemove(const int char_no, int slot_no, char ignorekurse)
{
    CHAR_T *pchar, *pitem;
    int item_no;

    
    pchar = &CharList[char_no];

    if(pchar->cap_no > 0 && slot_no < INVEN_COUNT)
    {
        item_no = pchar->inventory[slot_no].item_no;
        
        if(item_no > 0)
        {
            pitem = &CharList[item_no];
            
            if (pitem->iskursed && !ignorekurse )
            {
                // Flag the last found_item as not removed
                /*
                @todo: msgSend(char_no, ALERTIF_NOTTAKENOUT, chr_get_name())
                SET_BIT( pitem->ai.alert, ALERTIF_NOTTAKENOUT );  // Same as ALERTIF_NOTPUTAWAY
                if ( pchar->islocalplayer ) DisplayMsg_printf( "%s won't go out!", chr_get_name( item, CHRNAME_ARTICLE | CHRNAME_DEFINITE | CHRNAME_CAPITAL, NULL, 0 ) );
                */
                return 0;
            }
            else
            {
                // Remove item from inventory
                pchar->inventory[slot_no].item_no = 0;
                pitem->attached_to  = 0;
                // Return number of item to caller for further handling
                return item_no;
            }
        }
    }
    
    return 0;
}

/*
 * Name:
 *     charInventorySwap
 * Description:
 *     This function swaps items between the specified inventory slot and the specified grip
 *     If slot_no < 0 is , the function will swap with the first item found in the inventory
 * Input:
 *     char_no:  Number of character to remove item from
 *     slot_no:  Swap with this slot
 *     grip_off: To this grip
 * Output:
 *     Worked yes/no
 */
char charInventorySwap(const int char_no, int slot_no, int grip_off)
{
    int i;
    CHAR_T *pchar;
    int inv_item, grip_item;

    pchar = &CharList[char_no];
    
    if(pchar->cap_no > 0)
    {
        if(slot_no < 0)
        {
            // Find the first slot
            for(i = SLOT_COUNT; i < INVEN_COUNT; i++)
            {
                if(pchar->inventory[i].item_no > 0)
                {
                    slot_no = i;
                    break;
                }
            }
        }

        if(grip_off <= HAND_RIGHT && slot_no > HAND_RIGHT && slot_no < INVEN_COUNT)
        {
            inv_item  = pchar->inventory[slot_no].item_no;
            grip_item = pchar->inventory[grip_off].item_no;

            // Make space to put the item into the grip
            pchar->inventory[grip_off].item_no = 0;

            // Put inventory item to grip
            if(charInventoryAdd(char_no, inv_item, grip_off))
            {
                // Put item from grip to inventory
                pchar->inventory[slot_no].item_no = grip_item;
            }
            else
            {
                // Put item back to grip
                pchar->inventory[grip_off].item_no = grip_item;
            }
        }
    }

    return 0;
}

/* ============ Other functions ======================== */
/*
 * Name:
 *     charSpawnPoint
 * Description:
 *     This function does the handlig of spawn points
 * Input:
 *     sp_no:  Work with this spawn point
 *     spt *:  Pointer on Spawn-Point data to get/set
 *     action: What to do with this spawn point
 * Output:
 *     Number of spawn point, if a new one is created
 */
int charSpawnPoint(int sp_no, EDITFILE_SPAWNPT_T *spt, char action)
{
    int i;
    
    
    switch(action)
    {
        case CHAR_SPTNEW:
            /* Look for buffer available */
            for (i = 1; i < CHAR_MAXSPAWN; i++)
            {
                if (SpawnPts[i].rec_no <= 0)
                {   
                    SpawnPts[i].rec_no = i;
                    SpawnPts[i].x_pos  = spt->x_pos;
                    SpawnPts[i].y_pos  = spt->y_pos;
                    SpawnPts[i].z_pos  = spt->z_pos;

                    /* Give it a name and make it valid */
                    sprintf(SpawnPts[i].obj_name, "%02dPassage", i);

                    /* -- Return data to caller -- */
                    memcpy(spt, &SpawnPts[i], sizeof(EDITFILE_SPAWNPT_T));
                    return i;
                }
            }
            break;
        case CHAR_SPTSET:
            // Save changes
            if (sp_no > 0 && sp_no < CHAR_MAXSPAWN)
            {
                memcpy(&SpawnPts[sp_no], spt, sizeof(EDITFILE_SPAWNPT_T));
            }
            break;
        case CHAR_SPTCLEAR:
            if (sp_no > 0 && sp_no < CHAR_MAXSPAWN)
            {
                memset(spt, 0, sizeof(EDITFILE_SPAWNPT_T));
                spt->rec_no = -1;     /* Sign it as free */
            }
            break;
    }

    return 0;
}