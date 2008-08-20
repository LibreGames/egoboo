/* Egoboo - Network.h
 * Definitions for Egoboo network functionality
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

#include "input.h"
#include "char.h"

#include "egoboo.h"

#include <enet/enet.h>
#include <SDL_thread.h>

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------

struct sConfigData;

// the type of a function that can process an ENetEvent
typedef retval_t (*EventHandler_Ptr_t) (void * data, ENetEvent * event);

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------

#define NETREFRESH          1000                    // Every second
#define NONETWORK           numservice              //

#define SHORTLATCH 1024.0
#define MAXSENDSIZE 8192
#define COPYSIZE    4096
#define TOTALSIZE   2097152

#define MAXLAG      (1<<6)                          //
#define LAGAND      (MAXLAG-1)                      //
#define STARTTALK   (MAXLAG>>3)                     //

#define MAXSERVICE 1024
#define NETNAMESIZE 16
#define MAXSESSION 16
#define MAXNETPLAYER 8

#define INVALID_TIMESTAMP ((Uint32)(-1))

// these values for the message headers were generated using a ~15 bit linear congruential
// generator.  They should be relatively good choices that are free from any bias...
enum e_network_packet_types
{
  TO_ANY_TEXT             = 0x1D66,

  TO_HOST_LOGON           = 0xF8E1,
  TO_HOST_LOGOFF          = 0x3E46,
  TO_HOST_REQUEST_MODULE  = 0x5DB6,
  TO_HOST_MODULE          = 0xEFD0,
  TO_HOST_MODULEOK        = 0xA31A,
  TO_HOST_MODULEBAD       = 0xB473,
  TO_HOST_LATCH           = 0xF296,
  TO_HOST_RTS             = 0x0A47,
  TO_HOST_IM_LOADED       = 0xDD2C,
  TO_HOST_FILE            = 0xBC6B,
  TO_HOST_DIR             = 0xDB29,
  TO_HOST_FILESENT        = 0x6437,

  TO_REMOTE_LOGON         = 0x763A,
  TO_REMOTE_LOGOFF        = 0x989B,
  TO_REMOTE_KICK          = 0xA64F,
  TO_REMOTE_MODULE        = 0xF068,
  TO_REMOTE_MODULEBAD     = 0x2FE9,
  TO_REMOTE_MODULEINFO    = 0x00E8,
  TO_REMOTE_LATCH         = 0x9526,
  TO_REMOTE_FILE          = 0xA04E,
  TO_REMOTE_DIR           = 0x13E2,
  TO_REMOTE_RTS           = 0x4817,
  TO_REMOTE_START         = 0x27AF,
  TO_REMOTE_FILE_COUNT    = 0x81B9,
  TO_REMOTE_CHR_SPAWN     = 0x9840,


  NET_CHECK_CRC           = 0x88CF,  // Someone is asking us to check the CRC of a certain file
  NET_ACKNOWLEDGE_CRC     = 0x6240,  // Someone is acknowledging a CRC request from us
  NET_SEND_CRC            = 0xA0E1,  // Someone is sending us a CRC
  NET_TRANSFER_FILE       = 0x16B5,  // Packet contains a file.
  NET_REQUEST_FILE        = 0x9ABF,  // Someone has asked us to send them a file
  NET_TRANSFER_ACK        = 0x2B22,  // Acknowledgement packet for a file send
  NET_CREATE_DIRECTORY    = 0xCFC9,  // Tell the peer to create the named directory
  NET_DONE_SENDING_FILES  = 0xCB08,  // Sent when there are no more files to send.
  NET_NUM_FILES_TO_SEND   = 0x9E40,  // Let the other person know how many files you're sending

  // Unused values
  //XXXX                    = ,
  //XXXX                    = ,
  //XXXX                    = ,
  //XXXX                    = ,
  //XXXX                    = ,
  //XXXX                    = ,
  //XXXX                    = ,
  //XXXX                    = ,
  //XXXX                    = ,
  //XXXX                    = ,
  //XXXX                    = ,
  //XXXX                    = ,
  //XXXX                    = ,
  //XXXX                    = ,
  //XXXX                    = 0xB312,
  //XXXX                    = 0x9D13,
  //XXXX                    = 0xEB6C,
  //XXXX                    = 0x4151,
  //XXXX                    = 0xA140,
  //XXXX                    = 0xB5DD,
  //XXXX                    = 0x7A54,
  //XXXX                    = 0xB64D,
};

// Networking constants
enum e_NetworkConstant
{
  NET_UNRELIABLE_CHANNEL  = 0,
  NET_GUARANTEED_CHANNEL  = 1,
  NET_EGOBOO_NUM_CHANNELS,
  NET_MAX_FILE_NAME       = 128,
  NET_MAX_FILE_TRANSFERS  = 1024, // Maximum files queued up at once
  NET_EGOBOO_NETWORK_PORT = 0x8741,
  NET_EGOBOO_SERVER_PORT  = 0x8742,
  NET_EGOBOO_CLIENT_PORT  = 0x8743
};

//---------------------------------------------------------------------------------------------

struct sGame;
struct sClient;
struct sServer;
struct sNFileState;

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------

struct sNetThread
{
  egoboo_key_t ekey;

  // thread status
  bool_t Active;             // Keep looping or quit?
  bool_t Paused;             // Is it paused?
  bool_t KillMe;             // someone requested that we terminate!
  bool_t Terminated;         // We are completely done.

  // thread callback info
  SDL_Thread     * Thread;
  SDL_Callback_Ptr Callback;

} ;
typedef struct sNetThread NetThread_t;

NetThread_t * NetThread_create(SDL_Callback_Ptr pcall);
bool_t        NetThread_destroy(NetThread_t ** pnt);
NetThread_t * NetThread_new(NetThread_t * nt, SDL_Callback_Ptr pcall);
bool_t        NetThread_delete(NetThread_t * nt);
bool_t        NetThread_started(NetThread_t * nt);
bool_t        NetThread_stop(NetThread_t * nt);
bool_t        NetThread_pause(NetThread_t * nt);
bool_t        NetThread_unpause(NetThread_t * nt);


//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
// data for handling asynchronous communication

#define REQEST_BUFFER_SIZE 1024

struct sNetAsynchData;

struct sPacketRequest
{
  egoboo_key_t ekey;

  bool_t       waiting, received;
  Uint32       starttimestamp, expiretimestamp;
  ENetAddress  address;
  Uint16       packettype;
  size_t       data_size;
  void        *data;
  ENetPeer   **peer;
};
typedef struct sPacketRequest PacketRequest_t;

PacketRequest_t * NetRequest_new(PacketRequest_t * pr);
PacketRequest_t * NetRequest_check(struct sNetAsynchData * prlist, size_t prcount, ENetEvent * pevent);
PacketRequest_t * NetRequest_getFree(struct sNetAsynchData * prlist, size_t prcount);
int               NetRequest_getFreeIndex(struct sNetAsynchData * prlist, size_t prcount);

bool_t NetRequest_test(PacketRequest_t * prequest);
bool_t NetRequest_release(PacketRequest_t * prequest);

typedef Uint8 NetRequestBuff[REQEST_BUFFER_SIZE];

struct sNetAsynchData
{
  PacketRequest_t     req;
  NetRequestBuff buf;
};
typedef struct sNetAsynchData NetAsynchData_t;

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
// Network connection information

#define MAXCONNECTION 16

struct sCListOut_Info
{
  egoboo_key_t ekey;

  void     * client_data;
  ENetPeer * peer;
};
typedef struct sCListOut_Info CListOut_Info_t;

CListOut_Info_t * CListOut_Info_new(CListOut_Info_t *);
bool_t            CListOut_Info_delete(CListOut_Info_t *);


// definition of the outgoing connection list
struct sCListOut
{
  // outgoing channels
  int           Count;
  int           References[MAXCONNECTION];
  CListOut_Info_t Clients[MAXCONNECTION];
};
typedef struct sCListOut CListOut_t;

bool_t     CListOut_prune(CListOut_t * co);
ENetPeer * CListOut_add(CListOut_t * co, ENetHost * host, ENetAddress * address, void * client_data);
bool_t     CListOut_remove(CListOut_t * co, ENetPeer * peer);
void       CListOut_close(CListOut_t * clo, void * client_data);

//---------------------------------------------------------------------------------------------

struct sCListIn_Info
{
  char          Name[NETNAMESIZE];       // logon/screen name
  char          Hostname[NETNAMESIZE];   // Net name of connected machine
  ENetAddress   Address;                 // Network address of machine
  int           Slot;

  // a pointer to an event handler for events commming in to this cinnection
  // handler_ptr will normally point to something like sv_handlePacket()
  // This data needs to be established when the connection is opened
  void             * handler_data;
  EventHandler_Ptr_t   handler_ptr;
};
typedef struct sCListIn_Info CListIn_Info_t;

struct sCListIn_Client
{
  egoboo_key_t ekey;

  Uint32             guid;
  void             * handler_data;
  EventHandler_Ptr_t   handler_ptr;
};
typedef struct sCListIn_Client CListIn_Client_t;

CListIn_Client_t * CListIn_Client_new(CListIn_Client_t * cli, EventHandler_Ptr_t handler, void * data, Uint32 net_guid);
bool_t           CListIn_Client_delete(CListIn_Client_t * cli);


struct sCListIn
{
  int            Count;
  CListIn_Info_t   Info[MAXCONNECTION];

  int            Client_count;
  CListIn_Client_t ClientLst[MAXCONNECTION];
};
typedef struct sCListIn CListIn_t;

bool_t           CListIn_prune(CListIn_t * ci);
CListIn_Info_t * CListIn_add(CListIn_t * ci, CListIn_Info_t * info);
bool_t           CListIn_remove(CListIn_t * ci, ENetPeer * peer);

CListIn_Client_t * CListIn_add_client(CListIn_t * ci, EventHandler_Ptr_t handler, void * data, Uint32 net_guid);
bool_t           CListIn_remove_client(CListIn_t * ci, Uint32 id);
void             CListIn_close_client(CListIn_t * cli, void * client_data);
CListIn_Client_t * CListIn_find(CListIn_t * cli, Uint32 id);

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
// a generic network host "wrapper"

#define NETHOST_REQ_COUNT   32

struct sNetHost
{
  egoboo_key_t       ekey;
  int              references;

  ENetHost       * Host;

  NetThread_t        nthread;

  // outgoing channels
  CListOut_t cout;

  // incomming channels
  CListIn_t  cin;

  // data for handling asynchronous transfers
  NetAsynchData_t asynch[NETHOST_REQ_COUNT];

};
typedef struct sNetHost NetHost_t;

// create/destroy and instance of the NetHost_t
NetHost_t * NetHost_create(SDL_Callback_Ptr pcall);
bool_t    NetHost_destroy(NetHost_t ** pnh );

// start and stop the NetHost_t
retval_t  NetHost_startUp( NetHost_t * nh, Uint16 port );
retval_t  NetHost_shutDown( NetHost_t * nh );

// register a client with the NetHost_t so we can dispatch incoming packets to it
Uint32 NetHost_register(NetHost_t * nh, EventHandler_Ptr_t client_handler, void * client_data, Uint32 net_guid);
bool_t NetHost_unregister(NetHost_t * nh, Uint32 id);

// route packets to the correct client
retval_t NetHost_dispatch( NetHost_t * nh );

// close a connection, given connection data
bool_t NetHost_close(NetHost_t * nh, void * client_data);

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
// the actual network state

#define NET_REQ_SIZE 1024


struct sNet
{
  egoboo_key_t ekey;

  // external links
  struct sGame      * parent;
  struct sNFileState * nfs;

};
typedef struct sNet Net_t;

Net_t * CNet_create(struct sGame * gs);
bool_t CNet_destroy(Net_t ** pns);
bool_t CNet_initialize(Net_t * ns);
//bool_t CNet_shutDown(Net_t * ns);

//------------------------------------------------------------------
// Packet writing
struct s_local_packet
{
  Uint32 head;                             // The write head
  Uint32 size;                             // The size of the packet
  Uint8  buffer[MAXSENDSIZE];              // The data packet
};
typedef struct s_local_packet SYS_PACKET;

//------------------------------------------------------------------
struct s_stream
{
  FILE  * pfile;
  Uint8 * data;
  size_t data_size, readLocation;
};
typedef struct s_stream STREAM;

//------------------------------------------------------------------
struct s_remote_packet
{
  ENetPacket * pkt;
  STREAM wrapper;
};
typedef struct s_remote_packet NET_PACKET;

//------------------------------------------------------------------
struct s_time_latch
{
  bool_t valid;
  Uint32 stamp;
  Latch_t  latch;
};
typedef struct s_time_latch TIME_LATCH;

typedef TIME_LATCH CHR_TIME_LATCH[MAXLAG];

//---------------------------------------------------------------------------------------------
struct s_time_latch_buffer
{
  Uint32 numtimes;
  Uint32 nextstamp;                    // Expected timestamp
  CHR_TIME_LATCH buffer[CHRLST_COUNT];
};
typedef struct s_time_latch_buffer TIME_LATCH_BUFFER;

//---------------------------------------------------------------------------------------------
// Networking functions
//---------------------------------------------------------------------------------------------

retval_t net_startUp(struct sConfigData * cd);
retval_t net_shutDown();
bool_t   net_Started();

bool_t     net_handlePacket(Net_t * ns, ENetEvent *event);
ENetPeer * net_disconnectPeer(ENetPeer * peer, int granularity_ms, int timeout_ms);
retval_t   net_waitForPacket(NetAsynchData_t * asynch_list, ENetPeer * peer, Uint32 timeout, Uint16 packet_type, size_t * data_size);

ENetPeer * net_startPeer(ENetHost * host, ENetAddress * address );
ENetPeer * net_startPeerByName(ENetHost * host, const char* connect_name, const int connect_port );
ENetPeer * net_stopPeer(ENetPeer * peer);

void net_logf(const char *format, ...);

bool_t net_sendSysPacketToAllPeers(ENetHost * host, SYS_PACKET * egop);
bool_t net_sendSysPacketToAllPeersGuaranteed(ENetHost * host, SYS_PACKET * egop);

PacketRequest_t * net_prepareWaitForPacket(NetAsynchData_t * request_list, ENetPeer ** peer, Uint32 timeout, Uint16 packet_type);

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
retval_t net_copyFileToAllPeers(Net_t * ns, char *source, char *dest);
retval_t net_copyFileToHost(Net_t * ns, char *source, char *dest);
retval_t net_copyDirectoryToHost(Net_t * ns, char *dirname, char *todirname);
retval_t net_copyDirectoryToAllPeers(Net_t * ns, char *dirname, char *todirname);
retval_t net_copyDirectoryToPeer(Net_t * ns, ENetPeer * peer, char *dirname, char *todirname);

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------

bool_t net_sendSysPacketToPeer(ENetPeer *peer, SYS_PACKET * egop);
bool_t net_sendSysPacketToPeerGuaranteed(ENetPeer *peer, SYS_PACKET * egop);

INLINE bool_t net_startNewSysPacket(SYS_PACKET * egop);
INLINE void   net_packet_startReading(NET_PACKET * p, ENetPacket * enpkt);
INLINE void   net_packet_doneReading(NET_PACKET * p);
INLINE size_t net_packet_remainingSize(NET_PACKET * p);

INLINE bool_t packet_readString(NET_PACKET * p, char *buffer, int maxLen);
INLINE Uint8  net_packet_readUint8(NET_PACKET * p);
INLINE Sint8  net_packet_readSint8(NET_PACKET * p);
INLINE Uint16 net_packet_readUint16(NET_PACKET * p);
INLINE Uint16 net_packet_peekUint16(NET_PACKET * p);
INLINE Sint16 net_packet_readSint16(NET_PACKET * p);
INLINE Uint32 net_packet_readUint32(NET_PACKET * p);
INLINE Sint32 net_packet_readSint32(NET_PACKET * p);

INLINE void sys_packet_addUint8(SYS_PACKET * egop, Uint8 uc);
INLINE void sys_packet_addSint8(SYS_PACKET * egop, Sint8 sc);
INLINE void sys_packet_addUint16(SYS_PACKET * egop, Uint16 us);
INLINE void sys_packet_addSint16(SYS_PACKET * egop, Sint16 ss);
INLINE void sys_packet_addUint32(SYS_PACKET * egop, Uint32 ui);
INLINE void sys_packet_addSint32(SYS_PACKET * egop, Sint32 si);
INLINE void sys_packet_addFString(SYS_PACKET * egop, const char *format, ...);
INLINE void sys_packet_addString(SYS_PACKET * egop, const char *string);

INLINE bool_t stream_startFile(STREAM * pwrapper, FILE * pfile);
INLINE bool_t stream_startRaw(STREAM * pwrapper, Uint8 * buffer, size_t buffer_size);
INLINE bool_t stream_startLocal(STREAM * pwrapper, SYS_PACKET * pegopkt);
INLINE bool_t stream_startENet(STREAM * pwrapper, ENetPacket * packet);
INLINE bool_t stream_startRemote(STREAM * pwrapper, NET_PACKET * pnetpkt);
INLINE bool_t stream_done(STREAM * pwrapper);

INLINE bool_t stream_readString(STREAM * p, char *buffer, size_t maxLen);
INLINE Uint8  stream_readUint8(STREAM * p);
INLINE Sint8  stream_readSint8(STREAM * p);
INLINE Uint16 stream_readUint16(STREAM * p);
INLINE Uint16 stream_peekUint16(STREAM * p);
INLINE Sint16 stream_readSint16(STREAM * p);
INLINE Uint32 stream_readUint32(STREAM * p);
INLINE Sint32 stream_readSint32(STREAM * p);
INLINE size_t stream_remainingSize(STREAM * p);

//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------

void    do_chat_input();
void    net_sayHello(struct sGame * gs);
bool_t  net_beginGame(struct sGame * gs);

PLA_REF add_player(struct sGame * gs,  CHR_REF character, Uint8 device );
char *  convert_host(Uint32 host);

char * net_crack_enet_packet_type(ENetPacket * enpkt);
char * net_crack_sys_packet_type(SYS_PACKET * enpkt);

void net_KickOnePlayer(ENetPeer * peer);

void     * net_getService(Uint32 id);
Uint32     net_addService(NetHost_t * host, void * data);
bool_t     net_removeService(Uint32 id);
