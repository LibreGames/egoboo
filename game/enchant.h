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

/// The definition of the enchant data
/// This has been separated from ego_enc so that it can be initialized easily without upsetting anuthing else
struct ego_enc_data
{
    //---- constructors and destructors

    /// default constructor
    explicit ego_enc_data()  { ego_enc_data::ctor( this ); };

    /// default destructor
    ~ego_enc_data() { ego_enc_data::dtor( this ); };

    //---- construction and destruction

    /// construct this struct, ONLY
    static ego_enc_data * ctor( ego_enc_data * );
    /// destruct this struct, ONLY
    static ego_enc_data * dtor( ego_enc_data * );
    /// do some initialization
    static ego_enc_data * init( ego_enc_data * );

    //---- generic enchant data

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

protected:

    //---- construction and destruction

    /// construct this struct, and ALL dependent structs
    static ego_enc_data * do_ctor( ego_enc_data * ptr ) { return ego_enc_data::ctor( ptr ); };
    /// denstruct this struct, and ALL dependent structs
    static ego_enc_data * do_dtor( ego_enc_data * ptr ) { return ego_enc_data::dtor( ptr ); };

    //---- memory management

    /// allocate data for this struct, ONLY
    static ego_enc_data * alloc( ego_enc_data * pdata ) { return pdata; };
    /// deallocate data for this struct, ONLY
    static ego_enc_data * dealloc( ego_enc_data * pdata ) { return pdata; };

    /// allocate data for this struct, and ALL dependent structs
    static ego_enc_data * do_alloc( ego_enc_data * pdata ) { return pdata; };
    /// deallocate data for this struct, and ALL dependent structs
    static ego_enc_data * do_dealloc( ego_enc_data * pdata ) { return pdata; };
};

//--------------------------------------------------------------------------------------------

/// The definition of the enchant
/// This encapsulates all the enchant functions and some extra data
struct ego_enc : public ego_enc_data
{
    friend struct ego_obj_enc;

public:
    ego_enc_spawn_data  spawn_data;

    //---- constructors and destructors

    /// non-default constructor. We MUST know who our marent is
    explicit ego_enc( ego_obj_enc * _pparent ) : _parent_obj_ptr( _pparent ) { ego_enc::ctor( this ); };

    /// default destructor
    ~ego_enc() { ego_enc::dtor( this ); };

    //---- implementation of required accessors

    // This ego_enc is contained by ego_che_obj. We need some way of accessing it
    // These have to have generic names to that all objects that are contained in
    // an ego_object can be interfaced with in the same way

    const ego_obj_enc * cget_pparent() const { return _parent_obj_ptr; }
    ego_obj_enc       * get_pparent()  const { return ( ego_obj_enc * )_parent_obj_ptr; }

    //---- construction and destruction

    /// construct this struct, ONLY
    static ego_enc * ctor( ego_enc * penc );
    /// destruct this struct, ONLY
    static ego_enc * dtor( ego_enc * penc );
    /// do some initialization
    static ego_enc * init( ego_enc * penc ) { return penc; };

    //---- generic enchant functions

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

    //---- memory management

    /// allocate data for this struct, ONLY
    static ego_enc * alloc( ego_enc * penc );
    /// deallocate data for this struct, ONLY
    static ego_enc * dealloc( ego_enc * penc );

    /// allocate data for this struct, and ALL dependent structs
    static ego_enc * do_alloc( ego_enc * penc );
    /// deallocate data for this struct, and ALL dependent structs
    static ego_enc * do_dealloc( ego_enc * penc );

    //---- private implementations of the configuration functions

    static ego_enc * do_ctor( ego_enc * penc );
    static ego_enc * do_init( ego_enc * penc );
    static ego_enc * do_process( ego_enc * penc );
    static ego_enc * do_deinit( ego_enc * penc );
    static ego_enc * do_dtor( ego_enc * penc );

private:

    /// a hook to ego_obj_enc, which is the parent container of this object
    const ego_obj_enc * _parent_obj_ptr;
};

//--------------------------------------------------------------------------------------------

/// A refinement of the ego_obj and the actual container that will be stored in the t_cpp_list<>
/// This adds functions for implementing the state machine on this object (the run* and do* functions)
/// and encapsulates the enchant data

struct ego_obj_enc : public ego_obj
{
    //---- typedefs

    /// a type definition so that parent struct t_ego_obj_lst<> can define a function
    /// to get at the actual enchant data. For instance EncObjList.get_pdata() to return
    /// a pointer to the enchant data
    typedef ego_enc data_type;

    //---- constructors and destructors

    /// default constructor
    explicit ego_obj_enc() : _enc_data( this ) { ctor( this ); }

    /// default destructor
    ~ego_obj_enc() { dtor( this ); }

    //---- implementation of required accessors

    // This container "has a" ego_enc, so we need some way of accessing it
    // These have to have generic names to that t_ego_obj_lst<> can access the data
    // for all container types

    ego_enc & get_data()  { return _enc_data; }
    ego_enc * get_pdata() { return &_enc_data; }

    const ego_enc & cget_data()  const { return _enc_data; }
    const ego_enc * cget_pdata() const { return &_enc_data; }

    //---- construction and destruction

    /// construct this struct, ONLY
    static ego_obj_enc * ctor( ego_obj_enc * pobj );
    /// destruct this struct, ONLY
    static ego_obj_enc * dtor( ego_obj_enc * pobj );

    /// construct this struct, and ALL dependent structs
    static ego_obj_enc * do_ctor( ego_obj_enc * pobj );
    /// destruct this struct, and ALL dependent structs
    static ego_obj_enc * do_dtor( ego_obj_enc * pobj );

    //---- global configuration functions

    /// External handle for iterating the "egoboo object process" state machine
    static ego_obj_enc * run( ego_obj_enc * penc );
    /// External handle for getting an "egoboo object process" into the constructed state
    static ego_obj_enc * run_construct( ego_obj_enc * pprt, int max_iterations );
    /// External handle for getting an "egoboo object process" into the initialized state
    static ego_obj_enc * run_initialize( ego_obj_enc * pprt, int max_iterations );
    /// External handle for getting an "egoboo object process" into the active state
    static ego_obj_enc * run_activate( ego_obj_enc * pprt, int max_iterations );
    /// External handle for getting an "egoboo object process" into the deinitialized state
    static ego_obj_enc * run_deinitialize( ego_obj_enc * pprt, int max_iterations );
    /// External handle for getting an "egoboo object process" into the deconstructed state
    static ego_obj_enc * run_deconstruct( ego_obj_enc * pprt, int max_iterations );

    /// Ask the "egoboo object process" to terminate itself
    static bool_t        request_terminate( const ENC_REF & ienc );

protected:

    //---- memory management

    /// allocate data for this struct, ONLY
    static ego_obj_enc * alloc( ego_obj_enc * pobj );
    /// deallocate data for this struct, ONLY
    static ego_obj_enc * dealloc( ego_obj_enc * pobj );

    /// allocate data for this struct, and ALL dependent structs
    static ego_obj_enc * do_alloc( ego_obj_enc * pobj );
    /// deallocate data for this struct, and ALL dependent structs
    static ego_obj_enc * do_dealloc( ego_obj_enc * pobj );

    //---- private implementations of the configuration functions

    /// private implementation of egoboo "egoboo object process's" constructing method
    static ego_obj_enc * do_constructing( ego_obj_enc * penc );
    /// private implementation of egoboo "egoboo object process's" initializing method
    static ego_obj_enc * do_initializing( ego_obj_enc * penc );
    /// private implementation of egoboo "egoboo object process's" deinitializing method
    static ego_obj_enc * do_deinitializing( ego_obj_enc * penc );
    /// private implementation of egoboo "egoboo object process's" processing method
    static ego_obj_enc * do_processing( ego_obj_enc * penc );
    /// private implementation of egoboo "egoboo object process's" destructing method
    static ego_obj_enc * do_destructing( ego_obj_enc * penc );

private:
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
