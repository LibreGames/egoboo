#pragma once

#include "Network.h"

#include "egoboo_types.h"


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct s_nfile_SendState;
struct s_nfile_ReceiveState;

NetHost_t * nfile_getHost();
void      nfile_quitHost();
bool_t    nfile_Started();

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

retval_t nfhost_checkCRC(ENetPeer * peer, const char * source, Uint32 seed, Uint32 CRC);
