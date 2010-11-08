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

#include "EncList.h"

#include "egoboo_object_list.inl"

//--------------------------------------------------------------------------------------------
// MACROS
//--------------------------------------------------------------------------------------------

// pointer conversion

#define PENC_CGET_POBJ( PENC )       ego_enc::cget_obj_ptr(PENC)
#define POBJ_ENC_CGET_PCONT( POBJ )  ego_obj_enc::cget_container_ptr( POBJ )
#define PENC_CGET_PCONT( PENC )      POBJ_ENC_CGET_PCONT( PENC_CGET_POBJ(PENC) )

#define PENC_GET_POBJ( PENC )       ego_enc::get_obj_ptr(PENC)
#define POBJ_ENC_GET_PCONT( POBJ )  ego_obj_enc::get_container_ptr( POBJ )
#define PENC_GET_PCONT( PENC )      POBJ_ENC_GET_PCONT( PENC_GET_POBJ(PENC) )

#define IENC_GET_POBJ( IENC )       EncObjList.get_data_ptr(IENC)
#define IENC_GET_PCONT( IENC )      EncObjList.get_ptr(IENC)

// grab the index from the ego_enc
#define GET_IDX_PENC( PENC )      GET_INDEX_PCONT( ego_enc_container, PENC_CGET_PCONT(PENC), MAX_ENC )
#define GET_REF_PENC( PENC )      ENC_REF(GET_IDX_PENC( PENC ))

//// grab the allocated flag
#define ALLOCATED_ENC( IENC )      FLAG_ALLOCATED_PCONT(ego_enc_container,  IENC_GET_PCONT(IENC) )
#define ALLOCATED_PENC( PENC )     FLAG_ALLOCATED_PCONT(ego_enc_container,  PENC_CGET_PCONT(PENC) )

//--------------------------------------------------------------------------------------------
// Macros to automate looping through a EncObjList. Based on generic code for
// looping through a t_obj_lst_map<>
//
// This still looks a bit messy, but I think it can't be helped if we want the
// rest of our codebase to look "pretty"...

#define ENC_LOOP_WRAP_PTR( PENC, POBJ ) \
    ego_enc * PENC = (ego_enc *)static_cast<const ego_enc *>(POBJ); \
     
#define ENC_LOOP_WRAP_BDL( BDL, PENC ) \
    ego_bundle_enc BDL( PENC );

/// loops through EncObjList for all allocated enchants, creating a reference, and a pointer
#define ENC_BEGIN_LOOP_ALLOCATED(IT, PENC) \
    OBJ_LIST_BEGIN_LOOP_ALLOCATED(ego_obj_enc, EncObjList, IT, PENC##_obj) \
    ENC_LOOP_WRAP_PTR( PENC, PENC##_obj )

/// loops through EncObjList for all "defined" enchants, creating a reference, and a pointer
#define ENC_BEGIN_LOOP_DEFINED(IT, PENC) \
    OBJ_LIST_BEGIN_LOOP_DEFINED(ego_obj_enc, EncObjList, IT, PENC##_obj) \
    ENC_LOOP_WRAP_PTR( PENC, PENC##_obj )

/// loops through EncObjList for all "active" enchants, creating a reference, and a pointer
#define ENC_BEGIN_LOOP_PROCESSING(IT, PENC) \
    OBJ_LIST_BEGIN_LOOP_PROCESSING(ego_obj_enc, EncObjList, IT, PENC##_obj) \
    ENC_LOOP_WRAP_PTR( PENC, PENC##_obj )

/// loops through EncObjList for all in-game enchants, creating a reference, and a pointer
#define ENC_BEGIN_LOOP_INGAME(IT, PENC) \
    OBJ_LIST_BEGIN_LOOP_INGAME(ego_obj_enc, EncObjList, IT, PENC##_obj) \
    ENC_LOOP_WRAP_PTR( PENC, PENC##_obj )

/// loops through EncObjList for all in-game enchants that are registered in the BSP
#define ENC_BEGIN_LOOP_BSP(IT, PENC) \
    ENC_BEGIN_LOOP_INGAME(IT, PENC) \
    if( !PENC##_obj->bsp_leaf.inserted ) continue;

/// loops through EncObjList for all "defined" enchants, creating a reference, and a pointer
#define ENC_BEGIN_LOOP_ALLOCATED_BDL(IT, ENC_BDL) \
    ENC_BEGIN_LOOP_ALLOCATED( IT, ENC_BDL##_ptr ) \
    ENC_LOOP_WRAP_BDL( ENC_BDL, ENC_BDL##_ptr )

/// loops through EncObjList for all "defined" enchants, creating a reference, and a pointer
#define ENC_BEGIN_LOOP_DEFINED_BDL(IT, ENC_BDL) \
    ENC_BEGIN_LOOP_DEFINED( IT, ENC_BDL##_ptr ) \
    ENC_LOOP_WRAP_BDL( ENC_BDL, ENC_BDL##_ptr )

/// loops through EncObjList for all "active" enchants, creating a reference, and a pointer
#define ENC_BEGIN_LOOP_PROCESSING_BDL(IT, ENC_BDL) \
    ENC_BEGIN_LOOP_PROCESSING( IT, ENC_BDL##_ptr ) \
    ENC_LOOP_WRAP_BDL( ENC_BDL, ENC_BDL##_ptr )

/// loops through EncObjList for all in-game enchants, creating a reference, and a pointer
#define ENC_BEGIN_LOOP_INGAME_BDL(IT, ENC_BDL) \
    ENC_BEGIN_LOOP_INGAME( IT, ENC_BDL##_ptr ) \
    ENC_LOOP_WRAP_BDL( ENC_BDL, ENC_BDL##_ptr )

/// loops through EncObjList for all "active" enchants that are registered in the BSP
#define ENC_BEGIN_LOOP_BSP_BDL(IT, ENC_BDL) \
    ENC_BEGIN_LOOP_BSP( IT, ENC_BDL##_ptr ) \
    ENC_LOOP_WRAP_BDL( ENC_BDL, ENC_BDL##_ptr )

/// the termination for each EncObjList loop
#define ENC_END_LOOP() \
    OBJ_LIST_END_LOOP(EncObjList)

//--------------------------------------------------------------------------------------------
// FORWARD DECLARATION
//--------------------------------------------------------------------------------------------

static INLINE bool_t VALID_ENC( const ENC_REF & ienc );
static INLINE bool_t DEFINED_ENC( const ENC_REF & ienc );
static INLINE bool_t TERMINATED_ENC( const ENC_REF & ienc );
static INLINE bool_t CONSTRUCTING_ENC( const ENC_REF & ienc );
static INLINE bool_t INITIALIZING_ENC( const ENC_REF & ienc );
static INLINE bool_t PROCESSING_ENC( const ENC_REF & ienc );
static INLINE bool_t DEINITIALIZING_ENC( const ENC_REF & ienc );
static INLINE bool_t WAITING_ENC( const ENC_REF & ienc );
static INLINE bool_t INGAME_ENC( const ENC_REF & ienc );

static INLINE bool_t VALID_PENC( const ego_enc * penc );
static INLINE bool_t DEFINED_PENC( const ego_enc * penc );
static INLINE bool_t TERMINATED_PENC( const ego_enc * penc );
static INLINE bool_t CONSTRUCTING_PENC( const ego_enc * penc );
static INLINE bool_t INITIALIZING_PENC( const ego_enc * penc );
static INLINE bool_t PROCESSING_PENC( const ego_enc * penc );
static INLINE bool_t DEINITIALIZING_PENC( const ego_enc * penc );
static INLINE bool_t WAITING_PENC( const ego_enc * penc );
static INLINE bool_t INGAME_PENC( const ego_enc * penc );

//--------------------------------------------------------------------------------------------
// IMPLEMENTATION
//--------------------------------------------------------------------------------------------

static INLINE bool_t VALID_ENC( const ENC_REF & ienc )
{
    const ego_enc_container * pcont = EncObjList.get_allocated_ptr( ienc );
    if ( NULL == pcont ) return bfalse;

    const ego_obj_enc * pobj = IENC_GET_POBJ( ienc );
    if ( NULL == pobj ) return bfalse;

    return OBJ_VALID_RAW( pobj );
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t DEFINED_ENC( const ENC_REF & ienc )
{
    const ego_enc_container * pcont = EncObjList.get_allocated_ptr( ienc );
    if ( NULL == pcont ) return bfalse;

    const ego_obj_enc * pobj = IENC_GET_POBJ( ienc );
    if ( NULL == pobj ) return bfalse;

    return OBJ_DEFINED_RAW( pobj );
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t TERMINATED_ENC( const ENC_REF & ienc )
{
    const ego_enc_container * pcont = EncObjList.get_allocated_ptr( ienc );
    if ( NULL == pcont ) return bfalse;

    const ego_obj_enc * pobj = IENC_GET_POBJ( ienc );
    if ( NULL == pobj ) return bfalse;

    return OBJ_TERMINATED_RAW( pobj );
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t CONSTRUCTING_ENC( const ENC_REF & ienc )
{
    const ego_enc_container * pcont = EncObjList.get_allocated_ptr( ienc );
    if ( NULL == pcont ) return bfalse;

    const ego_obj_enc * pobj = IENC_GET_POBJ( ienc );
    if ( NULL == pobj ) return bfalse;

    return OBJ_CONSTRUCTING_RAW( pobj );
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t INITIALIZING_ENC( const ENC_REF & ienc )
{
    const ego_enc_container * pcont = EncObjList.get_allocated_ptr( ienc );
    if ( NULL == pcont ) return bfalse;

    const ego_obj_enc * pobj = IENC_GET_POBJ( ienc );
    if ( NULL == pobj ) return bfalse;

    return OBJ_INITIALIZING_RAW( pobj );
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t PROCESSING_ENC( const ENC_REF & ienc )
{
    const ego_enc_container * pcont = EncObjList.get_allocated_ptr( ienc );
    if ( NULL == pcont ) return bfalse;

    const ego_obj_enc * pobj = IENC_GET_POBJ( ienc );
    if ( NULL == pobj ) return bfalse;

    return OBJ_PROCESSING_RAW( pobj );
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t DEINITIALIZING_ENC( const ENC_REF & ienc )
{
    const ego_enc_container * pcont = EncObjList.get_allocated_ptr( ienc );
    if ( NULL == pcont ) return bfalse;

    const ego_obj_enc * pobj = IENC_GET_POBJ( ienc );
    if ( NULL == pobj ) return bfalse;

    return OBJ_DEINITIALIZING_RAW( pobj );
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t WAITING_ENC( const ENC_REF & ienc )
{
    const ego_enc_container * pcont = EncObjList.get_allocated_ptr( ienc );
    if ( NULL == pcont ) return bfalse;

    const ego_obj_enc * pobj = IENC_GET_POBJ( ienc );
    if ( NULL == pobj ) return bfalse;

    return OBJ_WAITING_RAW( pobj );
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t INGAME_ENC( const ENC_REF & ienc )
{
    bool_t retval = bfalse;

    const ego_enc_container * pcont = EncObjList.get_allocated_ptr( ienc );
    if ( NULL == pcont ) return bfalse;

    const ego_obj_enc * pobj = IENC_GET_POBJ( ienc );
    if ( NULL == pobj ) return bfalse;

    if ( ego_obj::get_spawn_depth() > 0 )
    {
        retval = OBJ_DEFINED_RAW( pobj );
    }
    else
    {
        retval = OBJ_PROCESSING_RAW( pobj );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static INLINE bool_t VALID_PENC( const ego_enc * penc )
{
    const ego_obj_enc * pobj = PENC_CGET_POBJ( penc );
    if ( NULL == pobj ) return bfalse;

    const ego_enc_container * pcont = POBJ_ENC_CGET_PCONT( pobj );
    if ( !allocator_client::has_valid_id( pcont ) ) return bfalse;

    return OBJ_VALID_RAW( pobj );
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t DEFINED_PENC( const ego_enc * penc )
{
    const ego_obj_enc * pobj = PENC_CGET_POBJ( penc );
    if ( NULL == pobj ) return bfalse;

    const ego_enc_container * pcont = POBJ_ENC_CGET_PCONT( pobj );
    if ( !allocator_client::has_valid_id( pcont ) ) return bfalse;

    return OBJ_DEFINED_RAW( pobj );
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t TERMINATED_PENC( const ego_enc * penc )
{
    const ego_obj_enc * pobj = PENC_CGET_POBJ( penc );
    if ( NULL == pobj ) return bfalse;

    const ego_enc_container * pcont = POBJ_ENC_CGET_PCONT( pobj );
    if ( !allocator_client::has_valid_id( pcont ) ) return bfalse;

    return OBJ_TERMINATED_RAW( pobj );
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t CONSTRUCTING_PENC( const ego_enc * penc )
{
    const ego_obj_enc * pobj = PENC_CGET_POBJ( penc );
    if ( NULL == pobj ) return bfalse;

    const ego_enc_container * pcont = POBJ_ENC_CGET_PCONT( pobj );
    if ( !allocator_client::has_valid_id( pcont ) ) return bfalse;

    return OBJ_CONSTRUCTING_RAW( pobj );
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t INITIALIZING_PENC( const ego_enc * penc )
{
    const ego_obj_enc * pobj = PENC_CGET_POBJ( penc );
    if ( NULL == pobj ) return bfalse;

    const ego_enc_container * pcont = POBJ_ENC_CGET_PCONT( pobj );
    if ( !allocator_client::has_valid_id( pcont ) ) return bfalse;

    return OBJ_INITIALIZING_RAW( pobj );
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t PROCESSING_PENC( const ego_enc * penc )
{
    const ego_obj_enc * pobj = PENC_CGET_POBJ( penc );
    if ( NULL == pobj ) return bfalse;

    const ego_enc_container * pcont = POBJ_ENC_CGET_PCONT( pobj );
    if ( !allocator_client::has_valid_id( pcont ) ) return bfalse;

    return OBJ_PROCESSING_RAW( pobj );
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t DEINITIALIZING_PENC( const ego_enc * penc )
{
    const ego_obj_enc * pobj = PENC_CGET_POBJ( penc );
    if ( NULL == pobj ) return bfalse;

    const ego_enc_container * pcont = POBJ_ENC_CGET_PCONT( pobj );
    if ( !allocator_client::has_valid_id( pcont ) ) return bfalse;

    return OBJ_DEINITIALIZING_RAW( pobj );
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t WAITING_PENC( const ego_enc * penc )
{
    const ego_obj_enc * pobj = PENC_CGET_POBJ( penc );
    if ( NULL == pobj ) return bfalse;

    const ego_enc_container * pcont = POBJ_ENC_CGET_PCONT( pobj );
    if ( !allocator_client::has_valid_id( pcont ) ) return bfalse;

    return OBJ_WAITING_RAW( pobj );
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t INGAME_PENC( const ego_enc * penc )
{
    bool_t retval = bfalse;

    const ego_obj_enc * pobj = PENC_CGET_POBJ( penc );
    if ( NULL == pobj ) return bfalse;

    const ego_enc_container * pcont = POBJ_ENC_CGET_PCONT( pobj );
    if ( !allocator_client::has_valid_id( pcont ) ) return bfalse;

    if ( ego_obj::get_spawn_depth() > 0 )
    {
        retval = OBJ_DEFINED_RAW( pobj );
    }
    else
    {
        retval = OBJ_PROCESSING_RAW( pobj );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define _EncList_inl