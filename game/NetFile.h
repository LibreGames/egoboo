#pragma once

#include "Network.h"

#include "egoboo_types.h"


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct nfile_SendState_t;
struct nfile_ReceiveState_t;

bool_t    nfile_Started();
NetHost * nfile_getHost();

//--------------------------------------------------------------------------------------------

typedef struct NFileState_t
{
  egoboo_key ekey;

  // info for handling NET_* type transfers
  Uint32                       crc_seed;
  NetHost                    * host;
  struct nfile_SendState_t    * snd;
  struct nfile_ReceiveState_t * rec;

  // external links
  Uint32       net_guid;
  CNet   * parent;

} NFileState;

NFileState * NFileState_create(CNet * ns);
bool_t       NFileState_destroy(NFileState ** nfs);
retval_t     NFileState_initialize(NFileState * nfs);

retval_t NFileState_startUp(NFileState * nfs);
retval_t NFileState_shutDown(NFileState * nfs);

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------

struct nfile_SendQueue_t;

void net_updateFileTransfers(struct nfile_SendQueue_t * queue);
int  net_pendingFileTransfers(struct nfile_SendQueue_t * queue);

bool_t nfile_dispatchPackets(NFileState * nfs);

retval_t nfile_SendQueue_add(NFileState * nfs, ENetAddress * target_address, char *source, char *dest);
retval_t nfile_ReceiveQueue_add(NFileState * nfs, ENetEvent * event, char * dest);

retval_t nfhost_checkCRC(ENetPeer * peer, const char * source, Uint32 seed, Uint32 CRC);
