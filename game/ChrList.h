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

#define VALID_CHR_IDX( INDX )      ( ((INDX) >= 0) && ((INDX) < MAX_CHR) )
#define VALID_CHR_REF( ICHR )      ChrList.validate_ref(ICHR)
#define ALLOCATED_CHR( ICHR )      ( VALID_CHR_REF( ICHR ) && ALLOCATED_PBASE ( POBJ_GET_PBASE(ChrList.lst + (ICHR)) ) )
#define VALID_CHR( ICHR )          ( VALID_CHR_REF( ICHR ) && VALID_PBASE     ( POBJ_GET_PBASE(ChrList.lst + (ICHR)) ) )
#define DEFINED_CHR( ICHR )        ( VALID_CHR_REF( ICHR ) && VALID_PBASE     ( POBJ_GET_PBASE(ChrList.lst + (ICHR)) ) && !TERMINATED_PBASE ( POBJ_GET_PBASE(ChrList.lst + (ICHR)) ) )
#define ACTIVE_CHR( ICHR )         ( VALID_CHR_REF( ICHR ) && ACTIVE_PBASE    ( POBJ_GET_PBASE(ChrList.lst + (ICHR)) ) )
#define WAITING_CHR( ICHR )        ( VALID_CHR_REF( ICHR ) && WAITING_PBASE   ( POBJ_GET_PBASE(ChrList.lst + (ICHR)) ) )
#define TERMINATED_CHR( ICHR )     ( VALID_CHR_REF( ICHR ) && TERMINATED_PBASE( POBJ_GET_PBASE(ChrList.lst + (ICHR)) ) )

#define GET_IDX_PCHR( PCHR )        ((size_t)GET_IDX_POBJ( PCHR, MAX_CHR ))
#define GET_REF_PCHR( PCHR )        ((CHR_REF)GET_REF_POBJ( PCHR, MAX_CHR ))
#define VALID_CHR_PTR( PCHR )       ( (NULL != (PCHR)) && VALID_CHR_REF( GET_REF_PCHR( PCHR ) ) )
#define ALLOCATED_PCHR( PCHR )      ( VALID_CHR_PTR( PCHR ) && ALLOCATED_PBASE( POBJ_GET_PBASE(PCHR) ) )
#define VALID_PCHR( PCHR )          ( VALID_CHR_PTR( PCHR ) && VALID_PBASE( POBJ_GET_PBASE(PCHR) ) )
#define DEFINED_PCHR( PCHR )        ( VALID_CHR_PTR( PCHR ) && VALID_PBASE ( POBJ_GET_PBASE(PCHR) ) && !TERMINATED_PBASE ( POBJ_GET_PBASE(PCHR) ) )
#define ACTIVE_PCHR( PCHR )         ( VALID_CHR_PTR( PCHR ) && ACTIVE_PBASE( POBJ_GET_PBASE(PCHR) ) )
#define TERMINATED_PCHR( PCHR )     ( VALID_CHR_PTR( PCHR ) && TERMINATED_PBASE( POBJ_GET_PBASE(PCHR) ) )

// Macros automate looping through the ChrList. This hides code which defers the creation and deletion of
// objects until the loop terminates, so that the length of the list will not change during the loop.
#define CHR_BEGIN_LOOP_ACTIVE(IT, PCHR)  {size_t IT##_internal; int chr_loop_start_depth = ChrList.loop_depth; ChrList.loop_depth++; for(IT##_internal=0;IT##_internal<ChrList.used_count;IT##_internal++) { CHR_REF IT; ego_chr * PCHR = NULL; IT = (CHR_REF)ChrList.used_ref[IT##_internal]; if(!ACTIVE_CHR (IT)) continue; PCHR =  ChrList.get_valid_ptr(IT);
#define CHR_BEGIN_LOOP_BSP(IT, PCHR)     {size_t IT##_internal; int chr_loop_start_depth = ChrList.loop_depth; ChrList.loop_depth++; for(IT##_internal=0;IT##_internal<ChrList.used_count;IT##_internal++) { CHR_REF IT; ego_chr * PCHR = NULL; IT = (CHR_REF)ChrList.used_ref[IT##_internal]; if(!ACTIVE_CHR (IT)) continue; PCHR =  ChrList.get_valid_ptr(IT); if( !PCHR->bsp_leaf.inserted ) continue;
#define CHR_END_LOOP() } ChrList.loop_depth--; if(chr_loop_start_depth != ChrList.loop_depth) EGOBOO_ASSERT(bfalse); ChrList.cleanup(); }


// Macros to determine whether the character is in the game or not.
// If objects are being spawned, then any object that is just "defined" is treated as "in game"
#define INGAME_CHR_BASE(ICHR)       ( VALID_CHR_REF( ICHR ) && ACTIVE_PBASE( POBJ_GET_PBASE(ChrList.lst + (ICHR)) ) && ON_PBASE( POBJ_GET_PBASE(ChrList.lst + (ICHR)) ) )
#define INGAME_PCHR_BASE(PCHR)      ( VALID_CHR_PTR( PCHR ) && ACTIVE_PBASE( POBJ_GET_PBASE(PCHR) ) && ON_PBASE( POBJ_GET_PBASE(PCHR) ) )

#define INGAME_CHR(ICHR)            ( (ego_object::spawn_depth) > 0 ? DEFINED_CHR(ICHR) : INGAME_CHR_BASE(ICHR) )
#define INGAME_PCHR(PCHR)           ( (ego_object::spawn_depth) > 0 ? DEFINED_PCHR(PCHR) : INGAME_PCHR_BASE(PCHR) )

//--------------------------------------------------------------------------------------------
// list declaration
//--------------------------------------------------------------------------------------------
extern t_ego_obj_lst<ego_chr,MAX_CHR> ChrList;

//--------------------------------------------------------------------------------------------
// Function prototypes
//--------------------------------------------------------------------------------------------

#define _ChrList_h