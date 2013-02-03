/*******************************************************************************
*  CHAR.C                                                                      *
*    - EGOBOO-Game                                                             *     
*                                                                              *
*    - Egoboo Characters                                                       *
*      (c) The Egoboo Team                                                     *
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

#ifndef _CHAR_H_
#define _CHAR_H_

/*******************************************************************************
* INCLUDES								                                   *
*******************************************************************************/

#include "ai.h"         /* AI_STATE_T and its initialization            */
#include "idsz.h"       // List of special skills 

/*******************************************************************************
* DEFINES								                                   *
*******************************************************************************/

#define ULTRABLUDY           2          ///< This makes any damage draw blud

//Damage shifts
#define DAMAGEINVICTUS      (1 << 5)                ///< 00x00000 Invictus to this type of damage
#define DAMAGEMANA          (1 << 4)                ///< 000x0000 Deals damage to mana
#define DAMAGECHARGE        (1 << 3)                ///< 0000x000 Converts damage to mana
#define DAMAGEINVERT        (1 << 2)                ///< 00000x00 Makes damage heal
#define DAMAGESHIFT         3                       ///< 000000xx Resistance ( 1 is common )

#define GET_DAMAGE_RESIST(BITS) ( (BITS) & DAMAGESHIFT )

#define CHAR_NAMELEN  24  
#define CHARSTAT_ACT  0         // Actual state with modifications by armor an spells
#define CHARSTAT_BASE 1         // State without any modifications by armor or spells

#define CHAR_MAX_TIMER 15
#define CHAR_MAX_SKILL 32

// Special non-enchant timers
#define CHAR_GROGTIME    ((char)-1)  ///< Grog timer
#define CHAR_DAZETIME    ((char)-2)  ///< Daze timer
#define CHAR_BORETIME    ((char)-3)  ///< Boredom timer
#define CHAR_CAREFULTIME ((char)-4)  ///< "You hurt me!" timer
#define CHAR_RELOADTIME  ((char)-5)  ///< Time before another shot
#define CHAR_DAMAGETIME  ((char)-6)  ///< Invincibility timer
#define CHAR_JUMPTIME    ((char)-7)  ///< Delay until next jump
#define CHAR_FATGOTTIME  ((char)-8)  ///< Time left in size change
#define CHAR_POOFTIME    ((char)-9)  ///< Time left for poof
#define CHAR_DEFENDTIME  ((char)-10) ///< Time left for defense

// Teams constants
#define TEAM_DAMAGE ((char)(('Z' - 'A') + 2))   ///< For damage tiles
#define TEAM_NOLEADER 0xFFFF                    ///< If the team has no leader...

/*******************************************************************************
* ENUMS                                                                        *
*******************************************************************************/

/// The possible damage types
enum
{
    DAMAGE_NONE  = 0,
    DAMAGE_SLASH = 1,
    DAMAGE_CRUSH,
    DAMAGE_POKE,
    DAMAGE_HOLY,                             ///< (Most invert Holy damage )
    DAMAGE_EVIL,
    DAMAGE_FIRE,
    DAMAGE_ICE,
    DAMAGE_ZAP,
    DAMAGE_COUNT
    
} E_DAMAGE_TYPE;

/// A list of the possible special experience types
enum
{
    XP_FINDSECRET = 1,                          ///< Finding a secret
    XP_WINQUEST,                                ///< Beating a module or a subquest
    XP_USEDUNKOWN,                              ///< Used an unknown item
    XP_KILLENEMY,                               ///< Killed an enemy
    XP_KILLSLEEPY,                              ///< Killed a sleeping enemy
    XP_KILLHATED,                               ///< Killed a hated enemy
    XP_TEAMKILL,                                ///< Team has killed an enemy
    XP_TALKGOOD,                                ///< Talk good, er...  I mean well
    XP_COUNT,                                   ///< Number of ways to get experience

    XP_DIRECT     = 255                         ///< No modification
} E_XP_TYPE;

enum
{
    SOUND_NONE     = 0,
    SOUND_FOOTFALL = 1,
    SOUND_JUMP,
    SOUND_SPAWN,
    SOUND_DEATH,

    /// old "RTS" stuff
    SPEECH_MOVE,
    SPEECH_MOVEALT,
    SPEECH_ATTACK,
    SPEECH_ASSIST,
    SPEECH_TERRAIN,
    SPEECH_SELECT,

    SOUND_COUNT,

    SPEECH_BEGIN = SPEECH_MOVE,
    SPEECH_END   = SPEECH_SELECT
} E_SOUND_TYPES;

/* All items a character has: 0 + 1 are the hands others are equip and backpack */
enum
{
    HAND_LEFT  = 0,
    HAND_RIGHT,         // @todo: Adjust code for handling 'equipment'
    INVEN_NECK,         // Are weared if correct IDSZ
    INVEN_WRIS,         // Are weared if correct IDSZ
    INVEN_FOOT,         // Are weared if correct IDSZ
    INVEN_PACK,         /* >= INVEN_PACK is in backpack */
    INVEN_COUNT = 8,
    SLOT_COUNT  = 2
    
} E_INVENTORY;


/// What gender a character can be spawned with
enum
{
    GENDER_FEMALE = 0,
    GENDER_MALE,
    GENDER_OTHER,
    GENDER_RANDOM,
    GENDER_COUNT
    
} E_GENDER;

enum
{
    CHAR_INVFUNC_UNKURSE = 1
    
} E_INVENT_FUNC;

enum 
{
    CHAR_GIVE_XP = 1,
    CHAR_DAMAGE,
    CHAR_HEAL
    
} E_CHAR_FUNC;

enum 
{
    CHAR_VAL_NONE = 0,
    CHAR_VAL_LIFE,
    CHAR_VAL_MANA,
    CHAR_VAL_STR,
    CHAR_VAL_STRPERC,      
    CHAR_VAL_INTEL,
    CHAR_VAL_WIS,
    CHAR_VAL_DEX,
    CHAR_VAL_CON,
    CHAR_VAL_CHA,
    CHAR_VAL_DEFENSE,
    CHAR_VAL_MANARET,
    CHAR_VAL_MANAFLOW,
    CHAR_VAL_DMGBOOST,  ///< Add to swipe damage  ///<damage_boost
    CHAR_VAL_DMGTHRES,  ///< Damage below this number is ignored
    CHAR_VAL_XP,
    CHAR_VAL_XP_TEAM,
    // Skills
    CHAR_VAL_DVLVL,     ///<darkvision_level
    CHAR_VAL_SKLVL,     ///<see_kurse_level
    CHAR_VAL_SILVL,     ///<see_invisible_level
    CHAR_VAL_DMGRESIST, ///<dmg_resist
    CHAR_VAL_COLSHIFT,  ///<chr_set_redshift, chr_set_grnshift, chr_set_blushift
    CHAR_VAL_JUMPPOWER,
    CHAR_VAL_BUMPDAMPEN,
    CHAR_VAL_BOUNCINESS,    
    CHAR_VAL_SIZE,      ///<fat_goto
    CHAR_VAL_ACCEL
    
} E_CHAR_VALUE;

/*******************************************************************************
* TYPEDEFS                                                                     *
*******************************************************************************/

typedef struct
{
    char which;         /* < 0: Display timer and 'GROG' and so on..., > 0: Number of temp value    */
    int  clock_sec;     /* > Down-Counter for this one in seconds                                   */
    char add_val;       // Temporary added value, to remove as time is over
    
} CHAR_TIMER_T;

typedef struct 
{
    int  item_no;   /* Character in inventory slot                      */
    char count;     /* Number of characters in inventory slot if stack  */ 
    
} INVENT_SLOT_T;

typedef struct
{
    int  id;            /* < 0: deleted / > 0: Exists, number of character              */
    int  cap_no;        /* Has this character profile -- > 0: Character is available    */
    int  mdl_no;        /* Number of model for display                                  */
    char icon_no;       /* Number of icon to display                                    */   
    char skin_no;       /* Number of skin for display                                   */     
    int  obj_no;        /* Attached to this 3D-Object                                   */
    char which_player;  ///< > 0 = player: Attached to this player for movement   
    char is_overlay;    ///< Is this an overlay? Track aitarget...    
    int  attached_to;   ///< > 0 if character is a held weapon or in inventory (HAND_LEFT, HAND_RIGHT)    
                        /// Or the character is riding a mount
    char life_color,
         mana_color;    ///< Colors for displayed bars
    // Belongs  to a passage, invokes code for this passage / For editor
    char psg_no;
    // name and gender
    char name[CHAR_NAMELEN + 1]; /* Name of this character                              */
    char gender;                /* Gender of character          */
    // character state    
    char state;         /* ///< Short term memory for AI: Comment: Number of AI-State = char_no */
    char latch;         /* Latch data                                   */    
    int  money;         ///< Money this character has
    // Team stuff -- CHARSTAT_ACT / CHARSTAT_FULL
    char team[2];       ///< Character's team -- Character's starting team   
    unsigned int t_foes;    // Our foes as flags   !foe == friend 
    // Basic character stats -- CHARSTAT_ACT / CHARSTAT_FULL
    short int life[2];  ///< Hit-Points
    short int mana[2];  ///< Mana stuff 
    char str[2];        // strength       
    char itl[2];        // intelligence
    char wis[2];        // wisdom
    char dex[2];        // dexterity
    char con[2];        // constitution
    char cha[2];        // charisma
    char defense[2];    // Given by armor
    char mana_return;
    char mana_flow;
    // combat stuff -- CHARSTAT_ACT / CHARSTAT_FULL
    char dmg_boost[2];                  ///< Add to swipe damage
    char dmg_threshold[2];              ///< Damage below this number is ignored
    int  max_accel[2];                  ///< Maximal acceleration
    int  ammo[2];                       ///< Actual and max ammo
    // Experience points and level
    int  experience;        ///< Experience
    char experience_level;  ///< The Character's current level
    // Skills               ///< @todo: Replace this by multiple skills based on IDSZ
    char darkvision_level;
    char see_kurse_level;
    char see_invisible_level;
    char dmg_resist[DAMAGE_COUNT][2];   // Resistance to damage in percent
    char dmg_modify[DAMAGE_COUNT][2];   // Attacking modifier to damage in percent added to base
    // Hands (0, 1) and inventory
    INVENT_SLOT_T inventory[SLOT_COUNT + INVEN_COUNT]; /* > 0 if theres a character in given slot */
    
    // enchant and other timed data e.g time is always counted in ticks that is 1000 ticks = 1 second
    CHAR_TIMER_T timers[CHAR_MAX_TIMER];
    // Skills of character
    IDSZ_T skills[CHAR_MAX_SKILL + 1];
    // Size of model
    float fat;                  ///< Character's size
    float fat_goto;             ///< Character's size goto
    float fat_goto_time;        ///< Time to go ot that size
    
    // jump stuff
    char jump_number;           ///< Number of jumps remaining
    char jump_power;            ///< Pwer for jump
    char jump_ready;            ///< For standing on a platform character
    unsigned char fly_height;           ///< Fly height
    // NEW: Properties as flags (saves saving space)
    unsigned int properties;    ///< Boolean values as flags
    // "variable" properties
    char is_hidden;
    char islocalplayer;         ///< btrue = local player
    char waskilled;             ///< Fix for network
    char hitready;              ///< Was it just dropped?
    char iskursed;              ///< For boots and rings and stuff
    char istoobig;              ///< For items
    char isitem;  

    // "constant" properties
    char isshopitem;            ///< Spawned in a shop?
    char canbecrushed;          ///< Crush in a door?
    char canchannel;            ///< Can it convert life to mana?
    
    // graphics info
    int   sparkle;          ///< Sparkle the displayed icon? 0 for off
    char  draw_stats;       ///< Display stats?
    float shadow_size[2];   ///< Current size of shadow  CHARSTAT_ACT / CHARSTAT_FULL
    int   ibillboard;       ///< The attached billboard
    
    // missile handling
    char missiletreatment;  ///< For deflection, etc.
    char missilecost;       ///< Mana cost for each one
    int  missilehandler;    ///< Who pays the bill for each one...
    
    // sound stuff
    int sound_index[SOUND_COUNT];   ///< All this characters sounds
    int loopedsound_channel;        ///< Which sound channel it is looping on, -1 is none.
    
    /* Other info for graphics and movement is held in the general SDL3D_OBJECT */
    char stopped_by;
    int reaffirm_damagetype;
    char invictus;
    char nameknown;            

    /* Additional help info  */
    char *obj_name;     /* Pointer on the name of the object in CAP_T           */
    char *class_name;   /* @todo: Pointer on the classes name from CAP_T        */
    // AI-Stuff if the character is AI-Controlled
    AI_STATE_T ai;              ///< ai data and script to use
    // More flags from CAP_T for faster code and model graphics
    unsigned char flashand;
    unsigned char alpha_base;
    unsigned char light_base;
    // physics
    int   weight;             ///< Weight
    float dampen;                     ///< Bounciness
    float bumpdampen;                 ///< Mass
    // Other stuff
    char platform;
    char canuseplatforms;
    
} CHAR_T;

/*******************************************************************************
* CODE 								                                           *
*******************************************************************************/

/* ================= Basic character functions =============== */
void charInit(void);
int  charCreate(char *objname, char team, char stt, int money, char skin, char psg);
CHAR_T *charGet(int char_no); 

/* ================= General inventory functions ===================== */
char charInventoryAdd(const int char_no, const int item_no, int inventory_slot);
int  charInventoryRemove(const int char_no, int inventory_slot, char ignorekurse, char destroy_it);
char charInventorySwap(const int char_no, int inventory_slot, int grip_off);

/* ================= Timer functions ===================== */
// @todo: Add timers, delete timers

/* ================= Gameplay functions ================== */
void charInventoryFunc(int char_no, int func_no);
void charDamage(const char char_no, char dir_no, int valpair[2], char team,
                int attacker, int effects);
void charAddValue(int char_no, int which, int sub_type, int valpair[2], int duration_sec);
void charSetValue(int char_no, int which, int sub_type, int amount);
int  charGetSkill(int char_no, unsigned int whichskill); /* IDSZ */

char charSetTimer(int char_no, int which, int add_val, int duration_sec);
void charUpdateAll(float sec_passed);

#endif  /* #define _CHAR_H_ */