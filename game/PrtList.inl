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

#include "PrtList.h"

#include "egoboo_object_list.inl"

//--------------------------------------------------------------------------------------------
// MACROS
//--------------------------------------------------------------------------------------------

#define PRT_LIMBO_RAW(pobj)   ( pobj->require_bits( ego_obj::full_active ) && pobj->obj_base_display && pobj->reject_bits( ego_obj::killed_bit | ego_obj::on_bit ) )
#define PRT_DISPLAY_RAW(pobj) ( pobj->require_bits( ego_obj::full_active ) && pobj->obj_base_display )

// pointer conversion

#define PPRT_CGET_POBJ( PPRT )       ego_prt::cget_obj_ptr(PPRT)
#define POBJ_PRT_CGET_PCONT( POBJ )  ego_obj_prt::cget_container_ptr( POBJ )
#define PPRT_CGET_PCONT( PPRT )      POBJ_PRT_CGET_PCONT( PPRT_CGET_POBJ(PPRT) )

#define PPRT_GET_POBJ( PPRT )       ego_prt::get_obj_ptr(PPRT)
#define POBJ_PRT_GET_PCONT( POBJ )  ego_obj_prt::get_container_ptr( POBJ )
#define PPRT_GET_PCONT( PPRT )      POBJ_PRT_GET_PCONT( PPRT_GET_POBJ(PPRT) )

#define IPRT_GET_POBJ( IPRT )       PrtObjList.get_data_ptr(IPRT)
#define IPRT_GET_PCONT( IPRT )      PrtObjList.get_ptr(IPRT)

// grab the index from the ego_prt
#define GET_IDX_PPRT( PPRT )      GET_INDEX_PCONT( ego_prt_container, PPRT_CGET_PCONT(PPRT), MAX_PRT )
#define GET_REF_PPRT( PPRT )      PRT_REF(GET_IDX_PPRT( PPRT ))

//// grab the allocated flag
#define ALLOCATED_PRT( IPRT )      FLAG_ALLOCATED_PCONT(ego_prt_container,  IPRT_GET_PCONT(IPRT) )
#define ALLOCATED_PPRT( PPRT )     FLAG_ALLOCATED_PCONT(ego_prt_container,  PPRT_CGET_PCONT(PPRT) )

/// Is the particle flagged as being in limbo?
#define FLAG_DISPLAY_PPRT_OBJ( PPRT_OBJ )  ( (PPRT_OBJ)->obj_base_display )
/// Is the particle object in limbo?
#define STATE_LIMBO_PPRT_OBJ( PPRT_OBJ ) (STATE_PROCESSING_PBASE(PBASE) || STATE_WAITING_PBASE(PBASE))

//--------------------------------------------------------------------------------------------
// Macros to automate looping through a PrtObjList. Based on generic code for
// looping through a t_obj_lst_map<>
//
// This still looks a bit messy, but I think it can't be helped if we want the
// rest of our codebase to look "pretty"...

#define PRT_LOOP_WRAP_PTR( PPRT, POBJ ) \
    ego_prt * PPRT = (ego_prt *)static_cast<const ego_prt *>(POBJ); \
     
#define PRT_LOOP_WRAP_BDL( BDL, PPRT ) \
    ego_bundle_prt BDL( PPRT );

/// loops through PrtObjList for all allocated particles, creating a reference, and a pointer
#define PRT_BEGIN_LOOP_ALLOCATED(IT, PPRT) \
    OBJ_LIST_BEGIN_LOOP_ALLOCATED(ego_obj_prt, PrtObjList, IT, PPRT##_obj) \
    PRT_LOOP_WRAP_PTR( PPRT, PPRT##_obj )

/// loops through PrtObjList for all "defined" particles, creating a reference, and a pointer
#define PRT_BEGIN_LOOP_DEFINED(IT, PPRT) \
    OBJ_LIST_BEGIN_LOOP_DEFINED(ego_obj_prt, PrtObjList, IT, PPRT##_obj) \
    PRT_LOOP_WRAP_PTR( PPRT, PPRT##_obj )

/// loops through PrtObjList for all "active" particles, creating a reference, and a pointer
#define PRT_BEGIN_LOOP_PROCESSING(IT, PPRT) \
    OBJ_LIST_BEGIN_LOOP_PROCESSING(ego_obj_prt, PrtObjList, IT, PPRT##_obj) \
    PRT_LOOP_WRAP_PTR( PPRT, PPRT##_obj )

/// loops through PrtObjList for all in-game particles, creating a reference, and a pointer
#define PRT_BEGIN_LOOP_INGAME(IT, PPRT) \
    OBJ_LIST_BEGIN_LOOP_INGAME(ego_obj_prt, PrtObjList, IT, PPRT##_obj) \
    PRT_LOOP_WRAP_PTR( PPRT, PPRT##_obj )

/// loops through PrtObjList for all in-game particles that are registered in the BSP
#define PRT_BEGIN_LOOP_BSP(IT, PPRT) \
    PRT_BEGIN_LOOP_INGAME(IT, PPRT) \
    if( !PPRT->bsp_leaf.inserted ) continue;

/// loops through PrtObjList for all "defined" particles, creating a reference, and a pointer
#define PRT_BEGIN_LOOP_ALLOCATED_BDL(IT, PRT_BDL) \
    PRT_BEGIN_LOOP_ALLOCATED( IT, PRT_BDL##_ptr ) \
    PRT_LOOP_WRAP_BDL( PRT_BDL, PRT_BDL##_ptr )

/// loops through PrtObjList for all "defined" particles, creating a reference, and a pointer
#define PRT_BEGIN_LOOP_DEFINED_BDL(IT, PRT_BDL) \
    PRT_BEGIN_LOOP_DEFINED( IT, PRT_BDL##_ptr ) \
    PRT_LOOP_WRAP_BDL( PRT_BDL, PRT_BDL##_ptr )

/// loops through PrtObjList for all "active" particles, creating a reference, and a pointer
#define PRT_BEGIN_LOOP_PROCESSING_BDL(IT, PRT_BDL) \
    PRT_BEGIN_LOOP_PROCESSING( IT, PRT_BDL##_ptr ) \
    PRT_LOOP_WRAP_BDL( PRT_BDL, PRT_BDL##_ptr )

/// loops through PrtObjList for all in-game particles, creating a reference, and a pointer
#define PRT_BEGIN_LOOP_INGAME_BDL(IT, PRT_BDL) \
    PRT_BEGIN_LOOP_INGAME( IT, PRT_BDL##_ptr ) \
    PRT_LOOP_WRAP_BDL( PRT_BDL, PRT_BDL##_ptr )

/// loops through PrtObjList for all "active" particles that are registered in the BSP
#define PRT_BEGIN_LOOP_BSP_BDL(IT, PRT_BDL) \
    PRT_BEGIN_LOOP_BSP( IT, PRT_BDL##_ptr ) \
    PRT_LOOP_WRAP_BDL( PRT_BDL, PRT_BDL##_ptr )

/// the termination for each PrtObjList loop
#define PRT_END_LOOP() \
    OBJ_LIST_END_LOOP(PrtObjList)

/// loops through PrtObjList for all "defined" particles that are flagged as being in limbo
#define PRT_BEGIN_LOOP_LIMBO( IT, PPRT )  \
    { \
        int internal__##PrtObjList##_start_depth = PrtObjList.get_loop_depth(); \
        PrtObjList.increment_loop_depth(); \
        for( PrtObjList##_t::iterator internal__##IT = PrtObjList.iterator_begin_limbo(); !PrtObjList.iterator_finished(internal__##IT); PrtObjList.iterator_increment_limbo(internal__##IT) ) \
        { \
            PrtObjList##_t::lst_reference IT( *internal__##IT );\
            PrtObjList##_t::container_type * PPRT##_obj##_cont = PrtObjList.get_ptr( IT );\
            if( NULL == PPRT##_obj##_cont ) continue;\
            ego_obj_prt * PPRT##_obj = PrtObjList##_t::container_type::get_data_ptr( PPRT##_obj##_cont ); \
            if( NULL == PPRT##_obj ) continue;\
            int internal__##PrtObjList##_internal_depth = PrtObjList.get_loop_depth();

/// loops through PrtObjList for all "defined" particles that are flagged as being in limbo
#define PRT_BEGIN_LOOP_LIMBO_BDL(IT, PRT_BDL) \
    PRT_BEGIN_LOOP_LIMBO( IT, PRT_BDL##_ptr ) \
    PRT_LOOP_WRAP_BDL( PRT_BDL, PRT_BDL##_ptr )

//--------------------------------------------------------------------------------------------
// FORWARD DECLARATION
//--------------------------------------------------------------------------------------------

static INLINE bool_t VALID_PRT( const PRT_REF & iprt );
static INLINE bool_t DEFINED_PRT( const PRT_REF & iprt );
static INLINE bool_t TERMINATED_PRT( const PRT_REF & iprt );
static INLINE bool_t CONSTRUCTING_PRT( const PRT_REF & iprt );
static INLINE bool_t INITIALIZING_PRT( const PRT_REF & iprt );
static INLINE bool_t PROCESSING_PRT( const PRT_REF & iprt );
static INLINE bool_t DEINITIALIZING_PRT( const PRT_REF & iprt );
static INLINE bool_t WAITING_PRT( const PRT_REF & iprt );
static INLINE bool_t INGAME_PRT( const PRT_REF & iprt );

static INLINE bool_t VALID_PPRT( const ego_prt * pprt );
static INLINE bool_t DEFINED_PPRT( const ego_prt * pprt );
static INLINE bool_t TERMINATED_PPRT( const ego_prt * pprt );
static INLINE bool_t CONSTRUCTING_PPRT( const ego_prt * pprt );
static INLINE bool_t INITIALIZING_PPRT( const ego_prt * pprt );
static INLINE bool_t PROCESSING_PPRT( const ego_prt * pprt );
static INLINE bool_t DEINITIALIZING_PPRT( const ego_prt * pprt );
static INLINE bool_t WAITING_PPRT( const ego_prt * pprt );
static INLINE bool_t INGAME_PPRT( const ego_prt * pprt );

//--------------------------------------------------------------------------------------------
// IMPLEMENTATION
//--------------------------------------------------------------------------------------------

static INLINE bool_t VALID_PRT( const PRT_REF & iprt )
{
    const ego_prt_container * pcont = PrtObjList.get_allocated_ptr( iprt );
    if ( NULL == pcont ) return bfalse;

    const ego_obj_prt * pobj = IPRT_GET_POBJ( iprt );
    if ( NULL == pobj ) return bfalse;

    return OBJ_VALID_RAW( pobj );
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t DEFINED_PRT( const PRT_REF & iprt )
{
    const ego_prt_container * pcont = PrtObjList.get_allocated_ptr( iprt );
    if ( NULL == pcont ) return bfalse;

    const ego_obj_prt * pobj = IPRT_GET_POBJ( iprt );
    if ( NULL == pobj ) return bfalse;

    return OBJ_DEFINED_RAW( pobj );
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t TERMINATED_PRT( const PRT_REF & iprt )
{
    const ego_prt_container * pcont = PrtObjList.get_allocated_ptr( iprt );
    if ( NULL == pcont ) return bfalse;

    const ego_obj_prt * pobj = IPRT_GET_POBJ( iprt );
    if ( NULL == pobj ) return bfalse;

    return OBJ_TERMINATED_RAW( pobj );
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t CONSTRUCTING_PRT( const PRT_REF & iprt )
{
    const ego_prt_container * pcont = PrtObjList.get_allocated_ptr( iprt );
    if ( NULL == pcont ) return bfalse;

    const ego_obj_prt * pobj = IPRT_GET_POBJ( iprt );
    if ( NULL == pobj ) return bfalse;

    return OBJ_CONSTRUCTING_RAW( pobj );
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t INITIALIZING_PRT( const PRT_REF & iprt )
{
    const ego_prt_container * pcont = PrtObjList.get_allocated_ptr( iprt );
    if ( NULL == pcont ) return bfalse;

    const ego_obj_prt * pobj = IPRT_GET_POBJ( iprt );
    if ( NULL == pobj ) return bfalse;

    return OBJ_INITIALIZING_RAW( pobj );
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t PROCESSING_PRT( const PRT_REF & iprt )
{
    const ego_prt_container * pcont = PrtObjList.get_allocated_ptr( iprt );
    if ( NULL == pcont ) return bfalse;

    const ego_obj_prt * pobj = IPRT_GET_POBJ( iprt );
    if ( NULL == pobj ) return bfalse;

    return OBJ_PROCESSING_RAW( pobj );
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t DEINITIALIZING_PRT( const PRT_REF & iprt )
{
    const ego_prt_container * pcont = PrtObjList.get_allocated_ptr( iprt );
    if ( NULL == pcont ) return bfalse;

    const ego_obj_prt * pobj = IPRT_GET_POBJ( iprt );
    if ( NULL == pobj ) return bfalse;

    return OBJ_DEINITIALIZING_RAW( pobj );
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t WAITING_PRT( const PRT_REF & iprt )
{
    const ego_prt_container * pcont = PrtObjList.get_allocated_ptr( iprt );
    if ( NULL == pcont ) return bfalse;

    const ego_obj_prt * pobj = IPRT_GET_POBJ( iprt );
    if ( NULL == pobj ) return bfalse;

    return OBJ_WAITING_RAW( pobj );
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t INGAME_PRT( const PRT_REF & iprt )
{
    bool_t retval = bfalse;

    const ego_prt_container * pcont = PrtObjList.get_allocated_ptr( iprt );
    if ( NULL == pcont ) return bfalse;

    const ego_obj_prt * pobj = IPRT_GET_POBJ( iprt );
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
static INLINE bool_t LIMBO_PRT( const PRT_REF & iprt )
{
    const ego_prt_container * pcont = PrtObjList.get_allocated_ptr( iprt );
    if ( NULL == pcont ) return bfalse;

    const ego_obj_prt * pobj = IPRT_GET_POBJ( iprt );
    if ( NULL == pobj ) return bfalse;

    return PRT_LIMBO_RAW( pobj );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static INLINE bool_t VALID_PPRT( const ego_prt * pprt )
{
    const ego_obj_prt * pobj = PPRT_CGET_POBJ( pprt );
    if ( NULL == pobj ) return bfalse;

    const ego_prt_container * pcont = POBJ_PRT_CGET_PCONT( pobj );
    if ( !allocator_client::has_valid_id( pcont ) ) return bfalse;

    return OBJ_VALID_RAW( pobj );
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t DEFINED_PPRT( const ego_prt * pprt )
{
    const ego_obj_prt * pobj = PPRT_CGET_POBJ( pprt );
    if ( NULL == pobj ) return bfalse;

    const ego_prt_container * pcont = POBJ_PRT_CGET_PCONT( pobj );
    if ( !allocator_client::has_valid_id( pcont ) ) return bfalse;

    return OBJ_DEFINED_RAW( pobj );
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t TERMINATED_PPRT( const ego_prt * pprt )
{
    const ego_obj_prt * pobj = PPRT_CGET_POBJ( pprt );
    if ( NULL == pobj ) return bfalse;

    const ego_prt_container * pcont = POBJ_PRT_CGET_PCONT( pobj );
    if ( !allocator_client::has_valid_id( pcont ) ) return bfalse;

    return OBJ_TERMINATED_RAW( pobj );
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t CONSTRUCTING_PPRT( const ego_prt * pprt )
{
    const ego_obj_prt * pobj = PPRT_CGET_POBJ( pprt );
    if ( NULL == pobj ) return bfalse;

    const ego_prt_container * pcont = POBJ_PRT_CGET_PCONT( pobj );
    if ( !allocator_client::has_valid_id( pcont ) ) return bfalse;

    return OBJ_CONSTRUCTING_RAW( pobj );
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t INITIALIZING_PPRT( const ego_prt * pprt )
{
    const ego_obj_prt * pobj = PPRT_CGET_POBJ( pprt );
    if ( NULL == pobj ) return bfalse;

    const ego_prt_container * pcont = POBJ_PRT_CGET_PCONT( pobj );
    if ( !allocator_client::has_valid_id( pcont ) ) return bfalse;

    return OBJ_INITIALIZING_RAW( pobj );
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t PROCESSING_PPRT( const ego_prt * pprt )
{
    const ego_obj_prt * pobj = PPRT_CGET_POBJ( pprt );
    if ( NULL == pobj ) return bfalse;

    const ego_prt_container * pcont = POBJ_PRT_CGET_PCONT( pobj );
    if ( !allocator_client::has_valid_id( pcont ) ) return bfalse;

    return OBJ_PROCESSING_RAW( pobj );
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t DEINITIALIZING_PPRT( const ego_prt * pprt )
{
    const ego_obj_prt * pobj = PPRT_CGET_POBJ( pprt );
    if ( NULL == pobj ) return bfalse;

    const ego_prt_container * pcont = POBJ_PRT_CGET_PCONT( pobj );
    if ( !allocator_client::has_valid_id( pcont ) ) return bfalse;

    return OBJ_DEINITIALIZING_RAW( pobj );
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t WAITING_PPRT( const ego_prt * pprt )
{
    const ego_obj_prt * pobj = PPRT_CGET_POBJ( pprt );
    if ( NULL == pobj ) return bfalse;

    const ego_prt_container * pcont = POBJ_PRT_CGET_PCONT( pobj );
    if ( !allocator_client::has_valid_id( pcont ) ) return bfalse;

    return OBJ_WAITING_RAW( pobj );
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t INGAME_PPRT( const ego_prt * pprt )
{
    bool_t retval = bfalse;

    const ego_obj_prt * pobj = PPRT_CGET_POBJ( pprt );
    if ( NULL == pobj ) return bfalse;

    const ego_prt_container * pcont = POBJ_PRT_CGET_PCONT( pobj );
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
static INLINE bool_t LIMBO_PPRT( const ego_prt * pprt )
{
    const ego_obj_prt * pobj = PPRT_CGET_POBJ( pprt );
    if ( NULL == pobj ) return bfalse;

    const ego_prt_container * pcont = POBJ_PRT_CGET_PCONT( pobj );
    if ( !allocator_client::has_valid_id( pcont ) ) return bfalse;

    return PRT_LIMBO_RAW( pobj );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define _PrtList_inl