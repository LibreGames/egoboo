/* Egoboo - Server.h
 * Basic skeleton for the server portion of a client-server architecture,
 * this is totally not in use yet.
 */

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

#pragma once

#include "Network.h"
#include "object.h"
#include "char.h"
#include "input.h"

#include "egoboo_types.h"
#include "egoboo.h"

#include <enet/enet.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

bool_t      sv_Started( void );
NetHost_t * sv_getHost( void );
void        sv_quitHost( void );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct sServer
{
  egoboo_key_t ekey;

  // my network
  Uint32           net_guid;
  struct sGame * parent;

  // connection to the host handling TO_HOST_* type transfers
  NetHost_t  * host;

  // server states
  bool_t ready;                             // Ready to hit the Start Game button?
  Uint32 rand_idx;

  // a copy of all the character latches
  Latch_t  latch[CHRLST_COUNT];

  // the buffered latches that have been stored on the server
  TIME_LATCH_BUFFER tlb;

  // local module parameters
  int         selectedModule;
  size_t      loc_mod_count;
  MOD_INFO    loc_mod[MAXMODULE];
  ModSummary_t loc_modtxt;

  // Selected module parameters. Store them here just in case.
  MOD_INFO  mod;
  ModState_t  modstate;

  int    rem_pla_count;
  int    num_loaded;                       //
  int    num_logon;

};
typedef struct sServer Server_t;

Server_t * Server_create(struct sGame * gs);
bool_t     Server_destroy(Server_t ** pss);
Server_t * Server_renew(Server_t * ss);
retval_t   Server_startUp(Server_t * ss);
retval_t   Server_shutDown(Server_t * ss);

void Server_reset_latches(Server_t * ss);

void Server_bufferLatches(Server_t * ss);
void Server_unbufferLatches(Server_t * ss);
void Server_resetTimeLatches(Server_t * ss, CHR_REF ichr);

retval_t sv_shutDown(Server_t * ss);
bool_t   sv_Running(Server_t * ss);

void     sv_frameStep(Server_t * ss);
void     sv_talkToRemotes(Server_t * ss);

bool_t sv_sendPacketToAllClients(Server_t * ss, SYS_PACKET * egop);
bool_t sv_sendPacketToAllClientsGuaranteed(Server_t * ss, SYS_PACKET * egop);

bool_t sv_unhostGame(Server_t * ss);
void   sv_letPlayersJoin(Server_t * ss);

bool_t sv_dispatchPackets(Server_t * ss);
bool_t sv_handlePacket(Server_t * ss, ENetEvent *event);
bool_t sv_send_chr_setup( Server_t * ss, CHR_SPAWN_INFO * si );

// More to come...
// int  sv_beginSinglePlayer(...)
// int  sv_beginMultiPlayer(...)
// int  sv_loadModule(...)
