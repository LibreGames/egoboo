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
/// This "inherits" from ego_obj
struct ego_enc : public ego_enc_data
{
    friend struct ego_obj_enc;

private:
    const ego_obj_enc * _parent_obj_ptr;

public:
    ego_enc_spawn_data  spawn_data;

    explicit ego_enc( ego_obj_enc * _parent ) : _parent_obj_ptr( _parent ) { ego_enc::ctor( this ); }
    ~ego_enc() { ego_enc::dtor( this ); }

    const ego_obj_enc * cget_pparent() const { return _parent_obj_ptr; }
    ego_obj_enc       * get_pparent()  const { return ( ego_obj_enc * )_parent_obj_ptr; }

    static ego_enc * ctor( ego_enc * penc );
    static ego_enc * dtor( ego_enc * penc );

    static ENC_REF value_filled( const ENC_REF & enchant_idx, int value_idx );
    static void    apply_set( const ENC_REF &  enchant_idx, int value_idx, const PRO_REF & profile );
    static void    apply_add( const ENC_REF &  enchant_idx, int value_idx, const EVE_REF & enchanttype );
    static void    remove_set( const ENC_REF &  enchant_idx, int value_idx );
    static void    remove_add( const ENC_REF &  enchant_idx, int value_idx );

    static INLINE PRO_REF   get_ipro( const ENC_REF & ienc );
    static INLINE ego_pro * get_ppro( const ENC_REF & ienc );

    static INLINE CHR_REF   get_iowner( const ENC_REF & ienc );
    static INLINE ego_chr * get_powner( const ENC_REF & ienc );

    static INLINE EVE_REF   get_ieve( const ENC_REF & ienc );
    static INLINE ego_eve * get_peve( const ENC_REF & ienc );

    static INLINE IDSZ      get_idszremove( const ENC_REF & ienc );
    static INLINE bool_t    is_removed( const ENC_REF & ienc, const PRO_REF & test_profile );

protected:
    static ego_enc * alloc( ego_enc * penc );
    static ego_enc * dealloc( ego_enc * penc );

    static ego_enc * do_init( ego_enc * penc );
    static ego_enc * do_active( ego_enc * penc );
    static ego_enc * do_deinit( ego_enc * penc );
};


//--------------------------------------------------------------------------------------------
struct ego_obj_enc : public ego_obj
{
    typedef ego_enc data_type;

    ego_enc & get_enc();
    ego_enc * get_penc();

    ego_obj_enc() : _enc_data( this ) { ctor( this, bfalse ); }
    ~ego_obj_enc() { dtor( this, bfalse ); }

    // This container "has a" ego_enc, so we need some way of accessing it
    // These have to have generic names to that t_ego_obj_lst<> can access the data
    // for all container types
    ego_enc & get_data()  { return _enc_data; }
    ego_enc * get_pdata() { return &_enc_data; }

    const ego_enc & cget_data()  const { return _enc_data; }
    const ego_enc * cget_pdata() const { return &_enc_data; }

    static ego_obj_enc * ctor( ego_obj_enc * pobj, bool_t recursive = btrue ) { if ( NULL == pobj ) return NULL; if ( recursive ) ego_enc::ctor( pobj->get_pdata() ); return pobj; };
    static ego_obj_enc * dtor( ego_obj_enc * pobj, bool_t recursive = btrue ) { if ( NULL == pobj ) return NULL; if ( recursive ) ego_enc::dtor( pobj->get_pdata() ); return pobj; };

    // global enc configuration functions
    static ego_obj_enc * run( ego_obj_enc * penc );
    static ego_obj_enc * run_construct( ego_obj_enc * pprt, int max_iterations );
    static ego_obj_enc * run_initialize( ego_obj_enc * pprt, int max_iterations );
    static ego_obj_enc * run_activate( ego_obj_enc * pprt, int max_iterations );
    static ego_obj_enc * run_deinitialize( ego_obj_enc * pprt, int max_iterations );
    static ego_obj_enc * run_deconstruct( ego_obj_enc * pprt, int max_iterations );

    static bool_t    request_terminate( const ENC_REF & ienc );

    static ego_obj_enc * do_constructing( ego_obj_enc * penc );
    static ego_obj_enc * do_initializing( ego_obj_enc * penc );
    static ego_obj_enc * do_deinitializing( ego_obj_enc * penc );
    static ego_obj_enc * do_processing( ego_obj_enc * penc );
    static ego_obj_enc * do_destructing( ego_obj_enc * penc );

protected:
    ego_enc _enc_data;

};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// Prototypes

void enchant_system_begin();
void enchant_system_end();

void   init_all_eve();
void   release_all_eve();
bool_t release_one_eve( const EVE_REF & ieve );

void    update_all_enchants();
void    cleanup_all_enchants();

void    increment_all_enchant_update_counters( void );
bool_t  remove_enchant( const ENC_REF &  enchant_idx, ENC_REF *  enchant_parent );
bool_t  remove_all_enchants_with_idsz( CHR_REF ichr, IDSZ remove_idsz );

ENC_REF spawn_one_enchant( const CHR_REF & owner, const CHR_REF & target, const CHR_REF & spawner, const ENC_REF & ego_enc_override, const PRO_REF & modeloptional );
EVE_REF load_one_enchant_profile_vfs( const char* szLoadName, const EVE_REF & profile );
ENC_REF cleanup_enchant_list( const ENC_REF & ienc, ENC_REF * ego_enc_parent );

#define  remove_all_character_enchants( ICHR ) remove_all_enchants_with_idsz( ICHR, IDSZ_NONE )

#define _enchant_h
