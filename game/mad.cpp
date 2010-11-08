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

/// \file mad.c
/// \brief The files for handling Egoboo's internal model definitions
/// \details

#include "mad.h"

#include "md2.inl"

#include "file_formats/cap_file.h"

#include "log.h"
#include "script_compile.h"
#include "graphic.h"
#include "texture.h"
#include "sound.h"

#include "egoboo_setup.h"
#include "egoboo_fileutil.h"
#include "egoboo_strutil.h"

#include "particle.inl"
#include "egoboo_math.inl"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

t_stack< ego_mad, MAX_MAD  > MadStack;

mad_loader MadLoader;

/// the flip tolerance is the default flip increment / 2
const float pose_data::flip_tolerance = 0.25f * 0.5f;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static char    cActionName[ACTION_COUNT][2];      ///< Two letter name code
static STRING  cActionComent[ACTION_COUNT];       ///< Strings explaining the action codes

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static int mad_get_action( const ego_mad * pmad, const int action );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void MadStack_init()
{
    // initialize all ego_mad
    for ( MAD_REF cnt( 0 ); cnt < MAX_MAD; cnt++ )
    {
        ego_mad * pmad = MadStack + cnt;

        // blank out all the data, including the obj_base data
        ego_mad::dtor_this( pmad );

        ego_mad::reconstruct( pmad );
    }
}

//--------------------------------------------------------------------------------------------
void MadStack_dtor()
{
    for ( MAD_REF cnt( 0 ); cnt < MAX_MAD; cnt++ )
    {
        ego_mad::dtor_this( MadStack + cnt );
    }
}

//--------------------------------------------------------------------------------------------
void MadStack_reconstruct_all()
{
    MAD_REF cnt;

    for ( cnt = 0; cnt < MAX_MAD; cnt++ )
    {
        ego_mad::reconstruct( MadStack + cnt );
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void MadStack_release_all()
{
    MAD_REF cnt;

    for ( cnt = 0; cnt < MAX_MAD; cnt++ )
    {
        MadStack_release_one( cnt );
    }
}

//--------------------------------------------------------------------------------------------
bool_t MadStack_release_one( const MAD_REF & imad )
{
    ego_mad * pmad;

    if ( !MadStack.in_range_ref( imad ) ) return bfalse;
    pmad = MadStack + imad;

    if ( !pmad->loaded ) return btrue;

    // free any md2 data
    ego_mad::reconstruct( pmad );

    return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ego_mad * ego_mad::clear( ego_mad * pmad )
{
    if ( NULL == pmad ) return pmad;

    SDL_memset( pmad, 0, sizeof( *pmad ) );

    return pmad;
}

//--------------------------------------------------------------------------------------------
bool_t ego_mad::dealloc( ego_mad * pmad )
{
    /// Free all allocated memory

    if ( NULL == pmad || !pmad->loaded ) return bfalse;

    ego_MD2_Model::destroy( &( pmad->md2_ptr ) );

    return btrue;
}

//--------------------------------------------------------------------------------------------
ego_mad * ego_mad::reconstruct( ego_mad * pmad )
{
    /// \author BB
    /// \details  initialize the character data to safe values
    ///     since we use SDL_memset(..., 0, ...), all = 0, = false, and = 0.0f
    ///     statements are redundant

    int action;

    if ( NULL == pmad ) return NULL;

    ego_mad::dealloc( pmad );

    pmad = ego_mad::clear( pmad );

    strncpy( pmad->name, "*NONE*", SDL_arraysize( pmad->name ) );

    // Clear out all actions and reset to invalid
    for ( action = 0; action < ACTION_COUNT; action++ )
    {
        pmad->action_map[action]   = ACTION_COUNT;
    }

    return pmad;
}

//--------------------------------------------------------------------------------------------
ego_mad * ego_mad::ctor_this( ego_mad * pmad )
{
    if ( NULL == ego_mad::reconstruct( pmad ) ) return NULL;

    // nothing to do yet

    return pmad;
}

//--------------------------------------------------------------------------------------------
ego_mad * ego_mad::dtor_this( ego_mad * pmad )
{
    /// \author BB
    /// \details  deinitialize the character data

    if ( NULL == pmad || !pmad->loaded ) return NULL;

    // deinitialize the object
    ego_mad::reconstruct( pmad );

    // "destruct" the base object
    pmad->loaded = bfalse;

    return pmad;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
egoboo_rv mad_instance::set_action( mad_instance * pmad_inst, const int next_action, const bool_t next_ready, const bool_t override_action )
{
    ego_mad * pmad;

    // did we get a bad pointer?
    if ( NULL == pmad_inst ) return rv_error;

    // is the action in the valid range?
    if ( next_action < 0 || next_action > ACTION_COUNT ) return rv_error;

    // do we have a valid model?
    if ( !LOADED_MAD( pmad_inst->imad ) ) return rv_error;
    pmad = MadStack + pmad_inst->imad;

    // is the chosen action valid?
    if ( !pmad->action_valid[ next_action ] ) return rv_fail;

    // are we going to check action.ready?
    if ( !override_action && !pmad_inst->action.ready ) return rv_fail;

    // set up the action
    pmad_inst->action.which     = next_action;
    pmad_inst->action.next      = ACTION_DA;
    pmad_inst->action.ready     = next_ready;
    pmad_inst->action.frame_stt = pmad->action_stt[next_action];
    pmad_inst->action.frame_end = pmad->action_end[next_action];

    return rv_success;
}

//--------------------------------------------------------------------------------------------
egoboo_rv mad_instance::begin_action( mad_instance * pmad_inst )
{
    int frame_nxt;
    bool_t updated = bfalse;

    // did we get a bad pointer?
    if ( NULL == pmad_inst ) return rv_error;

    frame_nxt = pmad_inst->action.frame_stt;

    if ( pmad_inst->state.frame_nxt != frame_nxt )
    {
        pmad_inst->state.frame_nxt = frame_nxt;
        updated = btrue;
    }

    //---- don't mess with the frame inbetweening

    return updated ? rv_success : rv_fail;
}

//--------------------------------------------------------------------------------------------
egoboo_rv mad_instance::set_frame( mad_instance * pmad_inst, const int frame, const int ilip )
{
    int frame_stt, frame_nxt;
    bool_t updated = bfalse;

    // did we get a bad pointer?
    if ( NULL == pmad_inst ) return rv_error;

    // is the action in the valid range?
    if ( /* pmad_inst->action.which < 0 || */ pmad_inst->action.which > ACTION_COUNT ) return rv_error;

    // is the frame within the valid range for this action?
    if ( frame <  0 ) return rv_fail;
    if ( frame >= pmad_inst->action.frame_end - pmad_inst->action.frame_stt ) return rv_fail;

    frame_stt = pmad_inst->action.frame_stt + frame;
    frame_stt = SDL_min( frame_stt, pmad_inst->action.frame_end );

    frame_nxt = frame_stt + 1;
    frame_nxt = SDL_min( frame_nxt, pmad_inst->action.frame_end );

    if ( pmad_inst->state.frame_lst != frame_stt )
    {
        pmad_inst->state.frame_lst  = frame_stt;
        updated = btrue;
    }

    if ( pmad_inst->state.frame_nxt != frame_nxt )
    {
        pmad_inst->state.frame_nxt  = frame_nxt;
        updated = btrue;
    }

    if ( ilip != pmad_inst->state.ilip )
    {
        pmad_inst->state.ilip = ilip;
        updated = btrue;
    }

    pmad_inst->state.flip = ilip / 4.0f;

    return updated ? rv_success : rv_fail;
}

//--------------------------------------------------------------------------------------------
egoboo_rv mad_instance::start_anim( mad_instance * pmad_inst, const int next_action, const bool_t next_ready, const bool_t override_action )
{
    egoboo_rv retval = rv_fail;

    if ( NULL == pmad_inst ) return rv_error;

    if ( rv_success == set_action( pmad_inst, next_action, next_ready, override_action ) )
    {
        retval = begin_action( pmad_inst );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
egoboo_rv mad_instance::increment_action( mad_instance * pmad_inst )
{
    /// \author BB
    /// \details  This function starts the next action for a character

    egoboo_rv retval;
    ego_mad * pmad;
    int     action;
    bool_t  next_ready;

    if ( NULL == pmad_inst ) return rv_error;

    if ( !LOADED_MAD( pmad_inst->imad ) ) return rv_error;
    pmad = MadStack + pmad_inst->imad;

    // get the correct action
    action = mad_get_action( pmad, pmad_inst->action.next );

    // determine if the action is one of the types that can be broken at any time
    // D == "dance" and "W" == walk
    next_ready = ACTION_IS_TYPE( action, D ) || ACTION_IS_TYPE( action, W );

    retval = mad_instance::set_action( pmad_inst, action, next_ready, btrue );
    if ( rv_success != retval ) return retval;

    // just update the next frame directly. this will leave the
    // old frame at the end of the last action and the new frame at the beginning of the new action
    pmad_inst->state.frame_nxt = pmad_inst->action.frame_stt;

    return retval;
}

//--------------------------------------------------------------------------------------------
egoboo_rv mad_instance::increment_frame( mad_instance * pmad_inst, const bool_t mounted )
{
    /// \author BB
    /// \details  all the code necessary to move on to the next frame of the animation

    int tmp_action;
    ego_mad * pmad;

    if ( NULL == pmad_inst ) return rv_error;

    if ( !LOADED_MAD( pmad_inst->imad ) ) return rv_error;
    pmad = MadStack + pmad_inst->imad;

    // Change frames
    pmad_inst->state.frame_lst = pmad_inst->state.frame_nxt;
    pmad_inst->state.frame_nxt++;

    // detect the end of the animation and handle special end conditions
    if ( pmad_inst->state.frame_nxt > pmad_inst->action.frame_end )
    {
        // make sure that the frame_nxt points to a valid frame in this action
        pmad_inst->state.frame_nxt = pmad_inst->action.frame_end;

        if ( pmad_inst->action.keep )
        {
            // Freeze that animation at the last frame
            pmad_inst->state.frame_nxt = pmad_inst->state.frame_lst;

            // Break a kept action at any time
            pmad_inst->action.ready = btrue;
        }
        else if ( pmad_inst->action.loop )
        {
            // Convert the action into a riding action if the character is mounted
            if ( mounted )
            {
                tmp_action = mad_get_action( pmad, ACTION_MI );
                mad_instance::start_anim( pmad_inst, tmp_action, btrue, btrue );
            }

            // set the frame to the beginning of the action
            pmad_inst->state.frame_nxt = pmad_inst->action.frame_stt;

            // Break a looped action at any time
            pmad_inst->action.ready = btrue;
        }
        else
        {
            // Go on to the next action. don't let just anything interrupt it?
            mad_instance::increment_action( pmad_inst );
        }
    }

    return rv_success;
}

//--------------------------------------------------------------------------------------------
egoboo_rv mad_instance::play_action( mad_instance * pmad_inst, const int next_action, const bool_t next_ready )
{
    /// \author ZZ
    /// \details  This function starts a generic action for a character
    ego_mad * pmad;

    if ( NULL == pmad_inst ) return rv_error;

    if ( !LOADED_MAD( pmad_inst->imad ) ) return rv_error;
    pmad = MadStack + pmad_inst->imad;

    int dst_action = mad_get_action( pmad, next_action );

    return mad_instance::start_anim( pmad_inst, dst_action, next_ready, btrue );
}

//--------------------------------------------------------------------------------------------
BIT_FIELD mad_instance::get_framefx( mad_instance * pmad_inst )
{
    int             frame_count;
    ego_MD2_Frame * frame_list, * pframe_nxt;
    ego_mad       * pmad;
    ego_MD2_Model * pmd2;

    if ( NULL == pmad_inst ) return EMPTY_BIT_FIELD;

    if ( !LOADED_MAD( pmad_inst->imad ) ) return EMPTY_BIT_FIELD;
    pmad = MadStack + pmad_inst->imad;

    pmd2 = pmad->md2_ptr;
    if ( NULL == pmd2 ) return 0;

    frame_count = md2_get_numFrames( pmd2 );
    EGOBOO_ASSERT( pmad_inst->state.frame_nxt < frame_count );

    frame_list  = ( ego_MD2_Frame * )md2_get_Frames( pmd2 );
    pframe_nxt  = frame_list + pmad_inst->state.frame_nxt;

    return pframe_nxt->framefx;
}

//--------------------------------------------------------------------------------------------
BIT_FIELD mad_instance::update_frame( mad_instance * pmad_inst )
{
    BIT_FIELD fx = EMPTY_BIT_FIELD;

    if ( rv_success == mad_instance::increment_frame( pmad_inst ) )
    {
        pmad_inst->state.flip  = fmod( pmad_inst->state.flip, 1.0f );
        pmad_inst->state.ilip %= 4;

        fx = get_framefx( pmad_inst );
    }
    else
    {
        log_warning( "%s did not succeed", __FUNCTION__ );
    }

    return fx;
}

//--------------------------------------------------------------------------------------------
BIT_FIELD mad_instance::update_animation_one( mad_instance * pmad_inst )
{
    BIT_FIELD fx = EMPTY_BIT_FIELD;

    if ( NULL == pmad_inst ) return 0;

    // do the basic frame update
    pmad_inst->state.ilip += 1;
    pmad_inst->state.flip += 0.25f;

    // make them wrap around properly
    if ( pmad_inst->state.ilip >= 4 )
    {
        pmad_inst->state.flip = fmod( pmad_inst->state.flip, 1.0f );
        pmad_inst->state.ilip %= 4;
    }

    pmad_inst->state.id++;

    // increment the frame, if necessary
    if ( 0 == pmad_inst->state.ilip )
    {
        fx = mad_instance::update_frame( pmad_inst );
    }

    return fx;
}

//--------------------------------------------------------------------------------------------
BIT_FIELD mad_instance::update_animation( mad_instance * pmad_inst, const float dt )
{
    // this is a bit more complicated than the version that updates one frame at a time
    // beause of keeping the floating point flip and the integer ilip in synch.

    bool_t new_ilip = bfalse;

    BIT_FIELD fx = EMPTY_BIT_FIELD;

    if ( NULL == pmad_inst || 0.0f == dt ) return 0;

    int ilip_old = pmad_inst->state.ilip;

    // do the basic frame update
    pmad_inst->state.flip += dt;
    pmad_inst->state.ilip  = floorf( pmad_inst->state.flip * 4.0f );

    // did we transition to a new ilip
    new_ilip = ( ilip_old !=  pmad_inst->state.ilip );

    // make them wrap around properly
    if ( pmad_inst->state.ilip >= 4 )
    {
        pmad_inst->state.flip = fmod( pmad_inst->state.flip, 1.0f );
        pmad_inst->state.ilip %= 4;
        new_ilip = btrue;
    }

    // don't update the id unless there is a significant change in the parameters
    if ( dt > pose_data::flip_tolerance || new_ilip )
    {
        pmad_inst->state.id++;
    }

    // increment the frame, if necessary
    if ( 0 == pmad_inst->state.ilip && new_ilip )
    {
        fx = mad_instance::update_frame( pmad_inst );
    }

    return fx;
}

//--------------------------------------------------------------------------------------------
mad_instance * mad_instance::clear( mad_instance * ptr )
{
    if ( NULL == ptr ) return ptr;

    // model info
    ptr->imad = MAX_MAD;

    // animation info
    ptr->state.init();

    // action info
    ptr->action.init();

    return ptr;
}

//--------------------------------------------------------------------------------------------
bool_t mad_instance::spawn( mad_instance * pmad_inst, const PRO_REF & profile )
{
    ego_pro * pobj;

    if ( NULL == pmad_inst ) return bfalse;

    // clear the instance
    mad_instance::ctor_this( pmad_inst );

    if ( !LOADED_PRO( profile ) ) return bfalse;
    pobj = ProList.lst + profile;

    // model parameters
    mad_instance::set_mad( pmad_inst, pro_get_imad( profile ) );

    // set the initial action, all actions override it
    mad_instance::play_action( pmad_inst, ACTION_DA, btrue );

    return btrue;
}

//--------------------------------------------------------------------------------------------
mad_instance * mad_instance::ctor_this( mad_instance * pmad_inst )
{
    if ( NULL == pmad_inst ) return pmad_inst;

    // model parameters
    pmad_inst->imad = MAX_MAD;

    // set the action state
    pmad_inst->action.init();

    // set the animation state
    pmad_inst->state.init();

    return pmad_inst;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
MAD_REF mad_loader::load_vfs( const char* tmploadname, const MAD_REF & imad )
{
    ego_mad * pmad;
    STRING  newloadname;

    if ( !MadStack.in_range_ref( imad ) ) return MAD_REF( MAX_MAD );
    pmad = MadStack + imad;

    // clear out the mad
    ego_mad::reconstruct( pmad );

    // set up the EGO_PROFILE_STUFF
    strncpy( pmad->name, tmploadname, SDL_arraysize( pmad->name ) );
    pmad->name[ SDL_arraysize( pmad->name ) - 1 ] = CSTR_END;
    pmad->loaded = btrue;

    // Load the imad model
    make_newloadname( tmploadname, "/tris.md2", newloadname );

    // load the model from the file
    pmad->md2_ptr = ego_MD2_Model::load( vfs_resolveReadFilename( newloadname ), NULL );

    // set the model's file name
    szModelName[0] = CSTR_END;
    if ( NULL != pmad->md2_ptr )
    {
        strncpy( szModelName, vfs_resolveReadFilename( newloadname ), SDL_arraysize( szModelName ) );

        /// \author BB
        /// \details  Egoboo md2 models were designed with 1 tile = 32x32 units, but internally Egoboo uses
        ///      1 tile = 128x128 units. Previously, this was handled by sprinkling a bunch of
        ///      commands that multiplied various quantities by 4 or by 4.125 throughout the code.
        ///      It was very counter-intuitive, and caused me no end of headaches...  Of course the
        ///      solution is to scale the model!
        ego_MD2_Model::scale( pmad->md2_ptr, -3.5f, 3.5f, 3.5f );
    }

    // Create the actions table for this imad
    pmad = make_actions( pmad );
    pmad = heal_actions( pmad, tmploadname );
    pmad = finalize( pmad );

    return imad;
}

//--------------------------------------------------------------------------------------------
ego_mad * mad_loader::copy_action_correct( ego_mad * pmad, const int actiona, const int actionb )
{
    /// \author ZZ
    /// \details  This function makes sure both actions are valid if either of them
    ///    are valid.  It will copy start and ends to mirror the valid action.

    if ( NULL == pmad ) return pmad;

    if ( actiona < 0 || actiona >= ACTION_COUNT ) return pmad;
    if ( actionb < 0 || actionb >= ACTION_COUNT ) return pmad;

    // With the new system using the action_map, this is all that is really necessary
    if ( ACTION_COUNT == pmad->action_map[actiona] )
    {
        if ( pmad->action_valid[actionb] )
        {
            pmad->action_map[actiona] = actionb;
        }
        else if ( ACTION_COUNT != pmad->action_map[actionb] )
        {
            pmad->action_map[actiona] = pmad->action_map[actionb];
        }
    }
    else if ( ACTION_COUNT == pmad->action_map[actionb] )
    {
        if ( pmad->action_valid[actiona] )
        {
            pmad->action_map[actionb] = actiona;
        }
        else if ( ACTION_COUNT != pmad->action_map[actiona] )
        {
            pmad->action_map[actionb] = pmad->action_map[actiona];
        }
    }

    //if ( pmad->action_valid[actiona] == pmad->action_valid[actionb] )
    //{
    //    // They are either both valid or both invalid, in either case we can't help
    //    return;
    //}
    //else
    //{
    //    // Fix the invalid one
    //    if ( !pmad->action_valid[actiona] )
    //    {
    //        // Fix actiona
    //        pmad->action_valid[actiona] = btrue;
    //        pmad->action_stt[actiona] = pmad->action_stt[actionb];
    //        pmad->action_end[actiona]   = pmad->action_end[actionb];
    //    }
    //    else
    //    {
    //        // Fix actionb
    //        pmad->action_valid[actionb] = btrue;
    //        pmad->action_stt[actionb] = pmad->action_stt[actiona];
    //        pmad->action_end[actionb]   = pmad->action_end[actiona];
    //    }
    //}

    return pmad;
}

//--------------------------------------------------------------------------------------------
ego_mad * mad_loader::apply_copy_file_vfs( ego_mad * pmad, const char* loadname )
{
    /// \author ZZ
    /// \details  This function copies a model's actions

    vfs_FILE *fileread;
    int actiona, actionb;
    char szOne[16] = EMPTY_CSTR;
    char szTwo[16] = EMPTY_CSTR;

    if ( NULL == pmad ) return pmad;

    fileread = vfs_openRead( loadname );
    if ( NULL == fileread ) return pmad;

    while ( goto_colon( NULL, fileread, btrue ) )
    {
        fget_string( fileread, szOne, SDL_arraysize( szOne ) );
        actiona = action_which( szOne[0] );

        fget_string( fileread, szTwo, SDL_arraysize( szTwo ) );
        actionb = action_which( szTwo[0] );

        pmad = copy_action_correct( pmad, actiona + 0, actionb + 0 );
        pmad = copy_action_correct( pmad, actiona + 1, actionb + 1 );
        pmad = copy_action_correct( pmad, actiona + 2, actionb + 2 );
        pmad = copy_action_correct( pmad, actiona + 3, actionb + 3 );
    }

    vfs_close( fileread );

    return pmad;
}

//--------------------------------------------------------------------------------------------
ego_mad * mad_loader::make_walk_lip( ego_mad * pmad, const int lip, const int action )
{
    /// \author ZZ
    /// \details  This helps make walking look right

    int frame = 0;
    int framesinaction, action_stt;
    int new_action = ACTION_DA;

    if ( NULL == pmad ) return pmad;

    new_action = mad_get_action( pmad, action );
    if ( ACTION_COUNT == new_action )
    {
        framesinaction = 1;
        action_stt     = pmad->action_stt[ACTION_DA];
    }
    else
    {
        framesinaction = pmad->action_end[new_action] - pmad->action_stt[new_action];
        action_stt     = pmad->action_stt[new_action];
    }

    for ( frame = 0; frame < 16; frame++ )
    {
        int framealong = 0;

        if ( framesinaction > 0 )
        {
            framealong = (( frame * framesinaction / 16 ) + 2 ) % framesinaction;
        }

        pmad->frameliptowalkframe[lip][frame] = action_stt + framealong;
    }

    return pmad;
}

//--------------------------------------------------------------------------------------------
ego_mad * mad_loader::make_framefx( ego_mad * pmad, const char * cFrameName, const int frame )
{
    /// \author ZZ
    /// \details  This function figures out the IFrame invulnerability, and Attack, Grab, and
    ///           Drop timings
    ///
    /// \note BB@> made a bit more sturdy parser that is not going to confuse strings like "LCRA"
    ///            which would not crop up if the convention of L or R going first was applied universally.
    ///            However, there are existing (and common) models which use the opposite convention, leading
    ///            to the possibility that an fx string "LARC" could be interpreted as ACTLEFT, CHARRIGHT, *and*
    ///            ACTRIGHT.

    BIT_FIELD fx = 0;
    char name_action[16], name_fx[16];
    int name_count;
    int fields;
    int cnt;

    static int token_count = -1;
    static const char * tokens[] = { "I", "S", "F", "P", "A", "G", "D", "C",          /* the normal command tokens */
                                     "LA", "LG", "LD", "LC", "RA", "RG", "RD", "RC", NULL
                                   }; /* the "bad" token aliases */

    const char * ptmp, * ptmp_end;
    char *paction, *paction_end;

    ego_MD2_Frame * pframe;

    if ( NULL == pmad || NULL == pmad->md2_ptr ) return pmad;

    // check for a valid frame number
    if ( frame >= md2_get_numFrames( pmad->md2_ptr ) ) return pmad;
    pframe = ( ego_MD2_Frame * )( md2_get_Frames( pmad->md2_ptr ) + frame );

    // this should only be initialized the first time through
    if ( token_count < 0 )
    {
        token_count = 0;
        for ( cnt = 0; NULL != tokens[token_count] && cnt < 256; cnt++ ) token_count++;
    }

    // set the default values
    fx = 0;
    pframe->framefx = fx;

    // check for a non-trivial frame name
    if ( !VALID_CSTR( cFrameName ) ) return pmad;

    // skip over whitespace
    ptmp     = cFrameName;
    ptmp_end = cFrameName + 16;
    for ( /* nothing */; ptmp < ptmp_end && SDL_isspace( *ptmp ); ptmp++ ) {};

    // copy non-numerical text
    paction     = name_action;
    paction_end = name_action + 16;
    for ( /* nothing */; ptmp < ptmp_end && paction < paction_end && !SDL_isspace( *ptmp ); ptmp++, paction++ )
    {
        if ( SDL_isdigit( *ptmp ) ) break;
        *paction = *ptmp;
    }
    if ( paction < paction_end ) *paction = CSTR_END;

    name_fx[0] = CSTR_END;
    fields = SDL_sscanf( ptmp, "%d %15s", &name_count, name_fx );
    name_action[15] = CSTR_END;
    name_fx[15] = CSTR_END;

    // check for a non-trivial fx command
    if ( !VALID_CSTR( name_fx ) ) return pmad;

    // scan the fx string for valid commands
    ptmp     = name_fx;
    ptmp_end = name_fx + 15;
    while ( CSTR_END != *ptmp && ptmp < ptmp_end )
    {
        size_t len;
        int token_index = -1;
        for ( cnt = 0; cnt < token_count; cnt++ )
        {
            len = SDL_strlen( tokens[cnt] );
            if ( 0 == strncmp( tokens[cnt], ptmp, len ) )
            {
                ptmp += len;
                token_index = cnt;
                break;
            }
        }

        if ( -1 == token_index )
        {
            // BB> disable this since the loader seems to be working perfectly fine as of 10-21-10
            // the problem is that models that are edited with certain editors pad the frame number with
            // zeros and that makes this interpreter think that there are excess unknown commands.

            //log_debug( "Model %s, frame %d, frame name \"%s\" has unknown frame effects command \"%s\"\n", szModelName, frame, cFrameName, ptmp );
            ptmp++;
        }
        else
        {
            bool_t bad_form = bfalse;
            switch ( token_index )
            {
                case  0: // "I" == invulnerable
                    ADD_BITS( fx, MADFX_INVICTUS );
                    break;

                case  1: // "S" == stop
                    ADD_BITS( fx, MADFX_STOP );
                    break;

                case  2: // "F" == footfall
                    ADD_BITS( fx, MADFX_FOOTFALL );
                    break;

                case  3: // "P" == poof
                    ADD_BITS( fx, MADFX_POOF );
                    break;

                case  4: // "A" == action

                    // get any modifiers
                    while (( CSTR_END != *ptmp && ptmp < ptmp_end ) && ( 'R' == *ptmp || 'L' == *ptmp ) )
                    {
                        ADD_BITS( fx, ( 'L' == *ptmp ) ? MADFX_ACTLEFT : MADFX_ACTRIGHT );
                        ptmp++;
                    }
                    break;

                case  5: // "G" == grab

                    // get any modifiers
                    while (( CSTR_END != *ptmp && ptmp < ptmp_end ) && ( 'R' == *ptmp || 'L' == *ptmp ) )
                    {
                        ADD_BITS( fx, ( 'L' == *ptmp ) ? MADFX_GRABLEFT : MADFX_GRABRIGHT );
                        ptmp++;
                    }
                    break;

                case  6: // "D" == drop

                    // get any modifiers
                    while (( CSTR_END != *ptmp && ptmp < ptmp_end ) && ( 'R' == *ptmp || 'L' == *ptmp ) )
                    {
                        fx |= ( 'L' == *ptmp ) ? MADFX_DROPLEFT : MADFX_DROPRIGHT;
                        ptmp++;
                    }
                    break;

                case  7: // "C" == grab a character

                    // get any modifiers
                    while (( CSTR_END != *ptmp && ptmp < ptmp_end ) && ( 'R' == *ptmp || 'L' == *ptmp ) )
                    {
                        ADD_BITS( fx, ( 'L' == *ptmp ) ? MADFX_CHARLEFT : MADFX_CHARRIGHT );
                        ptmp++;
                    }
                    break;

                case  8: // "LA"
                    bad_form = btrue;
                    ADD_BITS( fx, MADFX_ACTLEFT );
                    break;

                case  9: // "LG"
                    bad_form = btrue;
                    ADD_BITS( fx, MADFX_GRABLEFT );
                    break;

                case 10: // "LD"
                    bad_form = btrue;
                    ADD_BITS( fx, MADFX_DROPLEFT );
                    break;

                case 11: // "LC"
                    bad_form = btrue;
                    ADD_BITS( fx, MADFX_CHARLEFT );
                    break;

                case 12: // "RA"
                    bad_form = btrue;
                    ADD_BITS( fx, MADFX_ACTRIGHT );
                    break;

                case 13: // "RG"
                    bad_form = btrue;
                    ADD_BITS( fx, MADFX_GRABRIGHT );
                    break;

                case 14: // "RD"
                    bad_form = btrue;
                    ADD_BITS( fx, MADFX_DROPRIGHT );
                    break;

                case 15: // "RC"
                    bad_form = btrue;
                    ADD_BITS( fx, MADFX_CHARRIGHT );
                    break;
            }

            if ( bad_form && -1 != token_index )
            {
                log_warning( "Model %s, frame %d, frame name \"%s\" has a frame effects command in an improper configuration \"%s\"\n", szModelName, frame, cFrameName, tokens[token_index] );
            }
        }
    }

    pframe->framefx = fx;

    return pmad;
}

//--------------------------------------------------------------------------------------------
ego_mad * mad_loader::make_frame_lip( ego_mad * pmad, const int action )
{
    /// \author ZZ
    /// \details  This helps make walking look right

    int frame_count, frame, framesinaction;

    ego_MD2_Model * md2;
    ego_MD2_Frame * frame_list, * pframe;

    if ( NULL == pmad ) return pmad;

    md2 = pmad->md2_ptr;
    if ( NULL == md2 ) return pmad;

    int loc_action = mad_get_action( pmad, action );
    if ( ACTION_COUNT == loc_action || ACTION_DA == loc_action ) return pmad;

    if ( !pmad->action_valid[loc_action] ) return pmad;

    frame_count = md2_get_numFrames( md2 );
    frame_list  = ( ego_MD2_Frame * )md2_get_Frames( md2 );

    framesinaction = pmad->action_end[loc_action] - pmad->action_stt[loc_action];

    for ( frame = pmad->action_stt[loc_action]; frame < pmad->action_end[loc_action]; frame++ )
    {
        if ( frame > frame_count ) continue;
        pframe = frame_list + frame;

        pframe->framelip = ( frame - pmad->action_stt[loc_action] ) * 15 / framesinaction;
        pframe->framelip = ( pframe->framelip ) & 15;
    }

    return pmad;
}

//--------------------------------------------------------------------------------------------
ego_mad * mad_loader::heal_actions( ego_mad * pmad, const char * tmploadname )
{
    STRING newloadname;

    if ( NULL == pmad ) return pmad;

    // Make sure actions are made valid if a similar one exists
    pmad = copy_action_correct( pmad, ACTION_DA, ACTION_DB );  // All dances should be safe
    pmad = copy_action_correct( pmad, ACTION_DB, ACTION_DC );
    pmad = copy_action_correct( pmad, ACTION_DC, ACTION_DD );
    pmad = copy_action_correct( pmad, ACTION_DB, ACTION_DC );
    pmad = copy_action_correct( pmad, ACTION_DA, ACTION_DB );
    pmad = copy_action_correct( pmad, ACTION_UA, ACTION_UB );
    pmad = copy_action_correct( pmad, ACTION_UB, ACTION_UC );
    pmad = copy_action_correct( pmad, ACTION_UC, ACTION_UD );
    pmad = copy_action_correct( pmad, ACTION_TA, ACTION_TB );
    pmad = copy_action_correct( pmad, ACTION_TC, ACTION_TD );
    pmad = copy_action_correct( pmad, ACTION_CA, ACTION_CB );
    pmad = copy_action_correct( pmad, ACTION_CC, ACTION_CD );
    pmad = copy_action_correct( pmad, ACTION_SA, ACTION_SB );
    pmad = copy_action_correct( pmad, ACTION_SC, ACTION_SD );
    pmad = copy_action_correct( pmad, ACTION_BA, ACTION_BB );
    pmad = copy_action_correct( pmad, ACTION_BC, ACTION_BD );
    pmad = copy_action_correct( pmad, ACTION_LA, ACTION_LB );
    pmad = copy_action_correct( pmad, ACTION_LC, ACTION_LD );
    pmad = copy_action_correct( pmad, ACTION_XA, ACTION_XB );
    pmad = copy_action_correct( pmad, ACTION_XC, ACTION_XD );
    pmad = copy_action_correct( pmad, ACTION_FA, ACTION_FB );
    pmad = copy_action_correct( pmad, ACTION_FC, ACTION_FD );
    pmad = copy_action_correct( pmad, ACTION_PA, ACTION_PB );
    pmad = copy_action_correct( pmad, ACTION_PC, ACTION_PD );
    pmad = copy_action_correct( pmad, ACTION_ZA, ACTION_ZB );
    pmad = copy_action_correct( pmad, ACTION_ZC, ACTION_ZD );
    pmad = copy_action_correct( pmad, ACTION_WA, ACTION_WB );
    pmad = copy_action_correct( pmad, ACTION_WB, ACTION_WC );
    pmad = copy_action_correct( pmad, ACTION_WC, ACTION_WD );
    pmad = copy_action_correct( pmad, ACTION_DA, ACTION_WD );  // All walks should be safe
    pmad = copy_action_correct( pmad, ACTION_WC, ACTION_WD );
    pmad = copy_action_correct( pmad, ACTION_WB, ACTION_WC );
    pmad = copy_action_correct( pmad, ACTION_WA, ACTION_WB );
    pmad = copy_action_correct( pmad, ACTION_JA, ACTION_JB );
    pmad = copy_action_correct( pmad, ACTION_JB, ACTION_JC );
    pmad = copy_action_correct( pmad, ACTION_DA, ACTION_JC );  // All jumps should be safe
    pmad = copy_action_correct( pmad, ACTION_JB, ACTION_JC );
    pmad = copy_action_correct( pmad, ACTION_JA, ACTION_JB );
    pmad = copy_action_correct( pmad, ACTION_HA, ACTION_HB );
    pmad = copy_action_correct( pmad, ACTION_HB, ACTION_HC );
    pmad = copy_action_correct( pmad, ACTION_HC, ACTION_HD );
    pmad = copy_action_correct( pmad, ACTION_HB, ACTION_HC );
    pmad = copy_action_correct( pmad, ACTION_HA, ACTION_HB );
    pmad = copy_action_correct( pmad, ACTION_KA, ACTION_KB );
    pmad = copy_action_correct( pmad, ACTION_KB, ACTION_KC );
    pmad = copy_action_correct( pmad, ACTION_KC, ACTION_KD );
    pmad = copy_action_correct( pmad, ACTION_KB, ACTION_KC );
    pmad = copy_action_correct( pmad, ACTION_KA, ACTION_KB );
    pmad = copy_action_correct( pmad, ACTION_MH, ACTION_MI );
    pmad = copy_action_correct( pmad, ACTION_DA, ACTION_MM );
    pmad = copy_action_correct( pmad, ACTION_MM, ACTION_MN );

    // Copy entire actions to save frame space COPY.TXT
    make_newloadname( tmploadname, "/copy.txt", newloadname );
    apply_copy_file_vfs( pmad, newloadname );

    return pmad;
}

//--------------------------------------------------------------------------------------------
ego_mad * mad_loader::finalize( ego_mad * pmad )
{
    int frame, frame_count;

    ego_MD2_Model * pmd2;
    ego_MD2_Frame * frame_list;

    if ( NULL == pmad ) return pmad;

    pmd2 = pmad->md2_ptr;
    if ( NULL == pmd2 ) return pmad;

    frame_count = md2_get_numFrames( pmd2 );
    frame_list  = ( ego_MD2_Frame * )md2_get_Frames( pmd2 );

    // Create table for doing transition from one type of walk to another...
    // Clear 'em all to start
    for ( frame = 0; frame < frame_count; frame++ )
    {
        frame_list[frame].framelip = 0;
    }

    // Need to figure out how far into action each frame is
    pmad = make_frame_lip( pmad, ACTION_WA );
    pmad = make_frame_lip( pmad, ACTION_WB );
    pmad = make_frame_lip( pmad, ACTION_WC );

    // Now do the same, in reverse, for walking animations
    pmad = make_walk_lip( pmad, LIPDA, ACTION_DA );
    pmad = make_walk_lip( pmad, LIPWA, ACTION_WA );
    pmad = make_walk_lip( pmad, LIPWB, ACTION_WB );
    pmad = make_walk_lip( pmad, LIPWC, ACTION_WC );

    return pmad;
}

//--------------------------------------------------------------------------------------------
ego_mad * mad_loader::make_actions( ego_mad * pmad )
{
    /// \author ZZ
    /// \details  This function creates the iframe lists for each this_action based on the
    ///    name of each md2 iframe in the model

    int frame_count, iframe;
    int this_action, last_action;

    ego_MD2_Model    * pmd2;
    ego_MD2_Frame    * frame_list;

    if ( NULL == pmad ) return pmad;

    pmd2 = pmad->md2_ptr;
    if ( NULL == pmd2 ) return pmad;

    // Clear out all actions and reset to invalid
    for ( this_action = 0; this_action < ACTION_COUNT; this_action++ )
    {
        pmad->action_map[this_action]   = ACTION_COUNT;
        pmad->action_stt[this_action]   = -1;
        pmad->action_end[this_action]   = -1;
        pmad->action_valid[this_action] = bfalse;
    }

    // grab the frame info from the md2
    frame_count = md2_get_numFrames( pmd2 );
    frame_list  = ( ego_MD2_Frame * )md2_get_Frames( pmd2 );

    // is there anything to do?
    if ( 0 == frame_count ) return pmad;

    // Make a default dance action (ACTION_DA) to be the 1st frame of the animation
    pmad->action_map[ACTION_DA]   = ACTION_DA;
    pmad->action_valid[ACTION_DA] = btrue;
    pmad->action_stt[ACTION_DA]   = 0;
    pmad->action_end[ACTION_DA]   = 0;

    // Now go huntin' to see what each iframe is, look for runs of same this_action
    last_action = ACTION_COUNT;
    for ( iframe = 0; iframe < frame_count; iframe++ )
    {
        this_action = get_action_index( frame_list[iframe].name );

        if ( last_action != this_action )
        {
            // start a new action
            pmad->action_map[this_action]   = this_action;
            pmad->action_stt[this_action]   = iframe;
            pmad->action_end[this_action]   = iframe;
            pmad->action_valid[this_action] = btrue;

            last_action = this_action;
        }
        else
        {
            // keep expanding the action_end until the end of the action
            pmad->action_end[this_action] = iframe;
        }

        make_framefx( pmad, frame_list[iframe].name, iframe );
    }

    return pmad;
}

//--------------------------------------------------------------------------------------------
int mad_loader::get_action_index( const char * cFrameName )
{
    /// \author ZZ
    /// \details  This function returns the number of the action in cFrameName, or
    ///    it returns NOACTION if it could not find a match

    int cnt;
    char tmp_str[16];
    char first, second;

    SDL_sscanf( cFrameName, " %15s", tmp_str );

    first  = tmp_str[0];
    second = tmp_str[1];

    for ( cnt = 0; cnt < ACTION_COUNT; cnt++ )
    {
        if ( first == cActionName[cnt][0] && second == cActionName[cnt][1] )
        {
            return cnt;
        }
    }

    return NOACTION;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
int mad_get_action( const ego_mad * pmad, const int action )
{
    /// \author BB
    /// \details  translate the action that was given into a valid action for the model.
    /// returns ACTION_COUNT on a complete failure, or the default ACTION_DA if it exists

    int     retval;

    if ( NULL == pmad ) return ACTION_COUNT;

    // you are pretty much guaranteed that ACTION_DA will be valid for a model,
    // I guess it could be invalid if the model had no frames or something
    retval = ACTION_DA;
    if ( !pmad->action_valid[ACTION_DA] )
    {
        retval = ACTION_COUNT;
    }

    // check for a valid action range
    if ( action < 0 || action > ACTION_COUNT ) return retval;

    // track down a valid value
    if ( pmad->action_valid[action] )
    {
        retval = action;
    }
    else if ( ACTION_COUNT != pmad->action_map[action] )
    {
        int cnt, tnc;

        // do a "recursive" search for a valid action
        // we should never really have to check more than once if the map is prepared
        // properly BUT you never can tell. Make sure we do not get a runaway loop by
        // you never go farther than ACTION_COUNT steps and that you never see the
        // original action again

        tnc = pmad->action_map[action];
        for ( cnt = 0; cnt < ACTION_COUNT; cnt++ )
        {
            if ( tnc >= ACTION_COUNT || tnc < 0 || tnc == action ) break;

            if ( pmad->action_valid[tnc] )
            {
                retval = tnc;
                break;
            }
        }
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
int mad_get_action( const MAD_REF & imad, const int action )
{
    /// \author BB
    /// \details version that accepts a reference rather than a pointer

    if ( !LOADED_MAD( imad ) ) return ACTION_COUNT;

    return mad_get_action( MadStack + imad, action );
}

//--------------------------------------------------------------------------------------------
BIT_FIELD mad_get_actionfx( const MAD_REF & imad, const int action )
{
    BIT_FIELD retval = EMPTY_BIT_FIELD;
    int cnt;
    ego_mad * pmad;
    ego_MD2_Model * md2;
    ego_MD2_Frame * frame_lst, * pframe;

    if ( !LOADED_MAD( imad ) ) return 0;
    pmad = MadStack + imad;

    md2 = MadStack[imad].md2_ptr;
    if ( NULL == md2 ) return 0;

    if ( action < 0 || action >= ACTION_COUNT ) return 0;

    if ( !pmad->action_valid[action] ) return 0;

    frame_lst = ( ego_MD2_Frame * )md2_get_Frames( md2 );
    for ( cnt = pmad->action_stt[action]; cnt <= pmad->action_end[action]; cnt++ )
    {
        pframe = frame_lst + cnt;

        ADD_BITS( retval, pframe->framefx );
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
int action_which( char cTmp )
{
    /// \author ZZ
    /// \details  This function changes a letter into an action code
    int action;

    switch ( SDL_toupper( cTmp ) )
    {
            /// \note ZF@> Attack animation WALK is used for doing nothing (for example charging spells)
            ///            Make it default to ACTION_DA in this case.
        case 'W': action = ACTION_DA; break;
        case 'D': action = ACTION_DA; break;
        case 'U': action = ACTION_UA; break;
        case 'T': action = ACTION_TA; break;
        case 'C': action = ACTION_CA; break;
        case 'S': action = ACTION_SA; break;
        case 'B': action = ACTION_BA; break;
        case 'L': action = ACTION_LA; break;
        case 'X': action = ACTION_XA; break;
        case 'F': action = ACTION_FA; break;
        case 'P': action = ACTION_PA; break;
        case 'Z': action = ACTION_ZA; break;
        case 'H': action = ACTION_HA; break;
        case 'K': action = ACTION_KA; break;
        default:  action = ACTION_DA; break;
    }

    return action;
}

//--------------------------------------------------------------------------------------------
void mad_make_equally_lit( const MAD_REF & imad )
{
    /// \author ZZ
    /// \details  This function makes ultra low poly models look better

    int cnt, vert;
    ego_MD2_Model * md2;
    int frame_count, vert_count;

    if ( !LOADED_MAD( imad ) ) return;

    md2         = MadStack[imad].md2_ptr;
    frame_count = md2_get_numFrames( md2 );
    vert_count  = md2_get_numVertices( md2 );

    for ( cnt = 0; cnt < frame_count; cnt++ )
    {
        ego_MD2_Frame * pframe = ( ego_MD2_Frame * )md2_get_Frames( md2 );
        for ( vert = 0; vert < vert_count; vert++ )
        {
            pframe->vertex_lst[vert].normal = EGO_AMBIENT_INDEX;
        }
    }
}

//--------------------------------------------------------------------------------------------
void load_action_names_vfs( const char* loadname )
{
    /// \author ZZ
    /// \details  This function loads all of the 2 letter action names

    vfs_FILE* fileread;
    int cnt;

    char first = CSTR_END, second = CSTR_END;
    STRING comment;
    bool_t found;

    fileread = vfs_openRead( loadname );
    if ( !fileread ) return;

    for ( cnt = 0; cnt < ACTION_COUNT; cnt++ )
    {
        comment[0] = CSTR_END;

        found = bfalse;
        if ( goto_colon( NULL, fileread, bfalse ) )
        {
            if ( vfs_scanf( fileread, " %c%c %s", &first, &second, &comment ) >= 2 )
            {
                found = btrue;
            }
        }

        if ( found )
        {
            cActionName[cnt][0] = first;
            cActionName[cnt][1] = second;
            cActionComent[cnt][0] = CSTR_END;

            if ( VALID_CSTR( comment ) )
            {
                strncpy( cActionComent[cnt], comment, SDL_arraysize( cActionComent[cnt] ) );
                cActionComent[cnt][255] = CSTR_END;
            }
        }
        else
        {
            cActionName[cnt][0] = CSTR_END;
            cActionComent[cnt][0] = CSTR_END;
        }
    }

    vfs_close( fileread );
}

//--------------------------------------------------------------------------------------------
int mad_randomize_action( const int src_action, const int slot )
{
    /// \author BB
    /// \details  this function actually determines whether the action fillows the
    ///               pattern of ACTION_?A, ACTION_?B, ACTION_?C, ACTION_?D, with
    ///               A and B being for the left hand, and C and D being for the right hand

    int diff = 0;

    int dst_action = src_action;

    // a valid slot?
    if ( slot < 0 || slot >= SLOT_COUNT ) return dst_action;

    // a valid dst_action?
    if ( dst_action < 0 || dst_action >= ACTION_COUNT ) return bfalse;

    diff = slot * 2;

    //---- non-randomizable actions
    if ( ACTION_MG == dst_action ) return dst_action;       // MG      = Open Chest
    else if ( ACTION_MH == dst_action ) return dst_action;       // MH      = Sit
    else if ( ACTION_MI == dst_action ) return dst_action;       // MI      = Ride
    else if ( ACTION_MJ == dst_action ) return dst_action;       // MJ      = Object Activated
    else if ( ACTION_MK == dst_action ) return dst_action;       // MK      = Snoozing
    else if ( ACTION_ML == dst_action ) return dst_action;       // ML      = Unlock
    else if ( ACTION_JA == dst_action ) return dst_action;       // JA      = Jump
    else if ( ACTION_RA == dst_action ) return dst_action;       // RA      = Roll
    else if ( ACTION_IS_TYPE( dst_action, W ) ) return dst_action;  // WA - WD = Walk

    //---- do a couple of special actions that have left/right
    else if ( ACTION_EA == dst_action || ACTION_EB == dst_action ) dst_action = ACTION_JB + slot;   // EA/EB = Evade left/right
    else if ( ACTION_JB == dst_action || ACTION_JC == dst_action ) dst_action = ACTION_JB + slot;   // JB/JC = Dropped item left/right
    else if ( ACTION_MA == dst_action || ACTION_MB == dst_action ) dst_action = ACTION_MA + slot;   // MA/MB = Drop left/right item
    else if ( ACTION_MC == dst_action || ACTION_MD == dst_action ) dst_action = ACTION_MC + slot;   // MC/MD = Slam left/right
    else if ( ACTION_ME == dst_action || ACTION_MF == dst_action ) dst_action = ACTION_ME + slot;   // ME/MF = Grab item left/right
    else if ( ACTION_MM == dst_action || ACTION_MN == dst_action ) dst_action = ACTION_MM + slot;   // MM/MN = Held left/right

    //---- actions that can be randomized, but are not left/right sensitive
    // D = dance
    else if ( ACTION_IS_TYPE( dst_action, D ) ) dst_action = ACTION_TYPE( D ) + generate_randmask( 0, 3 );

    //---- handle all the normal attack/defense animations
    // U = unarmed
    else if ( ACTION_IS_TYPE( dst_action, U ) ) dst_action = ACTION_TYPE( U ) + diff + generate_randmask( 0, 1 );
    // T = thrust
    else if ( ACTION_IS_TYPE( dst_action, T ) ) dst_action = ACTION_TYPE( T ) + diff + generate_randmask( 0, 1 );
    // C = chop
    else if ( ACTION_IS_TYPE( dst_action, C ) ) dst_action = ACTION_TYPE( C ) + diff + generate_randmask( 0, 1 );
    // S = slice
    else if ( ACTION_IS_TYPE( dst_action, S ) ) dst_action = ACTION_TYPE( S ) + diff + generate_randmask( 0, 1 );
    // B = bash
    else if ( ACTION_IS_TYPE( dst_action, B ) ) dst_action = ACTION_TYPE( B ) + diff + generate_randmask( 0, 1 );
    // L = longbow
    else if ( ACTION_IS_TYPE( dst_action, L ) ) dst_action = ACTION_TYPE( L ) + diff + generate_randmask( 0, 1 );
    // X = crossbow
    else if ( ACTION_IS_TYPE( dst_action, X ) ) dst_action = ACTION_TYPE( X ) + diff + generate_randmask( 0, 1 );
    // F = fling
    else if ( ACTION_IS_TYPE( dst_action, F ) ) dst_action = ACTION_TYPE( F ) + diff + generate_randmask( 0, 1 );
    // P = parry/block
    else if ( ACTION_IS_TYPE( dst_action, P ) ) dst_action = ACTION_TYPE( P ) + diff + generate_randmask( 0, 1 );
    // Z = zap
    else if ( ACTION_IS_TYPE( dst_action, Z ) ) dst_action = ACTION_TYPE( Z ) + diff + generate_randmask( 0, 1 );

    //---- these are passive actions
    // H = hurt
    else if ( ACTION_IS_TYPE( dst_action, H ) ) dst_action = ACTION_TYPE( H ) + generate_randmask( 0, 3 );
    // K = killed
    else if ( ACTION_IS_TYPE( dst_action, K ) ) dst_action = ACTION_TYPE( K ) + generate_randmask( 0, 3 );

    return dst_action;
}
