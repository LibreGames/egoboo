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

#include <enet.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

NetHost * sv_getHost();
bool_t    sv_Started();

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
typedef struct ServerState_t
{
  bool_t initialized;

  // my network
  Uint32     net_guid;
  NetState * parent;

  // connection to the host handling TO_HOST_* type transfers
  NetHost  * host;

  // server states
  bool_t ready;                             // Ready to hit the Start Game button?
  Uint32 rand_idx;

  // a copy of all the character latches
  LATCH  latch[MAXCHR];

  // the buffered latches that have been stored on the server
  TIME_LATCH_BUFFER tlb;

  // local module parameters
  int         selectedModule;
  int         loc_mod_count;
  MOD_INFO    loc_mod[MAXMODULE];
  MOD_SUMMARY loc_modtxt;

  // Selected module parameters. Store them here just in case.
  MOD_INFO  mod;
  ModState  modstate;

  int    rem_pla_count;
  int    num_loaded;                       //
  int    num_logon;

} ServerState;

ServerState * ServerState_create(struct NetState_t * ns);
bool_t        ServerState_destroy(ServerState ** pss);
ServerState * ServerState_renew(ServerState * ss);
retval_t      ServerState_startUp(ServerState * ss);
retval_t      ServerState_shutDown(ServerState * ss);

retval_t      sv_shutDown(ServerState * ss);
bool_t        sv_Running(ServerState * ss);

void     sv_frameStep(ServerState * ss);

void ServerState_reset_latches(ServerState * ss);
void sv_talkToRemotes(ServerState * ss);
void ServerState_bufferLatches(ServerState * ss);
void sv_unbufferLatches(ServerState * ss);
void ServerState_resetTimeLatches(ServerState * ss, Sint32 ichr);

bool_t sv_sendPacketToAllClients(ServerState * ss, SYS_PACKET * egop);
bool_t sv_sendPacketToAllClientsGuaranteed(ServerState * ss, SYS_PACKET * egop);

bool_t sv_unhostGame(ServerState * ss);
void   sv_letPlayersJoin(ServerState * ss);

bool_t sv_dispatchPackets(ServerState * ss);
bool_t sv_handlePacket(ServerState * ss, ENetEvent *event);

// More to come...
// int  sv_beginSinglePlayer(...)
// int  sv_beginMultiPlayer(...)
// int  sv_loadModule(...)


