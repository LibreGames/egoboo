/* Egoboo - Client.h
 * Basic skeleton for the client portion of a client-server architecture,
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

#include <enet.h>
#include "Network.h"
#include "char.h"

#include "egoboo_types.h"
#include "egoboo.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// Globally accesible client module functions

NetHost * cl_getHost();
bool_t    cl_Started();

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// Status displays
typedef struct Status_t
{
  bool_t on;

  Uint16  delay;
  Uint16  chr_ref;
  vect2   pos;
  float   y_pos;
} Status;

Status * Status_new( Status * pstat );
bool_t   Status_delete( Status * pstat );
Status * Status_renew( Status * pstat );




//--------------------------------------------------------------------------------------------
typedef struct ClientState_t
{
  bool_t initialized;

  // my network
  Uint32       net_guid;
  NetState   * parent;

  // game info
  ENetPeer   * gamePeer;
  Uint32       gameID;

  // connection to the host handling TO_REMOTE_* type transfers
  NetHost  * host;

  // local states
  Status StatList[MAXSTAT];
  size_t StatList_count;
  Uint16 msg_timechange;
  Uint8  stat_clock;                        // For stat regeneration

  Uint32 loc_pla_count;
  bool_t seeinvisible;
  bool_t seekurse;

  bool_t waiting;                          // Has everyone talked to the host?
  int    selectedPlayer;
  int    selectedModule;
  bool_t logged_on;

  // latch info
  TIME_LATCH_BUFFER tlb;

  // client information about remote modules
  int          rem_mod_count;
  MOD_INFO     rem_mod[MAXMODULE];
  MOD_SUMMARY  rem_modtxt;

  // info needed to request module info from the host(s)
  ENetPeer *   rem_req_peer [MAXNETPLAYER];
  bool_t       rem_req_info [MAXNETPLAYER];
  bool_t       rem_req_image[MAXNETPLAYER];

  // client information about desired local module 
  char      req_host[MAXCAPNAMESIZE];
  MOD_INFO  req_mod;
  ModState  req_modstate;

} ClientState;

ClientState * ClientState_create(NetState * ns);
bool_t        ClientState_destroy(ClientState ** pcs);
ClientState * ClientState_renew(ClientState * cs);
retval_t      ClientState_startUp(ClientState * cs);
retval_t      ClientState_shutDown(ClientState * cs);
bool_t        ClientState_Running(ClientState * cs);



void ClientState_reset_latches(ClientState * cs);
void ClientState_resetTimeLatches(ClientState * cs, Sint32 ichr);
void ClientState_bufferLatches(ClientState * cs);



bool_t ClientState_connect(ClientState * cs, const char* hostname);
bool_t ClientState_disconnect(ClientState * cs);

ENetPeer * cl_startPeer(const char* hostname );
 
void     ClientState_talkToHost(ClientState * cs);
retval_t ClientState_joinGame(ClientState * cs, const char *hostname);
bool_t   ClientState_unjoinGame(ClientState * cs);

bool_t ClientState_sendPacketToHost(ClientState * cs, SYS_PACKET * egop);
bool_t ClientState_sendPacketToHostGuaranteed(ClientState * cs, SYS_PACKET * egop);


void   ClientState_unbufferLatches(ClientState * cs);

bool_t cl_begin_request_module(ClientState * cs);
bool_t cl_end_request_module(ClientState * cs);

void   cl_request_module_images(ClientState * cs);
bool_t cl_load_module_images(ClientState * cs);

void   cl_request_module_info(ClientState * cs);
bool_t cl_load_module_info(ClientState * cs);

bool_t cl_dispatchPackets(ClientState * cs);
bool_t cl_handlePacket(ClientState * cs, ENetEvent *event);

bool_t cl_count_players(ClientState * cs);


bool_t StatList_new( ClientState * cs );
bool_t StatList_delete( ClientState * cs );
bool_t StatList_renew( ClientState * cs );
size_t StatList_add( Status lst[], size_t lst_size, CHR_REF ichr );
void   StatList_move_to_top( Status lst[], size_t lst_size, CHR_REF ichr );

// Much more to come...
//int  cl_connectToServer(...);
//int  cl_loadModule(...);
//void     cl_frameStep();

