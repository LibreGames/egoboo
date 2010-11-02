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

/// @file egoboo_defs.h

#include "egoboo_typedef.h"

#if defined(__cplusplus)
extern "C"
{
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// The following magic allows this include to work in multiple files
#if defined(DECLARE_GLOBALS)
#    define EXTERN
#    define EQ(x) = x
#else
#    define EXTERN extern
#    define EQ(x)
#endif

#define VERSION "2.8.1"                            ///< Version of the game

#define SPELLBOOK           127                     ///< The spellbook model

#define DAMAGERAISE         25                  ///< Tolerance for damage tiles

#define INVISIBLE           20                      ///< The character can't be detected

    /* SDL_GetTicks() always returns milli seconds */
#define TICKS_PER_SEC                   1000.0f

#define TARGET_FPS                      30.0f
#define FRAME_SKIP                      (TICKS_PER_SEC/TARGET_FPS)    ///< 1000 tics per sec / 50 fps = 20 ticks per frame

#define EMULATE_UPS                     50.0f
#define TARGET_UPS                      50.0f
#define UPDATE_SCALE                    (EMULATE_UPS/(stabilized_ups_sum/stabilized_ups_weight))
#define UPDATE_SKIP                     (TICKS_PER_SEC/TARGET_UPS)    ///< 1000 tics per sec / 50 fps = 20 ticks per frame
#define ONESECOND                       (TICKS_PER_SEC/UPDATE_SKIP)    ///< 1000 tics per sec / 20 ticks per frame = 50 fps

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

//---- Timers

// Display
    EXTERN bool_t          timeron     EQ( bfalse );          ///< Game timer displayed?
    EXTERN Uint32          timervalue  EQ( 0 );           ///< Timer time ( 50ths of a second )

// fps stuff
    EXTERN float           stabilized_fps        EQ( TARGET_FPS );
    EXTERN float           stabilized_fps_sum    EQ( 0 );
    EXTERN float           stabilized_fps_weight EQ( 0 );

    EXTERN float           est_max_fps           EQ( TARGET_FPS );
    EXTERN float           est_render_time       EQ( 1.0f / TARGET_FPS );

    EXTERN float           est_update_time       EQ( 1.0f / TARGET_UPS );
    EXTERN float           est_max_ups           EQ( TARGET_UPS );

    EXTERN float           est_gfx_time          EQ( 1.0f / TARGET_FPS );
    EXTERN float           est_max_gfx           EQ( TARGET_FPS );

    EXTERN float           est_single_update_time  EQ( 1.0f / TARGET_UPS );
    EXTERN float           est_single_ups          EQ( TARGET_UPS );

    EXTERN float           est_update_game_time  EQ( 1.0f / TARGET_UPS );
    EXTERN float           est_max_game_ups      EQ( TARGET_UPS );

    EXTERN float           stabilized_ups        EQ( TARGET_UPS );
    EXTERN float           stabilized_ups_sum    EQ( 0 );
    EXTERN float           stabilized_ups_weight EQ( 0 );

// Timers
    EXTERN unsigned        ticks_last  EQ( 0 );
    EXTERN unsigned        ticks_now   EQ( 0 );
    EXTERN unsigned        clock_stt   EQ( 0 );             ///< egoboo_get_ticks() at start
    EXTERN unsigned        clock_all   EQ( 0 );             ///< The total number of ticks so far
    EXTERN unsigned        clock_lst   EQ( 0 );             ///< The last total of ticks so far
    EXTERN signed          clock_wld   EQ( 0 );             ///< The sync clock
    EXTERN Uint32          clock_enc_stat  EQ( 0 );         ///< For character stat regeneration
    EXTERN Uint32          clock_chr_stat  EQ( 0 );         ///< For enchant stat regeneration

// counters
    EXTERN unsigned        update_wld  EQ( 0 );             ///< The number of times the game has been updated
    EXTERN unsigned        frame_all   EQ( 0 );             ///< The total number of frames drawn so far
    EXTERN unsigned        true_update EQ( 0 );             ///< estimation of the total number of updates that should have happened in the game so far
    EXTERN unsigned        true_frame  EQ( 0 );             ///< estimation of the total number of frames that should have happened in the game so far
    EXTERN signed          estimated_updates  EQ( 0 );      ///< the estimation of how many times the update loop will have to be run to "catch up"
    EXTERN signed          update_lag  EQ( 0 );             ///< the number of updated that occured the last time the update counter tried to catch up to the true_update

// timers
    EXTERN signed          timer_heartbeat EQ( 0 );         ///< For game updates that happen once a second (should time ot immediately)
    EXTERN Uint32          timer_pit   EQ( 0 );             ///< For pit kills
    EXTERN int             timer_revive EQ( 0 );            ///< Respawning

// KEYBOARD
    EXTERN bool_t console_mode EQ( bfalse );                   ///< Input text from keyboard?
    EXTERN bool_t console_done EQ( bfalse );                   ///< Input text from keyboard finished?

// the variables controlling the single-frame-mode

    EXTERN bool_t single_update_mode EQ( bfalse );
    EXTERN bool_t single_frame_keyready EQ( btrue );
    EXTERN bool_t single_frame_requested EQ( bfalse );
    EXTERN bool_t single_update_requested EQ( bfalse );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

    void ego_init_SDL_base( void );

    void egoboo_clear_vfs_paths( void );
    void egoboo_setup_vfs_paths( void );

    Uint32 egoboo_get_ticks( void );

#if defined(__cplusplus)
}
#endif

#define mad_defs_h
