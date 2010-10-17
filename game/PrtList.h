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

//--------------------------------------------------------------------------------------------
// list definitions
//--------------------------------------------------------------------------------------------

DECLARE_LIST_EXTERN( ego_prt, PrtList, TOTAL_MAX_PRT );

#define VALID_PRT_RANGE( IPRT ) ( ((IPRT) >= 0) && ((IPRT) < maxparticles) && ((IPRT) < TOTAL_MAX_PRT) )
#define ALLOCATED_PRT( IPRT )   ( VALID_PRT_RANGE( IPRT ) && ALLOCATED_PBASE (POBJ_GET_PBASE(PrtList.lst + (IPRT))) )
#define VALID_PRT( IPRT )       ( VALID_PRT_RANGE( IPRT ) && VALID_PBASE (POBJ_GET_PBASE(PrtList.lst + (IPRT))) )
#define DEFINED_PRT( IPRT )     ( VALID_PRT_RANGE( IPRT ) && VALID_PBASE (POBJ_GET_PBASE(PrtList.lst + (IPRT)) ) && !TERMINATED_PBASE ( POBJ_GET_PBASE(PrtList.lst + (IPRT)) ) )
#define ACTIVE_PRT( IPRT )      ( VALID_PRT_RANGE( IPRT ) && ACTIVE_PBASE    (POBJ_GET_PBASE(PrtList.lst + (IPRT))) )
#define WAITING_PRT( IPRT )     ( VALID_PRT_RANGE( IPRT ) && WAITING_PBASE   (POBJ_GET_PBASE(PrtList.lst + (IPRT))) )
#define TERMINATED_PRT( IPRT )  ( VALID_PRT_RANGE( IPRT ) && TERMINATED_PBASE(POBJ_GET_PBASE(PrtList.lst + (IPRT))) )

#define GET_INDEX_PPRT( PPRT )  ((size_t)GET_INDEX_POBJ( PPRT, TOTAL_MAX_PRT ))
#define GET_REF_PPRT( PPRT )    ((PRT_REF)GET_INDEX_PPRT( PPRT ))
#define VALID_PRT_PTR( PPRT )   ( (NULL != (PPRT)) && VALID_PRT_RANGE( GET_REF_POBJ( PPRT, TOTAL_MAX_PRT) ) )
#define ALLOCATED_PPRT( PPRT )  ( VALID_PRT_PTR( PPRT ) && ALLOCATED_PBASE( POBJ_GET_PBASE(PPRT) ) )
#define VALID_PPRT( PPRT )      ( VALID_PRT_PTR( PPRT ) && VALID_PBASE( POBJ_GET_PBASE(PPRT) ) )
#define DEFINED_PPRT( PPRT )    ( VALID_PRT_PTR( PPRT ) && VALID_PBASE (POBJ_GET_PBASE(PPRT)) && !TERMINATED_PBASE (POBJ_GET_PBASE(PPRT)) )
#define TERMINATED_PPRT( PPRT ) ( VALID_PRT_PTR( PPRT ) && TERMINATED_PBASE(POBJ_GET_PBASE(PPRT)) )
#define ACTIVE_PPRT( PPRT )     ( VALID_PRT_PTR( PPRT ) && ACTIVE_PBASE(POBJ_GET_PBASE(PPRT)) )

// Macros automate looping through the PrtList. This hides code which defers the creation and deletion of
// objects until the loop terminates, so that the length of the list will not change during the loop.
#define PRT_BEGIN_LOOP_USED(IT, PRT_BDL)   {size_t IT##_internal; int prt_loop_start_depth = prt_loop_depth; prt_loop_depth++; for(IT##_internal=0;IT##_internal<PrtList.used_count;IT##_internal++) { PRT_REF IT; ego_prt_bundle PRT_BDL; IT = (PRT_REF)PrtList.used_ref[IT##_internal]; if(!DEFINED_PRT (IT)) continue; ego_prt_bundle::set(&PRT_BDL, PrtList.lst + IT);
#define PRT_BEGIN_LOOP_ACTIVE(IT, PRT_BDL) {size_t IT##_internal; int prt_loop_start_depth = prt_loop_depth; prt_loop_depth++; for(IT##_internal=0;IT##_internal<PrtList.used_count;IT##_internal++) { PRT_REF IT; ego_prt_bundle PRT_BDL; IT = (PRT_REF)PrtList.used_ref[IT##_internal]; if(!ACTIVE_PRT (IT)) continue; ego_prt_bundle::set(&PRT_BDL, PrtList.lst + IT);
#define PRT_BEGIN_LOOP_LIMBO(IT, PRT_BDL)  {size_t IT##_internal; int prt_loop_start_depth = prt_loop_depth; prt_loop_depth++; for(IT##_internal=0;IT##_internal<PrtList.used_count;IT##_internal++) { PRT_REF IT; ego_prt_bundle PRT_BDL; IT = (PRT_REF)PrtList.used_ref[IT##_internal]; if(!LIMBO_PRT(IT)) continue; ego_prt_bundle::set(&PRT_BDL, PrtList.lst + IT);
#define PRT_END_LOOP() } prt_loop_depth--; if(prt_loop_start_depth != prt_loop_depth) EGOBOO_ASSERT(bfalse); PrtList_cleanup(); }

extern int prt_loop_depth;

/// Is the particle flagged as being in limbo?
#define FLAG_LIMBO_PPRT( PPRT )  ( (PPRT)->obj_base_display )
/// Is the particle object in limbo?
#define STATE_LIMBO_PPRT( PPRT ) ( VALID_PBASE( POBJ_GET_PBASE(PPRT) ) && FLAG_LIMBO_PPRT(PPRT) )

// Macros to determine whether the particle is in the game or not.
// If objects are being spawned, then any object that is just "defined" is treated as "in game"
#define INGAME_PRT_BASE(IPRT)       ( VALID_PRT ( IPRT ) && FLAG_ON_PBASE( POBJ_GET_PBASE(PrtList.lst + (IPRT)) )  )
#define INGAME_PPRT_BASE(PPRT)      ( VALID_PPRT( PPRT ) && FLAG_ON_PBASE( POBJ_GET_PBASE(PPRT) ) )

#define INGAME_PRT(IPRT)            ( ( (ego_object::spawn_depth > 0) ? DEFINED_PRT(IPRT) : INGAME_PRT_BASE(IPRT) ) && !FLAG_LIMBO_PPRT( PrtList.lst + (IPRT) ) )
#define INGAME_PPRT(PPRT)           ( ( (ego_object::spawn_depth > 0) ? DEFINED_PPRT(PPRT) : INGAME_PPRT_BASE(PPRT) ) && !FLAG_LIMBO_PPRT( PPRT ) )

// the same as INGAME_*_BASE except that the particle does not have to be "on"
// visible particles stick around in the active state (but off) until they have been displayed at least once
#define LIMBO_PRT(IPRT)            ( ( (ego_object::spawn_depth > 0) ? DEFINED_PRT(IPRT) : INGAME_PRT_BASE(IPRT) ) && FLAG_LIMBO_PRT( IPRT ) )
#define LIMBO_PPRT(PPRT)           ( ( (ego_object::spawn_depth > 0) ? DEFINED_PPRT(PPRT) : INGAME_PPRT_BASE(PPRT) ) && FLAG_LIMBO_PPRT( PPRT ) )

//--------------------------------------------------------------------------------------------
// Function prototypes
//--------------------------------------------------------------------------------------------

void    PrtList_init();
void    PrtList_dtor();

PRT_REF PrtList_allocate( bool_t force );

bool_t  PrtList_free_one( const PRT_REF by_reference iprt );
void    PrtList_free_all();

bool_t  PrtList_add_used( const PRT_REF by_reference iprt );

void    PrtList_update_used();

void    PrtList_cleanup();

bool_t PrtList_add_activation( PRT_REF iprt );
bool_t PrtList_add_termination( PRT_REF iprt );

void PrtList_cleanup();