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
#define ALLOCATED_PRT( IPRT )   ( VALID_PRT_REF( IPRT ) && ALLOCATED_PBASE (POBJ_CGET_PBASE(PrtObjList.lst + (IPRT))) )
#define VALID_PRT( IPRT )       ( VALID_PRT_REF( IPRT ) && VALID_PBASE (POBJ_CGET_PBASE(PrtObjList.lst + (IPRT))) )
#define DEFINED_PRT( IPRT )     ( VALID_PRT_REF( IPRT ) && VALID_PBASE (POBJ_CGET_PBASE(PrtObjList.lst + (IPRT)) ) && !TERMINATED_PBASE ( POBJ_CGET_PBASE(PrtObjList.lst + (IPRT)) ) )
#define ACTIVE_PRT( IPRT )      ( VALID_PRT_REF( IPRT ) && ACTIVE_PBASE    (POBJ_CGET_PBASE(PrtObjList.lst + (IPRT))) )
#define WAITING_PRT( IPRT )     ( VALID_PRT_REF( IPRT ) && WAITING_PBASE   (POBJ_CGET_PBASE(PrtObjList.lst + (IPRT))) )
#define TERMINATED_PRT( IPRT )  ( VALID_PRT_REF( IPRT ) && TERMINATED_PBASE(POBJ_CGET_PBASE(PrtObjList.lst + (IPRT))) )

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

// Macros automate looping through the PrtObjList. This hides code which defers the creation and deletion of
// objects until the loop terminates, so that the length of the list will not change during the loop.
#define PRT_BEGIN_LOOP_USED(IT, PRT_BDL)   {size_t IT##_internal; int prt_loop_start_depth = PrtObjList.loop_depth; PrtObjList.loop_depth++; for(IT##_internal=0;IT##_internal<PrtObjList.used_count;IT##_internal++) { PRT_REF IT; ego_prt_bundle PRT_BDL; IT = (PRT_REF)PrtObjList.used_ref[IT##_internal]; if(!DEFINED_PRT (IT)) continue; ego_prt_bundle::set(&PRT_BDL, PrtObjList.get_valid_pdata(IT));
#define PRT_BEGIN_LOOP_ACTIVE(IT, PRT_BDL) {size_t IT##_internal; int prt_loop_start_depth = PrtObjList.loop_depth; PrtObjList.loop_depth++; for(IT##_internal=0;IT##_internal<PrtObjList.used_count;IT##_internal++) { PRT_REF IT; ego_prt_bundle PRT_BDL; IT = (PRT_REF)PrtObjList.used_ref[IT##_internal]; if(!ACTIVE_PRT (IT)) continue; ego_prt_bundle::set(&PRT_BDL, PrtObjList.get_valid_pdata(IT));
#define PRT_BEGIN_LOOP_LIMBO(IT, PRT_BDL)  {size_t IT##_internal; int prt_loop_start_depth = PrtObjList.loop_depth; PrtObjList.loop_depth++; for(IT##_internal=0;IT##_internal<PrtObjList.used_count;IT##_internal++) { PRT_REF IT; ego_prt_bundle PRT_BDL; IT = (PRT_REF)PrtObjList.used_ref[IT##_internal]; if(!LIMBO_PRT(IT)) continue; ego_prt_bundle::set(&PRT_BDL, PrtObjList.get_valid_pdata(IT));
#define PRT_END_LOOP() } PrtObjList.loop_depth--; if(prt_loop_start_depth != PrtObjList.loop_depth) EGOBOO_ASSERT(bfalse); PrtObjList.cleanup(); }

/// Is the particle flagged as being in limbo?
#define FLAG_LIMBO_PPRT_OBJ( PPRT_OBJ )  ( (PPRT_OBJ)->obj_base_display )
/// Is the particle object in limbo?
#define STATE_LIMBO_PPRT_OBJ( PPRT_OBJ ) ( VALID_PBASE( POBJ_CGET_PBASE(PPRT_OBJ) ) && FLAG_LIMBO_PPRT_OBJ(PPRT_OBJ) )

// Macros to determine whether the particle is in the game or not.
// If objects are being spawned, then any object that is just "defined" is treated as "in game"
#define INGAME_PRT_BASE(IPRT)       ( VALID_PRT ( IPRT ) && FLAG_ON_PBASE( POBJ_CGET_PBASE(PrtObjList.lst + (IPRT)) )  )
#define INGAME_PPRT_BASE(PPRT)      ( VALID_PPRT( PPRT ) && FLAG_ON_PBASE( PDATA_CGET_PBASE(PPRT) ) )

#define INGAME_PRT(IPRT)            ( ( (ego_obj::spawn_depth > 0) ? DEFINED_PRT(IPRT) : INGAME_PRT_BASE(IPRT) ) && !FLAG_LIMBO_PPRT_OBJ( PrtObjList.lst + (IPRT) ) )
#define INGAME_PPRT(PPRT)           ( ( (ego_obj::spawn_depth > 0) ? DEFINED_PPRT(PPRT) : INGAME_PPRT_BASE(PPRT) ) && !FLAG_LIMBO_PPRT_OBJ( PDATA_GET_POBJ(PPRT) ) )

// the same as INGAME_*_BASE except that the particle does not have to be "on"
// visible particles stick around in the active state (but off) until they have been displayed at least once
#define LIMBO_PRT(IPRT)            ( ( (ego_obj::spawn_depth > 0) ? DEFINED_PRT(IPRT) : INGAME_PRT_BASE(IPRT) ) && FLAG_LIMBO_PPRT_OBJ( PrtObjList.lst + (IPRT) ) )
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

extern ego_particle_list PrtObjList;

//--------------------------------------------------------------------------------------------
// Function prototypes
//--------------------------------------------------------------------------------------------

#define _PrtList_h