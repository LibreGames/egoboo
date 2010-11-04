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

/// @file ChrList.h
/// @brief Routines for character list management

#include "egoboo_object.h"
#include "egoboo_object_list.h"

#include "char.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct ego_chr;
struct ego_obj_chr;

typedef t_ego_obj_container< ego_obj_chr, MAX_CHR >  ego_chr_container;

//--------------------------------------------------------------------------------------------
// MACROS
//--------------------------------------------------------------------------------------------

// pointer conversion

#define PCHR_CGET_POBJ( PCHR )       ego_chr::cget_obj_ptr(PCHR)
#define POBJ_CHR_CGET_PCONT( POBJ )  ego_obj_chr::cget_container_ptr( POBJ )
#define PCHR_CGET_PCONT( PCHR )      POBJ_CHR_CGET_PCONT( PCHR_CGET_POBJ(PCHR) )

#define PCHR_GET_POBJ( PCHR )       ego_chr::get_obj_ptr(PCHR)
#define POBJ_CHR_GET_PCONT( POBJ )  ego_obj_chr::get_container_ptr( POBJ )
#define PCHR_GET_PCONT( PCHR )      POBJ_CHR_GET_PCONT( PCHR_GET_POBJ(PCHR) )

#define ICHR_GET_POBJ( ICHR )       ChrObjList.get_data_ptr(ICHR)
#define ICHR_GET_PCONT( ICHR )      ChrObjList.get_ptr(ICHR)

// grab the index from the ego_chr
#define GET_IDX_PCHR( PCHR )      GET_INDEX_PCONT( ego_chr_container, PCHR_CGET_PCONT(PCHR), MAX_CHR )
#define GET_REF_PCHR( PCHR )      CHR_REF(GET_IDX_PCHR( PCHR ))

//// grab the allocated flag
#define ALLOCATED_CHR( ICHR )      FLAG_ALLOCATED_PCONT(ego_chr_container,  ICHR_GET_PCONT(ICHR) )
#define ALLOCATED_PCHR( PCHR )     FLAG_ALLOCATED_PCONT(ego_chr_container,  PCHR_CGET_PCONT(PCHR) )

//#define VALID_CHR( ICHR )          ( ALLOCATED_CHR(ICHR) && FLAG_VALID_PBASE(ICHR_GET_POBJ(ICHR)) )
//#define DEFINED_CHR( ICHR )        ( VALID_CHR(ICHR)     && !FLAG_TERMINATED_PBASE(ICHR_GET_POBJ(ICHR)) )
//#define TERMINATED_CHR( ICHR )     ( VALID_CHR(ICHR)     && FLAG_TERMINATED_PBASE(ICHR_GET_POBJ(ICHR)) )
//#define CONSTRUCTING_CHR( ICHR )   ( ALLOCATED_CHR(ICHR) && CONSTRUCTING_PBASE(ICHR_GET_POBJ(ICHR)) )
//#define INITIALIZING_CHR( ICHR )   ( ALLOCATED_CHR(ICHR) && INITIALIZING_PBASE(ICHR_GET_POBJ(ICHR)) )
//#define PROCESSING_CHR( ICHR )     ( ALLOCATED_CHR(ICHR) && PROCESSING_PBASE(ICHR_GET_POBJ(ICHR)) )
//#define DEINITIALIZING_CHR( ICHR ) ( ALLOCATED_CHR(ICHR) && DEINITIALIZING_PBASE(ICHR_GET_POBJ(ICHR)) )
//#define WAITING_CHR( ICHR )        ( ALLOCATED_CHR(ICHR) && WAITING_PBASE(ICHR_GET_POBJ(ICHR)) )
//
//#define VALID_PCHR( PCHR )          ( ALLOCATED_PCHR(PCHR) && FLAG_VALID_PBASE(PCHR_CGET_POBJ(PCHR)) )
//#define DEFINED_PCHR( PCHR )        ( VALID_PCHR(PCHR)     && !FLAG_TERMINATED_PBASE(PCHR_CGET_POBJ(PCHR)) )
//#define TERMINATED_PCHR( PCHR )     ( VALID_PCHR(PCHR)     && FLAG_TERMINATED_PBASE(PCHR_CGET_POBJ(PCHR)) )
//#define CONSTRUCTING_PCHR( PCHR )   ( ALLOCATED_PCHR(PCHR) && CONSTRUCTING_PBASE(PCHR_CGET_POBJ(PCHR)) )
//#define INITIALIZING_PCHR( PCHR )   ( ALLOCATED_PCHR(PCHR) && INITIALIZING_PBASE(PCHR_CGET_POBJ(PCHR)) )
//#define PROCESSING_PCHR( PCHR )     ( ALLOCATED_PCHR(PCHR) && PROCESSING_PBASE(PCHR_CGET_POBJ(PCHR)) )
//#define DEINITIALIZING_PCHR( PCHR ) ( ALLOCATED_PCHR(PCHR) && DEINITIALIZING_PBASE(PCHR_CGET_POBJ(PCHR)) )
//#define WAITING_PCHR( PCHR )        ( ALLOCATED_PCHR(PCHR) && WAITING_PBASE(PCHR_CGET_POBJ(PCHR)) )
//
////--------------------------------------------------------------------------------------------
//// Macros to determine whether the character is in the game or not.
//// If objects are being spawned, then any object that is just "defined" is treated as "in game"
//
//#define INGAME_CHR(ICHR)            ( (ego_obj::get_spawn_depth()) > 0 ? DEFINED_CHR(ICHR) : ALLOCATED_CHR(ICHR) && PROCESSING_PBASE(ICHR_GET_POBJ(ICHR)) )
//#define INGAME_PCHR(PCHR)           ( (ego_obj::get_spawn_depth()) > 0 ? DEFINED_PCHR(PCHR) : ALLOCATED_PCHR(PCHR) && PROCESSING_PBASE(PCHR_CGET_POBJ(PCHR)) )

//--------------------------------------------------------------------------------------------
// Macros to automate looping through a ChrObjList. Based on generic code for
// looping through a t_obj_lst_map<>
//
// This still looks a bit messy, but I think it can't be helped if we want the
// rest of our codebase to look "pretty"...

/// loops through ChrObjList for all "defined" characters, creating a referchre, and a pointer
#define CHR_BEGIN_LOOP_DEFINED(IT, PCHR) \
    OBJ_LIST_BEGIN_LOOP_DEFINED(ego_obj_chr, ChrObjList, IT, PCHR##_obj) \
    ego_chr * PCHR = (ego_chr *)static_cast<const ego_chr *>(PCHR##_obj); \
    if( NULL == PCHR ) continue;

/// loops through ChrObjList for all "active" characters, creating a referchre, and a pointer
#define CHR_BEGIN_LOOP_PROCESSING(IT, PCHR) \
    OBJ_LIST_BEGIN_LOOP_PROCESSING(ego_obj_chr, ChrObjList, IT, PCHR##_obj) \
    ego_chr * PCHR = (ego_chr *)static_cast<const ego_chr *>(PCHR##_obj); \
    if( NULL == PCHR ) continue;

/// loops through ChrObjList for all in-game characters, creating a referchre, and a pointer
#define CHR_BEGIN_LOOP_INGAME(IT, PCHR) \
    OBJ_LIST_BEGIN_LOOP_INGAME(ego_obj_chr, ChrObjList, IT, PCHR##_obj) \
    ego_chr * PCHR = (ego_chr *)static_cast<const ego_chr *>(PCHR##_obj); \
    if( NULL == PCHR ) continue;

/// loops through ChrObjList for all "active" characters that are registered in the BSP
#define CHR_BEGIN_LOOP_BSP(IT, PCHR) \
    CHR_BEGIN_LOOP_PROCESSING(IT, PCHR) \
    if( !PCHR->bsp_leaf.inserted ) continue;

/// the termination for each ChrObjList loop
#define CHR_END_LOOP() \
    OBJ_LIST_END_LOOP(ChrObjList)

//--------------------------------------------------------------------------------------------
// list declaration
//--------------------------------------------------------------------------------------------
typedef t_obj_lst_deque<ego_obj_chr, MAX_CHR> ChrObjList_t;

extern ChrObjList_t ChrObjList;

//--------------------------------------------------------------------------------------------
// Function prototypes
//--------------------------------------------------------------------------------------------

static INLINE bool_t VALID_CHR( const CHR_REF & ichr )
{
    const ego_chr_container * pcont = ChrObjList.get_allocated_ptr( ichr );
    if ( NULL == pcont ) return bfalse;

    const ego_obj_chr * pobj = ICHR_GET_POBJ( ichr );
    if ( NULL == pobj ) return bfalse;

    return ego_object_process_state_data::get_valid( pobj );
}

static INLINE bool_t DEFINED_CHR( const CHR_REF & ichr )
{
    const ego_chr_container * pcont = ChrObjList.get_allocated_ptr( ichr );
    if ( NULL == pcont ) return bfalse;

    const ego_obj_chr * pobj = ICHR_GET_POBJ( ichr );
    if ( NULL == pobj ) return bfalse;

    return
        ego_object_process_state_data::get_valid( pobj )
        && !ego_obj::get_killed( pobj );
}

static INLINE bool_t TERMINATED_CHR( const CHR_REF & ichr )
{
    const ego_chr_container * pcont = ChrObjList.get_allocated_ptr( ichr );
    if ( NULL == pcont ) return bfalse;

    const ego_obj_chr * pobj = ICHR_GET_POBJ( ichr );
    if ( NULL == pobj ) return bfalse;

    return
        ego_object_process_state_data::get_valid( pobj )
        && ego_obj::get_killed( pobj );
}

static INLINE bool_t CONSTRUCTING_CHR( const CHR_REF & ichr )
{
    const ego_chr_container * pcont = ChrObjList.get_allocated_ptr( ichr );
    if ( NULL == pcont ) return bfalse;

    const ego_obj_chr * pobj = ICHR_GET_POBJ( ichr );
    if ( NULL == pobj ) return bfalse;

    return
        ego_object_process_state_data::get_valid( pobj )
        && !ego_obj::get_killed( pobj )
        && ( ego_obj_constructing == ego_obj::get_action( pobj ) );
}

static INLINE bool_t INITIALIZING_CHR( const CHR_REF & ichr )
{
    const ego_chr_container * pcont = ChrObjList.get_allocated_ptr( ichr );
    if ( NULL == pcont ) return bfalse;

    const ego_obj_chr * pobj = ICHR_GET_POBJ( ichr );
    if ( NULL == pobj ) return bfalse;

    return
        ego_object_process_state_data::get_constructed( pobj )
        && !ego_obj::get_killed( pobj )
        && ( ego_obj_initializing == ego_obj::get_action( pobj ) );
}

static INLINE bool_t PROCESSING_CHR( const CHR_REF & ichr )
{
    const ego_chr_container * pcont = ChrObjList.get_allocated_ptr( ichr );
    if ( NULL == pcont ) return bfalse;

    const ego_obj_chr * pobj = ICHR_GET_POBJ( ichr );
    if ( NULL == pobj ) return bfalse;

    return
        ego_obj::get_on( pobj )
        && !ego_obj::get_killed( pobj )
        && ( ego_obj_processing == ego_obj::get_action( pobj ) );
}

static INLINE bool_t DEINITIALIZING_CHR( const CHR_REF & ichr )
{
    const ego_chr_container * pcont = ChrObjList.get_allocated_ptr( ichr );
    if ( NULL == pcont ) return bfalse;

    const ego_obj_chr * pobj = ICHR_GET_POBJ( ichr );
    if ( NULL == pobj ) return bfalse;

    return
        ego_object_process_state_data::get_constructed( pobj )
        && !ego_obj::get_killed( pobj )
        && ( ego_obj_deinitializing == ego_obj::get_action( pobj ) );
}

static INLINE bool_t WAITING_CHR( const CHR_REF & ichr )
{
    const ego_chr_container * pcont = ChrObjList.get_allocated_ptr( ichr );
    if ( NULL == pcont ) return bfalse;

    const ego_obj_chr * pobj = ICHR_GET_POBJ( ichr );
    if ( NULL == pobj ) return bfalse;

    return
        ego_object_process_state_data::get_valid( pobj )
        && !ego_obj::get_killed( pobj )
        && ( ego_obj_waiting == ego_object_process_state_data::get_action( pobj ) );
}

static INLINE bool_t VALID_PCHR( const ego_chr * pchr )
{
    const ego_obj_chr * pobj = PCHR_CGET_POBJ( pchr );
    if ( NULL == pobj ) return bfalse;

    const ego_chr_container * pcont = POBJ_CHR_CGET_PCONT( pobj );
    if ( !allocator_client::has_valid_id( pcont ) ) return bfalse;

    return
        ego_object_process_state_data::get_valid( pobj );
}

static INLINE bool_t DEFINED_PCHR( const ego_chr * pchr )
{
    const ego_obj_chr * pobj = PCHR_CGET_POBJ( pchr );
    if ( NULL == pobj ) return bfalse;

    const ego_chr_container * pcont = POBJ_CHR_CGET_PCONT( pobj );
    if ( !allocator_client::has_valid_id( pcont ) ) return bfalse;

    return
        ego_object_process_state_data::get_valid( pobj )
        && !ego_obj::get_killed( pobj );
}

static INLINE bool_t TERMINATED_PCHR( const ego_chr * pchr )
{
    const ego_obj_chr * pobj = PCHR_CGET_POBJ( pchr );
    if ( NULL == pobj ) return bfalse;

    const ego_chr_container * pcont = POBJ_CHR_CGET_PCONT( pobj );
    if ( !allocator_client::has_valid_id( pcont ) ) return bfalse;

    return
        ego_object_process_state_data::get_valid( pobj )
        && ego_obj::get_killed( pobj );
}

static INLINE bool_t CONSTRUCTING_PCHR( const ego_chr * pchr )
{
    const ego_obj_chr * pobj = PCHR_CGET_POBJ( pchr );
    if ( NULL == pobj ) return bfalse;

    const ego_chr_container * pcont = POBJ_CHR_CGET_PCONT( pobj );
    if ( !allocator_client::has_valid_id( pcont ) ) return bfalse;

    return
        ego_object_process_state_data::get_valid( pobj )
        && !ego_obj::get_killed( pobj )
        && ( ego_obj_constructing == ego_obj::get_action( pobj ) );
}

static INLINE bool_t INITIALIZING_PCHR( const ego_chr * pchr )
{
    const ego_obj_chr * pobj = PCHR_CGET_POBJ( pchr );
    if ( NULL == pobj ) return bfalse;

    const ego_chr_container * pcont = POBJ_CHR_CGET_PCONT( pobj );
    if ( !allocator_client::has_valid_id( pcont ) ) return bfalse;

    return
        ego_object_process_state_data::get_constructed( pobj )
        && !ego_obj::get_killed( pobj )
        && ( ego_obj_initializing == ego_obj::get_action( pobj ) );
}

static INLINE bool_t PROCESSING_PCHR( const ego_chr * pchr )
{
    const ego_obj_chr * pobj = PCHR_CGET_POBJ( pchr );
    if ( NULL == pobj ) return bfalse;

    const ego_chr_container * pcont = POBJ_CHR_CGET_PCONT( pobj );
    if ( !allocator_client::has_valid_id( pcont ) ) return bfalse;

    return
        ego_obj::get_on( pobj )
        && !ego_obj::get_killed( pobj )
        && ( ego_obj_processing == ego_obj::get_action( pobj ) );
}

static INLINE bool_t DEINITIALIZING_PCHR( const ego_chr * pchr )
{
    const ego_obj_chr * pobj = PCHR_CGET_POBJ( pchr );
    if ( NULL == pobj ) return bfalse;

    const ego_chr_container * pcont = POBJ_CHR_CGET_PCONT( pobj );
    if ( !allocator_client::has_valid_id( pcont ) ) return bfalse;

    return
        ego_object_process_state_data::get_constructed( pobj )
        && !ego_obj::get_killed( pobj )
        && ( ego_obj_deinitializing == ego_obj::get_action( pobj ) );
}

static INLINE bool_t WAITING_PCHR( const ego_chr * pchr )
{
    const ego_obj_chr * pobj = PCHR_CGET_POBJ( pchr );
    if ( NULL == pobj ) return bfalse;

    const ego_chr_container * pcont = POBJ_CHR_CGET_PCONT( pobj );
    if ( !allocator_client::has_valid_id( pcont ) ) return bfalse;

    return
        ego_object_process_state_data::get_valid( pobj )
        && !ego_obj::get_killed( pobj )
        && ( ego_obj_waiting == ego_object_process_state_data::get_action( pobj ) );
}

static INLINE bool_t INGAME_CHR( const CHR_REF & ichr )
{
    bool_t retval = bfalse;

    const ego_chr_container * pcont = ChrObjList.get_allocated_ptr( ichr );
    if ( NULL == pcont ) return bfalse;

    const ego_obj_chr * pobj = ICHR_GET_POBJ( ichr );
    if ( NULL == pobj ) return bfalse;

    if ( ego_obj::get_spawn_depth() > 0 )
    {
        retval =
            ego_object_process_state_data::get_valid( pobj )
            && !ego_obj::get_killed( pobj );
    }
    else
    {
        retval = ego_obj::get_on( pobj ) &&
                 !ego_obj::get_killed( pobj ) &&
                 ( ego_obj_processing == ego_obj::get_action( pobj ) );
    }

    return retval;
}

static INLINE bool_t INGAME_PCHR( const ego_chr * pchr )
{
    bool_t retval = bfalse;

    const ego_obj_chr * pobj = PCHR_CGET_POBJ( pchr );
    if ( NULL == pobj ) return bfalse;

    const ego_chr_container * pcont = POBJ_CHR_CGET_PCONT( pobj );
    if ( !allocator_client::has_valid_id( pcont ) ) return bfalse;

    if ( ego_obj::get_spawn_depth() > 0 )
    {
        retval =
            ego_object_process_state_data::get_valid( pobj )
            && !ego_obj::get_killed( pobj );
    }
    else
    {
        retval = ego_obj::get_on( pobj ) &&
                 !ego_obj::get_killed( pobj ) &&
                 ( ego_obj_processing == ego_obj::get_action( pobj ) );
    }

    return retval;
}

#define _ChrList_h