/*******************************************************************************
*  CHAR.C                                                                      *
*    - EGOBOO-Editor                                                           *     
*                                                                              *
*    - Egoboo Characters                                                       *
*      (c)2012 Paul Mueller <muellerp61@bluewin.ch>                            *
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

#include <string.h>     /* strlwr() */

#include "sdlglcfg.h"   /* Read egoboo text files eg. passage, spawn    */
#include "editfile.h"   /* Get the work-directoires                     */    
#include "idsz_map.h";
#include "misc.h"       /* Random treasure objects                      */
#include "egodefs.h"    /* MPDFX_IMPASS                                 */

#include "char.h"       /* Own header                                   */

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

#define CHAR_MAX_CAP    100
#define CHAR_MAX        500
#define CHAR_MAX_SKIN          4  

/*******************************************************************************
* ENUMS								                                           *
*******************************************************************************/

/// The various ID strings that every character has
enum {
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
typedef struct {

    int val[2];       /* from, to */
    int perlevel[2];  /* from, to */
    
} CAP_STAT_T;

//--------------------------------------------------------------------------------------------

/// The character profile data, or cap (Type of Character)
/// The internal representation of the information in data.txt
typedef struct 
{
    char cap_name[MAXCAPNAMESIZE];             /// Name of this character profile (directory)
    // naming
    char classname[MAXCAPNAMESIZE];            ///< Class name
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

    float size;                          ///< Scale of model
    float size_perlevel;                 ///< Scale increases
    int   shadow_size;                   ///< Shadow size
    int   bump_size;                     ///< Bounding octagon
    char  bump_override_size;            ///< let bump_size override the measured object size
    int   bump_sizebig;                  ///< For octagonal bumpers
    char  bump_override_sizebig;         ///< let bump_sizebig override the measured object size
    int   bump_height;                   ///< the height of the object
    char  bump_override_height;          ///< let bump_height overrride the measured height of the object
    unsigned char stoppedby;                     ///< Collision Mask

    // movement
    float jump;                         ///< Jump power
    char  jump_number;                  ///< Number of jumps ( Ninja )
    float anim_speed_sneak;             ///< Sneak threshold
    float anim_speed_walk;              ///< Walk threshold
    float anim_speed_run;               ///< Run threshold
    unsigned char fly_height;           ///< Fly height
    char  waterwalk;                    ///< Walk on water?

    // status graphics
    unsigned char life_color;                     ///< Life bar color
    unsigned char mana_color;                     ///< Mana bar color
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
    int   experience_forlevel[MAXLEVEL];  ///< Experience needed for next level
    float experience[2];                     ///< Starting experience
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
    IDSZ_NODE_T  skills[MAX_IDSZ_MAP_SIZE];
    int          see_invisible_level;             ///< Can it see invisible?

    // random stuff
    char       stickybutt;      ///< Stick to the ground?
    // For graphics
    char       mdl_no;          /// < Display model for this character profile
    // particles for this profile
    int prt_list[CAP_MAX_PRT];
    
} CAP_T;



/*******************************************************************************
* DATA									                                       *
*******************************************************************************/

static int NumCap;
static int NumChar;
static CAP_T  CapList[CHAR_MAX_CAP + 2];
static CHAR_T CharList[CHAR_MAX + 2];

// Description for reading a cap-file
static SDLGLCFG_NAMEDVALUE CapVal[] = {
    // Read in the real general data
    { SDLGLCFG_VAL_STRING, CapList[0].classname, MAXCAPNAMESIZE },
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
* CODE									                                       *
*******************************************************************************/

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

            // Clear the idsz
            for(cnt = 0; cnt < IDSZ_COUNT; cnt++)
            {
                pcap -> idsz[cnt] = IDSZ_NONE;
            }

            // clear out the sounds
            for ( cnt = 0; cnt < SOUND_COUNT; cnt++ )
            {
                pcap->sound_index[cnt] = -1;
            }

            // Clear expansions...
            pcap -> reflect = 1;
            pcap -> isvaluable = -1;

            // either these will be overridden by data in the data.txt, or
            // they will be limited by the spawning character's max stats
            pcap -> life_spawn = 10000;
            pcap -> mana_spawn = 10000;

            // More stuff I forgot
            pcap -> stoppedby  = MPDFX_IMPASS;

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
        if(CharList[i].cap_no == 0)
        {
            return i;
        }
    }

    return 0;
}

/*
 * Name:
 *     charReadCap
 * Description:
 *     Spawns an object. If needed, the character profile is loaded 
 *     Loads a single object with given name. Looks up first in the module
 *     directory and after that
 * Input:
 *     pname *: Pointer on name of random treasure
 * Output:
 *     Number
 */
 static int charReadCap(char *objname)
 {

    int cno;
    char *fname;
    CAP_T *pcap;


    // Compare lower case
    strlwr(objname);

    // Only read if 'cap_name' not available yet
    for(cno = 1; cno < CHAR_MAX_CAP; cno++)
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

    if(cno > 0)
    {
        pcap = &CapList[cno];

        strcpy(pcap -> cap_name, objname);  // For checking if profile and object are already loaded
        // @todo: Get the directory of this object first
        // fdir = editfileMakeFileName(EDITFILE_OBJECTDIR, "");
        // Read character profile
        fname = editfileMakeFileName(EDITFILE_OBJECTDIR, "data.txt");

        if(fname[0] != 0)
        {
            // If object was found
            sdlglcfgEgobooValues(fname, CapVal, 0);

            // "message.txt"
            // "naming.txt"
            // "part0.txt" - "part9.txt"
            // "script.txt"
            // "tris.md2"
            // "icon0,bmp" - "icon4.bmp"
            // "tris0.bmp" - "tris0.bmp"
            // "sound0.wav" - "sound9.wav"

            return cno;
        }
        else
        {
            // @todo: msgSend(MSG_LOG, MSG_GAME_OBJNOTFOUND, objname)
        }
    }

    return 0;
 }

/* ========================================================================== */
/* ============================= PUBLIC FUNCTION(S) ========================= */
/* ========================================================================== */

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
    NumCap  = 0;
    CapList[0].cap_name[0] = 0;
    CapList[1].cap_name[0] = 0;
    
    NumChar = 0;
    CharList[0].cap_no = 0;
    CharList[1].cap_no = 0;
}

/*
 * Name:
 *     charCreate
 * Description:
 *     Creates a character. If needed, the character profile is loaded
 *     Loads a single object with given name. Looks up first in the module
 *     directory and after that
 * Input:
 *     objname *:    Pointer on name of object to create character from
 *     attachedto:   Is in inventory of this character
 *     inwhich_slot: In this slot of inventory ('L', 'R', 'I') or
 *     team:         Belongs to this team ('A' - 'Z')
 *     stt:          Statusbar should appear yes/no
 *     skin:         Which skin to use
 * Output:
 *     Number of character
 */
int charCreate(char *objname, int attachedto, char inwhich_slot, char team, char stt, char skin)
{
    int cap_no;
    int char_no;
    int slot_no;
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

            // @todo: Now create the char from the character profile
            pchar -> cap_no = cap_no;           /* Has this character profile   */
            pchar -> mdl_no = pcap -> mdl_no;   /* Number of model for display  */
            pchar -> icon_no = skin;            /* Number of models icon for display */
            pchar -> skin_no = skin;            /* Number of models skin for display */
            // Gender
            if('F' == pcap -> gender ) pchar -> gender = GENDER_FEMALE;
            else if ('M' == pcap -> gender) pchar -> gender = GENDER_MALE;
            else if ('R' == pcap -> gender) pchar -> gender = GENDER_RANDOM;
            else pcap->gender = GENDER_OTHER;
            
            // Team stuff
            team = (char)('Z' - team);
            pchar -> team[0] = team;    // Actual team
            pchar -> team[1] = team;    // Original team

            /*
            // character stats -- CHARSTAT_ACT / CHARSTAT_FULL
            char life[2];       ///< Basic character stats
            char mana[2];       ///< Mana stuff

            // combat stuff -- CHARSTAT_ACT / CHARSTAT_FULL
            char damageboost[2];                   ///< Add to swipe damage
            char damagethreshold[2];               ///< Damage below this number is ignored

            int  experience;        ///< Experience
            char experience_level;  ///< The Character's current level

            float fat;                           ///< Character's size
            float fat_goto;                      ///< Character's size goto

            // jump stuff
            char jump_number;                   ///< Number of jumps remaining
            char jump_ready;                    ///< For standing on a platform character
            */
            if(attachedto > 0)
            {
                /* It's an item */
                if(inwhich_slot == 'L') {
                    slot_no = 0;
                }
                else if(inwhich_slot == 'R')
                {
                    slot_no = 1;
                }
                else
                {
                    slot_no = -1;   /* Slot is found by function */
                }
                // Put it into inventory
                charInventoryAdd(attachedto, char_no, slot_no);
            }
            else
            {
                // 'inwhich_slot' is the characters direction
            }

            // "variable" properties
            pchar -> is_hidden = 0;
            pchar -> alive     = 1;
            /*
            // "constant" properties
            char isshopitem;                    ///< Spawned in a shop?
            char canbecrushed;                  ///< Crush in a door?
            char canchannel;                    ///< Can it convert life to mana?

            // graphics info
            int   sparkle;              ///< Sparkle the displayed icon? 0 for off
            */
            pchar -> draw_stats = stt;  ///< Display stats?
            /*
            float shadow_size[2];   ///< Size of shadow  -- CHARSTAT_ACT / CHARSTAT_FULL
            int   ibillboard;       ///< The attached billboard

            // Skills
            int  darkvision_level;
            int  see_kurse_level;

            // missile handling
            char missiletreatment;  ///< For deflection, etc.
            char missilecost;       ///< Mana cost for each one
            int  missilehandler;    ///< Who pays the bill for each one...
            
            // sound stuff
            int  loopedsound_channel;           ///< Which sound channel it is looping on, -1 is none.
            */

            
            // Character is created, return its number
            /*
            // Other info
                char *script;       // Pointer ai_on script to use for this character, if any 
            */
            
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

    if(pchar -> cap_no > 0 && CharList[item_no].cap_no > 0)
    {
        pitem = &CharList[item_no];

        // don't allow sub-inventories, e.g.
        if(pitem -> attached_to > 0)
        {
            return 0;
        }

        //too big item?
        if(pitem -> istoobig)
        {
            // SET_BIT( pitem->ai.alert, ALERTIF_NOTPUTAWAY );
            if(pchar -> islocalplayer)
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
                if(pchar -> inventory[i].item_no == 0)
                {
                    slot_no = i;
                    break;
                }
            }
        }

        // @todo: Support stacked items (In hand too)
        // @todo: Support 'merge' items

        // don't override existing items
        if(slot_no < INVEN_COUNT)
        {
            if(pchar -> inventory[slot_no].item_no == 0)
            {
                pchar -> inventory[slot_no].item_no = item_no;
                // Link back to character
                // display code has to check slots 0 and 1 for display in 3D
                pitem -> attached_to = char_no;

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

    if(pchar -> cap_no > 0 && slot_no < INVEN_COUNT)
    {
        item_no = pchar -> inventory[slot_no].item_no;
        
        if(item_no > 0)
        {
            pitem = &CharList[item_no];
            
            if (pitem -> iskursed && !ignorekurse )
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
                pchar -> inventory[slot_no].item_no = 0;
                pitem -> attached_to  = 0;
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
    
    if(pchar -> cap_no > 0)
    {
        if(slot_no < 0)
        {
            // Find the first slot
            for(i = SLOT_COUNT; i < INVEN_COUNT; i++)
            {
                if(pchar -> inventory[i].item_no > 0)
                {
                    slot_no = i;
                    break;
                }
            }
        }

        if(grip_off <= HAND_RIGHT && slot_no > HAND_RIGHT && slot_no < INVEN_COUNT)
        {
            inv_item  = pchar -> inventory[slot_no].item_no;
            grip_item = pchar -> inventory[grip_off].item_no;

            // Make space to put the item into the grip
            pchar -> inventory[grip_off].item_no = 0;

            // Put inventory item to grip
            if(charInventoryAdd(char_no, inv_item, grip_off))
            {
                // Put item from grip to inventory
                pchar -> inventory[slot_no].item_no = grip_item;
            }
            else
            {
                // Put item back to grip
                pchar -> inventory[grip_off].item_no = grip_item;
            }
        }
    }

    return 0;
}

/* ================= _____ functions ===================== */