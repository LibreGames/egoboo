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
/// @brief Implementation of the Remote Procedure Call functions
/// @details this is the implementation for the RPC @("remote procedure call"@) functions that are necessary for the
/// client/server networking. Stub these out for the moment.

#include "egoboo_rpc.h"

#include "game.h"

#include "egoboo_types.inl"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void snd_prt_spawn( PRT_SPAWN_INFO si )
{
  /// @details BB@> send a request to the server to spawn a particle with the properties in si

  Game_t * gs = si.gs;
  if( !EKEY_PVALID(gs) ) return;

  if( RESERVED_PRT(gs->PrtList, si.iprt) )
  {
    rec_prt_spawn( si );
  }
}

//--------------------------------------------------------------------------------------------
void rec_prt_spawn( PRT_SPAWN_INFO si )
{
  /// @details BB@> act on a request from a server to spawn a particle with the properties in si

  Game_t * gs = si.gs;
  if( !EKEY_PVALID(gs) ) return;

  if( RESERVED_PRT(gs->PrtList, si.iprt) )
  {
    gs->PrtList[ si.iprt ].req_active = btrue;
    gs->PrtList[ si.iprt ].reserved   = bfalse;
  }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void snd_chr_spawn( CHR_SPAWN_INFO si )
{
  /// @details BB@> send a request to the server to spawn a character with the properties in si

  Game_t * gs = si.gs;
  if( !EKEY_PVALID(gs) ) return;

  if( RESERVED_CHR(gs->ChrList, si.ichr) )
  {
    rec_chr_spawn( si );
  }
}

//--------------------------------------------------------------------------------------------
void rec_chr_spawn( CHR_SPAWN_INFO si )
{
  /// @details BB@> act on a request from a server to spawn a character with the properties in si

  Game_t * gs = si.gs;
  if( !EKEY_PVALID(gs) ) return;

  if( RESERVED_CHR(gs->ChrList, si.ichr) )
  {
    gs->ChrList[ si.ichr ].req_active = btrue;
    gs->ChrList[ si.ichr ].reserved   = bfalse;
  }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void snd_enc_spawn( ENC_SPAWN_INFO si )
{
  /// @details BB@> send a request to the server to spawn a enchant with the properties in si

  Game_t * gs = si.gs;
  if( !EKEY_PVALID(gs) ) return;

  if( RESERVED_ENC(gs->EncList, si.ienc) )
  {
    rec_enc_spawn( si );
  }
}

//--------------------------------------------------------------------------------------------
void rec_enc_spawn( ENC_SPAWN_INFO si )
{
  /// @details BB@> act on a request from a server to spawn an enchant with the properties in si

  Game_t * gs = si.gs;
  if( !EKEY_PVALID(gs) ) return;

  if( RESERVED_ENC(gs->EncList, si.ienc) )
  {
    gs->EncList[ si.ienc ].req_active = btrue;
    gs->EncList[ si.ienc ].reserved   = bfalse;
  }
}
