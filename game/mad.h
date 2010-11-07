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

/// \file mad.h

#include "mad_defs.h"

#include "file_formats/id_md2.h"
#include "md2.h"

#include <SDL_opengl.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct Mix_Chunk;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// The definition of the Egoboo model type
struct ego_mad
{
    EGO_PROFILE_STUFF

    Uint16  frameliptowalkframe[4][16];        ///< For walk animations

    int     action_map[ACTION_COUNT];          ///< actual action = action_map[requested action]
    bool_t  action_valid[ACTION_COUNT];        ///< bfalse if not valid
    int     action_stt[ACTION_COUNT];          ///< First frame of anim
    int     action_end[ACTION_COUNT];          ///< One past last frame

    //---- per-object data ----

    // model data
    ego_MD2_Model * md2_ptr;                       ///< the pointer that will eventually be used

    static ego_mad * ctor_this( ego_mad * pmad );
    static ego_mad * dtor_this( ego_mad * pmad );
    static ego_mad * reconstruct( ego_mad * pmad );

private:
    static bool_t    dealloc( ego_mad * pmad );
    static ego_mad * clear( ego_mad * pmad );
};

//--------------------------------------------------------------------------------------------
extern t_stack< ego_mad, MAX_MAD > MadStack;

void MadStack_init();
void MadStack_dtor();

#define LOADED_MAD( IMAD )       ( MadStack.in_range_ref( IMAD ) && MadStack[IMAD].loaded )

//--------------------------------------------------------------------------------------------
struct mad_loader
{
    STRING  szModelName;      ///< MD2 Model name

    mad_loader()
    {
        szModelName[0] = '\0';
    }

    MAD_REF load_vfs( const char* tmploadname, const MAD_REF & object );

protected:

    ego_mad * make_actions( ego_mad * pmad );
    ego_mad * finalize( ego_mad * pmad );
    ego_mad * heal_actions( ego_mad * pmad, const char * loadname );
    ego_mad * make_framefx( ego_mad * pmad, const char * cFrameName, const int frame );
    ego_mad * make_walk_lip( ego_mad * pmad, const int lip, const int action );
    ego_mad * make_frame_lip( ego_mad * pmad, const int action );
    ego_mad * apply_copy_file_vfs( ego_mad * pmad, const char* loadname );
    ego_mad * copy_action_correct( ego_mad * pmad, const int actiona, const int actionb );
    int       get_action_index( const char * cFrameName );

};

extern mad_loader MadLoader;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// the the data to describe the "pose" that the model is currently in
/// a snapshot of the current animation state.
///
/// \note this data is needed by multiple game systems
struct pose_data
{
    static const float flip_tolerance;

    ego_uint       id;              ///< the current number of updates

    // animation info

    Uint16         frame_nxt;       ///< mad's frame
    Uint16         frame_lst;       ///< mad's last frame
    Uint8          ilip;            ///< mad's frame inbetweening
    float          flip;            ///< mad's frame inbetweening

    pose_data() { init(); }

    friend bool operator == ( const pose_data & lhs, const pose_data & rhs )
    {
        bool poses_match = false;

        bool flips_match = ( SDL_abs( lhs.flip - rhs.flip ) < flip_tolerance );

        if ( flips_match )
        {
            poses_match = ( lhs.frame_nxt == rhs.frame_nxt ) && ( lhs.frame_lst == rhs.frame_lst );
        }
        else
        {
            bool no_animation_lhs = ( lhs.frame_nxt == lhs.frame_lst );
            bool no_animation_rhs = ( rhs.frame_nxt == rhs.frame_lst );

            poses_match = no_animation_lhs && no_animation_rhs && ( lhs.frame_nxt == rhs.frame_nxt );
        }

        return poses_match;
    }

    static float get_remaining_flip( pose_data * ptr )
    {
        float remaining = 0.0f;

        if ( NULL == ptr ) return 0.0f;

        remaining = ( ptr->ilip + 1 ) * 0.25f - ptr->flip;

        CPP_EGOBOO_ASSERT( remaining >= 0.0f );

        return remaining;
    }

    void init()
    {
        id = 0;
        frame_nxt = frame_lst = 0;
        ilip = 0;
        flip = 0.0f;
    }
};

//--------------------------------------------------------------------------------------------

/// info on the model's animation...
/// which animation is current, what is the next animation,
/// what to do at the end of the current animation, etc.
struct anim_data
{
    ego_uint       id;       ///< the current number of updates

    // action info
    Uint8          ready;    ///< ready to play a new one
    Uint8          which;    ///< mad's action
    bool_t         keep;     ///< keep the action playing
    bool_t         loop;     ///< loop it too
    Uint8          next;     ///< mad's action to play next

    Uint16         frame_stt;  ///< the first frame of the animation
    Uint16         frame_end;  ///< the last frame of the animation

    float          rate;      ///< the animation frame rate (per update, not per frame)

    anim_data() { init(); }

    void init()
    {
        id    = 0;
        ready = btrue;                // argh! this must be set at the beginning, script's spawn animations do not work!
        which = next = ACTION_DA;
        keep  = loop = bfalse;
        rate  = 1.0f;
        frame_stt = frame_end = 0;
    }
};

//--------------------------------------------------------------------------------------------

// An instance of a model
struct mad_instance
{
    // model info
    MAD_REF        imad;           ///< model

    pose_data      state;          ///< the current "pose" of the model

    anim_data      action;         ///< the animation info for the model

    mad_instance() { ctor_this( this ); }

    static mad_instance * ctor_this( mad_instance * pmad_inst );

    static mad_instance * clear( mad_instance * ptr );

    static egoboo_rv set_action( mad_instance * pinst, const int next_action, const bool_t next_ready, const bool_t override_action );
    static egoboo_rv start_anim( mad_instance * pinst, const int next_action, const bool_t next_ready, const bool_t override_action );
    static egoboo_rv set_frame( mad_instance * pinst, const int frame, const int ilip = 0 );

    static egoboo_rv increment_action( mad_instance * pinst );
    static egoboo_rv increment_frame( mad_instance * pinst, const bool_t mounted = bfalse );
    static egoboo_rv play_action( mad_instance * pinst, const int action, const bool_t actionready );

    static bool_t    set_mad( mad_instance * pinst, const MAD_REF & imad );

    static BIT_FIELD update_animation( mad_instance * pinst, const float dt );
    static BIT_FIELD update_animation_one( mad_instance * pmad_inst );
    static BIT_FIELD get_framefx( mad_instance * pinst );

    static bool_t spawn( mad_instance * pmad_inst, const PRO_REF & profile );

protected:

    static BIT_FIELD update_frame( mad_instance * pmad_inst );
    static egoboo_rv begin_action( mad_instance * pmad_inst );

};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

void    MadStack_reconstruct_all();
void    MadStack_release_all();
bool_t  MadStack_release_one( const MAD_REF & imad );

void   load_action_names_vfs( const char* loadname );

void   mad_make_equally_lit( const MAD_REF & imad );

int       mad_get_action( const MAD_REF & imad, const int action );
BIT_FIELD mad_get_actionfx( const MAD_REF & imad, const int action );
int       mad_randomize_action( const int action, const int slot );
