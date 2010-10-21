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

/// @file EncList.h
/// @brief Routines for enchant list management

#include "egoboo_object.h"
#include "enchant.h"

#include "egoboo_object_list.inl"

//--------------------------------------------------------------------------------------------
// MACROS
//--------------------------------------------------------------------------------------------

#define VALID_ENC_IDX( INDX )      ( ((INDX) >= 0) && ((INDX) < MAX_ENC) )
#define VALID_ENC_REF( IENC )      EncObjList.validate_ref(IENC)
#define ALLOCATED_ENC( IENC )      ( VALID_ENC_REF( IENC ) && ALLOCATED_PBASE ( POBJ_CGET_PBASE(EncObjList.get_ptr(IENC)) ) )
#define VALID_ENC( IENC )          ( VALID_ENC_REF( IENC ) && VALID_PBASE     ( POBJ_CGET_PBASE(EncObjList.get_ptr(IENC)) ) )
#define DEFINED_ENC( IENC )        ( VALID_ENC_REF( IENC ) && VALID_PBASE     ( POBJ_CGET_PBASE(EncObjList.get_ptr(IENC)) ) && !TERMINATED_PBASE ( POBJ_CGET_PBASE(EncObjList.get_ptr(IENC)) ) )
#define ACTIVE_ENC( IENC )         ( VALID_ENC_REF( IENC ) && ACTIVE_PBASE    ( POBJ_CGET_PBASE(EncObjList.get_ptr(IENC)) ) )
#define WAITING_ENC( IENC )        ( VALID_ENC_REF( IENC ) && WAITING_PBASE   ( POBJ_CGET_PBASE(EncObjList.get_ptr(IENC)) ) )
#define TERMINATED_ENC( IENC )     ( VALID_ENC_REF( IENC ) && TERMINATED_PBASE( POBJ_CGET_PBASE(EncObjList.get_ptr(IENC)) ) )

#define GET_IDX_PENC( PENC )        ((size_t)GET_IDX_POBJ( PDATA_CGET_POBJ(PENC), MAX_ENC ))
#define GET_REF_PENC( PENC )        ((ENC_REF)GET_REF_POBJ( PDATA_CGET_POBJ(PENC), MAX_ENC ))
#define VALID_ENC_PTR( PENC )       ( (NULL != (PENC)) && VALID_ENC_REF( GET_REF_PENC( PENC ) ) )
#define ALLOCATED_PENC( PENC )      ( VALID_ENC_PTR( PENC ) && ALLOCATED_PBASE( PDATA_CGET_PBASE(PENC) ) )
#define VALID_PENC( PENC )          ( VALID_ENC_PTR( PENC ) && VALID_PBASE( PDATA_CGET_PBASE(PENC) ) )
#define DEFINED_PENC( PENC )        ( VALID_ENC_PTR( PENC ) && VALID_PBASE ( PDATA_CGET_PBASE(PENC) ) && !TERMINATED_PBASE ( PDATA_CGET_PBASE(PENC) ) )
#define ACTIVE_PENC( PENC )         ( VALID_ENC_PTR( PENC ) && ACTIVE_PBASE( PDATA_CGET_PBASE(PENC) ) )
#define TERMINATED_PENC( PENC )     ( VALID_ENC_PTR( PENC ) && TERMINATED_PBASE( PDATA_CGET_PBASE(PENC) ) )

#define GET_IDX_PENC_OBJ( PENC_OBJ )        ((size_t)GET_IDX_POBJ( PENC_OBJ, MAX_ENC ))
#define GET_REF_PENC_OBJ( PENC_OBJ )        ((ENC_REF)GET_REF_POBJ( PENC_OBJ, MAX_ENC ))
#define VALID_ENC_OBJ_PTR( PENC_OBJ )       ( (NULL != (PENC_OBJ)) && VALID_ENC_OBJ_REF( GET_REF_PENC_OBJ( PENC_OBJ ) ) )
#define ALLOCATED_PENC_OBJ( PENC_OBJ )      ( VALID_ENC_OBJ_PTR( PENC_OBJ ) && ALLOCATED_PBASE( POBJ_CGET_PBASE(PENC_OBJ) ) )
#define VALID_PENC_OBJ( PENC_OBJ )          ( VALID_ENC_OBJ_PTR( PENC_OBJ ) && VALID_PBASE( POBJ_CGET_PBASE(PENC_OBJ) ) )
#define DEFINED_PENC_OBJ( PENC_OBJ )        ( VALID_ENC_OBJ_PTR( PENC_OBJ ) && VALID_PBASE ( POBJ_CGET_PBASE(PENC_OBJ) ) && !TERMINATED_PBASE ( POBJ_CGET_PBASE(PENC_OBJ) ) )
#define ACTIVE_PENC_OBJ( PENC_OBJ )         ( VALID_ENC_OBJ_PTR( PENC_OBJ ) && ACTIVE_PBASE( POBJ_CGET_PBASE(PENC_OBJ) ) )
#define TERMINATED_PENC_OBJ( PENC_OBJ )     ( VALID_ENC_OBJ_PTR( PENC_OBJ ) && TERMINATED_PBASE( POBJ_CGET_PBASE(PENC_OBJ) ) )

//--------------------------------------------------------------------------------------------
// Macros to automate looping through a EncObjList. Based on generic code for
// looping through a t_ego_obj_lst<>
//
// This still looks a bit messy, but I think it can't be helped if we want the
// rest of our codebase to look "pretty"...

/// loops through EncObjList for all "defined" enchants, creating a reference, and a pointer
#define ENC_BEGIN_LOOP_DEFINED(IT, PENC) \
    OBJ_LIST_BEGIN_LOOP_DEFINED(ego_obj_enc, EncObjList, IT, internal__##PENC##_pobj) \
    ego_enc * PENC = (ego_enc *)internal__##PENC##_pobj->cget_pdata(); if( NULL == PENC ) continue; \
    ENC_REF IT(internal__##IT->first);

/// loops through EncObjList for all "active" enchants, creating a reference, and a pointer
#define ENC_BEGIN_LOOP_ACTIVE(IT, PENC) \
    OBJ_LIST_BEGIN_LOOP_ACTIVE(ego_obj_enc, EncObjList, IT, internal__##PENC##_pobj) \
    ego_enc * PENC = (ego_enc *)internal__##PENC##_pobj->cget_pdata(); if( NULL == PENC ) continue; \
    ENC_REF IT(internal__##IT->first);

/// the termination for each EncObjList loop
#define ENC_END_LOOP()                   OBJ_LIST_END_LOOP(EncObjList)

//--------------------------------------------------------------------------------------------
// Macros to determine whether the enchant is in the game or not.
// If objects are being spawned, then any object that is just "defined" is treated as "in game"
#define INGAME_ENC_BASE(IENC)       ( VALID_ENC_REF( IENC ) && ACTIVE_PBASE( POBJ_CGET_PBASE(EncObjList.get_ptr(IENC)) ) && ON_PBASE( POBJ_CGET_PBASE(EncObjList.get_ptr(IENC)) ) )
#define INGAME_PENC_BASE(PENC)      ( VALID_ENC_PTR( PENC ) && ACTIVE_PBASE( POBJ_CGET_PBASE(PENC) ) && ON_PBASE( POBJ_CGET_PBASE(PENC) ) )

#define INGAME_ENC(IENC)            ( (ego_obj::spawn_depth) > 0 ? DEFINED_ENC(IENC) : INGAME_ENC_BASE(IENC) )
#define INGAME_PENC(PENC)           ( (ego_obj::spawn_depth) > 0 ? DEFINED_PENC(PENC) : INGAME_PENC_BASE(PENC) )

//--------------------------------------------------------------------------------------------
// list dclaration
//--------------------------------------------------------------------------------------------
typedef t_ego_obj_lst<ego_obj_enc, MAX_ENC> EncObjList_t;

extern EncObjList_t EncObjList;

//--------------------------------------------------------------------------------------------
// Function prototypes
//--------------------------------------------------------------------------------------------

#define _EncList_h