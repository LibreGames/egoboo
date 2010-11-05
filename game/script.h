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

#include "egoboo_typedef_cpp.h"

#include "clock.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct ego_cap;
struct ego_chr;
struct ego_pro;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define MSGDISTANCE         2000                    ///< Range for SendMessageNear
#define PITNOSOUND          -256                    ///< Stop sound at bottom of pits...

// These are for FindPath function only
#define   MOVE_MELEE  300
#define   MOVE_RANGED  600
#define   MOVE_DISTANCE 175
#define   MOVE_RETREAT  900
#define   MOVE_CHARGE  111
#define   MOVE_FOLLOW  0

// Camera bounds/edge of the map
#define EDGE                128

// AI targeting
#define NEARBY      3*GRID_SIZE    ///< 3 tiles away
#define WIDE        6*GRID_SIZE    ///< 6 tiles away
#define NEAREST     0              ///< unlimited range

/// Character AI alerts
enum chr_alert_bits
{
    ALERT_NONE                           = 0,
    ALERTIF_SPAWNED                      = 1 <<  0,
    ALERTIF_HITVULNERABLE                = 1 <<  1,
    ALERTIF_ATWAYPOINT                   = 1 <<  2,
    ALERTIF_ATLASTWAYPOINT               = 1 <<  3,
    ALERTIF_ATTACKED                     = 1 <<  4,
    ALERTIF_BUMPED                       = 1 <<  5,
    ALERTIF_ORDERED                      = 1 <<  6,
    ALERTIF_CALLEDFORHELP                = 1 <<  7,
    ALERTIF_KILLED                       = 1 <<  8,
    ALERTIF_TARGETKILLED                 = 1 <<  9,
    ALERTIF_DROPPED                      = 1 << 10,
    ALERTIF_GRABBED                      = 1 << 11,
    ALERTIF_REAFFIRMED                   = 1 << 12,
    ALERTIF_LEADERKILLED                 = 1 << 13,
    ALERTIF_USED                         = 1 << 14,
    ALERTIF_CLEANEDUP                    = 1 << 15,
    ALERTIF_SCOREDAHIT                   = 1 << 16,
    ALERTIF_HEALED                       = 1 << 17,
    ALERTIF_DISAFFIRMED                  = 1 << 18,
    ALERTIF_CHANGED                      = 1 << 19,
    ALERTIF_INWATER                      = 1 << 20,
    ALERTIF_BORED                        = 1 << 21,
    ALERTIF_TOOMUCHBAGGAGE               = 1 << 22,
    ALERTIF_LEVELUP                      = 1 << 23,
    ALERTIF_CONFUSED                     = 1 << 24,
    ALERTIF_HITGROUND                    = 1 << 25,
    ALERTIF_NOTDROPPED                   = 1 << 26,
    ALERTIF_BLOCKED                      = 1 << 27,
    ALERTIF_THROWN                       = 1 << 28,
    ALERTIF_CRUSHED                      = 1 << 29,
    ALERTIF_NOTPUTAWAY                   = 1 << 30,
    ALERTIF_TAKENOUT                     = 1 << 31,

    // add in some aliases
    ALERTIF_PUTAWAY     = ALERTIF_ATLASTWAYPOINT,
    ALERTIF_NOTTAKENOUT = ALERTIF_NOTPUTAWAY
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// AI variables
#define MAXWAY              8                       ///< Waypoints
#define WAYTHRESH           (TILE_ISIZE >> 1)       ///< Threshold for reaching waypoint (GRID_SIZE/2)

// swig chokes on the definition below
#if defined(SWIG)
#   define STOR_BITS            4
#   define STOR_COUNT          16                      ///< Storage data (Used in SetXY)
#   define STOR_AND            15                      ///< Storage data bitmask
#else
#   define STOR_BITS            4
#   define STOR_COUNT          (1 << STOR_BITS)        ///< Storage data (Used in SetXY)
#   define STOR_AND            (STOR_COUNT - 1)        ///< Storage data bitmask
#endif

typedef float waypoint_t[3];

struct ego_waypoint_list
{
    int          tail;         ///< Which waypoint
    int          head;         ///< Where to stick next
    waypoint_t   pos[MAXWAY];  ///< Waypoint

    static bool_t push( ego_waypoint_list * plst, float x, float y, float z );
    static bool_t reset( ego_waypoint_list * plst );
    static bool_t clear( ego_waypoint_list * plst );
    static bool_t advance( ego_waypoint_list * plst );
    static bool_t peek( ego_waypoint_list * plst, waypoint_t wp );
    static bool_t empty( const ego_waypoint_list * plst );
    static bool_t finished( const ego_waypoint_list * plst );
};

/// the state variables for a script / AI
struct ego_ai_state
{
    // which script to run
    REF_T          type;

    // the execution pointer(s)
    size_t         exe_stt;
    size_t         exe_end;
    size_t         exe_pos;
    Uint32         opcode;

    // some script states
    Sint32         poof_time;
    bool_t         changed;
    bool_t         terminate;
    Uint32         indent;
    Uint32         indent_last;

    // who are we related to?
    CHR_REF        index;         ///< what is the index value of this character
    CHR_REF        target;        ///< Who the AI is after
    CHR_REF        owner;         ///< The character's owner
    CHR_REF        child;         ///< The character's child

    // some local storage
    BIT_FIELD      alert;            ///< Alerts for AI script
    int            state;            ///< Short term memory for AI
    int            content;          ///< More short term memory
    int            passage;          ///< The passage associated with this character
    Uint32         timer;            ///< AI Timer
    int            x[STOR_COUNT];    ///< Temporary values...  SetXY
    int            y[STOR_COUNT];

    // ai memory from the last event
    CHR_REF        bumplast;        ///< Last character it was bumped by
    int            bumplast_time;   ///< The last time that a ALERTIF_BUMPED was sent

    CHR_REF        attacklast;      ///< Last character it was attacked by
    CHR_REF        hitlast;         ///< Last character it hit
    FACING_T       directionlast;   ///< Direction of last attack/healing
    Uint16         damagetypelast;  ///< Last damage type
    CHR_REF        lastitemused;    ///< The last item the character used
    CHR_REF        target_old;      ///< Target in the previous update

    // message handling
    Uint32         order_value;           ///< The last order given the character
    Uint16         order_counter;         ///< The rank of the character on the order chain

    // waypoints
    bool_t          wp_valid;            ///< is the current waypoint valid?
    waypoint_t      wp;                  ///< current waypoint
    ego_waypoint_list wp_lst;              ///< Stored waypoints

    ego_ai_state() : PROFILE_INIT_STRUCT( ai ) { ctor_this( this ); }
    ~ego_ai_state() { dtor_this( this ); }

    static ego_ai_state * ctor_this( ego_ai_state * pself );
    static ego_ai_state * dtor_this( ego_ai_state * pself );
    static bool_t         dealloc( ego_ai_state * pself );
    static ego_ai_state * reconstruct( ego_ai_state * pself );

    static void           spawn( ego_ai_state * pself, const CHR_REF & index, const PRO_REF & iobj, Uint16 rank );
    static bool_t         set_bumplast( ego_ai_state * pself, const CHR_REF &  ichr );
    static bool_t         get_wp( ego_ai_state * pself );
    static bool_t         ensure_wp( ego_ai_state * pself );
    static bool_t         set_changed( ego_ai_state * pself );
    static bool_t         add_order( ego_ai_state * pai, Uint32 value, Uint16 counter );

    // performance monitoring
    PROFILE_DECLARE_STRUCT( ai );
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// The state of the scripting system
/// @details It is not persistent between one evaluation of a script and another
struct ego_script_state
{
    int     x;
    int     y;
    int     turn;
    int     distance;
    int     argument;
    int     operationsum;
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct ego_ai_bundle
{
    CHR_REF   chr_ref;
    ego_chr * chr_ptr;

    PRO_REF   pro_ref;
    ego_pro * pro_ptr;

    CAP_REF   cap_ref;
    ego_cap * cap_ptr;

    REF_T            ai_state_ref;
    ego_ai_state   * ai_state_ptr;

    static ego_ai_bundle * ctor_this( ego_ai_bundle * pbundle );
    static ego_ai_bundle * validate( ego_ai_bundle * pbundle );
    static ego_ai_bundle * set( ego_ai_bundle * pbundle, struct ego_chr * pchr );
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// Prototypes

void  scr_run_chr_script( ego_ai_bundle * pbdl_ai );

void issue_order( const CHR_REF & character, Uint32 order );
void issue_special_order( Uint32 order, IDSZ idsz );
void set_alerts( ego_ai_bundle * pbdl_ai );

void scripting_system_begin();
void scripting_system_end();
