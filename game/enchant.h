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

#include "egoboo_object_list.h"

#include "egoboo.h"

#include "file_formats/eve_file.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct ego_pro;
struct ego_chr;

typedef t_ego_obj_container< ego_obj_enc, MAX_ENC >  ego_enc_container;

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
extern t_cpp_stack< ego_eve, MAX_EVE > EveStack;

#define LOADED_EVE( IEVE )      ( EveStack.in_range_ref( IEVE ) && EveStack[IEVE].loaded )

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
    explicit ego_enc_data()  { ego_enc_data::ctor_this( this ); };

    /// default destructor
    ~ego_enc_data() { ego_enc_data::dtor_this( this ); };

    //---- construction and destruction

    /// construct this struct, ONLY
    static ego_enc_data * ctor_this( ego_enc_data * );
    /// destruct this struct, ONLY
    static ego_enc_data * dtor_this( ego_enc_data * );
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

    /// construct this struct, and ALL dependent structs. use placement new
    static ego_enc_data * ctor_all( ego_enc_data * ptr ) { if ( NULL != ptr ) { /* puts( "\t" __FUNCTION__ ); */ new( ptr ) ego_enc_data(); } return ptr; }
    /// denstruct this struct, and ALL dependent structs. call the destructor
    static ego_enc_data * dtor_all( ego_enc_data * ptr )  { if ( NULL != ptr ) { ptr->~ego_enc_data(); /* puts( "\t" __FUNCTION__ ); */ } return ptr; }

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
    friend struct t_ego_obj_container< ego_enc, MAX_ENC >;

    typedef ego_obj_enc object_type;

public:
    ego_enc_spawn_data  spawn_data;

    //---- constructors and destructors

    /// non-default constructor. We MUST know who our parent is
    explicit ego_enc( ego_obj_enc * _pparent ) : _obj_ptr( _pparent ) { ego_enc::ctor_this( this ); };

    /// default destructor
    ~ego_enc() { ego_enc::dtor_this( this ); };

    //---- implementation of required accessors

    // This ego_enc is contained by ego_che_obj. We need some way of accessing it
    // These have to have generic names to that all objects that are contained in
    // an ego_object can be interfaced with in the same way

    static       ego_obj_enc &  get_obj_ref( ego_enc & ref ) { return *(( ego_obj_enc * )ref._obj_ptr ); }
    static       ego_obj_enc *  get_obj_ptr( ego_enc * ptr ) { return ( NULL == ptr ) ? NULL : ( ego_obj_enc * )ptr->_obj_ptr; }
    static const ego_obj_enc * cget_obj_ptr( const ego_enc * ptr ) { return ( NULL == ptr ) ? NULL : ptr->_obj_ptr; }

    //---- construction and destruction

    /// construct this struct, ONLY
    static ego_enc * ctor_this( ego_enc * penc );
    /// destruct this struct, ONLY
    static ego_enc * dtor_this( ego_enc * penc );
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

    //---- construction and destruction

    /// construct this struct, and ALL dependent structs. use placement new
    static ego_enc * ctor_all( ego_enc * ptr, ego_obj_enc * pparent ) { if ( NULL != ptr ) { /* puts( "\t" __FUNCTION__ ); */ new( ptr ) ego_enc( pparent ); } return ptr; }
    /// denstruct this struct, and ALL dependent structs. call the destructor
    static ego_enc * dtor_all( ego_enc * ptr )  { if ( NULL != ptr ) { ptr->~ego_enc(); /* puts( "\t" __FUNCTION__ ); */ } return ptr; }

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

    static ego_enc * do_constructing( ego_enc * penc );
    static ego_enc * do_initializing( ego_enc * penc );
    static ego_enc * do_processing( ego_enc * penc );
    static ego_enc * do_deinitializing( ego_enc * penc );
    static ego_enc * do_destructing( ego_enc * penc );

private:

    /// a hook to ego_obj_enc, which is the parent container of this object
    const ego_obj_enc * _obj_ptr;
};

//--------------------------------------------------------------------------------------------

/// The actual container that will be stored in the t_cpp_list<>

struct ego_obj_enc : public ego_obj, public ego_enc
{
    typedef ego_enc_container container_type;
    typedef ego_enc           data_type;
    typedef ENC_REF           reference_type;
    typedef ego_obj_enc       its_type;

    ego_obj_enc( const ego_enc_container * pcont ) : ego_enc( this ), _container_ptr( pcont ) {};

    // This its_type is contained by container_type. We need some way of accessing it.

    static       container_type *  get_container_ptr( its_type * ptr ) { return NULL == ptr ? NULL : ( container_type * )ptr->_container_ptr; }
    static const container_type * cget_container_ptr( const its_type * ptr ) { return NULL == ptr ? NULL : ptr->_container_ptr; }

    static       data_type *  get_data_ptr( its_type * ptr ) { return NULL == ptr ? NULL : static_cast<data_type *>( ptr ); }
    static const data_type * cget_data_ptr( const its_type * ptr ) { return NULL == ptr ? NULL : static_cast<const data_type *>( ptr ); }

    static bool_t request_terminate( const reference_type & ienc );
    static bool_t request_terminate( its_type * pobj );
    //static bool_t request_terminate( ego_enc_bundle * pbdl_enc );

    //---- specialization of the ego_object_process methods
    virtual int do_constructing( void ) { if ( NULL == this ) return -1; return NULL == data_type::do_constructing( this ) ? -1 : 1; };
    virtual int do_initializing( void ) { if ( NULL == this ) return -1; return NULL == data_type::do_initializing( this ) ? -1 : 1; };
    virtual int do_deinitializing( void ) { if ( NULL == this ) return -1; return NULL == data_type::do_deinitializing( this ) ? -1 : 1; };
    virtual int do_processing( void ) { if ( NULL == this ) return -1; return NULL == data_type::do_processing( this ) ? -1 : 1; };
    virtual int do_destructing( void ) { if ( NULL == this ) return -1; return NULL == data_type::do_destructing( this ) ? -1 : 1; };

    //---- specialization (if any) of the i_ego_obj interface
    virtual bool_t    object_allocated( void );
    virtual bool_t    object_update_list_id( void );
    //virtual ego_obj     * get_obj_ptr(void);
    //virtual ego_obj_chr * get_obj_chr_ptr(void);
    virtual ego_obj_enc * get_obj_enc_ptr( void )  { return this; }
    //virtual ego_obj_prt * get_obj_prt_ptr(void);

private:

    /// a hook to container_type, which is the parent container of this object
    const container_type * _container_ptr;
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
