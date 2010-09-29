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

/// @file script.c
/// @brief Implements the game's scripting language.
/// @details

#include "script.h"
#include "script_compile.h"
#include "script_functions.h"

#include "char.inl"
#include "mad.h"
#include "profile.inl"
#include "mesh.inl"

#include "log.h"
#include "camera.h"
#include "game.h"
#include "network.h"

#include "egoboo_vfs.h"
#include "egoboo_setup.h"
#include "egoboo_strutil.h"

#include "egoboo_math.inl"

#include <assert.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static const char *  script_error_classname = "UNKNOWN";
static PRO_REF       script_error_model     = ( PRO_REF )MAX_PROFILE;
static const char *  script_error_name      = "UNKNOWN";
static REF_T         script_error_index     = ( Uint16 )( ~0 );

static bool_t scr_increment_exe( ai_state_t * pself );
static bool_t scr_set_exe( ai_state_t * pself, size_t offset );

// static Uint8 run_function_obsolete( script_state_t * pstate, ai_state_t * pself );
static Uint8 scr_run_function( script_state_t * pstate, ai_state_bundle_t * pself );
static void  scr_set_operand( script_state_t * pstate, Uint8 variable );
static void  scr_run_operand( script_state_t * pstate, ai_state_bundle_t * pself );

static bool_t scr_run_operation( script_state_t * pstate, ai_state_bundle_t * pself );
static bool_t scr_run_function_call( script_state_t * pstate, ai_state_bundle_t * pself );

PROFILE_DECLARE( script_function )

static int    _script_function_calls[SCRIPT_FUNCTIONS_COUNT];
static double _script_function_times[SCRIPT_FUNCTIONS_COUNT];

static bool_t _scripting_system_initialized = bfalse;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void scripting_system_begin()
{
    if ( !_scripting_system_initialized )
    {
        int cnt;

        PROFILE_INIT( script_function );

        for ( cnt = 0; cnt < SCRIPT_FUNCTIONS_COUNT; cnt++ )
        {
            _script_function_calls[cnt] = 0;
            _script_function_times[cnt] = 0.0;
        }

        _scripting_system_initialized = btrue;
    }
}

//--------------------------------------------------------------------------------------------
void scripting_system_end()
{
    if ( _scripting_system_initialized )
    {
        PROFILE_FREE( script_function );

#if (DEBUG_SCRIPT_LEVEL > 1 ) && defined(DEBUG_PROFILE) && EGO_DEBUG
        {
            FILE * ftmp = fopen( vfs_resolveWriteFilename( "/debug/script_function_timing.txt" ), "a+" );

            if ( NULL != ftmp )
            {
                int cnt;

                for ( cnt = 0; cnt < SCRIPT_FUNCTIONS_COUNT; cnt++ )
                {
                    if ( _script_function_calls[cnt] > 0 )
                    {
                        fprintf( ftmp, "function == %d\tname == \"%s\"\tcalls == %d\ttime == %lf\n",
                                 cnt, script_function_names[cnt], _script_function_calls[cnt], _script_function_times[cnt] );
                    }
                }

                fflush( ftmp );
                fclose( ftmp );
            }
        }
#endif

        _scripting_system_initialized = bfalse;
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void scr_run_chr_script( ai_state_bundle_t * pbdl_ai )
{
    /// @details ZZ@> This function lets one character do AI stuff

    script_state_t    my_state;

    ai_state_t * pai;
    chr_t      * pchr;

    // make sure that the scripting module is initialized
    scripting_system_begin();

    if ( NULL == pbdl_ai || NULL == pbdl_ai->chr_ptr ) return;

    // alias a couple of pointers to make the notation easier to read
    pai  = pbdl_ai->ai_state_ptr;
    pchr = pbdl_ai->chr_ptr;

    // clear the latches on every single object
    pchr->latch.raw_valid = bfalse;
    fvec2_self_clear( pchr->latch.raw.dir );

    pchr->latch.trans_valid = bfalse;
    fvec3_self_clear( pchr->latch.trans.dir );

    // has the time for this pbdl_ai->chr_ref to die come and gone?
    if ( pai->poof_time >= 0 && pai->poof_time <= ( Sint32 )update_wld ) return;

    // grab the "changed" value from the last time the script was run
    if ( pai->changed )
    {
        ADD_BITS( pai->alert, ALERTIF_CHANGED );
        pai->changed = bfalse;
    }

    PROFILE_BEGIN_STRUCT( pai );

    // debug a certain script
    // debug_scripts = ( pai->index == 385 && pchr->profile_ref == 76 );

    // target_old is set to the target every time the script is run
    pai->target_old = pai->target;

    // Make life easier
    script_error_classname = "UNKNOWN";
    script_error_model     = pchr->profile_ref;
    script_error_index     = ( Uint16 )( ~0 );
    script_error_name      = "UNKNOWN";
    if ( script_error_model < MAX_PROFILE )
    {
        script_error_classname = pbdl_ai->cap_ptr->classname;

        script_error_index = pbdl_ai->ai_state_ref;
        if ( script_error_index < MAX_AI )
        {
            script_error_name = AisStorage.ary[script_error_index].szName;
        }
    }

    if ( debug_scripts )
    {
        FILE * scr_file = ( NULL == debug_script_file ) ? stdout : debug_script_file;

        fprintf( scr_file,  "\n\n--------\n%d - %s\n", script_error_index, script_error_name );
        fprintf( scr_file,  "%d - %s\n", REF_TO_INT( script_error_model ), script_error_classname );

        // who are we related to?
        fprintf( scr_file,  "\tindex  == %d\n", REF_TO_INT( pai->index ) );
        fprintf( scr_file,  "\ttarget == %d\n", REF_TO_INT( pai->target ) );
        fprintf( scr_file,  "\towner  == %d\n", REF_TO_INT( pai->owner ) );
        fprintf( scr_file,  "\tchild  == %d\n", REF_TO_INT( pai->child ) );

        // some local storage
        fprintf( scr_file,  "\talert     == %x\n", pai->alert );
        fprintf( scr_file,  "\tstate     == %d\n", pai->state );
        fprintf( scr_file,  "\tcontent   == %d\n", pai->content );
        fprintf( scr_file,  "\ttimer     == %d\n", pai->timer );
        fprintf( scr_file,  "\tupdate_wld == %d\n", update_wld );

        // ai memory from the last event
        fprintf( scr_file,  "\tbumplast       == %d\n", REF_TO_INT( pai->bumplast ) );
        fprintf( scr_file,  "\tattacklast     == %d\n", REF_TO_INT( pai->attacklast ) );
        fprintf( scr_file,  "\thitlast        == %d\n", REF_TO_INT( pai->hitlast ) );
        fprintf( scr_file,  "\tdirectionlast  == %d\n", pai->directionlast );
        fprintf( scr_file,  "\tdamagetypelast == %d\n", pai->damagetypelast );
        fprintf( scr_file,  "\tlastitemused   == %d\n", REF_TO_INT( pai->lastitemused ) );
        fprintf( scr_file,  "\ttarget_old     == %d\n", REF_TO_INT( pai->target_old ) );

        // message handling
        fprintf( scr_file,  "\torder == %d\n", pai->order_value );
        fprintf( scr_file,  "\tcounter == %d\n", pai->order_counter );

        // waypoints
        fprintf( scr_file,  "\twp_tail == %d\n", pai->wp_lst.tail );
        fprintf( scr_file,  "\twp_head == %d\n\n", pai->wp_lst.head );
    }

    // Clear the button latches
    if ( !VALID_PLA( pchr->is_which_player ) )
    {
        CLEAR_BIT_FIELD( pchr->latch.trans.b );
    }

    // Reset the target if it can't be seen
    if (( pai->target != MAX_CHR ) && ( pai->target != pai->index ) && !chr_can_see_object( pbdl_ai->chr_ref, pai->target ) )
    {
        pai->target = pai->index;
    }

    // reset the script state
    memset( &my_state, 0, sizeof( my_state ) );

    // reset the ai
    pai->terminate = bfalse;
    pai->indent    = 0;
    pai->exe_stt   = AisStorage.ary[pai->type].iStartPosition;
    pai->exe_end   = AisStorage.ary[pai->type].iEndPosition;

    // Run the AI Script
    scr_set_exe( pai, pai->exe_stt );
    while ( !pai->terminate && pai->exe_pos < pai->exe_end )
    {
        // This is used by the Else function
        // it only keeps track of functions
        pai->indent_last = pai->indent;
        pai->indent = GET_DATA_BITS( pai->opcode );

        // Was it a function
        if ( HAS_SOME_BITS( pai->opcode, FUNCTION_BIT ) )
        {
            if ( !scr_run_function_call( &my_state, pbdl_ai ) )
            {
                break;
            }
        }
        else
        {
            if ( !scr_run_operation( &my_state, pbdl_ai ) )
            {
                break;
            }
        }
    }

    // Set the oject's latches, but only if it is "alive"
    if ( pchr->alive && !VALID_PLA( pchr->is_which_player ) )
    {
        float scale;
        CHR_REF rider_ref = pchr->holdingwhich[SLOT_LEFT];

        ai_state_ensure_wp( pai );

        if ( pchr->ismount && INGAME_CHR( rider_ref ) )
        {
            chr_t * prider = ChrList.lst + rider_ref;

            // Mount
            pchr->latch.raw_valid = prider->latch.raw_valid;
            pchr->latch.raw       = prider->latch.raw;

            pchr->latch.trans_valid = prider->latch.trans_valid;
            pchr->latch.trans       = prider->latch.trans;
        }
        else if ( pai->wp_valid )
        {
            // Normal AI
            fvec3_t vec_tmp;
            pchr->latch.raw_valid = bfalse;
            fvec2_self_clear( pchr->latch.raw.dir );

            vec_tmp = fvec3_sub( pai->wp, pchr->pos.v );
            pchr->latch.trans_valid   = btrue;
            pchr->latch.trans.dir[kX] = vec_tmp.v[kX];
            pchr->latch.trans.dir[kY] = vec_tmp.v[kY];
            pchr->latch.trans.dir[kZ] = vec_tmp.v[kZ];
        }

        scale = 1.0f;
        if ( pchr->latch.trans_valid && fvec3_length_abs( pchr->latch.trans.dir ) > 0.0f )
        {
            float horiz_len2 = pchr->latch.trans.dir[kX] * pchr->latch.trans.dir[kX] + pchr->latch.trans.dir[kY] * pchr->latch.trans.dir[kY];
            float vert_len2  = pchr->latch.trans.dir[kZ] * pchr->latch.trans.dir[kZ];

            if ( vert_len2 > 1.0f && vert_len2 >= horiz_len2 )
            {
                scale = 1.0f / ABS( pchr->latch.trans.dir[kZ] );
            }
            else if ( horiz_len2 > 1.0f && horiz_len2 > vert_len2 )
            {
                scale = 1.0f / SQRT( horiz_len2 );
            }
        }

        if ( 1.0f != scale )
        {
            fvec3_self_scale( pchr->latch.trans.dir, scale );
        }
    }

    // Clear alerts for next time around
    CLEAR_BIT_FIELD( pai->alert );

    PROFILE_END2_STRUCT( pai );
}

//--------------------------------------------------------------------------------------------
bool_t scr_run_function_call( script_state_t * pstate, ai_state_bundle_t * pbdl_ai )
{
    Uint8  functionreturn;
    ai_state_t * pself;

    // check for valid pointers
    if ( NULL == pstate || NULL == pbdl_ai || NULL == pbdl_ai->ai_state_ptr ) return bfalse;

    // use aliases to simplify the notation
    pself = pbdl_ai->ai_state_ptr;

    // check for valid execution pointer
    if ( pself->exe_pos < pself->exe_stt || pself->exe_pos >= pself->exe_end ) return bfalse;

    // Run the function
    functionreturn = scr_run_function( pstate, pbdl_ai );

    // move the execution pointer to the jump code
    scr_increment_exe( pself );
    if ( functionreturn )
    {
        // move the execution pointer to the next opcode
        scr_increment_exe( pself );
    }
    else
    {
        // use the jump code to jump to the right location
        size_t new_index = pself->opcode;

        // make sure the value is valid
        EGOBOO_ASSERT( new_index < AISMAXCOMPILESIZE && new_index >= pself->exe_stt && new_index <= pself->exe_end );

        // actually do the jump
        scr_set_exe( pself, new_index );
    }

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t scr_run_operation( script_state_t * pstate, ai_state_bundle_t * pbdl_ai )
{
    const char * variable;
    Uint32 var_value, operand_count, i;
    ai_state_t * pself;

    // check for valid pointers
    if ( NULL == pstate || NULL == pbdl_ai || NULL == pbdl_ai->ai_state_ptr ) return bfalse;

    // use aliases to simplify the notation
    pself = pbdl_ai->ai_state_ptr;

    // check for valid execution pointer
    if ( pself->exe_pos < pself->exe_stt || pself->exe_pos >= pself->exe_end ) return bfalse;

    var_value = pself->opcode & VALUE_BITS;

    // debug stuff
    variable = "UNKNOWN";
    if ( debug_scripts )
    {
        FILE * scr_file = ( NULL == debug_script_file ) ? stdout : debug_script_file;

        for ( i = 0; i < pself->indent; i++ ) { fprintf( scr_file, "  " ); }

        for ( i = 0; i < MAX_OPCODE; i++ )
        {
            if ( 'V' == OpList.ary[i].cType && var_value == OpList.ary[i].iValue )
            {
                variable = OpList.ary[i].cName;
                break;
            };
        }

        fprintf( scr_file, "%s = ", variable );
    }

    // Get the number of operands
    scr_increment_exe( pself );
    operand_count = pself->opcode;

    // Now run the operation
    pstate->operationsum = 0;
    for ( i = 0; i < operand_count && pself->exe_pos < pself->exe_end; i++ )
    {
        scr_increment_exe( pself );
        scr_run_operand( pstate, pbdl_ai );
    }
    if ( debug_scripts )
    {
        FILE * scr_file = ( NULL == debug_script_file ) ? stdout : debug_script_file;
        fprintf( scr_file, " == %d \n", pstate->operationsum );
    }

    // Save the results in the register that called the arithmetic
    scr_set_operand( pstate, var_value );

    // go to the next opcode
    scr_increment_exe( pself );

    return btrue;
}

//--------------------------------------------------------------------------------------------
Uint8 scr_run_function( script_state_t * pstate, ai_state_bundle_t * pbdl_ai )
{
    /// @details BB@> This is about half-way to what is needed for Lua integration

    // Mask out the indentation
    Uint32       valuecode;
    Uint8        returncode;
    ai_state_t * pself;

    if ( NULL == pstate || NULL == pbdl_ai || NULL == pbdl_ai->ai_state_ptr ) return bfalse;

    // use aliases to simplify the notation
    pself = pbdl_ai->ai_state_ptr;

    valuecode = pself->opcode & VALUE_BITS;

    // Assume that the function will pass, as most do
    returncode = btrue;
    if ( MAX_OPCODE == valuecode )
    {
        log_message( "SCRIPT ERROR: scr_run_function() - model == %d, class name == \"%s\" - Unknown opcode found!\n", REF_TO_INT( script_error_model ), script_error_classname );
        return bfalse;
    }

    // debug stuff
    if ( debug_scripts )
    {
        Uint32 i;
        FILE * scr_file = ( NULL == debug_script_file ) ? stdout : debug_script_file;

        for ( i = 0; i < pself->indent; i++ ) { fprintf( scr_file,  "  " ); }

        for ( i = 0; i < MAX_OPCODE; i++ )
        {
            if ( 'F' == OpList.ary[i].cType && valuecode == OpList.ary[i].iValue )
            {
                fprintf( scr_file,  "%s\n", OpList.ary[i].cName );
                break;
            };
        }
    }

    if ( valuecode > SCRIPT_FUNCTIONS_COUNT )
    {
    }
    else
    {
        PROFILE_RESET( script_function );

        PROFILE_BEGIN( script_function )
        {
            // Figure out which function to run
            switch ( valuecode )
            {
                case FIFSPAWNED: returncode = scr_Spawned( pstate, pbdl_ai ); break;
                case FIFTIMEOUT: returncode = scr_TimeOut( pstate, pbdl_ai ); break;
                case FIFATWAYPOINT: returncode = scr_AtWaypoint( pstate, pbdl_ai ); break;
                case FIFATLASTWAYPOINT: returncode = scr_AtLastWaypoint( pstate, pbdl_ai ); break;
                case FIFATTACKED: returncode = scr_Attacked( pstate, pbdl_ai ); break;
                case FIFBUMPED: returncode = scr_Bumped( pstate, pbdl_ai ); break;
                case FIFORDERED: returncode = scr_Ordered( pstate, pbdl_ai ); break;
                case FIFCALLEDFORHELP: returncode = scr_CalledForHelp( pstate, pbdl_ai ); break;
                case FSETCONTENT: returncode = scr_set_Content( pstate, pbdl_ai ); break;
                case FIFKILLED: returncode = scr_Killed( pstate, pbdl_ai ); break;
                case FIFTARGETKILLED: returncode = scr_TargetKilled( pstate, pbdl_ai ); break;
                case FCLEARWAYPOINTS: returncode = scr_ClearWaypoints( pstate, pbdl_ai ); break;
                case FADDWAYPOINT: returncode = scr_AddWaypoint( pstate, pbdl_ai ); break;
                case FFINDPATH: returncode = scr_FindPath( pstate, pbdl_ai ); break;
                case FCOMPASS: returncode = scr_Compass( pstate, pbdl_ai ); break;
                case FGETTARGETARMORPRICE: returncode = scr_get_TargetArmorPrice( pstate, pbdl_ai ); break;
                case FSETTIME: returncode = scr_set_Time( pstate, pbdl_ai ); break;
                case FGETCONTENT: returncode = scr_get_Content( pstate, pbdl_ai ); break;
                case FJOINTARGETTEAM: returncode = scr_JoinTargetTeam( pstate, pbdl_ai ); break;
                case FSETTARGETTONEARBYENEMY: returncode = scr_set_TargetToNearbyEnemy( pstate, pbdl_ai ); break;
                case FSETTARGETTOTARGETLEFTHAND: returncode = scr_set_TargetToTargetLeftHand( pstate, pbdl_ai ); break;
                case FSETTARGETTOTARGETRIGHTHAND: returncode = scr_set_TargetToTargetRightHand( pstate, pbdl_ai ); break;
                case FSETTARGETTOWHOEVERATTACKED: returncode = scr_set_TargetToWhoeverAttacked( pstate, pbdl_ai ); break;
                case FSETTARGETTOWHOEVERBUMPED: returncode = scr_set_TargetToWhoeverBumped( pstate, pbdl_ai ); break;
                case FSETTARGETTOWHOEVERCALLEDFORHELP: returncode = scr_set_TargetToWhoeverCalledForHelp( pstate, pbdl_ai ); break;
                case FSETTARGETTOOLDTARGET: returncode = scr_set_TargetToOldTarget( pstate, pbdl_ai ); break;
                case FSETTURNMODETOVELOCITY: returncode = scr_set_TurnModeToVelocity( pstate, pbdl_ai ); break;
                case FSETTURNMODETOWATCH: returncode = scr_set_TurnModeToWatch( pstate, pbdl_ai ); break;
                case FSETTURNMODETOSPIN: returncode = scr_set_TurnModeToSpin( pstate, pbdl_ai ); break;
                case FSETBUMPHEIGHT: returncode = scr_set_BumpHeight( pstate, pbdl_ai ); break;
                case FIFTARGETHASID: returncode = scr_TargetHasID( pstate, pbdl_ai ); break;
                case FIFTARGETHASITEMID: returncode = scr_TargetHasItemID( pstate, pbdl_ai ); break;
                case FIFTARGETHOLDINGITEMID: returncode = scr_TargetHoldingItemID( pstate, pbdl_ai ); break;
                case FIFTARGETHASSKILLID: returncode = scr_TargetHasSkillID( pstate, pbdl_ai ); break;
                case FELSE: returncode = scr_Else( pstate, pbdl_ai ); break;
                case FRUN: returncode = scr_Run( pstate, pbdl_ai ); break;
                case FWALK: returncode = scr_Walk( pstate, pbdl_ai ); break;
                case FSNEAK: returncode = scr_Sneak( pstate, pbdl_ai ); break;
                case FDOACTION: returncode = scr_DoAction( pstate, pbdl_ai ); break;
                case FKEEPACTION: returncode = scr_KeepAction( pstate, pbdl_ai ); break;
                case FISSUEORDER: returncode = scr_IssueOrder( pstate, pbdl_ai ); break;
                case FDROPWEAPONS: returncode = scr_DropWeapons( pstate, pbdl_ai ); break;
                case FTARGETDOACTION: returncode = scr_TargetDoAction( pstate, pbdl_ai ); break;
                case FOPENPASSAGE: returncode = scr_OpenPassage( pstate, pbdl_ai ); break;
                case FCLOSEPASSAGE: returncode = scr_ClosePassage( pstate, pbdl_ai ); break;
                case FIFPASSAGEOPEN: returncode = scr_PassageOpen( pstate, pbdl_ai ); break;
                case FGOPOOF: returncode = scr_GoPoof( pstate, pbdl_ai ); break;
                case FCOSTTARGETITEMID: returncode = scr_CostTargetItemID( pstate, pbdl_ai ); break;
                case FDOACTIONOVERRIDE: returncode = scr_DoActionOverride( pstate, pbdl_ai ); break;
                case FIFHEALED: returncode = scr_Healed( pstate, pbdl_ai ); break;
                case FSENDMESSAGE: returncode = scr_SendPlayerMessage( pstate, pbdl_ai ); break;
                case FCALLFORHELP: returncode = scr_CallForHelp( pstate, pbdl_ai ); break;
                case FADDIDSZ: returncode = scr_AddIDSZ( pstate, pbdl_ai ); break;
                case FSETSTATE: returncode = scr_set_State( pstate, pbdl_ai ); break;
                case FGETSTATE: returncode = scr_get_State( pstate, pbdl_ai ); break;
                case FIFSTATEIS: returncode = scr_StateIs( pstate, pbdl_ai ); break;
                case FIFTARGETCANOPENSTUFF: returncode = scr_TargetCanOpenStuff( pstate, pbdl_ai ); break;
                case FIFGRABBED: returncode = scr_Grabbed( pstate, pbdl_ai ); break;
                case FIFDROPPED: returncode = scr_Dropped( pstate, pbdl_ai ); break;
                case FSETTARGETTOWHOEVERISHOLDING: returncode = scr_set_TargetToWhoeverIsHolding( pstate, pbdl_ai ); break;
                case FDAMAGETARGET: returncode = scr_DamageTarget( pstate, pbdl_ai ); break;
                case FIFXISLESSTHANY: returncode = scr_XIsLessThanY( pstate, pbdl_ai ); break;
                case FSETWEATHERTIME: returncode = scr_set_WeatherTime( pstate, pbdl_ai ); break;
                case FGETBUMPHEIGHT: returncode = scr_get_BumpHeight( pstate, pbdl_ai ); break;
                case FIFREAFFIRMED: returncode = scr_Reaffirmed( pstate, pbdl_ai ); break;
                case FUNKEEPACTION: returncode = scr_UnkeepAction( pstate, pbdl_ai ); break;
                case FIFTARGETISONOTHERTEAM: returncode = scr_TargetIsOnOtherTeam( pstate, pbdl_ai ); break;
                case FIFTARGETISONHATEDTEAM: returncode = scr_TargetIsOnHatedTeam( pstate, pbdl_ai ); break;
                case FPRESSLATCHBUTTON: returncode = scr_PressLatchButton( pstate, pbdl_ai ); break;
                case FSETTARGETTOTARGETOFLEADER: returncode = scr_set_TargetToTargetOfLeader( pstate, pbdl_ai ); break;
                case FIFLEADERKILLED: returncode = scr_LeaderKilled( pstate, pbdl_ai ); break;
                case FBECOMELEADER: returncode = scr_BecomeLeader( pstate, pbdl_ai ); break;
                case FCHANGETARGETARMOR: returncode = scr_ChangeTargetArmor( pstate, pbdl_ai ); break;
                case FGIVEMONEYTOTARGET: returncode = scr_GiveMoneyToTarget( pstate, pbdl_ai ); break;
                case FDROPKEYS: returncode = scr_DropKeys( pstate, pbdl_ai ); break;
                case FIFLEADERISALIVE: returncode = scr_LeaderIsAlive( pstate, pbdl_ai ); break;
                case FIFTARGETISOLDTARGET: returncode = scr_TargetIsOldTarget( pstate, pbdl_ai ); break;
                case FSETTARGETTOLEADER: returncode = scr_set_TargetToLeader( pstate, pbdl_ai ); break;
                case FSPAWNCHARACTER: returncode = scr_SpawnCharacter( pstate, pbdl_ai ); break;
                case FRESPAWNCHARACTER: returncode = scr_RespawnCharacter( pstate, pbdl_ai ); break;
                case FCHANGETILE: returncode = scr_ChangeTile( pstate, pbdl_ai ); break;
                case FIFUSED: returncode = scr_Used( pstate, pbdl_ai ); break;
                case FDROPMONEY: returncode = scr_DropMoney( pstate, pbdl_ai ); break;
                case FSETOLDTARGET: returncode = scr_set_OldTarget( pstate, pbdl_ai ); break;
                case FDETACHFROMHOLDER: returncode = scr_DetachFromHolder( pstate, pbdl_ai ); break;
                case FIFTARGETHASVULNERABILITYID: returncode = scr_TargetHasVulnerabilityID( pstate, pbdl_ai ); break;
                case FCLEANUP: returncode = scr_CleanUp( pstate, pbdl_ai ); break;
                case FIFCLEANEDUP: returncode = scr_CleanedUp( pstate, pbdl_ai ); break;
                case FIFSITTING: returncode = scr_Sitting( pstate, pbdl_ai ); break;
                case FIFTARGETISHURT: returncode = scr_TargetIsHurt( pstate, pbdl_ai ); break;
                case FIFTARGETISAPLAYER: returncode = scr_TargetIsAPlayer( pstate, pbdl_ai ); break;
                case FPLAYSOUND: returncode = scr_PlaySound( pstate, pbdl_ai ); break;
                case FSPAWNPARTICLE: returncode = scr_SpawnParticle( pstate, pbdl_ai ); break;
                case FIFTARGETISALIVE: returncode = scr_TargetIsAlive( pstate, pbdl_ai ); break;
                case FSTOP: returncode = scr_Stop( pstate, pbdl_ai ); break;
                case FDISAFFIRMCHARACTER: returncode = scr_DisaffirmCharacter( pstate, pbdl_ai ); break;
                case FREAFFIRMCHARACTER: returncode = scr_ReaffirmCharacter( pstate, pbdl_ai ); break;
                case FIFTARGETISSELF: returncode = scr_TargetIsSelf( pstate, pbdl_ai ); break;
                case FIFTARGETISMALE: returncode = scr_TargetIsMale( pstate, pbdl_ai ); break;
                case FIFTARGETISFEMALE: returncode = scr_TargetIsFemale( pstate, pbdl_ai ); break;
                case FSETTARGETTOSELF: returncode = scr_set_TargetToSelf( pstate, pbdl_ai ); break;
                case FSETTARGETTORIDER: returncode = scr_set_TargetToRider( pstate, pbdl_ai ); break;
                case FGETATTACKTURN: returncode = scr_get_AttackTurn( pstate, pbdl_ai ); break;
                case FGETDAMAGETYPE: returncode = scr_get_DamageType( pstate, pbdl_ai ); break;
                case FBECOMESPELL: returncode = scr_BecomeSpell( pstate, pbdl_ai ); break;
                case FBECOMESPELLBOOK: returncode = scr_BecomeSpellbook( pstate, pbdl_ai ); break;
                case FIFSCOREDAHIT: returncode = scr_ScoredAHit( pstate, pbdl_ai ); break;
                case FIFDISAFFIRMED: returncode = scr_Disaffirmed( pstate, pbdl_ai ); break;
                case FTRANSLATEORDER: returncode = scr_TranslateOrder( pstate, pbdl_ai ); break;
                case FSETTARGETTOWHOEVERWASHIT: returncode = scr_set_TargetToWhoeverWasHit( pstate, pbdl_ai ); break;
                case FSETTARGETTOWIDEENEMY: returncode = scr_set_TargetToWideEnemy( pstate, pbdl_ai ); break;
                case FIFCHANGED: returncode = scr_Changed( pstate, pbdl_ai ); break;
                case FIFINWATER: returncode = scr_InWater( pstate, pbdl_ai ); break;
                case FIFBORED: returncode = scr_Bored( pstate, pbdl_ai ); break;
                case FIFTOOMUCHBAGGAGE: returncode = scr_TooMuchBaggage( pstate, pbdl_ai ); break;
                case FIFGROGGED: returncode = scr_Confused( pstate, pbdl_ai ); break;
                case FIFDAZED: returncode = scr_Confused( pstate, pbdl_ai ); break;
                case FIFTARGETHASSPECIALID: returncode = scr_TargetHasSpecialID( pstate, pbdl_ai ); break;
                case FPRESSTARGETLATCHBUTTON: returncode = scr_PressTargetLatchButton( pstate, pbdl_ai ); break;
                case FIFINVISIBLE: returncode = scr_Invisible( pstate, pbdl_ai ); break;
                case FIFARMORIS: returncode = scr_ArmorIs( pstate, pbdl_ai ); break;
                case FGETTARGETGROGTIME: returncode = scr_get_TargetGrogTime( pstate, pbdl_ai ); break;
                case FGETTARGETDAZETIME: returncode = scr_get_TargetDazeTime( pstate, pbdl_ai ); break;
                case FSETDAMAGETYPE: returncode = scr_set_DamageType( pstate, pbdl_ai ); break;
                case FSETWATERLEVEL: returncode = scr_set_WaterLevel( pstate, pbdl_ai ); break;
                case FENCHANTTARGET: returncode = scr_EnchantTarget( pstate, pbdl_ai ); break;
                case FENCHANTCHILD: returncode = scr_EnchantChild( pstate, pbdl_ai ); break;
                case FTELEPORTTARGET: returncode = scr_TeleportTarget( pstate, pbdl_ai ); break;
                case FGIVEEXPERIENCETOTARGET: returncode = scr_GiveExperienceToTarget( pstate, pbdl_ai ); break;
                case FINCREASEAMMO: returncode = scr_IncreaseAmmo( pstate, pbdl_ai ); break;
                case FUNKURSETARGET: returncode = scr_UnkurseTarget( pstate, pbdl_ai ); break;
                case FGIVEEXPERIENCETOTARGETTEAM: returncode = scr_GiveExperienceToTargetTeam( pstate, pbdl_ai ); break;
                case FIFUNARMED: returncode = scr_Unarmed( pstate, pbdl_ai ); break;
                case FRESTOCKTARGETAMMOIDALL: returncode = scr_RestockTargetAmmoIDAll( pstate, pbdl_ai ); break;
                case FRESTOCKTARGETAMMOIDFIRST: returncode = scr_RestockTargetAmmoIDFirst( pstate, pbdl_ai ); break;
                case FFLASHTARGET: returncode = scr_FlashTarget( pstate, pbdl_ai ); break;
                case FSETREDSHIFT: returncode = scr_set_RedShift( pstate, pbdl_ai ); break;
                case FSETGREENSHIFT: returncode = scr_set_GreenShift( pstate, pbdl_ai ); break;
                case FSETBLUESHIFT: returncode = scr_set_BlueShift( pstate, pbdl_ai ); break;
                case FSETLIGHT: returncode = scr_set_Light( pstate, pbdl_ai ); break;
                case FSETALPHA: returncode = scr_set_Alpha( pstate, pbdl_ai ); break;
                case FIFHITFROMBEHIND: returncode = scr_HitFromBehind( pstate, pbdl_ai ); break;
                case FIFHITFROMFRONT: returncode = scr_HitFromFront( pstate, pbdl_ai ); break;
                case FIFHITFROMLEFT: returncode = scr_HitFromLeft( pstate, pbdl_ai ); break;
                case FIFHITFROMRIGHT: returncode = scr_HitFromRight( pstate, pbdl_ai ); break;
                case FIFTARGETISONSAMETEAM: returncode = scr_TargetIsOnSameTeam( pstate, pbdl_ai ); break;
                case FKILLTARGET: returncode = scr_KillTarget( pstate, pbdl_ai ); break;
                case FUNDOENCHANT: returncode = scr_UndoEnchant( pstate, pbdl_ai ); break;
                case FGETWATERLEVEL: returncode = scr_get_WaterLevel( pstate, pbdl_ai ); break;
                case FCOSTTARGETMANA: returncode = scr_CostTargetMana( pstate, pbdl_ai ); break;
                case FIFTARGETHASANYID: returncode = scr_TargetHasAnyID( pstate, pbdl_ai ); break;
                case FSETBUMPSIZE: returncode = scr_set_BumpSize( pstate, pbdl_ai ); break;
                case FIFNOTDROPPED: returncode = scr_NotDropped( pstate, pbdl_ai ); break;
                case FIFYISLESSTHANX: returncode = scr_YIsLessThanX( pstate, pbdl_ai ); break;
                case FSETFLYHEIGHT: returncode = scr_set_FlyHeight( pstate, pbdl_ai ); break;
                case FIFBLOCKED: returncode = scr_Blocked( pstate, pbdl_ai ); break;
                case FIFTARGETISDEFENDING: returncode = scr_TargetIsDefending( pstate, pbdl_ai ); break;
                case FIFTARGETISATTACKING: returncode = scr_TargetIsAttacking( pstate, pbdl_ai ); break;
                case FIFSTATEIS0: returncode = scr_StateIs0( pstate, pbdl_ai ); break;
                case FIFSTATEIS1: returncode = scr_StateIs1( pstate, pbdl_ai ); break;
                case FIFSTATEIS2: returncode = scr_StateIs2( pstate, pbdl_ai ); break;
                case FIFSTATEIS3: returncode = scr_StateIs3( pstate, pbdl_ai ); break;
                case FIFSTATEIS4: returncode = scr_StateIs4( pstate, pbdl_ai ); break;
                case FIFSTATEIS5: returncode = scr_StateIs5( pstate, pbdl_ai ); break;
                case FIFSTATEIS6: returncode = scr_StateIs6( pstate, pbdl_ai ); break;
                case FIFSTATEIS7: returncode = scr_StateIs7( pstate, pbdl_ai ); break;
                case FIFCONTENTIS: returncode = scr_ContentIs( pstate, pbdl_ai ); break;
                case FSETTURNMODETOWATCHTARGET: returncode = scr_set_TurnModeToWatchTarget( pstate, pbdl_ai ); break;
                case FIFSTATEISNOT: returncode = scr_StateIsNot( pstate, pbdl_ai ); break;
                case FIFXISEQUALTOY: returncode = scr_XIsEqualToY( pstate, pbdl_ai ); break;
                case FDEBUGMESSAGE: returncode = scr_DebugMessage( pstate, pbdl_ai ); break;
                case FBLACKTARGET: returncode = scr_BlackTarget( pstate, pbdl_ai ); break;
                case FSENDMESSAGENEAR: returncode = scr_SendMessageNear( pstate, pbdl_ai ); break;
                case FIFHITGROUND: returncode = scr_HitGround( pstate, pbdl_ai ); break;
                case FIFNAMEISKNOWN: returncode = scr_NameIsKnown( pstate, pbdl_ai ); break;
                case FIFUSAGEISKNOWN: returncode = scr_UsageIsKnown( pstate, pbdl_ai ); break;
                case FIFHOLDINGITEMID: returncode = scr_HoldingItemID( pstate, pbdl_ai ); break;
                case FIFHOLDINGRANGEDWEAPON: returncode = scr_HoldingRangedWeapon( pstate, pbdl_ai ); break;
                case FIFHOLDINGMELEEWEAPON: returncode = scr_HoldingMeleeWeapon( pstate, pbdl_ai ); break;
                case FIFHOLDINGSHIELD: returncode = scr_HoldingShield( pstate, pbdl_ai ); break;
                case FIFKURSED: returncode = scr_Kursed( pstate, pbdl_ai ); break;
                case FIFTARGETISKURSED: returncode = scr_TargetIsKursed( pstate, pbdl_ai ); break;
                case FIFTARGETISDRESSEDUP: returncode = scr_TargetIsDressedUp( pstate, pbdl_ai ); break;
                case FIFOVERWATER: returncode = scr_OverWater( pstate, pbdl_ai ); break;
                case FIFTHROWN: returncode = scr_Thrown( pstate, pbdl_ai ); break;
                case FMAKENAMEKNOWN: returncode = scr_MakeNameKnown( pstate, pbdl_ai ); break;
                case FMAKEUSAGEKNOWN: returncode = scr_MakeUsageKnown( pstate, pbdl_ai ); break;
                case FSTOPTARGETMOVEMENT: returncode = scr_StopTargetMovement( pstate, pbdl_ai ); break;
                case FSETXY: returncode = scr_set_XY( pstate, pbdl_ai ); break;
                case FGETXY: returncode = scr_get_XY( pstate, pbdl_ai ); break;
                case FADDXY: returncode = scr_AddXY( pstate, pbdl_ai ); break;
                case FMAKEAMMOKNOWN: returncode = scr_MakeAmmoKnown( pstate, pbdl_ai ); break;
                case FSPAWNATTACHEDPARTICLE: returncode = scr_SpawnAttachedParticle( pstate, pbdl_ai ); break;
                case FSPAWNEXACTPARTICLE: returncode = scr_SpawnExactParticle( pstate, pbdl_ai ); break;
                case FACCELERATETARGET: returncode = scr_AccelerateTarget( pstate, pbdl_ai ); break;
                case FIFDISTANCEISMORETHANTURN: returncode = scr_distanceIsMoreThanTurn( pstate, pbdl_ai ); break;
                case FIFCRUSHED: returncode = scr_Crushed( pstate, pbdl_ai ); break;
                case FMAKECRUSHVALID: returncode = scr_MakeCrushValid( pstate, pbdl_ai ); break;
                case FSETTARGETTOLOWESTTARGET: returncode = scr_set_TargetToLowestTarget( pstate, pbdl_ai ); break;
                case FIFNOTPUTAWAY: returncode = scr_NotPutAway( pstate, pbdl_ai ); break;
                case FIFTAKENOUT: returncode = scr_TakenOut( pstate, pbdl_ai ); break;
                case FIFAMMOOUT: returncode = scr_AmmoOut( pstate, pbdl_ai ); break;
                case FPLAYSOUNDLOOPED: returncode = scr_PlaySoundLooped( pstate, pbdl_ai ); break;
                case FSTOPSOUND: returncode = scr_StopSound( pstate, pbdl_ai ); break;
                case FHEALSELF: returncode = scr_HealSelf( pstate, pbdl_ai ); break;
                case FEQUIP: returncode = scr_Equip( pstate, pbdl_ai ); break;
                case FIFTARGETHASITEMIDEQUIPPED: returncode = scr_TargetHasItemIDEquipped( pstate, pbdl_ai ); break;
                case FSETOWNERTOTARGET: returncode = scr_set_OwnerToTarget( pstate, pbdl_ai ); break;
                case FSETTARGETTOOWNER: returncode = scr_set_TargetToOwner( pstate, pbdl_ai ); break;
                case FSETFRAME: returncode = scr_set_Frame( pstate, pbdl_ai ); break;
                case FBREAKPASSAGE: returncode = scr_BreakPassage( pstate, pbdl_ai ); break;
                case FSETRELOADTIME: returncode = scr_set_ReloadTime( pstate, pbdl_ai ); break;
                case FSETTARGETTOWIDEBLAHID: returncode = scr_set_TargetToWideBlahID( pstate, pbdl_ai ); break;
                case FPOOFTARGET: returncode = scr_PoofTarget( pstate, pbdl_ai ); break;
                case FCHILDDOACTIONOVERRIDE: returncode = scr_ChildDoActionOverride( pstate, pbdl_ai ); break;
                case FSPAWNPOOF: returncode = scr_SpawnPoof( pstate, pbdl_ai ); break;
                case FSETSPEEDPERCENT: returncode = scr_set_SpeedPercent( pstate, pbdl_ai ); break;
                case FSETCHILDSTATE: returncode = scr_set_ChildState( pstate, pbdl_ai ); break;
                case FSPAWNATTACHEDSIZEDPARTICLE: returncode = scr_SpawnAttachedSizedParticle( pstate, pbdl_ai ); break;
                case FCHANGEARMOR: returncode = scr_ChangeArmor( pstate, pbdl_ai ); break;
                case FSHOWTIMER: returncode = scr_ShowTimer( pstate, pbdl_ai ); break;
                case FIFFACINGTARGET: returncode = scr_FacingTarget( pstate, pbdl_ai ); break;
                case FPLAYSOUNDVOLUME: returncode = scr_PlaySoundVolume( pstate, pbdl_ai ); break;
                case FSPAWNATTACHEDFACEDPARTICLE: returncode = scr_SpawnAttachedFacedParticle( pstate, pbdl_ai ); break;
                case FIFSTATEISODD: returncode = scr_StateIsOdd( pstate, pbdl_ai ); break;
                case FSETTARGETTODISTANTENEMY: returncode = scr_set_TargetToDistantEnemy( pstate, pbdl_ai ); break;
                case FTELEPORT: returncode = scr_Teleport( pstate, pbdl_ai ); break;
                case FGIVESTRENGTHTOTARGET: returncode = scr_GiveStrengthToTarget( pstate, pbdl_ai ); break;
                case FGIVEWISDOMTOTARGET: returncode = scr_GiveWisdomToTarget( pstate, pbdl_ai ); break;
                case FGIVEINTELLIGENCETOTARGET: returncode = scr_GiveIntelligenceToTarget( pstate, pbdl_ai ); break;
                case FGIVEDEXTERITYTOTARGET: returncode = scr_GiveDexterityToTarget( pstate, pbdl_ai ); break;
                case FGIVELIFETOTARGET: returncode = scr_GiveLifeToTarget( pstate, pbdl_ai ); break;
                case FGIVEMANATOTARGET: returncode = scr_GiveManaToTarget( pstate, pbdl_ai ); break;
                case FSHOWMAP: returncode = scr_ShowMap( pstate, pbdl_ai ); break;
                case FSHOWYOUAREHERE: returncode = scr_ShowYouAreHere( pstate, pbdl_ai ); break;
                case FSHOWBLIPXY: returncode = scr_ShowBlipXY( pstate, pbdl_ai ); break;
                case FHEALTARGET: returncode = scr_HealTarget( pstate, pbdl_ai ); break;
                case FPUMPTARGET: returncode = scr_PumpTarget( pstate, pbdl_ai ); break;
                case FCOSTAMMO: returncode = scr_CostAmmo( pstate, pbdl_ai ); break;
                case FMAKESIMILARNAMESKNOWN: returncode = scr_MakeSimilarNamesKnown( pstate, pbdl_ai ); break;
                case FSPAWNATTACHEDHOLDERPARTICLE: returncode = scr_SpawnAttachedHolderParticle( pstate, pbdl_ai ); break;
                case FSETTARGETRELOADTIME: returncode = scr_set_TargetReloadTime( pstate, pbdl_ai ); break;
                case FSETFOGLEVEL: returncode = scr_set_FogLevel( pstate, pbdl_ai ); break;
                case FGETFOGLEVEL: returncode = scr_get_FogLevel( pstate, pbdl_ai ); break;
                case FSETFOGTAD: returncode = scr_set_FogTAD( pstate, pbdl_ai ); break;
                case FSETFOGBOTTOMLEVEL: returncode = scr_set_FogBottomLevel( pstate, pbdl_ai ); break;
                case FGETFOGBOTTOMLEVEL: returncode = scr_get_FogBottomLevel( pstate, pbdl_ai ); break;
                case FCORRECTACTIONFORHAND: returncode = scr_CorrectActionForHand( pstate, pbdl_ai ); break;
                case FIFTARGETISMOUNTED: returncode = scr_TargetIsMounted( pstate, pbdl_ai ); break;
                case FSPARKLEICON: returncode = scr_SparkleIcon( pstate, pbdl_ai ); break;
                case FUNSPARKLEICON: returncode = scr_UnsparkleIcon( pstate, pbdl_ai ); break;
                case FGETTILEXY: returncode = scr_get_TileXY( pstate, pbdl_ai ); break;
                case FSETTILEXY: returncode = scr_set_TileXY( pstate, pbdl_ai ); break;
                case FSETSHADOWSIZE: returncode = scr_set_ShadowSize( pstate, pbdl_ai ); break;
                case FORDERTARGET: returncode = scr_OrderTarget( pstate, pbdl_ai ); break;
                case FSETTARGETTOWHOEVERISINPASSAGE: returncode = scr_set_TargetToWhoeverIsInPassage( pstate, pbdl_ai ); break;
                case FIFCHARACTERWASABOOK: returncode = scr_CharacterWasABook( pstate, pbdl_ai ); break;
                case FSETENCHANTBOOSTVALUES: returncode = scr_set_EnchantBoostValues( pstate, pbdl_ai ); break;
                case FSPAWNCHARACTERXYZ: returncode = scr_SpawnCharacterXYZ( pstate, pbdl_ai ); break;
                case FSPAWNEXACTCHARACTERXYZ: returncode = scr_SpawnExactCharacterXYZ( pstate, pbdl_ai ); break;
                case FCHANGETARGETCLASS: returncode = scr_ChangeTargetClass( pstate, pbdl_ai ); break;
                case FPLAYFULLSOUND: returncode = scr_PlayFullSound( pstate, pbdl_ai ); break;
                case FSPAWNEXACTCHASEPARTICLE: returncode = scr_SpawnExactChaseParticle( pstate, pbdl_ai ); break;
                case FCREATEORDER: returncode = scr_CreateOrder( pstate, pbdl_ai ); break;
                case FORDERSPECIALID: returncode = scr_OrderSpecialID( pstate, pbdl_ai ); break;
                case FUNKURSETARGETINVENTORY: returncode = scr_UnkurseTargetInventory( pstate, pbdl_ai ); break;
                case FIFTARGETISSNEAKING: returncode = scr_TargetIsSneaking( pstate, pbdl_ai ); break;
                case FDROPITEMS: returncode = scr_DropItems( pstate, pbdl_ai ); break;
                case FRESPAWNTARGET: returncode = scr_RespawnTarget( pstate, pbdl_ai ); break;
                case FTARGETDOACTIONSETFRAME: returncode = scr_TargetDoActionSetFrame( pstate, pbdl_ai ); break;
                case FIFTARGETCANSEEINVISIBLE: returncode = scr_TargetCanSeeInvisible( pstate, pbdl_ai ); break;
                case FSETTARGETTONEARESTBLAHID: returncode = scr_set_TargetToNearestBlahID( pstate, pbdl_ai ); break;
                case FSETTARGETTONEARESTENEMY: returncode = scr_set_TargetToNearestEnemy( pstate, pbdl_ai ); break;
                case FSETTARGETTONEARESTFRIEND: returncode = scr_set_TargetToNearestFriend( pstate, pbdl_ai ); break;
                case FSETTARGETTONEARESTLIFEFORM: returncode = scr_set_TargetToNearestLifeform( pstate, pbdl_ai ); break;
                case FFLASHPASSAGE: returncode = scr_FlashPassage( pstate, pbdl_ai ); break;
                case FFINDTILEINPASSAGE: returncode = scr_FindTileInPassage( pstate, pbdl_ai ); break;
                case FIFHELDINLEFTHAND: returncode = scr_HeldInLeftHand( pstate, pbdl_ai ); break;
                case FNOTANITEM: returncode = scr_NotAnItem( pstate, pbdl_ai ); break;
                case FSETCHILDAMMO: returncode = scr_set_ChildAmmo( pstate, pbdl_ai ); break;
                case FIFHITVULNERABLE: returncode = scr_HitVulnerable( pstate, pbdl_ai ); break;
                case FIFTARGETISFLYING: returncode = scr_TargetIsFlying( pstate, pbdl_ai ); break;
                case FIDENTIFYTARGET: returncode = scr_IdentifyTarget( pstate, pbdl_ai ); break;
                case FBEATMODULE: returncode = scr_BeatModule( pstate, pbdl_ai ); break;
                case FENDMODULE: returncode = scr_EndModule( pstate, pbdl_ai ); break;
                case FDISABLEEXPORT: returncode = scr_DisableExport( pstate, pbdl_ai ); break;
                case FENABLEEXPORT: returncode = scr_EnableExport( pstate, pbdl_ai ); break;
                case FGETTARGETSTATE: returncode = scr_get_TargetState( pstate, pbdl_ai ); break;
                case FIFEQUIPPED: returncode = scr_Equipped( pstate, pbdl_ai ); break;
                case FDROPTARGETMONEY: returncode = scr_DropTargetMoney( pstate, pbdl_ai ); break;
                case FGETTARGETCONTENT: returncode = scr_get_TargetContent( pstate, pbdl_ai ); break;
                case FDROPTARGETKEYS: returncode = scr_DropTargetKeys( pstate, pbdl_ai ); break;
                case FJOINTEAM: returncode = scr_JoinTeam( pstate, pbdl_ai ); break;
                case FTARGETJOINTEAM: returncode = scr_TargetJoinTeam( pstate, pbdl_ai ); break;
                case FCLEARMUSICPASSAGE: returncode = scr_ClearMusicPassage( pstate, pbdl_ai ); break;
                case FCLEARENDMESSAGE: returncode = scr_ClearEndMessage( pstate, pbdl_ai ); break;
                case FADDENDMESSAGE: returncode = scr_AddEndMessage( pstate, pbdl_ai ); break;
                case FPLAYMUSIC: returncode = scr_PlayMusic( pstate, pbdl_ai ); break;
                case FSETMUSICPASSAGE: returncode = scr_set_MusicPassage( pstate, pbdl_ai ); break;
                case FMAKECRUSHINVALID: returncode = scr_MakeCrushInvalid( pstate, pbdl_ai ); break;
                case FSTOPMUSIC: returncode = scr_StopMusic( pstate, pbdl_ai ); break;
                case FFLASHVARIABLE: returncode = scr_FlashVariable( pstate, pbdl_ai ); break;
                case FACCELERATEUP: returncode = scr_AccelerateUp( pstate, pbdl_ai ); break;
                case FFLASHVARIABLEHEIGHT: returncode = scr_FlashVariableHeight( pstate, pbdl_ai ); break;
                case FSETDAMAGETIME: returncode = scr_set_DamageTime( pstate, pbdl_ai ); break;
                case FIFSTATEIS8: returncode = scr_StateIs8( pstate, pbdl_ai ); break;
                case FIFSTATEIS9: returncode = scr_StateIs9( pstate, pbdl_ai ); break;
                case FIFSTATEIS10: returncode = scr_StateIs10( pstate, pbdl_ai ); break;
                case FIFSTATEIS11: returncode = scr_StateIs11( pstate, pbdl_ai ); break;
                case FIFSTATEIS12: returncode = scr_StateIs12( pstate, pbdl_ai ); break;
                case FIFSTATEIS13: returncode = scr_StateIs13( pstate, pbdl_ai ); break;
                case FIFSTATEIS14: returncode = scr_StateIs14( pstate, pbdl_ai ); break;
                case FIFSTATEIS15: returncode = scr_StateIs15( pstate, pbdl_ai ); break;
                case FIFTARGETISAMOUNT: returncode = scr_TargetIsAMount( pstate, pbdl_ai ); break;
                case FIFTARGETISAPLATFORM: returncode = scr_TargetIsAPlatform( pstate, pbdl_ai ); break;
                case FADDSTAT: returncode = scr_AddStat( pstate, pbdl_ai ); break;
                case FDISENCHANTTARGET: returncode = scr_DisenchantTarget( pstate, pbdl_ai ); break;
                case FDISENCHANTALL: returncode = scr_DisenchantAll( pstate, pbdl_ai ); break;
                case FSETVOLUMENEARESTTEAMMATE: returncode = scr_set_VolumeNearestTeammate( pstate, pbdl_ai ); break;
                case FADDSHOPPASSAGE: returncode = scr_AddShopPassage( pstate, pbdl_ai ); break;
                case FTARGETPAYFORARMOR: returncode = scr_TargetPayForArmor( pstate, pbdl_ai ); break;
                case FJOINEVILTEAM: returncode = scr_JoinEvilTeam( pstate, pbdl_ai ); break;
                case FJOINNULLTEAM: returncode = scr_JoinNullTeam( pstate, pbdl_ai ); break;
                case FJOINGOODTEAM: returncode = scr_JoinGoodTeam( pstate, pbdl_ai ); break;
                case FPITSKILL: returncode = scr_PitsKill( pstate, pbdl_ai ); break;
                case FSETTARGETTOPASSAGEID: returncode = scr_set_TargetToPassageID( pstate, pbdl_ai ); break;
                case FMAKENAMEUNKNOWN: returncode = scr_MakeNameUnknown( pstate, pbdl_ai ); break;
                case FSPAWNEXACTPARTICLEENDSPAWN: returncode = scr_SpawnExactParticleEndSpawn( pstate, pbdl_ai ); break;
                case FSPAWNPOOFSPEEDSPACINGDAMAGE: returncode = scr_SpawnPoofSpeedSpacingDamage( pstate, pbdl_ai ); break;
                case FGIVEEXPERIENCETOGOODTEAM: returncode = scr_GiveExperienceToGoodTeam( pstate, pbdl_ai ); break;
                case FDONOTHING: returncode = scr_DoNothing( pstate, pbdl_ai ); break;
                case FGROGTARGET: returncode = scr_GrogTarget( pstate, pbdl_ai ); break;
                case FDAZETARGET: returncode = scr_DazeTarget( pstate, pbdl_ai ); break;
                case FENABLERESPAWN: returncode = scr_EnableRespawn( pstate, pbdl_ai ); break;
                case FDISABLERESPAWN: returncode = scr_DisableRespawn( pstate, pbdl_ai ); break;
                case FDISPELTARGETENCHANTID: returncode = scr_DispelTargetEnchantID( pstate, pbdl_ai ); break;
                case FIFHOLDERBLOCKED: returncode = scr_HolderBlocked( pstate, pbdl_ai ); break;
                    // case FGETSKILLLEVEL: returncode = scr_get_SkillLevel( pstate, pbdl_ai ); break;
                case FIFTARGETHASNOTFULLMANA: returncode = scr_TargetHasNotFullMana( pstate, pbdl_ai ); break;
                case FENABLELISTENSKILL: returncode = scr_EnableListenSkill( pstate, pbdl_ai ); break;
                case FSETTARGETTOLASTITEMUSED: returncode = scr_set_TargetToLastItemUsed( pstate, pbdl_ai ); break;
                case FFOLLOWLINK: returncode = scr_FollowLink( pstate, pbdl_ai ); break;
                case FIFOPERATORISLINUX: returncode = scr_OperatorIsLinux( pstate, pbdl_ai ); break;
                case FIFTARGETISAWEAPON: returncode = scr_TargetIsAWeapon( pstate, pbdl_ai ); break;
                case FIFSOMEONEISSTEALING: returncode = scr_SomeoneIsStealing( pstate, pbdl_ai ); break;
                case FIFTARGETISASPELL: returncode = scr_TargetIsASpell( pstate, pbdl_ai ); break;
                case FIFBACKSTABBED: returncode = scr_Backstabbed( pstate, pbdl_ai ); break;
                case FGETTARGETDAMAGETYPE: returncode = scr_get_TargetDamageType( pstate, pbdl_ai ); break;
                case FADDQUEST: returncode = scr_AddQuest( pstate, pbdl_ai ); break;
                case FBEATQUESTALLPLAYERS: returncode = scr_BeatQuestAllPlayers( pstate, pbdl_ai ); break;
                case FIFTARGETHASQUEST: returncode = scr_TargetHasQuest( pstate, pbdl_ai ); break;
                case FSETQUESTLEVEL: returncode = scr_set_QuestLevel( pstate, pbdl_ai ); break;
                case FADDQUESTALLPLAYERS: returncode = scr_AddQuestAllPlayers( pstate, pbdl_ai ); break;
                case FADDBLIPALLENEMIES: returncode = scr_AddBlipAllEnemies( pstate, pbdl_ai ); break;
                case FPITSFALL: returncode = scr_PitsFall( pstate, pbdl_ai ); break;
                case FIFTARGETISOWNER: returncode = scr_TargetIsOwner( pstate, pbdl_ai ); break;
                case FEND: returncode = scr_End( pstate, pbdl_ai ); break;

                case FSETSPEECH:           returncode = scr_set_Speech( pstate, pbdl_ai );           break;
                case FSETMOVESPEECH:       returncode = scr_set_MoveSpeech( pstate, pbdl_ai );       break;
                case FSETSECONDMOVESPEECH: returncode = scr_set_SecondMoveSpeech( pstate, pbdl_ai ); break;
                case FSETATTACKSPEECH:     returncode = scr_set_AttackSpeech( pstate, pbdl_ai );     break;
                case FSETASSISTSPEECH:     returncode = scr_set_AssistSpeech( pstate, pbdl_ai );     break;
                case FSETTERRAINSPEECH:    returncode = scr_set_TerrainSpeech( pstate, pbdl_ai );    break;
                case FSETSELECTSPEECH:     returncode = scr_set_SelectSpeech( pstate, pbdl_ai );     break;

                case FTAKEPICTURE:           returncode = scr_TakePicture( pstate, pbdl_ai );         break;
                case FIFOPERATORISMACINTOSH: returncode = scr_OperatorIsMacintosh( pstate, pbdl_ai ); break;
                case FIFMODULEHASIDSZ:       returncode = scr_ModuleHasIDSZ( pstate, pbdl_ai );       break;
                case FMORPHTOTARGET:         returncode = scr_MorphToTarget( pstate, pbdl_ai );       break;
                case FGIVEMANAFLOWTOTARGET:  returncode = scr_GiveManaFlowToTarget( pstate, pbdl_ai ); break;
                case FGIVEMANARETURNTOTARGET:returncode = scr_GiveManaReturnToTarget( pstate, pbdl_ai ); break;
                case FSETMONEY:              returncode = scr_set_Money( pstate, pbdl_ai );           break;
                case FIFTARGETCANSEEKURSES:  returncode = scr_TargetCanSeeKurses( pstate, pbdl_ai );  break;
                case FSPAWNATTACHEDCHARACTER:returncode = scr_SpawnAttachedCharacter( pstate, pbdl_ai ); break;
                case FKURSETARGET:           returncode = scr_KurseTarget( pstate, pbdl_ai );            break;
                case FSETCHILDCONTENT:       returncode = scr_set_ChildContent( pstate, pbdl_ai );    break;
                case FSETTARGETTOCHILD:      returncode = scr_set_TargetToChild( pstate, pbdl_ai );   break;
                case FSETDAMAGETHRESHOLD:     returncode = scr_set_DamageThreshold( pstate, pbdl_ai );   break;
                case FACCELERATETARGETUP:    returncode = scr_AccelerateTargetUp( pstate, pbdl_ai ); break;
                case FSETTARGETAMMO:         returncode = scr_set_TargetAmmo( pstate, pbdl_ai ); break;
                case FENABLEINVICTUS:        returncode = scr_EnableInvictus( pstate, pbdl_ai ); break;
                case FDISABLEINVICTUS:       returncode = scr_DisableInvictus( pstate, pbdl_ai ); break;
                case FTARGETDAMAGESELF:      returncode = scr_TargetDamageSelf( pstate, pbdl_ai ); break;
                case FSETTARGETSIZE:         returncode = scr_SetTargetSize( pstate, pbdl_ai ); break;
                case FIFTARGETISFACINGSELF:  returncode = scr_TargetIsFacingSelf( pstate, pbdl_ai ); break;
                case FDRAWBILLBOARD:         returncode = scr_DrawBillboard( pstate, pbdl_ai ); break;
                case FSETTARGETTOFIRSTBLAHINPASSAGE: returncode = scr_set_TargetToBlahInPassage( pstate, pbdl_ai ); break;

                case FIFLEVELUP:            returncode = scr_LevelUp( pstate, pbdl_ai ); break;
                case FGIVESKILLTOTARGET:    returncode = scr_GiveSkillToTarget( pstate, pbdl_ai ); break;

                    // if none of the above, skip the line and log an error
                default:
                    log_message( "SCRIPT ERROR: scr_run_function() - ai script %d - unhandled script function %d\n", pself->type, valuecode );
                    returncode = bfalse;
                    break;
            }

        }
        PROFILE_END2( script_function );

        _script_function_calls[valuecode] += 1;
        _script_function_times[valuecode] += clktime_script_function;
    }

    return returncode;
}

//--------------------------------------------------------------------------------------------
void scr_set_operand( script_state_t * pstate, Uint8 variable )
{
    /// @details ZZ@> This function sets one of the tmp* values for scripted AI
    switch ( variable )
    {
        case VARTMPX:
            pstate->x = pstate->operationsum;
            break;

        case VARTMPY:
            pstate->y = pstate->operationsum;
            break;

        case VARTMPDISTANCE:
            pstate->distance = pstate->operationsum;
            break;

        case VARTMPTURN:
            pstate->turn = pstate->operationsum;
            break;

        case VARTMPARGUMENT:
            pstate->argument = pstate->operationsum;
            break;

        default:
            log_warning( "scr_set_operand() - cannot assign a number to index %d", variable );
            break;
    }
}

//--------------------------------------------------------------------------------------------
void scr_run_operand( script_state_t * pstate, ai_state_bundle_t * pbdl_ai )
{
    /// @details ZZ@> This function does the scripted arithmetic in OPERATOR, OPERAND pairs

    const char * varname, * op;

    STRING buffer = EMPTY_CSTR;
    Uint8  variable;
    Uint8  operation;

    Uint32 iTmp;

    chr_t * pchr = NULL, * ptarget = NULL, * powner = NULL;
    ai_state_t * pself;

    if ( NULL == pstate || NULL == pbdl_ai || NULL == pbdl_ai->ai_state_ptr ) return;

    if ( !DEFINED_CHR( pbdl_ai->chr_ref ) ) return;

    // use an alias to make the notation easier
    pchr  = pbdl_ai->chr_ptr;
    pself = pbdl_ai->ai_state_ptr;

    if ( DEFINED_CHR( pself->target ) )
    {
        ptarget = ChrList.lst + pself->target;
    }

    if ( DEFINED_CHR( pself->owner ) )
    {
        powner = ChrList.lst + pself->owner;
    }

    // get the operator
    iTmp      = 0;
    varname   = buffer;
    operation = GET_DATA_BITS( pself->opcode );
    if ( HAS_SOME_BITS( pself->opcode, FUNCTION_BIT ) )
    {
        // Get the working opcode from a constant, constants are all but high 5 bits
        iTmp = pself->opcode & VALUE_BITS;
        if ( debug_scripts ) snprintf( buffer, SDL_arraysize( buffer ), "%d", iTmp );
    }
    else
    {
        // Get the variable opcode from a register
        variable = pself->opcode & VALUE_BITS;

        switch ( variable )
        {
            case VARTMPX:
                varname = "TMPX";
                iTmp = pstate->x;
                break;

            case VARTMPY:
                varname = "TMPY";
                iTmp = pstate->y;
                break;

            case VARTMPDISTANCE:
                varname = "TMPDISTANCE";
                iTmp = pstate->distance;
                break;

            case VARTMPTURN:
                varname = "TMPTURN";
                iTmp = pstate->turn;
                break;

            case VARTMPARGUMENT:
                varname = "TMPARGUMENT";
                iTmp = pstate->argument;
                break;

            case VARRAND:
                varname = "RAND";
                iTmp = RANDIE;
                break;

            case VARSELFX:
                varname = "SELFX";
                iTmp = pchr->pos.x;
                break;

            case VARSELFY:
                varname = "SELFY";
                iTmp = pchr->pos.y;
                break;

            case VARSELFTURN:
                varname = "SELFTURN";
                iTmp = pchr->ori.facing_z;
                break;

            case VARSELFCOUNTER:
                varname = "SELFCOUNTER";
                iTmp = pself->order_counter;
                break;

            case VARSELFORDER:
                varname = "SELFORDER";
                iTmp = pself->order_value;
                break;

            case VARSELFMORALE:
                varname = "SELFMORALE";
                iTmp = TeamStack.lst[pchr->baseteam].morale;
                break;

            case VARSELFLIFE:
                varname = "SELFLIFE";
                iTmp = pchr->life;
                break;

            case VARTARGETX:
                varname = "TARGETX";
                iTmp = ( NULL == ptarget ) ? 0 : ptarget->pos.x;
                break;

            case VARTARGETY:
                varname = "TARGETY";
                iTmp = ( NULL == ptarget ) ? 0 : ptarget->pos.y;
                break;

            case VARTARGETDISTANCE:
                varname = "TARGETDISTANCE";
                if ( NULL == ptarget )
                {
                    iTmp = 0x7FFFFFFF;
                }
                else
                {
                    iTmp = fvec2_dist_abs( ptarget->pos.v, pchr->pos.v );
                }
                break;

            case VARTARGETTURN:
                varname = "TARGETTURN";
                iTmp = ( NULL == ptarget ) ? 0 : ptarget->ori.facing_z;
                break;

            case VARLEADERX:
                varname = "LEADERX";
                iTmp = pchr->pos.x;
                if ( TeamStack.lst[pchr->team].leader != NOLEADER )
                    iTmp = team_get_pleader( pchr->team )->pos.x;

                break;

            case VARLEADERY:
                varname = "LEADERY";
                iTmp = pchr->pos.y;
                if ( TeamStack.lst[pchr->team].leader != NOLEADER )
                    iTmp = team_get_pleader( pchr->team )->pos.y;

                break;

            case VARLEADERDISTANCE:
                {
                    chr_t * pleader;
                    varname = "LEADERDISTANCE";

                    pleader = team_get_pleader( pchr->team );

                    if ( NULL == pleader )
                    {
                        iTmp = 0x7FFFFFFF;
                    }
                    else
                    {
                        iTmp = fvec2_dist_abs( pleader->pos.v, pchr->pos.v );
                    }
                }
                break;

            case VARLEADERTURN:
                varname = "LEADERTURN";
                iTmp = pchr->ori.facing_z;
                if ( TeamStack.lst[pchr->team].leader != NOLEADER )
                    iTmp = team_get_pleader( pchr->team )->ori.facing_z;

                break;

            case VARGOTOX:
                varname = "GOTOX";

                ai_state_ensure_wp( pself );

                if ( !pself->wp_valid )
                {
                    iTmp = pchr->pos.x;
                }
                else
                {
                    iTmp = pself->wp[kX];
                }
                break;

            case VARGOTOY:
                varname = "GOTOY";

                ai_state_ensure_wp( pself );

                if ( !pself->wp_valid )
                {
                    iTmp = pchr->pos.y;
                }
                else
                {
                    iTmp = pself->wp[kY];
                }
                break;

            case VARGOTODISTANCE:
                varname = "GOTODISTANCE";

                ai_state_ensure_wp( pself );

                if ( !pself->wp_valid )
                {
                    iTmp = 0x7FFFFFFF;
                }
                else
                {
                    iTmp = ABS( pself->wp[kX] - pchr->pos.x ) +
                           ABS( pself->wp[kY] - pchr->pos.y );
                }
                break;

            case VARTARGETTURNTO:
                varname = "TARGETTURNTO";
                if ( NULL == ptarget )
                {
                    iTmp = 0;
                }
                else
                {
                    iTmp = vec_to_facing( ptarget->pos.x - pchr->pos.x , ptarget->pos.y - pchr->pos.y );
                    iTmp = CLIP_TO_16BITS( iTmp );
                }
                break;

            case VARPASSAGE:
                varname = "PASSAGE";
                iTmp = pself->passage;
                break;

            case VARWEIGHT:
                varname = "WEIGHT";
                iTmp = pchr->holdingweight;
                break;

            case VARSELFALTITUDE:
                varname = "SELFALTITUDE";
                iTmp = pchr->pos.z - pchr->enviro.grid_level;
                break;

            case VARSELFID:
                varname = "SELFID";
                iTmp = chr_get_idsz( pbdl_ai->chr_ref, IDSZ_TYPE );
                break;

            case VARSELFHATEID:
                varname = "SELFHATEID";
                iTmp = chr_get_idsz( pbdl_ai->chr_ref, IDSZ_HATE );
                break;

            case VARSELFMANA:
                varname = "SELFMANA";
                iTmp = pchr->mana;
                if ( pchr->canchannel )  iTmp += pchr->life;

                break;

            case VARTARGETSTR:
                varname = "TARGETSTR";
                iTmp = ( NULL == ptarget ) ? 0 : ptarget->strength;
                break;

            case VARTARGETWIS:
                varname = "TARGETWIS";
                iTmp = ( NULL == ptarget ) ? 0 : ptarget->wisdom;
                break;

            case VARTARGETINT:
                varname = "TARGETINT";
                iTmp = ( NULL == ptarget ) ? 0 : ptarget->intelligence;
                break;

            case VARTARGETDEX:
                varname = "TARGETDEX";
                iTmp = ( NULL == ptarget ) ? 0 : ptarget->dexterity;
                break;

            case VARTARGETLIFE:
                varname = "TARGETLIFE";
                iTmp = ( NULL == ptarget ) ? 0 : ptarget->life;
                break;

            case VARTARGETMANA:
                varname = "TARGETMANA";
                if ( NULL == ptarget )
                {
                    iTmp = 0;
                }
                else
                {
                    iTmp = ptarget->mana;
                    if ( ptarget->canchannel ) iTmp += ptarget->life;
                }

                break;

            case VARTARGETLEVEL:
                varname = "TARGETLEVEL";
                iTmp = ( NULL == ptarget ) ? 0 : ptarget->experiencelevel;
                break;

            case VARTARGETSPEEDX:
                varname = "TARGETSPEEDX";
                iTmp = ( NULL == ptarget ) ? 0 : ABS( ptarget->vel.x );
                break;

            case VARTARGETSPEEDY:
                varname = "TARGETSPEEDY";
                iTmp = ( NULL == ptarget ) ? 0 : ABS( ptarget->vel.y );
                break;

            case VARTARGETSPEEDZ:
                varname = "TARGETSPEEDZ";
                iTmp = ( NULL == ptarget ) ? 0 : ABS( ptarget->vel.z );
                break;

            case VARSELFSPAWNX:
                varname = "SELFSPAWNX";
                iTmp = pchr->pos_stt.x;
                break;

            case VARSELFSPAWNY:
                varname = "SELFSPAWNY";
                iTmp = pchr->pos_stt.y;
                break;

            case VARSELFSTATE:
                varname = "SELFSTATE";
                iTmp = pself->state;
                break;

            case VARSELFCONTENT:
                varname = "SELFCONTENT";
                iTmp = pself->content;
                break;

            case VARSELFSTR:
                varname = "SELFSTR";
                iTmp = pchr->strength;
                break;

            case VARSELFWIS:
                varname = "SELFWIS";
                iTmp = pchr->wisdom;
                break;

            case VARSELFINT:
                varname = "SELFINT";
                iTmp = pchr->intelligence;
                break;

            case VARSELFDEX:
                varname = "SELFDEX";
                iTmp = pchr->dexterity;
                break;

            case VARSELFMANAFLOW:
                varname = "SELFMANAFLOW";
                iTmp = pchr->manaflow;
                break;

            case VARTARGETMANAFLOW:
                varname = "TARGETMANAFLOW";
                iTmp = ( NULL == ptarget ) ? 0 : ptarget->manaflow;
                break;

            case VARSELFATTACHED:
                varname = "SELFATTACHED";
                iTmp = number_of_attached_particles( pbdl_ai->chr_ref );
                break;

            case VARSWINGTURN:
                varname = "SWINGTURN";
                iTmp = PCamera->swing << 2;
                break;

            case VARXYDISTANCE:
                varname = "XYDISTANCE";
                iTmp = SQRT( pstate->x * pstate->x + pstate->y * pstate->y );
                break;

            case VARSELFZ:
                varname = "SELFZ";
                iTmp = pchr->pos.z;
                break;

            case VARTARGETALTITUDE:
                varname = "TARGETALTITUDE";
                iTmp = ( NULL == ptarget ) ? 0 : ptarget->pos.z - ptarget->enviro.grid_level;
                break;

            case VARTARGETZ:
                varname = "TARGETZ";
                iTmp = ( NULL == ptarget ) ? 0 : ptarget->pos.z;
                break;

            case VARSELFINDEX:
                varname = "SELFINDEX";
                iTmp = REF_TO_INT( pbdl_ai->chr_ref );
                break;

            case VAROWNERX:
                varname = "OWNERX";
                iTmp = ( NULL == powner ) ? 0 : powner->pos.x;
                break;

            case VAROWNERY:
                varname = "OWNERY";
                iTmp = ( NULL == powner ) ? 0 : powner->pos.y;
                break;

            case VAROWNERTURN:
                varname = "OWNERTURN";
                iTmp = ( NULL == powner ) ? 0 : powner->ori.facing_z;
                break;

            case VAROWNERDISTANCE:
                varname = "OWNERDISTANCE";
                if ( NULL == powner )
                {
                    iTmp = 0x7FFFFFFF;
                }
                else
                {
                    iTmp = fvec2_dist_abs( powner->pos.v, pchr->pos.v );
                }
                break;

            case VAROWNERTURNTO:
                varname = "OWNERTURNTO";
                if ( NULL == powner )
                {
                    iTmp = 0;
                }
                else
                {
                    iTmp = vec_to_facing( powner->pos.x - pchr->pos.x , powner->pos.y - pchr->pos.y );
                    iTmp = CLIP_TO_16BITS( iTmp );
                }
                break;

            case VARXYTURNTO:
                varname = "XYTURNTO";
                iTmp = vec_to_facing( pstate->x - pchr->pos.x , pstate->y - pchr->pos.y );
                iTmp = CLIP_TO_16BITS( iTmp );
                break;

            case VARSELFMONEY:
                varname = "SELFMONEY";
                iTmp = pchr->money;
                break;

            case VARSELFACCEL:
                varname = "SELFACCEL";
                iTmp = ( pchr->maxaccel_reset * 100.0f );
                break;

            case VARTARGETEXP:
                varname = "TARGETEXP";
                iTmp = ( NULL == ptarget ) ? 0 : ptarget->experience;
                break;

            case VARSELFAMMO:
                varname = "SELFAMMO";
                iTmp = pchr->ammo;
                break;

            case VARTARGETAMMO:
                varname = "TARGETAMMO";
                iTmp = ( NULL == ptarget ) ? 0 : ptarget->ammo;
                break;

            case VARTARGETMONEY:
                varname = "TARGETMONEY";
                iTmp = ( NULL == ptarget ) ? 0 : ptarget->money;
                break;

            case VARTARGETTURNAWAY:
                varname = "TARGETTURNAWAY";
                if ( NULL == ptarget )
                {
                    iTmp = 0;
                }
                else
                {
                    iTmp = vec_to_facing( ptarget->pos.x - pchr->pos.x , ptarget->pos.y - pchr->pos.y );
                    iTmp = CLIP_TO_16BITS( iTmp );
                }
                break;

            case VARSELFLEVEL:
                varname = "SELFLEVEL";
                iTmp = pchr->experiencelevel;
                break;

            case VARTARGETRELOADTIME:
                varname = "TARGETRELOADTIME";
                iTmp = ( NULL == ptarget ) ? 0 : ptarget->reloadtime;
                break;

            case VARSPAWNDISTANCE:
                varname = "SPAWNDISTANCE";
                iTmp = fvec2_dist_abs( pchr->pos_stt.v, pchr->pos.v );
                break;

            case VARTARGETMAXLIFE:
                varname = "TARGETMAXLIFE";
                iTmp = ( NULL == ptarget ) ? 0 : ptarget->lifemax;
                break;

            case VARTARGETTEAM:
                varname = "TARGETTEAM";
                iTmp = ( NULL == ptarget ) ? 0 : ptarget->team;
                //iTmp = REF_TO_INT( chr_get_iteam( pself->target ) );
                break;

            case VARTARGETARMOR:
                varname = "TARGETARMOR";
                iTmp = ( NULL == ptarget ) ? 0 : ptarget->skin;
                break;

            case VARDIFFICULTY:
                varname = "DIFFICULTY";
                iTmp = cfg.difficulty;
                break;

            case VARTIMEHOURS:
                varname = "TIMEHOURS";
                iTmp = getCurrentTime()->tm_hour;
                break;

            case VARTIMEMINUTES:
                varname = "TIMEMINUTES";
                iTmp = getCurrentTime()->tm_min;
                break;

            case VARTIMESECONDS:
                varname = "TIMESECONDS";
                iTmp = getCurrentTime()->tm_sec;
                break;

            case VARDATEMONTH:
                varname = "DATEMONTH";
                iTmp = getCurrentTime()->tm_mon + 1;
                break;

            case VARDATEDAY:
                varname = "DATEDAY";
                iTmp = getCurrentTime()->tm_mday;
                break;

            default:
                log_message( "SCRIPT ERROR: scr_run_operand() - model == %d, class name == \"%s\" - Unknown variable found!\n", REF_TO_INT( script_error_model ), script_error_classname );
                break;
        }
    }

    // Now do the math
    op = "UNKNOWN";
    switch ( operation )
    {
        case OPADD:
            op = "ADD";
            pstate->operationsum += iTmp;
            break;

        case OPSUB:
            op = "SUB";
            pstate->operationsum -= iTmp;
            break;

        case OPAND:
            op = "AND";
            pstate->operationsum &= iTmp;
            break;

        case OPSHR:
            op = "SHR";
            pstate->operationsum >>= iTmp;
            break;

        case OPSHL:
            op = "SHL";
            pstate->operationsum <<= iTmp;
            break;

        case OPMUL:
            op = "MUL";
            pstate->operationsum *= iTmp;
            break;

        case OPDIV:
            op = "DIV";
            if ( iTmp != 0 )
            {
                pstate->operationsum = (( float )pstate->operationsum ) / iTmp;
            }
            else
            {
                log_message( "SCRIPT ERROR: scr_run_operand() - model == %d, class name == \"%s\" - Cannot divide by zero!\n", REF_TO_INT( script_error_model ), script_error_classname );
            }
            break;

        case OPMOD:
            op = "MOD";
            if ( iTmp != 0 )
            {
                pstate->operationsum %= iTmp;
            }
            else
            {
                log_message( "SCRIPT ERROR: scr_run_operand() - model == %d, class name == \"%s\" - Cannot modulo by zero!\n", REF_TO_INT( script_error_model ), script_error_classname );
            }
            break;

        default:
            log_message( "SCRIPT ERROR: scr_run_operand() - model == %d, class name == \"%s\" - unknown op\n", REF_TO_INT( script_error_model ), script_error_classname );
            break;
    }

    if ( debug_scripts )
    {
        FILE * scr_file = ( NULL == debug_script_file ) ? stdout : debug_script_file;
        fprintf( scr_file, "%s %s(%d) ", op, varname, iTmp );
    }
}

//--------------------------------------------------------------------------------------------
bool_t scr_increment_exe( ai_state_t * pself )
{
    if ( NULL == pself ) return bfalse;
    if ( pself->exe_pos < pself->exe_stt || pself->exe_pos >= pself->exe_end ) return bfalse;

    pself->exe_pos++;
    pself->opcode = AisCompiled_buffer[pself->exe_pos];

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t scr_set_exe( ai_state_t * pself, size_t offset )
{
    if ( NULL == pself ) return bfalse;
    if ( offset < pself->exe_stt || offset >= pself->exe_end ) return bfalse;

    pself->exe_pos = offset;
    pself->opcode  = AisCompiled_buffer[pself->exe_pos];

    return btrue;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t waypoint_list_peek( waypoint_list_t * plst, waypoint_t wp )
{
    int index;

    // is the list valid?
    if ( NULL == plst || plst->tail >= MAXWAY ) return bfalse;

    // is the list is empty?
    if ( 0 == plst->head ) return bfalse;

    if ( plst->tail > plst->head )
    {
        // fix the tail
        plst->tail = plst->head;

        // we have passed the last waypoint
        // just tell them the previous waypoint
        index = plst->tail - 1;
    }
    else if ( plst->tail == plst->head )
    {
        // we have passed the last waypoint
        // just tell them the previous waypoint
        index = plst->tail - 1;
    }
    else
    {
        // tell them the current waypoint
        index = plst->tail;
    }

    wp[kX] = plst->pos[index][kX];
    wp[kY] = plst->pos[index][kY];
    wp[kZ] = plst->pos[index][kZ];

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t waypoint_list_push( waypoint_list_t * plst, float x, float y, float z )
{
    /// @details BB@> Add a waypoint to the waypoint list

    float level;

    if ( NULL == plst ) return bfalse;

    level = 0.0f;
    if ( INVALID_TILE != mesh_get_tile( PMesh, x, y ) )
    {
        level = mesh_get_level( PMesh, x, y );
    }

    // add the value
    plst->pos[plst->head][kX] = x;
    plst->pos[plst->head][kY] = y;
    plst->pos[plst->head][kZ] = z + level;

    // do not let the list overflow
    plst->head++;
    if ( plst->head >= MAXWAY ) plst->head = MAXWAY - 1;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t waypoint_list_reset( waypoint_list_t * plst )
{
    /// @details BB@> reset the waypoint list to the beginning

    if ( NULL == plst ) return bfalse;

    plst->tail = 0;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t waypoint_list_clear( waypoint_list_t * plst )
{
    /// @details BB@> Clear out all waypoints

    if ( NULL == plst ) return bfalse;

    plst->tail = 0;
    plst->head = 0;

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t waypoint_list_empty( const waypoint_list_t * plst )
{
    if ( NULL == plst ) return btrue;

    return 0 == plst->head;
}

//--------------------------------------------------------------------------------------------
bool_t waypoint_list_finished( const waypoint_list_t * plst )
{
    if ( NULL == plst || 0 == plst->head ) return btrue;

    return plst->tail == plst->head;
}

//--------------------------------------------------------------------------------------------
bool_t waypoint_list_advance( waypoint_list_t * plst )
{
    bool_t retval;

    if ( NULL == plst ) return bfalse;

    retval = bfalse;
    if ( plst->tail > plst->head )
    {
        // fix the tail
        plst->tail = plst->head;
    }
    else if ( plst->tail < plst->head )
    {
        // advance the tail
        plst->tail++;
        retval = btrue;
    }

    // clamp the tail to valid values
    if ( plst->tail >= MAXWAY ) plst->tail = MAXWAY - 1;

    return retval;
}

//--------------------------------------------------------------------------------------------
bool_t ai_state_get_wp( ai_state_t * pself )
{
    // try to load up the top waypoint

    if ( NULL == pself || !INGAME_CHR( pself->index ) ) return bfalse;

    pself->wp_valid = waypoint_list_peek( &( pself->wp_lst ), pself->wp );

    return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t ai_state_ensure_wp( ai_state_t * pself )
{
    // is the current waypoint is not valid, try to load up the top waypoint

    if ( NULL == pself || !INGAME_CHR( pself->index ) ) return bfalse;

    if ( pself->wp_valid ) return btrue;

    return ai_state_get_wp( pself );
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void set_alerts( ai_state_bundle_t * pbdl_ai )
{
    /// @details ZZ@> This function polls some alert conditions

    chr_t           * pchr;
    ai_state_t      * pself;
    waypoint_list_t * pwaypoints;
    bool_t            at_waypoint;

    // if the bundle does not exist, return
    if ( NULL == pbdl_ai ) return;

    // invalid characters do not think
    if ( NULL == pbdl_ai || !INGAME_CHR( pbdl_ai->chr_ref ) ) return;

    // use aliases to make the notation easier
    pself      = pbdl_ai->ai_state_ptr;
    pchr       = pbdl_ai->chr_ptr;
    pwaypoints = &( pself->wp_lst );

    if ( waypoint_list_empty( pwaypoints ) ) return;

    // let's let mounts get alert updates...
    // imagine a mount, like a racecar, that needs to make sure that it follows X
    // waypoints around a track or something

    // mounts do not get alerts
    // if ( INGAME_CHR(pchr->attachedto) ) return;

    // is the current waypoint is not valid, try to load up the top waypoint
    ai_state_ensure_wp( pself );

    at_waypoint = bfalse;
    if ( pself->wp_valid )
    {
        at_waypoint = ( ABS( pchr->pos.x - pself->wp[kX] ) < WAYTHRESH ) &&
                      ( ABS( pchr->pos.y - pself->wp[kY] ) < WAYTHRESH );
    }

    if ( at_waypoint )
    {
        ADD_BITS( pself->alert, ALERTIF_ATWAYPOINT );

        if ( waypoint_list_finished( pwaypoints ) )
        {
            // we are now at the last waypoint
            // if the object can be alerted to last waypoint, do it
            // this test needs to be done because the ALERTIF_ATLASTWAYPOINT
            // doubles for "at last waypoint" and "not put away"
            if ( !chr_get_pcap( pbdl_ai->chr_ref )->isequipment )
            {
                ADD_BITS( pself->alert, ALERTIF_ATLASTWAYPOINT );
            }

            // !!!!restart the waypoint list, do not clear them!!!!
            waypoint_list_reset( pwaypoints );

            // load the top waypoint
            ai_state_get_wp( pself );
        }
        else if ( waypoint_list_advance( pwaypoints ) )
        {
            // load the top waypoint
            ai_state_get_wp( pself );
        }
    }
}

//--------------------------------------------------------------------------------------------
void issue_order( const CHR_REF by_reference character, Uint32 value )
{
    /// @details ZZ@> This function issues an value for help to all teammates

    CHR_REF cnt;
    int     counter;

    for ( cnt = 0, counter = 0; cnt < MAX_CHR; cnt++ )
    {
        if ( !INGAME_CHR( cnt ) ) continue;

        if ( chr_get_iteam( cnt ) == chr_get_iteam( character ) )
        {
            ai_add_order( chr_get_pai( cnt ), value, counter );
            counter++;
        }
    }
}

//--------------------------------------------------------------------------------------------
void issue_special_order( Uint32 value, IDSZ idsz )
{
    /// @details ZZ@> This function issues an order to all characters with the a matching special IDSZ

    CHR_REF cnt;
    int     counter;

    for ( cnt = 0, counter = 0; cnt < MAX_CHR; cnt++ )
    {
        cap_t * pcap;

        if ( !INGAME_CHR( cnt ) ) continue;

        pcap = chr_get_pcap( cnt );
        if ( NULL == pcap ) continue;

        if ( idsz == pcap->idsz[IDSZ_SPECIAL] )
        {
            ai_add_order( chr_get_pai( cnt ), value, counter );
            counter++;
        }
    }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t ai_state_free( ai_state_t * pself )
{
    if ( NULL == pself ) return bfalse;

    // free any allocated data
    PROFILE_FREE_STRUCT( pself );

    return btrue;
};

//--------------------------------------------------------------------------------------------
ai_state_t * ai_state_reconstruct( ai_state_t * pself )
{
    if ( NULL == pself ) return pself;

    // deallocate any existing data
    ai_state_free( pself );

    // set everything to safe values
    memset( pself, 0, sizeof( *pself ) );

    pself->index      = ( CHR_REF )MAX_CHR;
    pself->target     = ( CHR_REF )MAX_CHR;
    pself->owner      = ( CHR_REF )MAX_CHR;
    pself->child      = ( CHR_REF )MAX_CHR;
    pself->target_old = ( CHR_REF )MAX_CHR;
    pself->poof_time  = -1;

    pself->bumplast   = ( CHR_REF )MAX_CHR;
    pself->attacklast = ( CHR_REF )MAX_CHR;
    pself->hitlast    = ( CHR_REF )MAX_CHR;

    return pself;
}

//--------------------------------------------------------------------------------------------
ai_state_t * ai_state_ctor( ai_state_t * pself )
{
    if ( NULL == ai_state_reconstruct( pself ) ) return NULL;

    PROFILE_INIT_STRUCT( ai, pself );

    return pself;
}

//--------------------------------------------------------------------------------------------
ai_state_t * ai_state_dtor( ai_state_t * pself )
{
    if ( NULL == pself ) return pself;

    // initialize the object
    ai_state_reconstruct( pself );

    return pself;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
ai_state_bundle_t * ai_state_bundle_ctor( ai_state_bundle_t * pbundle )
{
    if ( NULL == pbundle ) return NULL;

    pbundle->ai_state_ref = ( REF_T ) MAX_AI;
    pbundle->ai_state_ptr = NULL;

    pbundle->chr_ref = ( CHR_REF ) MAX_CHR;
    pbundle->chr_ptr = NULL;

    pbundle->cap_ref = ( CAP_REF ) MAX_CAP;
    pbundle->cap_ptr = NULL;

    pbundle->pro_ref = ( PRO_REF ) MAX_PROFILE;
    pbundle->pro_ptr = NULL;

    return pbundle;
}

//--------------------------------------------------------------------------------------------
ai_state_bundle_t * ai_state_bundle_validate( ai_state_bundle_t * pbundle )
{
    if ( NULL == pbundle ) return NULL;

    // get the character info from the reference or the pointer
    if ( ALLOCATED_CHR( pbundle->chr_ref ) )
    {
        pbundle->chr_ptr = ChrList.lst + pbundle->chr_ref;
    }
    else if ( NULL != pbundle->chr_ptr )
    {
        pbundle->chr_ref = GET_REF_PCHR( pbundle->chr_ptr );
    }
    else
    {
        pbundle->chr_ref = MAX_CHR;
        pbundle->chr_ptr = NULL;
    }

    if ( NULL == pbundle->chr_ptr ) goto ai_state_bundle_validate_fail;

    // get the profile info
    pbundle->pro_ref = pbundle->chr_ptr->profile_ref;
    if ( !LOADED_PRO( pbundle->pro_ref ) ) goto ai_state_bundle_validate_fail;

    pbundle->pro_ptr = ProList.lst + pbundle->pro_ref;

    // get the cap info
    pbundle->cap_ref = pbundle->pro_ptr->icap;

    if ( !LOADED_CAP( pbundle->cap_ref ) ) goto ai_state_bundle_validate_fail;
    pbundle->cap_ptr = CapStack.lst + pbundle->cap_ref;

    // get the script info
    pbundle->ai_state_ref = pbundle->chr_ptr->ai.type;
    pbundle->ai_state_ptr = &( pbundle->chr_ptr->ai );

    return pbundle;

ai_state_bundle_validate_fail:

    return ai_state_bundle_ctor( pbundle );
}

//--------------------------------------------------------------------------------------------
ai_state_bundle_t * ai_state_bundle_set( ai_state_bundle_t * pbundle, chr_t * pchr )
{
    if ( NULL == pbundle ) return NULL;

    // blank out old data
    pbundle = ai_state_bundle_ctor( pbundle );

    if ( NULL == pbundle || NULL == pchr ) return pbundle;

    // set the particle pointer
    pbundle->chr_ptr = pchr;

    // validate the particle data
    pbundle = ai_state_bundle_validate( pbundle );

    return pbundle;
}
