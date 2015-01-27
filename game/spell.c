/*******************************************************************************
*  SPELL.C                                                                     *
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
* INCLUDES								                                   *
*******************************************************************************/

#include <stdio.h>          // sprintf()


#include "sdlglcfg.h"       // Load a file with given description
#include "egofile.h"        // Directory of actual object to use
#include "egodefs.h"
#include "idsz.h"
#include "char.h"

// Own header
#include "spell.h"

/*******************************************************************************
* DEFINES								                                   *
*******************************************************************************/

#define MAX_SPELL 120
#define MAX_SPELL_SET 700
#define MAX_SPELL_ADD 700 

// The different Flags
#define SPELL_FRETARGET         ((char)(0))   ///< Pick a weapon?
#define SPELL_FOVERRIDE         ((char)(1))   ///< Override other enchants?
#define SPELL_FREMOVEOVERRIDDEN ((char)(2))   ///< Remove other enchants?
#define SPELL_FKILLTARGETONEND  ((char)(3))   ///< Kill the target on end?
#define SPELL_FPOOFONEND        ((char)(4))   ///< Spawn a poof on end?
#define SPELL_FENDIFCANTPAY     ((char)(5))   ///< End on out of mana
#define SPELL_FSTAYIFNOOWNER    ((char)(6))   ///< Stay if owner has died?
#define SPELL_FSPAWN_OVERLAY    ((char)(7))   ///< Spawn an overlay? 'spawn_overlay'
#define SPELL_FSTAYIFTARGETDEAD ((char)(8))   ///< Stay if target has died?
#define SPELL_FSEEKURSE         ((char)(9))   ///< Allow target to see kurses
#define SPELL_FDARKVISION       ((char)(10))  ///< Allow target to see in darkness


#define MAX_SET_VALUES  23
#define MAX_ADD_VALUES  16
#define MAX_BOOL_VALUES 12
#define MAX_IDSZ_VALUES 12

/*******************************************************************************
* TYPEDEFS								                                   *
*******************************************************************************/

typedef struct
{
    int  flags;                        ///< All booleans as flags
    int  time;                         ///< Time in seconds
    int  endmessage;                   ///< Message for end -1 for none
    char dontdamagetype;               ///< Don't work if ...
    char onlydamagetype;               ///< Only work if ...
    unsigned int  removedbyidsz;       ///< By particle or [NONE]
    int   contspawn_timer;             ///< Spawn timer
    int   contspawn_amount;            ///< Spawn amount
    int   contspawn_facingadd;         ///< Spawn in circle
    int   contspawn_pip;               ///< Spawn type ( local )
    int   endsound_index;              ///< Sound on end (-1 for none)
    char  spawn_overlay;               ///< Spawn an overlay?
    char  stayiftargetdead;            ///< Stay if target has died?

    // Boost values
    short int owner_mana;
    short int owner_life;
    short int target_mana;
    short int target_life;

    // the enchant values
    // Number of enchant values
    char num_set;                   ///<Number of values to set
    int set_idx;                    ///<Which values to set and their amount
    char num_add;                   ///<Number of values to add
    int add_idx;                    ///<Which values to set and their amount
   
    // other values that are enchanted
    int  seekurse;                  ///< Allow target to see kurses
    int  darkvision;                ///< Allow target to see in darkness
    
} SPELL_T;

/*******************************************************************************
* DATA									                                   *
*******************************************************************************/

// Data to change to set after 'raw' reading of file
static char  BoolVal[33];       // Boolean values
static char  RemoveIdsz[15];    // Removed by IDSZ
static char  ExtIdsz[MAX_IDSZ_VALUES + 2][15];      // Expansions
static char  SetValues[MAX_SET_VALUES + 1][15]; // Reading it raw as string : MAX_ENCHANT_SET
static float AddValues[MAX_ADD_VALUES + 2];     // Reading raw, only use if != 0MAX_ENCHANT_ADD


// The spells itself
static Spell_Idx;
static SPELL_T Spells[MAX_SPELL + 2];
// The set values
static SpellSet_Idx;    // Index while loading spells for pointers
static SPELL_SET_T SpellSet[MAX_SPELL_SET + 2];
// The addition values 
static SpellAdd_Idx;
static SPELL_ADD_T SpellAdd[MAX_SPELL_SET + 2];

static SDLGLCFG_NAMEDVALUE SpellVal[] =
{
    { SDLGLCFG_VAL_BOOLEAN, &BoolVal[SPELL_FRETARGET] },
    { SDLGLCFG_VAL_BOOLEAN, &BoolVal[SPELL_FOVERRIDE] },
    { SDLGLCFG_VAL_BOOLEAN, &BoolVal[SPELL_FREMOVEOVERRIDDEN] },
    { SDLGLCFG_VAL_BOOLEAN, &BoolVal[SPELL_FKILLTARGETONEND] },
    { SDLGLCFG_VAL_BOOLEAN, &BoolVal[SPELL_FPOOFONEND] },
    { SDLGLCFG_VAL_INT, &Spells[0].time },
    { SDLGLCFG_VAL_INT, &Spells[0].endmessage },
    // Drain stuff
    { SDLGLCFG_VAL_FLOAT, &Spells[0].owner_mana },
    { SDLGLCFG_VAL_FLOAT, &Spells[0].target_mana },
    { SDLGLCFG_VAL_BOOLEAN, &BoolVal[SPELL_FENDIFCANTPAY] },
    { SDLGLCFG_VAL_INT, &Spells[0].owner_life },
    { SDLGLCFG_VAL_INT, &Spells[0].target_life },

    { SDLGLCFG_VAL_ONECHAR, &Spells[0].dontdamagetype },
    { SDLGLCFG_VAL_ONECHAR, &Spells[0].onlydamagetype },
    { SDLGLCFG_VAL_STRING,  RemoveIdsz, 15 },
    // Now the set values
    { SDLGLCFG_VAL_STRING, SetValues[0], 15 },
    { SDLGLCFG_VAL_STRING, SetValues[1], 15 },
    { SDLGLCFG_VAL_STRING, SetValues[2], 15 },
    { SDLGLCFG_VAL_STRING, SetValues[3], 15 },
    { SDLGLCFG_VAL_STRING, SetValues[4], 15 },
    { SDLGLCFG_VAL_STRING, SetValues[5], 15 },
    { SDLGLCFG_VAL_STRING, SetValues[6], 15 },
    { SDLGLCFG_VAL_STRING, SetValues[7], 15 },
    { SDLGLCFG_VAL_STRING, SetValues[8], 15 },
    { SDLGLCFG_VAL_STRING, SetValues[9], 15 },
    { SDLGLCFG_VAL_STRING, SetValues[10], 15 },
    { SDLGLCFG_VAL_STRING, SetValues[11], 15 },
    { SDLGLCFG_VAL_STRING, SetValues[12], 15 },
    { SDLGLCFG_VAL_STRING, SetValues[13], 15 },
    { SDLGLCFG_VAL_STRING, SetValues[14], 15 },
    { SDLGLCFG_VAL_STRING, SetValues[15], 15 },
    { SDLGLCFG_VAL_STRING, SetValues[16], 15 },
    { SDLGLCFG_VAL_STRING, SetValues[17], 15 },
    { SDLGLCFG_VAL_STRING, SetValues[18], 15 },
    { SDLGLCFG_VAL_STRING, SetValues[19], 15 },
    { SDLGLCFG_VAL_STRING, SetValues[20], 15 },
    { SDLGLCFG_VAL_STRING, SetValues[21], 15 },
    { SDLGLCFG_VAL_STRING, SetValues[22], 15 },
    // Now the values to add (cumulative)
    { SDLGLCFG_VAL_FLOAT,  &AddValues[0] },
    { SDLGLCFG_VAL_FLOAT,  &AddValues[1] },
    { SDLGLCFG_VAL_FLOAT,  &AddValues[2] },
    { SDLGLCFG_VAL_FLOAT,  &AddValues[3] },
    { SDLGLCFG_VAL_FLOAT,  &AddValues[4] },
    { SDLGLCFG_VAL_FLOAT,  &AddValues[5] },
    { SDLGLCFG_VAL_FLOAT,  &AddValues[6] },
    { SDLGLCFG_VAL_FLOAT,  &AddValues[7] },
    { SDLGLCFG_VAL_FLOAT,  &AddValues[8] },
    { SDLGLCFG_VAL_FLOAT,  &AddValues[9] },
    { SDLGLCFG_VAL_FLOAT,  &AddValues[10] },
    { SDLGLCFG_VAL_FLOAT,  &AddValues[11] },
    { SDLGLCFG_VAL_FLOAT,  &AddValues[12] },
    { SDLGLCFG_VAL_FLOAT,  &AddValues[13] },
    { SDLGLCFG_VAL_FLOAT,  &AddValues[14] },
    { SDLGLCFG_VAL_FLOAT,  &AddValues[15] },
    // Possible extensions
    { SDLGLCFG_VAL_STRING, ExtIdsz[0], 15 },
    { SDLGLCFG_VAL_STRING, ExtIdsz[1], 15 },
    { SDLGLCFG_VAL_STRING, ExtIdsz[2], 15 },
    { SDLGLCFG_VAL_STRING, ExtIdsz[3], 15 },
    { SDLGLCFG_VAL_STRING, ExtIdsz[4], 15 },
    { SDLGLCFG_VAL_STRING, ExtIdsz[5], 15 },
    { SDLGLCFG_VAL_STRING, ExtIdsz[6], 15 },
    { SDLGLCFG_VAL_STRING, ExtIdsz[7], 15 },
    { SDLGLCFG_VAL_STRING, ExtIdsz[8], 15 },
    { SDLGLCFG_VAL_STRING, ExtIdsz[9], 15 },
    { SDLGLCFG_VAL_STRING, ExtIdsz[10], 15 },
    { SDLGLCFG_VAL_STRING, ExtIdsz[11], 15 },
    { SDLGLCFG_VAL_STRING, ExtIdsz[12], 15 },
    { 0 } 
};

/*******************************************************************************
* CODE									                                   *
*******************************************************************************/

/*
 * Name:
 *     spellGetValue
 * Description:
 *     Get value from given string
 * Input:
 *     pset *:   Value descriptor to fill
 *     which:    Which character value
 *     pval *:   Pointer on value to scan  
 */
void spellGetValue(SPELL_SET_T *pset, char which, char *pval)
{
    int val;
    
    
    pset->which = which;
    pset->sub_code = 0;      
    sscanf(pval, "%d", &val);
    pset->value = (short int)val;
}

/*
 * Name:
 *     spellGetModifier
 * Description:
 *     Get modifier from given string
 * Input:
 *     pset *:   Modifier-Description to fill
 *     sub_code: Damage type 
 *     inv_type: Inversion-Code
 *     pval *:   Pointer on value to scan  
 */
static void spellGetModifier(SPELL_SET_T *pset, char sub_code, char inv_type, char *pval)
{
    int val;
    
    
    pset->which = CHAR_VAL_DMGMOD;
    pset->sub_code = sub_code;  // Slash
    pset->inv_type = inv_type;
    sscanf(pval, "%d", &val);
    pset->value = (short int)val;
}

/* ========================================================================== */
/* ============================= PUBLIC FUNCTION(S) ========================= */
/* ========================================================================== */

/*
 * Name:
 *     spellInit
 * Description:
 *     Resets the buffers and indices for the spells
 * Input:
 *     None
 */
void spellInit(void)
{
    Spell_Idx = 1;
    SpellSet_Idx = 0;
    SpellAdd_Idx = 0;
}

/*
 * Name:
 *     spellLoad
 * Description:
 *     Loads the spell from the actual set directory for an object
 * Input:
 *     None 
 * Output:
 *     >0: Number of spell
 */
int spellLoad(void)
{
    char s1[10], s2[10], s3[10];
    float fval;
    int i;
    char num_val;
    char *fname;
    SPELL_T *pspell;
    IDSZ_T loc_idsz;


    fname = egofileMakeFileName(EGOFILE_ACTOBJDIR, "enchant.txt");

    if(! sdlglcfgEgobooValues(fname, SpellVal, 0))
    {
        // No enchant file found
        return 0;
    }


    // Translate the values from strings read
    if(Spell_Idx < MAX_SPELL)
    {
        // We found a free slot
        pspell = &Spells[Spell_Idx];
        // Now get the booleans
        pspell->flags = 0;
        for(i = 0; i < MAX_BOOL_VALUES; i++)
        {
            pspell->flags |=  (1 << i);
        }
 
        // Removed by IDSZ
        idszStringtoIDSZ(RemoveIdsz, &loc_idsz, NULL);  
        pspell->removedbyidsz = loc_idsz.idsz;
        
        // Get the set values
        num_val = 0;
        pspell->set_idx = SpellSet_Idx;
        for(i = 0; i < MAX_SET_VALUES; i++)
        {
            // Only add value if true
            if(SetValues[i][0] != 'F')
            {
                // Scan maximum three strings/values
                sscanf(SetValues[i], "%s%s%s", s1, s2, s3);
                
                switch(i)
                {
                    case 0:
                        // Set Damage type
                        SpellSet[SpellSet_Idx].which = CHAR_VAL_DMGTYPE;
                        // @todo: Translate char to type-value by called function ?
                        SpellSet[SpellSet_Idx].sub_code = s2[0];
                        break;
                    case 1:
                        spellGetValue(&SpellSet[SpellSet_Idx], CHAR_VAL_NUMJUMPS, s2); 
                        break;
                    case 2:
                        spellGetValue(&SpellSet[SpellSet_Idx], CHAR_VAL_LIFEBARCOL, s2);                       
                        break;
                    case 3:
                        spellGetValue(&SpellSet[SpellSet_Idx], CHAR_VAL_MANABARCOL, s2);    
                        break;
                    case 4:
                        spellGetModifier(&SpellSet[SpellSet_Idx], DAMAGE_SLASH, s2[0], s3);                        
                        break;
                    case 5:
                        spellGetModifier(&SpellSet[SpellSet_Idx], DAMAGE_CRUSH, s2[0], s3); 
                        break;
                    case 6:
                        spellGetModifier(&SpellSet[SpellSet_Idx], DAMAGE_POKE, s2[0], s3); 
                        break;
                    case 7:
                        spellGetModifier(&SpellSet[SpellSet_Idx], DAMAGE_HOLY, s2[0], s3); 
                        break;
                    case 8:
                        spellGetModifier(&SpellSet[SpellSet_Idx], DAMAGE_EVIL, s2[0], s3); 
                        break;
                    case 9:
                        spellGetModifier(&SpellSet[SpellSet_Idx], DAMAGE_FIRE, s2[0], s3); 
                        break;
                    case 10:
                        spellGetModifier(&SpellSet[SpellSet_Idx], DAMAGE_ICE, s2[0], s3); 
                        break;
                    case 11:
                        spellGetModifier(&SpellSet[SpellSet_Idx], DAMAGE_ZAP, s2[0], s3); 
                        break;
                    // Graphics
                    case 12:
                        // Flashing AND
                        spellGetValue(&SpellSet[SpellSet_Idx], CHAR_VAL_FLASHAND, s2);
                        break;
                    case 13:
                        // Light blending 
                        spellGetValue(&SpellSet[SpellSet_Idx], CHAR_VAL_LIGHTBLEND, s2);
                        break;
                    case 14:
                        // Alpha blending
                        spellGetValue(&SpellSet[SpellSet_Idx], CHAR_VAL_ALPHABLEND, s2);
                        break;
                    case 15:
                        // Sheen
                        spellGetValue(&SpellSet[SpellSet_Idx], CHAR_VAL_SHEEN, s2);
                        break;
                    case 16:
                        // Fly to height
                        spellGetValue(&SpellSet[SpellSet_Idx], CHAR_VAL_FLYHEIGHT, s2);
                        break;
                    case 17:
                        // Walk on water
                        SpellSet[SpellSet_Idx].which = CHAR_VAL_WALKWATER;
                        SpellSet[SpellSet_Idx].value = 1;
                        break;
                    case 18:
                        // Can see invisible
                        SpellSet[SpellSet_Idx].which = CHAR_VAL_SEEINVISI;
                        SpellSet[SpellSet_Idx].value = 1;            
                        break;
                    case 19:
                        // Missile treatment 
                        SpellSet[SpellSet_Idx].which = CHAR_VAL_MISSILETREAT;
                        SpellSet[SpellSet_Idx].sub_code = s2[0];    
                        break;
                    case 20:
                        // Cost for each missile treated (float)
                        sscanf(s2, "%f", &fval);
                        SpellSet[SpellSet_Idx].which = CHAR_VAL_MISSILECOST;
                        SpellSet[SpellSet_Idx].value = (short int)fval; 
                        break;
                    case 21:
                        // Morph target 
                        SpellSet[SpellSet_Idx].which = CHAR_VAL_MORPH;
                        SpellSet[SpellSet_Idx].value = 1;    
                        break;
                    case 22:
                        // Target may now channel life
                        SpellSet[SpellSet_Idx].which = CHAR_VAL_MANACHANNEL;
                        SpellSet[SpellSet_Idx].value = 1; 
                        break;
                }
                // Count the values to set
                SpellSet_Idx++;
                num_val++;
            }
        }
        // Number of values to set
        pspell->num_set = num_val;
        
        // Get the add values
        num_val = 0;
        pspell->add_idx = SpellAdd_Idx;
        for(i = 0; i < MAX_ADD_VALUES; i++)
        {
            if(AddValues[i] != 0.0)
            {
                // Only set it, if used
                switch(i)
                {
                    case 0:
                        SpellAdd[SpellAdd_Idx].which = CHAR_VAL_JUMPPOWER;                        
                        break;  //  pspell->addvalue[ADDJUMPPOWER]
                    case 1:
                        SpellAdd[SpellAdd_Idx].which = CHAR_VAL_BUMPDAMPEN;   
                        break; //addvalue[ADDBUMPDAMPEN]  / 256.0f; ==> Stored as 8.8-fixed, used as float
                    case 2:
                        SpellAdd[SpellAdd_Idx].which = CHAR_VAL_BOUNCINESS;   
                        break; //  pspell->addvalue[ADDBOUNCINESS]  / 256.0f;    // Stored as 8.8-fixed, used as float
                    case 3:
                        SpellAdd[SpellAdd_Idx].which = CHAR_VAL_DAMAGE;
                        break; //  pspell->addvalue[ADDDAMAGE] * 256.0f;    // Stored as float, used as 8.8-fixed
                    case 4:
                        SpellAdd[SpellAdd_Idx].which = CHAR_VAL_SIZE; 
                        break; // pspell->addvalue[ADDSIZE]     
                    case 5:
                        SpellAdd[SpellAdd_Idx].which = CHAR_VAL_ACCEL; 
                        break;  // pspell->addvalue[ADDACCEL] / 80.0f;     // Stored as int, used as float
                    case 6:
                        SpellAdd[SpellAdd_Idx].which = CHAR_VAL_RED; 
                        break;  //  pspell->addvalue[ADDRED]   
                    case 7:
                        SpellAdd[SpellAdd_Idx].which = CHAR_VAL_GREEN; 
                        break;  // pspell->addvalue[ADDGRN]  
                    case 8:
                        SpellAdd[SpellAdd_Idx].which = CHAR_VAL_BLUE; 
                        break;  // pspell->addvalue[ADDBLU]
                    case 9:
                        SpellAdd[SpellAdd_Idx].which = CHAR_VAL_DEFENSE; 
                        break; // pspell->addvalue[ADDDEFENSE] -fget_next_int( fileread );              // Defense is backwards
                    case 10:
                        SpellAdd[SpellAdd_Idx].which = CHAR_VAL_MANA; 
                        break;  //  pspell->addvalue[ADDMANA]  * 256.0f;    // Stored as float, used as 8.8-fixed
                    case 11:
                        SpellAdd[SpellAdd_Idx].which = CHAR_VAL_LIFE;
                        break;  // pspell->addvalue[ADDLIFE]  * 256.0f;    // Stored as float, used as 8.8-fixed
                    case 12:
                        SpellAdd[SpellAdd_Idx].which = CHAR_VAL_STR;
                        break;  // pspell->addvalue[ADDSTRENGTH]  * 256.0f;    // Stored as float, used as 8.8-fixed
                    case 13:
                        SpellAdd[SpellAdd_Idx].which = CHAR_VAL_WIS;
                        break;  // pspell->addvalue[ADDWISDOM]  * 256.0f;    // Stored as float, used as 8.8-fixed
                    case 14:
                        SpellAdd[SpellAdd_Idx].which = CHAR_VAL_INTEL;
                        break;  // pspell->addvalue[ADDINTELLIGENCE] * 256.0f;    // Stored as float, used as 8.8-fixed
                    case 15:
                        SpellAdd[SpellAdd_Idx].which = CHAR_VAL_DEX;
                        break;  // pspell->addvalue[ADDDEXTERITY] 
                }
                //             
                SpellAdd[SpellAdd_Idx].value = AddValues[i];
                SpellAdd_Idx++;
                num_val++;
            } // if(AddValues[i] != 0)            
        }
        // Number of values to add
        pspell->num_add = num_val;         
        
        // Get the expansions        
        i = 0;
        while(ExtIdsz[i][0] != 0)
        {
            idszStringtoIDSZ(ExtIdsz[i], &loc_idsz, NULL);  
            
            switch(loc_idsz.idsz)
            {
                case 'AMOU':
                    pspell->contspawn_amount = loc_idsz.value;
                    break;
                case 'TYPE':
                    pspell->contspawn_pip = loc_idsz.value;
                    break;
                case 'TIME':
                    pspell->contspawn_timer = loc_idsz.value;
                    break;
                case 'FACE':
                    pspell->contspawn_facingadd = loc_idsz.value;
                    break;
                case 'SEND':
                    pspell->endsound_index = loc_idsz.value;
                    break;
                case 'STAY':
                    EGODEF_SET_FLAG(pspell->flags, SPELL_FSTAYIFNOOWNER); 
                    break;
                case 'OVER':
                    EGODEF_SET_FLAG(pspell->flags, SPELL_FSPAWN_OVERLAY); 
                    break;
                case 'DEAD':
                    EGODEF_SET_FLAG(pspell->flags, SPELL_FSTAYIFTARGETDEAD); 
                    break;
                case 'CKUR':
                    EGODEF_SET_FLAG(pspell->flags, SPELL_FSTAYIFTARGETDEAD); 
                    pspell->seekurse = loc_idsz.value;
                    break;
                case 'DARK':
                    EGODEF_SET_FLAG(pspell->flags, SPELL_FDARKVISION); 
                    pspell->darkvision = loc_idsz.value;
                    break;
            }
            // Next value
            i++;
        }
   
        // Number of this spell
        return Spell_Idx;
    }
    
    return 0;
}