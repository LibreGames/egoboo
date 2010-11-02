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

/// @file network.c
/// @brief Egoboo networking implementation
/// @details Shuttles bits across the network, using Enet.  Networked play doesn't
/// really work at the moment.

#include "network.h"

#include "file_common.h"
#include "log.h"
#include "input.h"
#include "char.inl"
#include "file_formats/module_file.h"
#include "game.h"
#include "menu.h"

#include "egoboo_strutil.h"
#include "egoboo_vfs.h"
#include "egoboo_setup.h"
#include "egoboo.h"

#include "file_common.h"

#include <stdarg.h>
#include "egoboo_mem.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
static int  numfile = 0;                                // For network copy
static int  numfilesent = 0;                            // For network copy
static int  numfileexpected = 0;                        // For network copy
static int  numplayerrespond = 0;

static net_instance _gnet;

static bool_t net_instance_init( net_instance * pnet );

static void PlaStack_init();
static void PlaStack_reinit();
static void PlaStack_dtor();

static void net_initialize( ego_config_data * pcfg );
static void net_shutDown();
//static void net_logf( const char *format, ... );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
t_cpp_stack< ego_player, MAX_PLAYER  > PlaStack;

FILE *      globalnetworkerr = NULL;

Uint32 sv_last_frame = Uint32( ~0L );

net_shared_stats net_stats;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// Networking constants
enum NetworkConstant
{
    NET_UNRELIABLE_CHANNEL    = 0,
    NET_GUARANTEED_CHANNEL    = 1,
    NET_EGOBOO_NUM_CHANNELS,
    NET_EGOBOO_PORT           = 34626,
    NET_MAX_FILE_NAME         = 128,
    NET_MAX_FILE_TRANSFERS    = 1024  // Maximum files queued up at once
};

/// All the supported network messages
enum NetworkMessage
{
    NET_TRANSFER_FILE       = 10001,  // Packet contains a file.
    NET_TRANSFER_OK         = 10002,  // Acknowledgment packet for a file send
    NET_CREATE_DIRECTORY    = 10003,  // Tell the peer to create the named directory
    NET_DONE_SENDING_FILES  = 10009,  // Sent when there are no more files to send.
    NET_NUM_FILES_TO_SEND   = 10010  // Let the other person know how many files you're sending
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// Packet reading
struct enet_pkt
{
    ENetPacket*    packet;
    size_t         location;

    enet_pkt( ENetPacket * pkt = NULL ) { init( pkt ); }

    void init( ENetPacket * pkt = NULL )
    {
        packet = pkt;
        location = 0;
    }

};

enet_pkt net_read;

struct net_packet
{
    Uint32  head;                             ///< The write head
    Uint32  size;                             ///< The size of the packet
    Uint8   buffer[MAXSENDSIZE];              ///< The data packet

    static void start_new( net_packet * pkt );
    static void add_UnsignedByte( net_packet *, Uint8 uc );
    static void add_SignedByte( net_packet *, Sint8 sc );
    static void add_UnsignedShort( net_packet *, Uint16 us );
    static void add_SignedShort( net_packet *, Sint16 ss );
    static void add_UnsignedInt( net_packet *, Uint32 ui );
    static void add_SignedInt( net_packet *, Sint32 si );
    static void add_String( net_packet *, const char *string );
};

static net_packet tmp_packet;

static void net_sendPacketToHost( net_packet * );
static void net_sendPacketToAllPlayers( net_packet * );
static void net_sendPacketToHostGuaranteed( net_packet * );
static void net_sendPacketToAllPlayersGuaranteed( net_packet * );
static void net_sendPacketToOnePlayerGuaranteed( net_packet *, int player );
static void net_sendPacketToPeer( net_packet *, ENetPeer *peer );
static void net_sendPacketToPeerGuaranteed( net_packet *, ENetPeer *peer );

/// Data for network file transfers
struct NetFileTransfer
{
    char sourceName[NET_MAX_FILE_NAME];
    char destName[NET_MAX_FILE_NAME];
    ENetPeer *target;
};

/// Network file transfer queue
static NetFileTransfer net_transferStates[NET_MAX_FILE_TRANSFERS];
static int net_numFileTransfers = 0;  ///< Queue count
static int net_fileTransferHead = 0;  ///< Queue start index
static int net_fileTransferTail = 0;  ///< Queue end index
static int net_waitingForXferAck = 0; ///< Queue state

static Uint8  * transferBuffer = NULL;
static size_t   transferSize = 0;

// Receiving files
static NetFileTransfer net_receiveState;

static bool_t _network_system_init = bfalse;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void network_system_begin( ego_config_data * pcfg )
{
    if ( !_network_system_init )
    {
        PlaStack_init();
        net_initialize( pcfg );

        _network_system_init = btrue;

        atexit( network_system_end );
    }
}

//--------------------------------------------------------------------------------------------
void network_system_end( void )
{
    if ( _network_system_init )
    {
        PlaStack_dtor();
        net_shutDown();

        _network_system_init = bfalse;
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
const net_instance * network_get_instance()
{
    net_instance * retval = NULL;

    if ( !_network_system_init || !cfg.network_allowed ) return NULL;

    // if it is not initialized, give it one more try
    if ( !_gnet.initialized )
    {
        net_initialize( &ego_cfg );
    }

    if ( _gnet.initialized )
    {
        retval = &_gnet;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t network_initialized()
{
    if ( !_network_system_init ) return bfalse;

    return _gnet.initialized;
}

//--------------------------------------------------------------------------------------------
bool_t network_get_host_active()
{
    if ( !_network_system_init ) return bfalse;

    return _gnet.hostactive;
}

//--------------------------------------------------------------------------------------------
bool_t network_set_host_active( bool_t state )
{
    if ( !_network_system_init ) return bfalse;

    _gnet.hostactive = state;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t network_waiting_for_players()
{
    if ( !_network_system_init || !_gnet.initialized ) return bfalse;

    return _gnet.waitingforplayers;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void close_session()
{
    size_t i, numPeers;
    ENetEvent event;

    /// @details ZZ@> This function gets the computer out of a network game

    if ( _gnet.initialized )
    {
        if ( _gnet.am_host )
        {
            // Disconnect the peers
            numPeers = _gnet.my_host->peerCount;

            for ( i = 0; i < numPeers; i++ )
            {
#if defined(ENET11)
                enet_peer_disconnect( &_gnet.my_host->peers[i], 0 );
#else
                enet_peer_disconnect( &_gnet.my_host->peers[i] );
#endif
            }

            // Allow up to 5 seconds for peers to drop
            while ( enet_host_service( _gnet.my_host, &event, 5000 ) )
            {
                switch ( event.type )
                {
                    case ENET_EVENT_TYPE_RECEIVE:
                        enet_packet_destroy( event.packet );
                        break;

                    case ENET_EVENT_TYPE_DISCONNECT:
                        log_info( "close_session: Peer id %d disconnected gracefully.\n", event.peer->address.host );
                        numPeers--;
                        break;

                    default:
                        break;
                }
            }

            // Forcefully disconnect any peers leftover
            for ( i = 0; i < _gnet.my_host->peerCount; i++ )
            {
                enet_peer_reset( &_gnet.my_host->peers[i] );
            }
        }

        log_info( "close_session: Disconnecting from network.\n" );
        enet_host_destroy( _gnet.my_host );
        _gnet.my_host = NULL;
        _gnet.game_host = NULL;
    }
}

//--------------------------------------------------------------------------------------------
void net_packet::start_new( net_packet * pkt )
{
    /// @details ZZ@> This function starts building a network packet

    pkt->head = 0;
    pkt->size = 0;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void net_packet::add_UnsignedByte( net_packet * pkt, Uint8 uc )
{
    /// @details ZZ@> This function appends an Uint8 to the packet

    Uint8* ucp;
    ucp = ( Uint8* )( &pkt->buffer[pkt->head] );
    *ucp = uc;
    pkt->head += 1;
    pkt->size += 1;
}

//--------------------------------------------------------------------------------------------
void net_packet::add_SignedByte( net_packet * pkt, Sint8 sc )
{
    /// @details ZZ@> This function appends a Sint8 to the packet

    signed char* scp;
    scp = ( signed char* )( &pkt->buffer[pkt->head] );
    *scp = sc;
    pkt->head += 1;
    pkt->size += 1;
}

//--------------------------------------------------------------------------------------------
void net_packet::add_UnsignedShort( net_packet * pkt, Uint16 us )
{
    /// @details ZZ@> This function appends an Uint16 to the packet

    Uint16* usp;
    usp = ( Uint16* )( &pkt->buffer[pkt->head] );

    *usp = ENET_HOST_TO_NET_16( us );
    pkt->head += 2;
    pkt->size += 2;
}

//--------------------------------------------------------------------------------------------
void net_packet::add_SignedShort( net_packet * pkt, Sint16 ss )
{
    /// @details ZZ@> This function appends a Sint16 to the packet

    signed short* ssp;
    ssp = ( signed short* )( &pkt->buffer[pkt->head] );

    *ssp = ENET_HOST_TO_NET_16( ss );

    pkt->head += 2;
    pkt->size += 2;
}

//--------------------------------------------------------------------------------------------
void net_packet::add_UnsignedInt( net_packet * pkt, Uint32 ui )
{
    /// @details ZZ@> This function appends an Uint32 to the packet

    Uint32* uip;
    uip = ( Uint32* )( &pkt->buffer[pkt->head] );

    *uip = ENET_HOST_TO_NET_32( ui );

    pkt->head += 4;
    pkt->size += 4;
}

//--------------------------------------------------------------------------------------------
void net_packet::add_SignedInt( net_packet * pkt, Sint32 si )
{
    /// @details ZZ@> This function appends a Sint32 to the packet

    signed int* sip;
    sip = ( signed int* )( &pkt->buffer[pkt->head] );

    *sip = ENET_HOST_TO_NET_32( si );

    pkt->head += 4;
    pkt->size += 4;
}

//--------------------------------------------------------------------------------------------
void net_packet::add_String( net_packet * pkt, const char *string )
{
    /// @details ZZ@> This function appends a null terminated string to the packet

    char* cp;
    char cTmp;
    int cnt;

    cnt = 0;
    cTmp = 1;
    cp = ( char* )( &pkt->buffer[pkt->head] );

    while ( cTmp != 0 )
    {
        cTmp = string[cnt];
        *cp = cTmp;
        cp += 1;
        pkt->head += 1;
        pkt->size += 1;
        cnt++;
    }
}

//--------------------------------------------------------------------------------------------
void packet_startReading( enet_pkt & pkt, ENetPacket *packet )
{
    pkt.init( packet );
}

//--------------------------------------------------------------------------------------------
void packet_doneReading( enet_pkt & pkt )
{
    pkt.init();
}

//--------------------------------------------------------------------------------------------
void packet_readString( enet_pkt & pkt, char *buffer, int maxLen )
{
    /// @details ZZ@> This function reads a null terminated string from the packet

    Uint8 uc;
    Uint16 outindex;

    outindex = 0;
    uc = pkt.packet->data[pkt.location];
    pkt.location++;

    while ( uc != 0 && outindex < maxLen )
    {
        buffer[outindex] = uc;
        outindex++;
        uc = pkt.packet->data[pkt.location];
        pkt.location++;
    }

    buffer[outindex] = 0;
}

//--------------------------------------------------------------------------------------------
Uint8 packet_readUnsignedByte( enet_pkt & pkt )
{
    /// @details ZZ@> This function reads an Uint8 from the packet

    Uint8 uc;

    uc = ( Uint8 )pkt.packet->data[pkt.location];
    pkt.location++;

    return uc;
}

//--------------------------------------------------------------------------------------------
Sint8 packet_readSignedByte( enet_pkt & pkt )
{
    /// @details ZZ@> This function reads a Sint8 from the packet

    Sint8 sc;

    sc = ( signed char )pkt.packet->data[pkt.location];
    pkt.location++;

    return sc;
}

//--------------------------------------------------------------------------------------------
Uint16 packet_readUnsignedShort( enet_pkt & pkt )
{
    /// @details ZZ@> This function reads an Uint16 from the packet

    Uint16 us;
    Uint16* usp;

    usp = ( Uint16* )( pkt.packet->data + pkt.location );
    us = ENET_NET_TO_HOST_16( *usp );

    pkt.location += 2;

    return us;
}

//--------------------------------------------------------------------------------------------
Sint16 packet_readSignedShort( enet_pkt & pkt )
{
    /// @details ZZ@> This function reads a Sint16 from the packet

    Sint16 ss;
    signed short* ssp;

    ssp = ( signed short* )( pkt.packet->data + pkt.location );

    ss = ENET_NET_TO_HOST_16( *ssp );

    pkt.location += 2;

    return ss;
}

//--------------------------------------------------------------------------------------------
Uint32 packet_readUnsignedInt( enet_pkt & pkt )
{
    /// @details ZZ@> This function reads an Uint32 from the packet

    Uint32 ui;
    Uint32* uip;
    uip = ( Uint32* )( pkt.packet->data + pkt.location );

    ui = ENET_NET_TO_HOST_32( *uip );

    pkt.location += 4;
    return ui;
}

//--------------------------------------------------------------------------------------------
Sint32 packet_readSignedInt( enet_pkt & pkt )
{
    /// @details ZZ@> This function reads a Sint32 from the packet

    Sint32 si;
    signed int* sip;
    sip = ( signed int* )( pkt.packet->data + pkt.location );

    si = ENET_NET_TO_HOST_32( *sip );

    pkt.location += 4;
    return si;
}

//--------------------------------------------------------------------------------------------
size_t packet_remainingSize( enet_pkt & pkt )
{
    /// @details ZZ@> This function tells if there's still data left in the packet

    return pkt.packet->dataLength - pkt.location;
}

//--------------------------------------------------------------------------------------------
void net_sendPacketToHost( net_packet * pkt )
{
    /// @details ZZ@> This function sends a packet to the host

    ENetPacket *packet = enet_packet_create( pkt->buffer, pkt->size, 0 );

    enet_peer_send( _gnet.game_host, NET_UNRELIABLE_CHANNEL, packet );
}

//--------------------------------------------------------------------------------------------
void net_sendPacketToAllPlayers( net_packet * pkt )
{
    /// @details ZZ@> This function sends a packet to all the players

    ENetPacket *packet = enet_packet_create( pkt->buffer, pkt->size, 0 );

    enet_host_broadcast( _gnet.my_host, NET_UNRELIABLE_CHANNEL, packet );
}

//--------------------------------------------------------------------------------------------
void net_sendPacketToHostGuaranteed( net_packet * pkt )
{
    /// @details ZZ@> This function sends a packet to the host

    ENetPacket *packet = enet_packet_create( pkt->buffer, pkt->size, ENET_PACKET_FLAG_RELIABLE );

    enet_peer_send( _gnet.game_host, NET_UNRELIABLE_CHANNEL, packet );
}

//--------------------------------------------------------------------------------------------
void net_sendPacketToAllPlayersGuaranteed( net_packet * pkt )
{
    /// @details ZZ@> This function sends a packet to all the players

    ENetPacket *packet = enet_packet_create( pkt->buffer, pkt->size, ENET_PACKET_FLAG_RELIABLE );
    enet_host_broadcast( _gnet.my_host, NET_GUARANTEED_CHANNEL, packet );
}

//--------------------------------------------------------------------------------------------
void net_sendPacketToOnePlayerGuaranteed( net_packet * pkt, int player )
{
    /// @details ZZ@> This function sends a packet to one of the players

    ENetPacket *packet = enet_packet_create( pkt->buffer, pkt->size, ENET_PACKET_FLAG_RELIABLE );
    if ( player < _gnet.player_count )
    {
        enet_peer_send( &_gnet.my_host->peers[player], NET_GUARANTEED_CHANNEL, packet );
    }
}

//--------------------------------------------------------------------------------------------
void net_sendPacketToPeer( net_packet * pkt, ENetPeer *peer )
{
    /// @details JF@> This function sends a packet to a given peer

    ENetPacket *packet = enet_packet_create( pkt->buffer, pkt->size, ENET_PACKET_FLAG_RELIABLE );

    enet_peer_send( peer, NET_UNRELIABLE_CHANNEL, packet );
}

//--------------------------------------------------------------------------------------------
void net_sendPacketToPeerGuaranteed( net_packet * pkt, ENetPeer *peer )
{
    /// @details JF@> This function sends a packet to a given peer, with guaranteed delivery

    ENetPacket *packet = enet_packet_create( pkt->buffer, pkt->size, 0 );

    enet_peer_send( peer, NET_GUARANTEED_CHANNEL, packet );
}

//--------------------------------------------------------------------------------------------
void net_copyFileToAllPlayers( const char *source, const char *dest )
{
    /// @details JF@> This function queues up files to send to all the hosts.
    ///    @todo Deal with having to send to up to MAX_PLAYER players...

    NetFileTransfer *state;
    if ( net_numFileTransfers < NET_MAX_FILE_TRANSFERS )
    {
        // net_fileTransferTail should already be pointed at an open
        // slot in the queue.
        state = &( net_transferStates[net_fileTransferTail] );
        EGOBOO_ASSERT( state->sourceName[0] == 0 );

        // Just do the first player for now
        state->target = &_gnet.my_host->peers[0];
        strncpy( state->sourceName, source, NET_MAX_FILE_NAME );
        strncpy( state->destName, dest, NET_MAX_FILE_NAME );

        // advance the tail index
        net_numFileTransfers++;
        net_fileTransferTail++;
        if ( net_fileTransferTail >= NET_MAX_FILE_TRANSFERS )
        {
            net_fileTransferTail = 0;
        }
        if ( net_fileTransferTail == net_fileTransferHead )
        {
            log_warning( "net_copyFileToAllPlayers: Warning!  Queue tail caught up with the head!\n" );
        }
    }
}

//--------------------------------------------------------------------------------------------
void net_copyFileToAllPlayersOld_vfs( const char *source, const char *dest )
{
    /// @details ZZ@> This function copies a file on the host to every remote computer.
    ///    Packets are sent in chunks of COPYSIZE bytes.  The max file size
    ///    that can be sent is 2 Megs ( TOTALSIZE ).

    vfs_FILE* fileread;
    int size, start;
    int filesize;
    int fileisdir;
    char cTmp;

    net_packet tmp_packet;

    log_info( "net_copyFileToAllPlayers: %s, %s\n", source, dest );
    if ( _gnet.initialized && _gnet.hostactive )
    {
        fileisdir = vfs_isDirectory( source );
        if ( fileisdir )
        {
            net_packet::start_new( &tmp_packet );
            net_packet::add_UnsignedShort( &tmp_packet, TO_REMOTE_DIR );
            net_packet::add_String( &tmp_packet, dest );
            net_sendPacketToAllPlayersGuaranteed( &tmp_packet );
        }
        else
        {
            fileread = vfs_openReadB( source );
            if ( fileread )
            {
                filesize = vfs_fileLength( fileread );
                vfs_seek( fileread, 0 );
                if ( filesize > 0 && filesize < TOTALSIZE )
                {
                    size = 0;
                    start = 0;
                    numfilesent++;

                    net_packet::start_new( &tmp_packet );
                    net_packet::add_UnsignedShort( &tmp_packet, TO_REMOTE_FILE );
                    net_packet::add_String( &tmp_packet, dest );
                    net_packet::add_UnsignedInt( &tmp_packet, filesize );
                    net_packet::add_UnsignedInt( &tmp_packet, start );

                    while ( start < filesize )
                    {
                        // This will probably work...
                        // vfs_read((packetbuffer + packethead), COPYSIZE, 1, fileread);

                        // But I'll leave it alone for now
                        vfs_scanf( fileread, "%c", &cTmp );

                        net_packet::add_UnsignedByte( &tmp_packet, cTmp );
                        size++;
                        start++;
                        if ( size >= COPYSIZE )
                        {
                            // Send off the packet
                            net_sendPacketToAllPlayersGuaranteed( &tmp_packet );
                            enet_host_flush( _gnet.my_host );

                            // Start on the next 4K
                            size = 0;
                            net_packet::start_new( &tmp_packet );
                            net_packet::add_UnsignedShort( &tmp_packet, TO_REMOTE_FILE );
                            net_packet::add_String( &tmp_packet, dest );
                            net_packet::add_UnsignedInt( &tmp_packet, filesize );
                            net_packet::add_UnsignedInt( &tmp_packet, start );
                        }
                    }

                    // Send off the packet
                    net_sendPacketToAllPlayersGuaranteed( &tmp_packet );
                }

                vfs_close( fileread );
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void net_copyFileToHost( const char *source, const char *dest )
{
    NetFileTransfer *state;

    /// @details JF@> New function merely queues up a new file to be sent

    // If this is the host, just copy the file locally
    if ( _gnet.hostactive )
    {
        // Simulate a network transfer
        if ( vfs_isDirectory( source ) )
        {
            vfs_mkdir( dest );
        }
        else
        {
            vfs_copyFile( source, dest );
        }

        return;
    }
    if ( net_numFileTransfers < NET_MAX_FILE_TRANSFERS )
    {
        // net_fileTransferTail should already be pointed at an open
        // slot in the queue.
        state = &( net_transferStates[net_fileTransferTail] );
        EGOBOO_ASSERT( state->sourceName[0] == 0 );

        state->target = _gnet.game_host;
        strncpy( state->sourceName, source, NET_MAX_FILE_NAME );
        strncpy( state->destName, dest, NET_MAX_FILE_NAME );

        // advance the tail index
        net_numFileTransfers++;
        net_fileTransferTail++;
        if ( net_fileTransferTail >= NET_MAX_FILE_TRANSFERS )
        {
            net_fileTransferTail = 0;
        }
        if ( net_fileTransferTail == net_fileTransferHead )
        {
            log_warning( "net_copyFileToHost: Warning!  Queue tail caught up with the head!\n" );
        }
    }
}

//--------------------------------------------------------------------------------------------
void net_copyFileToHostOld_vfs( const char *source, const char *dest )
{
    /// @details ZZ@> This function copies a file on the remote to the host computer.
    ///    Packets are sent in chunks of COPYSIZE bytes.  The max file size
    ///    that can be sent is 2 Megs ( TOTALSIZE ).

    vfs_FILE* fileread;
    int size, start;
    int filesize;
    int fileisdir;
    char cTmp;

    net_packet tmp_packet;

    log_info( "net_copyFileToHost: " );
    fileisdir = vfs_isDirectory( source );
    if ( _gnet.hostactive )
    {
        // Simulate a network transfer
        if ( fileisdir )
        {
            log_info( "Creating local directory %s\n", dest );
            vfs_mkdir( dest );
        }
        else
        {
            log_info( "Copying local file %s --> %s\n", source, dest );
            vfs_copyFile( source, dest );
        }
    }
    else
    {
        if ( fileisdir )
        {
            log_info( "Creating directory on host: %s\n", dest );
            net_packet::start_new( &tmp_packet );
            net_packet::add_UnsignedShort( &tmp_packet, TO_HOST_DIR );
            net_packet::add_String( &tmp_packet, dest );
//     net_sendPacketToAllPlayersGuaranteed( &tmp_packet);
            net_sendPacketToHost( &tmp_packet );
        }
        else
        {
            log_info( "Copying local file to host file: %s --> %s\n", source, dest );
            fileread = vfs_openReadB( source );
            if ( fileread )
            {
                filesize = vfs_fileLength( fileread );
                vfs_seek( fileread, 0 );
                if ( filesize > 0 && filesize < TOTALSIZE )
                {
                    numfilesent++;
                    size = 0;
                    start = 0;
                    net_packet::start_new( &tmp_packet );
                    net_packet::add_UnsignedShort( &tmp_packet, TO_HOST_FILE );
                    net_packet::add_String( &tmp_packet, dest );
                    net_packet::add_UnsignedInt( &tmp_packet, filesize );
                    net_packet::add_UnsignedInt( &tmp_packet, start );

                    while ( start < filesize )
                    {
                        vfs_scanf( fileread, "%c", &cTmp );
                        net_packet::add_UnsignedByte( &tmp_packet, cTmp );
                        size++;
                        start++;
                        if ( size >= COPYSIZE )
                        {
                            // Send off the packet
                            net_sendPacketToHostGuaranteed( &tmp_packet );
                            enet_host_flush( _gnet.my_host );

                            // Start on the next 4K
                            size = 0;
                            net_packet::start_new( &tmp_packet );
                            net_packet::add_UnsignedShort( &tmp_packet, TO_HOST_FILE );
                            net_packet::add_String( &tmp_packet, dest );
                            net_packet::add_UnsignedInt( &tmp_packet, filesize );
                            net_packet::add_UnsignedInt( &tmp_packet, start );
                        }
                    }

                    // Send off the packet
                    net_sendPacketToHostGuaranteed( &tmp_packet );
                }

                vfs_close( fileread );
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void net_copyDirectoryToHost( const char *dirname, const char *todirname )
{
    /// @details ZZ@> This function copies all files in a directory

    vfs_search_context_t * ctxt;
    const char *searchResult;

    STRING fromname;
    STRING toname;

    log_info( "net_copyDirectoryToHost: %s, %s\n", dirname, todirname );

    // Search for all files
    ctxt = vfs_findFirst( dirname, NULL, VFS_SEARCH_FILE | VFS_SEARCH_BARE );
    searchResult = vfs_search_context_get_current( ctxt );

    if ( NULL != ctxt && VALID_CSTR( searchResult ) )
    {
        // Make the new directory
        net_copyFileToHost( dirname, todirname );

        // Copy each file
        while ( VALID_CSTR( searchResult ) )
        {
            // If a file begins with a dot, assume it's something
            // that we don't want to copy.  This keeps repository
            // directories, /., and /.. from being copied
            // Also avoid copying directories in general.
            SDL_snprintf( fromname, SDL_arraysize( fromname ), "%s/%s", dirname, searchResult );
            if ( '.' == searchResult[0] || vfs_isDirectory( fromname ) )
            {
                ctxt = vfs_findNext( &ctxt );
                searchResult = vfs_search_context_get_current( ctxt );
                continue;
            }

            SDL_snprintf( fromname, SDL_arraysize( fromname ), "%s/%s", dirname, searchResult );
            SDL_snprintf( toname, SDL_arraysize( toname ), "%s/%s", todirname, searchResult );

            net_copyFileToHost( fromname, toname );

            ctxt = vfs_findNext( &ctxt );
            searchResult = vfs_search_context_get_current( ctxt );
        }
    }

    vfs_findClose( &ctxt );
}

//--------------------------------------------------------------------------------------------
void net_copyDirectoryToAllPlayers( const char *dirname, const char *todirname )
{
    /// @details ZZ@> This function copies all files in a directory

    vfs_search_context_t * ctxt;
    const char *searchResult;

    STRING fromname;
    STRING toname;

    log_info( "net_copyDirectoryToAllPlayers: %s, %s\n", dirname, todirname );

    // Search for all files
    ctxt = vfs_findFirst( dirname, NULL, VFS_SEARCH_FILE | VFS_SEARCH_BARE );
    searchResult = vfs_search_context_get_current( ctxt );

    if ( NULL != ctxt && VALID_CSTR( searchResult ) )
    {
        // Make the new directory
        net_copyFileToAllPlayers( dirname, todirname );

        // Copy each file
        while ( VALID_CSTR( searchResult ) )
        {
            // If a file begins with a dot, assume it's something
            // that we don't want to copy.  This keeps repository
            // directories, /., and /.. from being copied
            if ( '.' == searchResult[0] )
            {
                ctxt = vfs_findNext( &ctxt );
                searchResult = vfs_search_context_get_current( ctxt );

                continue;
            }

            SDL_snprintf( fromname, SDL_arraysize( fromname ), "%s/%s", dirname, searchResult );
            SDL_snprintf( toname, SDL_arraysize( toname ), "%s/%s", todirname, searchResult );
            net_copyFileToAllPlayers( fromname, toname );

            ctxt = vfs_findNext( &ctxt );
            searchResult = vfs_search_context_get_current( ctxt );
        }
    }

    vfs_findClose( &ctxt );
}

//--------------------------------------------------------------------------------------------
void net_sayHello()
{
    /// @details ZZ@> This function lets everyone know we're here

    if ( !_gnet.initialized )
    {
        _gnet.waitingforplayers = bfalse;
    }
    else if ( _gnet.hostactive )
    {
        log_info( "net_sayHello: Server saying hello.\n" );
        net_stats.pla_count_loaded++;
        if ( net_stats.pla_count_loaded >= _gnet.player_count )
        {
            _gnet.waitingforplayers = bfalse;
        }
    }
    else
    {
        log_info( "net_sayHello: Client saying hello.\n" );
        net_packet::start_new( &tmp_packet );
        net_packet::add_UnsignedShort( &tmp_packet, TO_HOST_IM_LOADED );
        net_sendPacketToHostGuaranteed( &tmp_packet );
    }
}

//--------------------------------------------------------------------------------------------
void cl_talkToHost()
{
    /// @details ZZ@> This function sends the latch packets to the host machine

    PLA_REF player;

    // Let the players respawn
    if ( SDLKEYDOWN( SDLK_SPACE )
         && (( 0 == net_stats.pla_count_total_alive ) || PMod->respawnanytime )
         && PMod->respawnvalid
         && cfg.difficulty < GAME_HARD
         && !console_mode )
    {
        for ( player = 0; player < MAX_PLAYER; player++ )
        {
            ego_player * ppla = PlaStack + player;
            if ( !ppla->valid ) continue;

            if ( INPUT_BITS_NONE != ppla->device.bits )
            {
                ADD_BITS( ppla->local_latch.b, LATCHBUTTON_RESPAWN );  // Press the respawn button...
            }
        }
    }

    // Start talkin'
    if ( _gnet.initialized && !_gnet.hostactive /*&& !PMod->rtscontrol*/ )
    {
        net_packet::start_new( &tmp_packet );
        net_packet::add_UnsignedShort( &tmp_packet, TO_HOST_LATCH );        // The message header

        for ( player = 0; player < MAX_PLAYER; player++ )
        {
            ego_player * ppla = PlaStack + player;
            if ( !ppla->valid ) continue;

            // Find the local players
            if ( INPUT_BITS_NONE != ppla->device.bits )
            {
                net_packet::add_UnsignedByte( &tmp_packet, ( player ).get_value() );                     // The player index

                net_packet::add_SignedShort( &tmp_packet, ppla->local_latch.raw[kX]*LATCH_TO_FFFF );  // Raw control value
                net_packet::add_SignedShort( &tmp_packet, ppla->local_latch.raw[kY]*LATCH_TO_FFFF );  // Raw control value

                net_packet::add_SignedShort( &tmp_packet, ppla->local_latch.dir[kX]*LATCH_TO_FFFF );  // Player motion
                net_packet::add_SignedShort( &tmp_packet, ppla->local_latch.dir[kY]*LATCH_TO_FFFF );  // Player motion
                net_packet::add_SignedShort( &tmp_packet, ppla->local_latch.dir[kZ]*LATCH_TO_FFFF );  // Player motion

                net_packet::add_UnsignedInt( &tmp_packet, ppla->local_latch.b );             // Player button states
            }
        }

        // Send it to the host
        net_sendPacketToHost( &tmp_packet );
    }
}

//--------------------------------------------------------------------------------------------
void sv_talkToRemotes()
{
    /// @details ZZ@> This function sends the character data to all the remote machines

    PLA_REF player;
    int time;

    // make sure there is only one update per frame;
    if ( update_wld == sv_last_frame ) return;
    sv_last_frame = update_wld;

    if ( _gnet.hostactive )
    {
        if ( _gnet.initialized )
        {
            time = true_update + net_stats.lag_tolerance;

            // Send a message to all players
            net_packet::start_new( &tmp_packet );
            net_packet::add_UnsignedShort( &tmp_packet, TO_REMOTE_LATCH );                       // The message header
            net_packet::add_UnsignedInt( &tmp_packet, time );                                  // The stamp

            // Send all player latches...
            for ( player = 0; player < MAX_PLAYER; player++ )
            {
                ego_player * ppla = PlaStack + player;

                if ( !ppla->valid ) continue;

                net_packet::add_UnsignedByte( &tmp_packet, ( player ).get_value() );                     // The player index

                net_packet::add_SignedShort( &tmp_packet, ppla->local_latch.raw[kX]*LATCH_TO_FFFF );  // Player motion
                net_packet::add_SignedShort( &tmp_packet, ppla->local_latch.raw[kY]*LATCH_TO_FFFF );  // Player motion

                net_packet::add_SignedShort( &tmp_packet, ppla->local_latch.dir[kX]*LATCH_TO_FFFF );  // Player motion
                net_packet::add_SignedShort( &tmp_packet, ppla->local_latch.dir[kY]*LATCH_TO_FFFF );  // Player motion
                net_packet::add_SignedShort( &tmp_packet, ppla->local_latch.dir[kZ]*LATCH_TO_FFFF );  // Player motion

                net_packet::add_UnsignedInt( &tmp_packet, ppla->local_latch.b );        // Player button states
            }

            // Send the packet
            net_sendPacketToAllPlayers( &tmp_packet );
        }
        else
        {
            time = true_update + 1;
        }

        // update the local timed latches with the same info
        _gnet.timed_latch_count = 0;
        for ( player = 0; player < MAX_PLAYER; player++ )
        {
            int index;
            Uint32 cnt;
            ego_player * ppla = PlaStack + player;

            if ( !ppla->valid ) continue;

            index = ppla->tlatch_count;
            if ( index < MAXLAG )
            {
                ego_time_latch * ptlatch = ppla->tlatch + index;

                ptlatch->button = ppla->local_latch.b;

                // reduce the resolution of the motion to match the network packets
                ptlatch->raw[kX] = floor( ppla->local_latch.raw[kX] * LATCH_TO_FFFF ) * FFFF_TO_LATCH;
                ptlatch->raw[kY] = floor( ppla->local_latch.raw[kY] * LATCH_TO_FFFF ) * FFFF_TO_LATCH;

                ptlatch->dir[kX] = floor( ppla->local_latch.dir[kX] * LATCH_TO_FFFF ) * FFFF_TO_LATCH;
                ptlatch->dir[kY] = floor( ppla->local_latch.dir[kY] * LATCH_TO_FFFF ) * FFFF_TO_LATCH;
                ptlatch->dir[kZ] = floor( ppla->local_latch.dir[kZ] * LATCH_TO_FFFF ) * FFFF_TO_LATCH;

                ptlatch->time = true_update;

                ppla->tlatch_count++;
            }

            // determine the max amount of lag
            for ( cnt = 0; cnt < ppla->tlatch_count; cnt++ )
            {
                int loc_lag = update_wld - ppla->tlatch[index].time  + 1;

                if ( loc_lag > 0 && ( Uint32 )loc_lag > _gnet.timed_latch_count )
                {
                    _gnet.timed_latch_count = loc_lag;
                }
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void pla_add_tlatch( const PLA_REF & iplayer, Uint32 time, latch_input_t net_latch )
{
    ego_player * ppla;

    if ( !VALID_PLA( iplayer ) ) return;
    ppla = PlaStack + iplayer;

    if ( ppla->tlatch_count >= MAXLAG ) return;

    ppla->tlatch[ ppla->tlatch_count ].raw[kX] = net_latch.raw[kX];
    ppla->tlatch[ ppla->tlatch_count ].raw[kY] = net_latch.raw[kY];

    ppla->tlatch[ ppla->tlatch_count ].dir[kX] = net_latch.dir[kX];
    ppla->tlatch[ ppla->tlatch_count ].dir[kY] = net_latch.dir[kY];
    ppla->tlatch[ ppla->tlatch_count ].dir[kZ] = net_latch.dir[kZ];

    ppla->tlatch[ ppla->tlatch_count ].button  = net_latch.b;
    ppla->tlatch[ ppla->tlatch_count ].time    = time;

    ppla->tlatch_count++;
}

//--------------------------------------------------------------------------------------------
void net_handlePacket( ENetEvent *event )
{
    Uint16 header;
    STRING filename;      // also used for reading various strings
    int filesize, newfilesize, fileposition;
    char newfile;
    PLA_REF player;
    Uint32 stamp;
    int time;
    vfs_FILE *file;
    size_t fileSize;

    log_info( "net_handlePacket: Received " );

    packet_startReading( net_read,  event->packet );
    header = packet_readUnsignedShort( net_read );

    switch ( header )
    {
        case TO_ANY_TEXT:
            log_info( "TO_ANY_TEXT\n" );
            packet_readString( net_read, filename, 255 );
            debug_printf( filename );
            break;

        case TO_HOST_MODULEOK:
            log_info( "TO_HOSTMODULEOK\n" );
            if ( _gnet.hostactive )
            {
                net_stats.pla_count_ready++;
                if ( net_stats.pla_count_ready >= _gnet.player_count )
                {
                    _gnet.readytostart = btrue;
                }
            }
            break;

        case TO_HOST_LATCH:
            log_info( "TO_HOST_LATCH\n" );
            if ( _gnet.hostactive )
            {
                while ( packet_remainingSize( net_read ) > 0 )
                {
                    latch_input_t tmp_latch;

                    player = packet_readUnsignedByte( net_read );
                    time   = packet_readUnsignedInt( net_read );

                    tmp_latch.raw[kX] = packet_readSignedShort( net_read ) * FFFF_TO_LATCH;
                    tmp_latch.raw[kY] = packet_readSignedShort( net_read ) * FFFF_TO_LATCH;

                    tmp_latch.dir[kX] = packet_readSignedShort( net_read ) * FFFF_TO_LATCH;
                    tmp_latch.dir[kY] = packet_readSignedShort( net_read ) * FFFF_TO_LATCH;
                    tmp_latch.dir[kZ] = packet_readSignedShort( net_read ) * FFFF_TO_LATCH;

                    tmp_latch.b       = packet_readUnsignedInt( net_read );

                    pla_add_tlatch( player, time, tmp_latch );
                }

            }
            break;

        case TO_HOST_IM_LOADED:
            log_info( "TO_HOST_IMLOADED\n" );
            if ( _gnet.hostactive )
            {
                net_stats.pla_count_loaded++;
                if ( net_stats.pla_count_loaded == _gnet.player_count )
                {
                    // Let the games begin...
                    _gnet.waitingforplayers = bfalse;
                    net_packet::start_new( &tmp_packet );
                    net_packet::add_UnsignedShort( &tmp_packet, TO_REMOTE_START );
                    net_sendPacketToAllPlayersGuaranteed( &tmp_packet );
                }
            }
            break;

        case TO_HOST_RTS:
            log_info( "TO_HOST_RTS\n" );
            if ( _gnet.hostactive )
            {
                /*whichorder = get_empty_order();
                if(whichorder < MAXORDER)
                  {
                  // Add the order on the host machine
                  cnt = 0;
                  while(cnt < MAXSELECT)
                  {
                    who = packet_readUnsignedByte( net_read );
                    orderwho[whichorder][cnt] = who;
                    cnt++;
                  }
                  what = packet_readUnsignedInt( net_read );
                  when = update_wld + orderlag;
                  orderwhat[whichorder] = what;
                  orderwhen[whichorder] = when;

                  // Send the order off to everyone else
                  net_packet::start_new( &tmp_packet );
                  net_packet::add_UnsignedShort( &tmp_packet,TO_REMOTE_RTS);
                  cnt = 0;
                  while(cnt < MAXSELECT)
                  {
                    net_packet::add_UnsignedByte( &tmp_packet,orderwho[whichorder][cnt]);
                    cnt++;
                  }
                  net_packet::add_UnsignedInt( &tmp_packet,what);
                  net_packet::add_UnsignedInt( &tmp_packet,when);
                  net_sendPacketToAllPlayersGuaranteed( &tmp_packet);
                  }*/
            }
            break;

        case NET_TRANSFER_FILE:
            packet_readString( net_read, filename, 256 );
            fileSize = packet_readUnsignedInt( net_read );

            log_info( "NET_TRANSFER_FILE: %s with size %d.\n", filename, fileSize );

            // Try and save the file
            file = vfs_openWriteB( filename );
            if ( NULL != file )
            {
                vfs_write( net_read.packet->data + net_read.location, 1, fileSize, file );
                vfs_close( file );
            }
            else
            {
                log_warning( "net_handlePacket: Couldn't write new file!\n" );
            }

            // Acknowledge that we got this file
            net_packet::start_new( &tmp_packet );
            net_packet::add_UnsignedShort( &tmp_packet, NET_TRANSFER_OK );
            net_sendPacketToPeer( &tmp_packet, event->peer );

            // And note that we've gotten another one
            numfile++;
            break;

        case NET_TRANSFER_OK:
            log_info( "NET_TRANSFER_OK. The last file sent was successful.\n" );
            net_waitingForXferAck = 0;
            net_numFileTransfers--;

            break;

        case NET_CREATE_DIRECTORY:
            packet_readString( net_read, filename, 256 );
            log_info( "NET_CREATE_DIRECTORY: %s\n", filename );

            vfs_mkdir( filename );

            // Acknowledge that we got this file
            net_packet::start_new( &tmp_packet );
            net_packet::add_UnsignedShort( &tmp_packet, NET_TRANSFER_OK );
            net_sendPacketToPeer( &tmp_packet, event->peer );

            numfile++;  // The client considers directories it sends to be files, so ya.
            break;

        case NET_DONE_SENDING_FILES:
            log_info( "NET_DONE_SENDING_FILES\n" );
            numplayerrespond++;
            break;

        case NET_NUM_FILES_TO_SEND:
            log_info( "NET_NUM_FILES_TO_SEND\n" );
            numfileexpected = ( int )packet_readUnsignedShort( net_read );
            break;

        case TO_HOST_FILE:
            log_info( "TO_HOST_FILE\n" );
            packet_readString( net_read, filename, 255 );
            newfilesize = packet_readUnsignedInt( net_read );

            // Change the size of the file if need be
            newfile = 0;
            file = vfs_openReadB( filename );
            if ( file )
            {
                filesize = vfs_fileLength( file );
                vfs_close( file );
                if ( filesize != newfilesize )
                {
                    // Destroy the old file
                    newfile = 1;
                }
            }
            else
            {
                newfile = 1;
            }
            if ( newfile )
            {
                // file must be created.  Write zeroes to the file to do it
                numfile++;
                file = vfs_openWriteB( filename );
                if ( file )
                {
                    filesize = 0;

                    while ( filesize < newfilesize )
                    {
                        vfs_putc( 0, file );
                        filesize++;
                    }

                    vfs_close( file );
                }
            }

            // Go to the position in the file and copy data
            fileposition = packet_readUnsignedInt( net_read );
            file = vfs_openReadB( filename );
            if ( file )
            {
                if ( vfs_seek( file, fileposition ) == 0 )
                {
                    while ( packet_remainingSize( net_read ) > 0 )
                    {
                        vfs_putc( packet_readUnsignedByte( net_read ), file );
                    }
                }

                vfs_close( file );
            }
            break;

        case TO_HOST_DIR:
            log_info( "TO_HOST_DIR\n" );
            if ( _gnet.hostactive )
            {
                packet_readString( net_read, filename, 255 );
                vfs_mkdir( filename );
            }
            break;

        case TO_HOST_FILESENT:
            log_info( "TO_HOST_FILESENT\n" );
            if ( _gnet.hostactive )
            {
                numfileexpected += packet_readUnsignedInt( net_read );
                numplayerrespond++;
            }
            break;

        case TO_REMOTE_FILESENT:
            log_info( "TO_REMOTE_FILESENT\n" );
            if ( !_gnet.hostactive )
            {
                numfileexpected += packet_readUnsignedInt( net_read );
                numplayerrespond++;
            }
            break;

        case TO_REMOTE_MODULE:
            log_info( "TO_REMOTE_MODULE\n" );
            if ( !_gnet.hostactive && !_gnet.readytostart )
            {
                PMod->seed = packet_readUnsignedInt( net_read );
                packet_readString( net_read, filename, 255 );

                pickedmodule.init( mnu_get_mod_number( filename ) );

                // Check to see if the module exists
                if ( -1 != pickedmodule.index )
                {
                    strncpy( pickedmodule.path,       mnu_ModList_get_vfs_path( pickedmodule.index ), SDL_arraysize( pickedmodule.path ) );
                    strncpy( pickedmodule.name,       mnu_ModList_get_name( pickedmodule.index ), SDL_arraysize( pickedmodule.name ) );
                    strncpy( pickedmodule.write_path, mnu_ModList_get_dest_path( pickedmodule.index ), SDL_arraysize( pickedmodule.write_path ) );

                    pickedmodule.ready = btrue;

                    // Make ourselves ready
                    _gnet.readytostart = btrue;

                    // Tell the host we're ready
                    net_packet::start_new( &tmp_packet );
                    net_packet::add_UnsignedShort( &tmp_packet, TO_HOST_MODULEOK );
                    net_sendPacketToHostGuaranteed( &tmp_packet );
                }
                else
                {
                    // The module doesn't exist locally
                    pickedmodule.ready = bfalse;

                    // Halt the process
                    _gnet.readytostart = bfalse;

                    // Tell the host we're not ready
                    net_packet::start_new( &tmp_packet );
                    net_packet::add_UnsignedShort( &tmp_packet, TO_HOST_MODULEBAD );
                    net_sendPacketToHostGuaranteed( &tmp_packet );
                }
            }
            break;

        case TO_REMOTE_START:
            log_info( "TO_REMOTE_START\n" );
            if ( !_gnet.hostactive )
            {
                _gnet.waitingforplayers = bfalse;
            }
            break;

        case TO_REMOTE_RTS:
            log_info( "TO_REMOTE_RTS\n" );
            if ( !_gnet.hostactive )
            {
                /*    whichorder = get_empty_order();
                    if(whichorder < MAXORDER)
                    {
                      // Add the order on the remote machine
                      cnt = 0;
                      while(cnt < MAXSELECT)
                      {
                        who = packet_readUnsignedByte( net_read );
                        orderwho[whichorder][cnt] = who;
                        cnt++;
                      }
                      what = packet_readUnsignedInt( net_read );
                      when = packet_readUnsignedInt( net_read );
                      orderwhat[whichorder] = what;
                      orderwhen[whichorder] = when;
                    }*/
            }
            break;

        case TO_REMOTE_FILE:
            log_info( "TO_REMOTE_FILE\n" );
            if ( !_gnet.hostactive )
            {
                packet_readString( net_read, filename, 255 );
                newfilesize = packet_readUnsignedInt( net_read );

                // Change the size of the file if need be
                newfile = 0;
                file = vfs_openReadB( filename );
                if ( file )
                {
                    filesize = vfs_fileLength( file );
                    vfs_close( file );
                    if ( filesize != newfilesize )
                    {
                        // Destroy the old file
                        newfile = 1;
                    }
                }
                else
                {
                    newfile = 1;
                }
                if ( newfile )
                {
                    // file must be created.  Write zeroes to the file to do it
                    numfile++;
                    file = vfs_openWriteB( filename );
                    if ( file )
                    {
                        filesize = 0;

                        while ( filesize < newfilesize )
                        {
                            vfs_putc( 0, file );
                            filesize++;
                        }

                        vfs_close( file );
                    }
                }

                // Go to the position in the file and copy data
                fileposition = packet_readUnsignedInt( net_read );
                file = vfs_openReadB( filename );
                if ( file )
                {
                    if ( vfs_seek( file, fileposition ) == 0 )
                    {
                        while ( packet_remainingSize( net_read ) > 0 )
                        {
                            vfs_putc( packet_readUnsignedByte( net_read ), file );
                        }
                    }

                    vfs_close( file );
                }
            }
            break;

        case TO_REMOTE_DIR:
            log_info( "TO_REMOTE_DIR\n" );
            if ( !_gnet.hostactive )
            {
                packet_readString( net_read, filename, 255 );
                vfs_mkdir( filename );
            }
            break;

        case TO_REMOTE_LATCH:
            log_info( "TO_REMOTE_LATCH\n" );
            if ( !_gnet.hostactive )
            {
                stamp = packet_readUnsignedInt( net_read );
                time = stamp & LAGAND;
                if ((( Uint32 )( ~0 ) ) == _gnet.next_time_stamp )
                {
                    _gnet.next_time_stamp = stamp;
                }
                if ( stamp < _gnet.next_time_stamp )
                {
                    log_warning( "net_handlePacket: OUT OF ORDER PACKET\n" );
                    net_stats.out_of_sync = btrue;
                }
                if ( stamp <= update_wld )
                {
                    log_warning( "net_handlePacket: LATE PACKET\n" );
                    net_stats.out_of_sync = btrue;
                }
                if ( stamp > _gnet.next_time_stamp )
                {
                    log_warning( "net_handlePacket: MISSED PACKET\n" );
                    _gnet.next_time_stamp = stamp;  // Still use it
                    net_stats.out_of_sync = btrue;
                }
                if ( stamp == _gnet.next_time_stamp )
                {
                    // Remember that we got it
                    _gnet.timed_latch_count++;

                    // Read latches for each player sent
                    while ( packet_remainingSize( net_read ) > 0 )
                    {
                        player = packet_readUnsignedByte( net_read );
                        if ( VALID_PLA( player ) )
                        {
                            ego_player * ppla = PlaStack + player;

                            ppla->tlatch[time].raw[kX] = packet_readSignedShort( net_read ) * FFFF_TO_LATCH;
                            ppla->tlatch[time].raw[kY] = packet_readSignedShort( net_read ) * FFFF_TO_LATCH;

                            ppla->tlatch[time].dir[kX] = packet_readSignedShort( net_read ) * FFFF_TO_LATCH;
                            ppla->tlatch[time].dir[kY] = packet_readSignedShort( net_read ) * FFFF_TO_LATCH;
                            ppla->tlatch[time].dir[kZ] = packet_readSignedShort( net_read ) * FFFF_TO_LATCH;

                            ppla->tlatch[time].button  = packet_readUnsignedInt( net_read );
                        }
                        else
                        {
                            // dump the data
                            packet_readSignedShort( net_read );
                            packet_readSignedShort( net_read );
                            packet_readSignedShort( net_read );
                            packet_readSignedShort( net_read );
                            packet_readUnsignedInt( net_read );
                        }
                    }

                    _gnet.next_time_stamp = stamp + 1;
                }
            }
            break;
    }
}

//--------------------------------------------------------------------------------------------
void net_dispatch_packets()
{
    /// @details ZZ@> This function reads any new messages and sets the player latch and matrix needed
    ///    lists...

    ENetEvent event;
    if ( _gnet.initialized )
    {
        // Listen for new messages
        while ( enet_host_service( _gnet.my_host, &event, 0 ) != 0 )
        {
            switch ( event.type )
            {
                case ENET_EVENT_TYPE_RECEIVE:
                    net_handlePacket( &event );
                    enet_packet_destroy( event.packet );
                    break;

                case ENET_EVENT_TYPE_CONNECT:
                    // don't allow anyone to connect during the game session
                    log_warning( "net_dispatch_packets: Client tried to connect during the game: %x:%u\n",
                                 event.peer->address.host, event.peer->address.port );
#if defined(ENET11)
                    enet_peer_disconnect( event.peer, 0 );
#else
                    enet_peer_disconnect( event.peer );
#endif
                    break;

                case ENET_EVENT_TYPE_DISCONNECT:

                    // Is this a player disconnecting, or just a rejected connection
                    // from above?
                    if ( event.peer->data != 0 )
                    {
                        NetPlayerInfo *info = ( NetPlayerInfo * )event.peer->data;

                        // uh oh, how do we handle losing a player?
                        log_warning( "net_dispatch_packets: Player %d disconnected!\n",
                                     info->which_slot );
                    }
                    break;

                default:
                    break;
            }
        }
    }
}

//--------------------------------------------------------------------------------------------
void unbuffer_one_player_latch_do_network( ego_player * ppla )
{
    // get the "network" latch for each valid player

    Uint32 latch_count, tnc;
    latch_input_t tmp_latch;
    ego_time_latch * tlatch_list;

    if ( NULL == ppla ) return;

    tlatch_list = ppla->tlatch;

    // copy the latch from last time
    tmp_latch = ppla->net_latch;

    // what are the minimum and maximum indices that can be applies this update?
    for ( tnc = 0; tnc < ppla->tlatch_count; tnc++ )
    {
        int dt;

        dt = update_wld - tlatch_list[tnc].time;

        if ( dt < 0 )
            break;
    }
    latch_count = tnc;

    if ( latch_count == 1 )
    {
        // there is just one valid latch
        tmp_latch.raw[kX] = tlatch_list[0].raw[kX];
        tmp_latch.raw[kY] = tlatch_list[0].raw[kY];

        tmp_latch.dir[kX] = tlatch_list[0].dir[kX];
        tmp_latch.dir[kY] = tlatch_list[0].dir[kY];
        tmp_latch.dir[kZ] = tlatch_list[0].dir[kZ];

        tmp_latch.b       = tlatch_list[0].button;

        //log_info( "<<%1.4f, %1.4f>, 0x%x>, Just one latch for %s\n", tmp_latch.x, tmp_latch.y, tmp_latch.b, ChrObjList.get_data_ref(ppla->index).name );
    }
    else if ( latch_count > 1 )
    {
        int weight, weight_sum;
        int dt;

        // estimate the best latch value by weighting latches that are back in time
        // by dt*dt. This estimates the effect of actually integrating the position over
        // that much time without the hastle of actually integrating the trajectory.

        // blank the current latch so that we can sum the latch values
        latch_input_init( &( tmp_latch ) );

        // apply the latch
        weight_sum = 0;
        for ( tnc = 0; tnc < latch_count; tnc++ )
        {
            dt = update_wld - tlatch_list[tnc].time;

            weight      = ( dt + 1 ) * ( dt + 1 );

            weight_sum  += weight;

            tmp_latch.raw[kX] += tlatch_list[tnc].raw[kX] * weight;
            tmp_latch.raw[kY] += tlatch_list[tnc].raw[kY] * weight;

            tmp_latch.dir[kX] += tlatch_list[tnc].dir[kX] * weight;
            tmp_latch.dir[kY] += tlatch_list[tnc].dir[kY] * weight;
            tmp_latch.dir[kZ] += tlatch_list[tnc].dir[kZ] * weight;

            ADD_BITS( tmp_latch.b, tlatch_list[tnc].button );
        }

        _gnet.timed_latch_count = SDL_max( _gnet.timed_latch_count, latch_count );
        if ( weight_sum > 0.0f )
        {
            tmp_latch.raw[kX] /= ( float )weight_sum;
            tmp_latch.raw[kY] /= ( float )weight_sum;

            tmp_latch.dir[kX] /= ( float )weight_sum;
            tmp_latch.dir[kY] /= ( float )weight_sum;
            tmp_latch.dir[kZ] /= ( float )weight_sum;
        }

        //log_info( "<<%1.4f, %1.4f>, 0x%x>, %d, multiple latches for %s\n", tmp_latch.x, tmp_latch.y, tmp_latch.b, latch_count, ChrObjList.get_data_ref(ppla->index).name );
    }
    else
    {
        // there are no valid latches
        // do nothing. this lets the old value of the latch persist.
        // this might be a decent guess as to what to do if a packet was
        // dropped?
        //log_info( "<<%1.4f, %1.4f>, 0x%x>, latch dead reckoning for %s\n", tmp_latch.x, tmp_latch.y, tmp_latch.b, ChrObjList.get_data_ref(ppla->index).name );
    }

    if ( latch_count >= ppla->tlatch_count )
    {
        // we have emptied all of the latches
        ppla->tlatch_count = 0;
    }
    else if ( latch_count > 0 )
    {
        int index;

        // concatenate the list
        for ( tnc = latch_count, index = 0; tnc < ppla->tlatch_count; tnc++, index++ )
        {
            tlatch_list[index].raw[kX] = tlatch_list[tnc].raw[kX];
            tlatch_list[index].raw[kY] = tlatch_list[tnc].raw[kY];

            tlatch_list[index].dir[kX] = tlatch_list[tnc].dir[kX];
            tlatch_list[index].dir[kY] = tlatch_list[tnc].dir[kY];
            tlatch_list[index].dir[kZ] = tlatch_list[tnc].dir[kZ];

            tlatch_list[index].button  = tlatch_list[tnc].button;
            tlatch_list[index].time    = tlatch_list[tnc].time;
        }
        ppla->tlatch_count = index;
    }

    // fix the network latch
    ppla->net_latch = tmp_latch;
}

//--------------------------------------------------------------------------------------------
void unbuffer_one_player_latch_download( ego_player * ppla )
{
    ego_chr * pchr;

    if ( NULL == ppla ) return;

    if ( !INGAME_CHR( ppla->index ) ) return;
    pchr = ChrObjList.get_data_ptr( ppla->index );

    pchr->latch.raw_valid = btrue;
    pchr->latch.raw.dir[kX] = ppla->net_latch.raw[kX];
    pchr->latch.raw.dir[kY] = ppla->net_latch.raw[kY];
    pchr->latch.raw.b       = ppla->net_latch.b;

    pchr->latch.trans_valid = btrue;
    pchr->latch.trans.dir[kX] = ppla->net_latch.dir[kX];
    pchr->latch.trans.dir[kY] = ppla->net_latch.dir[kY];
    pchr->latch.trans.dir[kZ] = ppla->net_latch.dir[kZ];
    pchr->latch.trans.b       = ppla->net_latch.b;
}

//--------------------------------------------------------------------------------------------
void unbuffer_one_player_latch_do_respawn( ego_player * ppla )
{
    ego_chr * pchr;

    if ( NULL == ppla ) return;

    if ( !INGAME_CHR( ppla->index ) ) return;
    pchr = ChrObjList.get_data_ptr( ppla->index );

    if ( cfg.difficulty < GAME_HARD && HAS_SOME_BITS( pchr->latch.trans.b, LATCHBUTTON_RESPAWN ) && PMod->respawnvalid )
    {
        if ( !pchr->alive && 0 == timer_revive )
        {
            ego_chr::respawn( ppla->index );
            TeamStack[pchr->team].leader = ppla->index;
            ADD_BITS( pchr->ai.alert, ALERTIF_CLEANEDUP );

            // cost some experience for doing this...  never lose a level
            pchr->experience *= EXPKEEP;
            if ( cfg.difficulty > GAME_EASY ) pchr->money *= EXPKEEP;
        }

        // remove all latches other than latchbutton_respawn
        REMOVE_BITS( pchr->latch.trans.b, LATCHBUTTON_RESPAWN );
    }
}

//--------------------------------------------------------------------------------------------
void unbuffer_all_player_latches()
{
    /// @details ZZ@> This function sets character latches based on player input to the host

    PLA_REF ipla;

    // if ( PMod->rtscontrol ) { _gnet.timed_latch_count--; return; }

    _gnet.timed_latch_count = 0;
    for ( ipla = 0; ipla < MAX_PLAYER; ipla++ )
    {
        ego_player * ppla = PlaStack + ipla;
        if ( !ppla->valid ) continue;

        unbuffer_one_player_latch_do_network( ppla );
    }

    // set the player latch
    for ( ipla = 0; ipla < MAX_PLAYER; ipla++ )
    {
        ego_player * ppla = PlaStack + ipla;
        if ( !ppla->valid ) continue;

        unbuffer_one_player_latch_download( ppla );
    }

    // Let players respawn
    for ( ipla = 0; ipla < MAX_PLAYER; ipla++ )
    {
        ego_player * ppla = PlaStack + ipla;
        if ( !ppla->valid ) continue;

        unbuffer_one_player_latch_do_respawn( ppla );
    }
}

//--------------------------------------------------------------------------------------------
void net_initialize( ego_config_data * pcfg )
{
    /// @details ZZ@> This starts up the network and logs whatever goes on

    // assume the worst
    _gnet.session_count = 0;
    _gnet.service_count = 0;

    net_instance_init( &_gnet );

    // Clear all the state variables to 0 to start.
    SDL_memset( _gnet.player_peers, 0, sizeof( _gnet.player_peers ) );
    SDL_memset( _gnet.player_info, 0, sizeof( _gnet.player_info ) );
    SDL_memset( tmp_packet.buffer, 0, sizeof( tmp_packet.buffer ) );
    SDL_memset( net_transferStates, 0, sizeof( net_transferStates ) );
    SDL_memset( &net_receiveState, 0, sizeof( net_receiveState ) );

    sv_last_frame = Uint32( ~0L );

    if ( NULL != pcfg && pcfg->cfg_ptr()->network_allowed )
    {
        // initialize enet
        log_info( "net_initialize: Initializing enet... " );
        if ( 0 != enet_initialize() )
        {
            log_info( "Failure!\n" );

            _gnet.initialized = bfalse;
            _gnet.serviceon = 0;
        }
        else
        {
            log_info( "Success!\n" );

            _gnet.initialized = btrue;
            _gnet.serviceon = btrue;
            _gnet.service_count = 1;
        }
    }
    else
    {
        // We're not doing networking this time...
        log_info( "net_initialize: Networking not enabled.\n" );
    }
}

//--------------------------------------------------------------------------------------------
void net_shutDown()
{
    if ( !_gnet.initialized ) return;

    log_info( "net_shutDown: Turning off networking.\n" );
    enet_deinitialize();
}

//--------------------------------------------------------------------------------------------
void find_open_sessions()
{
    /*PORT - if we are connected to a host, ask it for the open sessions?

    /// @details ZZ@> This function finds some open games to join

    DPSESSIONDESC2      sessionDesc;
    HRESULT             hr;
    if(_gnet.initialized)
      {
    _gnet.session_count = 0;
    if(globalnetworkerr)  vfs_printf(globalnetworkerr, "  Looking for open games...\n");
    ZeroMemory(&sessionDesc, sizeof(DPSESSIONDESC2));
    sessionDesc.dwSize = sizeof(DPSESSIONDESC2);
    sessionDesc.guidApplication = NETWORKID;
    hr = lpDirectPlay3A->EnumSessions(&sessionDesc, 0, SessionsCallback, hGlobalWindow, DPENUMSESSIONS_AVAILABLE);
    if(globalnetworkerr)  vfs_printf(globalnetworkerr, "    %d sessions found\n", _gnet.session_count);
      }
    */
}

//--------------------------------------------------------------------------------------------
void sv_letPlayersJoin()
{
    /// @details ZZ@> This function finds all the players in the game

    ENetEvent event;
    STRING hostName;

    // Check all pending events for players joining
    while ( enet_host_service( _gnet.my_host, &event, 0 ) > 0 )
    {
        switch ( event.type )
        {
            case ENET_EVENT_TYPE_CONNECT:
                // Look up the hostname the player is connecting from
                enet_address_get_host( &event.peer->address, hostName, 64 );

                log_info( "sv_letPlayersJoin: A new player connected from %s:%u\n",
                          hostName, event.peer->address.port );

                // save the player data here.
                enet_address_get_host( &event.peer->address, hostName, 64 );
                strncpy( _gnet.player_name[_gnet.player_count], hostName, 16 );

                event.peer->data = &( _gnet.player_info[_gnet.player_count] );
                _gnet.player_count++;
                break;

            case ENET_EVENT_TYPE_RECEIVE:
                log_info( "sv_letPlayersJoin: Recieved a packet when we weren't expecting it...\n" );
                log_info( "\tIt came from %x:%u\n", event.peer->address.host, event.peer->address.port );

                // clean up the packet
                enet_packet_destroy( event.packet );
                break;

            case ENET_EVENT_TYPE_DISCONNECT:
                log_info( "sv_letPlayersJoin: A client disconnected!  Address %x:%u\n",
                          event.peer->address.host, event.peer->address.port );

                // Reset that peer's data
                event.peer->data = NULL;
                break;

            default:
                break;
        }
    }
}

//--------------------------------------------------------------------------------------------
int cl_joinGame( const char* hostname )
{
    /// @details ZZ@> This function tries to join one of the sessions we found

    ENetAddress address;
    ENetEvent event;
    if ( _gnet.initialized )
    {
        log_info( "cl_joinGame: Creating client network connection... " );
        // Create my host thingamabober
        /// @todo Should I limit client bandwidth here?
        _gnet.my_host = enet_host_create( NULL, 1, 0, 0 );
        if ( NULL == _gnet.my_host )
        {
            // can't create a network connection at all
            log_info( "Failure!\n" );
            return bfalse;
        }
        else
        {
            log_info( "Success!\n" );
        };

        // Now connect to the remote host
        log_info( "cl_joinGame: Attempting to connect to %s:%d\n", hostname, NET_EGOBOO_PORT );
        enet_address_set_host( &address, hostname );
        address.port = NET_EGOBOO_PORT;
        _gnet.game_host = enet_host_connect( _gnet.my_host, &address, NET_EGOBOO_NUM_CHANNELS );
        if ( NULL == _gnet.game_host )
        {
            log_info( "cl_joinGame: No available peers to create a connection!\n" );
            return bfalse;
        }

        // Wait for up to 5 seconds for the connection attempt to succeed
        if ( enet_host_service( _gnet.my_host, &event, 5000 ) > 0 &&
             event.type == ENET_EVENT_TYPE_CONNECT )
        {
            log_info( "cl_joinGame: Connected to %s:%d\n", hostname, NET_EGOBOO_PORT );
            return btrue;
            // return create_player(bfalse);
        }
        else
        {
            log_info( "cl_joinGame: Could not connect to %s:%d!\n", hostname, NET_EGOBOO_PORT );
        }
    }

    return bfalse;
}

//--------------------------------------------------------------------------------------------
void stop_players_from_joining()
{
    /// @details ZZ@> This function stops players from joining a game
}

//--------------------------------------------------------------------------------------------
int sv_hostGame()
{
    /// @details ZZ@> This function tries to host a new session

    ENetAddress address;
    if ( _gnet.initialized )
    {
        // Try to create a new session
        address.host = ENET_HOST_ANY;
        address.port = NET_EGOBOO_PORT;

        log_info( "sv_hostGame: Creating game on port %d\n", NET_EGOBOO_PORT );
        _gnet.my_host = enet_host_create( &address, MAX_PLAYER, 0, 0 );
        if ( NULL == _gnet.my_host )
        {
            log_info( "sv_hostGame: Could not create network connection!\n" );
            return bfalse;
        }

        // Try to create a host player
        //   return create_player(btrue);
        _gnet.am_host = btrue;

        // Moved from net_sayHello because there they cause a race issue
        _gnet.waitingforplayers = btrue;
        net_stats.pla_count_loaded = 0;
    }

    // Run in solo mode
    return btrue;
}

//--------------------------------------------------------------------------------------------
/*void turn_on_service( int service )
{
    /// ZZ@> This function turns on a network service ( IPX, TCP, serial, modem )
}

*/

//--------------------------------------------------------------------------------------------
int  net_pendingFileTransfers()
{
    return net_numFileTransfers;
}

//--------------------------------------------------------------------------------------------
void net_updateFileTransfers()
{
    NetFileTransfer *state;
    ENetPacket *packet;
    size_t nameLen, fileSize;
    Uint32 networkSize;
    vfs_FILE *file;
    char *p;

    // Are there any pending file sends?
    if ( net_numFileTransfers > 0 )
    {
        if ( !net_waitingForXferAck )
        {
            state = &net_transferStates[net_fileTransferHead];

            // Check and see if this is a directory, instead of a file
            if ( vfs_isDirectory( state->sourceName ) )
            {
                // Tell the target to create a directory
                log_info( "net_updateFileTranfers: Creating directory %s on target\n", state->destName );
                net_packet::start_new( &tmp_packet );
                net_packet::add_UnsignedShort( &tmp_packet, NET_CREATE_DIRECTORY );
                net_packet::add_String( &tmp_packet, state->destName );
                net_sendPacketToPeerGuaranteed( &tmp_packet, state->target );

                net_waitingForXferAck = 1;
            }
            else
            {
                file = vfs_openReadB( state->sourceName );
                if ( file )
                {
                    log_info( "net_updateFileTransfers: Attempting to send %s to %s\n", state->sourceName, state->destName );

                    fileSize = vfs_fileLength( file );
                    vfs_seek( file, 0 );

                    // Make room for the file's name
                    nameLen = SDL_strlen( state->destName ) + 1;
                    transferSize = nameLen;

                    // And for the file's size
                    transferSize += 6;  // Uint32 size, and Uint16 message type
                    transferSize += fileSize;

                    transferBuffer = EGOBOO_NEW_ARY( Uint8, transferSize );
                    *( Uint16* )transferBuffer = ENET_HOST_TO_NET_16( NET_TRANSFER_FILE );

                    // Add the string and file length to the buffer
                    p = ( char * )( transferBuffer + 2 );
                    strcpy( p, state->destName );
                    p += nameLen;

                    networkSize = ENET_HOST_TO_NET_32(( Uint32 )fileSize );
                    *( size_t* )p = networkSize;
                    p += 4;

                    vfs_read( p, 1, fileSize, file );
                    vfs_close( file );

                    packet = enet_packet_create( transferBuffer, transferSize, ENET_PACKET_FLAG_RELIABLE );
                    enet_peer_send( state->target, NET_GUARANTEED_CHANNEL, packet );

                    EGOBOO_DELETE_ARY( transferBuffer );
                    transferSize = 0;

                    net_waitingForXferAck = 1;
                }
                else
                {
                    log_warning( "net_updateFileTransfers: Could not open file %s to send it!\n", state->sourceName );
                }
            }

            // update transfer queue state
            SDL_memset( state, 0, sizeof( *state ) );
            net_fileTransferHead++;
            if ( net_fileTransferHead >= NET_MAX_FILE_TRANSFERS )
            {
                net_fileTransferHead = 0;
            }

        } // end if waiting for ack
    } // end if net_numFileTransfers > 0

    // Let the recieve loop run at least once
    net_dispatch_packets();
}

//--------------------------------------------------------------------------------------------
void net_send_message()
{
    /// @details ZZ@> sends the message in the keyboard buffer to all other players

    if ( !_gnet.initialized ) return;

    if ( console_mode || !console_done ) return;

    net_packet::start_new( &tmp_packet );
    net_packet::add_UnsignedShort( &tmp_packet, TO_ANY_TEXT );
    net_packet::add_String( &tmp_packet, keyb.buffer );
    net_sendPacketToAllPlayers( &tmp_packet );
}

//--------------------------------------------------------------------------------------------
bool_t net_instance_init( net_instance * pnet )
{
    if ( NULL == pnet ) return bfalse;

    SDL_memset( pnet, 0, sizeof( *pnet ) );

    // make sure to update the network player count
    net_stats.pla_count_network = _gnet.player_count;

    return bfalse;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
CHR_REF ego_player::get_ichr( const PLA_REF & iplayer )
{
    ego_player * pplayer;

    if ( !VALID_PLA( iplayer ) ) return CHR_REF( MAX_CHR );
    pplayer = PlaStack + iplayer;

    if ( !INGAME_CHR( pplayer->index ) ) return CHR_REF( MAX_CHR );

    return pplayer->index;
}

//--------------------------------------------------------------------------------------------
ego_player* net_get_ppla( const CHR_REF & ichr )
{
    PLA_REF iplayer;

    if ( !INGAME_CHR( ichr ) ) return NULL;
    iplayer = ChrObjList.get_data_ref( ichr ).is_which_player;

    if ( !VALID_PLA( iplayer ) ) return NULL;

    return PlaStack + iplayer;
}

//--------------------------------------------------------------------------------------------
ego_chr  * pla_get_pchr( const PLA_REF & iplayer )
{
    ego_player * pplayer;

    if ( !VALID_PLA( iplayer ) ) return NULL;
    pplayer = PlaStack + iplayer;

    if ( !INGAME_CHR( pplayer->index ) ) return NULL;

    return ChrObjList.get_data_ptr( pplayer->index );
}

//--------------------------------------------------------------------------------------------
latch_2d_t ego_player::convert_latch_2d( const PLA_REF & iplayer, const latch_2d_t & src )
{
    latch_2d_t dst = LATCH_2D_INIT;
    ego_player * ppla;

    if ( !VALID_PLA( iplayer ) ) return dst;
    ppla = PlaStack + iplayer;

    // is there a valid character?
    if ( !DEFINED_CHR( ppla->index ) ) return dst;

    return ego_chr::convert_latch_2d( ChrObjList.get_data_ptr( ppla->index ), src );
}

//--------------------------------------------------------------------------------------------
void net_reset_players()
{
    PlaStack_reinit();

    _gnet.next_time_stamp = ( Uint32( ~0L ) );
    _gnet.timed_latch_count   = 0;
}

//--------------------------------------------------------------------------------------------
void tlatch_ary_init( ego_time_latch ary[], size_t len )
{
    size_t cnt;

    if ( NULL == ary || 0 == len ) return;

    for ( cnt = 0; cnt < len; cnt++ )
    {
        ary[cnt].raw[kX] = 0.0f;
        ary[cnt].raw[kY] = 0.0f;
        ary[cnt].dir[kX] = 0.0f;
        ary[cnt].dir[kY] = 0.0f;
        ary[cnt].dir[kZ] = 0.0f;
        ary[cnt].button  = 0;
        ary[cnt].time    = ( Uint32 )( ~0 );
    }
}

//--------------------------------------------------------------------------------------------
ego_player * ego_player::reinit( ego_player * ppla )
{
    if ( NULL == ppla ) return ppla;

    if ( ppla->valid )
    {
        ppla = ego_player::dtor( ppla );
    }

    return ego_player::ctor( ppla );
}

//--------------------------------------------------------------------------------------------
ego_player * ego_player::dtor( ego_player * ppla )
{
    if ( NULL == ppla ) return ppla;

    //---- skeleton for using a ConfigFile to save quests
    // close any ConfigFile
    // ConfigFile_destroy( &(ppla->quest_file) );

    // turn off the player
    ppla->valid       = bfalse;
    ppla->index       = CHR_REF( MAX_CHR );
    ppla->device.bits = INPUT_BITS_NONE;

    return ppla;
}

//--------------------------------------------------------------------------------------------
ego_player * ego_player::ctor( ego_player * ppla )
{
    if ( NULL == ppla ) return ppla;

    SDL_memset( ppla, 0, sizeof( *ppla ) );

    ppla->index       = CHR_REF( MAX_CHR );

    // initialize the device
    input_device_init( &( ppla->device ) );

    // initialize the latches
    latch_input_init( &( ppla->local_latch ) );
    latch_input_init( &( ppla->net_latch ) );

    // initialize the tlatch array
    tlatch_ary_init( ppla->tlatch, MAXLAG );

    return ppla;
}

//--------------------------------------------------------------------------------------------
// Sustain old movements to smooth play
void input_device_add_latch( ego_input_device * pdevice, latch_input_t latch )
{
    float dist;

    if ( NULL == pdevice ) return;

    pdevice->latch_old = pdevice->latch;

    pdevice->latch.raw[kX] = pdevice->latch_old.raw[kX] * pdevice->sustain + latch.raw[kX] * pdevice->cover;
    pdevice->latch.raw[kY] = pdevice->latch_old.raw[kY] * pdevice->sustain + latch.raw[kY] * pdevice->cover;

    dist = pdevice->latch.raw[kX] * pdevice->latch.raw[kX] + pdevice->latch.raw[kY] * pdevice->latch.raw[kY];
    if ( dist > 1.0f )
    {
        float scale = 1.0f / SQRT( dist );

        pdevice->latch.raw[kX] *= scale;
        pdevice->latch.raw[kY] *= scale;
    }

    pdevice->latch.dir[kX] = pdevice->latch_old.dir[kX] * pdevice->sustain + latch.dir[kX] * pdevice->cover;
    pdevice->latch.dir[kY] = pdevice->latch_old.dir[kY] * pdevice->sustain + latch.dir[kY] * pdevice->cover;
    pdevice->latch.dir[kZ] = pdevice->latch_old.dir[kZ] * pdevice->sustain + latch.dir[kZ] * pdevice->cover;

    // make sure that the latch never overflows
    dist = pdevice->latch.dir[kX] * pdevice->latch.dir[kX] + pdevice->latch.dir[kY] * pdevice->latch.dir[kY] + pdevice->latch.dir[kZ] * pdevice->latch.dir[kZ];
    if ( dist > 1.0f )
    {
        float scale = 1.0f / SQRT( dist );

        pdevice->latch.dir[kX] *= scale;
        pdevice->latch.dir[kY] *= scale;
        pdevice->latch.dir[kZ] *= scale;
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void PlaStack_init()
{
    // construct the player stack

    PLA_REF ipla;

    for ( ipla = 0; ipla < MAX_PLAYER; ipla++ )
    {
        ego_player::ctor( PlaStack + ipla );
    }
    PlaStack.count = 0;
}

//--------------------------------------------------------------------------------------------
void PlaStack_dtor()
{
    // deconstruct the player stack

    PLA_REF ipla;

    for ( ipla = 0; ipla < MAX_PLAYER; ipla++ )
    {
        ego_player::dtor( PlaStack + ipla );
    }
    PlaStack.count = 0;
}

//--------------------------------------------------------------------------------------------
void PlaStack_reinit()
{
    // deconstruct the player stack

    PLA_REF ipla;

    for ( ipla = 0; ipla < MAX_PLAYER; ipla++ )
    {
        ego_player::reinit( PlaStack + ipla );
    }
    PlaStack.count = 0;
}

//--------------------------------------------------------------------------------------------
void PlaStack_toggle_all_explore()
{
    PLA_REF ipla;

    for ( ipla = 0; ipla < MAX_PLAYER; ipla++ )
    {
        ego_player * ppla = PlaStack + ipla;

        if ( !ppla->valid ) continue;

        ppla->explore_mode = !ppla->explore_mode;
    }
}

//--------------------------------------------------------------------------------------------
void PlaStack_toggle_all_wizard()
{
    PLA_REF ipla;

    for ( ipla = 0; ipla < MAX_PLAYER; ipla++ )
    {
        ego_player * ppla = PlaStack + ipla;

        if ( !ppla->valid ) continue;

        ppla->wizard_mode = !ppla->wizard_mode;
    }
}

//--------------------------------------------------------------------------------------------
bool_t PlaStack_has_explore()
{
    bool_t  retval = bfalse;
    PLA_REF ipla;

    for ( ipla = 0; ipla < MAX_PLAYER; ipla++ )
    {
        ego_player * ppla = PlaStack + ipla;

        if ( !ppla->valid ) continue;

        if ( ppla->explore_mode )
        {
            retval = btrue;
            break;
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t PlaStack_has_wizard()
{
    bool_t  retval = bfalse;
    PLA_REF ipla;

    for ( ipla = 0; ipla < MAX_PLAYER; ipla++ )
    {
        ego_player * ppla = PlaStack + ipla;

        if ( !ppla->valid ) continue;

        if ( ppla->wizard_mode )
        {
            retval = btrue;
            break;
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

void net_synchronize_game_clock()
{
    if ( !network_initialized() ) return;

    if ( 0 == _gnet.timed_latch_count )
    {
        // The remote ran out of messages, and is now twiddling its thumbs...
        // Make it go slower so it doesn't happen again
        clock_wld += 25;
    }

    if ( _gnet.timed_latch_count > 3 && !network_get_host_active() )
    {
        // The host has too many messages, and is probably experiencing control
        // lag...  Speed it up so it gets closer to sync
        clock_wld -= 5;
    }
}