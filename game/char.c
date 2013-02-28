/*******************************************************************************
*  CHAR.C                                                                      *
*    - EGOBOO-Game                                                             *
*                                                                              *
*    - Egoboo Characters                                                       *
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


/*******************************************************************************
* INCLUDES								                                       *
*******************************************************************************/

#include <stdio.h>      /* sscanf() */
#include <string.h>
#include <ctype.h>      // tolower()

#include "sdlglcfg.h"   /* Read egoboo text files eg. passage, spawn    */

#include "egodefs.h"    /* MPDFX_IMPASS                                 */
#include "egofile.h"    /* Get the work-directoires                     */
#include "idsz.h";
#include "misc.h"       /* Random treasure objects                      */
#include "msg.h"        /* Handle sending game messages                 */

// Own header
#include "char.h"

/*******************************************************************************
* DEFINES								                                       *
*******************************************************************************/

#define MAXCAPNAMESIZE     32   ///< Character class names

//Levels
#define MAXBASELEVEL        6   ///< Basic Levels 0-5
#define MAXLEVEL           20   ///< Absolute max level

#define GRIP_VERTS          4

#define CAP_INFINITE_WEIGHT  0xFF
#define CAP_MAX_WEIGHT       (CAP_INFINITE_WEIGHT - 1)

#define CHAR_MAX_CAP    180
#define CHAR_MAX_SKIN     4
// Maximum of characters that can be loaded, for each an AI_STATE may be needed
#define CHAR_MAX        500
#define CHAR_MAX_WEIGHT 127
#define CHAR_MAX_XP     9999

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

// Damage Flags
#define DAMAGE_INVERT   0x01
#define DAMAGE_CHARGE   0x02
#define DAMAGE_MANA     0x04
#define DAMAGE_INVICTUS 0x08

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
    unsigned int foes;              // Our foes as flags, friends == !foes
    
} TEAM_T;

//--------------------------------------------------------------------------------------------

/// The character profile data, or cap (Type of Character)
/// The internal representation of the information in data.txt
typedef struct
{
    int  id;
    char cap_name[MAXCAPNAMESIZE];             /// Name of this character profile (directory)
    // naming
    char class_name[MAXCAPNAMESIZE];           ///< Class name
    // For graphics
    int            mdl_no;          /// < Display model for this character profile
    // skins
    char           skinname[CHAR_MAX_SKIN][MAXCAPNAMESIZE]; ///< Skin name
    int            skincost[CHAR_MAX_SKIN];                 ///< Store prices
    int            maxaccel[CHAR_MAX_SKIN];                 ///< Acceleration for each skin
    char           skindressy;                              ///< Bits to tell whether the skins are "dressy"

    // overrides
    int           skin_override;                  ///< -1 or 0-3.. For import
    unsigned char level_override;                 ///< 0 for normal
    int           state_override;                 ///< 0 for normal
    int           content_override;               ///< 0 for normal

    IDSZ_T        idsz[IDSZ_COUNT];               ///< ID strings

    // inventory
    unsigned char ammo_max;                        ///< Ammo stuff
    unsigned char ammo;
    short int     money;                          ///< Money

    // character stats
    unsigned char gender;                        ///< Gender

    // life
    CAP_STAT_T   life_stat;                     ///< Life statistics
    unsigned int life_return;                    ///< Life regeneration
    float        life_heal;
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
    int   weight;                    ///< Weight
    float dampen;                     ///< Bounciness
    float bumpdampen;                 ///< Mass

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

    // status graphics
    unsigned char life_color;           ///< Life bar color
    unsigned char mana_color;           ///< Mana bar color                       

    // model graphics
    unsigned char flashand;             ///< Flashing rate
    unsigned char alpha;                ///< Transparency
    unsigned char light;                ///< Light blending    
    unsigned char sheen;                ///< How shiny it is ( 0-15 )   
    int  uoffvel;                       ///< "horizontal" texture movement rate
    int  voffvel;                       ///< "vertical" texture movement rate
    char uniformlit;                    ///< Bad lighting?
    char reflect;                       ///< Draw the reflection
    char alwaysdraw;                    ///< Always render
    char forceshadow;                   ///< Draw a shadow?
    char ripple;                        ///< Spawn ripples? unsigned

    // attack blocking info
    int iframefacing;                  ///< Invincibility frame
    int iframeangle;
    int nframefacing;                  ///< Normal frame
    int nframeangle;

    // defense
    char          resistbumpspawn;                          ///< Don't catch fire
    unsigned char defense[CHAR_MAX_SKIN];                   ///< Defense for each skin
    unsigned char dmg_resist[DAMAGE_COUNT][CHAR_MAX_SKIN];
    unsigned char dmg_modify[DAMAGE_COUNT][CHAR_MAX_SKIN];

    // xp
    int   exp_forlevel[MAXLEVEL];    ///< Experience needed for next level
    int   experience[2];                    ///< Starting experience
    unsigned short exp_worth;        ///< Amount given to killer/user
    float exp_exchange;              ///< Adds to worth
    float exp_rate[XP_COUNT];

    // sound
    char  sound_index[SOUND_COUNT];       ///< a map for soundX.wav to sound types

    // Booleans as flags packed into one integer    
    int fprops;                        ///< Character-Profile properties as flags
    
    unsigned char damagetargettype;      ///< For AI DamageTarget   
    char kursechance;                    ///< Chance of being kursed
    char hidestate;                      ///< Don't draw when...                       
    int  spelleffect_type;               ///< is the object that a spellbook generates

    // item usage    
    unsigned char weaponaction;                   ///< Animation needed to swing
    float         manacost;                       ///< How much mana to use this object?
    char          attack_attached;                ///< Do we have attack particles?
    char          attack_pip;                     ///< What kind of attack particles?

    float str_bonus;                      ///< Strength     damage factor
    float wis_bonus;                      ///< Wisdom       damage factor
    float int_bonus;                      ///< Intelligence damage factor
    float dex_bonus;                      ///< dexterity    damage factor

    // special particle effects
    char attachedprt_amount;                  ///< Number of sticky particles
    char attachedprt_reaffirmdamagetype;      ///< Re-attach sticky particles? Relight that torch...
    int       attachedprt_pip;                ///< Which kind of sticky particle
    char      gopoofprt_amount;               ///< Amount of poof particles
    int       gopoofprt_facingadd;            ///< Angular spread of poof particles
    int       gopoofprt_pip;                  ///< Which poof particle                
    int       blud_pip;                       ///< What kind of blud?

    // skill system
    IDSZ_T      skills[MAX_IDSZ_MAP_SIZE];
    int         see_invisible_level;             ///< Can it see invisible?
    char        item_type;                     ///< New value to check for scripts     
    // particles for this profile, loaded in consecutive order
    int prt_first_no;           // Number of first particle
    int prt_cnt;                // Total number of particles
    int sound_first_no;         // Number of first local sound
    int sound_cnt;              // Number of local sounds
    int script_no;              // Number of script belonging to this one
    // Unused data
    char life_add, mana_add;
    
} CAP_T;


/*******************************************************************************
* DATA									                                   *
*******************************************************************************/

// Load-Buffer for 'raw' data which is converted into 'packed' data
static char BoolVal[33];        // Boolean values
static char Defenses[20][20];   // Differences to translate to values
static char IdszStrings[8][10];
static char ExpIdsz[15][18];
static char DamageType[4]; 
static char WeaponAction[20];

// The different lists
static TEAM_T TeamList[TEAM_MAX + 2];     
static CAP_T  CapList[CHAR_MAX_CAP + 2];
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
    { SDLGLCFG_VAL_BOOLEAN, &BoolVal[CHAR_CFTRANSFERBLEND] },
    { SDLGLCFG_VAL_INT, &CapList[0].sheen },
    { SDLGLCFG_VAL_BOOLEAN, &BoolVal[CHAR_CFENVIRO] },
    { SDLGLCFG_VAL_FLOAT, &CapList[0].uoffvel },   // FLOAT_TO_FFFF( fTmp ); ??
    { SDLGLCFG_VAL_FLOAT, &CapList[0].voffvel },   // FLOAT_TO_FFFF( fTmp ); ??
    { SDLGLCFG_VAL_BOOLEAN, &BoolVal[CHAR_CFSTICKYBUTT] },
    { SDLGLCFG_VAL_BOOLEAN, &BoolVal[CHAR_CFINVICTUS] },
    { SDLGLCFG_VAL_INT, &CapList[0].nframefacing },
    { SDLGLCFG_VAL_INT, &CapList[0].nframeangle },
    { SDLGLCFG_VAL_INT, &CapList[0].iframefacing },
    { SDLGLCFG_VAL_INT, &CapList[0].iframeangle },
    // Skin defenses ( 4 skins )
    { SDLGLCFG_VAL_STRING, Defenses[0], 20 },   // Base defense rating of skin
    // Defense shifts for different damage types
    { SDLGLCFG_VAL_STRING, Defenses[1], 20 },
    { SDLGLCFG_VAL_STRING, Defenses[2], 20 },
    { SDLGLCFG_VAL_STRING, Defenses[3], 20 },
    { SDLGLCFG_VAL_STRING, Defenses[4], 20 },
    { SDLGLCFG_VAL_STRING, Defenses[5], 20 },
    { SDLGLCFG_VAL_STRING, Defenses[6], 20 },
    { SDLGLCFG_VAL_STRING, Defenses[7], 20 },
    { SDLGLCFG_VAL_STRING, Defenses[8], 20 },
    // Inversion flag chars
    { SDLGLCFG_VAL_STRING, Defenses[9], 20 },
    { SDLGLCFG_VAL_STRING, Defenses[10], 20 },
    { SDLGLCFG_VAL_STRING, Defenses[11], 20 },
    { SDLGLCFG_VAL_STRING, Defenses[12], 20 },
    { SDLGLCFG_VAL_STRING, Defenses[13], 20 },
    { SDLGLCFG_VAL_STRING, Defenses[14], 20 },
    { SDLGLCFG_VAL_STRING, Defenses[15], 20 },
    { SDLGLCFG_VAL_STRING, Defenses[16], 20 },
    // Acceleration rate
    { SDLGLCFG_VAL_STRING, Defenses[17], 20 },
    // Experience per level
    { SDLGLCFG_VAL_INT, &CapList[0].exp_forlevel[1] },
    { SDLGLCFG_VAL_INT, &CapList[0].exp_forlevel[2] },
    { SDLGLCFG_VAL_INT, &CapList[0].exp_forlevel[3] },
    { SDLGLCFG_VAL_INT, &CapList[0].exp_forlevel[4] },
    { SDLGLCFG_VAL_INT, &CapList[0].exp_forlevel[5] },
    // Starting experience
    { SDLGLCFG_VAL_IPAIR, &CapList[0].experience[0] },
    { SDLGLCFG_VAL_INT, &CapList[0].exp_worth },
    { SDLGLCFG_VAL_FLOAT, &CapList[0].exp_exchange },
    // Experience rate
    { SDLGLCFG_VAL_FLOAT, &CapList[0].exp_rate[0] },
    { SDLGLCFG_VAL_FLOAT, &CapList[1].exp_rate[0] },
    { SDLGLCFG_VAL_FLOAT, &CapList[2].exp_rate[0] },
    { SDLGLCFG_VAL_FLOAT, &CapList[3].exp_rate[0] },
    { SDLGLCFG_VAL_FLOAT, &CapList[4].exp_rate[0] },
    { SDLGLCFG_VAL_FLOAT, &CapList[5].exp_rate[0] },
    { SDLGLCFG_VAL_FLOAT, &CapList[6].exp_rate[0] },
    { SDLGLCFG_VAL_FLOAT, &CapList[7].exp_rate[0] },
    // IDSZ Identification tags ( [NONE] is valid )    
    { SDLGLCFG_VAL_STRING, IdszStrings[0], 10 },    // Parent ID
    { SDLGLCFG_VAL_STRING, IdszStrings[1], 10 },    // Type ID
    { SDLGLCFG_VAL_STRING, IdszStrings[2], 10 },    // Skill ID
    { SDLGLCFG_VAL_STRING, IdszStrings[3], 10 },    // Special ID
    { SDLGLCFG_VAL_STRING, IdszStrings[4], 10 },    // Hate group ID
    { SDLGLCFG_VAL_STRING, IdszStrings[5], 10 },    // Vuilnerability ID
    // Item and damage flags
    { SDLGLCFG_VAL_BOOLEAN, &BoolVal[CHAR_CFITEM] },    // pcap->isitem
    { SDLGLCFG_VAL_BOOLEAN, &BoolVal[CHAR_CFMOUNT] },   // pcap->ismount
    { SDLGLCFG_VAL_BOOLEAN, &BoolVal[CHAR_CFSTACKABLE] },   // pcap->isstackable
    { SDLGLCFG_VAL_BOOLEAN, &BoolVal[CHAR_CFNAMEKNOWN] },   // pcap->nameknown
    { SDLGLCFG_VAL_BOOLEAN, &BoolVal[CHAR_CFUSAGEKNOWN] },   // pcap->usageknown
    { SDLGLCFG_VAL_BOOLEAN, &BoolVal[CHAR_CFCANCARRYTONEXTMODULE] },   // pcap->cancarrytonextmodule
    { SDLGLCFG_VAL_BOOLEAN, &BoolVal[CHAR_CFNEEDSKILLIDTOUSE] },   // pcap->needskillidtouse
    { SDLGLCFG_VAL_BOOLEAN, &BoolVal[CHAR_CFPLATFORM] },   // pcap->platform
    { SDLGLCFG_VAL_BOOLEAN, &BoolVal[CHAR_CFCANGRABMONEY] },   // pcap->cangrabmoney
    { SDLGLCFG_VAL_BOOLEAN, &BoolVal[CHAR_CFCANOPENSTUFF] },   // pcap->canopenstuff
    // Other item and damage stuff
    // pcap->damagetargettype
    { SDLGLCFG_VAL_ONECHAR, &DamageType[0] },  // fget_next_damage_type( fileread );
    // pcap->weaponaction     = action_which( fget_next_char( fileread ) );    
    { SDLGLCFG_VAL_STRING, WeaponAction, 20 },
    // Particle attachments
    { SDLGLCFG_VAL_INT, &CapList[0].attachedprt_amount },
    // pcap->attachedprt_reaffirmdamagetype     
    { SDLGLCFG_VAL_ONECHAR, &DamageType[1] },  // fget_next_damage_type( fileread );
    { SDLGLCFG_VAL_INT, &CapList[0].attachedprt_pip },
    // Character hands
    { SDLGLCFG_VAL_INT, &CapList[0].attachedprt_pip },
    { SDLGLCFG_VAL_BOOLEAN, &BoolVal[CHAR_CFLEFTGRIPVALID] },
    { SDLGLCFG_VAL_BOOLEAN, &BoolVal[CHAR_CFRIGHTGRIPVALID] },
    // Attack order ( weapon )
    { SDLGLCFG_VAL_BOOLEAN, &CapList[0].attack_attached },
    { SDLGLCFG_VAL_CHAR, &CapList[0].attack_pip },
    // GoPoof
    { SDLGLCFG_VAL_CHAR, &CapList[0].gopoofprt_amount },
    { SDLGLCFG_VAL_INT, &CapList[0].gopoofprt_facingadd },
    { SDLGLCFG_VAL_INT, &CapList[0].gopoofprt_pip },
    // Blud 
    { SDLGLCFG_VAL_BOOLEAN, &BoolVal[CHAR_CFBLUDVALID] },
    { SDLGLCFG_VAL_INT, &CapList[0].blud_pip },
    // Stuff I forgot
    { SDLGLCFG_VAL_BOOLEAN, &BoolVal[CHAR_CFWATERWALK] },   // pcap->waterwalk
    { SDLGLCFG_VAL_FLOAT, &CapList[0].dampen },             // pcap->dampen  
    // More stuff I forgot
    { SDLGLCFG_VAL_FLOAT, &CapList[0].life_heal },          // pcap->life_heal
    { SDLGLCFG_VAL_FLOAT, &CapList[0].manacost },           // pcap->manacost
    { SDLGLCFG_VAL_INT, &CapList[0].life_return },          // pcap->life_return
    { SDLGLCFG_VAL_CHAR, &CapList[0].stoppedby },           // pcap->stoppedby
    // Skin names
    { SDLGLCFG_VAL_STRING, CapList[0].skinname[0], MAXCAPNAMESIZE },
    { SDLGLCFG_VAL_STRING, CapList[0].skinname[1], MAXCAPNAMESIZE },
    { SDLGLCFG_VAL_STRING, CapList[0].skinname[2], MAXCAPNAMESIZE },
    { SDLGLCFG_VAL_STRING, CapList[0].skinname[3], MAXCAPNAMESIZE },
    // Skin costs
    { SDLGLCFG_VAL_INT, &CapList[0].skincost[0] },
    { SDLGLCFG_VAL_INT, &CapList[0].skincost[1] },
    { SDLGLCFG_VAL_INT, &CapList[0].skincost[2] },
    { SDLGLCFG_VAL_INT, &CapList[0].skincost[3] },
    /// \note ZF@> Deprecated, but keep here for backwards compatability
    { SDLGLCFG_VAL_FLOAT, &CapList[0].str_bonus },          // pcap->str_bonus
    // Another memory lapse
    { SDLGLCFG_VAL_BOOLEAN, &BoolVal[CHAR_CFRIDERCANNOTATTACK] },    // !pcap->ridercanattack
    { SDLGLCFG_VAL_BOOLEAN, &BoolVal[CHAR_CFCANBEDAZED] },          // pcap->canbedazed
    { SDLGLCFG_VAL_BOOLEAN, &BoolVal[CHAR_CFCANBEGROGGED] },        // pcap->canbegrogged
       
    // Two unused slots
    { SDLGLCFG_VAL_CHAR, &CapList[0].life_add },    // !!!BAD!!! Life add
    { SDLGLCFG_VAL_CHAR, &CapList[0].mana_add },    // !!!BAD!!! Mana add
    //
    { SDLGLCFG_VAL_BOOLEAN, &CapList[0].see_invisible_level },  // 0 or 1
    { SDLGLCFG_VAL_CHAR, &CapList[0].kursechance },     // pcap->kursechance
    // Sounds
    { SDLGLCFG_VAL_CHAR, &CapList[0].sound_index[SOUND_FOOTFALL] }, // Footfall sound
    { SDLGLCFG_VAL_CHAR, &CapList[0].sound_index[SOUND_JUMP] },     // Jump sound
    // Expansions
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
        xpneeded = pcap->exp_forlevel[MAXBASELEVEL - 1];
        xpneeded += ( level * level * level * 15 );
        xpneeded -= (( MAXBASELEVEL - 1 ) * ( MAXBASELEVEL - 1 ) * ( MAXBASELEVEL - 1 ) * 15 );
        pcap->exp_forlevel[level] = xpneeded;
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
            // Set number of character profile ==> Number of script and number of model
            pcap->id     = i;
            pcap->mdl_no = i;
            // clear out the sounds
            for ( cnt = 0; cnt < SOUND_COUNT; cnt++ )
            {
                pcap->sound_index[cnt] = -1;
            }

            // Clear expansions...
            pcap->reflect = 1;
            CHAR_BIT_CLEAR(pcap->fprops, CHAR_CFVALUABLE);

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
 *     charCompleteCap
 * Description:
 *     Completes given 'pcap' by  data read in 'raw' from file 
 * Input:
 *     pcap *: Pointer on pcap-data to complete from 'raw' data
 */
void charCompleteCap(CAP_T *pcap)
{
    char * ptr, idsz_str[20];
    IDSZ_T loc_idsz;
    int i, dmg_type;
    int ivalue[5];
    char cvalue[5];
    
    
    // Resist burning and stuck arrows with nframe angle of 1 or more
    if(pcap->nframeangle > 0)
    {
        if (pcap->nframeangle == 1)
        {
            pcap->nframeangle = 0;
        }
    }
    
    // ** 'Translate' raw to profile data **
    // Change booleans to flags
    for (i = 0; i < 32; i++)
    {
        if(BoolVal[i] != 0)
        {
            pcap->fprops |= (int)(1 << i);
        }
    }

    // Skin defenses ( 4 skins )
    sscanf(Defenses[0], "%d%d%d%d", &ivalue[0], &ivalue[1], &ivalue[2], &ivalue[3]);
    for(i = 0; i < CHAR_MAX_SKIN; i++)
    {
        pcap->defense[i] = (unsigned char)ivalue[i]; // CLIP( iTmp, 0, 0xFF );
    }

    for(dmg_type = 0; dmg_type < DAMAGE_COUNT; dmg_type++)
    {
        sscanf(Defenses[dmg_type + 1], "%d%d%d%d", &ivalue[0], &ivalue[1], &ivalue[2], &ivalue[3]);
        for(i = 0; i < CHAR_MAX_SKIN; i++)
        {
            pcap->dmg_modify[dmg_type][i] = (unsigned char)ivalue[i];
        }
    }
    
    for(dmg_type = 0; dmg_type < DAMAGE_COUNT; dmg_type++)
    {
        sscanf(Defenses[dmg_type + 9], "%c%c%c%c", &cvalue[0], &cvalue[1], &cvalue[2], &cvalue[3]);

        for(i = 0; i < CHAR_MAX_SKIN; i++ )
        {
            cvalue[i] = (char)toupper(cvalue[i]);
            switch(cvalue[i])
            {
                case 'T': pcap->dmg_modify[dmg_type][i] |= DAMAGE_INVERT;   break;
                case 'C': pcap->dmg_modify[dmg_type][i] |= DAMAGE_CHARGE;   break;
                case 'M': pcap->dmg_modify[dmg_type][i] |= DAMAGE_MANA;     break;
                case 'I': pcap->dmg_modify[dmg_type][i] |= DAMAGE_INVICTUS; break;
                    // F is nothing
                default: break;
            }
        }
    }
    
    // Acceleration rate
    sscanf(Defenses[17], "%d%d%d%d", &ivalue[0], &ivalue[1], &ivalue[2], &ivalue[3]);
    for(i = 0; i < CHAR_MAX_SKIN; i++)
    {
        pcap->maxaccel[i] = ivalue[i]; 
    }
    
    // Experience and level data
    pcap->exp_forlevel[0] = 0;
    pcap->experience[0] /= 256;
    pcap->experience[1] /= 256;
    
    // IDSZ tags
    for(i = 0; i < 6; i++)
    {
        idszStringtoIDSZ(IdszStrings[i], &pcap->idsz[i], NULL);
    }    
    // Other value adjustments
    pcap->life_heal *= 256;
    pcap->manacost  *= 256;
    
    // assume the normal dependence of ripple on isitem
    if(CHAR_BIT_ISSET(pcap->fprops, CHAR_CFITEM))
    {
        CHAR_BIT_CLEAR(pcap->fprops, CHAR_CFRIPPLE);    // pcap->ripple = !pcap->isitem;
    } 
    
    // assume a round object
    pcap->bump_sizebig = pcap->bump_size * 1.414;
    
    if(CHAR_BIT_ISSET(pcap->fprops, CHAR_CFUSAGEKNOWN))
    {
         // assume the normal icon usage
        CHAR_BIT_SET(pcap->fprops, CHAR_CFDRAWICON);    // pcap->draw_icon = pcap->usageknown;
    }
    
    if(! CHAR_BIT_ISSET(pcap->fprops, CHAR_CFPLATFORM))
    {   
        // assume normal platform usage
        CHAR_BIT_SET(pcap->fprops, CHAR_CFCANUSEPLATFORMS); // pcap->canuseplatforms = !pcap->platform;
    }
    
    // Read expansions
    for(i = 0; i < 13; i++)
    {
        idszStringtoIDSZ(ExpIdsz[i], &loc_idsz, idsz_str);
        // Now react, epending on type of IDSZ
        switch(loc_idsz.idsz)
        {
            case 'DRES':
                CHAR_BIT_SET(pcap->skindressy, loc_idsz.value);                
                break;
            case 'GOLD':
                pcap->money = loc_idsz.value;
                break;
            case 'STUK':
                // pcap->resistbumpspawn = ( 0 != ( 1 - fget_int( fileread ) ) );
                break;
            case 'PACK':
                // pcap->istoobig = !( 0 != fget_int( fileread ) );
                break;
            case 'VAMP':
                // pcap->reflect = !( 0 != fget_int( fileread ) );
                break;
            case 'DRAW':
                // pcap->alwaysdraw = ( 0 != fget_int( fileread ) );
                break;
            case 'RANG':
                // pcap->isranged = ( 0 != fget_int( fileread ) );
                break;
            case 'HIDE':
                // pcap->hidestate = fget_int( fileread );
                break;
            case 'EQUI':
                // pcap->isequipment = ( 0 != fget_int( fileread ) );
                break;
            case 'SQUA':
                // pcap->bump_sizebig = pcap->bump_size * 2;
                break;
            case 'ICON':
                // pcap->draw_icon = ( 0 != fget_int( fileread ) );
                break;
            case 'SHAD':
                // pcap->forceshadow = ( 0 != fget_int( fileread ) );
                break;
            case 'SKIN':
                // pcap->skin_override = fget_int( fileread ) & 3;
                break;
            case 'CONT':
                // pcap->content_override = fget_int( fileread );
                break;
            case 'STAT':
                // pcap->state_override = fget_int( fileread );
                break;
            case 'LEVL':
                // pcap->level_override = fget_int( fileread );
                break;
            case 'PLAT':
                // pcap->canuseplatforms = ( 0 != fget_int( fileread ) );
                break;
            case 'RIPP':
                // pcap->ripple = ( 0 != fget_int( fileread ) );
                break;
            case 'VALU':
                // pcap->isvaluable = fget_int( fileread );
                break;
            case 'LIFE':
                // pcap->life_spawn = 256.0f * fget_float( fileread );
                break;
            case 'MANA':
                // pcap->mana_spawn = 256.0f * fget_float( fileread );
                break;
            case 'BOOK':
                // pcap->spelleffect_type = fget_int( fileread ) % MAX_SKIN;
                break;
            case 'FAST':
                // pcap->attack_fast = ( 0 != fget_int( fileread ) );
                break;
            // Damage bonuses from stats
            case 'STRD':
                // pcap->str_bonus = fget_float( fileread );
                break;
            case 'INTD':
                // pcap->int_bonus = fget_float( fileread );
                break;
            case 'WISD':
                // pcap->wis_bonus = fget_float( fileread );
                break;
            case 'DEXD':
                // pcap->dex_bonus = fget_float( fileread );
                break;
            case 'MODL':
                ptr = strpbrk(idsz_str, "SBH");
                {
                    while(NULL != ptr)
                    {
                        if('S' == *ptr)
                        {
                            // pcap->bump_override_size = 1;
                        }
                        else if('B' == *ptr)
                        {
                            // pcap->bump_override_sizebig = 1;
                        }
                        else if ('H' == *ptr)
                        {
                            // pcap->bump_override_height = 1;
                        }

                        ptr = strpbrk(idsz_str, "SBH" );
                    }
                }
                break;
                // If it is none of the predefined IDSZ extensions then add it as a new skill
            default:
                idszMapAdd(pcap->skills, MAX_IDSZ_MAP_SIZE, &loc_idsz);
                break;
        }
    }
     // @todo: Set itemtype
    // item_type = ;
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
    char *fdir;
    char fname[512];
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

        /* @todo: // Load random treasure object
        if('%' == objname)
        {
        }        
        */
        // Get the directory of this object        
        fdir = egofileMakeFileName(EGOFILE_OBJECTDIR, objname);
        
        // Read character profile        
        if(fdir[0] != 0)
        {
            // If object was found
            // Save name to prevent multiple loading of same profile and its data
            strcpy(pcap->cap_name, objname);  
            // Read the basic profile
            sprintf(fname, "%sdata.txt", fdir); 
            // Do the 'raw' loading
            if(! sdlglcfgEgobooValues(fname, CapVal, 0))
            {
                // @todo: Mark 'pcap' as unused
                charCompleteCap(pcap);
                return 0;
            }
           

            /* @todo: Load the real data
            // @todo: Function for loading the naming data
            // sprintf(fname, "%snaming.txt", fdir);            
            // @todo: Load its particles
            // "part0.txt" - "part9.txt"
            // pcap->prt_first_no = particleLoad(fdir, &pcap->prt_cnt)
            // sdlglmd2Load(fdir, cno); ==> egomodelLoad(fdir, cno);
            // sprintf(fname, "%stris.md2", fdir);
            // "icon0,bmp" - "icon4.bmp"
            // "tris0.bmp" - "tris4.bmp"
            // Load sound
            // "sound0.wav" - "sound9.wav"
            // @todo: Load its scripts including messages
            // pcap->script_no = scriptLoad(fdir); // Each character profile has its script and its messages
            // sprintf(fname, "%smessage.txt", fdir);
            // @todo: pscript->first_msg_no = msgObjectLoad(char *fname);
            // sprintf(fname, "%sscript.txt", fdir);
            */
           charSetupXPTable(pcap);
        }
        else
        {
            msgSend(MSG_REC_LOG, MSG_GAME_OBJNOTFOUND, 0, objname);
        }
        
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
 *     char_no: Number of character to do level up for
 */
static void charDoLevelUp(const int char_no)
{
    CHAR_T *pchar;
    CAP_T *pcap;
    char curlevel;
    int xpcurrent, xpneeded;
    int valpair[2];


    pchar = &CharList[char_no];
    pcap  = &CapList[pchar->cap_no];

    if (0 == pchar->cap_no) return;

    // Do level ups and stat changes
    curlevel = (char)(pchar->experience_level + 1);

    if (curlevel < MAXLEVEL)
    {
        xpcurrent = pchar->experience;
        xpneeded  = pcap->exp_forlevel[curlevel];

        if ( xpcurrent >= xpneeded )
        {
            // do the level up
            pchar->experience_level++;
            // The character is ready to advance...
            msgSend(0, char_no, MSG_LEVELUP, NULL);

            // Size
            valpair[0] = (10.0 * pcap->size_perlevel);
            charAddValue(char_no, CHAR_VAL_SIZE, 0, valpair, 0);
            // Strength
            charAddValue(char_no, CHAR_VAL_STR, 0, pcap->strength_stat.perlevel, 0);
            // Wisdom
            charAddValue(char_no, CHAR_VAL_WIS, 0, pcap->wisdom_stat.perlevel, 0);
            // Intelligence
            charAddValue(char_no, CHAR_VAL_INTEL, 0, pcap->intelligence_stat.perlevel, 0);
            // Dexterity
            charAddValue(char_no, CHAR_VAL_DEX, 0, pcap->dexterity_stat.perlevel, 0);
            // Life
            charAddValue(char_no, CHAR_VAL_LIFE, 0, pcap->life_stat.perlevel, 0);
            // Mana
            charAddValue(char_no, CHAR_VAL_MANA, 0, pcap->mana_stat.perlevel, 0);
            // Mana Return
            charAddValue(char_no, CHAR_VAL_MANARET, 0, pcap->mana_return_stat.perlevel, 0);
            // Mana Flow
            charAddValue(char_no, CHAR_VAL_MANAFLOW, 0, pcap->mana_flow_stat.perlevel, 0);            
        }
    }
}

/*
 * Name:
 *     charGiveXPOne
 * Description:
 *     Gives experence to the given character.
 * Input:
 *     pchar *: Pointer on character to give experience
 *     amount:  Amount of experience to give
 *     xp_type: Type of experience to give          
 */
static void charGiveXPOne(CHAR_T *pchar, int amount, int xp_type)
{
    CAP_T *pcap;
    int newamount;
    int intadd, wisadd;


    if (0 == amount) return;    //No xp to give

    newamount = amount;

    if (xp_type < XP_COUNT )
    {
        pcap  = &CapList[pchar->cap_no];
        // Multiplier based on character profile
        newamount = amount * pcap->exp_rate[xp_type];
    }

    // Figure out how much experience to give
    intadd = pchar->itl[CHARSTAT_ACT];
    wisadd = pchar->wis[CHARSTAT_ACT];

    // Intelligence and slightly wisdom increases xp gained (0,5% per int and 0,25% per wisdom above 10)
    intadd = (intadd > 10) ? (newamount * (intadd - 10) / 200) : 0;
    wisadd = (wisadd > 10) ? (newamount * (wisadd - 10) / 400) : 0;

    newamount += (intadd + wisadd);

    // @todo:Apply XP bonus/penality depending on game difficulty
    /*
    if ( cfg.difficulty >= GAME_HARD ) newamount += newamount / 5;          // 20% extra on hard
    else if ( cfg.difficulty >= GAME_NORMAL ) newamount += newamount / 10;  // 10% extra on normal
    */
    pchar->experience += newamount;
    // @todo: Level up, if enough experience
    charDoLevelUp(pchar->id);
    // @todo: Send message if levelled up
}

/*
 * Name:
 *     charGiveXPTeam
 * Description:
 *      Gives experence to the given characters team. Including given character. 
 * Input:
 *     char_no: Number of character to give experience
 *     amount:  Amount of experience to give
 *     xp_type: Type of experience to give 
 */
void charGiveXPTeam(int char_no, int amount, int xp_type)
{
    CHAR_T *pchar;
    int team_no;
    
    
    if (0 == amount) return;    //No xp to give
    
    pchar = &CharList[char_no];
    
    // Give experience to whole team
    team_no = pchar->team[0];
    // @todo: Use 'teamlist' for faster calculation
    pchar = &CharList[1];
    while(pchar->cap_no != 0)
    {
        if(pchar->team[0] == team_no)
        {
            charGiveXPOne(pchar, amount, xp_type);
        }
        // Next character
        pchar++;
    }
}

/*
 * Name:
 *     charCleanUp
 * Description:
 *     Everything necessary to disconnect one character from the game
 * Input:
 *     pchar *: Pointer on character to cleanup
 */
static void charCleanUp(CHAR_T * pchar)
{  
    int act_team;
    

    // Remove it from the team
    act_team = pchar->team[CHARSTAT_ACT];
    if (TeamList[act_team].morale > 0 ) TeamList[act_team].morale--;

    if (TeamList[act_team].leader_no == pchar->id)
    {
        // The team now has no leader if the character is the leader
        TeamList[act_team].leader_no = 0;
    }

    // Clear all shop passages that it owned...
    // @todo: Do this in 'egomap'-Code
    
    // detach from any mount
    /*
    if ( INGAME_CHR( pchar->attachedto ) )
    {
        detach_character_from_mount( ichr, char, 0 );
    }
    */

    // drop your left item
    /*
    itmp = pchar->holdingwhich[SLOT_LEFT];
    if ( INGAME_CHR( itmp ) && ChrList.lst[itmp].isitem )
    {
        detach_character_from_mount( itmp, char, 0 );
    }

    // drop your right item
    itmp = pchar->holdingwhich[SLOT_RIGHT];
    if ( INGAME_CHR( itmp ) && ChrList.lst[itmp].isitem )
    {
        detach_character_from_mount( itmp, char, 0 );
    }
    */

    // remove enchants from the character
    if(pchar->life >= 0)
    {
        // remove all SPELLS
    }
    
    // @todo: Stop all sound loops for this object
    // looped_stop_object_sounds(pchar->id);
    memset(pchar, 0, sizeof(CHAR_T));
    // Sign it as invalid
    pchar->id     = -1;
    pchar->cap_no = -1;
}

/*
 * Name:
 *     charKill
 * Description:
 *     Handle a character death. Set various states, disconnect it from the world, etc.
 * Input:
 *     char_no:   Number of character to kill
 *     killer_no: Number of character which killed this one
 *     ignore_invictus:  
 */
static void charKill(const int char_no, const int killer_no, char ignore_invictus)
{
    CHAR_T *pchar, *pkiller;
    CAP_T *pcap;
    int action;
    int experience;
    int killer_team;


    pchar = &CharList[char_no];
    //No need to continue is there?
    if ( !pchar->life[CHARSTAT_ACT] <= 0
        || (CHAR_BIT_ISSET(pchar->cap_props, CHAR_CFINVICTUS) && !ignore_invictus ) ) return;

    pcap = &CapList[pchar->cap_no];      

    msgSend(killer_no, char_no, MSG_KILLED, NULL);
    CHAR_BIT_SET(pchar->cap_props, CHAR_CFPLATFORM);
    CHAR_BIT_SET(pchar->cap_props, CHAR_CFCANUSEPLATFORMS);

    pchar->life[CHARSTAT_ACT] = -1;
    pchar->bumpdampen         *= 0.5f;

    // @todo: Play the death animation
    // action = generate_randmask( ACTION_KA, 3 );
    // chr_play_action( pchar, action, 0 );
    // chr_instance_set_action_keep( &( pchar->inst ), char );

    // Give kill experience
    experience = pcap->exp_worth + (pchar->experience * pcap->exp_exchange);
    pkiller = &CharList[killer_no];

    killer_team = pkiller->team[CHARSTAT_ACT];
    // distribute experience to the attacker
    if (pkiller->cap_no > 0)
    {
        // Set target
        pchar->target_no = killer_no;
        if (killer_team == TEAM_DAMAGE || killer_team == 0 )  pchar->target_no = char_no;

        // Award experience for kill?
        if(pkiller->t_foes & (~pchar->t_foes))  // Team hates team
        {
            //Check for special hatred
            if (charGetSkill(killer_no, IDSZ_HATE ) == charGetSkill(char_no, IDSZ_PARENT ) ||
                 charGetSkill(killer_no, IDSZ_HATE ) == charGetSkill(char_no, IDSZ_TYPE ) )
            {
                charGiveXPOne(pkiller, experience, XP_KILLHATED);
            }

            // Nope, award direct kill experience instead
            else charGiveXPOne(pkiller, experience, XP_KILLENEMY);
        }
    }

    //Set various alerts to let others know it has died
    //and distribute experience to whoever needs it
    // @todo: Send a message
    msgSend(0, char_no, MSG_KILLED, NULL);

    /*
    CHR_BEGIN_LOOP_ACTIVE( tnc, plistener )
    {
        if ( !plistener->alive ) continue;

        // All allies get team experience, but only if they also hate the dead guy's team
        if ( tnc != actual_killer && !team_hates_team( plistener->team, killer_team ) && team_hates_team( plistener->team, pchar->team ) )
        {
            give_experience( tnc, experience, XP_TEAMKILL, 0 );
        }

        // Check if it was a leader
        if ( TeamList[pchar->team].leader == ichr && chr_get_iteam( tnc ) == pchar->team )
        {
            // All folks on the leaders team get the alert
            msgSend(0, tnc, MSG_LEADERKILLED);
        }

        // Let the other characters know it died
        if ( plistener->ai.target == ichr )
        {
            msgSend(0, tnc, MSG_TARGETKILLED);
        }
    }
    CHR_END_LOOP();
    */

    // @todo: If it's a player, let it die properly before enabling respawn
    // if ( VALID_PLA( pchar->is_which_player ) ) local_stats.revivetimer = ONESECOND; // 1 second
    msgSend(0, char_no, MSG_KILLED, NULL);

    // Let it's AI script run one last time
    // scriptRun(char_no, pchar->cap_no, MSG_KILLED);
    // pchar->ai.timer = update_wld + 1;            // Prevent IfTimeOut in scr_run_chr_script()
    // scr_run_chr_script( ichr );

    // Stop any looped sounds
    // looped_stop_object_sounds( ichr );
    // Detach the character from the game
    charCleanUp(pchar);
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
        TeamList[i].foes = ~t_flag;  // !friend == foe
    }
    
    // The neutral team hates nobody
    TeamList[('N' - 'A')].foes = 0;
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
 *     objname *: Pointer on name of object to create character from
 *     team:      Belongs to this team ('A' - 'Z')
 *     stt:       Statusbar should appear yes/no
 *     money;     Bonus money for the character for this module.
 *     skin:      Which skin to use
 *     psg:       Number of passage the character belongs to
 * Output:
 *     Number of character, if any is created
 */
int charCreate(char *objname, char team, char stt, int money, char skin, char psg)
{
    int i;
    int cap_no;
    int char_no;    
    CAP_T  *pcap;
    CHAR_T *pchar;
    

    // @todo: Support Random Treasure
    cap_no = charReadCap(objname);

    if(cap_no > 0)
    {
        char_no = charNewChar();

        if(char_no > 0)
        {
            pcap  = &CapList[cap_no];
            pchar = &CharList[char_no];

            // @todo: Complete creation of character profile           
            pchar->id      = char_no;         /* Number of slot > 0: Is occupied            */
            pchar->cap_no  = cap_no;          /* Has this character profile, this script_no */
            pchar->mdl_no  = pcap->mdl_no;    /* Number of model for display          */
            pchar->icon_no = skin;            /* Number of models icon for display    */
            pchar->skin_no = skin;            /* Number of models skin for display    */
            // Attached to passage
            pchar->psg_no  = psg;
            // Gender
            pchar->gender = pcap->gender;       // As char
            if(pcap->gender == 'R')
            {
                // @todo: Generate random gender
            }
            
            // Team stuff
            team = (char)(team - 'A');  // From 0 to ('Z' - 'A')
            pchar->team[0] = team;    // Actual team
            pchar->team[1] = team;    // Original team
            
            // Firstborn becomes the leader
            if (TeamList[team].leader_no == 0)
            {
                TeamList[team].leader_no = char_no;
            }

            pchar->money = money;   

            // Damage
            pchar->defense[0] = pchar->defense[1] = pcap->defense[pchar->skin_no];
            for(i = 0; i < DAMAGE_COUNT; i++ )
            {
                pchar->dmg_modify[i][1] = pcap->dmg_modify[i][pchar->skin_no];
                pchar->dmg_modify[i][0] = pchar->dmg_modify[i][1];
                pchar->dmg_resist[i][1] = pcap->dmg_resist[i][pchar->skin_no];
                pchar->dmg_modify[i][0] = pchar->dmg_resist[i][1];
            }

            // Flags
            pchar->cap_props = pcap->fprops;

            // calculate a base kurse state. this may be overridden later
            if (CHAR_BIT_ISSET(pchar->cap_props, CHAR_CFITEM))
            {
                if((miscRandVal(100) == 2))
                {
                    CHAR_BIT_SET(pchar->cap_props, CHAR_CFKURSED);
                }
            }
            
            // Enchant stuff
            pchar->see_invisible_level = (char)pcap->see_invisible_level;
            
            // Skillz
            /* @todo: Skill from map
            idsz_map_copy( pcap->skills, SDL_arraysize( pcap->skills ), pchar->skills );
            pchar->darkvision_level = charGetSkill( pchar, MAKE_IDSZ( 'D', 'A', 'R', 'K' ) );
            */
            
            // Ammo
            pchar->ammo[1] = pcap->ammo_max;
            pchar->ammo[0] = pcap->ammo;

            // Life and Mana
            pchar->life_color  = pcap->life_color;
            pchar->mana_color  = pcap->mana_color;
            pchar->life[0] = pchar->life[1] = (short int)miscRandRange(pcap->life_stat.val);
            pchar->mana[0] = pchar->mana[1] = (short int)miscRandRange(pcap->mana_stat.val);
            pchar->mana_flow   = (char)miscRandRange(pcap->mana_flow_stat.val);
            pchar->mana_return = (char)miscRandRange(pcap->mana_return_stat.val);
    
            // SWID
            pchar->str[0] = pchar->str[1] = (char)miscRandRange(pcap->strength_stat.val);
            pchar->wis[0] = pchar->wis[1] = (char)miscRandRange(pcap->wisdom_stat.val);
            pchar->itl[0] = pchar->itl[1] = (char)miscRandRange(pcap->intelligence_stat.val);
            pchar->dex[0] = pchar->dex[1] = (char)miscRandRange(pcap->dexterity_stat.val);
            
            // Skin
            // @todo: Get it from IDSZ, iv available
            if(pchar->skin_no >= CHAR_MAX_SKIN)
            {
                pchar->skin_no = (char)(miscRandVal(4)-1);
            }

            // Jumping
            pchar->jump_power  = pcap->jump;
            pchar->jump_number = pcap->jump_number;

            // Other junk
            pchar->fly_height  = pcap->fly_height;
            pchar->max_accel[0] = pchar->max_accel[1] = pcap->maxaccel[pchar->skin_no];
            pchar->alpha_base  = pcap->alpha;
            pchar->light_base  = pcap->light;
            pchar->flashand    = pcap->flashand;
            pchar->dampen      = pcap->dampen;
            pchar->bumpdampen  = pcap->bumpdampen;

            
            if(CAP_INFINITE_WEIGHT == pcap->weight)            
            {
                pchar->weight = CHAR_MAX_WEIGHT;
            }
            else
            {
                i = pcap->weight * pcap->size * pcap->size * pcap->size;
                pchar->weight = (i > CHAR_MAX_WEIGHT) ? CHAR_MAX_WEIGHT: i;
            }
            
            // Experience
            // MIN(miscRandRange(pcap->experience), MAXXP );
            pchar->experience       = miscRandRange(pcap->experience);
            pchar->experience_level = pcap->level_override;
            
            // Particle attachments
            // pchar->reaffirm_damagetype = pcap->attachedprt_reaffirm_damagetype;
            
            // IMPORTANT!!!
            pchar->missilehandler = char_no;
            
            if(! CHAR_BIT_ISSET(pchar->cap_props, CHAR_CFINVICTUS))
            {
                TeamList[team].morale++;
            }

            /*
            // Character size and bumping
            chr_init_size( pchar, pcap );
            */
            /*
            // Heal the spawn_ptr->skin, if needed
            // Get it from IDSZ, if available
            if(pchar->skin < 0)
            {
                // Get it from IDSZ, if available
            }
            */
            /*
            // Image rendering
            pchar->uoffvel = pcap->uoffvel;
            pchar->voffvel = pcap->voffvel;
            */
            
            // Particle attachments
            /*
            for ( tnc = 0; tnc < pcap->attachedprt_amount; tnc++ )
            {
                spawn_one_particle( pchar->pos.v, pchar->ori.facing_z, pchar->profile_ref, pcap->attachedprt_lpip,
                                    ichr, GRIP_LAST + tnc, pchar->team, ichr, INVALID_PRT_REF, tnc, INVALID_CHR_REF );
            }
            */
           // Shop thing is done as the character is dropped to the map

            if(stt)  ///< Display stats?
            {
                CHAR_BIT_SET(pchar->var_props, CHAR_FDRAWSTATS);
            }
             
            // sound stuff...  copy from the cap
            for(i = 0; i < SOUND_COUNT; i++ )
            {
                pchar->sound_index[i] = pcap->sound_index[i];
            }
            
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

/* ================= inventory functions ===================== */

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
        if(CHAR_BIT_SET(pitem->cap_props, CHAR_CFTOOBIG))
        {
            // SET_BIT( pitem->ai.alert, ALERTIF_NOTPUTAWAY );
            if(CHAR_BIT_SET(pchar->var_props, CHAR_FISLOCALPLAYER))
            {
                // @todo: Send message for display or to AI/Player
                msgSend(0, char_no, MSG_TOOBIG, NULL); // , chr_get_name()
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
                msgSend(0, char_no, MSG_ITEMFOUND, NULL); // , chr_get_name()
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
 *     destroy_it: Delete the character after removing it  
 * Output:
 *     > 0: Number of character removed from inventory
 */
int charInventoryRemove(const int char_no, int slot_no, char ignorekurse, char destroy_it)
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
            
            if (CHAR_BIT_SET(pitem->cap_props, CHAR_CFKURSED) && !ignorekurse )
            {
                msgSend(0, char_no, MSG_NOTTAKENOUT, NULL); // , chr_get_name()
                // Flag the last found_item as not removed
                /*
                // if ( pchar->islocalplayer ) DisplayMsg_printf( "%s won't go out!", chr_get_name( item, CHRNAME_ARTICLE | CHRNAME_DEFINITE | CHRNAME_CAPITAL, NULL, 0 ) );
                */
                return 0;
            }
            else
            {
                // Remove item from inventory
                pchar->inventory[slot_no].item_no = 0;
                pitem->attached_to  = 0;
                // Return number of item to caller for further handling if not deleted
                if(destroy_it)
                {   
                    // Destroy the item
                    memset(&CharList[item_no], 0, sizeof(CHAR_T));
                    CharList[item_no].id     = -1;
                    CharList[item_no].cap_no = -1;
                }
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

/* ================= Gameplay functions ================== */

/*
 * Name:
 *     charInventoryFunc
 * Description:
 *     Applies the given function on inventory item(s), using the given argument, if needed
 * Input:
 *     char_no:  Number of character to handle inventory for
 *     func_no:  Number of function to execute
 *     arg:      Additional argument for function, if needed
 */
void charInventoryFunc(int char_no, int func_no)
{
    int i, item_no;
    CHAR_T *pchar, *pitem;


    // @todo: Search inventories of the whole team (if 'player') 'use_team'
    pchar = &CharList[char_no];

    for(i = 0; i < (INVEN_COUNT + SLOT_COUNT); i++)
    {
        item_no = pchar->inventory[i].item_no;

        if(item_no > 0)
        {
            pitem = &CharList[item_no];

            switch(func_no)
            {
                case CHAR_INVFUNC_UNKURSE:
                    CHAR_BIT_CLEAR(pitem->cap_props, CHAR_CFKURSED);
                    break;
            }
        }
    }
}



/*
 * Name:
 *     charDamage
 *      replaces:
 *          damage_character, kill_character(use high valpair)          
 * Description:
 *     Applies the given function on character(s), using the given arguments
 *   This function calculates and applies damage to a character.  It also
 *   sets alerts and begins actions. Blocking and frame invincibility
 *   are done here too.  Direction is ATK_FRONT if the attack is coming head on,
 *   ATK_RIGHT if from the right, ATK_BEHIND if from the back, ATK_LEFT if from the
 *   left.
 * Input:
 *     char_no:    Number of character to handle 
 *     dir_no:     Attack direction, if any
 *     valpair[2]: Number, fixed points are in 'valpair[0]'
 *     team:       Damages the whole team ?!
 *     attacker:   If damage
 *     effects:    Flags about effects to spawn...
 */
void charDamage(const char char_no, char dir_no, int valpair[2], char team,
                int attacker, int effects)
{
    CHAR_T *pchar;
    // CAP_T *pcap;
    int amount;


    // Fixed value for experience and killing characters
    if(valpair[1] == 0)
    {
        amount = valpair[0];
    }
    else
    {
        amount = miscRandRange(valpair);
    }

    pchar = &CharList[char_no];
    // pcap  = &CapList[pchar->cap_no];

    pchar->life[CHARSTAT_ACT] -= (short int)amount;

    if(pchar->life[CHARSTAT_ACT] <= 0)
    {
        charKill(char_no, attacker, 1);
    }
}

/*
 * Name:
 *     charAddValue
 *      replaces:
 *          give_experience, give_team_experience, heal_character
 * Description:
 *     Changes the given value on character(s), using the given arguments
 *     This function calculates and applies changing of stats and so on to a character or a team.
 * Input:
 *     char_no:    Number of character to handle 
 *     which:      Which value to change 
 *     sub_type:   Of 'which', e.g. for damage resistance values, otherwise e.g. 'damage' for life
 *     valpair[2]: For damage healing, Experience points are in 'valpair[0]', negative is damage
 *     duration_sec: > 0: Is a temporary effect 
 */
void charAddValue(int char_no, int which, int sub_type, int valpair[2], int duration_sec)
{
    CHAR_T *pchar;
    int amount;


    pchar = &CharList[char_no];

    if(valpair[1] > 0)
    {
        // Random value
        amount = (char)miscRandRange(valpair);    
    }
    else if(valpair[0] > 0)
    {
        amount = (char)valpair[0];        
    }
    else
    {
        return; // No value to set at all
    }
    
    switch(which)
    {
        case CHAR_VAL_LIFE:     ///< Hit-Points
            if(duration_sec > 0)
            {
                // Temporary value
                if(charSetTimer(char_no, which, amount, duration_sec))
                {
                    pchar->life[0] += (short int)amount;
                    return;
                }
            }
            // Add permanent 
            amount = ((amount + pchar->life[1]) > PERFECTBIG) ? (PERFECTBIG - pchar->life[1]) : amount;
            pchar->life[0] += (short int)amount;
            pchar->life[1] += (short int)amount;
            break;
        case CHAR_VAL_MANA:     ///< Mana stuff 
            if(duration_sec > 0)
            {
                if(charSetTimer(char_no, which, amount, duration_sec))
                {
                    pchar->mana[0] += (short int)amount;
                    return;
                }
            }
            amount = ((amount + pchar->mana[1]) > PERFECTBIG) ? (PERFECTBIG - pchar->mana[1]) : amount;
            pchar->mana[0] += (short int)amount;
            pchar->mana[1] += (short int)amount;
            break;
        case CHAR_VAL_STR:      // strength   
            if(duration_sec > 0)
            {
                if(charSetTimer(char_no, which, amount, duration_sec))
                {
                    pchar->str[0] += (char)amount;
                    return;
                }
            }
            amount = ((amount + pchar->str[1]) > PERFECTSTAT) ? (PERFECTSTAT - pchar->str[1]) : amount;
            pchar->str[0] += (char)amount;
            pchar->str[1] += (char)amount;
            break;
        case CHAR_VAL_INTEL:    // intelligence
            if(duration_sec > 0)
            {
                if(charSetTimer(char_no, which, amount, duration_sec))
                {
                    pchar->itl[0] += (char)amount;
                    return;
                }
            }
            amount = ((amount + pchar->itl[1]) > PERFECTSTAT) ? (PERFECTSTAT - pchar->itl[1]) : amount;
            pchar->itl[0] += (char)amount;
            pchar->itl[1] += (char)amount;
            break;
        case CHAR_VAL_WIS:      // wisdom
            if(duration_sec > 0)
            {
                if(charSetTimer(char_no, which, amount, duration_sec))
                {
                    pchar->wis[0] += (char)amount;
                    return;
                }
            }
            amount = ((amount + pchar->wis[1]) > PERFECTSTAT) ? (PERFECTSTAT - pchar->wis[1]) : amount;
            pchar->wis[0] += (char)amount;
            pchar->wis[1] += (char)amount;
            break;
        case CHAR_VAL_DEX:      // dexterity
            if(duration_sec > 0)
            {
                if(charSetTimer(char_no, which, amount, duration_sec))
                {
                    pchar->dex[0] += (char)amount;
                    return;
                }
            }
            amount = ((amount + pchar->dex[1]) > PERFECTSTAT) ? (PERFECTSTAT - pchar->dex[1]) : amount;
            pchar->dex[0] += (char)amount;
            pchar->dex[1] += (char)amount;
            break;
        case CHAR_VAL_CON:
            if(duration_sec > 0)
            {
                if(charSetTimer(char_no, which, amount, duration_sec))
                {
                    pchar->con[0] += (char)amount;
                    return;
                }
            }
            amount = ((amount + pchar->con[1]) > PERFECTSTAT) ? (PERFECTSTAT - pchar->con[1]) : amount;
            pchar->con[0] += (char)amount;
            pchar->con[1] += (char)amount;
            break;
        case CHAR_VAL_CHA:
            if(duration_sec > 0)
            {
                if(charSetTimer(char_no, which, amount, duration_sec))
                {
                    pchar->cha[0] += (char)amount;
                    return;
                }
            }
            amount = ((amount + pchar->cha[1]) > PERFECTSTAT) ? (PERFECTSTAT - pchar->cha[1]) : amount;
            pchar->cha[0] += (char)amount;
            pchar->cha[1] += (char)amount;
            break;
        case CHAR_VAL_DEFENSE:
            if(duration_sec > 0)
            {
                if(charSetTimer(char_no, which, amount, duration_sec))
                {
                    pchar->defense[0] += (char)amount;
                    return;
                }
            }
            break;
        case CHAR_VAL_MANARET:
            amount += pchar->mana_return;
            if (amount > PERFECTSTAT) amount = PERFECTSTAT;
            pchar->mana_return += (char)amount;
            break;
        case CHAR_VAL_MANAFLOW:
            amount += pchar->mana_flow;
            if (amount > PERFECTSTAT) amount = PERFECTSTAT;
            pchar->mana_flow = (char)amount;
            break;
        case CHAR_VAL_DMGBOOST:
            if(duration_sec > 0)
            {
                if(charSetTimer(char_no, which, amount, duration_sec))
                {
                    pchar->dmg_boost[0] += (char)amount;
                    return;
                }
            }
            amount = ((amount + pchar->dmg_boost[1]) > PERFECTSTAT) ? (PERFECTSTAT - pchar->dmg_boost[1]) : amount;
            pchar->dmg_boost[0] += (char)amount;
            pchar->dmg_boost[1] += (char)amount;
            break;
        case CHAR_VAL_DMGTHRES: ///< Damage below this number is ignored
            if(duration_sec > 0)
            {
                if(charSetTimer(char_no, which, amount, duration_sec))
                {
                    pchar->dmg_threshold[0] += (char)amount;
                    return;
                }
            }
            amount = ((amount + pchar->dmg_threshold[1]) > PERFECTSTAT) ? (PERFECTSTAT - pchar->dmg_threshold[1]) : amount;
            pchar->dmg_threshold[0] += (char)amount;
            pchar->dmg_threshold[1] += (char)amount;
            break;
        case CHAR_VAL_XP:
            ///< Experience
            charGiveXPOne(pchar, amount, sub_type);
            break;
        case CHAR_VAL_XP_TEAM:
            charGiveXPTeam(char_no, amount, sub_type);
            break;
            // Skills
        case CHAR_VAL_DVLVL:
            amount += pchar->darkvision_level;
            if (amount > PERFECTSTAT) amount = PERFECTSTAT;
            pchar->darkvision_level = (char)amount;
            break;
        case CHAR_VAL_SKLVL:
            amount += pchar->see_kurse_level;
            if (amount > PERFECTSTAT) amount = PERFECTSTAT;
            pchar->see_kurse_level = (char)amount;
            break;
        case CHAR_VAL_SILVL:
            amount += pchar->see_invisible_level;
            if (amount > PERFECTSTAT) amount = PERFECTSTAT;
            pchar->see_invisible_level = (char)amount;
            break;
        case CHAR_VAL_DMGRESIST:
            pchar->dmg_resist[sub_type][0] += (char)amount;
            break;
        case CHAR_VAL_COLSHIFT:
            ///<chr_set_redshift, chr_set_grnshift, chr_set_blushift
            break;
        case CHAR_VAL_JUMPPOWER:
            // pchar->jump_power
            break;
        case CHAR_VAL_BUMPDAMPEN:
            // pchar->bumpdampen
            break;
        case CHAR_VAL_BOUNCINESS:
            // pchar->dampen
            break;
        case CHAR_VAL_SIZE:
            // @todo: Use integer value divided  
            pchar->fat_goto += (float)valpair[0] * 0.025;  // Limit this?
            pchar->fat_goto_time += SIZETIME;
            break;
        case CHAR_VAL_ACCEL:
            if(duration_sec > 0)
            {
                if(charSetTimer(char_no, which, amount, duration_sec))
                {
                    pchar->max_accel[0] += (char)amount;
                    return;
                }
            }
            // chr_set_maxaccel( ptarget, ptarget->maxaccel_reset - fvaluetoadd );
            break;
    }    
}

/*
 * Name:
 *     charSetValue
 * Description:
 *     Sets the given value on character(s), using the given arguments
  * Input:
 *     char_no:    Number of character to handle 
 *     which:      Which value to change 
 *     sub_type:   Of 'which', e.g. for damage resistance values, otherwise e.g. 'damage' for life
 *     amount:     Which value to set
 */
void charSetValue(int char_no, int which, int sub_type, int amount)
{
    CHAR_T *pchar;
    // CAP_T *pcap;
    
    switch(which)
    {
        case CHAR_VAL_LIFE:
            break;
        case CHAR_VAL_MANA:
            break;
        case CHAR_VAL_STR:
            break;
        case CHAR_VAL_INTEL:
            break;
        case CHAR_VAL_WIS:
            break;
        case CHAR_VAL_DEX:
            break;
        case CHAR_VAL_CON:
            break;
        case CHAR_VAL_CHA:
            break;   
        case CHAR_VAL_DEFENSE:
            break;
        case CHAR_VAL_MANARET:
            break;
        case CHAR_VAL_MANAFLOW:
            break;
        case CHAR_VAL_DMGBOOST:
            break;
        case CHAR_VAL_DMGTHRES:
            break;
        case CHAR_VAL_XP:
            break;
        // Skills
        case CHAR_VAL_DVLVL:
            ///<darkvision_level
            break;
        case CHAR_VAL_SKLVL:
            ///<see_kurse_level
            break;
        case CHAR_VAL_SILVL:
            break;   
        case CHAR_VAL_DMGRESIST:
            break;   
        case CHAR_VAL_COLSHIFT:
            break;
        case CHAR_VAL_JUMPPOWER:
            break;
        case CHAR_VAL_BUMPDAMPEN:
            break;   
        case CHAR_VAL_BOUNCINESS:
            break;   
        case CHAR_VAL_SIZE:
            break;   
        case CHAR_VAL_ACCEL:
            break;
        /*
         @todo: Add these, set it as timer including save value
        case SETDAMAGETYPE:
            penc->setsave[value_idx]  = ptarget->damagetarget_damagetype;
            ptarget->damagetarget_damagetype = peve->setvalue[value_idx];
            break;

        case SETNUMBEROFJUMPS:
            penc->setsave[value_idx] = ptarget->jumpnumberreset;
            ptarget->jumpnumberreset = peve->setvalue[value_idx];
            break;

        case SETLIFEBARCOLOR:
            penc->setsave[value_idx] = ptarget->life_color;
            ptarget->life_color       = peve->setvalue[value_idx];
            break;

        case SETMANABARCOLOR:
            penc->setsave[value_idx] = ptarget->mana_color;
            ptarget->mana_color       = peve->setvalue[value_idx];
            break;

        case SETSLASHMODIFIER:
            penc->setsave[value_idx]              = ptarget->dmg_modifier[DAMAGE_SLASH];
            ptarget->dmg_modifier[DAMAGE_SLASH] = peve->setvalue[value_idx];
            break;

        case SETCRUSHMODIFIER:
            penc->setsave[value_idx]              = ptarget->dmg_modifier[DAMAGE_CRUSH];
            ptarget->dmg_modifier[DAMAGE_CRUSH] = peve->setvalue[value_idx];
            break;

        case SETPOKEMODIFIER:
            penc->setsave[value_idx]             = ptarget->dmg_modifier[DAMAGE_POKE];
            ptarget->dmg_modifier[DAMAGE_POKE] = peve->setvalue[value_idx];
            break;

        case SETHOLYMODIFIER:
            penc->setsave[value_idx]             = ptarget->dmg_modifier[DAMAGE_HOLY];
            ptarget->dmg_modifier[DAMAGE_HOLY] = peve->setvalue[value_idx];
            break;

        case SETEVILMODIFIER:
            penc->setsave[value_idx]             = ptarget->dmg_modifier[DAMAGE_EVIL];
            ptarget->dmg_modifier[DAMAGE_EVIL] = peve->setvalue[value_idx];
            break;

        case SETFIREMODIFIER:
            penc->setsave[value_idx]             = ptarget->dmg_modifier[DAMAGE_FIRE];
            ptarget->dmg_modifier[DAMAGE_FIRE] = peve->setvalue[value_idx];
            break;

        case SETICEMODIFIER:
            penc->setsave[value_idx]            = ptarget->dmg_modifier[DAMAGE_ICE];
            ptarget->dmg_modifier[DAMAGE_ICE] = peve->setvalue[value_idx];
            break;

        case SETZAPMODIFIER:
            penc->setsave[value_idx]            = ptarget->dmg_modifier[DAMAGE_ZAP];
            ptarget->dmg_modifier[DAMAGE_ZAP] = peve->setvalue[value_idx];
            break;

        case SETFLASHINGAND:
            penc->setsave[value_idx] = ptarget->flashand;
            ptarget->flashand        = peve->setvalue[value_idx];
            break;

        case SETLIGHTBLEND:
            penc->setsave[value_idx] = ptarget->inst.light;
            chr_set_light( ptarget, peve->setvalue[value_idx] );
            break;

        case SETALPHABLEND:
            penc->setsave[value_idx] = ptarget->inst.alpha;
            chr_set_alpha( ptarget, peve->setvalue[value_idx] );
            break;

        case SETSHEEN:
            penc->setsave[value_idx] = ptarget->inst.sheen;
            chr_set_sheen( ptarget, peve->setvalue[value_idx] );
            break;

        case SETFLYTOHEIGHT:
            penc->setsave[value_idx] = ptarget->flyheight;
            if ( 0 == ptarget->flyheight && ptarget->pos.z > -2 )
            {
                ptarget->flyheight = peve->setvalue[value_idx];
            }
            break;

        case SETWALKONWATER:
            penc->setsave[value_idx] = ptarget->waterwalk;
            if ( !ptarget->waterwalk )
            {
                ptarget->waterwalk = ( 0 != peve->setvalue[value_idx] );
            }
            break;

        case SETCANSEEINVISIBLE:
            penc->setsave[value_idx]     = ptarget->see_invisible_level > 0;
            ptarget->see_invisible_level = peve->setvalue[value_idx];
            break;

        case SETMISSILETREATMENT:
            penc->setsave[value_idx]  = ptarget->missiletreatment;
            ptarget->missiletreatment = peve->setvalue[value_idx];
            break;

        case SETCOSTFOREACHMISSILE:
            penc->setsave[value_idx] = ptarget->missilecost;
            ptarget->missilecost     = peve->setvalue[value_idx] * 16.0f;    // adjustment to the value stored in the file
            ptarget->missilehandler  = penc->owner_ref;
            break;

        case SETMORPH:
            // Special handler for morph
            penc->setsave[value_idx] = ptarget->skin;
            change_character( character, profile, 0, ENC_LEAVE_ALL ); // ENC_LEAVE_FIRST);
            break;

        case SETCHANNEL:
            penc->setsave[value_idx] = ptarget->canchannel;
            ptarget->canchannel      = ( 0 != peve->setvalue[value_idx] );
            break;
        */
    }
}

/* ================= Information functions ===================== */

/*
 * Name:
 *     charGetSkill
 * Description:
 *     This returns the skill level for the specified skill or 0 if the character doesn't
 *     have the skill. Also checks the skill IDSZ.
 * Input:
 *     char_no:    Number of character to give experience
 *     whichskill: Skill to look for (IDSZ)        
 */
int charGetSkill(int char_no, unsigned int whichskill)
{
    CHAR_T *pchar;
    IDSZ_T *pskill;
    
    
    //Any [NONE] IDSZ returns always "true"
    if (IDSZ_NONE == whichskill) return 1;
    
    pchar = &CharList[char_no];
    
    //Do not allow poison or backstab skill if we are restricted by code of conduct
    if ('POIS' == whichskill || 'STAB' == whichskill)
    {
        if (NULL != idszMapGet(pchar->skills, 'CODE', CHAR_MAX_SKILL))
        {
            return 0;
        }
    }
    
    // First check the character Skill ID matches
    // Then check for expansion skills too.
    /* @todo:
    if ( chr_get_idsz( pchar->ai.index, IDSZ_SKILL )  == whichskill )
    {
        return 1;
    }
    */
    
    // Simply return the skill level if we have the skill
    pskill = idszMapGet(pchar->skills, whichskill, CHAR_MAX_SKILL);
    if (pskill != NULL )
    {
        return pskill->value;
    }
    
    // Truesight allows reading
    if ('READ' == whichskill )
    {
        pskill = idszMapGet(pchar->skills, 'CKUR', CHAR_MAX_SKILL);

        if (pskill != NULL && pchar->see_invisible_level > 0 )
        {
            return pchar->see_invisible_level + pskill->value;
        }
    }

    //Skill not found
    return 0;
}

/*
 * Name:
 *     charChangeArmor
 * Description:
 *     This function changes the armor of the given character
 * Input:
 *     char_no: Number of character
 *     skin_no: Number of new skin 
 * Output:
 *     Number of new skin
 */
 int charChangeArmor(const int char_no, int skin_no)
 {
    CHAR_T *pchar;
    CAP_T *pcap;
    int i;
    char diff;

    
    pchar = &CharList[char_no];
    
    for(i = 0; i < DAMAGE_COUNT; i++)
    {
        // Remove the resistance values of the actual skin, 'act' must be >= 'base'
        diff = (char)(pchar->dmg_resist[i][0] - pchar->dmg_resist[i][1]);
        if(diff < 0) diff = 0;
        // Magic resistance left, if any
        pchar->dmg_resist[i][0] -= diff;
    }

        
    pcap = &CapList[pchar->cap_no];
    
    // Change stats associated with skin
    pchar->defense[0] = pchar->defense[1] = pcap->defense[skin_no];

    for(i = 0; i < DAMAGE_COUNT; i++)
    {
        pchar->dmg_modify[i][1] = pcap->dmg_modify[i][skin_no];
        pchar->dmg_resist[i][1] = pcap->dmg_resist[i][skin_no];
        // Reistance including possible magic
        pchar->dmg_resist[i][0] += (char)pcap->dmg_resist[i][skin_no];
    }    
    
    // @todo: set the character's maximum acceleration
    // chr_set_maxaccel( pchar, pcap->skin_info.maxaccel[skin_no] );
    
    return skin_no; 
}

/*
 * Name:
 *     charSetTimer 
 * Description:
 *     Sets a timer with given number, if possible
 * Input:
 *     char_no:      Number of character to set timer for
 *     which:        Which temporary value 
 *     add_val:      Temporary added value 
 *     duration_sec: Duration in seconds (< 0: Remove it)
 * Output:
 *     Clock could be set yes/no
 */
char charSetTimer(int char_no, int which, int add_val, int duration_sec)
{
    int i;
    CHAR_T *pchar;


    pchar = &CharList[char_no];

    // Only set timer if not already set
    for(i = 0; i < CHAR_MAX_TIMER; i++)
    {
        if(pchar->timers[i].which == (char)which)
        {
            if(duration_sec > 0)
            {
                // Set the clock to the new time
                pchar->timers[i].clock_sec = duration_sec;
                return 1;
            }
            else
            {
                pchar->timers[i].which = 0;
                pchar->timers[i].clock_sec = 0;
                // @todo: Do remove all effects ('spell_no')
                return 1;
            }
        }
    }

    // Loop trough all timers
    for(i = 0; i < CHAR_MAX_TIMER; i++)
    {
        if(pchar->timers[i].which == 0)
        {
            // Set the clock
            pchar->timers[i].which     = (char)which;
            pchar->timers[i].clock_sec = duration_sec;
            pchar->timers[i].add_val   = (char)add_val;
            return 1;
        }
    }
    // No free slot with clock found
    return 0;
}

/*
 * Name:
 *     charUpdateAll
 * Description:
 *     Does an update of the state of all characters
 * Input:
 *     sec_passed: Seconds passed since last call
 */
void charUpdateAll(float sec_passed)
{
    static float stat_clock = 1.0;
    int i;
    CHAR_T *pchar;
    
    
    // Only do update every second
    stat_clock -= sec_passed;
    
    if(stat_clock <= 0.0)
    {
        // Loop trough all characters in list, ignore items


        pchar = &CharList[1];
        
        while(pchar->cap_no != 0)
        {
            // Countdown the 'time_out' timer and send a message if it's done
            if(pchar->timer_set)
            {
                pchar->timer--;
                
                if(pchar->timer <= 0)
                {
                    pchar->timer_set = 0;
                    pchar->timer     = 0;
                    msgSend(0, pchar->id, MSG_TIMEOUT, NULL);
                }

            }

            // And now the other timers and
            for(i = 0; i < CHAR_MAX_TIMER; i++)
            {
                if(pchar->timers[i].which != 0)
                {
                    pchar->timers[i].clock_sec--;

                    if(pchar->timers[i].clock_sec <= 0)
                    {
                        // @todo: Do proper action if the time for this clock is over
                        //        e. g. end of 'spell', send possible message
                        // Mark the clock as free slot
                        pchar->timers[i].which     = 0;
                        pchar->timers[i].clock_sec = 0;
                    }
                }
            }
            // Next character
            pchar++;
        }
        // Set counter for next second
        stat_clock += 1.0;      // Next second
    }
}


/*
 * Name:
 *     charCallForHelp
 * Description:
 *     This function issues a call for help to all allies
 * Input:
 *     char_no:   Number of character which calls for help
 */
void charCallForHelp(const int char_no)
{
    CHAR_T *pchar, *pother;
    int team;
    int friends;

    
    pchar = &CharList[char_no];
    team = pchar->team[CHARSTAT_ACT];


    // @todo: Only call these for help which are on the teams list ?!
    TeamList[team].sissy_no = char_no;

    friends = ~pchar->t_foes;

    pother = &CharList[1];

    while(pother->id != 0)
    {
        if(pother != pchar && (friends & ~pother->t_foes))
        {
            msgSend(char_no, pother->id, MSG_CALLEDFORHELP, NULL);
        }
        pother++;
    }
}