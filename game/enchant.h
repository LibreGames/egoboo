#pragma once

//********************************************************************************************
//*
//*    This file is part of Egoboo.
//*
//*    Egoboo is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Egoboo is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

///
/// @file
/// @brief Enchantment Definitions.
/// @details Definitions needed to manage the egoboo buff/debuf dydtem, called enchantments.

#include "egoboo_types.h"
#include "object.h"

#define EVELST_COUNT                          OBJLST_COUNT   ///< One enchant type per model
#define ENCLST_COUNT                      128          ///< Number of enchantments

struct sGame;

//------------------------------------
//Enchantment variables
//------------------------------------

enum e_eve_set
{
  SETDAMAGETYPE           = 0,
  SETNUMBEROFJUMPS,
  SETLIFEBARCOLOR,
  SETMANABARCOLOR,
  SETSLASHMODIFIER,       ///< Damage modifiers
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
  SETSHEEN,                ///< Shinyness
  SETFLYTOHEIGHT,
  SETWALKONWATER,
  SETCANSEEINVISIBLE,
  SETMISSILETREATMENT,
  SETCOSTFOREACHMISSILE,
  SETMORPH,                 ///< Morph character?
  SETCHANNEL,               ///< Can channel life as mana?
  EVE_SET_COUNT
};
typedef enum e_eve_set EVE_SET;

enum e_eve_add
{
  ADDJUMPPOWER = 0,
  ADDBUMPDAMPEN,
  ADDBOUNCINESS,
  ADDDAMAGE,
  ADDSIZE,
  ADDACCEL,
  ADDRED,             ///< Red shift
  ADDGRN,             ///< Green shift
  ADDBLU,             ///< Blue shift
  ADDDEFENSE,         ///< Defence adjustments
  ADDMANA,
  ADDLIFE,
  ADDSTRENGTH,
  ADDWISDOM,
  ADDINTELLIGENCE,
  ADDDEXTERITY,
  EVE_ADD_COUNT      ///< Number of adds
};
typedef enum e_eve_add EVE_ADD;

struct sEve
{
  egoboo_key_t    ekey;
  STRING          loadname;
  bool_t          Loaded;                      ///< Enchant.txt loaded?

  bool_t          override;                    ///< Override other enchants?
  bool_t          removeoverridden;            ///< Remove other enchants?
  bool_t          setyesno[EVE_SET_COUNT];     ///<Set this value?
  Uint8           setvalue[EVE_SET_COUNT];     ///< Value to use
  Sint8           addvalue[EVE_ADD_COUNT];     ///< The values to add
  bool_t          retarget;                    ///< Pick a weapon?
  bool_t          killonend;                   ///< Kill the target on end?
  bool_t          poofonend;                   ///< Spawn a poof on end?
  bool_t          endifcantpay;                ///< End on out of mana
  bool_t          stayifnoowner;               ///< Stay if owner has died?
  Sint16          time;                        ///< Time in seconds
  Sint8           endmessage;                  ///< Message for end -1 for none
  Sint16          ownermana_fp8;               ///< Boost values
  Sint16          ownerlife_fp8;               //
  Sint16          targetmana_fp8;              //
  Sint16          targetlife_fp8;              //
  DAMAGE          dontdamagetype;              ///< Don't work if ...
  DAMAGE          onlydamagetype;              ///< Only work if ...
  IDSZ            removedbyidsz;               ///< By particle or [NONE]
  Uint16          contspawntime;               ///< Spawn timer
  Uint8           contspawnamount;             ///< Spawn amount
  Uint16          contspawnfacingadd;          ///< Spawn in circle
  PIP_REF         contspawnpip;                ///< Spawn type ( local )
  Sint8           endsound;                    ///< Sound on end (-1 for none)
  Uint16          frequency;                   ///< Sound frequency
  Uint16          overlay;                     ///< Spawn an overlay?
  bool_t          canseekurse;                 ///< Allow target to see kurses?
};
typedef struct sEve Eve_t;

#ifdef __cplusplus
  typedef TList<sEve, EVELST_COUNT> EveList_t;
  typedef TPList<sEve, EVELST_COUNT> PEve_t;
#else
  typedef Eve_t EveList_t[EVELST_COUNT];
  typedef Eve_t * PEve_t;
#endif

Eve_t *  Eve_new(Eve_t *peve);
bool_t Eve_delete( Eve_t * peve );
Eve_t *  Eve_renew( Eve_t * peve );

#define VALID_EVE_RANGE(XX)   ( /*(((XX)>=0) && */ ((XX)<EVELST_COUNT) )
#define VALID_EVE(LST, XX)    ( VALID_EVE_RANGE(XX) && EKEY_VALID(LST[XX]) )
#define VALIDATE_EVE(LST, XX) ( VALID_EVE(LST, XX) ? (XX) : (INVALID_EVE) )
#define LOADED_EVE(LST, XX)   ( VALID_EVE(LST, XX) && LST[XX].Loaded )

struct s_enc_spawn_info
{
  egoboo_key_t ekey;
  struct sGame * gs;

  Uint32 seed;
  ENC_REF ienc;

  CHR_REF owner;
  CHR_REF target;
  CHR_REF spawner;
  ENC_REF enchantindex;
  OBJ_REF iobj;
  EVE_REF ieve;

};
typedef struct s_enc_spawn_info ENC_SPAWN_INFO;

ENC_SPAWN_INFO * enc_spawn_info_new(ENC_SPAWN_INFO * psi, struct sGame * gs);

struct sEnc
{
  egoboo_key_t      ekey;

  bool_t          reserved;         ///< Is it going to be used?
  bool_t          req_active;      ///< Are we going to auto-activate ASAP?
  bool_t          active;          ///< Is it currently on?
  bool_t          gopoof;          ///< Is poof requested?
  bool_t          freeme;          ///< Is EncList_free_one() requested?

  ENC_SPAWN_INFO  spinfo;

  OBJ_REF         profile;                 ///< The profile that the eve came from
  EVE_REF         eve;                     ///< The eve

  CHR_REF         target;                  ///< Who it enchants
  ENC_REF         nextenchant;             ///< Next in the list
  CHR_REF         owner;                   ///< Who cast the enchant
  CHR_REF         spawner;                 ///< The spellbook character
  CHR_REF         overlay;                 ///< The overlay character
  Sint16          ownermana_fp8;           ///< Boost values
  Sint16          ownerlife_fp8;           //
  Sint16          targetmana_fp8;          //
  Sint16          targetlife_fp8;          //
  bool_t          setyesno[EVE_SET_COUNT]; ///< Was it set?
  Uint8           setsave[EVE_SET_COUNT];  ///< The value to restore
  Sint16          addsave[EVE_ADD_COUNT];  ///< The value to take away
  Sint16          time;                    ///< Time before end
  float           spawntime;               ///< Time before spawn
};
typedef struct sEnc Enc_t;

#ifdef __cplusplus
  typedef TList<sEnc, ENCLST_COUNT> EncList_t;
  typedef TPList<sEnc, ENCLST_COUNT> PEnc_t;
#else
  typedef Enc_t EncList_t[ENCLST_COUNT];
  typedef Enc_t * PEnc_t;
#endif

Enc_t *  Enc_new(Enc_t *penc);
bool_t  Enc_delete( Enc_t * penc );
Enc_t *  Enc_renew( Enc_t * penc );

struct sEncHeap
{
  egoboo_key_t ekey;

  int       free_count;
  ENC_REF   free_list[ENCLST_COUNT];

  int       used_count;
  ENC_REF   used_list[ENCLST_COUNT];
};

typedef struct sEncHeap EncHeap_t;

EncHeap_t * EncHeap_new   ( EncHeap_t * pheap );
bool_t      EncHeap_delete( EncHeap_t * pheap );
EncHeap_t * EncHeap_renew ( EncHeap_t * pheap );
bool_t      EncHeap_reset ( EncHeap_t * pheap );

ENC_REF EncHeap_getFree( EncHeap_t * pheap, ENC_REF request );
ENC_REF EncHeap_iterateUsed( EncHeap_t * pheap, int * index );
bool_t  EncHeap_addUsed( EncHeap_t * pheap, ENC_REF ref );
bool_t  EncHeap_addFree( EncHeap_t * pheap, ENC_REF ref );

PROFILE_PROTOTYPE( EncHeap )

ENC_REF EncList_get_free( struct sGame * gs, ENC_REF irequest);

#define VALID_ENC_RANGE(XX)   ( /*(((XX)>=0) && */ ((XX)<ENCLST_COUNT) )
#define VALID_ENC(LST, XX)    ( VALID_ENC_RANGE(XX) && EKEY_VALID(LST[XX]) )
#define VALIDATE_ENC(LST, XX) ( VALID_ENC(LST, XX) ? (XX) : (INVALID_ENC) )
#define RESERVED_ENC(LST, XX) ( VALID_ENC(LST, XX) && LST[XX].reserved && !LST[XX].active   )
#define ACTIVE_ENC(LST, XX)   ( VALID_ENC(LST, XX) && LST[XX].active   && !LST[XX].reserved )
#define SEMIACTIVE_ENC(LST, XX)  ( VALID_ENC(LST, XX) && (LST[XX].active || LST[XX].req_active) && !LST[XX].reserved )
#define PENDING_ENC(LST, XX)  ( VALID_ENC(LST, XX) && LST[XX].req_active && !LST[XX].reserved )


void EncList_resynch( struct sGame * gs );

enum e_disenchant_mode
{
  LEAVE_ALL   = 0,
  LEAVE_FIRST,
  LEAVE_NONE
};
typedef enum e_disenchant_mode DISENCHANT_MODE;

extern STRING namingnames;   ///< The name returned by the function


void chr_reset_alpha( struct sGame * gs, CHR_REF character );

/// @details This function fixes a character's MAX acceleration
void chr_reset_accel( struct sGame * gs, CHR_REF character );

EVE_REF EveList_load_one( struct sGame * gs, EGO_CONST char * szObjectpath, EGO_CONST char * szObjectname, EVE_REF irequest );
bool_t  EveList_save_one( struct sGame * gs, EGO_CONST char * szFilename, EVE_REF ieve );
void    unset_enchant_value( struct sGame * gs, ENC_REF enchantindex, Uint8 valueindex );
void    remove_enchant_value( struct sGame * gs, ENC_REF enchantindex, Uint8 valueindex );

void   remove_enchant( struct sGame * gs, ENC_REF enchantindex );
ENC_REF enchant_value_filled( struct sGame * gs, ENC_REF enchantindex, Uint8 valueindex );
void   set_enchant_value( struct sGame * gs, ENC_REF enchantindex, Uint8 valueindex, EVE_REF enchanttype );
void   getadd( int min, int value, int max, int* valuetoadd );
void   fgetadd( float min, float value, float max, float* valuetoadd );
void   add_enchant_value( struct sGame * gs, ENC_REF enchantindex, Uint8 valueindex, EVE_REF enchanttype );



ENC_REF enc_spawn_info_init( ENC_SPAWN_INFO * psi, struct sGame * gs, CHR_REF owner, CHR_REF target, CHR_REF spawner, ENC_REF enchantindex, OBJ_REF modeloptional );
ENC_REF req_spawn_one_enchant( ENC_SPAWN_INFO si );

void enc_spawn_particles( struct sGame * gs, float dUpdate );
void disenchant_character( struct sGame * gs, CHR_REF character );
