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
/// @brief Client definitions for network gaming.
/// @details Basic skeleton for the client portion of a client-server architecture.
///   This is not totally working yet.

#include <enet/enet.h>
#include "Network.h"

#include "egoboo_types.h"
#include "egoboo.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// Globally accesible client module functions

NetHost_t * cl_getHost( void );
void      cl_quitHost( void );
bool_t    cl_Started( void );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// Status_t displays
struct sStatus
{
  egoboo_key_t ekey;

  bool_t on;

  Uint16   delay;
  CHR_REF  chr_ref;
  vect2    pos;
  float    y_pos;
};
typedef struct sStatus Status_t;

Status_t * Status_new( Status_t * pstat );
bool_t   Status_delete( Status_t * pstat );
Status_t * Status_renew( Status_t * pstat );




//--------------------------------------------------------------------------------------------
struct sClient
{
  egoboo_key_t      ekey;

  // my network
  Uint32           net_guid;
  struct sGame * parent;

  // game info
  ENetPeer   * gamePeer;
  Uint32       gameID;
  bool_t       outofsync;   ///< is the client too far out of synch with the server?

  // connection to the host handling TO_REMOTE_* type transfers
  NetHost_t  * host;

  // local states
  Status_t StatList[MAXSTAT];
  size_t StatList_count;
  Uint8  stat_clock;                        ///< For stat regeneration

  Uint32 loc_pla_count;
  bool_t seeinvisible;
  bool_t seekurse;

  bool_t waiting;                          ///< Has everyone talked to the host?
  int    selectedPlayer;
  int    selectedModule;
  bool_t logged_on;

  // latch info
  TIME_LATCH_BUFFER tlb;

  // client information about remote modules
  int          rem_mod_count;
  MOD_INFO     rem_mod[MAXMODULE];
  ModSummary_t  rem_modtxt;

  // info needed to request module info from the host(s)
  ENetPeer *   rem_req_peer [MAXNETPLAYER];
  bool_t       rem_req_info [MAXNETPLAYER];
  bool_t       rem_req_image[MAXNETPLAYER];

  // client information about desired local module
  char      req_host[MAXCAPNAMESIZE];
  MOD_INFO  req_mod;
  ModState_t  req_modstate;
  Uint32    req_seed;

  CHR_SPAWN_QUEUE chr_queue;

};
typedef struct sClient Client_t;

Client_t * Client_create(struct sGame * gs);
bool_t     Client_destroy(Client_t ** pcs);
Client_t * Client_renew(Client_t * cs);
retval_t   Client_startUp(Client_t * cs);
retval_t   Client_shutDown(Client_t * cs);
bool_t     cl_Running(Client_t * cs);



void Client_reset_latches(Client_t * cs);
void Client_resetTimeLatches(Client_t * cs, CHR_REF ichr);
void Client_bufferLatches(Client_t * cs);



bool_t CClient_connect(Client_t * cs, const char* hostname);
bool_t CClient_disconnect(Client_t * cs);

ENetPeer * cl_startPeer( const char* hostname );

void     Client_talkToHost(Client_t * cs);
retval_t Client_joinGame(Client_t * cs, const char *hostname);
bool_t   Client_unjoinGame(Client_t * cs);

bool_t Client_sendPacketToHost(Client_t * cs, SYS_PACKET * egop);
bool_t Client_sendPacketToHostGuaranteed(Client_t * cs, SYS_PACKET * egop);


void   Client_unbufferLatches(Client_t * cs);

bool_t cl_begin_request_module(Client_t * cs);
bool_t cl_end_request_module(Client_t * cs);

void   cl_request_module_images(Client_t * cs);

void   cl_request_module_info(Client_t * cs);
bool_t cl_load_module_info(Client_t * cs);

bool_t cl_dispatchPackets(Client_t * cs);
bool_t cl_handlePacket(Client_t * cs, ENetEvent *event);

bool_t cl_count_players(Client_t * cs);


bool_t StatList_new( Client_t * cs );
bool_t StatList_delete( Client_t * cs );
bool_t StatList_renew( Client_t * cs );
size_t StatList_add( Status_t lst[], size_t lst_size, CHR_REF ichr );
void   StatList_move_to_top( Status_t lst[], size_t lst_size, CHR_REF ichr );

// Much more to come...
//int  cl_connectToServer(...);
//int  cl_loadModule(...);
//void     cl_frameStep( void );

