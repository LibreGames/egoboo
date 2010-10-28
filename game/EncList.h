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

#include "egoboo_object_list.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct ego_enc;
struct ego_obj_enc;

typedef t_ego_obj_container< ego_obj_enc, MAX_ENC >  ego_enc_container;

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

// reference conversion
#define IENC_GET_POBJ( IENC )       EncObjList.get_data_ptr(IENC)
#define IENC_GET_PCONT( IENC )      EncObjList.get_ptr(IENC)

// grab the index from the ego_enc
#define GET_IDX_PENC( PENC )    ((NULL == PENC) ? MAX_ENC : INDEX_PCONT(ego_enc_container, PENC_CGET_PCONT(PENC)))
#define GET_REF_PENC( PENC )    ENC_REF(GET_IDX_PENC( PENC ))

// grab the allocated flag
#define ALLOCATED_ENC( IENC )    ego_enc_container::get_allocated( IENC_GET_PCONT(IENC) )
#define ALLOCATED_PENC( PENC )   ego_enc_container::get_allocated( PENC_CGET_PCONT(PENC) )

#define VALID_ENC( IENC )       ( ALLOCATED_ENC( IENC ) && FLAG_VALID_PBASE(IENC_GET_POBJ(IENC)) )
#define DEFINED_ENC( IENC )     ( VALID_PBASE(IENC_GET_POBJ(IENC) ) && !FLAG_TERMINATED_PBASE(IENC_GET_POBJ(IENC)) )
#define ACTIVE_ENC( IENC )      ( ACTIVE_PBASE(IENC_GET_POBJ(IENC)) )
#define WAITING_ENC( IENC )     ( WAITING_PBASE(IENC_GET_POBJ(IENC)) )
#define TERMINATED_ENC( IENC )  ( TERMINATED_PBASE(IENC_GET_POBJ(IENC)) )

#define VALID_PENC( PENC )       ( ALLOCATED_PENC( PENC ) && FLAG_VALID_PBASE(PENC_CGET_POBJ(PENC)) )
#define DEFINED_PENC( PENC )     ( VALID_PBASE(PENC_CGET_POBJ(PENC)) && !FLAG_TERMINATED_PBASE(PENC_CGET_POBJ(PENC)) )
#define ACTIVE_PENC( PENC )      ( ACTIVE_PBASE(PENC_CGET_POBJ(PENC)) )
#define WAITING_PENC( PENC )     ( WAITING_PBASE(PENC_CGET_POBJ(PENC)) )
#define TERMINATED_PENC( PENC )  ( TERMINATED_PBASE(PENC_CGET_POBJ(PENC)) )

//--------------------------------------------------------------------------------------------
// Macros to automate looping through a EncObjList. Based on generic code for
// looping through a t_ego_obj_lst<>
//
// This still looks a bit messy, but I think it can't be helped if we want the
// rest of our codebase to look "pretty"...

/// loops through EncObjList for all "defined" enchants, creating a reference, and a pointer
#define ENC_BEGIN_LOOP_DEFINED(IT, PENC) \
    /*printf( "++++ DEFINED EncObjList.get_loop_depth() == %d, %s\n", EncObjList.get_loop_depth(), __FUNCTION__ );*/ OBJ_LIST_BEGIN_LOOP_DEFINED(ego_obj_enc, EncObjList, IT, internal__##PENC##_pobj) \
    ego_enc * PENC = (ego_enc *)static_cast<const ego_enc *>(internal__##PENC##_pobj);\
    if( NULL == PENC ) continue; \
    ENC_REF IT(internal__##IT->first);

/// loops through EncObjList for all "active" enchants, creating a reference, and a pointer
#define ENC_BEGIN_LOOP_ACTIVE(IT, PENC) \
    /*printf( "++++ ACTIVE EncObjList.get_loop_depth() == %d, %s\n", EncObjList.get_loop_depth(), __FUNCTION__ );*/ \
    OBJ_LIST_BEGIN_LOOP_ACTIVE(ego_obj_enc, EncObjList, IT, internal__##PENC##_pobj) \
    ego_enc * PENC = (ego_enc *)static_cast<const ego_enc *>(internal__##PENC##_pobj); \
    if( NULL == PENC ) continue; \
    ENC_REF IT(internal__##IT->first);

/// the termination for each EncObjList loop
#define ENC_END_LOOP()                   OBJ_LIST_END_LOOP(EncObjList) /*printf( "---- EncObjList.get_loop_depth() == %d\n", EncObjList.get_loop_depth() );*/

//--------------------------------------------------------------------------------------------
// Macros to determine whether the enchant is in the game or not.
// If objects are being spawned, then any object that is just "defined" is treated as "in game"
#define INGAME_ENC_BASE(IENC)       ( EncObjList.in_range_ref( IENC ) && ACTIVE_PBASE( IENC_GET_POBJ(IENC) ) && ON_PBASE( IENC_GET_POBJ(IENC) ) )
#define INGAME_PENC_BASE(PENC)      ( VALID_PENC( PENC ) && ACTIVE_PBASE( PENC_CGET_POBJ(PENC) ) && ON_PBASE( PENC_CGET_POBJ(PENC) ) )

#define INGAME_ENC(IENC)            ( (ego_obj::get_spawn_depth()) > 0 ? DEFINED_ENC(IENC) : INGAME_ENC_BASE(IENC) )
#define INGAME_PENC(PENC)           ( (ego_obj::get_spawn_depth()) > 0 ? DEFINED_PENC(PENC) : INGAME_PENC_BASE(PENC) )

//--------------------------------------------------------------------------------------------
// list dclaration
//--------------------------------------------------------------------------------------------
typedef t_ego_obj_lst<ego_obj_enc, MAX_ENC> EncObjList_t;

extern EncObjList_t EncObjList;

//--------------------------------------------------------------------------------------------
// Function prototypes
//--------------------------------------------------------------------------------------------

#define _EncList_h