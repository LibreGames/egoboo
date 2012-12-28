/*******************************************************************************
*  CHAR.C                                                                      *
*    - EGOBOO-Editor                                                           *     
*                                                                              *
*    - Egoboo Characters                                                       *
*      (c)2012 Paul Mueller <muellerp61@bluewin.ch>                            *
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
* INCLUDES								                                       *
*******************************************************************************/

/*******************************************************************************
* DEFINES								                                       *
*******************************************************************************/

#define ULTRABLUDY           2          ///< This makes any damage draw blud

//Damage shifts
#define DAMAGEINVICTUS      (1 << 5)                      ///< 00x00000 Invictus to this type of damage
#define DAMAGEMANA          (1 << 4)                      ///< 000x0000 Deals damage to mana
#define DAMAGECHARGE        (1 << 3)                       ///< 0000x000 Converts damage to mana
#define DAMAGEINVERT        (1 << 2)                       ///< 00000x00 Makes damage heal
#define DAMAGESHIFT         3                       ///< 000000xx Resistance ( 1 is common )

#define GET_DAMAGE_RESIST(BITS) ( (BITS) & DAMAGESHIFT )

#define CHARSTAT_ACT  0
#define CHARSTAT_FULL 1

#define CHAR_MAX_TIMER 15

// Special non-enchant timers
#define CHAR_GROGTIME    ((char)-1) ///< Grog timer
#define CHAR_DAZETIME    ((char)-2) ///< Daze timer
#define CHAR_BORETIME    ((char)-3) ///< Boredom timer
#define CHAR_CAREFULTIME ((char)-4) ///< "You hurt me!" timer
#define CHAR_RELOADTIME  ((char)-5) ///< Time before another shot
#define CHAR_DAMAGETIME  ((char)-6) ///< Invincibility timer
#define CHAR_JUMPTIME    ((char)-7) ///< Delay until next jump
#define CHAR_FATGOTTIME  ((char)-8) ///< Time left in size change

// Special Teams
#define TEAM_DAMAGE ((char)(('Z' - 'A') + 1))   ///< For damage tiles
#define TEAM_MAX    ((char)(TEAM_DAMAGE + 1))

/*******************************************************************************
* ENUMS                                                                        *
*******************************************************************************/

/// The possible damage types
enum {
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
enum {
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

enum {
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
    HAND_RIGHT,    
    INVEN_NECK,
    INVEN_WRIS,
    INVEN_FOOT,
    INVEN_PACK,         /* >= INVEN_PACK is in backpack */
    INVEN_COUNT = 8,
    SLOT_COUNT  = 2
} E_INVENTORY;


/// What gender a character can be spawned with
enum {
    GENDER_FEMALE = 0,
    GENDER_MALE,
    GENDER_OTHER,
    GENDER_RANDOM,
    GENDER_COUNT
} E_GENDER;

/*******************************************************************************
* TYPEDEFS                                                                     *
*******************************************************************************/

typedef struct
{
    char which;         /* < 0: Display timerand 'GROG' and so on..., > 0: Spell(Enchant) number    */
    int  timer;         /* Down-Counter for this one                                                */
    
} CHAR_TIMER_T;

typedef struct 
{
    int  item_no;   /* Character in inventory slot                      */
    char count;     /* Number of characters in inventory slot if stack  */ 
    
} INVENT_SLOT_T;

typedef struct
{
    int  cap_no;        /* Has this character profile -- > 0: Character is available    */
    int  mdl_no;        /* Number of model for display                                  */
    char icon_no;       /* Number of icon to display                                    */   
    char skin_no;       /* Number of skin for display                                   */     
    int  obj_no;        /* Attached to this 3D-Object                                   */
    char is_overlay;    ///< Is this an overlay? Track aitarget...
    char alive;         ///< Is it alive?
    char which_player;  ///< > 0 = player: Attached to this player for movement
    int  attached_to;   ///< > 0 if character is a held weapon or in inventory (HAND_LEFT, HAND_RIGHT)    
    // gender
    char gender;        /* Gender of character          */
    // character state    
    char state;         /* State of this character      */
    char latch;         /* Latch data                   */
    
    // Team stuff -- CHARSTAT_ACT / CHARSTAT_FULL
    char team[2];       ///< Character's team -- Character's starting team
    
    // character stats -- CHARSTAT_ACT / CHARSTAT_FULL
    char life[2];       ///< Basic character stats
    char mana[2];       ///< Mana stuff 
    
    // combat stuff -- CHARSTAT_ACT / CHARSTAT_FULL
    char damageboost[2];                   ///< Add to swipe damage
    char damagethreshold[2];               ///< Damage below this number is ignored
    
    int  experience;        ///< Experience
    char experience_level;  ///< The Character's current level
        
    // Hands (0, 1) and inventory
    INVENT_SLOT_T inventory[SLOT_COUNT + INVEN_COUNT]; /* > 0 if theres a character in given slot */
    
    // enchant and other timed data e.g time is always counted in ticks that is 1000 ticks = 1 second
    CHAR_TIMER_T timers[CHAR_MAX_TIMER];

    float fat;                           ///< Character's size
    float fat_goto;                      ///< Character's size goto
    
    // jump stuff
    char jump_number;                   ///< Number of jumps remaining
    char jump_ready;                    ///< For standing on a platform character
    
    // "variable" properties
    char is_hidden;
    char islocalplayer;         ///< btrue = local player
    char waskilled;             ///< Fix for network
    char hitready;              ///< Was it just dropped?
    char iskursed;              ///< For boots and rings and stuff

    // "constant" properties
    char isshopitem;                    ///< Spawned in a shop?
    char canbecrushed;                  ///< Crush in a door?
    char canchannel;                    ///< Can it convert life to mana?
    
    // graphics info
    int   sparkle;          ///< Sparkle the displayed icon? 0 for off
    char  draw_stats;       ///< Display stats?
    float shadow_size[2];   ///< Current size of shadow  CHARSTAT_ACT / CHARSTAT_FULL
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
    // 
    int istoobig;       // For items
    // Other info
    char *script;       /* Pointer on ai_script to use for this character, if any */
    
    /* Other info for graphics and movement is held in the general SDL3D_OBJECT */
} CHAR_T;

/*******************************************************************************
* CODE 								                                           *
*******************************************************************************/

void charInit(void);
int  charCreate(char *objname, int attachedto, char inwhich_slot, char team, char stt, char skin);
CHAR_T *charGet(int char_no); 

// inventory functions
char charInventoryAdd(const int char_no, const int item, int inventory_slot);
int  charInventoryRemove(const int char_no, int inventory_slot, char ignorekurse);
char charInventorySwap(const int char_no, int inventory_slot, int grip_off);

#endif  /* #define _CHAR_H_ */