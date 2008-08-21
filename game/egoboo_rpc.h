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
/// @brief Remote Procedure Call definitions.
/// @details This is the header file for the RPC functions that are necessary for the
///   client/server networking.

#include "char.h"
#include "enchant.h"
#include "particle.h"

// the spawning is the most important thing that must be handled through RPCs
void snd_prt_spawn( PRT_SPAWN_INFO si );
void rec_prt_spawn( PRT_SPAWN_INFO si );

void snd_chr_spawn( CHR_SPAWN_INFO si );
void rec_chr_spawn( CHR_SPAWN_INFO si );

void snd_enc_spawn( ENC_SPAWN_INFO si );
void rec_enc_spawn( ENC_SPAWN_INFO si );


// Because of the way that egoboo is set up there are several modifications to these objects that
// get applied after they are spawned

void snd_prt_mod_XX( PRT_REF prt, int val );
void rec_prt_mod_XX( PRT_REF prt, int val );

void snd_chr_mod_XX( CHR_REF prt, int val );
void rec_chr_mod_XX( CHR_REF prt, int val );
