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
/// @brief Network Game Client Implementation
/// @details Even if networking is not enabled, the local latches are routed through this code.
///  This code is currently under development.

#include "Client.h"

#include "Log.h"
#include "NetFile.h"
#include "game.h"

#include "egoboo_strutil.h"
#include "egoboo.h"

#include "Network.inl"
#include "egoboo_stream.inl"
#include "egoboo_types.inl"

#include <enet/enet.h>
#include <assert.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// the cl module info

static bool_t    _cl_atexit_registered = bfalse;
static NetHost_t * _cl_host = NULL;

static retval_t  _cl_Initialize( void );
static void      _cl_Quit( void );
static retval_t  _cl_startUp( void );
static retval_t  _cl_shutDown( void );
static int       _cl_HostCallback(void *);

bool_t     cl_Running(Client_t * cs)  { return cl_Started() && !cs->waiting; }

static int _cl_HostCallback(void *);

bool_t cl_handle_chr_spawn(Client_t * cs, ENetEvent *event);


//--------------------------------------------------------------------------------------------
// private Client_t initialization
static Client_t * CClient_new(Client_t * cs, Game_t * gs);
static bool_t    CClient_delete(Client_t * cs);


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

//retval_t cl_startUp(Client_t * cs)
//{
//  retval_t host_return;
//
//  if(cl_initialized) return rv_succeed;
//
//  if(NULL == cs)
//  {
//    cs = &ACClient;
//  }
//
//  host_return = NetHost_startUp(&(cs->host), NET_EGOBOO_CLIENT_PORT);
//
//  if(!cl_atexit_registered)
//  {
//    atexit(cl_Quit);
//    cl_atexit_registered = btrue;
//  }
//
//  cl_initialized = btrue;
//
//  return host_return;
//}
//
////--------------------------------------------------------------------------------------------
//
//retval_t cl_shutDown(Client_t * cs)
//{
//  retval_t delete_return;
//
//  if(!cl_initialized) return rv_succeed;
//
//  if(NULL == cs)
//  {
//    cs = &ACClient;
//  }
//
//  delete_return = CClient_delete(cs) ? rv_succeed : rv_fail;
//
//  cl_initialized = bfalse;
//
//  return delete_return;
//}
//
//--------------------------------------------------------------------------------------------

retval_t Client_startUp(Client_t * cs)
{
  NetHost_t * cl_host;

  // error trap bad inputs
  if ( !EKEY_PVALID(cs) || !EKEY_PVALID(cs->parent) ) return rv_error;

  if(net_Started())
  {
    // only start this stuff if we are going to actually connect to the network

    cs->host     = cl_getHost();
    cs->net_guid = net_addService(cs->host, cs);
    cs->net_guid = NetHost_register(cs->host, net_handlePacket, cs, cs->net_guid);
  }
  else
  {
    Client_shutDown(cs);
    return rv_error;
  }

  // return btrue if the client is already started
  cl_host = cl_getHost();
  if(NULL == cl_host) return rv_error;

  if(!cl_Started())
  {
    // Try to create a new session
    net_logf("---------------------------------------------\n");
    net_logf("NET INFO: Client_startUp() - Starting game client\n");

    _cl_startUp();
  }
  else
  {
    net_logf("---------------------------------------------\n");
    net_logf("NET INFO: Client_startUp() - Restarting game server\n");
  }

  // To function, the client requires the file transfer component
  NFileState_startUp(cs->parent->ns->nfs);

  return cl_Started() ? rv_succeed : rv_fail;
}

//--------------------------------------------------------------------------------------------
retval_t Client_shutDown(Client_t * cs)
{
  if(NULL == cs) return rv_error;

  if(!cl_Started()) return rv_succeed;

  net_logf("NET INFO: Client_shutDown() - Shutting down a game client... ");

  // shut down the connection to the game host
  net_stopPeer( cs->gamePeer );
  cs->gamePeer = NULL;

  // close our connection to the our client host
  NetHost_close( cl_getHost(), cs );

  net_logf("Done\n");

  return rv_succeed;
}
//--------------------------------------------------------------------------------------------
//
//void cl_Quit( void )
//{
//  CClient_delete(&ACClient);
//};


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Client_t * Client_create(Game_t * gs)
{
  Client_t * cs = EGOBOO_NEW(Client_t);
  return CClient_new(cs, gs);
}

//--------------------------------------------------------------------------------------------
bool_t Client_destroy(Client_t ** pcs)
{
  bool_t retval;

  if(NULL == pcs || NULL == *pcs) return bfalse;
  if( !EKEY_PVALID( (*pcs) ) ) return btrue;

  retval = CClient_delete(*pcs);
  EGOBOO_DELETE( *pcs );

  return retval;
}

//--------------------------------------------------------------------------------------------
Client_t * CClient_new(Client_t * cs, Game_t * gs)
{
  int cnt;

  //fprintf( stdout, "CClient_new()\n");

  if(NULL == cs) return cs;

  CClient_delete(cs);

  memset( cs, 0, sizeof(Client_t) );

  EKEY_PNEW(cs, Client_t);

  cs->parent         = gs;
  cs->gamePeer       = NULL;
  cs->logged_on      = bfalse;
  cs->selectedPlayer = -1;
  cs->selectedModule = -1;

  for(cnt=0; cnt<MAXMODULE; cnt++)
  {
    ModInfo_new(cs->rem_mod + cnt);
  };

  Client_reset_latches(cs);

  chr_spawn_queue_new(&(cs->chr_queue), 256);

  return cs;
}

//--------------------------------------------------------------------------------------------
bool_t CClient_delete(Client_t * cs)
{
  if(NULL == cs) return bfalse;

  if(!EKEY_PVALID(cs)) return btrue;

  EKEY_PINVALIDATE(cs);

  net_removeService(cs->net_guid);
  NetHost_unregister(cs->host, cs->net_guid);

  chr_spawn_queue_delete(&(cs->chr_queue));

  return btrue;
}

//--------------------------------------------------------------------------------------------
Client_t * Client_renew(Client_t * cs)
{
  Game_t * gs;

  if(NULL == cs) return cs;

  gs = cs->parent;

  CClient_delete(cs);
  return CClient_new(cs, gs);
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void cl_frameStep()
{
}

//--------------------------------------------------------------------------------------------
//bool_t cl_stopClient(Client_t * cs)
//{
//  NetHost_t * cl_host;
//  if(NULL == cs) return bfalse;
//  if (!net_Started()) return bfalse;
//  if(cl_Started()) return btrue;
//
//  net_logf("NET INFO: cl_stopClient: Stopping the client ... ");
//
//  cl_host = cl_getHost();
//
//  enet_host_destroy(cl_host->Host);
//  cl_host->Host   = NULL;
//  cl_host->nthread.Active = bfalse;
//
//  net_logf("Done\n");
//
//  return btrue;
//};

//--------------------------------------------------------------------------------------------
ENetPeer * cl_startPeer(const char* hostname)
{
  /// @details BB@> start the peer connection process, do not wait for confirmation

  ENetPeer * peer;
  NetHost_t  * cs_host;

  // ensure a valid cs->host->Host
  if( !cl_Started() )
  {
    net_logf("NET WARN: cl_startPeer() - Trying to open a connection without a valid client. Restarting the client.\n");
    _cl_startUp();
  }

  cs_host = cl_getHost();
  peer = net_startPeerByName(cs_host->Host, hostname, NET_EGOBOO_SERVER_PORT);

  if (NULL ==peer)
  {
    net_logf("NET INFO: cl_startPeer() - Cannot open channel to host.\n");
  }

  return peer;
}


//--------------------------------------------------------------------------------------------
//bool_t cl_connectRemote(Client_t * cs, const char* hostname, int slot)
//{
//  /// @details ZZ@> This function tries to connect onto a server
//
//  int cnt;
//
//  // ensure a valid cs->host->Host
//  if(!cl_Started())
//  {
//    cl_startUp();
//  }
//  if(!cl_Started()) return bfalse;
//
//  // ensure that the cs->gamePeer is free
//  CClient_disconnect(cs);
//
//  // Now connect to the remote host
//  cs->rem_req_peer[slot] = cl_startPeer(cs, hostname);
//  if (NULL ==cs->rem_req_peer[slot])
//  {
//    net_logf("NET INFO: cl_connectRemote() - Cannot open channel to host.\n");
//    return bfalse;
//  }
//
//  // Wait for up to 5 seconds for the connection attempt to succeed
//  cnt = 0;
//  while(cnt<500 && ENET_PEER_STATE_CONNECTED != cs->rem_req_peer[slot]->state)
//  {
//    SDL_Delay(10);
//    cnt++;
//  }
//
//  return (ENET_PEER_STATE_CONNECTED == cs->rem_req_peer[slot]->state);
//};
//
//--------------------------------------------------------------------------------------------
bool_t CClient_disconnect(Client_t * cs)
{
  if(NULL ==cs) return bfalse;

  SDL_Delay(200);

  if(!cl_Started())
  {
    cs->gamePeer = NULL;
  }
  else
  {
    cs->gamePeer = net_disconnectPeer(cs->gamePeer, 10, 5000);
  };

  return btrue;
}



//--------------------------------------------------------------------------------------------
bool_t CClient_connect(Client_t * cs, const char* hostname)
{
  /// @details ZZ@> This function tries to connect onto a server

  int cnt;

  // throw away stupid stuff
  if(NULL == cs )
  {
    net_logf("NET ERROR: CClient_connect() - Called with invalid parameters.\n");
    return bfalse;
  }

  if(  !VALID_CSTR(hostname)  )
  {
    net_logf("NET WARN: CClient_connect() - Called with null filename.\n");
    return bfalse;
  }

  // ensure a valid client host
  if(!cl_Started())
  {
    net_logf("NET WARN: CClient_connect() - Starting client.\n");
    _cl_startUp();
  }
  if(!cl_Started())
  {
    net_logf("NET ERROR: CClient_connect() - Cannot start client.\n");
    return bfalse;
  }

  // ensure that the cs->gamePeer is not connected
  CClient_disconnect(cs);

  // Now connect to the remote host
  net_logf("NET INFO: CClient_connect() - Attempting to open the game connection to %s:0x%04x from %s:0x%04x... ", hostname, NET_EGOBOO_SERVER_PORT, convert_host(cs->host->Host->address.host), cs->host->Host->address.port);

  cs->gamePeer = cl_startPeer(hostname);
  if (NULL ==cs->gamePeer)
  {
    net_logf("Failed!\n");
    return bfalse;
  }

  // Wait for up to 5 seconds for the connection attempt to succeed
  cnt = 0;
  while(cnt<500 && ENET_PEER_STATE_CONNECTED != cs->gamePeer->state)
  {
    SDL_Delay(10);
    cnt++;
  }

  if(ENET_PEER_STATE_CONNECTED == cs->gamePeer->state)
  {
    net_logf("Connected!\n");
  }
  else
  {
    net_logf("Waiting... Game server is slow to respond.\n");
  }

  return (ENET_PEER_STATE_CONNECTED == cs->gamePeer->state);
}


//--------------------------------------------------------------------------------------------
retval_t Client_joinGame(Client_t * cs, const char * hostname)
{
  /// @details ZZ@> This function tries to join one of the sessions we found

  SYS_PACKET   egopkt;
  STREAM   stream;
  retval_t wait_return;
  Uint8    buffer[NET_REQ_SIZE];

  // throw away stupid stuff
  if(NULL == cs ||  !VALID_CSTR(hostname) ) return rv_fail;

  if ( !net_Started()  ) return rv_error;
  if ( !Client_startUp(cs) ) return rv_fail;

  if(!CClient_connect(cs, hostname)) return rv_error;

  net_startNewSysPacket(&egopkt);
  sys_packet_addUint16(&egopkt, TO_HOST_LOGON);           // try to logon
  sys_packet_addString(&egopkt, CData.net_messagename);             // logon name
  Client_sendPacketToHost(cs, &egopkt);

  // wait up to 5 seconds for the client to respond to the request
  wait_return = net_waitForPacket(cs->host->asynch, cs->gamePeer, 5000, TO_REMOTE_LOGON, NULL);
  CClient_disconnect(cs);
  if(rv_fail == wait_return || rv_error == wait_return) return rv_error;

  stream_startRaw(&stream, buffer, sizeof(buffer));
  assert(TO_REMOTE_LOGON == stream_readUint16(&stream));

  if(bfalse == stream_readUint8(&stream))
  {
    //login refused
    CClient_disconnect(cs);
    cs->gameID = (Uint32)(-1);
    return rv_fail;
  };

  cs->gameID = stream_readUint8(&stream);

  return rv_succeed;

}

//--------------------------------------------------------------------------------------------
bool_t Client_unjoinGame(Client_t * cs)
{
  NetHost_t * cl_host;
  SYS_PACKET egopkt;

  if(NULL ==cs || !cl_Started()) return bfalse;

  cl_host = cl_getHost();

  if( cs->logged_on )
  {
    // send logoff messages to all the clients
    net_logf("NET INFO: Client_unjoinGame() - Telling the server we are logging off.\n");
    net_startNewSysPacket(&egopkt);
    sys_packet_addUint16(&egopkt, TO_HOST_LOGOFF);
    Client_sendPacketToHostGuaranteed(cs, &egopkt);

    cs->logged_on = bfalse;
  }

  // close all outgoing connections
  net_logf("NET INFO: Client_unjoinGame() - Disconnecting from the client host.\n");
  Client_shutDown( cs );


  return btrue;
}

//--------------------------------------------------------------------------------------------
void Client_talkToHost(Client_t * cs)
{
  /// @details ZZ@> This function sends the latch packets to the host machine

  PLA_REF player;
  SYS_PACKET egopkt;
  Game_t * gs;

  if(!cl_Started() || !net_Started()) return;

  gs = cs->parent;

  // Start talkin'
  if (gs->wld_frame > STARTTALK && cs->loc_pla_count>0)
  {
    CHR_REF ichr;
    Uint32 stamp = ups_loops;
    Uint32 time = (stamp + 1) & LAGAND;

    net_startNewSysPacket(&egopkt);
    sys_packet_addUint16(&egopkt, TO_HOST_LATCH);     // The message header
    sys_packet_addUint32(&egopkt, stamp);

    for (player = 0; player < PLALST_COUNT; player++)
    {
      // Find the local players
      if (!gs->PlaList[player].Active || INBITS_NONE==gs->PlaList[player].device) continue;

      ichr = gs->PlaList[player].chr_ref;
      sys_packet_addUint16(&egopkt, REF_TO_INT(ichr));                                   // The character index
      sys_packet_addUint8 (&egopkt, cs->tlb.buffer[REF_TO_INT(ichr)][time].latch.b);         // Player_t button states
      sys_packet_addSint16(&egopkt, cs->tlb.buffer[REF_TO_INT(ichr)][time].latch.x*SHORTLATCH);    // Player_t motion
      sys_packet_addSint16(&egopkt, cs->tlb.buffer[REF_TO_INT(ichr)][time].latch.y*SHORTLATCH);    // Player_t motion
    }

    // Send it to the host
    Client_sendPacketToHost(cs, &egopkt);
  }
}

//--------------------------------------------------------------------------------------------
void Client_unbufferLatches(Client_t * cs)
{
  /// @details ZZ@> This function sets character latches based on player input to the host

  CHR_REF chr_cnt;
  Uint32  uiTime, stamp;
  Sint32  dframes;
  Chr_t  * pchr;
  CHR_TIME_LATCH * ptl;

  Game_t * gs = cs->parent;

  // Copy the latches
  stamp  = ups_loops;
  uiTime = stamp & LAGAND;
  for (chr_cnt = 0; chr_cnt < CHRLST_COUNT; chr_cnt++)
  {
    if( !ACTIVE_CHR(gs->ChrList, chr_cnt) ) continue;

    pchr = gs->ChrList + chr_cnt;
    ptl  = cs->tlb.buffer + REF_TO_INT(chr_cnt);

    if(!(*ptl)[uiTime].valid) continue;
    if(INVALID_TIMESTAMP == (*ptl)[uiTime].stamp) continue;

    dframes = (float)((*ptl)[uiTime].stamp - stamp);

    // copy the data over
    pchr->aistate.latch.x = (*ptl)[uiTime].latch.x;
    pchr->aistate.latch.y = (*ptl)[uiTime].latch.y;
    pchr->aistate.latch.b = (*ptl)[uiTime].latch.b;

    // set the data to invalid
    (*ptl)[uiTime].valid = bfalse;
    (*ptl)[uiTime].stamp = INVALID_TIMESTAMP;
  }
  cs->tlb.numtimes--;
}

//--------------------------------------------------------------------------------------------
bool_t cl_handlePacket(Client_t * cs, ENetEvent *event)
{
  Game_t * gs;

  Uint16 header;
  STRING filename;   // also used for reading various strings
  Uint32 stamp;
  int uiTime;
  SYS_PACKET egopkt;
  NET_PACKET netpkt;
  bool_t retval = bfalse;

  if(NULL == cs) return bfalse;

  gs = cs->parent;

  // do some error trapping
  //if(!cl_Started()) return bfalse;

  // send some log info
  net_logf("NET INFO: cl_handlePacket: Processing... ");

  // rewind the packet
  net_packet_startReading(&netpkt, event->packet);
  header = net_packet_readUint16(&netpkt);

  // process our messages
  retval = btrue;
  switch (header)
  {

  case TO_REMOTE_CHR_SPAWN:
    cl_handle_chr_spawn(cs, event);
    break;

  case TO_REMOTE_KICK:
    // we were kicked from the server

    net_logf("TO_REMOTE_KICK\n");

    cs->logged_on = bfalse;
    cs->waiting   = btrue;

    // shut down the connection gracefully on this end
    CClient_disconnect(cs);

    break;

  case TO_REMOTE_LOGON:

    // someone has sent a message saying we are logged on

    net_logf("TO_REMOTE_LOGON\n");

    if( (cs->gamePeer->address.host == event->peer->address.host) &&
        (cs->gamePeer->address.port == event->peer->address.port) )
    {
      cs->logged_on = btrue;
    }

    break;

  case TO_REMOTE_FILE_COUNT:
    // the server is telling us to expect a certain number of files to download

    net_logf("TO_REMOTE_FILE_COUNT\n");

    //numfileexpected += net_packet_readUint32(&netpkt);
    //numplayerrespond++;

    break;

  case TO_REMOTE_MODULE:
    // the server is telling us which module it is hosting

    net_logf("TO_REMOTE_MODULE\n");

    if (cs->waiting)
    {
      cs->req_modstate.seed = net_packet_readUint32(&netpkt);
      packet_readString(&netpkt, filename, 255);
      strcpy(cs->req_mod.loadname, filename);

      // Check to see if the module exists
      cs->selectedModule = module_find(cs->req_mod.loadname, cs->rem_mod, MAXMODULE);
      if (cs->selectedModule == -1)
      {
        net_startNewSysPacket(&egopkt);
        sys_packet_addUint16(&egopkt, TO_HOST_MODULEBAD);
        Client_sendPacketToHostGuaranteed(cs, &egopkt);
      }
      else
      {
        // Make ourselves ready
        cs->waiting = bfalse;

        // Tell the host we're ready
        net_startNewSysPacket(&egopkt);
        sys_packet_addUint16(&egopkt, TO_HOST_MODULEOK);
        Client_sendPacketToHostGuaranteed(cs, &egopkt);
      }
    }

    break;

  case TO_REMOTE_START:

    // the everyone has responded that they are ready. Stop waiting.

    net_logf("TO_REMOTE_START\n");

    cs->waiting = bfalse;

    break;

  case TO_REMOTE_RTS:

    // the host is sending an RTS order

    net_logf("TO_REMOTE_RTS\n");

    /*  whichorder = get_empty_order();
    if(whichorder < MAXORDER)
    {
    // Add the order on the remote machine
    cnt = 0;
    while(cnt < MAXSELECT)
    {
    who = net_packet_readUint8(event->packet);
    GOrder.who[whichorder][cnt] = who;
    cnt++;
    }
    what = net_packet_readUint32(event->packet);
    when = net_packet_readUint32(event->packet);
    GOrder.what[whichorder] = what;
    GOrder.when[whichorder] = when;
    }*/

    break;

  case TO_REMOTE_FILE:

    // the server is sending us a file

    net_logf("TO_REMOTE_FILE - this is handled elsewhere...\n");



    //packet_readString(&netpkt, filename, 255);
    //newfilesize = net_packet_readUint32(&netpkt);

    //// Change the size of the file if need be
    //newfile = 0;
    //file = fopen(filename, "rb");
    //if (file)
    //{
    //  fseek(file, 0, SEEK_END);
    //  filesize = ftell(file);
    //  fclose(file);

    //  if (filesize != newfilesize)
    //  {
    //    // Destroy the old file
    //    newfile = 1;
    //  }
    //}
    //else
    //{
    //  newfile = 1;
    //}

    //if (newfile)
    //{
    //  // file must be created.  Write zeroes to the file to do it
    //  //numfile++;
    //  file = fopen(filename, "wb");
    //  if (file)
    //  {
    //    filesize = 0;
    //    while (filesize < newfilesize)
    //    {
    //      fputc(0, file);
    //      filesize++;
    //    }
    //    fclose(file);
    //  }
    //}

    //// Go to the position in the file and copy data
    //fileposition = net_packet_readUint32(&netpkt);
    //file = fopen(filename, "r+b");
    //if (file)
    //{
    //  if (fseek(file, fileposition, SEEK_SET) == 0)
    //  {
    //    while (net_packet_remainingSize(&netpkt) > 0)
    //    {
    //      fputc(net_packet_readUint8(&netpkt), file);
    //    }
    //  }
    //  fclose(file);
    //}

    break;

  case TO_REMOTE_DIR:
    // the server is telling us to create a directory on our machine
    net_logf("TO_REMOTE_DIR - this is handled elsewhere...\n");

    //packet_readString(&netpkt, filename, 255);
    //fs_createDirectory(filename);

    break;

  case TO_REMOTE_LATCH:
    // the server is sending us a latch
    net_logf("TO_REMOTE_LATCH\n");

    stamp  = net_packet_readUint32(&netpkt);
    uiTime = stamp & LAGAND;

    if (INVALID_TIMESTAMP == cs->tlb.nextstamp)
    {
      cs->tlb.nextstamp = stamp;
    }
    if (stamp < cs->tlb.nextstamp)
    {
      net_logf("NET WARNING: net_handlePacket: OUT OF ORDER SYS_PACKET\n");
      cs->outofsync = btrue;
    }
    if (stamp <= ups_loops)
    {
      net_logf("NET WARNING: net_handlePacket: LATE SYS_PACKET\n");
      cs->outofsync = btrue;
    }
    if (stamp > cs->tlb.nextstamp)
    {
      net_logf("NET WARNING: net_handlePacket: MISSED SYS_PACKET\n");
      cs->tlb.nextstamp = stamp;  // Still use it
      cs->outofsync = btrue;
    }
    if (stamp == cs->tlb.nextstamp)
    {
      Uint16 ichr;

      // Remember that we got it
      cs->tlb.numtimes++;

      for(ichr=0; ichr<CHRLST_COUNT; ichr++)
      {
        cs->tlb.buffer[ichr][uiTime].stamp = INVALID_TIMESTAMP;
        cs->tlb.buffer[ichr][uiTime].valid = bfalse;
      };

      // Read latches for each player sent
      while (net_packet_remainingSize(&netpkt) > 0)
      {
        ichr = net_packet_readUint16(&netpkt);
        if(VALID_CHR_RANGE(ichr))
        {
          CHR_TIME_LATCH *ptl = cs->tlb.buffer + ichr;

          (*ptl)[uiTime].stamp   = stamp;
          (*ptl)[uiTime].valid   = btrue;
          (*ptl)[uiTime].latch.b = net_packet_readUint8(&netpkt);
          (*ptl)[uiTime].latch.x = (float)net_packet_readSint16(&netpkt) / (float)SHORTLATCH;
          (*ptl)[uiTime].latch.y = (float)net_packet_readSint16(&netpkt) / (float)SHORTLATCH;
        }
      };

      cs->tlb.nextstamp = stamp + 1;
    }

    break;

  default:
    retval = bfalse;
    net_logf("UNKNOWN\n");
    break;

  }



  return retval;
}

//--------------------------------------------------------------------------------------------
void Client_reset_latches(Client_t * cs)
{
  CHR_REF chr_cnt;
  if(NULL ==cs) return;

  for(chr_cnt = 0; chr_cnt<CHRLST_COUNT; chr_cnt++)
  {
    Client_resetTimeLatches(cs, chr_cnt);
  };

  cs->tlb.nextstamp = INVALID_TIMESTAMP;
  cs->tlb.numtimes   = STARTTALK + 1;
}


//--------------------------------------------------------------------------------------------
void Client_resetTimeLatches(Client_t * cs, CHR_REF ichr)
{
  int cnt;
  CHR_TIME_LATCH *ptl;

  if(NULL ==cs) return;
  if( !VALID_CHR_RANGE(ichr) ) return;
  ptl = cs->tlb.buffer + REF_TO_INT(ichr);

  for(cnt=0; cnt < MAXLAG; cnt++)
  {
    (*ptl)[cnt].valid   = bfalse;
    (*ptl)[cnt].stamp   = INVALID_TIMESTAMP;

    Latch_clear( &((*ptl)[cnt].latch) );
  }
}

//--------------------------------------------------------------------------------------------
void Client_bufferLatches(Client_t * cs)
{
  /// @details ZZ@> This function buffers the player data

  Uint32 stamp, uiTime;
  PLA_REF player;
  CHR_REF ichr;
  CHR_TIME_LATCH *ptl;

  Game_t * gs   = cs->parent;
  PChr_t chrlst = gs->ChrList;
  PPla_t plalst = gs->PlaList;

  stamp = ups_loops + 1;
  uiTime = stamp & LAGAND;
  for (player = 0; player < PLALST_COUNT; player++)
  {
    if (!VALID_PLA(plalst, player)) continue;
    ichr = plalst[player].chr_ref;

    if( !ACTIVE_CHR(chrlst, ichr) ) continue;
    ptl = cs->tlb.buffer + REF_TO_INT(ichr);

    (*ptl)[uiTime].valid   = btrue;
    (*ptl)[uiTime].stamp   = stamp;
    (*ptl)[uiTime].latch.x = ((Sint32)(plalst[player].latch.x * SHORTLATCH)) / SHORTLATCH;
    (*ptl)[uiTime].latch.y = ((Sint32)(plalst[player].latch.y * SHORTLATCH)) / SHORTLATCH;
    (*ptl)[uiTime].latch.b = plalst[player].latch.b;
  }

  cs->tlb.numtimes++;
}

//--------------------------------------------------------------------------------------------
int _cl_HostCallback(void * data)
{
  NetHost_t     * cl_host;
  NetThread_t   * nthread;
  retval_t      dispatch_return;

  // the client host is passed as the original argument
  cl_host = (NetHost_t *)data;

  // fail to start
  if(NULL == cl_host)
  {
    net_logf("NET ERROR: _cl_HostCallback() - Cannot start thread.\n");
    return rv_error;
  }

  net_logf("NET INFO: _cl_HostCallback() thread - starting normally.\n");

  nthread = &(cl_host->nthread);
  while(NULL !=cl_host && !nthread->KillMe)
  {
    // the client host could be removed/deleted at any time
    cl_host = cl_getHost();
    nthread = &(cl_host->nthread);

    if(!nthread->Paused)
    {
      dispatch_return = NetHost_dispatch(cl_host) ? rv_succeed : rv_fail;
    }

    // Let the OS breathe
    SDL_Delay(20);
  }

  net_logf("NET INFO: _cl_HostCallback() - Thread terminated successfully.\n");

  return rv_succeed;
}

//--------------------------------------------------------------------------------------------
//bool_t cl_dispatchPackets(Client_t * cs)
//{
//  ENetEvent event;
//  CListIn_Info_t cin_info, *pcin_info;
//  char hostName[64] = NULL_STRING;
//  PacketRequest_t * prequest;
//  size_t copy_size;
//
//  if(NULL ==cs || !cl_Started()) return bfalse;
//
//  while (enet_host_service(cs->host->Host, &event, 0) > 0)
//  {
//
//    net_logf("--------------------------------------------------\n");
//    net_logf("NET INFO: cl_dispatchPackets() - Received event... ");
//    if(!cl_Started())
//    {
//      net_logf("Ignored\n");
//      continue;
//    }
//    net_logf("Processing... ");
//
//    switch (event.type)
//    {
//    case ENET_EVENT_TYPE_RECEIVE:
//      net_logf("ENET_EVENT_TYPE_RECEIVE\n");
//
//      prequest = NetRequest_check(cs->host->asynch, NETHOST_REQ_COUNT, &event);
//      if( NULL != prequest && prequest->waiting )
//      {
//        // we have received a packet that someone is waiting for
//
//        net_logf("NET INFO: cl_dispatchPackets() - Received requested packet \"%s\" from %s:%04x\n", net_crack_enet_packet_type(event.packet), convert_host(event.peer->address.host), event.peer->address.port);
//
//        // copy the data from the packet to a waiting buffer
//        copy_size = MIN(event.packet->dataLength, prequest->data_size);
//        memcpy(prequest->data, event.packet->data, copy_size);
//        prequest->data_size = copy_size;
//
//        // tell the other thread that we're done
//        prequest->received = btrue;
//      }
//      else if(!net_handlePacket(cs->parent, &event))
//      {
//        net_logf("NET WARNING: cl_dispatchPackets() - Unhandled packet\n");
//      }
//
//      enet_packet_destroy(event.packet);
//      break;
//
//    case ENET_EVENT_TYPE_CONNECT:
//      net_logf("ENET_EVENT_TYPE_CONNECT\n");
//
//      cin_info.Address = event.peer->address;
//      cin_info.Hostname[0] = EOS;
//      cin_info.Name[0] = EOS;
//
//      // Look up the hostname the player is connecting from
//      //enet_address_get_host(&event.peer->address, hostName, 64);
//      //strncpy(ns->nfile.Hostname[cnt], hostName, sizeof(ns->nfile.Hostname[cnt]));
//
//      // see if we can add the 'player' to the connection list
//      pcin_info = CListIn_add( &(cs->host->cin), &cin_info );
//
//      if(NULL == pcin_info)
//      {
//        net_logf("NET WARN: cl_dispatchPackets() - Too many connections. Refusing connection from \"%s\" (%s:%04x)\n", hostName, convert_host(event.peer->address.host), event.peer->address.port);
//        net_stopPeer(event.peer);
//      }
//
//      // link the player info to the event.peer->data field
//      event.peer->data = &(pcin_info->Slot);
//
//      break;
//
//    case ENET_EVENT_TYPE_DISCONNECT:
//      net_logf("ENET_EVENT_TYPE_DISCONNECT\n");
//
//      if( !CListIn_remove( &(cs->host->cin), event.peer ) )
//      {
//        net_logf("NET INFO: cl_dispatchPackets() - Unknown connection closing from \"%s\" (%s:%04x)\n", hostName, convert_host(event.peer->address.host), event.peer->address.port);
//      }
//      break;
//
//    case ENET_EVENT_TYPE_NONE:
//      net_logf("ENET_EVENT_TYPE_NONE\n");
//      break;
//
//    default:
//      net_logf("UNKNOWN\n");
//      break;
//
//    }
//  }
//
//  // !!make sure that all of the packets get sent!!
//  if(cl_Started())
//  {
//    enet_host_flush(cs->host->Host);
//  }
//
//  return btrue;
//};

//--------------------------------------------------------------------------------------------
int cl_find_peer_index(Client_t * cs)
{
  int i = -1;

  if(NULL == cs) return i;

  for(i=0; i<MAXNETPLAYER; i++)
  {
    if(NULL == cs->rem_req_peer[i])
    {
      break;
    };
  }

  return i;
}


//--------------------------------------------------------------------------------------------
bool_t cl_begin_request_module(Client_t * cs)
{
  int i;

  if(NULL == cs) return bfalse;

  // start opening the connections
  for (i = 0; i < MAXNETPLAYER; i++)
  {
    cs->rem_req_image[i] = bfalse;
    cs->rem_req_info[i]  = bfalse;
    cs->rem_req_peer[i]  = NULL;

    if(EOS != CData.net_hosts[i][0])
    {
      cs->rem_req_peer[i] = cl_startPeer(CData.net_hosts[i]);
    }
  }

  return btrue;
}


//--------------------------------------------------------------------------------------------
bool_t cl_end_request_module(Client_t * cs)
{
  int i;

  if(NULL == cs) return bfalse;

  // start opening the connections
  for (i = 0; i < MAXNETPLAYER; i++)
  {
    if(NULL != cs->rem_req_peer[i])
    {
      net_stopPeer(cs->rem_req_peer[i] );
      cs->rem_req_peer[i] = NULL;
    }

    cs->rem_req_image[i] = bfalse;
    cs->rem_req_info[i]  = bfalse;
    cs->rem_req_peer[i]  = NULL;
  }

  return btrue;
}


//--------------------------------------------------------------------------------------------
void cl_request_module_info(Client_t * cs)
{
  /// @details BB@> begin the asynchronous transfer of hosted module info from each of the
  ///      potential hosts

  int i;
  SYS_PACKET egopkt;

  // request the module info when/if the connection opens
  for (i = 0;i < MAXNETPLAYER; i++)
  {
    if(NULL == cs->rem_req_peer[i]) continue;
    if(ENET_PEER_STATE_CONNECTED != cs->rem_req_peer[i]->state) continue;
    if(cs->rem_req_info[i]) continue;

    // send the request to the connected peer
    net_startNewSysPacket(&egopkt);
    sys_packet_addUint16(&egopkt, TO_HOST_MODULE);
    net_sendSysPacketToPeerGuaranteed(cs->rem_req_peer[i], &egopkt);
    cs->rem_req_info[i] = btrue;

    // set up the non-synchronous communication of the module info
    net_prepareWaitForPacket(cs->host->asynch, cs->rem_req_peer + i, 5000, TO_REMOTE_MODULEINFO);
  }

}




//--------------------------------------------------------------------------------------------
bool_t cl_load_module_info(Client_t * cs)
{
  /// @details BB@> finish downloading any module info from a given host

  STREAM stream;
  MOD_INFO * mi;
  PacketRequest_t * req;
  bool_t none_waiting;
  int i;

  none_waiting = btrue;
  for (i = 0; i < NETHOST_REQ_COUNT; i++)
  {
    req = &(cs->host->asynch[i].req);

    // ignore all the wront rype of packets
    if(req->packettype != TO_REMOTE_MODULEINFO) continue;

    // it it has not received a packet, move on
    if(!NetRequest_test( req )) { none_waiting = bfalse; continue; }

    mi = cs->rem_mod + cs->rem_mod_count;
    cs->rem_mod_count++;

    mi->is_hosted   = btrue;
    mi->is_verified = bfalse;

    // we have a response!!!
    stream_startRaw(&stream, (Uint8*)req->data, req->data_size);

    // make sure we have the right info
    assert(TO_REMOTE_MODULEINFO == stream_readUint16(&stream));

    // grab the data from the packet
    snprintf(mi->host, sizeof(STRING), "%s", convert_host(req->address.host) ); // the hex IP address of the host
    stream_readString(&stream, mi->rank, RANKSIZE);               // Number of stars
    stream_readString(&stream, mi->longname, MAXCAPNAMESIZE);     // Module names
    stream_readString(&stream, mi->loadname, MAXCAPNAMESIZE);     // Module load names
    mi->importamount = stream_readUint8(&stream);                 // # of import characters
    mi->allowexport  = (0 != stream_readUint8(&stream));          // Export characters?
    mi->minplayers   = stream_readUint8(&stream);                 // Number of players
    mi->maxplayers   = stream_readUint8(&stream);                 //
    mi->monstersonly = (0 != stream_readUint8(&stream));          // Only allow monsters
    mi->rts_control  = (0 != stream_readUint8(&stream));          // Real Time Stragedy?
    mi->respawnmode  = (RESPAWN_MODE)stream_readUint8(&stream);   // Allow respawn
    stream_done(&stream);

    NetRequest_release( req );
  }

  return none_waiting;
}

//--------------------------------------------------------------------------------------------
void cl_request_module_images(Client_t * cs)
{
  /// @details BB@> send a request to the server to download the module images
  ///      images will not be downloaded unless the local CRC does not match the
  ///      remote CRC (or the file does not exist locally)

  SYS_PACKET egopkt;
  STRING tmpstring;
  int i;

  // request the module info when/if the connection opens
  for (i = 0; i < MAXNETPLAYER; i++)
  {
    STRING fname_temp = NULL_STRING;

    if(NULL == cs->rem_req_peer[i]) continue;
    if(ENET_PEER_STATE_CONNECTED != cs->rem_req_peer[i]->state) continue;
    if(!cs->rem_mod[i].is_hosted) continue;
    if(cs->rem_req_image[i]) continue;

    // send the request to the connected peer

    net_startNewSysPacket(&egopkt);
    sys_packet_addUint16(&egopkt, NET_REQUEST_FILE);

    // build the local filename
    {
      strcpy(tmpstring, CData.modules_dir);
      str_append_slash(tmpstring, sizeof(STRING));
      strcat(tmpstring, cs->rem_mod[i].loadname);
      str_append_slash(tmpstring, sizeof(STRING));
      strcat(tmpstring, CData.basicdat_dir);
      str_append_slash(tmpstring, sizeof(STRING));
      strcat(tmpstring, CData.title_bitmap);
    }

    // add the local file name
    sys_packet_addString(&egopkt, tmpstring);

    // Send the directory and file info so that the names can be localized correctly
    // on the other machine

    // build the relative filename
    {
      strcat(fname_temp, "@modules_dir");
      str_append_slash(fname_temp, sizeof(STRING));

      strcat(fname_temp, cs->rem_mod[i].loadname);
      str_append_slash(fname_temp, sizeof(STRING));

      strcat(fname_temp, "@gamedat_dir");
      str_append_slash(fname_temp, sizeof(STRING));

      strcat(fname_temp, CData.title_bitmap);

      // convert all system slashes to network slashes
      str_convert_slash_net(fname_temp, sizeof(STRING));
    }

    sys_packet_addString(&egopkt, fname_temp);

    // terminate with an empty string
    sys_packet_addString(&egopkt, "");

    // send the packet
    net_sendSysPacketToPeerGuaranteed(cs->rem_req_peer[i], &egopkt);
    cs->rem_req_image[i] = btrue;
  }


}

//--------------------------------------------------------------------------------------------
bool_t Client_sendPacketToHost(Client_t * cs, SYS_PACKET * egop)
{
  /// @details ZZ@> This function sends a packet to the host

  net_logf("NET INFO: Client_sendPacketToHost()\n");

  if(NULL == cs || NULL == egop)
  {
    net_logf("NET ERROR: Client_sendPacketToHost() - Called with invalid parameters.\n");
    return bfalse;
  }

  if(!cl_Started())
  {
    net_logf("NET WARN: Client_sendPacketToHost() - Client is not active.\n");
    return bfalse;
  }

  return net_sendSysPacketToPeer(cs->gamePeer, egop);
}

//--------------------------------------------------------------------------------------------
bool_t Client_sendPacketToHostGuaranteed(Client_t * cs, SYS_PACKET * egop)
{
  /// @details ZZ@> This function sends a packet to the host

  net_logf("NET INFO: Client_sendPacketToHostGuaranteed()\n");

  if(NULL == cs || NULL == egop)
  {
    net_logf("NET ERROR: Client_sendPacketToHostGuaranteed() - Called with invalid parameters.\n");
    return bfalse;
  }

  if(!cl_Started())
  {
    net_logf("NET WARN: Client_sendPacketToHostGuaranteed() - Client is not active.\n");
    return bfalse;
  }

  return net_sendSysPacketToPeerGuaranteed(cs->gamePeer, egop);
}

//--------------------------------------------------------------------------------------------
size_t StatList_add( Status_t lst[], size_t lst_size, CHR_REF ichr )
{
  if ( lst_size < MAXSTAT )
  {
    lst[lst_size].chr_ref = ichr;
    lst[lst_size].on      = btrue;

    lst_size++;
  }

  return lst_size;
}

//--------------------------------------------------------------------------------------------
void StatList_move_to_top( Status_t lst[], size_t lst_size, CHR_REF character )
{
  /// @details ZZ@> This function puts the character on top of the lst

  size_t cnt, oldloc;

  // Find where it is
  oldloc = lst_size;

  for ( cnt = 0; cnt < lst_size; cnt++ )
  {
    if ( lst[cnt].chr_ref == character )
    {
      oldloc = cnt;
      cnt = lst_size;
      break;
    }
  }

  // Change position
  if ( oldloc < lst_size )
  {
    // Move all the lower ones up
    while ( oldloc > 0 )
    {
      oldloc--;
      memcpy(lst + (oldloc+1),  lst + oldloc, sizeof(Status_t));
    }
    // Put the character in the top slot
    lst[0].chr_ref = character;
  }
}



//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Status_t * Status_new( Status_t * pstat )
{
  //fprintf( stdout, "Status_new()\n");

  if(NULL == pstat) return pstat;

  Status_delete( pstat );

  memset(pstat, 0, sizeof(Status_t));

  EKEY_PNEW( pstat, Status_t );

  pstat->on      = bfalse;
  pstat->chr_ref = INVALID_CHR;
  pstat->delay   = 10;

  return pstat;
}

//--------------------------------------------------------------------------------------------
bool_t   Status_delete( Status_t * pstat )
{
  if(NULL == pstat) return bfalse;

  if(!EKEY_PVALID(pstat))  return btrue;

  EKEY_PINVALIDATE( pstat );

  pstat->on = bfalse;

  return btrue;
}

//--------------------------------------------------------------------------------------------
Status_t * Status_renew( Status_t * pstat )
{
  Status_delete(pstat);
  return Status_new(pstat);
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t StatList_new( Client_t * cs )
{
  int i;

  if(!EKEY_PVALID(cs)) return bfalse;

  cs->StatList_count = 0;
  for(i=0; i<MAXSTAT; i++)
  {
    Status_new(cs->StatList + i);
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t StatList_delete( Client_t * cs )
{
  int i;

  if(!EKEY_PVALID(cs)) return bfalse;

  cs->StatList_count = 0;
  for(i=0; i<MAXSTAT; i++)
  {
    Status_delete(cs->StatList + i);
  }

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t StatList_renew( Client_t * cs )
{
  int i;

  if(!EKEY_PVALID(cs)) return bfalse;

  cs->StatList_count = 0;
  for(i=0; i<MAXSTAT; i++)
  {
    Status_renew(cs->StatList + i);
  }

  return btrue;
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// cl module NetHost_t singleton management
NetHost_t * cl_getHost()
{
  // make sure the host exists
  if(NULL == _cl_host)
  {
    _cl_Initialize();
  }

  return _cl_host;
}

//------------------------------------------------------------------------------
retval_t _cl_Initialize()
{
  if( NULL != _cl_host) return rv_succeed;

  _cl_host = NetHost_create( _cl_HostCallback );
  if(NULL == _cl_host) return rv_fail;

  if(!_cl_atexit_registered)
  {
    atexit(_cl_Quit);
  }

  return _cl_startUp();
}

//------------------------------------------------------------------------------
void cl_quitHost()
{
  NetHost_shutDown(_cl_host);
  NetHost_destroy( &_cl_host );
}

//------------------------------------------------------------------------------
void _cl_Quit( void )
{
  if( !_cl_atexit_registered ) return;
  if( NULL == _cl_host ) return;

  _cl_shutDown();

  NetHost_destroy( &_cl_host );
}

//------------------------------------------------------------------------------
retval_t _cl_startUp( void )
{
  NetHost_t * nh = cl_getHost();
  if(NULL == nh) return rv_fail;

  return NetHost_startUp(nh, NET_EGOBOO_CLIENT_PORT);
}

//------------------------------------------------------------------------------
retval_t _cl_shutDown( void )
{
  if(NULL == _cl_host) return rv_fail;

  return NetHost_shutDown(_cl_host);
}

//------------------------------------------------------------------------------
bool_t cl_Started()  { return (NULL != _cl_host) && _cl_host->nthread.Active; }


//------------------------------------------------------------------------------
bool_t cl_handle_chr_spawn(Client_t * cs, ENetEvent *event)
{
  STREAM stream;
  CHR_SPAWN_INFO si;
  NET_PACKET netpkt;
  Uint16 ui16;

  if(NULL == cs || !cl_Running(cs)) return bfalse;

  // send the setup data
  stream_startENet(&stream, event->packet);

  ui16 = net_packet_readUint16(&netpkt);
  assert(TO_REMOTE_CHR_SPAWN == ui16);

  chr_spawn_info_new( &si, cs->parent );

  si.seed   = stream_readUint32(&stream);
  si.net_id = stream_readUint32(&stream);
  si.ichr   = stream_readUint32(&stream);

  // convert 16.16 to pos fixed point
  si.pos.x = stream_readSint32(&stream) / (float)(1<<16);
  si.pos.y = stream_readSint32(&stream) / (float)(1<<16);
  si.pos.z = stream_readSint32(&stream) / (float)(1<<16);

  // convert 16.16 to vel fixed point
  si.vel.x = stream_readSint32(&stream) / (float)(1<<16);
  si.vel.y = stream_readSint32(&stream) / (float)(1<<16);
  si.vel.z = stream_readSint32(&stream) / (float)(1<<16);

  si.iobj   = stream_readUint32(&stream);
  si.iteam  = stream_readUint32(&stream);
  si.iskin  = stream_readUint8 (&stream);
  si.facing = stream_readUint16(&stream);
  si.slot   = stream_readUint8 (&stream);
  stream_readString(&stream, si.name, sizeof(si.name));
  si.ioverride = stream_readUint32(&stream);

  si.money   = stream_readSint32(&stream);
  si.content = stream_readSint32(&stream);
  si.passage = stream_readUint32(&stream);
  si.level   = stream_readSint32(&stream);
  si.stat    = stream_readSint8 (&stream);
  si.ghost   = stream_readSint8 (&stream);

  // add this info to the queue
  chr_spawn_queue_push( &(cs->chr_queue), &si );

  return btrue;

}
