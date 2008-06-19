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
typedef struct CServer_t
{
  bool_t initialized;

  // my network
  Uint32           net_guid;
  struct CGame_t * parent;

  // connection to the host handling TO_HOST_* type transfers
  NetHost  * host;

  // server states
  bool_t ready;                             // Ready to hit the Start Game button?
  Uint32 rand_idx;

  // a copy of all the character latches
  CLatch  latch[CHRLST_COUNT];

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

} CServer;

CServer * CServer_create(struct CGame_t * gs);
bool_t    CServer_destroy(CServer ** pss);
CServer * CServer_renew(CServer * ss);
retval_t  CServer_startUp(CServer * ss);
retval_t  CServer_shutDown(CServer * ss);

retval_t      sv_shutDown(CServer * ss);
bool_t        sv_Running(CServer * ss);

void     sv_frameStep(CServer * ss);

void CServer_reset_latches(CServer * ss);
void sv_talkToRemotes(CServer * ss);
void CServer_bufferLatches(CServer * ss);
void CServer_unbufferLatches(CServer * ss);
void CServer_resetTimeLatches(CServer * ss, CHR_REF ichr);

bool_t sv_sendPacketToAllClients(CServer * ss, SYS_PACKET * egop);
bool_t sv_sendPacketToAllClientsGuaranteed(CServer * ss, SYS_PACKET * egop);

bool_t sv_unhostGame(CServer * ss);
void   sv_letPlayersJoin(CServer * ss);

bool_t sv_dispatchPackets(CServer * ss);
bool_t sv_handlePacket(CServer * ss, ENetEvent *event);

// More to come...
// int  sv_beginSinglePlayer(...)
// int  sv_beginMultiPlayer(...)
// int  sv_loadModule(...)


