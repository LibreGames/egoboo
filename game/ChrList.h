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
#include "char.h"

#include "egoboo_object_list.inl"

//--------------------------------------------------------------------------------------------
// MACROS
//--------------------------------------------------------------------------------------------

#define VALID_CHR_IDX( INDX )   ( ((INDX) >= 0) && ((INDX) < MAX_CHR) )
#define VALID_CHR_REF( ICHR )     ChrObjList.validate_ref(ICHR)
#define ALLOCATED_CHR( ICHR )   ( ALLOCATED_PBASE(ChrObjList.get_valid_ptr(ICHR)) )
#define VALID_CHR( ICHR )       ( VALID_PBASE(ChrObjList.get_valid_ptr(ICHR)) )
#define DEFINED_CHR( ICHR )     ( VALID_PBASE(ChrObjList.get_valid_ptr(ICHR) ) && !FLAG_TERMINATED_PBASE(ChrObjList.get_valid_ptr(ICHR)) )
#define ACTIVE_CHR( ICHR )      ( ACTIVE_PBASE(ChrObjList.get_valid_ptr(ICHR)) )
#define WAITING_CHR( ICHR )     ( WAITING_PBASE(ChrObjList.get_valid_ptr(ICHR)) )
#define TERMINATED_CHR( ICHR )  ( TERMINATED_PBASE(ChrObjList.get_valid_ptr(ICHR)) )

#define GET_IDX_PCHR( PCHR )    size_t(GET_IDX_POBJ( PDATA_CGET_PBASE( ego_chr, PCHR), MAX_CHR ))
#define GET_REF_PCHR( PCHR )    CHR_REF(GET_REF_POBJ( PDATA_CGET_PBASE( ego_chr, PCHR), MAX_CHR ))

#define VALID_CHR_PTR( PCHR )   ( (NULL != (PCHR)) && VALID_CHR_REF( GET_REF_PCHR( PCHR ) ) )

#define ALLOCATED_PCHR( PCHR )   ( ALLOCATED_PBASE(PDATA_CGET_PBASE( ego_chr, PCHR )) )
#define VALID_PCHR( PCHR )       ( VALID_PBASE(PDATA_CGET_PBASE( ego_chr, PCHR )) )
#define DEFINED_PCHR( PCHR )     ( VALID_PBASE(PDATA_CGET_PBASE( ego_chr, PCHR ) ) && !FLAG_TERMINATED_PBASE(PDATA_CGET_PBASE( ego_chr, PCHR )) )
#define ACTIVE_PCHR( PCHR )      ( ACTIVE_PBASE(PDATA_CGET_PBASE( ego_chr, PCHR )) )
#define WAITING_PCHR( PCHR )     ( WAITING_PBASE(PDATA_CGET_PBASE( ego_chr, PCHR )) )
#define TERMINATED_PCHR( PCHR )  ( TERMINATED_PBASE(PDATA_CGET_PBASE( ego_chr, PCHR )) )

#define GET_REF_PCHR_OBJ( POBJ )    ((CHR_REF)GET_REF_POBJ( POBJ, MAX_CHR ))

//--------------------------------------------------------------------------------------------
// Macros to automate looping through a ChrObjList. Based on generic code for
// looping through a t_ego_obj_lst<>
//
// This still looks a bit messy, but I think it can't be helped if we want the
// rest of our codebase to look "pretty"...

/// loops through ChrObjList for all "defined" characters, creating a reference, and a pointer
#define CHR_BEGIN_LOOP_DEFINED(IT, PCHR) \
    /* printf( "++++ DEFINED ChrObjList.loop_depth == %d, %s\n", ChrObjList.loop_depth, __FUNCTION__ ); */ OBJ_LIST_BEGIN_LOOP_DEFINED(ego_obj_chr, ChrObjList, IT, internal__##PCHR##_pobj) \
    ego_chr * PCHR = (ego_chr *)internal__##PCHR##_pobj->cget_pdata(); if( NULL == PCHR ) continue; \
    CHR_REF IT(internal__##IT->first);

/// loops through ChrObjList for all "active" characters, creating a reference, and a pointer
#define CHR_BEGIN_LOOP_ACTIVE(IT, PCHR) \
    /* printf( "++++ ACTIVE ChrObjList.loop_depth == %d, %s\n", ChrObjList.loop_depth, __FUNCTION__ );*/ OBJ_LIST_BEGIN_LOOP_ACTIVE(ego_obj_chr, ChrObjList, IT, internal__##PCHR##_pobj) \
    ego_chr * PCHR = (ego_chr *)internal__##PCHR##_pobj->cget_pdata(); if( NULL == PCHR ) continue; \
    CHR_REF IT(internal__##IT->first);

/// loops through all "active" characters that are registered in the BSP
#define CHR_BEGIN_LOOP_BSP(IT, PCHR)     /*printf( "++++ BSP ChrObjList.loop_depth == %d, %s\n", ChrObjList.loop_depth, __FUNCTION__ );*/  CHR_BEGIN_LOOP_ACTIVE(IT, PCHR) if( !PCHR->bsp_leaf.inserted ) continue;

/// the termination for each ChrObjList loop
#define CHR_END_LOOP()                   OBJ_LIST_END_LOOP(ChrObjList) /*printf( "---- ChrObjList.loop_depth == %d\n", ChrObjList.loop_depth ); */

//--------------------------------------------------------------------------------------------
// Macros to determine whether the character is in the game or not.
// If objects are being spawned, then any object that is just "defined" is treated as "in game"
#define INGAME_CHR_BASE(ICHR)       ( VALID_CHR_REF( ICHR ) && ACTIVE_PBASE( POBJ_CGET_PBASE(ChrObjList.get_ptr(ICHR)) ) && ON_PBASE( POBJ_CGET_PBASE(ChrObjList.get_ptr(ICHR)) ) )
#define INGAME_PCHR_BASE(PCHR)      ( VALID_CHR_PTR( PCHR ) && ACTIVE_PBASE( PDATA_CGET_PBASE( ego_chr, PCHR) ) && ON_PBASE( PDATA_CGET_PBASE( ego_chr, PCHR) ) )

#define INGAME_CHR(ICHR)            ( (ego_obj::spawn_depth) > 0 ? DEFINED_CHR(ICHR) : INGAME_CHR_BASE(ICHR) )
#define INGAME_PCHR(PCHR)           ( (ego_obj::spawn_depth) > 0 ? DEFINED_PCHR(PCHR) : INGAME_PCHR_BASE(PCHR) )

//--------------------------------------------------------------------------------------------
// list declaration
//--------------------------------------------------------------------------------------------
typedef t_ego_obj_lst<ego_obj_chr, MAX_CHR> ChrObjList_t;

extern ChrObjList_t ChrObjList;

//--------------------------------------------------------------------------------------------
// Function prototypes
//--------------------------------------------------------------------------------------------

#define _ChrList_h