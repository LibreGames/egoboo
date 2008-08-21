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
/// @brief Egoboo Network Filesystem
/// @details Definitions for an asynchronous file transfer system using Enet.

#include "Network.h"

#include "egoboo_types.h"


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_nfile_SendState;
struct s_nfile_ReceiveState;

NetHost_t * nfile_getHost( void );
void      nfile_quitHost( void );
bool_t    nfile_Started( void );

//--------------------------------------------------------------------------------------------

struct sNFileState
{
  egoboo_key_t ekey;

  // info for handling NET_* type transfers
  Uint32                       crc_seed;
  NetHost_t                    * host;
  struct s_nfile_SendState    * snd;
  struct s_nfile_ReceiveState * rec;

  // external links
  Uint32    net_guid;
  Net_t   * parent;

};
typedef struct sNFileState NFileState_t;

NFileState_t * NFileState_create(Net_t * ns);
bool_t       NFileState_destroy(NFileState_t ** nfs);
retval_t     NFileState_initialize(NFileState_t * nfs);

retval_t NFileState_startUp(NFileState_t * nfs);
retval_t NFileState_shutDown(NFileState_t * nfs);

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------

struct s_nfile_SendQueue;

void net_updateFileTransfers(struct s_nfile_SendQueue * queue);
int  net_pendingFileTransfers(struct s_nfile_SendQueue * queue);

bool_t nfile_dispatchPackets(NFileState_t * nfs);

retval_t nfile_SendQueue_add(NFileState_t * nfs, ENetAddress * target_address, char *source, char *dest);
retval_t nfile_ReceiveQueue_add(NFileState_t * nfs, ENetEvent * event, char * dest);

retval_t nfhost_checkCRC(ENetPeer * peer, EGO_CONST char * source, Uint32 seed, Uint32 CRC);
