//********************************************************************************************
//* Egoboo - Server.c
//*
//* Implements the game server for a network game
//*
//* Even if networking is not enabled, the local latches are routed through this code.
//*
//* This code is currently under development.
//*
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

#include "Server.h"

#include "Log.h"
#include "egoboo.h"
#include "NetFile.h"
#include "game.h"

#include "Network.inl"
#include "egoboo_types.inl"

#include <enet/enet.h>
#include <assert.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// the sv module info

static bool_t    _sv_atexit_registered = bfalse;
static NetHost_t * _sv_host = NULL;

static retval_t  _sv_Initialize(void);
static void      _sv_Quit(void);
static retval_t  _sv_startUp(void);
static retval_t  _sv_shutDown(void);
static int       _sv_HostCallback(void *);

//--------------------------------------------------------------------------------------------
bool_t sv_Running(Server_t * ss)  { return sv_Started() &&  ss->ready; }

//--------------------------------------------------------------------------------------------
// Server state private initialization
static Server_t * CServer_new(Server_t * ss, Game_t * gs);
static bool_t    CServer_delete(Server_t * ss);


//--------------------------------------------------------------------------------------------
//retval_t sv_startUp(Server_t * ss)
//{
//  retval_t host_return;
//
//  if(sv_initialized) return rv_succeed;
//
//  if(NULL == ss)
//  {
//    ss = &ACServer;
//  }
//
//  // Try to create a new session
//  host_return = NetHost_startUp(&(ss->host), NET_EGOBOO_SERVER_PORT);
//
//  // Moved from net_sayHello because there they cause a race issue
//  ss->num_loaded = 0;
//
//  if(!sv_atexit_registered)
//  {
//    atexit(sv_Quit);
//    sv_atexit_registered = btrue;
//  }
//
//  sv_initialized = btrue;
//
//  return host_return;
//}
//
////--------------------------------------------------------------------------------------------
//
//retval_t sv_shutDown(Server_t * ss)
//{
//  retval_t delete_return;
//
//  if(!sv_initialized) return rv_succeed;
//
//  if(NULL == ss)
//  {
//    ss = &ACServer;
//  }
//
//  delete_return = CServer_delete(ss) ? rv_succeed : rv_fail;
//
//  sv_initialized = bfalse;
//
//  return delete_return;
//}

//--------------------------------------------------------------------------------------------
retval_t CServer_startUp(Server_t * ss)
{
  // ZZ> This function tries to host a new session

  NetHost_t * sv_host;

  // trap bad server states
  if(NULL == ss || NULL == ss->parent) return rv_error;

  if (!net_Started())
  {
    CServer_shutDown(ss);
    return rv_error;
  }

  // return btrue if the server is already started
  sv_host = sv_getHost();
  if(NULL == sv_host) return rv_error;

  // ensure that the server module has been turned on
  if(!sv_Started())
  {
    net_logf("---------------------------------------------\n");
    net_logf("NET INFO: CServer_startUp() - Starting game server\n");

    _sv_startUp();
  }
  else
  {
    net_logf("---------------------------------------------\n");
    net_logf("NET INFO: CServer_startUp() - Restarting game server\n");
  }

  // To function, the client requires the file transfer component
  NFileState_startUp( ss->parent->ns->nfs );

  return sv_Started() ? rv_succeed : rv_fail;
}

////--------------------------------------------------------------------------------------------
retval_t CServer_shutDown(Server_t * ss)
{
  if(NULL == ss) return rv_error;

  if(!sv_Started()) return rv_succeed;

  net_logf("NET INFO: CServer_shutDown() - Shutting down the game server... ");

  // close all open connections to this state
  NetHost_close( sv_getHost(), ss );

  net_logf("Done.\n");

  return rv_succeed;
};

//--------------------------------------------------------------------------------------------
//void sv_Quit(void)
//{
//  CServer_delete(&ACServer);
//}
//
//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Server_t * CServer_create(Game_t * gs)
{
  Server_t * ss = (Server_t *)calloc(1, sizeof(Server_t));
  return CServer_new(ss, gs);
}

//--------------------------------------------------------------------------------------------
bool_t CServer_destroy(Server_t ** pss)
{
  bool_t retval;

  if(NULL == pss || NULL == *pss) return bfalse;
  if( !EKEY_PVALID((*pss)) ) return btrue;

  retval = CServer_delete(*pss);
  FREE( *pss );

  return retval;
}

//--------------------------------------------------------------------------------------------
Server_t * CServer_new(Server_t * ss, Game_t * gs)
{
  int cnt;

  //fprintf( stdout, "CServer_new()\n");

  if(NULL == ss) return ss;
  CServer_delete(ss);

  memset( ss, 0, sizeof(Server_t) );

  EKEY_PNEW(ss, Server_t);

  // only start this stuff if we are going to actually connect to the network
  if(net_Started())
  {
    ss->host     = sv_getHost();
    ss->net_guid = net_addService(ss->host, ss);
    ss->net_guid = NetHost_register(ss->host, net_handlePacket, ss, ss->net_guid);
  }

  ss->parent         = gs;
  ss->ready          = bfalse;
  ss->selectedModule = -1;
  ss->num_logon      = 0;

  for(cnt=0; cnt<MAXMODULE; cnt++)
  {
    ModInfo_new(ss->loc_mod + cnt);
  }

  CServer_reset_latches(ss);

  return ss;
}

//--------------------------------------------------------------------------------------------
bool_t CServer_delete(Server_t * ss)
{
  if(NULL == ss) return bfalse;
  if(!EKEY_PVALID(ss)) return btrue;

  EKEY_PINVALIDATE(ss);

  net_removeService(ss->net_guid);
  NetHost_unregister(ss->host, ss->net_guid);

  return btrue;
}

//--------------------------------------------------------------------------------------------
Server_t * CServer_renew(Server_t * ss)
{
  Game_t * gs;

  if(NULL == ss) return ss;

  gs = ss->parent;

  CServer_delete(ss);
  return CServer_new(ss, gs);
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void sv_frameStep(Server_t * ss)
{

}

//--------------------------------------------------------------------------------------------
void CServer_bufferLatches(Server_t * ss)
{
  // ZZ> This function buffers the character latches
  Game_t * gs;
  Uint32 uiTime, ichr;

  if (NULL ==ss || !sv_Started()) return;

  gs = ss->parent;

  if (gs->wld_frame > STARTTALK)
  {
    uiTime = gs->wld_frame + CData.lag;

    // Now pretend the host got the packet...
    uiTime = uiTime & LAGAND;
    for (ichr = 0; ichr < CHRLST_COUNT; ichr++)
    {
      ss->tlb.buffer[ichr][uiTime].latch.b = ss->latch[ichr].b;
      ss->tlb.buffer[ichr][uiTime].latch.x = ((Sint32)(ss->latch[ichr].x*SHORTLATCH)) / SHORTLATCH;
      ss->tlb.buffer[ichr][uiTime].latch.y = ((Sint32)(ss->latch[ichr].y*SHORTLATCH)) / SHORTLATCH;
    }
    ss->tlb.numtimes++;
  };

};

//--------------------------------------------------------------------------------------------
void sv_talkToRemotes(Server_t * ss)
{
  // ZZ> This function sends the character data to all the remote machines
  Uint32 uiTime;
  CHR_REF ichr;
  SYS_PACKET egopkt;
  int i,cnt,stamp;

  Game_t * gs = ss->parent;

  if(!sv_Started() || !net_Started()) return;

  if (gs->wld_frame > STARTTALK)
  {
    // check all possible lags
    for(i=0; i<MAXLAG; i++)
    {
      stamp  = gs->wld_frame - i;
      if(stamp<0) continue;

      uiTime = stamp & LAGAND;

      // Send a message to all players
      net_startNewSysPacket(&egopkt);
      sys_packet_addUint16(&egopkt, TO_REMOTE_LATCH);                       // The message header
      sys_packet_addUint32(&egopkt, stamp);

      // test all player latches...
      cnt = 0;
      for (ichr=0; ichr<CHRLST_COUNT; ichr++)
      {
        // do not look at characters that aren't on
        if ( !ACTIVE_CHR(gs->ChrList, ichr) ) continue;

        // do not look at invalid latches
        if(!ss->tlb.buffer[REF_TO_INT(ichr)][uiTime].valid) continue;

        // do not send "future" latches
        if(ss->tlb.buffer[REF_TO_INT(ichr)][uiTime].stamp <= uiTime)
        {
          sys_packet_addUint16(&egopkt, REF_TO_INT(ichr));                                // The character index
          sys_packet_addUint8(&egopkt, ss->tlb.buffer[REF_TO_INT(ichr)][uiTime].latch.b);         // Player_t button states
          sys_packet_addSint16(&egopkt, ss->tlb.buffer[REF_TO_INT(ichr)][uiTime].latch.x*SHORTLATCH);    // Player_t motion
          sys_packet_addSint16(&egopkt, ss->tlb.buffer[REF_TO_INT(ichr)][uiTime].latch.y*SHORTLATCH);    // Player_t motion

          ss->tlb.buffer[REF_TO_INT(ichr)][uiTime].valid = bfalse;

          cnt++;
        }
      };

      // Send the packet
      if(cnt>0)
      {
        sv_sendPacketToAllClients(ss, &egopkt);
      };
    }


  }
}

//--------------------------------------------------------------------------------------------
//void sv_letPlayersJoin(Server_t * ss)
//{
//  // ZZ> This function finds all the players in the game
//  ENetEvent event;
//  char hostName[64];
//
//  // Check all reserved events for players joining
//  while (enet_host_service(sv_host->Host, &event, 0) > 0)
//  {
//    switch (event.type)
//    {
//    case ENET_EVENT_TYPE_CONNECT:
//      // Look up the hostname the player is connecting from
//      enet_address_get_host(&event.peer->address, hostName, 64);
//
//      if(sv_host->Connection_count >= MAXNETPLAYER)
//      {
//        net_logf("NET INFO: sv_letPlayersJoin: Too many connections. Connection from %s:%u refused\n", hostName, event.peer->address.port);
//        enet_peer_disconnect_now(event.peer);
//      }
//      else
//      {
//        net_logf("NET INFO: sv_letPlayersJoin: A new player connected from %s:%u\n", hostName, event.peer->address.port);
//
//        for(cnt=0; cnt<MAXNETPLAYER; cnt++)
//        {
//          if( EMPTY_CSTR(sv_host->Hostname[cnt]) )
//          {
//            // found an empty connection
//            sv_host->Connection_count++;
//            strncpy(sv_host->Hostname[cnt], hostName, sizeof(ss->playeraddress))
//            event.peer->data = (sv_host->Connections + cnt);
//            break;
//          }
//        }
//      }
//      break;
//
//    case ENET_EVENT_TYPE_RECEIVE:
//      net_logf("NET INFO: sv_letPlayersJoin: Recieved a packet when we weren't expecting it...\n");
//      net_logf("NET INFO: \tIt came from %x:%u\n", event.peer->address.host, event.peer->address.port);
//
//      // clean up the packet
//      enet_packet_destroy(event.packet);
//      break;
//
//    case ENET_EVENT_TYPE_DISCONNECT:
//      enet_address_get_host(&event.peer->address, hostName, 64);
//
//      if(sv_host->Connection_count > 0)
//      {
//        net_logf("NET INFO: sv_letPlayersJoin: A new player is disconnecting from %s:%u\n", hostName, event.peer->address.port);
//
//        for(cnt=0; cnt<MAXNETPLAYER; cnt++)
//        {
//          if(0 == strncmp(sv_host->Hostname[cnt][0], hostName, sizeof(ss->playeraddress[cnt])) )
//          {
//            // found the connection
//            sv_host->Connection_count--;
//            sv_host->Hostname[cnt][0] = EOS;
//            break;
//          }
//        }
//      }
//
//      // Reset that peer's data
//      event.peer->data = NULL;
//    }
//  }
//}



//--------------------------------------------------------------------------------------------
void sv_sendModuleInfoToAllPlayers(Server_t * ss)
{
  SYS_PACKET egopkt;

  // tell the which module we are hosting
  net_startNewSysPacket(&egopkt);
  sys_packet_addUint16(&egopkt, TO_REMOTE_MODULE);
  sys_packet_addUint32(&egopkt, ss->modstate.seed);
  sys_packet_addString(&egopkt, ss->mod.loadname);

  sv_sendPacketToAllClientsGuaranteed(ss, &egopkt);
};

//--------------------------------------------------------------------------------------------
void sv_sendModuleInfoToOnePlayer(Server_t * ss, ENetPeer * peer)
{
  SYS_PACKET egopkt;

  // tell the which module we are hosting
  net_startNewSysPacket(&egopkt);
  sys_packet_addUint16(&egopkt, TO_REMOTE_MODULE);
  sys_packet_addUint32(&egopkt, ss->modstate.seed);
  sys_packet_addString(&egopkt, ss->mod.loadname);

  net_sendSysPacketToPeerGuaranteed(peer, &egopkt);
};


//--------------------------------------------------------------------------------------------
bool_t sv_handlePacket(Server_t * ss, ENetEvent *event)
{
  Uint16 header;
  STRING filename;   // also used for reading various strings
  int filesize, newfilesize, fileposition;
  char newfile;
  Uint16 character;
  FILE *file;
  SYS_PACKET egopkt;
  NET_PACKET netpkt;
  Uint32 stamp, time;

  bool_t retval = bfalse;

  NetHost_t * sv_host = sv_getHost();

  // do some error trapping
  //if(!sv_Started()) return bfalse;

  // send some log info
  net_logf("NET INFO: sv_handlePacket: Processing... ");

  // rewind the packet
  net_packet_startReading(&netpkt, event->packet);
  header = net_packet_readUint16(&netpkt);

  // process out messages
  retval = btrue;
  switch (header)
  {

  case TO_HOST_LOGON:
    {
      bool_t found;
      int cnt, tnc;
      CListIn_t * cli;

      net_logf("TO_HOST_LOGON\n");


      cli = &(sv_host->cin);

      // read the logon name
      packet_readString(&netpkt, filename, 255);
      found = bfalse;
      for(cnt=0, tnc=0; cnt<MAXCONNECTION && tnc<ss->num_logon; cnt++)
      {
        if(EOS != cli->Info[cnt].Name[0]) tnc++;

        if(0==strncmp(cli->Info[cnt].Name, filename, sizeof(cli->Info[cnt].Name)))
        {
          found = btrue;
          break;
        }
      }

      net_startNewSysPacket(&egopkt);
      sys_packet_addUint16(&egopkt, TO_REMOTE_LOGON);
      if(found)
      {
        sys_packet_addUint8(&egopkt, bfalse);       // deny the logon
        sys_packet_addUint8(&egopkt, (Uint8)-1 );
      }
      else
      {
        sys_packet_addUint8(&egopkt, btrue);            // accept the logon
        sys_packet_addUint8(&egopkt, ss->num_logon);   // tell the client their place in the queue
      }
      net_sendSysPacketToPeerGuaranteed(event->peer, &egopkt);

      if(!found)
      {
        // add the login name to the list
        strncpy(cli->Info[cnt].Name, filename, sizeof(cli->Info[cnt].Name));
        ss->num_logon++;
      }
    }
    break;

  case TO_HOST_LOGOFF:
    {
      bool_t found;
      Uint8  index;
      CListIn_t * cli;

      net_logf("TO_HOST_LOGOFF\n");

      cli = &(sv_host->cin);

      // read the logon name
      packet_readString(&netpkt, filename, 255);
      index = net_packet_readUint8(&netpkt);

      found = bfalse;
      if( index<MAXCONNECTION && ss->num_logon>0 &&
          0==strncmp(cli->Info[index].Name, filename, sizeof(cli->Info[index].Name)) )
      {
        cli->Info[index].Name[0] = EOS;
        ss->num_logon--;
      };

      if(found)
      {
        net_startNewSysPacket(&egopkt);
        sys_packet_addUint16(&egopkt, TO_REMOTE_LOGOFF);
        sys_packet_addUint8(&egopkt, index);
        net_sendSysPacketToPeerGuaranteed(event->peer, &egopkt);
      }
    }
    break;

  case TO_HOST_MODULE:
    if(-1 == ss->selectedModule)
    {
      // tell the client that no module has been selected
      net_logf("TO_HOST_MODULE - module invalid\n");

      net_startNewSysPacket(&egopkt);
      sys_packet_addUint16(&egopkt, TO_REMOTE_MODULEBAD);
      net_sendSysPacketToPeerGuaranteed(event->peer, &egopkt);
    }
    else
    {
      // give the client the module info
      net_logf("TO_HOST_MODULE - module \"%s\" == \"%s\"\n", ss->mod.loadname, ss->mod.longname);

      net_startNewSysPacket(&egopkt);
      sys_packet_addUint16(&egopkt, TO_REMOTE_MODULEINFO);
      sys_packet_addString(&egopkt, ss->mod.rank);
      sys_packet_addString(&egopkt, ss->mod.longname);
      sys_packet_addString(&egopkt, ss->mod.loadname);
      sys_packet_addUint8(&egopkt, ss->mod.importamount);
      sys_packet_addUint8(&egopkt, ss->mod.allowexport);
      sys_packet_addUint8(&egopkt, ss->mod.minplayers);
      sys_packet_addUint8(&egopkt, ss->mod.maxplayers);
      sys_packet_addUint8(&egopkt, ss->mod.monstersonly);
      sys_packet_addUint8(&egopkt, ss->mod.minplayers);
      sys_packet_addUint8(&egopkt, ss->mod.rts_control);
      sys_packet_addUint8(&egopkt, ss->mod.respawnmode);
      net_sendSysPacketToPeerGuaranteed(event->peer, &egopkt);
    };
    break;


  case TO_HOST_MODULEOK:
    net_logf("TO_HOST_MODULEOK\n");

    // the client is acknowledging that it actually has the module the server is hosting
    break;

  case TO_HOST_MODULEBAD:
    net_logf("TO_HOST_MODULEBAD\n");

    // copy the selected module from the server to a client that doesn't have the module
    if(-1 == ss->selectedModule)
    {
      // send the directory
      net_copyDirectoryToPeer(ss->parent->ns, event->peer, ss->mod.loadname, ss->mod.loadname);

      // tell it to try to find the module again
      sv_sendModuleInfoToOnePlayer(ss, event->peer);
    }
    break;

  case TO_HOST_LATCH:
    net_logf("TO_HOST_LATCH\n");

    stamp = net_packet_readUint32(&netpkt);
    time  = stamp & LAGAND;

    // the client is sending a latch
    while (net_packet_remainingSize(&netpkt) > 0)
    {
      character = net_packet_readUint16(&netpkt);
      ss->tlb.buffer[character][time].stamp  = stamp;
      ss->tlb.buffer[character][time].valid  = btrue;
      ss->tlb.buffer[character][time].latch.b = net_packet_readUint8(&netpkt);
      ss->tlb.buffer[character][time].latch.x      = (float)net_packet_readSint16(&netpkt) / (float)SHORTLATCH;
      ss->tlb.buffer[character][time].latch.y      = (float)net_packet_readSint16(&netpkt) / (float)SHORTLATCH;
    }
    break;

  case TO_HOST_IM_LOADED:
    net_logf("TO_HOST_IMLOADED\n");

    // the client has finished loading the module and is waiting for the other players and for the server
    // excessive connection requests have already been handled at this point

    ss->num_loaded++;
    if (ss->num_loaded >= ss->mod.minplayers && ss->num_loaded <= ss->mod.maxplayers)
    {
      // Let the games begin...
      ss->ready = btrue;
      net_startNewSysPacket(&egopkt);
      sys_packet_addUint16(&egopkt, TO_REMOTE_START);
      sv_sendPacketToAllClientsGuaranteed(ss, &egopkt);
    }
    break;

  case TO_HOST_RTS:
    net_logf("TO_HOST_RTS\n");

    // RTS no longer really supported

    /*whichorder = get_empty_order();
    if(whichorder < MAXORDER)
    {
    // Add the order on the host machine
    cnt = 0;
    while(cnt < MAXSELECT)
    {
    who = net_packet_readUint8();
    GOrder.who[whichorder][cnt] = who;
    cnt++;
    }
    what = net_packet_readUint32();
    when = gs->wld_frame + CData.GOrder.lag;
    GOrder.what[whichorder] = what;
    GOrder.when[whichorder] = when;


    // Send the order off to everyone else
    net_startNewSysPacket(&egopkt);
    sys_packet_addUint16(TO_REMOTE_RTS);
    cnt = 0;
    while(cnt < MAXSELECT)
    {
    sys_packet_addUint8(GOrder.who[whichorder][cnt]);
    cnt++;
    }
    sys_packet_addUint32(what);
    sys_packet_addUint32(when);
    sv_sendPacketToAllClientsGuaranteed();
    }*/
    break;

  case TO_HOST_FILE:
    net_logf("TO_HOST_FILE\n");

    // the client is sending us a file

    packet_readString(&netpkt, filename, 255);
    newfilesize = net_packet_readUint32(&netpkt);

    // Change the size of the file if need be
    newfile = 0;
    file = fopen(filename, "rb");
    if (file)
    {
      fseek(file, 0, SEEK_END);
      filesize = ftell(file);
      fclose(file);

      if (filesize != newfilesize)
      {
        // Destroy the old file
        newfile = 1;
      }
    }
    else
    {
      newfile = 1;
    }

    if (newfile)
    {
      // file must be created.  Write zeroes to the file to do it
      //numfile++;
      file = fopen(filename, "wb");
      if (file)
      {
        filesize = 0;
        while (filesize < newfilesize)
        {
          fputc(0, file);
          filesize++;
        }
        fclose(file);
      }
    }

    // Go to the position in the file and copy data
    fileposition = net_packet_readUint32(&netpkt);
    file = fopen(filename, "r+b");

    if (fseek(file, fileposition, SEEK_SET) == 0)
    {
      while (net_packet_remainingSize(&netpkt) > 0)
      {
        fputc(net_packet_readUint8(&netpkt), file);
      }
    }
    fclose(file);
    break;

  case TO_HOST_DIR:
    net_logf("TO_HOST_DIR\n");

    // the client is creating a dir on the server

    packet_readString(&netpkt, filename, 255);
    fs_createDirectory(filename);

    break;

  case TO_HOST_FILESENT:
    net_logf("TO_HOST_FILESENT\n");

    // the client is telling us to expect a certain number of files

    //numfileexpected += net_packet_readUint32(&netpkt);
    //numplayerrespond++;
    break;

  default:
    net_logf("UNKNOWN\n");
    retval = bfalse;
    break;
  }

  return retval;
};

//--------------------------------------------------------------------------------------------
void CServer_unbufferLatches(Server_t * ss)
{
  // ZZ> This function sets character latches based on player input to the host
  CHR_REF chr_cnt;
  Uint32  uiTime;
  Game_t * gs = ss->parent;

  if(!sv_Started()) return;

  // Copy the latches
  uiTime = gs->wld_frame & LAGAND;
  for (chr_cnt = 0; chr_cnt < CHRLST_COUNT; chr_cnt++)
  {
    if( !ACTIVE_CHR(gs->ChrList, chr_cnt) ) continue;
    if(!ss->tlb.buffer[REF_TO_INT(chr_cnt)][uiTime].valid) continue;
    if(INVALID_TIMESTAMP == ss->tlb.buffer[REF_TO_INT(chr_cnt)][uiTime].stamp) continue;

    ss->latch[REF_TO_INT(chr_cnt)].x = ss->tlb.buffer[REF_TO_INT(chr_cnt)][uiTime].latch.x;
    ss->latch[REF_TO_INT(chr_cnt)].y = ss->tlb.buffer[REF_TO_INT(chr_cnt)][uiTime].latch.y;
    ss->latch[REF_TO_INT(chr_cnt)].b = ss->tlb.buffer[REF_TO_INT(chr_cnt)][uiTime].latch.b;

    // Let players respawn
    if (gs->modstate.respawnvalid && (gs->ChrList[chr_cnt].aistate.latch.b & LATCHBUTTON_RESPAWN))
    {
      if (!gs->ChrList[chr_cnt].alive)
      {
        respawn_character(gs, chr_cnt);
        gs->TeamList[gs->ChrList[chr_cnt].team].leader = chr_cnt;
        gs->ChrList[chr_cnt].aistate.alert |= ALERT_CLEANEDUP;

        // Cost some experience for doing this...  Never lose a level
        gs->ChrList[chr_cnt].experience = gs->ChrList[chr_cnt].experience * EXPKEEP;
      }
      ss->latch[REF_TO_INT(chr_cnt)].b &= ~LATCHBUTTON_RESPAWN;
    }

  }
  ss->tlb.numtimes--;
}

//--------------------------------------------------------------------------------------------
void CServer_reset_latches(Server_t * ss)
{
  CHR_REF chr_cnt;
  if(NULL ==ss) return;

  for(chr_cnt = 0; chr_cnt<CHRLST_COUNT; chr_cnt++)
  {
    CServer_resetTimeLatches(ss,chr_cnt);
  };

  ss->tlb.nextstamp = INVALID_TIMESTAMP;
  ss->tlb.numtimes   = STARTTALK + 1;
};

//--------------------------------------------------------------------------------------------
void CServer_resetTimeLatches(Server_t * ss, CHR_REF ichr)
{
  int cnt, chr_val;

  if(NULL ==ss) return;

  chr_val = REF_TO_INT(ichr);

  CLatch_clear( ss->latch + chr_val );

  for(cnt=0; cnt < MAXLAG; cnt++)
  {
    ss->tlb.buffer[chr_val][cnt].valid   = bfalse;
    ss->tlb.buffer[chr_val][cnt].stamp   = INVALID_TIMESTAMP;

    CLatch_clear( &(ss->tlb.buffer[chr_val][cnt].latch) );
  }
};

//--------------------------------------------------------------------------------------------
bool_t sv_unhostGame(Server_t * ss)
{
  SYS_PACKET egopkt;
  NetHost_t * sv_host;

  if(NULL == ss || !sv_Started()) return bfalse;

  sv_host = sv_getHost();

  // send logoff messages to all the clients
  if(ss->ready)
  {
    net_logf("NET INFO: sv_unhostGame() - Telling all clients they are getting booted.\n");
    net_startNewSysPacket(&egopkt);
    sys_packet_addUint16(&egopkt, TO_REMOTE_LOGOFF);
    sv_sendPacketToAllClientsGuaranteed(ss, &egopkt);

    ss->ready = bfalse;
  };

  // close all outgoing connections
  if(sv_host->cin.Count > 0 || sv_host->cout.Count > 0 )
  {
    net_logf("NET INFO: sv_unhostGame() - Closing all open connections to this server.\n");
    NetHost_close( ss->host, ss );
  };

  return btrue;
};

//--------------------------------------------------------------------------------------------
int _sv_HostCallback(void * data)
{
  NetHost_t     * sv_host;
  NetThread_t   * nthread;
  retval_t      dispatch_return;

  sv_host = (NetHost_t *)data;

  // fail to start
  if(NULL == sv_host)
  {
    net_logf("NET ERROR: _sv_HostCallback() - Cannot start thread.\n");
    return rv_error;
  }

  net_logf("NET INFO: _sv_HostCallback() thread - starting normally.\n");

  sv_host = sv_getHost();
  nthread = &(sv_host->nthread);
  while(NULL !=sv_host && !nthread->KillMe)
  {
    // the server host could be removed/deleted at any time
    sv_host = sv_getHost();
    nthread = &(sv_host->nthread);

    if(!nthread->Paused)
    {
      dispatch_return = NetHost_dispatch(sv_host) ? rv_succeed : rv_fail;
    }

    // Let the OS breathe
    SDL_Delay(20);
  }

  net_logf("NET INFO: _sv_HostCallback() - Thread terminated successfully.\n");

  return rv_succeed;
}


//--------------------------------------------------------------------------------------------
//bool_t sv_dispatchPackets(Server_t * ss)
//{
//  ENetEvent event;
//  CListIn_Info_t cin_info, *pcin_info;
//  char hostName[64] = NULL_STRING;
//  PacketRequest_t * prequest;
//  size_t copy_size;
//  NetHost_t * sv_host;
//
//  if(NULL ==ss || !sv_Started(ss)) return bfalse;
//
//  sv_host = sv_getHost();
//
//  while (enet_host_service(sv_host->Host, &event, 0) > 0)
//  {
//    net_logf("--------------------------------------------------\n");
//    net_logf("NET INFO: sv_dispatchPackets() - Received event... ");
//    if(!sv_Started())
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
//      prequest = NetRequest_check(sv_host->asynch, NETHOST_REQ_COUNT, &event);
//      if( NULL != prequest && prequest->waiting )
//      {
//        // we have received a packet that someone is waiting for
//
//        net_logf("NET INFO: sv_dispatchPackets() - Received requested packet \"%s\" from %s:%04x\n", net_crack_enet_packet_type(event.packet), convert_host(event.peer->address.host), event.peer->address.port);
//
//        // copy the data from the packet to a waiting buffer
//        copy_size = MIN(event.packet->dataLength, prequest->data_size);
//        memcpy(prequest->data, event.packet->data, copy_size);
//        prequest->data_size = copy_size;
//
//        // tell the other thread that we're done
//        prequest->received = btrue;
//      }
//      else if(!net_handlePacket(ss->parent, &event))
//      {
//        net_logf("NET WARNING: sv_dispatchPackets() - Unhandled packet\n");
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
//      pcin_info = CListIn_add( &(sv_host->cin), &cin_info );
//
//      if(NULL == pcin_info)
//      {
//        net_logf("NET WARN: sv_dispatchPackets() - Too many connections. Refusing connection from \"%s\" (%s:%04x)\n", hostName, convert_host(event.peer->address.host), event.peer->address.port);
//        net_stopPeer(event.peer);
//      }
//
//      // link the player info to the event.peer->data field
//      event.peer->data = &(pcin_info->Slot);
//
//      break;
//
//    case ENET_EVENT_TYPE_DISCONNECT:
//
//      if( !CListIn_remove( &(sv_host->cin), event.peer ) )
//      {
//        net_logf("NET INFO: sv_dispatchPackets() - Unknown connection closing from \"%s\" (%s:%04x)\n", hostName, convert_host(event.peer->address.host), event.peer->address.port);
//      }
//
//      break;
//
//    case ENET_EVENT_TYPE_NONE:
//      net_logf("ENET_EVENT_TYPE_NONE\n");
//      break;
//
//    default:
//      net_logf("UNKNOWN\n");
//      break;
//    }
//
//
//  }
//
//  // !!make sure that all of the packets get sent!!
//  if(sv_Started(ss))
//  {
//    enet_host_flush(sv_host->Host);
//  }
//
//  return btrue;
//};
//
//--------------------------------------------------------------------------------------------
bool_t sv_sendPacketToAllClients(Server_t * ss, SYS_PACKET * egop)
{
  // ZZ> This function sends a packet to all the players

  NetHost_t * sv_host;

  if(NULL == ss || NULL == egop)
  {
    net_logf("NET ERROR: sv_sendPacketToAllClients() - Called with invalid parameters.\n");
    return bfalse;
  }

  sv_host = sv_getHost();
  if(NULL == sv_host) return bfalse;

  net_logf("NET INFO: sv_sendPacketToAllClients() - %s from %s:%04x\n", net_crack_sys_packet_type(egop), convert_host(sv_host->Host->address.host), sv_host->Host->address.port);

  if( !sv_Started() )
  {
    net_logf("NET WARN: sv_sendPacketToAllClients() - Server is not active.\n");
    return bfalse;
  }

  sv_host = sv_getHost();

  return net_sendSysPacketToAllPeers(sv_host->Host, egop);
}

//--------------------------------------------------------------------------------------------
bool_t sv_sendPacketToAllClientsGuaranteed(Server_t * ss, SYS_PACKET * egop)
{
  // ZZ> This function sends a packet to all the players

  NetHost_t * sv_host;

  if(NULL == ss || NULL == egop)
  {
    net_logf("NET ERROR: sv_sendPacketToAllClientsGuaranteed() - Called with invalid parameters.\n");
    return bfalse;
  }

  sv_host = sv_getHost();
  if(NULL == sv_host) return bfalse;

  net_logf("NET INFO: sv_sendPacketToAllClientsGuaranteed() - %s from %s:%04x\n", net_crack_sys_packet_type(egop), convert_host(sv_host->Host->address.host), sv_host->Host->address.port);

  if( !sv_Started() )
  {
    net_logf("NET WARN: sv_sendPacketToAllClientsGuaranteed() - Server is not active.\n");
    return bfalse;
  }

  sv_host = sv_getHost();

  return net_sendSysPacketToAllPeersGuaranteed(sv_host->Host, egop);
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// sv module NetHost_t singleton management
NetHost_t * sv_getHost()
{
  // make sure the host exists
  if(NULL == _sv_host)
  {
    _sv_Initialize();
  }

  return _sv_host;
}

//------------------------------------------------------------------------------
retval_t _sv_Initialize()
{
  if( NULL != _sv_host) return rv_succeed;

  _sv_host = NetHost_create( _sv_HostCallback );
  if(NULL == _sv_host) return rv_fail;

  if(!_sv_atexit_registered)
  {
    atexit(_sv_Quit);
  }

  return _sv_startUp();
}

//------------------------------------------------------------------------------
void sv_quitHost()
{
  NetHost_shutDown( _sv_host );
  NetHost_destroy( &_sv_host );
};

//------------------------------------------------------------------------------
void _sv_Quit(void)
{
  if( !_sv_atexit_registered ) return;
  if( NULL == _sv_host ) return;

  sv_quitHost();
}

//------------------------------------------------------------------------------
retval_t _sv_startUp(void)
{
  NetHost_t * nh = sv_getHost();
  if(NULL == nh) return rv_fail;

  return NetHost_startUp(nh, NET_EGOBOO_SERVER_PORT);
};

//------------------------------------------------------------------------------
retval_t _sv_shutDown(void)
{
  if(NULL == _sv_host) return rv_fail;

  return NetHost_shutDown(_sv_host);
}

//------------------------------------------------------------------------------
bool_t sv_Started()  { return (NULL != _sv_host) && _sv_host->nthread.Active; }

//------------------------------------------------------------------------------
bool_t sv_send_chr_setup( Server_t * ss, CHR_SPAWN_INFO * psi )
{
  SYS_PACKET egopkt;

  if( !EKEY_PVALID(psi) || !sv_Running(ss) ) return bfalse;

  // send the setup data
  net_startNewSysPacket(&egopkt);

  sys_packet_addUint16(&egopkt, TO_REMOTE_CHR_SPAWN);
  sys_packet_addUint32(&egopkt, psi->seed);
  sys_packet_addUint32(&egopkt, psi->net_id);
  sys_packet_addUint32(&egopkt, psi->ichr);

  // convert pos to 16.16 fixed point
  sys_packet_addSint32(&egopkt, psi->pos.x * (1<<16) );
  sys_packet_addSint32(&egopkt, psi->pos.y * (1<<16) );
  sys_packet_addSint32(&egopkt, psi->pos.z * (1<<16) );

  // convert vel to 16.16 fixed point
  sys_packet_addSint32(&egopkt, psi->vel.x * (1<<16) );
  sys_packet_addSint32(&egopkt, psi->vel.y * (1<<16) );
  sys_packet_addSint32(&egopkt, psi->vel.z * (1<<16) );

  sys_packet_addUint32(&egopkt, psi->iobj   );
  sys_packet_addUint32(&egopkt, psi->iteam  );
  sys_packet_addUint8 (&egopkt, psi->iskin  );
  sys_packet_addUint16(&egopkt, psi->facing );
  sys_packet_addUint8 (&egopkt, psi->slot   );
  sys_packet_addString(&egopkt, psi->name   );
  sys_packet_addUint32(&egopkt, psi->ioverride   );

  sys_packet_addSint32(&egopkt, psi->money   );
  sys_packet_addSint32(&egopkt, psi->content );
  sys_packet_addUint32(&egopkt, psi->passage );
  sys_packet_addSint32(&egopkt, psi->level   );
  sys_packet_addSint8 (&egopkt, psi->stat    );
  sys_packet_addSint8 (&egopkt, psi->ghost   );

  sv_sendPacketToAllClientsGuaranteed(ss, &egopkt);

  return btrue;
}