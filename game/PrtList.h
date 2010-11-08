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

#include "particle.h"

#include "egoboo_object_list.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct ego_prt;
struct ego_obj_prt;

//--------------------------------------------------------------------------------------------
// container definition
//--------------------------------------------------------------------------------------------

typedef t_ego_obj_container< ego_obj_prt, MAX_PRT >  ego_prt_container;

//--------------------------------------------------------------------------------------------
// list definition
//--------------------------------------------------------------------------------------------

struct ego_particle_list : public t_obj_lst_deque<ego_obj_prt, MAX_PRT>
{
    typedef t_obj_lst_deque<ego_obj_prt, MAX_PRT> list_type;

    PRT_REF allocate( const bool_t force, const PRT_REF & override = PRT_REF( MAX_PRT ) );

    ego_particle_list( const size_t len = 512 ) : t_obj_lst_deque<ego_obj_prt, MAX_PRT>( len ) {}

    /// begin an iteration through limbo particles
    iterator   iterator_begin_limbo();

    /// begin an iteration through display particles
    iterator   iterator_begin_display();

    /// iterate to next limbo particle
    iterator & iterator_increment_limbo( iterator & it );

    /// iterate to next display particle
    iterator & iterator_increment_display( iterator & it );

protected:
    PRT_REF allocate_find();
    PRT_REF allocate_activate( const PRT_REF & iprt );
};

/// We have to define this so that the looping macros will work correctly
typedef ego_particle_list PrtObjList_t;

//--------------------------------------------------------------------------------------------
// external variables
//--------------------------------------------------------------------------------------------

extern PrtObjList_t PrtObjList;

#define _PrtList_h