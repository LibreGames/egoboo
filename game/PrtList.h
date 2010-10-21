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

/// @file PrtList.h
/// @brief Routines for particle list management

#include "egoboo_object.h"
#include "particle.h"

#include "egoboo_object_list.inl"

//--------------------------------------------------------------------------------------------
// MACROS
//--------------------------------------------------------------------------------------------

#define VALID_PRT_IDX( INDX )   ( ((INDX) >= 0) && ((INDX) < MAX_PRT) )
#define VALID_PRT_REF( IPRT )     PrtObjList.validate_ref(IPRT)
#define ALLOCATED_PRT( IPRT )   ( VALID_PRT_REF( IPRT ) && ALLOCATED_PBASE (POBJ_CGET_PBASE(PrtObjList.get_ptr(IPRT))) )
#define VALID_PRT( IPRT )       ( VALID_PRT_REF( IPRT ) && VALID_PBASE (POBJ_CGET_PBASE(PrtObjList.get_ptr(IPRT))) )
#define DEFINED_PRT( IPRT )     ( VALID_PRT_REF( IPRT ) && VALID_PBASE (POBJ_CGET_PBASE(PrtObjList.get_ptr(IPRT)) ) && !TERMINATED_PBASE ( POBJ_CGET_PBASE(PrtObjList.get_ptr(IPRT)) ) )
#define ACTIVE_PRT( IPRT )      ( VALID_PRT_REF( IPRT ) && ACTIVE_PBASE    (POBJ_CGET_PBASE(PrtObjList.get_ptr(IPRT))) )
#define WAITING_PRT( IPRT )     ( VALID_PRT_REF( IPRT ) && WAITING_PBASE   (POBJ_CGET_PBASE(PrtObjList.get_ptr(IPRT))) )
#define TERMINATED_PRT( IPRT )  ( VALID_PRT_REF( IPRT ) && TERMINATED_PBASE(POBJ_CGET_PBASE(PrtObjList.get_ptr(IPRT))) )

#define GET_IDX_PPRT( PPRT )    ((size_t)GET_IDX_POBJ( PDATA_CGET_PBASE(PPRT), MAX_PRT ))
#define GET_REF_PPRT( PPRT )    ((PRT_REF)GET_REF_POBJ( PDATA_CGET_PBASE(PPRT), MAX_PRT ))
#define VALID_PRT_PTR( PPRT )   ( (NULL != (PPRT)) && VALID_PRT_REF( GET_REF_PPRT( PPRT ) ) )
#define ALLOCATED_PPRT( PPRT )  ( VALID_PRT_PTR( PPRT ) && ALLOCATED_PBASE( PDATA_CGET_PBASE(PPRT) ) )
#define VALID_PPRT( PPRT )      ( VALID_PRT_PTR( PPRT ) && VALID_PBASE( PDATA_CGET_PBASE(PPRT) ) )
#define DEFINED_PPRT( PPRT )    ( VALID_PRT_PTR( PPRT ) && VALID_PBASE (PDATA_CGET_PBASE(PPRT)) && !TERMINATED_PBASE (PDATA_CGET_PBASE(PPRT)) )
#define TERMINATED_PPRT( PPRT ) ( VALID_PRT_PTR( PPRT ) && TERMINATED_PBASE(PDATA_CGET_PBASE(PPRT)) )
#define ACTIVE_PPRT( PPRT )     ( VALID_PRT_PTR( PPRT ) && ACTIVE_PBASE(PDATA_CGET_PBASE(PPRT)) )

#define GET_IDX_PPRT_OBJ( PPRT_OBJ )        ((size_t)GET_IDX_POBJ( PPRT_OBJ, MAX_PRT ))
#define GET_REF_PPRT_OBJ( PPRT_OBJ )        ((PRT_REF)GET_REF_POBJ( PPRT_OBJ, MAX_PRT ))
#define VALID_PRT_OBJ_PTR( PPRT_OBJ )       ( (NULL != (PPRT_OBJ)) && VALID_PRT_OBJ_REF( GET_REF_PPRT_OBJ( PPRT_OBJ ) ) )
#define ALLOCATED_PPRT_OBJ( PPRT_OBJ )      ( VALID_PRT_OBJ_PTR( PPRT_OBJ ) && ALLOCATED_PBASE( POBJ_CGET_PBASE(PPRT_OBJ) ) )
#define VALID_PPRT_OBJ( PPRT_OBJ )          ( VALID_PRT_OBJ_PTR( PPRT_OBJ ) && VALID_PBASE( POBJ_CGET_PBASE(PPRT_OBJ) ) )
#define DEFINED_PPRT_OBJ( PPRT_OBJ )        ( VALID_PRT_OBJ_PTR( PPRT_OBJ ) && VALID_PBASE ( POBJ_CGET_PBASE(PPRT_OBJ) ) && !TERMINATED_PBASE ( POBJ_CGET_PBASE(PPRT_OBJ) ) )
#define ACTIVE_PPRT_OBJ( PPRT_OBJ )         ( VALID_PRT_OBJ_PTR( PPRT_OBJ ) && ACTIVE_PBASE( POBJ_CGET_PBASE(PPRT_OBJ) ) )
#define TERMINATED_PPRT_OBJ( PPRT_OBJ )     ( VALID_PRT_OBJ_PTR( PPRT_OBJ ) && TERMINATED_PBASE( POBJ_CGET_PBASE(PPRT_OBJ) ) )

/// Is the particle flagged as being in limbo?
#define FLAG_LIMBO_PPRT_OBJ( PPRT_OBJ )  ( (PPRT_OBJ)->obj_base_display )
/// Is the particle object in limbo?
#define STATE_LIMBO_PPRT_OBJ( PPRT_OBJ ) ( VALID_PBASE( POBJ_CGET_PBASE(PPRT_OBJ) ) && FLAG_LIMBO_PPRT_OBJ(PPRT_OBJ) )

//--------------------------------------------------------------------------------------------
// Macros to automate looping through a PrtObjList. Based on generic code for
// looping through a t_ego_obj_lst<>
//
// This still looks a bit messy, but I think it can't be helped if we want the
// rest of our codebase to look "pretty"...

/// loops through PrtObjList for all "used" particles, creating a reference, and a bundle
#define PRT_BEGIN_LOOP_USED(IT, PRT_BDL) \
    OBJ_LIST_BEGIN_LOOP(ego_obj_prt, PrtObjList, IT, internal__##PRT_BDL##_pobj) \
    ego_prt * internal__##PRT_BDL_pprt = (ego_prt *)internal__##PRT_BDL##_pobj->cget_pdata(); \
    if( NULL == internal__##PRT_BDL_pprt ) continue; \
    PRT_REF IT(internal__##IT->first); \
    ego_prt_bundle PRT_BDL( internal__##PRT_BDL_pprt );

/// loops through PrtObjList for all "defined" particles, creating a reference, and a bundle
#define PRT_BEGIN_LOOP_DEFINED(IT, PRT_BDL) \
    OBJ_LIST_BEGIN_LOOP_DEFINED(ego_obj_prt, PrtObjList, IT, internal__##PRT_BDL##_pobj) \
    ego_prt * internal__##PRT_BDL_pprt = (ego_prt *)internal__##PRT_BDL##_pobj->cget_pdata(); \
    if( NULL == internal__##PRT_BDL_pprt ) continue; \
    PRT_REF IT(internal__##IT->first); \
    ego_prt_bundle PRT_BDL( internal__##PRT_BDL_pprt );

/// loops through PrtObjList for all "active" particles, creating a reference, and a bundle
#define PRT_BEGIN_LOOP_ACTIVE(IT, PRT_BDL) \
    OBJ_LIST_BEGIN_LOOP_ACTIVE(ego_obj_prt, PrtObjList, IT, internal__##PRT_BDL##_pobj) \
    ego_prt * internal__##PRT_BDL_pprt = (ego_prt *)internal__##PRT_BDL##_pobj->cget_pdata(); \
    if( NULL == internal__##PRT_BDL_pprt ) continue; \
    PRT_REF IT(internal__##IT->first); \
    ego_prt_bundle PRT_BDL( internal__##PRT_BDL_pprt );

/// loops through PrtObjList for all "active" particles that are registered in the BSP
#define PRT_BEGIN_LOOP_BSP(IT, PRT_BDL)     PRT_BEGIN_LOOP_ACTIVE(IT, PPRT) if( !PPRT->bsp_leaf.inserted ) continue;

/// loops through PrtObjList for all "defined" particles that are flagged as being in limbo
#define PRT_BEGIN_LOOP_LIMBO(IT, PRT_BDL)   PRT_BEGIN_LOOP_DEFINED(IT, PPRT) if( !LIMBO_PPRT(PPRT) ) continue;

/// the termination for each PrtObjList loop
#define PRT_END_LOOP()                   OBJ_LIST_END_LOOP(PrtObjList)

//--------------------------------------------------------------------------------------------
// Macros to determine whether the particle is in the game or not.
// If objects are being spawned, then any object that is just "defined" is treated as "in game"
#define INGAME_PRT_BASE(IPRT)       ( VALID_PRT ( IPRT ) && FLAG_ON_PBASE( POBJ_CGET_PBASE(PrtObjList.get_ptr(IPRT)) )  )
#define INGAME_PPRT_BASE(PPRT)      ( VALID_PPRT( PPRT ) && FLAG_ON_PBASE( PDATA_CGET_PBASE(PPRT) ) )

#define INGAME_PRT(IPRT)            ( ( (ego_obj::spawn_depth > 0) ? DEFINED_PRT(IPRT) : INGAME_PRT_BASE(IPRT) ) && !FLAG_LIMBO_PPRT_OBJ( PrtObjList.get_ptr(IPRT) ) )
#define INGAME_PPRT(PPRT)           ( ( (ego_obj::spawn_depth > 0) ? DEFINED_PPRT(PPRT) : INGAME_PPRT_BASE(PPRT) ) && !FLAG_LIMBO_PPRT_OBJ( PDATA_GET_POBJ(PPRT) ) )

// the same as INGAME_*_BASE except that the particle does not have to be "on"
// visible particles stick around in the active state (but off) until they have been displayed at least once
#define LIMBO_PRT(IPRT)            ( ( (ego_obj::spawn_depth > 0) ? DEFINED_PRT(IPRT) : INGAME_PRT_BASE(IPRT) ) && FLAG_LIMBO_PPRT_OBJ( PrtObjList.get_ptr(IPRT) ) )
#define LIMBO_PPRT(PPRT)           ( ( (ego_obj::spawn_depth > 0) ? DEFINED_PPRT(PPRT) : INGAME_PPRT_BASE(PPRT) ) && FLAG_LIMBO_PPRT_OBJ( PDATA_GET_POBJ(PPRT) ) )

//--------------------------------------------------------------------------------------------
// list definition
//--------------------------------------------------------------------------------------------
struct ego_particle_list : public t_ego_obj_lst<ego_obj_prt, MAX_PRT>
{

    PRT_REF allocate( bool_t force, const PRT_REF & override = PRT_REF( MAX_PRT ) );

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