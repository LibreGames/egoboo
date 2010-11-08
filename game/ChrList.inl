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

#include "ChrList.h"

#include "egoboo_object_list.inl"

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

//--------------------------------------------------------------------------------------------
// Macros to automate looping through a ChrObjList. Based on generic code for
// looping through a t_obj_lst_map<>
//
// This still looks a bit messy, but I think it can't be helped if we want the
// rest of our codebase to look "pretty"...

#define CHR_LOOP_WRAP_PTR( PCHR, POBJ ) \
    ego_chr * PCHR = (ego_chr *)static_cast<const ego_chr *>(POBJ); \
     
#define CHR_LOOP_WRAP_BDL( BDL, PCHR ) \
    ego_bundle_chr BDL( PCHR );

/// loops through ChrObjList for all allocated characters, creating a reference, and a pointer
#define CHR_BEGIN_LOOP_ALLOCATED(IT, PCHR) \
    OBJ_LIST_BEGIN_LOOP_ALLOCATED(ego_obj_chr, ChrObjList, IT, PCHR##_obj) \
    CHR_LOOP_WRAP_PTR( PCHR, PCHR##_obj )

/// loops through ChrObjList for all "defined" characters, creating a reference, and a pointer
#define CHR_BEGIN_LOOP_DEFINED(IT, PCHR) \
    OBJ_LIST_BEGIN_LOOP_DEFINED(ego_obj_chr, ChrObjList, IT, PCHR##_obj) \
    CHR_LOOP_WRAP_PTR( PCHR, PCHR##_obj )

/// loops through ChrObjList for all "active" characters, creating a reference, and a pointer
#define CHR_BEGIN_LOOP_PROCESSING(IT, PCHR) \
    OBJ_LIST_BEGIN_LOOP_PROCESSING(ego_obj_chr, ChrObjList, IT, PCHR##_obj) \
    CHR_LOOP_WRAP_PTR( PCHR, PCHR##_obj )

/// loops through ChrObjList for all in-game characters, creating a reference, and a pointer
#define CHR_BEGIN_LOOP_INGAME(IT, PCHR) \
    OBJ_LIST_BEGIN_LOOP_INGAME(ego_obj_chr, ChrObjList, IT, PCHR##_obj) \
    CHR_LOOP_WRAP_PTR( PCHR, PCHR##_obj )

/// loops through ChrObjList for all in-game characters that are registered in the BSP
#define CHR_BEGIN_LOOP_BSP(IT, PCHR) \
    CHR_BEGIN_LOOP_INGAME(IT, PCHR) \
    if( !PCHR##_obj->bsp_leaf.inserted ) continue;

/// loops through ChrObjList for all "defined" characters, creating a reference, and a pointer
#define CHR_BEGIN_LOOP_ALLOCATED_BDL(IT, CHR_BDL) \
    CHR_BEGIN_LOOP_ALLOCATED( IT, CHR_BDL##_ptr ) \
    CHR_LOOP_WRAP_BDL( CHR_BDL, CHR_BDL##_ptr )

/// loops through ChrObjList for all "defined" characters, creating a reference, and a pointer
#define CHR_BEGIN_LOOP_DEFINED_BDL(IT, CHR_BDL) \
    CHR_BEGIN_LOOP_DEFINED( IT, CHR_BDL##_ptr ) \
    CHR_LOOP_WRAP_BDL( CHR_BDL, CHR_BDL##_ptr )

/// loops through ChrObjList for all "active" characters, creating a reference, and a pointer
#define CHR_BEGIN_LOOP_PROCESSING_BDL(IT, CHR_BDL) \
    CHR_BEGIN_LOOP_PROCESSING( IT, CHR_BDL##_ptr ) \
    CHR_LOOP_WRAP_BDL( CHR_BDL, CHR_BDL##_ptr )

/// loops through ChrObjList for all in-game characters, creating a reference, and a pointer
#define CHR_BEGIN_LOOP_INGAME_BDL(IT, CHR_BDL) \
    CHR_BEGIN_LOOP_INGAME( IT, CHR_BDL##_ptr ) \
    CHR_LOOP_WRAP_BDL( CHR_BDL, CHR_BDL##_ptr )

/// loops through ChrObjList for all "active" characters that are registered in the BSP
#define CHR_BEGIN_LOOP_BSP_BDL(IT, CHR_BDL) \
    CHR_BEGIN_LOOP_BSP( IT, CHR_BDL##_ptr ) \
    CHR_LOOP_WRAP_BDL( CHR_BDL, CHR_BDL##_ptr )

/// the termination for each ChrObjList loop
#define CHR_END_LOOP() \
    OBJ_LIST_END_LOOP(ChrObjList)

//--------------------------------------------------------------------------------------------
// FORWARD DECLARATION
//--------------------------------------------------------------------------------------------

static INLINE bool_t VALID_CHR( const CHR_REF & ichr );
static INLINE bool_t DEFINED_CHR( const CHR_REF & ichr );
static INLINE bool_t TERMINATED_CHR( const CHR_REF & ichr );
static INLINE bool_t CONSTRUCTING_CHR( const CHR_REF & ichr );
static INLINE bool_t INITIALIZING_CHR( const CHR_REF & ichr );
static INLINE bool_t PROCESSING_CHR( const CHR_REF & ichr );
static INLINE bool_t DEINITIALIZING_CHR( const CHR_REF & ichr );
static INLINE bool_t WAITING_CHR( const CHR_REF & ichr );
static INLINE bool_t INGAME_CHR( const CHR_REF & ichr );

static INLINE bool_t VALID_PCHR( const ego_chr * pchr );
static INLINE bool_t DEFINED_PCHR( const ego_chr * pchr );
static INLINE bool_t TERMINATED_PCHR( const ego_chr * pchr );
static INLINE bool_t CONSTRUCTING_PCHR( const ego_chr * pchr );
static INLINE bool_t INITIALIZING_PCHR( const ego_chr * pchr );
static INLINE bool_t PROCESSING_PCHR( const ego_chr * pchr );
static INLINE bool_t DEINITIALIZING_PCHR( const ego_chr * pchr );
static INLINE bool_t WAITING_PCHR( const ego_chr * pchr );
static INLINE bool_t INGAME_PCHR( const ego_chr * pchr );

//--------------------------------------------------------------------------------------------
// IMPLEMENTATION
//--------------------------------------------------------------------------------------------

static INLINE bool_t VALID_CHR( const CHR_REF & ichr )
{
    const ego_chr_container * pcont = ChrObjList.get_allocated_ptr( ichr );
    if ( NULL == pcont ) return bfalse;

    const ego_obj_chr * pobj = ICHR_GET_POBJ( ichr );
    if ( NULL == pobj ) return bfalse;

    return OBJ_VALID_RAW( pobj );
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t DEFINED_CHR( const CHR_REF & ichr )
{
    const ego_chr_container * pcont = ChrObjList.get_allocated_ptr( ichr );
    if ( NULL == pcont ) return bfalse;

    const ego_obj_chr * pobj = ICHR_GET_POBJ( ichr );
    if ( NULL == pobj ) return bfalse;

    return OBJ_DEFINED_RAW( pobj );
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t TERMINATED_CHR( const CHR_REF & ichr )
{
    const ego_chr_container * pcont = ChrObjList.get_allocated_ptr( ichr );
    if ( NULL == pcont ) return bfalse;

    const ego_obj_chr * pobj = ICHR_GET_POBJ( ichr );
    if ( NULL == pobj ) return bfalse;

    return OBJ_TERMINATED_RAW( pobj );
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t CONSTRUCTING_CHR( const CHR_REF & ichr )
{
    const ego_chr_container * pcont = ChrObjList.get_allocated_ptr( ichr );
    if ( NULL == pcont ) return bfalse;

    const ego_obj_chr * pobj = ICHR_GET_POBJ( ichr );
    if ( NULL == pobj ) return bfalse;

    return OBJ_CONSTRUCTING_RAW( pobj );
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t INITIALIZING_CHR( const CHR_REF & ichr )
{
    const ego_chr_container * pcont = ChrObjList.get_allocated_ptr( ichr );
    if ( NULL == pcont ) return bfalse;

    const ego_obj_chr * pobj = ICHR_GET_POBJ( ichr );
    if ( NULL == pobj ) return bfalse;

    return OBJ_INITIALIZING_RAW( pobj );
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t PROCESSING_CHR( const CHR_REF & ichr )
{
    const ego_chr_container * pcont = ChrObjList.get_allocated_ptr( ichr );
    if ( NULL == pcont ) return bfalse;

    const ego_obj_chr * pobj = ICHR_GET_POBJ( ichr );
    if ( NULL == pobj ) return bfalse;

    return OBJ_PROCESSING_RAW( pobj );
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t DEINITIALIZING_CHR( const CHR_REF & ichr )
{
    const ego_chr_container * pcont = ChrObjList.get_allocated_ptr( ichr );
    if ( NULL == pcont ) return bfalse;

    const ego_obj_chr * pobj = ICHR_GET_POBJ( ichr );
    if ( NULL == pobj ) return bfalse;

    return OBJ_DEINITIALIZING_RAW( pobj );
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t WAITING_CHR( const CHR_REF & ichr )
{
    const ego_chr_container * pcont = ChrObjList.get_allocated_ptr( ichr );
    if ( NULL == pcont ) return bfalse;

    const ego_obj_chr * pobj = ICHR_GET_POBJ( ichr );
    if ( NULL == pobj ) return bfalse;

    return OBJ_WAITING_RAW( pobj );
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t INGAME_CHR( const CHR_REF & ichr )
{
    bool_t retval = bfalse;

    const ego_chr_container * pcont = ChrObjList.get_allocated_ptr( ichr );
    if ( NULL == pcont ) return bfalse;

    const ego_obj_chr * pobj = ICHR_GET_POBJ( ichr );
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

static INLINE bool_t VALID_PCHR( const ego_chr * pchr )
{
    const ego_obj_chr * pobj = PCHR_CGET_POBJ( pchr );
    if ( NULL == pobj ) return bfalse;

    const ego_chr_container * pcont = POBJ_CHR_CGET_PCONT( pobj );
    if ( !allocator_client::has_valid_id( pcont ) ) return bfalse;

    return OBJ_VALID_RAW( pobj );
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t DEFINED_PCHR( const ego_chr * pchr )
{
    const ego_obj_chr * pobj = PCHR_CGET_POBJ( pchr );
    if ( NULL == pobj ) return bfalse;

    const ego_chr_container * pcont = POBJ_CHR_CGET_PCONT( pobj );
    if ( !allocator_client::has_valid_id( pcont ) ) return bfalse;

    return OBJ_DEFINED_RAW( pobj );
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t TERMINATED_PCHR( const ego_chr * pchr )
{
    const ego_obj_chr * pobj = PCHR_CGET_POBJ( pchr );
    if ( NULL == pobj ) return bfalse;

    const ego_chr_container * pcont = POBJ_CHR_CGET_PCONT( pobj );
    if ( !allocator_client::has_valid_id( pcont ) ) return bfalse;

    return OBJ_TERMINATED_RAW( pobj );
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t CONSTRUCTING_PCHR( const ego_chr * pchr )
{
    const ego_obj_chr * pobj = PCHR_CGET_POBJ( pchr );
    if ( NULL == pobj ) return bfalse;

    const ego_chr_container * pcont = POBJ_CHR_CGET_PCONT( pobj );
    if ( !allocator_client::has_valid_id( pcont ) ) return bfalse;

    return OBJ_CONSTRUCTING_RAW( pobj );
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t INITIALIZING_PCHR( const ego_chr * pchr )
{
    const ego_obj_chr * pobj = PCHR_CGET_POBJ( pchr );
    if ( NULL == pobj ) return bfalse;

    const ego_chr_container * pcont = POBJ_CHR_CGET_PCONT( pobj );
    if ( !allocator_client::has_valid_id( pcont ) ) return bfalse;

    return OBJ_INITIALIZING_RAW( pobj );
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t PROCESSING_PCHR( const ego_chr * pchr )
{
    const ego_obj_chr * pobj = PCHR_CGET_POBJ( pchr );
    if ( NULL == pobj ) return bfalse;

    const ego_chr_container * pcont = POBJ_CHR_CGET_PCONT( pobj );
    if ( !allocator_client::has_valid_id( pcont ) ) return bfalse;

    return OBJ_PROCESSING_RAW( pobj );
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t DEINITIALIZING_PCHR( const ego_chr * pchr )
{
    const ego_obj_chr * pobj = PCHR_CGET_POBJ( pchr );
    if ( NULL == pobj ) return bfalse;

    const ego_chr_container * pcont = POBJ_CHR_CGET_PCONT( pobj );
    if ( !allocator_client::has_valid_id( pcont ) ) return bfalse;

    return OBJ_DEINITIALIZING_RAW( pobj );
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t WAITING_PCHR( const ego_chr * pchr )
{
    const ego_obj_chr * pobj = PCHR_CGET_POBJ( pchr );
    if ( NULL == pobj ) return bfalse;

    const ego_chr_container * pcont = POBJ_CHR_CGET_PCONT( pobj );
    if ( !allocator_client::has_valid_id( pcont ) ) return bfalse;

    return OBJ_WAITING_RAW( pobj );
}

//--------------------------------------------------------------------------------------------
static INLINE bool_t INGAME_PCHR( const ego_chr * pchr )
{
    bool_t retval = bfalse;

    const ego_obj_chr * pobj = PCHR_CGET_POBJ( pchr );
    if ( NULL == pobj ) return bfalse;

    const ego_chr_container * pcont = POBJ_CHR_CGET_PCONT( pobj );
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

#define _ChrList_inl