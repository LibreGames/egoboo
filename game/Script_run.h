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

///
/// @file
/// @brief Egoscript execution
/// @details Execute compiled egoboo scripts

#include "script.h"

struct sMad;
struct sChr;
struct sGame;

bool_t _DoAction( struct sGame * gs, struct sChr * pchr, struct sMad * pmad, Uint16 iaction );
bool_t _DoActionOverride( struct sGame * gs, struct sChr * pchr, struct sMad * pmad, Uint16 iaction );

retval_t run_script( struct sGame * gs, struct s_ai_state * pstate, float dUpdate );
void run_all_scripts( struct sGame * gs, float dUpdate );