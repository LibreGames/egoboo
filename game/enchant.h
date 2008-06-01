#pragma once

#include "egoboo_types.h"
#include "object.h"

#define MAXEVE                          MAXPROFILE  // One enchant type per model
#define MAXENCHANT                      128         // Number of enchantments

struct GameState_t;

//------------------------------------
//Enchantment variables
//------------------------------------

typedef enum eve_set_e
{
  SETDAMAGETYPE           = 0,
  SETNUMBEROFJUMPS,
  SETLIFEBARCOLOR,
  SETMANABARCOLOR,
  SETSLASHMODIFIER,       //Damage modifiers
  SETCRUSHMODIFIER,
  SETPOKEMODIFIER,
  SETHOLYMODIFIER,
  SETEVILMODIFIER,
  SETFIREMODIFIER,
  SETICEMODIFIER,
  SETZAPMODIFIER,
  SETFLASHINGAND,
  SETLIGHTBLEND,
  SETALPHABLEND,
  SETSHEEN,                //Shinyness
  SETFLYTOHEIGHT,
  SETWALKONWATER,
  SETCANSEEINVISIBLE,
  SETMISSILETREATMENT,
  SETCOSTFOREACHMISSILE,
  SETMORPH,                //Morph character?
  SETCHANNEL,               //Can channel life as mana?
  EVE_SET_COUNT
} EVE_SET;

typedef enum eve_add_e
{
  ADDJUMPPOWER = 0,
  ADDBUMPDAMPEN,
  ADDBOUNCINESS,
  ADDDAMAGE,
  ADDSIZE,
  ADDACCEL,
  ADDRED,             //Red shift
  ADDGRN,             //Green shift
  ADDBLU,             //Blue shift
  ADDDEFENSE,         //Defence adjustments
  ADDMANA,
  ADDLIFE,
  ADDSTRENGTH,
  ADDWISDOM,
  ADDINTELLIGENCE,
  ADDDEXTERITY,
  EVE_ADD_COUNT      // Number of adds
} EVE_ADD;

typedef struct Eve_t
{
  bool_t          used;                        // Enchant.txt loaded?

  bool_t          override;                    // Override other enchants?
  bool_t          removeoverridden;            // Remove other enchants?
  bool_t          setyesno[EVE_SET_COUNT];     // Set this value?
  Uint8           setvalue[EVE_SET_COUNT];     // Value to use
  Sint8           addvalue[EVE_ADD_COUNT];     // The values to add
  bool_t          retarget;                    // Pick a weapon?
  bool_t          killonend;                   // Kill the target on end?
  bool_t          poofonend;                   // Spawn a poof on end?
  bool_t          endifcantpay;                // End on out of mana
  bool_t          stayifnoowner;               // Stay if owner has died?
  Sint16          time;                        // Time in seconds
  Sint8           endmessage;                  // Message for end -1 for none
  Sint16          ownermana_fp8;                   // Boost values
  Sint16          ownerlife_fp8;                   //
  Sint16          targetmana_fp8;                  //
  Sint16          targetlife_fp8;                  //
  DAMAGE          dontdamagetype;              // Don't work if ...
  DAMAGE          onlydamagetype;              // Only work if ...
  IDSZ            removedbyidsz;               // By particle or [NONE]
  Uint16          contspawntime;               // Spawn timer
  Uint8           contspawnamount;             // Spawn amount
  Uint16          contspawnfacingadd;          // Spawn in circle
  Uint16          contspawnpip;                // Spawn type ( local )
  Sint8           endsound;                    // Sound on end (-1 for none)
  Uint16          frequency;                   // Sound frequency
  Uint16          overlay;                     // Spawn an overlay?
  bool_t          canseekurse;                 // Allow target to see kurses?
} Eve;

Eve *  Eve_new(Eve *peve);
bool_t Eve_delete( Eve * peve );
Eve *  Eve_renew( Eve * peve );

typedef struct Enc_t
{
  bool_t          on;                      // Enchantment on
  EVE_REF         eve;                     // The type
  CHR_REF         target;                  // Who it enchants
  ENC_REF         nextenchant;             // Next in the list
  CHR_REF         owner;                   // Who cast the enchant
  CHR_REF         spawner;                 // The spellbook character
  CHR_REF         overlay;                 // The overlay character
  Sint16          ownermana_fp8;           // Boost values
  Sint16          ownerlife_fp8;           //
  Sint16          targetmana_fp8;          //
  Sint16          targetlife_fp8;          //
  bool_t          setyesno[EVE_SET_COUNT]; // Was it set?
  Uint8           setsave[EVE_SET_COUNT];  // The value to restore
  Sint16          addsave[EVE_ADD_COUNT];  // The value to take away
  Sint16          time;                    // Time before end
  float           spawntime;               // Time before spawn
} Enc;

Enc *  Enc_new(Enc *penc);
bool_t Enc_delete( Enc * penc );
Enc *  Enc_renew( Enc * penc );

Uint16 EncList_get_free( struct GameState_t * gs );

typedef enum disenchant_mode_e
{
  LEAVE_ALL   = 0,
  LEAVE_FIRST,
  LEAVE_NONE,
} DISENCHANT_MODE;


void reset_character_alpha( struct GameState_t * gs, CHR_REF character );
void chr_reset_accel( struct GameState_t * gs, CHR_REF character );

void   EveList_load_one( struct GameState_t * gs, char * szObjectpath, char * szObjectname, Uint16 profile );
void   unset_enchant_value( struct GameState_t * gs, Uint16 enchantindex, Uint8 valueindex );
void   remove_enchant_value( struct GameState_t * gs, Uint16 enchantindex, Uint8 valueindex );

void   remove_enchant( struct GameState_t * gs, Uint16 enchantindex );
Uint16 enchant_value_filled( struct GameState_t * gs, Uint16 enchantindex, Uint8 valueindex );
void   set_enchant_value( struct GameState_t * gs, Uint16 enchantindex, Uint8 valueindex, Uint16 enchanttype );
void   getadd( int min, int value, int max, int* valuetoadd );
void   fgetadd( float min, float value, float max, float* valuetoadd );
void   add_enchant_value( struct GameState_t * gs, Uint16 enchantindex, Uint8 valueindex, Uint16 enchanttype );
Uint16 spawn_enchant( struct GameState_t * gs, Uint16 owner, Uint16 target, Uint16 spawner, Uint16 enchantindex, Uint16 modeloptional );

void enc_spawn_particles( struct GameState_t * gs, float dUpdate );
void disenchant_character( struct GameState_t * gs, CHR_REF character );