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
#define GET_IDX_PCHR( PCHR )    ((NULL == PCHR) ? MAX_CHR : INDEX_PCONT(ego_chr_container, PCHR_CGET_PCONT(PCHR)))
#define GET_REF_PCHR( PCHR )    CHR_REF(GET_IDX_PCHR( PCHR ))

// grab the allocated flag
#define ALLOCATED_CHR( ICHR )    ego_chr_container::get_allocated( ICHR_GET_PCONT(ICHR) )
#define ALLOCATED_PCHR( PCHR )   ego_chr_container::get_allocated( PCHR_CGET_PCONT(PCHR) )

#define VALID_CHR( ICHR )       ( ALLOCATED_CHR( ICHR ) && FLAG_VALID_PBASE(ICHR_GET_POBJ(ICHR)) )
#define DEFINED_CHR( ICHR )     ( VALID_PBASE(ICHR_GET_POBJ(ICHR) ) && !FLAG_TERMINATED_PBASE(ICHR_GET_POBJ(ICHR)) )
#define ACTIVE_CHR( ICHR )      ( ACTIVE_PBASE(ICHR_GET_POBJ(ICHR)) )
#define WAITING_CHR( ICHR )     ( WAITING_PBASE(ICHR_GET_POBJ(ICHR)) )
#define TERMINATED_CHR( ICHR )  ( TERMINATED_PBASE(ICHR_GET_POBJ(ICHR)) )

#define VALID_PCHR( PCHR )       ( ALLOCATED_PCHR( PCHR ) && FLAG_VALID_PBASE(PCHR_CGET_POBJ(PCHR)) )
#define DEFINED_PCHR( PCHR )     ( VALID_PBASE(PCHR_CGET_POBJ(PCHR)) && !FLAG_TERMINATED_PBASE(PCHR_CGET_POBJ(PCHR)) )
#define ACTIVE_PCHR( PCHR )      ( ACTIVE_PBASE(PCHR_CGET_POBJ(PCHR)) )
#define WAITING_PCHR( PCHR )     ( WAITING_PBASE(PCHR_CGET_POBJ(PCHR)) )
#define TERMINATED_PCHR( PCHR )  ( TERMINATED_PBASE(PCHR_CGET_POBJ(PCHR)) )

//--------------------------------------------------------------------------------------------
// Macros to automate looping through a ChrObjList. Based on generic code for
// looping through a t_ego_obj_lst<>
//
// This still looks a bit messy, but I think it can't be helped if we want the
// rest of our codebase to look "pretty"...

/// loops through ChrObjList for all "defined" characters, creating a reference, and a pointer
#define CHR_BEGIN_LOOP_DEFINED(IT, PCHR) \
    /* printf( "++++ DEFINED ChrObjList.get_loop_depth() == %d, %s\n", ChrObjList.get_loop_depth(), __FUNCTION__ ); */ OBJ_LIST_BEGIN_LOOP_DEFINED(ego_obj_chr, ChrObjList, IT, internal__##PCHR##_pobj) \
    ego_chr * PCHR = internal__##PCHR##_pobj; if( NULL == PCHR ) continue; \
    CHR_REF IT(internal__##IT->first);

/// loops through ChrObjList for all "active" characters, creating a reference, and a pointer
#define CHR_BEGIN_LOOP_ACTIVE(IT, PCHR) \
    /* printf( "++++ ACTIVE ChrObjList.get_loop_depth() == %d, %s\n", ChrObjList.get_loop_depth(), __FUNCTION__ );*/ \
    OBJ_LIST_BEGIN_LOOP_ACTIVE(ego_obj_chr, ChrObjList, IT, internal__##PCHR##_pobj) \
    ego_chr * PCHR = (ego_chr *)static_cast<const ego_chr *>(internal__##PCHR##_pobj); \
    if( NULL == PCHR ) continue; \
    CHR_REF IT(internal__##IT->first);

/// loops through all "active" characters that are registered in the BSP
#define CHR_BEGIN_LOOP_BSP(IT, PCHR)     /*printf( "++++ BSP ChrObjList.get_loop_depth() == %d, %s\n", ChrObjList.get_loop_depth(), __FUNCTION__ );*/  CHR_BEGIN_LOOP_ACTIVE(IT, PCHR) if( !PCHR->bsp_leaf.inserted ) continue;

/// the termination for each ChrObjList loop
#define CHR_END_LOOP()                   OBJ_LIST_END_LOOP(ChrObjList) /*printf( "---- ChrObjList.get_loop_depth() == %d\n", ChrObjList.get_loop_depth() ); */

//--------------------------------------------------------------------------------------------
// Macros to determine whether the character is in the game or not.
// If objects are being spawned, then any object that is just "defined" is treated as "in game"
#define INGAME_CHR_BASE(ICHR)       ( ChrObjList.valid_ref( ICHR ) && ACTIVE_PBASE( ChrObjList.get_data_ptr(ICHR) ) && ON_PBASE( ChrObjList.get_data_ptr(ICHR) ) )
#define INGAME_PCHR_BASE(PCHR)      ( VALID_PCHR( PCHR ) && ACTIVE_PBASE( PCHR_CGET_POBJ(PCHR) ) && ON_PBASE( PCHR_CGET_POBJ(PCHR) ) )

#define INGAME_CHR(ICHR)            ( (ego_obj::get_spawn_depth()) > 0 ? DEFINED_CHR(ICHR) : INGAME_CHR_BASE(ICHR) )
#define INGAME_PCHR(PCHR)           ( (ego_obj::get_spawn_depth()) > 0 ? DEFINED_PCHR(PCHR) : INGAME_PCHR_BASE(PCHR) )

//--------------------------------------------------------------------------------------------
// list declaration
//--------------------------------------------------------------------------------------------
typedef t_ego_obj_lst<ego_obj_chr, MAX_CHR> ChrObjList_t;

extern ChrObjList_t ChrObjList;

//--------------------------------------------------------------------------------------------
// Function prototypes
//--------------------------------------------------------------------------------------------

#define _ChrList_h