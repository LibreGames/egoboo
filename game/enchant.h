#pragma once

#include "egoboo_types.h"
#include "object.h"

#define EVELST_COUNT                          OBJLST_COUNT  // One enchant type per model
#define ENCLST_COUNT                      128         // Number of enchantments

struct CGame_t;

//------------------------------------
//Enchantment variables
//------------------------------------

enum eve_set_e
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
};
typedef enum eve_set_e EVE_SET;

enum eve_add_e
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
};
typedef enum eve_add_e EVE_ADD;

struct CEve_t
{
  egoboo_key      ekey;
  bool_t          Loaded;                      // Enchant.txt loaded?

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
  PIP_REF         contspawnpip;                // Spawn type ( local )
  Sint8           endsound;                    // Sound on end (-1 for none)
  Uint16          frequency;                   // Sound frequency
  Uint16          overlay;                     // Spawn an overlay?
  bool_t          canseekurse;                 // Allow target to see kurses?
};
typedef struct CEve_t CEve;

#ifdef __cplusplus
  typedef TList<CEve_t, EVELST_COUNT> EveList_t;
  typedef TPList<CEve_t, EVELST_COUNT> PEve;
#else
  typedef CEve EveList_t[EVELST_COUNT];
  typedef CEve * PEve;
#endif

CEve *  Eve_new(CEve *peve);
bool_t Eve_delete( CEve * peve );
CEve *  Eve_renew( CEve * peve );

#define VALID_EVE_RANGE(XX) (((XX)>=0) && ((XX)<EVELST_COUNT))
#define VALID_EVE(LST, XX)    ( VALID_EVE_RANGE(XX) && EKEY_VALID(LST[XX]) )
#define VALIDATE_EVE(LST, XX) ( VALID_EVE(LST, XX) ? (XX) : (INVALID_EVE) )
#define LOADED_EVE(LST, XX)  ( VALID_EVE(LST, XX) && LST[XX].Loaded )

struct enc_spawn_info_t
{
  egoboo_key ekey;
  struct CGame_t * gs;

  Uint32 seed;
  ENC_REF ienc;

  CHR_REF owner;
  CHR_REF target;
  CHR_REF spawner;
  ENC_REF enchantindex;
  OBJ_REF iobj;
  EVE_REF ieve;

};
typedef struct enc_spawn_info_t enc_spawn_info;

enc_spawn_info * enc_spawn_info_new(enc_spawn_info * psi, struct CGame_t * gs);

typedef struct CEnc_t
{
  egoboo_key      ekey;

  bool_t          reserved;         // Is it going to be used?
  bool_t          req_active;      // Are we going to auto-activate ASAP?
  bool_t          active;          // Is it currently on?
  bool_t          gopoof;          // Is poof requested?
  bool_t          freeme;          // Is EncList_free_one() requested?

  enc_spawn_info  spinfo;

  OBJ_REF         profile;                 // The profile that the eve came from
  EVE_REF         eve;                     // The eve

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
} CEnc;

#ifdef __cplusplus
  typedef TList<CEnc_t, ENCLST_COUNT> EncList_t;
  typedef TPList<CEnc_t, ENCLST_COUNT> PEnc;
#else
  typedef CEnc EncList_t[ENCLST_COUNT];
  typedef CEnc * PEnc;
#endif

CEnc *  CEnc_new(CEnc *penc);
bool_t  CEnc_delete( CEnc * penc );
CEnc *  CEnc_renew( CEnc * penc );

ENC_REF EncList_get_free( struct CGame_t * gs, ENC_REF irequest);

#define VALID_ENC_RANGE(XX)   (((XX)>=0) && ((XX)<ENCLST_COUNT))
#define VALID_ENC(LST, XX)    ( VALID_ENC_RANGE(XX) && EKEY_VALID(LST[XX]) )
#define VALIDATE_ENC(LST, XX) ( VALID_ENC(LST, XX) ? (XX) : (INVALID_ENC) )
#define RESERVED_ENC(LST, XX) ( VALID_ENC(LST, XX) && LST[XX].reserved && !LST[XX].active   )
#define ACTIVE_ENC(LST, XX)   ( VALID_ENC(LST, XX) && LST[XX].active   && !LST[XX].reserved )
#define PENDING_ENC(LST, XX)  ( VALID_ENC(LST, XX) && (LST[XX].active || LST[XX].req_active) && !LST[XX].reserved )


void EncList_resynch( struct CGame_t * gs );

enum disenchant_mode_e
{
  LEAVE_ALL   = 0,
  LEAVE_FIRST,
  LEAVE_NONE,
};
typedef enum disenchant_mode_e DISENCHANT_MODE;

extern STRING namingnames;   // The name returned by the function


void reset_character_alpha( struct CGame_t * gs, CHR_REF character );
void chr_reset_accel( struct CGame_t * gs, CHR_REF character );

EVE_REF EveList_load_one( struct CGame_t * gs, const char * szObjectpath, const char * szObjectname, EVE_REF irequest );
void    unset_enchant_value( struct CGame_t * gs, ENC_REF enchantindex, Uint8 valueindex );
void    remove_enchant_value( struct CGame_t * gs, ENC_REF enchantindex, Uint8 valueindex );

void   remove_enchant( struct CGame_t * gs, ENC_REF enchantindex );
ENC_REF enchant_value_filled( struct CGame_t * gs, ENC_REF enchantindex, Uint8 valueindex );
void   set_enchant_value( struct CGame_t * gs, ENC_REF enchantindex, Uint8 valueindex, EVE_REF enchanttype );
void   getadd( int min, int value, int max, int* valuetoadd );
void   fgetadd( float min, float value, float max, float* valuetoadd );
void   add_enchant_value( struct CGame_t * gs, ENC_REF enchantindex, Uint8 valueindex, EVE_REF enchanttype );



ENC_REF enc_spawn_info_init( enc_spawn_info * psi, struct CGame_t * gs, CHR_REF owner, CHR_REF target, CHR_REF spawner, ENC_REF enchantindex, OBJ_REF modeloptional );
ENC_REF req_spawn_one_enchant( enc_spawn_info si );

void enc_spawn_particles( struct CGame_t * gs, float dUpdate );
void disenchant_character( struct CGame_t * gs, CHR_REF character );
