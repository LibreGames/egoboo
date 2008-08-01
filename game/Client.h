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
struct Status_t
{
  egoboo_key ekey;

  bool_t on;

  Uint16   delay;
  CHR_REF  chr_ref;
  vect2    pos;
  float    y_pos;
};
typedef struct Status_t Status;

Status * Status_new( Status * pstat );
bool_t   Status_delete( Status * pstat );
Status * Status_renew( Status * pstat );




//--------------------------------------------------------------------------------------------
struct CClient_t
{
  egoboo_key      ekey;

  // my network
  Uint32           net_guid;
  struct CGame_t * parent;

  // game info
  ENetPeer   * gamePeer;
  Uint32       gameID;

  // connection to the host handling TO_REMOTE_* type transfers
  NetHost  * host;

  // local states
  Status StatList[MAXSTAT];
  size_t StatList_count;
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
  Uint32    req_seed;

  chr_spawn_queue chr_queue;

};
typedef struct CClient_t CClient;

CClient * CClient_create(struct CGame_t * gs);
bool_t    CClient_destroy(CClient ** pcs);
CClient * CClient_renew(CClient * cs);
retval_t      CClient_startUp(CClient * cs);
retval_t      CClient_shutDown(CClient * cs);
bool_t        CClient_Running(CClient * cs);



void CClient_reset_latches(CClient * cs);
void CClient_resetTimeLatches(CClient * cs, CHR_REF ichr);
void CClient_bufferLatches(CClient * cs);



bool_t CClient_connect(CClient * cs, const char* hostname);
bool_t CClient_disconnect(CClient * cs);

ENetPeer * cl_startPeer(const char* hostname );
 
void     CClient_talkToHost(CClient * cs);
retval_t CClient_joinGame(CClient * cs, const char *hostname);
bool_t   CClient_unjoinGame(CClient * cs);

bool_t CClient_sendPacketToHost(CClient * cs, SYS_PACKET * egop);
bool_t CClient_sendPacketToHostGuaranteed(CClient * cs, SYS_PACKET * egop);


void   CClient_unbufferLatches(CClient * cs);

bool_t cl_begin_request_module(CClient * cs);
bool_t cl_end_request_module(CClient * cs);

void   cl_request_module_images(CClient * cs);

void   cl_request_module_info(CClient * cs);
bool_t cl_load_module_info(CClient * cs);

bool_t cl_dispatchPackets(CClient * cs);
bool_t cl_handlePacket(CClient * cs, ENetEvent *event);

bool_t cl_count_players(CClient * cs);


bool_t StatList_new( CClient * cs );
bool_t StatList_delete( CClient * cs );
bool_t StatList_renew( CClient * cs );
size_t StatList_add( Status lst[], size_t lst_size, CHR_REF ichr );
void   StatList_move_to_top( Status lst[], size_t lst_size, CHR_REF ichr );

// Much more to come...
//int  cl_connectToServer(...);
//int  cl_loadModule(...);
//void     cl_frameStep();

