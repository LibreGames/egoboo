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

/// @file camera.h

#include "egoboo_typedef.h"
#include "egoboo_math.h"
#include "physics.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct ego_mpd;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// The camera mode
enum e_camera_mode
{
    CAM_PLAYER = 0,
    CAM_FREE,
    CAM_RESET
};

/// The mode that the camera uses to determine where is is looking

#define CAM_TRACK_X_AREA_LOW     100
#define CAM_TRACK_X_AREA_HIGH    180
#define CAM_TRACK_Y_AREA_MINLOW  320
#define CAM_TRACK_Y_AREA_MAXLOW  460
#define CAM_TRACK_Y_AREA_MINHIGH 460
#define CAM_TRACK_Y_AREA_MAXHIGH 600

#define CAM_FOV                             60          ///< Field of view
#define CAM_TURN_JOY              (3.0f * 5.0f)         ///< Joystick camera rotation
#define CAM_TURN_KEY               CAM_TURN_JOY         ///< Keyboard camera rotation
#define CAM_TURN_TIME                       16          ///< Smooth turn
#define CAM_TRACK_FAR                     1200          ///< For outside modules...
#define CAM_TRACK_EDGE                     800          ///< Camtrack bounds

/// Multi cam (uses macro to switch between old and new camera
#if !defined(OLD_CAMERA_MODE)
#    define CAM_ZOOM_MIN                         800         ///< Camera distance
#    define CAM_ZOOM_MAX                         700
#    define CAM_ZADD_MIN                         800         ///< Camera height
#    define CAM_ZADD_MAX                         2500
#    define CAM_UPDOWN_MIN                       (0.24f*PI)    ///< Camera updown angle
#    define CAM_UPDOWN_MAX                       (0.10f*PI)
#else
#    define CAM_ZOOM_MIN                         500         ///< Camera distance
#    define CAM_ZOOM_MAX                         600
#    define CAM_ZADD_MIN                         800         ///< Camera height
#    define CAM_ZADD_MAX                         1500  ///< 1000
#    define CAM_UPDOWN_MIN                       (0.24f*PI)    ///< Camera updown angle
#    define CAM_UPDOWN_MAX                       (0.18f*PI)// (0.15f*PI) ///< (0.18f*PI)
#endif

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// definition of the Egoboo camera object
struct ego_camera
{
    fmat_4x4_t mView, mViewSave;      ///< View Matrix
    fmat_4x4_t mProjection;           ///< Projection Matrix

    Uint8  move_mode;               ///< what is the camera mode
    Uint8  move_mode_old;           ///< the default movement mode
    Uint8  turn_mode;               ///< what is the camera mode
    Uint8  turn_time;               ///< time for the smooth turn

    int           swing;                   ///< Camera swingin'
    int           swingrate;
    float         swingamp;

    fvec3_t       pos;                       ///< Camera position (z = 500-1000)
    ego_orientation ori;

    float         zoom;                    ///< Distance from the trackee
    fvec3_t       track_pos;                  ///< Trackee position
    float         track_level;
    fvec3_t       center;                 ///< Move character to side before tracking
    float         zadd;                    ///< Camera height above terrain
    float         zaddgoto;                ///< Desired z position
    float         zgoto;
    float         turn_z_rad;           ///< Camera rotations
    float         turn_z_one;
    float         turnadd;                 ///< Turning rate
    float         sustain;                 ///< Turning rate falloff
    float         turnupdown;
    float         roll;
    float         motion_blur;      ///< Blurry effect

    fvec3_t   vfw;                 ///< the camera forward vector
    fvec3_t   vup;                 ///< the camera up vector
    fvec3_t   vrt;                 ///< the camera right vector

    ego_camera() { clear( this ); ctor( this ); }

    static ego_camera * ctor( ego_camera * pcam );

    static void         reset( ego_camera * pcam, struct ego_mpd * pmesh );
    static void         adjust_angle( ego_camera * pcam, float height );
    static void         move( ego_camera * pcam, struct ego_mpd * pmesh );
    static void         make_matrix( ego_camera * pcam );
    static void         look_at( ego_camera * pcam, float x, float y );

    static bool_t       reset_target( ego_camera * pcam, struct ego_mpd * pmesh );
    static void         rotmesh_init();

private:
    static ego_camera * clear( ego_camera * ptr )
    {
        if ( NULL == ptr ) return ptr;

        memset( ptr, 0, sizeof( *ptr ) );

        return ptr;
    }
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
extern ego_camera gCamera;
