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

/// \file PrtList.h
/// \brief Routines for particle list management

#include "egoboo_object.h"
#include "particle.h"

#include "egoboo_object_list.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct ego_prt;
struct ego_obj_prt;

typedef t_ego_obj_container< ego_obj_prt, MAX_PRT >  ego_prt_container;

//--------------------------------------------------------------------------------------------
// MACROS
//--------------------------------------------------------------------------------------------

// pointer conversion
#define PPRT_CGET_POBJ( PPRT )       ego_prt::cget_obj_ptr(PPRT)
#define POBJ_PRT_CGET_PCONT( POBJ )  ego_obj_prt::cget_container_ptr( POBJ )
#define PPRT_CGET_PCONT( PPRT )      POBJ_PRT_CGET_PCONT( PPRT_CGET_POBJ(PPRT) )

#define PPRT_GET_POBJ( PPRT )       ego_prt::get_obj_ptr(PPRT)
#define POBJ_PRT_GET_PCONT( POBJ )  ego_obj_prt::get_container_ptr( POBJ )
#define PPRT_GET_PCONT( PPRT )      POBJ_PRT_GET_PCONT( PPRT_GET_POBJ(PPRT) )

// reference conversion
#define IPRT_GET_POBJ( IPRT )       PrtObjList.get_data_ptr(IPRT)
#define IPRT_GET_PCONT( IPRT )      PrtObjList.get_ptr(IPRT)

// grab the index from the ego_prt
#define GET_IDX_PPRT( PPRT )    GET_INDEX_PCONT( ego_prt_container, PPRT_CGET_PCONT(PPRT), MAX_PRT )
#define GET_REF_PPRT( PPRT )    PRT_REF(GET_IDX_PPRT( PPRT ))

// grab the allocated flag
#define ALLOCATED_PRT( IPRT )    FLAG_ALLOCATED_PCONT(ego_prt_container,  IPRT_GET_PCONT(IPRT) )
#define ALLOCATED_PPRT( PPRT )   FLAG_ALLOCATED_PCONT(ego_prt_container,  PPRT_CGET_PCONT(PPRT) )

#define VALID_PRT( IPRT )          ( ALLOCATED_PRT(IPRT) && FLAG_VALID_PBASE(IPRT_GET_POBJ(IPRT)) )
#define DEFINED_PRT( IPRT )        ( VALID_PRT(IPRT)     && !FLAG_TERMINATED_PBASE(IPRT_GET_POBJ(IPRT)) )
#define TERMINATED_PRT( IPRT )     ( VALID_PRT(IPRT)     && FLAG_TERMINATED_PBASE(IPRT_GET_POBJ(IPRT)) )
#define CONSTRUCTING_PRT( IPRT )   ( ALLOCATED_PRT(IPRT) && CONSTRUCTING_PBASE(IPRT_GET_POBJ(IPRT)) )
#define INITIALIZING_PRT( IPRT )   ( ALLOCATED_PRT(IPRT) && INITIALIZING_PBASE(IPRT_GET_POBJ(IPRT)) )
#define PROCESSING_PRT( IPRT )     ( ALLOCATED_PRT(IPRT) && PROCESSING_PBASE(IPRT_GET_POBJ(IPRT)) )
#define DEINITIALIZING_PRT( IPRT ) ( ALLOCATED_PRT(IPRT) && DEINITIALIZING_PBASE(IPRT_GET_POBJ(IPRT)) )
#define WAITING_PRT( IPRT )        ( ALLOCATED_PRT(IPRT) && WAITING_PBASE(IPRT_GET_POBJ(IPRT)) )

#define VALID_PPRT( PPRT )          ( ALLOCATED_PPRT(PPRT) && FLAG_VALID_PBASE(PPRT_CGET_POBJ(PPRT)) )
#define DEFINED_PPRT( PPRT )        ( VALID_PPRT(PPRT)     && !FLAG_TERMINATED_PBASE(PPRT_CGET_POBJ(PPRT)) )
#define TERMINATED_PPRT( PPRT )     ( VALID_PPRT(PPRT)     && FLAG_TERMINATED_PBASE(PPRT_CGET_POBJ(PPRT)) )
#define CONSTRUCTING_PPRT( PPRT )   ( ALLOCATED_PPRT(PPRT) && CONSTRUCTING_PBASE(PPRT_CGET_POBJ(PPRT)) )
#define INITIALIZING_PPRT( PPRT )   ( ALLOCATED_PPRT(PPRT) && INITIALIZING_PBASE(PPRT_CGET_POBJ(PPRT)) )
#define PROCESSING_PPRT( PPRT )     ( ALLOCATED_PPRT(PPRT) && PROCESSING_PBASE(PPRT_CGET_POBJ(PPRT)) )
#define DEINITIALIZING_PPRT( PPRT ) ( ALLOCATED_PPRT(PPRT) && DEINITIALIZING_PBASE(PPRT_CGET_POBJ(PPRT)) )
#define WAITING_PPRT( PPRT )        ( ALLOCATED_PPRT(PPRT) && WAITING_PBASE(PPRT_CGET_POBJ(PPRT)) )

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

/// loops through PrtObjList for all "defined" particles that are flagged as being in limbo
#define PRT_BEGIN_LOOP_LIMBO(IT, PPRT) \
    PRT_BEGIN_LOOP_DEFINED(IT, PPRT) \
    if( !LIMBO_PPRT(PPRT) ) continue;

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

/// loops through PrtObjList for all "defined" particles that are flagged as being in limbo
#define PRT_BEGIN_LOOP_LIMBO_BDL(IT, PRT_BDL) \
    PRT_BEGIN_LOOP_LIMBO( IT, PRT_BDL##_ptr ) \
    PRT_LOOP_WRAP_BDL( PRT_BDL, PRT_BDL##_ptr )

/// the termination for each PrtObjList loop
#define PRT_END_LOOP() \
    OBJ_LIST_END_LOOP(PrtObjList)

//--------------------------------------------------------------------------------------------
// Macros to determine whether the particle is in the game or not.
// If objects are being spawned, then any object that is just "defined" is treated as "in game"

#define INGAME_PRT(IPRT)            ( (ego_obj::get_spawn_depth()) > 0 ? DEFINED_PRT(IPRT) : ALLOCATED_PRT(IPRT) && PROCESSING_PBASE(IPRT_GET_POBJ(IPRT)) )
#define INGAME_PPRT(PPRT)           ( (ego_obj::get_spawn_depth()) > 0 ? DEFINED_PPRT(PPRT) : ALLOCATED_PPRT(PPRT) && PROCESSING_PBASE(PPRT_CGET_POBJ(PPRT)) )

// the same as INGAME_*_BASE except that the particle does not have to be "on"
// visible particles stick around in the active state (but off) until they have been displayed at least once
#define LIMBO_PRT(IPRT)            ( FLAG_INITIALIZED_PBASE(IPRT_GET_POBJ(IPRT)) && !FLAG_ON_PBASE(IPRT_GET_POBJ(IPRT)) && FLAG_DISPLAY_PPRT_OBJ( IPRT_GET_POBJ(IPRT) ) )
#define LIMBO_PPRT(PPRT)           ( FLAG_INITIALIZED_PBASE(PDATA_GET_POBJ(ego_prt, PPRT)) && !FLAG_ON_PBASE(PDATA_GET_POBJ(ego_prt, PPRT)) && FLAG_DISPLAY_PPRT_OBJ( PDATA_GET_POBJ(ego_prt, PPRT) ) )

//--------------------------------------------------------------------------------------------
// list definition
//--------------------------------------------------------------------------------------------
struct ego_particle_list : public t_obj_lst_deque<ego_obj_prt, MAX_PRT>
{
    typedef t_obj_lst_deque<ego_obj_prt, MAX_PRT> list_type;

    PRT_REF allocate( bool_t force, const PRT_REF & override = PRT_REF( MAX_PRT ) );

    ego_particle_list( size_t len = 512 ) : t_obj_lst_deque<ego_obj_prt, MAX_PRT>( len ) {}

protected:
    PRT_REF allocate_find();
    PRT_REF allocate_activate( const PRT_REF & iprt );
};

/// We have to define this so that the looping macros will work correctly
typedef ego_particle_list PrtObjList_t;

extern PrtObjList_t PrtObjList;

//--------------------------------------------------------------------------------------------
// Function prototypes
//--------------------------------------------------------------------------------------------

#define _PrtList_h