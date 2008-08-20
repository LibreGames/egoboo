//********************************************************************************************
//* Egoboo - Network.c
//*
//* This module implements the basic network functions that are used by
//* the modules that implement file transfers and network play
//*
//* Currently implemented using Enet.
//* Networked play doesn't really work at the moment.
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

#include "Network.inl"

#include "Client.h"
#include "Server.h"
#include "Log.h"
#include "NetFile.h"
#include "game.h"
#include "graphic.h"
#include "file_common.h"

#include "egoboo_utility.h"
#include "egoboo_strutil.h"

#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

#include "input.inl"
#include "egoboo_types.inl"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define MAX_NET_LOG_MESSAGE 1024

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static void close_session(struct sNet * ns);

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// network status
static bool_t       _enet_initialized       = bfalse;
static bool_t       _net_registered_atexit = bfalse;
static FILE       * net_logfile           = NULL;

// the network singleton
static void _net_Quit(void);

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct sNetService_Info
{
  int       references;
  NetHost_t * host;
  void    * data;
  Uint32    guid;
};
typedef struct sNetService_Info NetService_Info_t;

static int               _net_service_count = 0;
static NetService_Info_t _net_service_list[MAXSERVICE];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Uint32 net_nextGUID()
{
  // use a linear 32bit congruential generator to generate the GUIDs

  static Uint32 guid = (~(Uint32)0);

  if(guid == (~(Uint32)0)) guid = time(NULL);
  guid = (guid * 0x0019660DL) + 0x3C6EF35FL;

  return guid;
};

//--------------------------------------------------------------------------------------------
Uint32 net_addService(NetHost_t * host, void * data)
{
  int i;
  Uint32 id;
  bool_t found = bfalse;

  for(i=0; i<_net_service_count; i++)
  {
    if(_net_service_list[i].host == host &&
       _net_service_list[i].data == data)
    {
      found = btrue;
      break;
    }
  }

  if(found)
  {
    _net_service_list[i].references++;
    id = _net_service_list[i].guid;
  }
  else
  {
    id = net_nextGUID();

    _net_service_list[_net_service_count].references = 1;
    _net_service_list[_net_service_count].host       = host;
    _net_service_list[_net_service_count].data       = data;
    _net_service_list[_net_service_count].guid       = id;
    _net_service_count++;
  }

  return id;
}

//--------------------------------------------------------------------------------------------
bool_t net_removeService(Uint32 id)
{
  int i;
  bool_t found = bfalse;

  for(i=0; i<_net_service_count; i++)
  {
    if(_net_service_list[i].guid == id)
    {
      found = btrue;
      break;
    }
  }

  if(found)
  {
    _net_service_list[i].references--;

    if(_net_service_list[i].references <= 0)
    {
      int last = _net_service_count - 1;

      memmove(_net_service_list + i, _net_service_list + last, sizeof(NetService_Info_t));

      _net_service_count--;
    }
  }

  return found;
}

//--------------------------------------------------------------------------------------------
void * net_getService(Uint32 id)
{
  int i;
  void * retval = NULL;

  for(i=0; i<_net_service_count; i++)
  {
    if(_net_service_list[i].guid == id)
    {
      retval = _net_service_list[i].data;
      break;
    }
  }

  return retval;
}


//--------------------------------------------------------------------------------------------
// for identifying network connections
static Uint32 _CListIn_guid = 0;

//--------------------------------------------------------------------------------------------
// private Net_t initialization
static Net_t * CNet_new(Net_t * ns, Game_t * gs);
static bool_t     CNet_delete(Net_t * ns);

//--------------------------------------------------------------------------------------------
// private NetHost_t initialization
static NetHost_t * NetHost_new(NetHost_t * nh, SDL_Callback_Ptr pcall);
static bool_t    NetHost_delete(NetHost_t * nh);

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static retval_t net_loopWaitForPacket(PacketRequest_t * prequest, Uint32 granularity, size_t * data_size);


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

Uint32 NetHost_register(NetHost_t * nh, EventHandler_Ptr_t client_handler, void * client_data, Uint32 net_guid)
{
  Uint32 id = (~(Uint32)0);
  CListIn_Client_t * cli;

  if(NULL == nh) return id;

  cli = CListIn_add_client( &(nh->cin), client_handler, client_data, net_guid);

  if(NULL != cli) id = cli->guid;

  return id;
}

//--------------------------------------------------------------------------------------------
bool_t NetHost_unregister(NetHost_t * nh, Uint32 id)
{
  if(NULL == nh) return bfalse;

  return CListIn_remove_client( &(nh->cin), id);
}

//--------------------------------------------------------------------------------------------
retval_t NetHost_dispatch( NetHost_t * nh )
{
  // BB > Handle an ENet event.
  //      If we received a packet, send it to the correct location

  NetThread_t  * nthread;
  PacketRequest_t * prequest;
  Uint32 id;

  ENetEvent event;
  STREAM    tmpstream;
  CListIn_Info_t cin_info, *pcin_info;
  char hostName[64] = NULL_STRING;
  size_t copy_size;

  if( !EKEY_PVALID(nh) ) return rv_fail;

  nthread = &(nh->nthread);
  if(nthread->Terminated || nthread->KillMe) return rv_error;
  if(!nthread->Active) return rv_waiting;

  while ( enet_host_service(nh->Host, &event, 0) > 0 )
  {
    net_logf("--------------------------------------------------\n");
    net_logf("NET INFO: NetHost_dispatch() - Received event... ");
    if(!nthread->Active)
    {
      net_logf("Ignored\n");
      continue;
    }
    net_logf("Processing... ");

    switch (event.type)
    {
    case ENET_EVENT_TYPE_RECEIVE:
      net_logf("ENET_EVENT_TYPE_RECEIVE\n");

      prequest = NetRequest_check(nh->asynch, NETHOST_REQ_COUNT, &event);
      if( NULL != prequest && prequest->waiting )
      {
        // we have received a packet that someone is waiting for

        net_logf("NET INFO: NetHost_dispatch() - Received requested packet \"%s\" from %s:%04x\n", net_crack_enet_packet_type(event.packet), convert_host(event.peer->address.host), event.peer->address.port);

        // copy the data from the packet to a waiting buffer
        copy_size = MIN(event.packet->dataLength, prequest->data_size);
        memcpy(prequest->data, event.packet->data, copy_size);
        prequest->data_size = copy_size;

        // tell the other thread that we're done
        prequest->received = btrue;
      }
      else
      {
        CListIn_Client_t * pc;

        stream_startENet(&tmpstream, event.packet);
        id = stream_readUint32(&tmpstream);

        pc = CListIn_find( &(nh->cin), id );

        // if we trust pointers, we could actually call
        // (*pc->handler_ptr)(pc->handler_data, &event)

        if(!net_handlePacket( (Net_t *)pc->handler_data, &event))
        {
          net_logf("NET WARNING: NetHost_dispatch() - Unhandled packet\n");
        }
      }

      enet_packet_destroy(event.packet);
      break;

    case ENET_EVENT_TYPE_CONNECT:
      net_logf("ENET_EVENT_TYPE_CONNECT\n");

      cin_info.Address     = event.peer->address;
      cin_info.Hostname[0] = EOS;
      cin_info.Name[0]     = EOS;

      // grab the ID of the service the remote wants to connect to
      stream_startENet(&tmpstream, event.packet);
      id = stream_readUint32(&tmpstream);
      cin_info.handler_data = net_getService(id);

      // Look up the hostname the player is connecting from
      //enet_address_get_host(&event.peer->address, hostName, 64);
      //strncpy(ns->nfile.Hostname[cnt], hostName, sizeof(ns->nfile.Hostname[cnt]));

      // see if we can add the 'player' to the connection list
      pcin_info = CListIn_add( &(nh->cin), &cin_info );

      if(NULL == pcin_info)
      {
        net_logf("NET WARN: NetHost_dispatch() - Too many connections. Refusing connection from \"%s\" (%s:%04x)\n", hostName, convert_host(event.peer->address.host), event.peer->address.port);
        net_stopPeer(event.peer);
      }

      // link the player info to the event.peer->data field
      event.peer->data = &(pcin_info->Slot);

      break;

    case ENET_EVENT_TYPE_DISCONNECT:
      net_logf("ENET_EVENT_TYPE_DISCONNECT\n");

      if( !CListIn_remove( &(nh->cin), event.peer ) )
      {
        net_logf("NET INFO: NetHost_dispatch() - Unknown connection closing from \"%s\" (%s:%04x)\n", hostName, convert_host(event.peer->address.host), event.peer->address.port);
      }
      break;

    case ENET_EVENT_TYPE_NONE:
      net_logf("ENET_EVENT_TYPE_NONE\n");
      break;

    default:
      net_logf("UNKNOWN\n");
      break;
    }
  }

  // !!make sure that all of the packets get sent!!
  if(nthread->Active)
  {
    enet_host_flush(nh->Host);
  }

  return rv_succeed;
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
CListOut_Info_t * CListOut_Info_new(CListOut_Info_t *info)
{
  if(NULL == info) return info;

  CListOut_Info_delete(info);

  memset(info, 0, sizeof(CListOut_Info_t));

  EKEY_PNEW(info, CListOut_Info_t);

  return info;
}

//--------------------------------------------------------------------------------------------
bool_t CListOut_Info_delete(CListOut_Info_t * info)
{
  if(NULL == info) return bfalse;
  if( !EKEY_PVALID(info) ) return btrue;

  EKEY_PINVALIDATE( info );

  enet_peer_disconnect(info->peer, 0);

  memset(info, 0, sizeof(CListOut_Info_t));

  return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t CListOut_prune(CListOut_t * co)
{
  int imin, imax, j;

  imin = 0;
  imax = co->Count;

  // delete all clients whose references have gone to zero
  for(j=0; j<MAXCONNECTION; j++)
  {
    if(co->References[j] <= 0)
    {
      co->References[j] = 0;
      CListOut_Info_delete( co->Clients + j );
    }
  }

  // remove all deleted clients
  while(imin < imax)
  {
    // remove any null values from the end of the list
    while(imax > imin)
    {
      j = imax - 1;
      if( EKEY_VALID( co->Clients[j] ) ) break;
      imax--;
    }

    if(imin == imax) break;

    while(imin < imax)
    {
      if(!EKEY_VALID( co->Clients[j] ) ) break;
      imin++;
    }

    if(imin == imax) break;

    memcpy(co->Clients + imin, co->Clients + imax, sizeof(CListOut_Info_t));
    memcpy(co->References + imin, co->References + imax, sizeof(int));

    imax--;
  }

  co->Count = imax;

  return btrue;
}


//--------------------------------------------------------------------------------------------
ENetPeer * CListOut_add(CListOut_t * co, ENetHost * host, ENetAddress * address, void * client_data)
{
  int i;
  ENetPeer * retval = NULL;

  for(i=0; i<co->Count; i++)
  {
    if( (address->host == co->Clients[i].peer->address.host) &&
        (address->port == co->Clients[i].peer->address.port))
    {
      co->References[i]++;
      retval = co->Clients[i].peer;
      break;
    }
  }

  if(NULL == retval && co->Count < MAXCONNECTION)
  {
    retval = enet_host_connect(host, address, NET_EGOBOO_NUM_CHANNELS);

    if(NULL != retval)
    {
      CListOut_Info_new( co->Clients + co->Count );

      co->Clients[co->Count].peer        = retval;
      co->Clients[co->Count].client_data = client_data;

      co->Count++;
    }
  }

  return retval;
}

//--------------------------------------------------------------------------------------------
bool_t CListOut_remove(CListOut_t * co, ENetPeer * peer)
{
  int i, j;

  if(NULL == co || NULL == peer) return bfalse;

  j = -1;
  for(i=0; i<co->Count; i++)
  {
    if( (peer->address.host == co->Clients[i].peer->address.host) &&
        (peer->address.port == co->Clients[i].peer->address.port))
    {
      j = i;
      break;
    }
  }

  if(j != -1)
  {
    if(co->References[j] > 0)
    {
      co->References[j]--;
    };

    if(co->References[j] == 0)
    {
      CListOut_Info_delete(co->Clients + j);
    }
  }

  return CListOut_prune(co);
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void _net_Quit(void)
{
  // shut down all network services
  // I guess can't be left for the  _xx_Quit() functions because
  // they certainly need to be shut down before enet is shut down
  sv_quitHost();
  cl_quitHost();
  nfile_quitHost();

  // really shut down Enet
  if(_enet_initialized)
  {
    enet_deinitialize();
    _enet_initialized = bfalse;
  }

  // close the logfile
  if(NULL != net_logfile)
  {
    fclose(net_logfile);
    net_logfile = NULL;
  }
};

//--------------------------------------------------------------------------------------------
bool_t net_Started()
{
  return _enet_initialized;
}

//--------------------------------------------------------------------------------------------
retval_t net_startUp(ConfigData_t * cd)
{
  // ZZ> This starts up the network and logs whatever goes on

  bool_t request_network = bfalse;

  if(_enet_initialized) return rv_succeed;

  // open the log file
  net_logfile = fopen("net.txt", "w");

  // actually initialize the network
  if(NULL != cd) request_network = cd->request_network;
  if (request_network)
  {
    // initialize enet
    net_logf("NET INFO: net_startUp() - Initializing enet... ");
    if (0 != enet_initialize())
    {
      net_logf("Failed!\n");
      _enet_initialized = bfalse;
    }
    else
    {
      net_logf("Succeeded!\n");
      _enet_initialized = btrue;
    }
  }
  else
  {
    // We're not doing networking this time...
    net_logf("NET INFO: net_startUp() - Networking not enabled.\n");
    _enet_initialized = bfalse;
  }

  // only do this once
  if(!_net_registered_atexit)
  {
    atexit(_net_Quit);
    _net_registered_atexit = btrue;
  };

  // reset the registered services
  _net_service_count = 0;
  memset(_net_service_list, 0, MAXSERVICE * sizeof(NetService_Info_t));

  return _enet_initialized ? rv_succeed : rv_fail;
}

//--------------------------------------------------------------------------------------------
retval_t net_shutDown()
{
  net_logf("NET INFO: net_shutDown: Turning off networking.\n");

  if(!_enet_initialized) return rv_error;

  //cl_shutDown();
  //sv_shutDown();
  //nfile_shutDown();

  if(NULL !=net_logfile)
  {
    fclose(net_logfile);
    net_logfile = NULL;
  }

  // reset the registered services
  _net_service_count = 0;
  memset(_net_service_list, 0, MAXSERVICE * sizeof(NetService_Info_t));

  enet_deinitialize();

  _enet_initialized = bfalse;

  return rv_succeed;
}

//--------------------------------------------------------------------------------------------
//retval_t net_startUp(Net_t * ns, Game_t * gs)
//{
//  // ZZ> This starts up the network and logs whatever goes on
//
//  if(NULL == ns) return rv_error;
//
//  if(_enet_initialized) return rv_succeed;
//
//  // initialize the network state
//  CNet_new(ns);
//  CNet_initialize(ns, gs->cl, gs->sv, NULL, gs);
//
//  // open the log file
//  net_logfile = fopen("net.txt", "w");
//
//  // actually initialize the network
//  if (CData.request_network)
//  {
//    // initialize enet
//    net_logf("NET INFO: net_startUp() - Initializing enet... ");
//    if (0 != enet_initialize())
//    {
//      net_logf("Failed!\n");
//      _enet_initialized = bfalse;
//    }
//    else
//    {
//      net_logf("Succeeded!\n");
//      _enet_initialized = btrue;
//    }
//  }
//  else
//  {
//    // We're not doing networking this time...
//    net_logf("NET INFO: net_startUp() - Networking not enabled.\n");
//    _enet_initialized = bfalse;
//  }
//
//  // only do this once
//  if(!_net_registered_atexit)
//  {
//    atexit(_net_Quit);
//    _net_registered_atexit = btrue;
//  };
//
//  return _enet_initialized ? rv_succeed : rv_fail;
//}
//
//
//
////--------------------------------------------------------------------------------------------
//retval_t net_shutDown(Net_t * ns)
//{
//  net_logf("NET INFO: net_shutDown: Turning off networking.\n");
//
//  if(!_enet_initialized) return rv_error;
//
//  cl_shutDown( ns->parent->cl );
//  sv_shutDown( ns->parent->sv );
//  nfile_shutDown( ns->nfs );
//
//  CNet_delete(ns);
//
//  if(NULL !=net_logfile)
//  {
//    fclose(net_logfile);
//    net_logfile = NULL;
//  }
//
//  enet_deinitialize();
//
//  _enet_initialized = bfalse;
//
//  return rv_succeed;
//}
//
////--------------------------------------------------------------------------------------------
//bool_t net_Started(Net_t * ns)
//{
//  return _enet_initialized && EKEY_PVALID( ns );
//}
//

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Net_t * CNet_create(Game_t * gs)
{
  Net_t * ns = EGOBOO_NEW( Net_t );
  return CNet_new(ns, gs);
}

//--------------------------------------------------------------------------------------------
bool_t CNet_destroy(Net_t ** pns)
{
  bool_t retval;

  if(NULL == pns || NULL == *pns) return bfalse;
  if( !EKEY_PVALID( (*pns) ) ) return btrue;

  retval = CNet_delete(*pns);
  EGOBOO_DELETE( *pns );

  return retval;
}

//--------------------------------------------------------------------------------------------
Net_t * CNet_new(Net_t * ns, Game_t * gs)
{
  //fprintf( stdout, "CNet_new()\n");

  if(NULL == ns) return ns;

  CNet_delete(ns);

  memset(ns, 0, sizeof(Net_t));

  EKEY_PNEW(ns, Net_t);

  ns->parent = gs;

  // no need to transfer files if there is no network
  if(gs->cd->request_network)
  {
    ns->nfs = NFileState_create(ns);
  };

  return ns;
}

//--------------------------------------------------------------------------------------------
bool_t CNet_delete(Net_t * ns)
{
  if(NULL == ns) return bfalse;
  if(!EKEY_PVALID( ns )) return btrue;

  EKEY_PINVALIDATE( ns );

  NFileState_destroy(  &(ns->nfs) );

  // let the thread kill itself
  net_logf("NET INFO: CNet_delete(): deleting all Net_t services.\n");

  return btrue;
}

//--------------------------------------------------------------------------------------------
//bool_t CNet_initialize(Net_t * ns, Client_t * cs, Server_t * ss, NFileState_t * nfs, struct sGame * gs)
//{
//  // BB> This connects the Net_t to the outside world
//
//  // Link us with the client state
//  ns->parent->cl = cs;
//  if(NULL == ns->parent->cl)
//  {
//    ns->parent->cl = cl_getState();
//  };
//  ns->parent->cl = CClient_new(ns->parent->cl, ns);
//
//  // Link us with the server state
//  ns->parent->sv = ss;
//  if(NULL == ns->parent->sv)
//  {
//    ns->parent->sv = sv_getState();
//  }
//  ns->parent->sv = Server_new(ns->parent->sv, ns);
//
//  // Link us with the file transfer state
//  ns->nfs = nfs;
//  if(NULL == ns->nfs)
//  {
//    ns->nfs = nfile_getState();
//  }
//  ns->nfs = NFileState_new(ns->nfs);
//
//  // Link it to the game state
//  ns->parent = gs;
//
//  return btrue;
//}

//--------------------------------------------------------------------------------------------
//bool_t CNet_shutDown(Net_t * ns)
//{
//  if(NULL == ns ) return bfalse;
//  if(!EKEY_PVALID( ns )) return btrue;
//
//  //Client_shutDown(ns->parent->cl);
//  //Server_shutDown(ns->parent->sv);
//  NFileState_shutDown(ns->nfs);
//
//  return btrue;
//}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t net_sendSysPacketToPeer(ENetPeer *peer, SYS_PACKET * egop)
{
  // JF> This funciton sends a packet to a given peer, with guaranteed delivery
    ENetPacket *packet;

  if(NULL ==peer || NULL == egop)
  {
    net_logf("NET ERROR: net_sendSysPacketToPeerGuaranteed() - Called with invalid parameters.\n");
    return bfalse;
  }

  net_logf("NET INFO: net_sendSysPacketToPeer() - %s to %s:%04x\n", net_crack_sys_packet_type(egop), convert_host(peer->address.host), peer->address.port);

  if(0 == egop->size)
  {
    net_logf("NET ERROR: net_sendSysPacketToPeerGuaranteed() - Culling zero length packet\n");
    return btrue;
  }

  packet = enet_packet_create(egop->buffer, egop->size, 0);
  if(NULL != packet)
  {
    enet_peer_send(peer, NET_UNRELIABLE_CHANNEL, packet);
  }
  else
  {
    net_logf("NET WARN: net_sendSysPacketToPeer() - Unable to generate packet.\n" );
  }

  return (NULL != packet);
}

//--------------------------------------------------------------------------------------------
bool_t net_sendSysPacketToPeerGuaranteed(ENetPeer *peer, SYS_PACKET * egop)
{
  // JF> This funciton sends a packet to a given peer, with guaranteed delivery
    ENetPacket *packet;

  if(NULL ==peer || NULL == egop)
  {
    net_logf("NET ERROR: net_sendSysPacketToPeerGuaranteed() - Called with invalid parameters.\n");
    return bfalse;
  }

  net_logf("NET INFO: net_sendSysPacketToPeerGuaranteed() - %s to %s:%04x\n", net_crack_sys_packet_type(egop), convert_host(peer->address.host), peer->address.port);

  if(0 == egop->size)
  {
    net_logf("NET ERROR: net_sendSysPacketToPeerGuaranteed() - Culling zero length packet\n");
    return btrue;
  }

  packet = enet_packet_create(egop->buffer, egop->size, ENET_PACKET_FLAG_RELIABLE);
  if(NULL != packet)
  {
    enet_peer_send(peer, NET_GUARANTEED_CHANNEL, packet);
  }
  else
  {
    net_logf("NET WARN: net_sendSysPacketToPeerGuaranteed() - Unable to generate packet.\n" );
  }

  return (NULL != packet);
}


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//int net_Callback(void * data)
//{
//  Net_t * ns = (Net_t *)data;
//  Server_t  * ss = ns->parent->sv;
//  Client_t  * cs = ns->parent->cl;
//
//  if(NULL ==ns)
//  {
//    net_logf("NET WARNING: Net callback thread - unable to start.\n");
//    return bfalse;
//  }
//
//  net_logf("NET INFO: net_Callback() thread - starting normally.\n");
//
//  while(!ns->kill_me)
//  {
//    if(!ns->pause_me)
//    {
//      // do a *_dispatchPackets() call for each
//      // ENetHost that can be active
//
//      // do the server stuff (if any)
//      sv_dispatchPackets(ns->parent->sv);
//
//      // do the client stuff (if any)
//      cl_dispatchPackets(ns->parent->cl);
//
//      // do the file transfer stuff (if any)
//      nfile_dispatchPackets(ns->nfs);
//    }
//
//    SDL_Delay(10);
//  };
//
//  net_logf("NET INFO: Net callback thread - exiting normally\n");
//  return btrue;
//};

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
retval_t net_copyFileToPeer(Net_t * ns, ENetPeer * peer, char *source, char *dest)
{
  // JF> This function queues up files to send to all the hosts.
  //     TODO: Deal with having to send to up to PLALST_COUNT players...

  if( NULL == ns || NULL == peer ) return rv_error;

  if( !VALID_CSTR(source) )
  {
    net_logf("NET ERROR: net_copyFileToPeer() - null source file.\n");
    return rv_error;
  }

  if( !VALID_CSTR(dest) )
  {
    net_logf("NET ERROR: net_copyFileToPeer() - null dest file.\n");
    return rv_error;
  }

  // the CRCs are different. Send the file.
  return nfile_SendQueue_add(ns->nfs, &(peer->address), source, dest);
}


//------------------------------------------------------------------------------
retval_t net_copyFileToAllPeers(Net_t * ns, char *source, char *dest)
{
  // JF> This function queues up files to send to all the hosts.
  //     TODO: Deal with having to send to up to PLALST_COUNT players...

  if(NULL == ns)
  {
    net_logf("NET ERROR: net_copyFileToAllPeers() - Called with invalid parameters.\n");
    return rv_error;
  }

  if( !VALID_CSTR(source) )
  {
    net_logf("NET ERROR: net_copyFileToAllPeers() - null source file.\n");
    return rv_error;
  }

  if( !VALID_CSTR(dest) )
  {
    net_logf("NET ERROR: net_copyFileToAllPeers() - null dest file.\n");
    return rv_error;
  }

  if( !nfile_Started() )
  {
    net_logf("NET WARN: net_copyFileToAllPeers() - server is not active.\n");
    return rv_fail;
  }

  net_logf("NET INFO: net_copyFileToAllPeers() - copy \"%s\" to \"%s\" on all peers.\n", source, dest);

  return nfile_SendQueue_add(ns->nfs, NULL, source, dest);
}


//------------------------------------------------------------------------------
retval_t net_copyFileToHost(Net_t * ns, char *source, char *dest)
{
  // JF> New function merely queues up a new file to be sent

  NetHost_t * cl_host, * sv_host;

  if(NULL == ns || NULL == ns->parent  || NULL == ns->parent->sv)
  {
    net_logf("NET ERROR: net_copyFileToHost() - Called with invalid parameter.\n");
    return rv_error;
  }

  if( !VALID_CSTR(source) )
  {
    net_logf("NET ERROR: net_copyFileToHost() - null source file.\n");
    return rv_error;
  }

  if( !VALID_CSTR(dest) )
  {
    net_logf("NET ERROR: net_copyFileToHost() - null dest file.\n");
    return rv_error;
  }

  cl_host = cl_getHost();
  if(!cl_host->nthread.Active)
  {
    net_logf("NET WARN: net_copyFileToHost() - Client not active.\n");
    return rv_fail;
  }

  // If this is the host, just copy the file locally
  sv_host = sv_getHost();
  if (sv_host->nthread.Active)
  {
    net_logf("NET INFO: net_copyFileToHost() - Client and server active. Local game... ");

    // Simulate a network transfer
    if (fs_fileIsDirectory(source))
    {
      fs_createDirectory(dest);
    }
    else
    {
      fs_copyFile(source, dest);
    }

    net_logf("Done\n");
    return rv_succeed;
  }

  net_logf("NET INFO: net_copyFileToHost() - Copy %s:%04x:\"%s\" to %s:%04x:\"%s\".\n",
    convert_host(cl_host->Host->address.host), cl_host->Host->address.port, source,
    convert_host(ns->parent->cl->gamePeer->address.host), ns->parent->cl->gamePeer->address.port, dest);

  return nfile_SendQueue_add( ns->nfs, &(ns->parent->cl->gamePeer->address), source, dest );
}


//--------------------------------------------------------------------------------------------
retval_t net_copyDirectoryToHost(Net_t * ns, char *dirname, char *todirname)
{
  // ZZ> This function copies all files in a directory
  STRING searchname, fromname, toname;
  const char *searchResult;
  FS_FIND_INFO fnd_info;

  net_logf("NET INFO: net_copyDirectoryToHost: %s, %s\n", dirname, todirname);

  // Search for all files
  snprintf(searchname, sizeof(searchname), "%s" SLASH_STRING "*", dirname);
  searchResult = fs_findFirstFile(fs_find_info_new(&fnd_info), dirname, NULL, NULL);
  if ( NULL != searchResult )
  {
    // Make the new directory
    net_copyFileToHost(ns, dirname, todirname);

    // Copy each file
    while ( NULL != searchResult )
    {
      // If a file begins with a dot, assume it's something
      // that we don't want to copy.  This keeps repository
      // directories, /., and /.. from being copied
      // Also avoid copying directories in general.
      snprintf(fromname, sizeof(fromname), "%s" SLASH_STRING "%s", dirname, searchResult);
      if (searchResult[0] == '.' || fs_fileIsDirectory(fromname))
      {
        searchResult = fs_findNextFile(&fnd_info);
        continue;
      }

      snprintf(fromname, sizeof(fromname), "%s" SLASH_STRING "%s", dirname, searchResult);
      snprintf(toname, sizeof(toname), "%s" NET_SLASH_STRING "%s", todirname, searchResult);

      net_copyFileToHost(ns, fromname, toname);
      searchResult = fs_findNextFile(&fnd_info);
    }
  }

  fs_findClose(&fnd_info);

  return rv_succeed;
}

//--------------------------------------------------------------------------------------------
retval_t net_copyDirectoryToAllPeers(Net_t * ns, char *dirname, char *todirname)
{
  // ZZ> This function copies all files in a directory
  STRING searchname, fromname, toname;
  const char *searchResult;
  FS_FIND_INFO fnd_info;

  net_logf("NET INFO: net_copyDirectoryToAllPeers: %s, %s\n", dirname, todirname);
  // Search for all files
  snprintf(searchname, sizeof(searchname), "%s" SLASH_STRING "*.*", dirname);
  searchResult = fs_findFirstFile(fs_find_info_new(&fnd_info), dirname, NULL, NULL);
  if ( NULL != searchResult )
  {
    // Make the new directory
    net_copyFileToAllPeers(ns, dirname, todirname);

    // Copy each file
    while ( NULL != searchResult )
    {
      // If a file begins with a dot, assume it's something
      // that we don't want to copy.  This keeps repository
      // directories, /., and /.. from being copied
      if (searchResult[0] == '.')
      {
        searchResult = fs_findNextFile(&fnd_info);
        continue;
      }

      snprintf(fromname, sizeof(fromname), "%s" SLASH_STRING "%s", dirname, searchResult);
      snprintf(toname, sizeof(toname), "%s" NET_SLASH_STRING "%s", todirname, searchResult);
      net_copyFileToAllPeers(ns, fromname, toname);

      searchResult = fs_findNextFile(&fnd_info);
    }
  }
  fs_findClose(&fnd_info);

  return rv_succeed;
}

//--------------------------------------------------------------------------------------------
retval_t net_copyDirectoryToPeer(Net_t * ns, ENetPeer * peer, char *dirname, char *todirname)
{
  // ZZ> This function copies all files in a directory
  STRING searchname, fromname, toname;
  const char *searchResult;
  retval_t retval;
  FS_FIND_INFO fnd_info;

  net_logf("NET INFO: net_copyDirectoryToOnePlayers: %s, %s\n", dirname, todirname);

  // Search for all files
  snprintf(searchname, sizeof(searchname), "%s" SLASH_STRING "*.*", dirname);
  searchResult = fs_findFirstFile(fs_find_info_new(&fnd_info), dirname, NULL, NULL);
  if ( NULL != searchResult )
  {
    // Make the new directory
    retval = net_copyFileToPeer(ns, peer, dirname, todirname);
    if(rv_error == retval) return retval;

    // Copy each file
    while ( NULL != searchResult )
    {
      // If a file begins with a dot, assume it's something
      // that we don't want to copy.  This keeps repository
      // directories, /., and /.. from being copied
      if (searchResult[0] == '.')
      {
        searchResult = fs_findNextFile(&fnd_info);
        continue;
      }

      snprintf(fromname, sizeof(fromname), "%s" SLASH_STRING "%s", dirname, searchResult);
      snprintf(toname, sizeof(toname), "%s" NET_SLASH_STRING "%s", todirname, searchResult);
      retval = net_copyFileToPeer(ns, peer, fromname, toname);
      if(rv_error == retval) return retval;

      searchResult = fs_findNextFile(&fnd_info);
    }
  }
  fs_findClose(&fnd_info);

  return rv_succeed;
}

//--------------------------------------------------------------------------------------------
void net_sayHello(Game_t * gs)
{
  // ZZ> This function lets everyone know we're here

  Client_t * cl = gs->cl;
  Server_t * sv = gs->sv;

  if (!_enet_initialized)
  {
    cl->waiting = bfalse;
    sv->ready   = btrue;
  }
  else
  {
    NetHost_t * cl_host, * sv_host;

    // a given machine can be BOTH server and client at the moment
    sv_host = sv_getHost();
    if (sv_host->nthread.Active)
    {
      net_logf("NET INFO: net_sayHello: Server saying hello.\n");
    }

    cl_host = cl_getHost();
    if (cl_host->nthread.Active)
    {
      SYS_PACKET egopkt;
      net_logf("NET INFO: net_sayHello: Client saying hello.\n");
      net_startNewSysPacket(&egopkt);
      sys_packet_addUint16(&egopkt, TO_HOST_IM_LOADED);
      Client_sendPacketToHostGuaranteed(cl, &egopkt);
      cl->waiting = btrue;
    }
  }
}

//--------------------------------------------------------------------------------------------
bool_t localize_filename(char * szin, char * szout)
{
  char * ptmp;
  size_t len;
  if(NULL ==szin || NULL ==szout) return bfalse;

  // scan through the string and localize every part of the string beginning with '@'
  *szout = EOS;
  while(*szin != 0)
  {
    if('@' == *szin)
    {
      ptmp = get_config_string(&CData, szin+1, &szin);
      if(NULL !=ptmp)
      {
        len = strlen(ptmp);
        strcpy(szout, ptmp);
        szout += len;
      };
    }
    else
    {
      *szout++ = *szin++;
    }
  };

  return btrue;
};

//--------------------------------------------------------------------------------------------
bool_t net_handlePacket(Net_t * ns, ENetEvent *event)
{
  Uint16 header;
  STRING filename, local_filename;   // also used for reading various strings
  size_t fileSize;
  bool_t retval = bfalse;
  SYS_PACKET egopkt;
  NET_PACKET netpkt;

  Client_t * cl = ns->parent->cl;
  Server_t * sv = ns->parent->sv;

  net_logf("NET INFO: net_handlePacket: Received \n");

  //if(!_enet_initialized) return bfalse;

  if(cl_handlePacket(cl, event)) return btrue;
  if(sv_handlePacket(sv, event)) return btrue;

  net_logf("NET INFO: net_handlePacket: Processing... ");

  // rewind the packet
  net_packet_startReading(&netpkt, event->packet);
  header = net_packet_readUint16(&netpkt);

  // process our messages
  retval = btrue;
  switch (header)
  {
  case TO_ANY_TEXT:
    net_logf("TO_ANY_TEXT\n");

    // someone has sent us some text to display

    packet_readString(&netpkt, filename, 255);
    debug_message(1, "%s", filename);
    break;

  case NET_CHECK_CRC:
    {
      Uint32 CRC, CRCseed;

      net_logf("NET_CHECK_CRC\n");

      CRCseed = net_packet_readUint32(&netpkt);
      packet_readString(&netpkt, filename, 256);

      // Acknowledge that we got the request file
      net_startNewSysPacket(&egopkt);
      sys_packet_addUint16(&egopkt, NET_ACKNOWLEDGE_CRC);
      net_sendSysPacketToPeerGuaranteed(event->peer, &egopkt);

      if( rv_succeed != util_calculateCRC(filename, CRCseed, &CRC) ) CRC = (Uint32)(-1);

      // Acknowledge that we got the request file
      net_startNewSysPacket(&egopkt);
      sys_packet_addUint16(&egopkt, NET_SEND_CRC);
      sys_packet_addUint32(&egopkt, CRC);
      net_sendSysPacketToPeerGuaranteed(event->peer, &egopkt);
    }
    break;

  case NET_ACKNOWLEDGE_CRC:
    net_logf("NET_ACKNOWLEDGE_CRC\n");
    //assert(wating_for_crc);
    //crc_acknowledged = btrue;
    break;

  case NET_SEND_CRC:
    net_logf("NET_SEND_CRC\n");

    //assert(crc_acknowledged);
    //returned_crc = net_packet_readUint32(&netpkt);
    //wating_for_crc = bfalse;
    break;

  case NET_REQUEST_FILE:
    {
      // crack the incoming packet so we can localize the location of the file to this machine

      STRING remote_filename = NULL_STRING, local_filename = NULL_STRING, temp = NULL_STRING;

      // read the remote filename
      packet_readString(&netpkt, temp, sizeof(temp));
      localize_filename(temp, remote_filename);

      packet_readString(&netpkt, temp, sizeof(temp));
      localize_filename(temp, local_filename);

      net_logf("NET_REQUEST_FILE - \"%s\"\n", temp);

      // convert the web compatable directory slashes to local slashes
      str_convert_slash_sys(local_filename, sizeof(local_filename));

      // We successfully cracked the filename.  Now send the file if it exists on
      // our machine and if the remote machine's CRC does not match ours
      net_copyFileToPeer(ns, event->peer, local_filename, remote_filename);
    }
    break;

  case TO_HOST_REQUEST_MODULE:
    {
      // The other computer is requesting all the files for the module

      STRING remote_filename = NULL_STRING, module_filename = NULL_STRING, /* module_path = NULL_STRING, */ temp = NULL_STRING;

      net_logf("TO_HOST_REQUEST_MODULE\n");

      // read the module name
      packet_readString(&netpkt, temp, sizeof(module_filename));

      //// Acknowledge that we got the request file
      //net_startNewSysPacket(&egopkt);
      //sys_packet_addUint16(&egopkt, TO_CLIENT_ACKNOWLEDGE_REQUEST_MODULE);
      //net_sendSysPacketToPeerGuaranteed(event->peer, &egopkt);

      snprintf(module_filename, sizeof(module_filename), "%s" SLASH_STRING "%s", CData.modules_dir, temp);
      snprintf(remote_filename, sizeof(remote_filename), "@modules_dir" NET_SLASH_STRING "%s", module_filename);

      net_copyDirectoryToPeer(ns, event->peer, module_filename, remote_filename);

      net_startNewSysPacket(&egopkt);
      sys_packet_addUint16(&egopkt, NET_DONE_SENDING_FILES);
      net_sendSysPacketToPeerGuaranteed(event->peer, &egopkt);
    }
    break;

  case NET_TRANSFER_FILE:
    // we are receiving a file to a remote computer

    net_logf("NET_TRANSFER_FILE\n");

    packet_readString(&netpkt, filename, 256);
    localize_filename(filename, local_filename);

    fileSize = net_packet_readUint32(&netpkt);

    // add the file to the receive queue and let the receive thread handle
    // the file transfer
    retval = (rv_succeed == nfile_ReceiveQueue_add(ns->nfs, event, filename));

    break;

  case NET_TRANSFER_ACK:

    net_logf("NET_TRANSFER_ACK. The last file sent was successful.\n");

    // the other remote computer has responded that our file transfer was successful

    //ns->nq->waitingForAck = 0;
    //ns->nq->numTransfers--;
    break;

  case NET_CREATE_DIRECTORY:

    // a remote computer is requesting that we create a directory

    net_logf("NET_CREATE_DIRECTORY\n");

    packet_readString(&netpkt, filename, 256);
    localize_filename(filename, local_filename);

    // add the file to the receive queue and let the receive thread handle
    // the file transfer
    retval = (rv_succeed == nfile_ReceiveQueue_add(ns->nfs, event, filename));

    break;

  case NET_DONE_SENDING_FILES:
    net_logf("NET_DONE_SENDING_FILES\n");

    // a remote computer is telling us they are done sending files

    //numplayerrespond++;
    break;

  case NET_NUM_FILES_TO_SEND:
    net_logf("NET_NUM_FILES_TO_SEND\n");

    // a remote computer is telling us how many files to expect to receive

    //numfileexpected = (int)net_packet_readUint16(&netpkt);
    break;

  default:
    net_logf("UNKNOWN\n");
    retval = bfalse;
    break;
  }


  return retval;
}



//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void close_session(Net_t * ns)
{
  // ZZ> This function gets the computer out of a network game

  Client_t * cl = ns->parent->cl;
  Server_t * sv = ns->parent->sv;

  if ( !_enet_initialized ) return;

  sv_unhostGame(sv);
  Client_unjoinGame(cl);

  // pause the network threads
  Client_shutDown(cl);
  Server_shutDown(sv);
  NFileState_shutDown(ns->nfs);
}

//--------------------------------------------------------------------------------------------
bool_t net_beginGame(struct sGame * gs)
{
  NetHost_t *cl_host, * sv_host;
  bool_t retval = btrue;

  if(NULL ==gs) return bfalse;

  // try to host a game
  sv_host = sv_getHost();
  if(retval && sv_host->nthread.Active)
  {
    bool_t bool_tmp = (rv_succeed == Server_startUp(gs->sv));
    retval = retval && bool_tmp;
  };

  // try to join a hosted game
  cl_host = cl_getHost();
  if(retval && cl_host->nthread.Active)
  {
    bool_t bool_tmp = (rv_succeed == Client_joinGame(gs->cl, gs->cd->net_hosts[0]));
    retval = retval && bool_tmp;
  };

  return retval;
};


//--------------------------------------------------------------------------------------------
PLA_REF add_player(Game_t * gs, CHR_REF chr_ref, Uint8 device)
{
  // ZZ> This function adds a pla_ref, returning bfalse if it fails, btrue otherwise

  PLA_REF pla_ref;
  Player_t * pla;
  Chr_t    * chr;

  if( !EKEY_PVALID(gs) || gs->PlaList_count + 1 >= PLALST_COUNT ) return INVALID_PLA;

  if( !VALID_CHR(gs->ChrList, chr_ref) ) return INVALID_PLA;
  chr = gs->ChrList + chr_ref;

  // find the average spawn level of the characters
  // kludge to find the "floor level" of the dungeon for the reflective floors
  gs->PlaList_level = (gs->PlaList_level * gs->PlaList_count + chr->level) / (gs->PlaList_count + 1);

  pla_ref = gs->PlaList_count;
  gs->PlaList_count++;

  pla = gs->PlaList + pla_ref;
  Player_renew(pla);

  chr->whichplayer = pla_ref;
  pla->chr_ref = chr_ref;
  pla->Active = btrue;
  pla->device = device;
  Latch_clear( &(pla->latch) );

  Client_resetTimeLatches(gs->cl, chr_ref);
  Server_resetTimeLatches(gs->sv, chr_ref);

  gs->allpladead = bfalse;
  if (device != INBITS_NONE)
  {
    pla->is_local = btrue;
    gs->cl->loc_pla_count++;
  }
  else
  {
    pla->is_local = bfalse;
    gs->sv->rem_pla_count++;
  }

  return pla_ref;
}

//--------------------------------------------------------------------------------------------
void net_send_chat(Game_t *gs, KeyboardBuffer_t * kbuffer)
{
  SYS_PACKET egopkt;
  NetHost_t * cl_host, *sv_host;

  Server_t * sv = gs->sv;
  Client_t * cl = gs->cl;

  sv_host = sv_getHost();
  cl_host = cl_getHost();

  // Yes, so send it
  kbuffer->buffer[kbuffer->write] = 0;
  if(sv_host->nthread.Active || cl_host->nthread.Active)
  {
    net_startNewSysPacket(&egopkt);
    sys_packet_addUint16(&egopkt, TO_ANY_TEXT);
    sys_packet_addString(&egopkt, kbuffer->buffer);
    if(sv_host->nthread.Active && !cl_host->nthread.Active)
    {
      sv_sendPacketToAllClients(sv, &egopkt);
    }
    else
    {
      Client_sendPacketToHost(cl, &egopkt);
    };
  }
}

//--------------------------------------------------------------------------------------------
void do_chat_input()
{
  // ZZ> This function lets players communicate over network by hitting return, then
  //     typing text, then return again

  KeyboardBuffer_t * kbuffer = KeyboardBuffer_getState();

  if(keyb.mode)
  {
    // Make cursor flash
    if(kbuffer->write < sizeof(STRING)-4)
    {
      if((keyb.delay & 7) == 0)
      {
        kbuffer->buffer[kbuffer->write+0] = 'x';
        kbuffer->buffer[kbuffer->write+1] = EOS;
      }
      else
      {
        kbuffer->buffer[kbuffer->write+0] = '+';
        kbuffer->buffer[kbuffer->write+1] = EOS;
      }
    }
  }

  if(kbuffer->done)
  {
    if(net_Started())
    {
      net_send_chat( Graphics_requireGame(&gfxState), kbuffer );
    }

    debug_message(1, kbuffer->buffer);
    kbuffer->done = bfalse;
  }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t NetRequest_delete(PacketRequest_t * pr)
{
  if( NULL == pr ) return bfalse;
  if( !EKEY_PVALID(pr) ) return btrue;

  EKEY_PINVALIDATE( pr );

  return btrue;
}

//--------------------------------------------------------------------------------------------
PacketRequest_t * NetRequest_new(PacketRequest_t * pr)
{
  // do some simple initialization

  //fprintf( stdout, "NetRequest_new()\n");

  if( NULL == pr ) return pr;

  NetRequest_delete( pr );

  memset( pr, 0, sizeof(PacketRequest_t) );

  EKEY_PNEW( pr, PacketRequest_t );

  pr->received = bfalse;
  pr->data_size = 0;
  pr->data = NULL;

  return pr;
}

//--------------------------------------------------------------------------------------------
int NetRequest_getFreeIndex(NetAsynchData_t * asynch_list, size_t asynch_count)
{
  size_t index;
  if(NULL ==asynch_list || 0 == asynch_count) return -1;

  for(index=0; index<asynch_count; index++)
  {
    if( !asynch_list[index].req.waiting )
    {
      return index;
    };
  };

  return -1;
};

//--------------------------------------------------------------------------------------------
PacketRequest_t * NetRequest_getFree(NetAsynchData_t * asynch_list, size_t asynch_count)
{
  size_t index;
  if(NULL ==asynch_list || 0 == asynch_count) return NULL;

  for(index=0; index<asynch_count; index++)
  {
    if( !asynch_list[index].req.waiting )
    {
      return NetRequest_new( &(asynch_list[index].req) );
    };
  };

  return NULL;
};

//--------------------------------------------------------------------------------------------
bool_t NetRequest_test(PacketRequest_t * prequest)
{
  if(NULL ==prequest) return bfalse;
  return prequest->received;
};

//--------------------------------------------------------------------------------------------
bool_t NetRequest_release(PacketRequest_t * prequest)
{
  if(NULL ==prequest) return bfalse;
  prequest->waiting  = bfalse;
  prequest->received = bfalse;

  // unlink this request with external data
  prequest->peer     = NULL;
  prequest->data     = NULL;

  return btrue;
};



//--------------------------------------------------------------------------------------------
PacketRequest_t * NetRequest_check(NetAsynchData_t * asynch_list, size_t asynch_count, ENetEvent * pevent)
{
  size_t index;
  PacketRequest_t * retval = NULL;
  NET_PACKET       npkt;
  if(NULL ==asynch_list || 0 == asynch_count || NULL ==pevent) return NULL;

  for(index=0; index<asynch_count; index++)
  {
    if( !asynch_list[index].req.waiting ) continue;

    // get rid of all expired requests
    //if(SDL_GetTicks() > asynch_list[index].req.expiretimestamp)
    //{
    //  asynch_list[index].req.waiting = bfalse;
    //  continue;
    //}

    // grab the first one that fits out requirements
    if(NULL != retval) continue;
    if(NULL ==pevent->packet || pevent->packet->dataLength == 0) continue;

    net_packet_startReading(&npkt, pevent->packet);

    if( asynch_list[index].req.packettype == net_packet_peekUint16(&npkt) &&
        0 == memcmp(&pevent->peer->address, &asynch_list[index].req.address, sizeof(ENetAddress)) )
    {
      retval = &(asynch_list[index].req);
      break;
    }
  };

  return retval;
};

//--------------------------------------------------------------------------------------------
PacketRequest_t * net_prepareWaitForPacket(NetAsynchData_t * asynch_list, ENetPeer ** peer, Uint32 timeout, Uint16 packet_type)
{
  PacketRequest_t * prequest;
  Uint8 * pbuffer;
  int index;

  index = NetRequest_getFreeIndex(asynch_list, NETHOST_REQ_COUNT);
  if(-1 == index) return NULL;

  prequest = &(asynch_list[index].req);
  pbuffer  = asynch_list[index].buf;

  prequest->peer       = peer;
  prequest->waiting    = btrue;
  prequest->received   = bfalse;
  prequest->data       = pbuffer;
  prequest->data_size  = NET_REQ_SIZE;
  prequest->packettype = packet_type;
  prequest->expiretimestamp = SDL_GetTicks() + timeout;
  if(NULL == peer)
  {
    prequest->address.host = 0x0100007F; // localhost == 127.0.0.1
    prequest->address.host = 0;          // hmmmm..... we need to choose the right port, but this is a kludge anyway...
  }
  else
  {
    memcpy(&prequest->address, &((*peer)->address), sizeof(ENetAddress));
  }


  return prequest;
};

//--------------------------------------------------------------------------------------------
retval_t net_loopWaitForPacket(PacketRequest_t * prequest, Uint32 granularity, size_t * data_size)
{
  if(prequest->waiting && !prequest->received)
  {
    //if(SDL_GetTicks() >= prequest->expiretimestamp)
    //{
    //  NetRequest_release(prequest);
    //  return rv_fail;
    //}
    //else
    {
      SDL_Delay(granularity);
      return rv_waiting;
    };
  }

  if(NULL !=data_size)
  {
    *data_size = prequest->data_size;
  }
  NetRequest_release(prequest);

  return rv_succeed;
};

//--------------------------------------------------------------------------------------------
retval_t net_waitForPacket(NetAsynchData_t * asynch_list, ENetPeer * peer, Uint32 timeout, Uint16 packet_type, size_t * data_size)
{
  PacketRequest_t * prequest;
  retval_t wait_return;

  // can use &peer, because the stack variable cannot change while we are waiting
  prequest = net_prepareWaitForPacket(asynch_list, &peer, timeout, packet_type);
  if(NULL ==prequest) return rv_error;

  wait_return = net_loopWaitForPacket(prequest, 10, data_size);
  while(rv_waiting == wait_return)
  {
    wait_return = net_loopWaitForPacket(prequest, 10, data_size);
  };

  return wait_return;
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static void net_writeLogMessage(FILE * logFile, const char *format, va_list args)
{
  static char logBuffer[MAX_NET_LOG_MESSAGE];

  if ( NULL != logFile )
  {
    vsnprintf(logBuffer, MAX_NET_LOG_MESSAGE - 1, format, args);
    fprintf(stdout, logBuffer);
    fflush(stdout);

    fputs(logBuffer, logFile);
    fflush(logFile);
  }
}

//--------------------------------------------------------------------------------------------
void net_logf(const char *format, ...)
{
  va_list args;

  va_start(args, format);
  net_writeLogMessage(net_logfile, format, args);
  va_end(args);
}



//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ENetPeer * net_disconnectPeer(ENetPeer * peer, int granularity_ms, int timeout_ms)
{
  int cnt, steps;

  if( NULL ==peer ) return peer;

  if(granularity_ms <= 0) granularity_ms = timeout_ms;
  if(timeout_ms <= granularity_ms) timeout_ms = granularity_ms;

  // request a disconnection
  peer = net_stopPeer(peer);

  // Wait for some time for the disconnect to work
  cnt = 0;
  steps = timeout_ms/granularity_ms;
  while(cnt<steps && ENET_PEER_STATE_DISCONNECTED != (peer)->state)
  {
    SDL_Delay(granularity_ms);
    cnt++;
  }

  if(ENET_PEER_STATE_DISCONNECTED != peer->state)
  {
    // force the thing to reset immediately
    enet_peer_disconnect_now(peer, 0);
  }

  return NULL;
};

//--------------------------------------------------------------------------------------------
char * convert_host(Uint32 host)
{
  static char szAddress[20];

  host = SDL_SwapBE32(host);

  snprintf(szAddress, 20, "%02x.%02x.%02x.%02x", (host>>24) & 0xFF, (host>>16) & 0xFF, (host>> 8) & 0xFF, (host>> 0) & 0xFF);

  return szAddress;
}

//--------------------------------------------------------------------------------------------
ENetPeer * net_startPeer(ENetHost * host, ENetAddress * address )
{
  // BB > start the peer connection process, do not wait for confirmation

  ENetPeer * peer = NULL;

  if(NULL == host || NULL == address)
  {
    net_logf("NET INFO: net_startPeer() - Invalid function call.\n" );
    return peer;
  }

  net_logf("NET INFO: net_startPeer() - Starting connection to connect to %s:0x%04x through %s:0x%04x... ", convert_host(address->host), address->port, convert_host(host->address.host), host->address.port);

  peer = enet_host_connect(host, address, NET_EGOBOO_NUM_CHANNELS);

  // print out some diagnostics
  if (NULL ==peer)
  {
    net_logf("Failed! Cannot open channel.\n");
  }
  else
  {
    switch(peer->state)
    {
      case ENET_PEER_STATE_DISCONNECTED:
        net_logf("Failed! Connection refused?\n");
        break;

      case ENET_PEER_STATE_CONNECTING:
        net_logf("Waiting. Still connecting\n");
        break;

      case ENET_PEER_STATE_ACKNOWLEDGING_CONNECT:
        net_logf("Waiting. Acknowledging connection.\n");
        break;

      case ENET_PEER_STATE_CONNECTED:
        net_logf("Succeeded!\n");
        break;

      case ENET_PEER_STATE_DISCONNECTING:
        net_logf("Failing? Being disconnected.\n");
        break;

      case ENET_PEER_STATE_ACKNOWLEDGING_DISCONNECT:
        net_logf("Failing? Acknowledging disconnection\n");
        break;

      case ENET_PEER_STATE_ZOMBIE:
        net_logf("Failing? Connection timed out.\n");
        break;

      default:
        net_logf("UNKNOWN\n");
        break;
    }
  }


  return peer;
};

//--------------------------------------------------------------------------------------------
ENetPeer * net_startPeerByName(ENetHost * host, const char* connect_name, const int connect_port )
{
  // BB > start the peer connection process, do not wait for confirmation

  ENetAddress address;
  ENetPeer * peer = NULL;

  if(NULL == host || !VALID_CSTR(connect_name) )
  {
    net_logf("NET INFO: net_startPeerByName() - Invalid function call.\n" );
    return peer;
  }

  net_logf("NET INFO: net_startPeerByName() - Attempting to connect to %s:0x%04x through %s:0x%04x\n", connect_name, NET_EGOBOO_SERVER_PORT, convert_host(host->address.host), host->address.port);

  // determine the net address of the conection
  enet_address_set_host(&address, connect_name);
  if(connect_port>=0 && connect_port < UINT16_MAX)
  {
    address.port = connect_port;
  }

  // Now connect to the remote host
  peer = net_startPeer(host, &address);

  if (NULL ==peer)
  {
    net_logf("NET INFO: net_startPeerByName() - Cannot open channel.\n");
  }

  return peer;
};

//--------------------------------------------------------------------------------------------
ENetPeer * net_stopPeer(ENetPeer * peer)
{
  // BB > start the peer disconnection process, do not wait for confirmation

  if(NULL == peer)
  {
    return NULL;
  }

  net_logf("NET INFO: net_stopPeer() - Stopping connection %s:0x%04x... ", convert_host(peer->address.host), peer->address.port);

   enet_peer_disconnect(peer, 0);

  // print out some diagnostics

  switch(peer->state)
  {
    case ENET_PEER_STATE_DISCONNECTED:
      net_logf("Succeeded!\n");
      break;

    case ENET_PEER_STATE_CONNECTING:
      net_logf("Waiting. Still connecting.\n");
      break;

    case ENET_PEER_STATE_ACKNOWLEDGING_CONNECT:
      net_logf("Waiting. Still acknowledging connection?\n");
      break;

    case ENET_PEER_STATE_CONNECTED:
      net_logf("Waiting. Still connected\n");
      break;

    case ENET_PEER_STATE_DISCONNECTING:
      net_logf("Disconnecting\n");
      break;

    case ENET_PEER_STATE_ACKNOWLEDGING_DISCONNECT:
      net_logf("Acknowledging disconnection\n");
      break;

    case ENET_PEER_STATE_ZOMBIE:
      net_logf("Failed? Connection timed out.\n");
      break;

    default:
      net_logf("UNKNOWN\n");
      break;
  }

  return peer;
};


//--------------------------------------------------------------------------------------------
bool_t net_sendSysPacketToAllPeers(ENetHost * host, SYS_PACKET * egop)
{
  // BB> This function wraps the enet_host_broadcast() command
  ENetPacket *packet;

  if(NULL ==host || NULL == egop)
  {
    net_logf("NET ERROR: net_sendSysPacketToAllPeers() - Called with invalid parameters.\n");
    return bfalse;
  }

  net_logf("NET INFO: net_sendSysPacketToAllPeers() - %s from %s:%04x\n", net_crack_sys_packet_type(egop), convert_host(host->address.host), host->address.port);

  if(0 == egop->size)
  {
    net_logf("NET WARN: net_sendSysPacketToAllPeers() - Culling zero length packet.\n");
  }

  packet = enet_packet_create(egop->buffer, egop->size, 0);
  if(NULL != packet)
  {
    enet_host_broadcast(host, NET_UNRELIABLE_CHANNEL, packet);
  }
  else
  {
    net_logf("NET ERROR: net_sendSysPacketToAllPeers() - Unable to generate packet.\n");
  }

  return NULL != packet;
}

//--------------------------------------------------------------------------------------------
bool_t net_sendSysPacketToAllPeersGuaranteed(ENetHost * host, SYS_PACKET * egop)
{
  // BB> This function wraps the enet_host_broadcast() command for the guaranteed channel
  ENetPacket *packet;

  if(NULL ==host || NULL == egop)
  {
    net_logf("NET ERROR: net_sendSysPacketToAllPeersGuaranteed() - Called with invalid parameters.\n");
    return bfalse;
  }

  net_logf("NET INFO: net_sendSysPacketToAllPeersGuaranteed() - %s from %s:%04x\n", net_crack_sys_packet_type(egop), convert_host(host->address.host), host->address.port);

  if(0 == egop->size)
  {
    net_logf("NET WARN: net_sendSysPacketToAllPeersGuaranteed() - Culling zero length packet.\n");
  }

  packet = enet_packet_create(egop->buffer, egop->size, ENET_PACKET_FLAG_RELIABLE);
  if(NULL != packet)
  {
    enet_host_broadcast(host, NET_GUARANTEED_CHANNEL, packet);
  }
  else
  {
    net_logf("NET ERROR: net_sendSysPacketToAllPeersGuaranteed() - Unable to generate packet.\n");
  }


  return NULL != packet;
}

//------------------------------------------------------------------------------
//void net_copyFileToHostOld(Net_t * ns, char *source, char *dest)
//{
//  // ZZ> This function copies a file on the remote to the host computer.
//  //     Packets are sent in chunks of COPYSIZE bytes.  The MAX file size
//  //     that can be sent is 2 Megs ( TOTALSIZE ).
//  FILE* fileread;
//  int packetend, packetstart;
//  int filesize;
//  int fileisdir;
//  char cTmp;
//  SYS_PACKET egopkt;
//
//  net_logf("NET INFO: net_copyFileToHost: \n");
//  fileisdir = fs_fileIsDirectory(source);
//  if (sv_host->nthread.Active)
//  {
//    // Simulate a network transfer
//    if (fileisdir)
//    {
//      net_logf("NET INFO: Creating local directory %s\n", dest);
//      fs_createDirectory(dest);
//    }
//    else
//    {
//      net_logf("NET INFO: Copying local file %s --> %s\n", source, dest);
//      fs_copyFile(source, dest);
//    }
//  }
//  else
//  {
//    if (fileisdir)
//    {
//      net_logf("NET INFO: Creating directory on host: %s\n", dest);
//      net_startNewSysPacket(&egopkt);
//      sys_packet_addUint16(&egopkt, TO_HOST_DIR);
//      sys_packet_addString(&egopkt, dest);
////   sv_sendPacketToAllClientsGuaranteed(ns->parent->sv, &egopkt);
//      Client_sendPacketToHost(ns->parent->cl, &egopkt);
//    }
//    else
//    {
//      net_logf("NET INFO: Copying local file to host file: %s --> %s\n", source, dest);
//      fileread = fopen(source, "rb");
//      if (fileread)
//      {
//        fseek(fileread, 0, SEEK_END);
//        filesize = ftell(fileread);
//        fseek(fileread, 0, SEEK_SET);
//        if (filesize > 0 && filesize < TOTALSIZE)
//        {
//          numfilesent++;
//          packetend = 0;
//          packetstart = 0;
//          net_startNewSysPacket(&egopkt);
//          sys_packet_addUint16(&egopkt, TO_HOST_FILE);
//          sys_packet_addString(&egopkt, dest);
//          sys_packet_addUint32(&egopkt, filesize);
//          sys_packet_addUint32(&egopkt, packetstart);
//          while (packetstart < filesize)
//          {
//            fscanf(fileread, "%c", &cTmp);
//            sys_packet_addUint8(&egopkt, cTmp);
//            packetend++;
//            packetstart++;
//            if (packetend >= COPYSIZE)
//            {
//              // Send off the packet
//              Client_sendPacketToHostGuaranteed(ns->parent->cl, &egopkt);
//              enet_host_flush(cl_host->Host);
//
//              // Start on the next 4K
//              packetend = 0;
//              net_startNewSysPacket(&egopkt);
//              sys_packet_addUint16(&egopkt, TO_HOST_FILE);
//              sys_packet_addString(&egopkt, dest);
//              sys_packet_addUint32(&egopkt, filesize);
//              sys_packet_addUint32(&egopkt, packetstart);
//            }
//          }
//          // Send off the packet
//          Client_sendPacketToHostGuaranteed(ns->parent->cl, &egopkt);
//        }
//        fclose(fileread);
//      }
//    }
//  }
//}
//
//------------------------------------------------------------------------------
//void net_copyFileToAllPlayersOld(Net_t * ns, char *source, char *dest)
//{
//  // ZZ> This function copies a file on the host to every remote computer.
//  //     Packets are sent in chunks of COPYSIZE bytes.  The MAX file size
//  //     that can be sent is 2 Megs ( TOTALSIZE ).
//  FILE* fileread;
//  int packetend, packetstart;
//  int filesize;
//  int fileisdir;
//  char cTmp;
//  SYS_PACKET egopkt;
//
//  if(!_enet_initialized  || !sv_host->nthread.Active || !svStarted(ns->parent->sv)) return;
//
//  net_logf("NET INFO: net_copyFileToAllPeers: %s, %s\n", source, dest);
//
//  fileisdir = fs_fileIsDirectory(source);
//  if (fileisdir)
//  {
//    net_startNewSysPacket(&egopkt);
//    sys_packet_addUint16(&egopkt, TO_REMOTE_DIR);
//    sys_packet_addString(&egopkt, dest);
//    sv_sendPacketToAllClientsGuaranteed(ns->parent->sv, &egopkt);
//  }
//  else
//  {
//    fileread = fopen(source, "rb");
//    if (fileread)
//    {
//      fseek(fileread, 0, SEEK_END);
//      filesize = ftell(fileread);
//      fseek(fileread, 0, SEEK_SET);
//      if (filesize > 0 && filesize < TOTALSIZE)
//      {
//        packetend = 0;
//        packetstart = 0;
//        numfilesent++;
//
//        net_startNewSysPacket(&egopkt);
//        sys_packet_addUint16(&egopkt, TO_REMOTE_FILE);
//        sys_packet_addString(&egopkt, dest);
//        sys_packet_addUint32(&egopkt, filesize);
//        sys_packet_addUint32(&egopkt, packetstart);
//        while (packetstart < filesize)
//        {
//          // This will probably work...
//          //fread((egop->buffer + egop->head), COPYSIZE, 1, fileread);
//
//          // But I'll leave it alone for now
//          fscanf(fileread, "%c", &cTmp);
//
//          sys_packet_addUint8(&egopkt, cTmp);
//          packetend++;
//          packetstart++;
//          if (packetend >= COPYSIZE)
//          {
//            // Send off the packet
//            sv_sendPacketToAllClientsGuaranteed(ns->parent->sv, &egopkt);
//            enet_host_flush(sv_host->Host);
//
//            // Start on the next 4K
//            packetend = 0;
//            net_startNewSysPacket(&egopkt);
//            sys_packet_addUint16(&egopkt, TO_REMOTE_FILE);
//            sys_packet_addString(&egopkt, dest);
//            sys_packet_addUint32(&egopkt, filesize);
//            sys_packet_addUint32(&egopkt, packetstart);
//          }
//        }
//        // Send off the packet
//        sv_sendPacketToAllClientsGuaranteed(ns->parent->sv, &egopkt);
//      }
//      fclose(fileread);
//    }
//  }
//}
//

#define NET_CRACK_TYPE(TYPE) if(header == TYPE) return #TYPE;
char * net_crack_packet_type(Uint16 header)
{
  NET_CRACK_TYPE(TO_ANY_TEXT);

  NET_CRACK_TYPE(TO_HOST_LOGON);
  NET_CRACK_TYPE(TO_HOST_LOGOFF);
  NET_CRACK_TYPE(TO_HOST_REQUEST_MODULE);
  NET_CRACK_TYPE(TO_HOST_MODULE);
  NET_CRACK_TYPE(TO_HOST_MODULEOK);
  NET_CRACK_TYPE(TO_HOST_MODULEBAD);
  NET_CRACK_TYPE(TO_HOST_LATCH);
  NET_CRACK_TYPE(TO_HOST_RTS);
  NET_CRACK_TYPE(TO_HOST_IM_LOADED);
  NET_CRACK_TYPE(TO_HOST_FILE);
  NET_CRACK_TYPE(TO_HOST_DIR);
  NET_CRACK_TYPE(TO_HOST_FILESENT);

  NET_CRACK_TYPE(TO_REMOTE_LOGON);
  NET_CRACK_TYPE(TO_REMOTE_LOGOFF);
  NET_CRACK_TYPE(TO_REMOTE_KICK);
  NET_CRACK_TYPE(TO_REMOTE_MODULE);
  NET_CRACK_TYPE(TO_REMOTE_MODULEBAD);
  NET_CRACK_TYPE(TO_REMOTE_MODULEINFO);
  NET_CRACK_TYPE(TO_REMOTE_LATCH);
  NET_CRACK_TYPE(TO_REMOTE_FILE);
  NET_CRACK_TYPE(TO_REMOTE_DIR);
  NET_CRACK_TYPE(TO_REMOTE_RTS);
  NET_CRACK_TYPE(TO_REMOTE_START);
  NET_CRACK_TYPE(TO_REMOTE_FILE_COUNT);


  NET_CRACK_TYPE(NET_CHECK_CRC);
  NET_CRACK_TYPE(NET_ACKNOWLEDGE_CRC);
  NET_CRACK_TYPE(NET_SEND_CRC);
  NET_CRACK_TYPE(NET_TRANSFER_FILE);
  NET_CRACK_TYPE(NET_REQUEST_FILE);
  NET_CRACK_TYPE(NET_TRANSFER_ACK);
  NET_CRACK_TYPE(NET_CREATE_DIRECTORY);
  NET_CRACK_TYPE(NET_DONE_SENDING_FILES);
  NET_CRACK_TYPE(NET_NUM_FILES_TO_SEND);

  return "UNKNOWN";
}

char * net_crack_enet_packet_type(ENetPacket * enpkt)
{
  Uint16 header;
  STREAM stream;

  stream_startENet(&stream, enpkt);
  header = stream_readUint16(&stream);

  return net_crack_packet_type(header);
}

char * net_crack_sys_packet_type(SYS_PACKET * egop)
{
  Uint16 header;
  STREAM stream;

  stream_startLocal(&stream, egop);
  header = stream_readUint16(&stream);

  return net_crack_packet_type(header);
}

//--------------------------------------------------------------------------------------------
void net_KickOnePlayer(ENetPeer * peer)
{
  SYS_PACKET egopkt;

  // tell the player that he's gone
  net_startNewSysPacket(&egopkt);
  sys_packet_addUint16(&egopkt, TO_REMOTE_KICK);
  net_sendSysPacketToPeerGuaranteed(peer, &egopkt);

  // disconnect him rudely
  enet_peer_reset(peer);
};

//--------------------------------------------------------------------------------------------
void CListOut_close(CListOut_t * co, void * client_data)
{
  int i;

  if(NULL == client_data)
  {
    // close all clients
    for(i=0; i<MAXCONNECTION; i++)
    {
      // reduce the reserences by one
      co->References[i]--;

      // OR close the connections outright?
      // co->References[i] = 0;
    }
  }
  else
  {
    // close one client
    for(i=0; i<co->Count; i++)
    {
      if(client_data == co->Clients[i].client_data)
      {
        // reduce the reserences by one
        co->References[i]--;

        // OR close the connections outright?
        // co->References[i] = 0;
        break;
      }
    }
  }

  CListOut_prune(co);

}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
CListIn_Client_t * CListIn_Client_new(CListIn_Client_t * cli, EventHandler_Ptr_t handler, void * data, Uint32 net_guid)
{
  if(NULL == cli) return cli;

  // get rid of any old stuff
  CListIn_Client_delete(cli);

  // initialize the key
  EKEY_PNEW( cli, CListIn_Client_t );

  if((~(Uint32)0) == net_guid)
  {
    net_guid = net_nextGUID();
  }

  cli->guid         = net_guid;
  cli->handler_ptr  = handler;
  cli->handler_data = data;

  return cli;
}

//--------------------------------------------------------------------------------------------
bool_t CListIn_Client_delete(CListIn_Client_t * cli)
{
  if(NULL == cli) return bfalse;
  if( !EKEY_PVALID(cli) ) return btrue;

  EKEY_PINVALIDATE(cli);

  memset(cli, 0, sizeof(CListIn_Client_t));

  return btrue;
}


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
CListIn_Client_t * CListIn_add_client(CListIn_t * cli, EventHandler_Ptr_t handler, void * data, Uint32 net_guid)
{
  CListIn_Client_t * retval;
  int i, cnt;
  bool_t found;

  if(net_guid == (~(Uint32)0))
  {
    net_guid = net_nextGUID();
  }

  found = bfalse;
  cnt = MIN(cli->Client_count, MAXCONNECTION);
  for(i=0; i<cnt; i++)
  {
    if(cli->ClientLst[i].guid == net_guid)
    {
      found = btrue;
      break;
    }
  }

  if(found)
  {
    retval = cli->ClientLst + i;
  }
  else
  {
    int idx = cli->Client_count;

    retval = cli->ClientLst + idx;
    cli->Client_count++;
    CListIn_Client_new(cli->ClientLst + idx, handler, data, net_guid);
  }

  return retval;
}

//--------------------------------------------------------------------------------------------
bool_t CListIn_remove_client(CListIn_t * cli, Uint32 net_guid)
{
  bool_t found;
  int i, cnt;

  if(NULL == cli) return bfalse;

  // remove only one client
  found = bfalse;
  cnt = MIN(cli->Client_count, MAXCONNECTION);
  for(i=0; i<cnt; i++)
  {
    if(cli->ClientLst[i].guid == net_guid)
    {
      // found a match. remove it.
      memmove(cli->ClientLst + i, cli->ClientLst + cnt - 1, sizeof(CListIn_Client_t));
      cli->Client_count = cnt-1;
      found = btrue;
      break;
    }
  }

  return found;
}

//--------------------------------------------------------------------------------------------
void CListIn_close_client(CListIn_t * cli, void * client_data)
{
  int i, cnt;

  if(NULL == client_data)
  {
    // remove all clients
    memset(cli->ClientLst, 0, MAXCONNECTION*sizeof(CListIn_Client_t));
  }
  else
  {
    // remove only one client
    cnt = MIN(cli->Client_count, MAXCONNECTION);
    for(i=0; i<cnt; i++)
    {
      if(cli->ClientLst[i].handler_data == client_data)
      {
        // found a match. remove it.
        memmove(cli->ClientLst + i, cli->ClientLst + cnt - 1, sizeof(CListIn_Client_t));
        cli->Client_count = cnt-1;
        break;
      }
    }
  }

}


//--------------------------------------------------------------------------------------------
CListIn_Info_t * CListIn_add(CListIn_t * ci, CListIn_Info_t * info)
{
  int cnt;

  CListIn_Info_t * retval = NULL;

  if(NULL == ci || NULL == info) return retval;

  if(ci->Count >= MAXCONNECTION) return retval;

  net_logf("NET INFO: CListIn_add() - Registering connection from \"%s\" (%s:%04x) ... ", info->Hostname, convert_host(info->Address.host), info->Address.port);

  for(cnt=0; cnt<MAXCONNECTION; cnt++)
  {
    if(0 == ci->Info[cnt].Address.host && 0 == ci->Info[cnt].Address.port )
    {
      // found an empty connection
      ci->Count++;

      retval = ci->Info + cnt;

      // set the important player indo
      memcpy(retval, info, sizeof(CListIn_Info_t));

      retval->Slot = cnt;

      net_logf("socket %d", cnt);
      break;
    }
  }
  net_logf("\n", cnt);

  return retval;
}

//--------------------------------------------------------------------------------------------
bool_t CListIn_remove(CListIn_t * ci, ENetPeer * peer)
{
  int slot;
  bool_t retval = bfalse;

  if( NULL != peer->data || ci->Count <= 0 ) return retval;

  slot = *((int*)(peer->data));

  if(slot < MAXCONNECTION)
  {
    net_logf("NET INFO: CListIn_remove() - Connection is closing in socket %d \"%s\" (%s:%04x)\n",
      slot,  ci->Info[slot].Hostname,  convert_host(ci->Info[slot].Address.host), ci->Info[slot].Address.port);

    // found the connection
    ci->Count--;

    ci->Info[slot].Address.host = 0;
    ci->Info[slot].Address.port = 0;
    ci->Info[slot].Hostname[0]  = EOS;
    ci->Info[slot].Name[0]      = EOS;

    retval = btrue;
  }

  return retval;
}

//--------------------------------------------------------------------------------------------
CListIn_Client_t * CListIn_find(CListIn_t * cli, Uint32 id)
{
  CListIn_Client_t * retval = NULL;
  int i, cnt;

  if(NULL == cli || 0 == cli->Client_count) return retval;

  cnt = MIN(cli->Client_count, MAXCONNECTION);
  for(i=0; i<cnt; i++)
  {
    if(id == cli->ClientLst[i].guid)
    {
      retval = cli->ClientLst + i;
      break;
    }
  }

  return retval;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
NetHost_t * NetHost_create(SDL_Callback_Ptr pcall)
{
  NetHost_t * nh;

  if(!CData.request_network) return NULL;

  nh = EGOBOO_NEW( NetHost_t );
  return NetHost_new(nh, pcall);
}

//--------------------------------------------------------------------------------------------
bool_t NetHost_destroy(NetHost_t ** pnh)
{
  bool_t retval;

  if(NULL == pnh || NULL == *pnh || !EKEY_PVALID( (*pnh) ) ) return bfalse;

  retval = NetHost_delete(*pnh);
  EGOBOO_DELETE(*pnh);

  return retval;
}

//--------------------------------------------------------------------------------------------
static NetHost_t * NetHost_new(NetHost_t * nh, SDL_Callback_Ptr pcall)
{
  //fprintf( stdout, "NetHost_new()\n");

  if(NULL == nh) return nh;

  NetHost_delete( nh );

  memset(nh, 0, sizeof(NetHost_t));

  EKEY_PNEW( nh, NetHost_t );

  NetThread_new( &(nh->nthread), pcall);



  return nh;
}

//--------------------------------------------------------------------------------------------
static bool_t NetHost_delete(NetHost_t * nh)
{
 if(NULL == nh) return bfalse;
  if( !EKEY_PVALID(nh) ) return btrue;

  // Close all connections "politely"
  NetHost_close(nh, NULL);

  EKEY_PINVALIDATE(nh);

  // Close the host.
  // Technically, this will close the connections, but it is rude... :P
  if(NULL != nh->Host)
  {
    enet_host_destroy(nh->Host);
    nh->Host = NULL;
  }

  // erase all the info
  memset(nh, 0, sizeof(NetHost_t));

  return btrue;
};


//--------------------------------------------------------------------------------------------
bool_t NetHost_close(NetHost_t * nh, void * client_data)
{
  if(NULL == nh) return bfalse;

  // close the connection lists
  CListOut_close( &(nh->cout), client_data );
  CListIn_close_client( &(nh->cin), client_data );

  return btrue;
}


//--------------------------------------------------------------------------------------------
retval_t NetHost_startUp( NetHost_t * nh, Uint16 port )
{
  ENetAddress address;

  if(!EKEY_PVALID( nh )) return rv_error;

  // need to increment the references even if the state is not active
  nh->references++;

  if(nh->nthread.Active) return rv_succeed;

  if(NULL == nh->Host)
  {
    net_logf("NET INFO: NetHost_startUp() -  Starting host 0x%04x... ", port);

    // Try to create a new sension
    address.host = ENET_HOST_ANY;
    address.port = port;
    nh->Host = enet_host_create(&address, MAXCONNECTION, 0, 0);
    if (NULL == nh->Host)
    {
      net_logf("Failed!\n");
    }
    else
    {
      net_logf("Succeeded!\n");
    }
  }
  else
  {
    net_logf("NET INFO: NetHost_startUp() -  Restarting host on 0x%04x... ", nh->Host->address.port);
  }

  nh->nthread.Active = (NULL != nh->Host);

  return nh->nthread.Active ? rv_succeed : rv_fail;
}



//--------------------------------------------------------------------------------------------
retval_t NetHost_shutDown( NetHost_t * nh )
{
  if(!EKEY_PVALID( nh )) return rv_error;

  // need to decrement the referenced even if the state is not active
  nh->references--;
  if(nh->references<0) nh->references = 0;
  if(nh->references > 0) return rv_succeed;

  if(!nh->nthread.Active) return rv_succeed;

  NetHost_close(nh, NULL);
  nh->nthread.Active = bfalse;

  return !nh->nthread.Active ? rv_succeed : rv_fail;
}


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
NetThread_t * NetThread_create(SDL_Callback_Ptr pcall)
{
  NetThread_t * nt = EGOBOO_NEW( NetThread_t );
  return NetThread_new(nt,pcall);
}

//--------------------------------------------------------------------------------------------
bool_t NetThread_destroy(NetThread_t ** pnt)
{
  bool_t retval;

  if(NULL == pnt || NULL == *pnt) return bfalse;
  if( !EKEY_PVALID( (*pnt) ) ) return btrue;

  retval = NetThread_delete(*pnt);
  EGOBOO_DELETE( *pnt );

  return retval;
}

//--------------------------------------------------------------------------------------------
NetThread_t * NetThread_new(NetThread_t * nt, SDL_Callback_Ptr pcall)
{
  if(NULL == nt) return nt;

  NetThread_delete(nt);

  memset( nt, 0, sizeof(NetThread_t) );

  EKEY_PNEW( nt, NetThread_t );

  // Intial state is "Terminated."
  // Use NetThread_start() to actually start the thread
  nt->Callback   = pcall;
  nt->Active     = bfalse;
  nt->Paused     = btrue;
  nt->Terminated = btrue;

  return nt;
}

//--------------------------------------------------------------------------------------------
bool_t NetThread_delete(NetThread_t * nt)
{
  if(NULL == nt) return bfalse;
  if(!EKEY_PVALID( nt )) return btrue;

  EKEY_PINVALIDATE(nt);

  if(nt->Terminated)
  {
    nt->Thread = NULL;
  }

  if(NULL != nt->Thread)
  {
    SDL_KillThread(nt->Thread);
    nt->Terminated = btrue;
    nt->Thread     = NULL;
  }

  nt->Active = bfalse;
  nt->Paused = bfalse;

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t NetThread_start(NetThread_t * nt)
{
  if(!EKEY_PVALID( nt )) return bfalse;

  // if a callback is specified, start a SDL thread
  if(NULL == nt->Thread && NULL != nt->Callback)
  {
    nt->Thread = SDL_CreateThread(nt->Callback, nt);
  }

  if(NULL != nt->Callback)
  {
    // the NetThread_t is only set to active if the SDL_Thread is active
    nt->Active     = (NULL != nt->Thread);
    nt->Terminated = !nt->Active;
    nt->Paused     = bfalse;
  }
  else
  {
    // This may be a "Ego_Thread" instead of a SDL_Thread. Just set it to active.
    nt->Active     = btrue;
    nt->Terminated = bfalse;
    nt->Paused     = bfalse;
  }

  return nt->Active;
}

//--------------------------------------------------------------------------------------------
bool_t NetThread_stop(NetThread_t * nt)
{
  if(!EKEY_PVALID( nt )) return bfalse;
  if(!NetThread_started(nt)) return bfalse;

  nt->KillMe = btrue;

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t NetThread_pause(NetThread_t * nt)
{
  if(!NetThread_started(nt)) return bfalse;

  nt->Paused = btrue;

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t NetThread_unpause(NetThread_t * nt)
{
  if(!NetThread_started(nt)) return bfalse;

  nt->Paused = bfalse;

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t NetThread_started(NetThread_t * nt)
{
  if(!EKEY_PVALID( nt )) return bfalse;

  if(nt->Terminated) return bfalse;

  return nt->Active;
}


