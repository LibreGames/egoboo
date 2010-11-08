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

/// \file EncList.h
/// \brief Routines for enchant list management

#include "enchant.h"

#include "egoboo_object_list.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct ego_enc;
struct ego_obj_enc;

//--------------------------------------------------------------------------------------------
// container declaration
//--------------------------------------------------------------------------------------------

typedef t_ego_obj_container< ego_obj_enc, MAX_ENC >  ego_enc_container;

//--------------------------------------------------------------------------------------------
// list declaration
//--------------------------------------------------------------------------------------------

typedef t_obj_lst_deque<ego_obj_enc, MAX_ENC> EncObjList_t;

//--------------------------------------------------------------------------------------------
// external variables
//--------------------------------------------------------------------------------------------

extern EncObjList_t EncObjList;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define _EncList_h