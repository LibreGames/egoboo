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

/// @file network.h
/// @brief Skeleton for Egoboo networking

#include "IDSZ_map.h"
#include "file_formats/configfile.h"

#include "egoboo_typedef.h"
#include "egoboo_setup.h"
#include "egoboo.h"

#include "enet/enet.h"

//--------------------------------------------------------------------------------------------
// Network stuff
//--------------------------------------------------------------------------------------------

#define NETREFRESH          1000                    ///< Every second
#define NONETWORK           _gnet.service_count
#define MAXSERVICE          16
#define NETNAMESIZE         16
#define MAXSESSION          16
#define MAXNETPLAYER         8

#define TO_ANY_TEXT         25935                               ///< Message headers
#define TO_HOST_MODULEOK    14951
#define TO_HOST_MODULEBAD   14952
#define TO_HOST_LATCH       33911
#define TO_HOST_RTS         30376
#define TO_HOST_IM_LOADED   40192
#define TO_HOST_FILE        20482
#define TO_HOST_DIR         49230
#define TO_HOST_FILESENT    13131
#define TO_REMOTE_MODULE    56025
#define TO_REMOTE_LATCH     12715
#define TO_REMOTE_FILE      62198
#define TO_REMOTE_DIR       11034
#define TO_REMOTE_RTS        5143
#define TO_REMOTE_START     51390
#define TO_REMOTE_FILESENT  19903

#define LATCH_TO_FFFF 1024.0f
#define FFFF_TO_LATCH 0.0009765625f

#define CHARVEL 5.0f
#define MAXSENDSIZE 8192
#define COPYSIZE    4096
#define TOTALSIZE   2097152
#define MAX_PLAYER   8                               ///< 2 to a power...  2^3
#define MAXLAG      64
#define LAGAND      63
#define STARTTALK   10

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct net_shared_stats
{
    int pla_count_local;             ///< Number of local players
    int pla_count_network;               ///< Number of network players
    int pla_count_total;             ///< Number of total players

    int pla_count_ready;               ///< Number of players ready to start
    int pla_count_loaded;

    int pla_count_local_alive;
    int pla_count_network_alive;
    int pla_count_total_alive;

    int pla_count_local_dead;
    int pla_count_network_dead;
    int pla_count_total_dead;

    int    lag_tolerance;                 ///< Lag tolerance
    bool_t out_of_sync;

    net_shared_stats() { init(); }

    void init()
    {
        pla_count_local = 0;
        pla_count_network = 0;
        pla_count_total = 0;

        pla_count_local_alive = 0;
        pla_count_network_alive = 0;
        pla_count_total_alive = 0;

        pla_count_local_dead = 0;
        pla_count_network_dead = 0;
        pla_count_total_dead = 0;

        pla_count_ready = 0;
        pla_count_loaded = 0;

        lag_tolerance     = 3;
        out_of_sync       = bfalse;
    }
};

extern net_shared_stats net_stats;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// A latch with a time attached
/// @details This is recieved over the network, or inserted into the list by the local system to simulate
///  network traffic
struct ego_time_latch
{
    float     raw[2];

    float     dir[3];
    BIT_FIELD button;

    Uint32    time;
};

//--------------------------------------------------------------------------------------------
struct ego_input_device
{
    bool_t                  on;              ///< Is it alive?
    BIT_FIELD               bits;

    float                   sustain;         ///< Falloff rate for old movement
    float                   cover;           ///< For falloff

    latch_input_t           latch;
    latch_input_t           latch_old;       ///< For sustain
};

void input_device_init( ego_input_device * pdevice );
void input_device_add_latch( ego_input_device * pdevice, latch_input_t latch );

//--------------------------------------------------------------------------------------------
#define INVALID_PLAYER MAX_PLAYER

/// The state of a player
struct ego_player
{
    bool_t                  valid;                    ///< Player used?
    CHR_REF                 index;                    ///< Which character?

    /// special play modes from nethack
    bool_t wizard_mode;
    bool_t explore_mode;

    /// the buffered input from the local input devices
    ego_input_device          device;

    /// Local latch, set by read_player_local_latch(), read by sv_talkToRemotes()
    latch_input_t           local_latch;

    // quest log for this player
    IDSZ_node_t             quest_log[MAX_IDSZ_MAP_SIZE];          ///< lists all the character's quests

    //---- skeleton for using a ConfigFile to save quests
    //ConfigFilePtr_t         quest_file;

    // Timed latches
    Uint32                  tlatch_count;
    ego_time_latch            tlatch[MAXLAG];

    /// Network latch, set by unbuffer_all_player_latches(), used to set the local character's latch
    latch_input_t           net_latch;

    static ego_player * ctor( ego_player * ppla );
    static ego_player * dtor( ego_player * ppla );
    static ego_player * reinit( ego_player * ppla );
    static CHR_REF      get_ichr( const PLA_REF & iplayer );
    static latch_2d_t   convert_latch_2d( const PLA_REF & iplayer, const latch_2d_t & src );
};

extern t_stack< ego_player, MAX_PLAYER > PlaStack;                         ///< Stack for keeping track of players

#define VALID_PLA(IPLA)       ( PlaStack.in_range_ref(IPLA) && ((IPLA) < PlaStack.count) && PlaStack[IPLA].valid )
#define INVALID_PLA(IPLA)     ( !PlaStack.in_range_ref(IPLA) || ((IPLA) >= PlaStack.count)|| !PlaStack[IPLA].valid )

struct ego_chr * pla_get_pchr( const PLA_REF & iplayer );
ego_player*      net_get_ppla( const CHR_REF & ichr );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// Network information on connected players
struct NetPlayerInfo
{
    int which_slot;
};

//--------------------------------------------------------------------------------------------

/// The state of the network code
struct net_instance
{
    bool_t  initialized;             ///< Is Networking initialized?

    //---- old network stuff
    bool_t  serviceon;               ///< Do I need to free the interface?
    bool_t  hostactive;              ///< Hosting?
    bool_t  readytostart;            ///< Ready to hit the Start Game button?
    bool_t  waitingforplayers;       ///< Has everyone talked to the host?

    int     machine_type;           ///< 0 is host, 1 is 1st remote, 2 is 2nd...

    // information about the available services
    int     service_which;                             ///< which service are we using
    int     service_count;                                 ///< How many we found
    char    service_name[MAXSERVICE][NETNAMESIZE];    ///< Names of services

    // information about available sessions
    int     session_count;                                 ///< How many we found
    char    session_name[MAXSESSION][NETNAMESIZE];    ///< Names of sessions

    // which players are logged on to our machine
    int     player_count;                                  ///< How many we found
    char    player_name[MAXNETPLAYER][NETNAMESIZE];   ///< Names of machines

    Uint32  timed_latch_count;

    Uint32  next_time_stamp;                ///< Expected timestamp

    //---- new network stuff
    bool_t          am_host;
    ENetHost      * my_host;
    ENetPeer      * game_host;
    ENetPeer      * player_peers[MAX_PLAYER];
    NetPlayerInfo   player_info[MAXNETPLAYER];

    net_instance()
    {
        initialized        = bfalse;
        serviceon          = bfalse;
        hostactive         = bfalse;
        readytostart       = bfalse;
        waitingforplayers  = bfalse;

        machine_type  = 0;

        service_which = -1;
        service_count = 0;
        session_count = 0;
        player_count  = 0;

        timed_latch_count = 0;
        next_time_stamp   = Uint32( ~0 );

        am_host   = bfalse;
        my_host   = NULL;
        game_host = NULL;
    }
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

// Network orders
//extern unsigned char           ordervalid[MAXORDER];
//extern unsigned char           orderwho[MAXORDER][MAXSELECT];
//extern unsigned int            orderwhat[MAXORDER];
//extern unsigned int            orderwhen[MAXORDER];

extern FILE                   *globalnetworkerr;             ///< For debuggin' network

extern Uint32                  randsave;                  ///< Used in network timer

//--------------------------------------------------------------------------------------------
// Networking functions
//--------------------------------------------------------------------------------------------

const net_instance * network_get_instance();
bool_t               network_initialized();
bool_t               network_get_host_active();
bool_t               network_set_host_active( bool_t state );
bool_t               network_waiting_for_players();

void network_system_begin( ego_config_data * pcfg );
void network_system_end( void );

void net_dispatch_packets();
void unbuffer_all_player_latches();
void close_session();

void net_send_message();

void net_updateFileTransfers();
int  net_pendingFileTransfers();

void net_copyFileToAllPlayers( const char *source, const char *dest );
void net_copyFileToHost( const char *source, const char *dest );
void net_copyDirectoryToHost( const char *dirname, const char *todirname );
void net_copyDirectoryToAllPlayers( const char *dirname, const char *todirname );
void net_sayHello();
void cl_talkToHost();
void sv_talkToRemotes();

int sv_hostGame();
int cl_joinGame( const char *hostname );

void find_open_sessions();
void sv_letPlayersJoin();
void stop_players_from_joining();
// int create_player(int host);
// void turn_on_service(int service);

void net_reset_players();

void tlatch_ary_init( ego_time_latch ary[], size_t len );

void   PlaStack_toggle_all_explore();
void   PlaStack_toggle_all_wizard();
bool_t PlaStack_has_explore();
bool_t PlaStack_has_wizard();

void net_synchronize_game_clock();
