#include "egoboo_rpc.h"
#include "game.h"

// this is the implementation for the RPC ("remote procedure call") functions that are necessary for the
// client/server networking

// stub these out for the moment

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void snd_prt_spawn( prt_spawn_info si )
{
  // BB > send a request to the server to spawn a particle with the properties in si

  CGame * gs = si.gs;
  if( !EKEY_PVALID(gs) ) return;

  if( RESERVED_PRT(gs->PrtList, si.iprt) )
  {
    rec_prt_spawn( si );
  }
}

//--------------------------------------------------------------------------------------------
void rec_prt_spawn( prt_spawn_info si )
{
  // BB > act on a request from a server to spawn a particle with the properties in si

  CGame * gs = si.gs;
  if( !EKEY_PVALID(gs) ) return;

  if( RESERVED_PRT(gs->PrtList, si.iprt) )
  {
    gs->PrtList[ si.iprt ].req_active = btrue;
  }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void snd_chr_spawn( chr_spawn_info si )
{
  // BB > send a request to the server to spawn a character with the properties in si

  CGame * gs = si.gs;
  if( !EKEY_PVALID(gs) ) return;

  if( RESERVED_CHR(gs->ChrList, si.ichr) )
  {
    rec_chr_spawn( si );
  }
}

//--------------------------------------------------------------------------------------------
void rec_chr_spawn( chr_spawn_info si )
{
  // BB > act on a request from a server to spawn a character with the properties in si

  CGame * gs = si.gs;
  if( !EKEY_PVALID(gs) ) return;

  if( RESERVED_CHR(gs->ChrList, si.ichr) )
  {
    gs->ChrList[ si.ichr ].req_active = btrue;
  }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void snd_enc_spawn( enc_spawn_info si )
{
  // BB > send a request to the server to spawn a enchant with the properties in si

  CGame * gs = si.gs;
  if( !EKEY_PVALID(gs) ) return;

  if( RESERVED_ENC(gs->EncList, si.ienc) )
  {
    rec_enc_spawn( si );
  }
}

//--------------------------------------------------------------------------------------------
void rec_enc_spawn( enc_spawn_info si )
{
  // BB > act on a request from a server to spawn an enchant with the properties in si

  CGame * gs = si.gs;
  if( !EKEY_PVALID(gs) ) return;

  if( RESERVED_ENC(gs->EncList, si.ienc) )
  {
    gs->EncList[ si.ienc ].req_active = btrue;
  }
}