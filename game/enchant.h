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

/// @file enchant.h
/// @details Decleares some stuff used for handling enchants

#include "egoboo_object.h"

#include "egoboo.h"

#include "file_formats/eve_file.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct ego_pro;
struct ego_chr;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define ENC_LEAVE_ALL                0
#define ENC_LEAVE_FIRST              1
#define ENC_LEAVE_NONE               2

#define MAX_EVE                 MAX_PROFILE    ///< One enchant type per model
#define MAX_ENC                 200            ///< Number of enchantments

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct ego_eve_data : public s_eve_data
{
    static ego_eve_data * init( ego_eve_data * ptr ) { s_eve_data * rv = eve_data_init( ptr ); return NULL == rv ? NULL : ptr; }
};

struct ego_eve : public ego_eve_data
{
    static ego_eve * init( ego_eve * ptr ) { ego_eve_data * rv = ego_eve_data::init( ptr ); return NULL == rv ? NULL : ptr; }
};


/// Enchantment template
DECLARE_STACK_EXTERN( ego_eve, EveStack, MAX_EVE );

#define VALID_EVE_RANGE( IEVE ) ( ((IEVE) >= 0) && ((IEVE) < MAX_EVE) )
#define LOADED_EVE( IEVE )      ( VALID_EVE_RANGE( IEVE ) && EveStack.lst[IEVE].loaded )

//--------------------------------------------------------------------------------------------
struct ego_enc_spawn_data
{
    CHR_REF owner_ref;
    CHR_REF target_ref;
    CHR_REF spawner_ref;
    PRO_REF profile_ref;
    EVE_REF eve_ref;
};

//--------------------------------------------------------------------------------------------

/// The definition of a Egoboo enchantment's data
struct ego_enc_data
{
    int     time;                    ///< Time before end
    int     spawntime;               ///< Time before spawn

    PRO_REF profile_ref;             ///< The object  profile index that spawned this enchant
    EVE_REF eve_ref;                 ///< The enchant profile index

    CHR_REF target_ref;              ///< Who it enchants
    CHR_REF owner_ref;               ///< Who cast the enchant
    CHR_REF spawner_ref;             ///< The spellbook character
    PRO_REF spawnermodel_ref;        ///< The spellbook character's CapStack index
    CHR_REF overlay_ref;             ///< The overlay character

    int     owner_mana;               ///< Boost values
    int     owner_life;
    int     target_mana;
    int     target_life;

    ENC_REF nextenchant_ref;             ///< Next in the list

    bool_t  setyesno[MAX_ENCHANT_SET];  ///< Was it set?
    float   setsave[MAX_ENCHANT_SET];   ///< The value to restore

    bool_t  addyesno[MAX_ENCHANT_ADD];  ///< Was the value adjusted
    float   addsave[MAX_ENCHANT_ADD];   ///< The adjustment
};


//--------------------------------------------------------------------------------------------

/// The definition of a single Egoboo enchantment
/// This "inherits" from ego_object
struct ego_enc : public ego_enc_data
{
    ego_object obj_base;

    ego_enc_spawn_data  spawn_data;

    ego_enc()  { ego_enc::ctor( this ); };
    ~ego_enc() { ego_enc::dtor( this ); };

    static ego_enc * ctor( ego_enc * penc );
    static ego_enc * dtor( ego_enc * penc );

    static ENC_REF value_filled( const ENC_REF by_reference enchant_idx, int value_idx );
    static void    apply_set( const ENC_REF by_reference  enchant_idx, int value_idx, const PRO_REF by_reference profile );
    static void    apply_add( const ENC_REF by_reference  enchant_idx, int value_idx, const EVE_REF by_reference enchanttype );
    static void    remove_set( const ENC_REF by_reference  enchant_idx, int value_idx );
    static void    remove_add( const ENC_REF by_reference  enchant_idx, int value_idx );

    static bool_t request_terminate( const ENC_REF by_reference  ienc );

    static ego_enc * run_object( ego_enc * penc );

    static ego_enc * run_object_construct( ego_enc * penc, int max_iterations );
    static ego_enc * run_object_initialize( ego_enc * penc, int max_iterations );
    static ego_enc * run_object_activate( ego_enc * penc, int max_iterations );
    static ego_enc * run_object_deinitialize( ego_enc * penc, int max_iterations );
    static ego_enc * run_object_deconstruct( ego_enc * penc, int max_iterations );

    static INLINE PRO_REF   get_ipro( const ENC_REF by_reference ienc );
    static INLINE ego_pro * get_ppro( const ENC_REF by_reference ienc );

    static INLINE CHR_REF   get_iowner( const ENC_REF by_reference ienc );
    static INLINE ego_chr * get_powner( const ENC_REF by_reference ienc );

    static INLINE EVE_REF   get_ieve( const ENC_REF by_reference ienc );
    static INLINE ego_eve * get_peve( const ENC_REF by_reference ienc );

    static INLINE IDSZ      get_idszremove( const ENC_REF by_reference ienc );
    static INLINE bool_t    is_removed( const ENC_REF by_reference ienc, const PRO_REF by_reference test_profile );

private:
    static bool_t    dealloc( ego_enc * penc );

    static ego_enc * do_object_constructing( ego_enc * penc );
    static ego_enc * do_object_initializing( ego_enc * penc );
    static ego_enc * do_object_deinitializing( ego_enc * penc );
    static ego_enc * do_object_processing( ego_enc * penc );
    static ego_enc * do_object_destructing( ego_enc * penc );

    static ego_enc * do_init( ego_enc * penc );
    static ego_enc * do_active( ego_enc * penc );
    static ego_enc * do_deinit( ego_enc * penc );
    
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// Prototypes

void enchant_system_begin();
void enchant_system_end();

void   init_all_eve();
void   release_all_eve();
bool_t release_one_eve( const EVE_REF by_reference ieve );

void    update_all_enchants();
void    cleanup_all_enchants();

void    increment_all_enchant_update_counters( void );
bool_t  remove_enchant( const ENC_REF by_reference  enchant_idx, ENC_REF *  enchant_parent );
bool_t  remove_all_enchants_with_idsz( CHR_REF ichr, IDSZ remove_idsz );

ENC_REF spawn_one_enchant( const CHR_REF by_reference owner, const CHR_REF by_reference target, const CHR_REF by_reference spawner, const ENC_REF by_reference ego_enc_override, const PRO_REF by_reference modeloptional );
EVE_REF load_one_enchant_profile_vfs( const char* szLoadName, const EVE_REF by_reference profile );
ENC_REF cleanup_enchant_list( const ENC_REF by_reference ienc, ENC_REF * ego_enc_parent );

#define  remove_all_character_enchants( ICHR ) remove_all_enchants_with_idsz( ICHR, IDSZ_NONE )

#define ENCHANT_H