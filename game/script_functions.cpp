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

/// @file script_functions.c
/// @brief implementation of the internal egoscript functions
/// @details These are the c-functions that are called to implement the egoscript commsnds
/// At some pont, the non-trivial commands in this file will be broken out to discrete functions
/// and wrapped by some external language (like Lua) using something like SWIG, and a cross compiler written.
/// The current code is about 3/4 of the way toward this goal.
/// The functions below will then be replaced with stub calls to the "real" functions.

#include "script_functions.h"

#include "menu.h"

#include "mad.h"

#include "link.h"
#include "camera.h"
#include "passage.h"
#include "graphic.h"
#include "input.h"
#include "network.h"
#include "game.h"
#include "log.h"

#include "file_formats/spawn_file.h"
#include "quest.h"

#include "egoboo_strutil.h"
#include "egoboo_setup.h"

#include "profile.inl"
#include "enchant.inl"
#include "char.inl"
#include "particle.inl"
#include "mesh.inl"
#include "egoboo_math.inl"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// turn off this annoying warning
#if defined _MSC_VER
#    pragma warning(disable : 4189) // local variable is initialized but not referenced
#endif

#define SCRIPT_FUNCTION_BEGIN() \
    if( NULL == pstate || NULL == pbdl_self ) return bfalse; \
    Uint8 returncode = btrue; \
    ego_ai_state * pself = pbdl_self->ai_state_ptr; \
    ego_chr      * pchr  = pbdl_self->chr_ptr; \
    ego_pro      * ppro  = pbdl_self->pro_ptr;

#define SCRIPT_FUNCTION_END() \
    return returncode;

#define FUNCTION_BEGIN() \
    Uint8 returncode = btrue; \
    if( !VALID_PCHR( pchr ) ) return bfalse;

#define FUNCTION_END() \
    return returncode;

#define SET_TARGET_0(ITARGET)         pself->target = ITARGET;
#define SET_TARGET_1(ITARGET,PTARGET) if( NULL != PTARGET ) { PTARGET = (!INGAME_CHR(ITARGET) ? NULL : ChrObjList.get_data_ptr(ITARGET) ); }
#define SET_TARGET(ITARGET,PTARGET)   SET_TARGET_0( ITARGET ); SET_TARGET_1(ITARGET,PTARGET)

#define SCRIPT_REQUIRE_TARGET(PTARGET) \
    if( !INGAME_CHR(pself->target) ) return bfalse; \
    PTARGET = ChrObjList.get_data_ptr(pself->target);

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// @defgroup _bitwise_functions_ Bitwise Scripting Functions
/// @details These functions may be necessary to export the bitwise functions for handling alerts to
///  scripting languages where there is no support for bitwise operators (Lua, tcl, ...)

//--------------------------------------------------------------------------------------------

/// @ingroup _bitwise_functions_
Uint8 scr_set_AlertBit( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    /// @details BB@> Sets the bit in the 32-bit integer self.alert indexed by pstate->argument

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( pstate->argument >= 0 && pstate->argument < 32 )
    {
        ADD_BITS( pself->alert, 1 << pstate->argument );
        returncode = btrue;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------

/// @ingroup _bitwise_functions_
Uint8 scr_ClearAlertBit( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    /// @details BB@> Clears the bit in the 32-bit integer self.alert indexed by pstate->argument

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( pstate->argument >= 0 && pstate->argument < 32 )
    {
        REMOVE_BITS( pself->alert, 1 << pstate->argument );
        returncode = btrue;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------

/// @ingroup _bitwise_functions_
Uint8 scr_TestAlertBit( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    /// @details BB@> Tests to see if the the bit in the 32-bit integer self.alert indexed by pstate->argument is non-zero

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( pstate->argument >= 0 && pstate->argument < 32 )
    {
        returncode = HAS_SOME_BITS( pself->alert,  1 << pstate->argument );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------

/// @ingroup _bitwise_functions_
Uint8 scr_set_Alert( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    /// @details BB@> Sets one or more bits of the self.alert variable given by the bitmask in tmpargument

    SCRIPT_FUNCTION_BEGIN();

    ADD_BITS( pself->alert, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------

/// @ingroup _bitwise_functions_
Uint8 scr_ClearAlert( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    /// @details BB@> Clears one or more bits of the self.alert variable given by the bitmask in tmpargument

    SCRIPT_FUNCTION_BEGIN();

    REMOVE_BITS( pself->alert, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------

/// @ingroup _bitwise_functions_
Uint8 scr_TestAlert( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    /// @details BB@> Tests one or more bits of the self.alert variable given by the bitmask in tmpargument

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pself->alert, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------

/// @ingroup _bitwise_functions_
Uint8 scr_set_Bit( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    /// @details BB@> Sets the bit in the 32-bit tmpx variable with the offset given in tmpy

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( pstate->y >= 0 && pstate->y < 32 )
    {
        ADD_BITS( pstate->x, 1 << pstate->y );
        returncode = btrue;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------

/// @ingroup _bitwise_functions_
Uint8 scr_ClearBit( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    /// @details BB@> Clears the bit in the 32-bit tmpx variable with the offset given in tmpy

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( pstate->y >= 0 && pstate->y < 32 )
    {
        REMOVE_BITS( pstate->x, 1 << pstate->y );
        returncode = btrue;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------

/// @ingroup _bitwise_functions_
Uint8 scr_TestBit( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    /// @details BB@> Tests the bit in the 32-bit tmpx variable with the offset given in tmpy

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( pstate->y >= 0 && pstate->y < 32 )
    {
        returncode = HAS_SOME_BITS( pstate->x, 1 << pstate->y );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------

/// @ingroup _bitwise_functions_
Uint8 scr_set_Bits( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    /// @details BB@> Adds the bits in the 32-bit tmpx based on the bitmask in tmpy

    SCRIPT_FUNCTION_BEGIN();

    ADD_BITS( pstate->x, pstate->y );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------

/// @ingroup _bitwise_functions_
Uint8 scr_ClearBits( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    /// @details BB@> Clears the bits in the 32-bit tmpx based on the bitmask in tmpy

    SCRIPT_FUNCTION_BEGIN();

    REMOVE_BITS( pstate->x, pstate->y );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------

/// @ingroup _bitwise_functions_
Uint8 scr_TestBits( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    /// @details BB@> Tests the bits in the 32-bit tmpx based on the bitmask in tmpy

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pstate->x, pstate->y );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Uint8 scr_Spawned( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfSpawned()
    /// @details ZZ@> This function proceeds if the character was spawned this update

    SCRIPT_FUNCTION_BEGIN();

    // Proceed only if it's a new character
    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_SPAWNED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TimeOut( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfTimeOut()
    /// @details ZZ@> This function proceeds if the character's aitime is 0.  Use
    /// in conjunction with set_Time

    SCRIPT_FUNCTION_BEGIN();

    // Proceed only if time alert is set
    returncode = ( update_wld > pself->timer );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AtWaypoint( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfAtWaypoint()
    /// @details ZZ@> This function proceeds if the character reached its waypoint this
    /// update

    SCRIPT_FUNCTION_BEGIN();

    // Proceed only if the character reached a waypoint
    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_ATWAYPOINT );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AtLastWaypoint( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfAtLastWaypoint()
    /// @details ZZ@> This function proceeds if the character reached its last waypoint this
    /// update

    SCRIPT_FUNCTION_BEGIN();

    // Proceed only if the character reached its last waypoint
    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_ATLASTWAYPOINT );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Attacked( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfAttacked()
    /// @details ZZ@> This function proceeds if the character ( an item ) was put in its
    /// owner's pocket this update

    SCRIPT_FUNCTION_BEGIN();

    // Proceed only if the character was damaged
    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_ATTACKED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Bumped( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfBumped()
    /// @details ZZ@> This function proceeds if the character was bumped by another character
    /// this update

    SCRIPT_FUNCTION_BEGIN();

    // Proceed only if the character was bumped
    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_BUMPED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Ordered( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfOrdered()
    /// @details ZZ@> This function proceeds if the character got an order from another
    /// character on its team this update

    SCRIPT_FUNCTION_BEGIN();

    // Proceed only if the character was ordered
    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_ORDERED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_CalledForHelp( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfCalledForHelp()
    /// @details ZZ@> This function proceeds if one of the character's teammates was nearly
    /// killed this update

    SCRIPT_FUNCTION_BEGIN();

    // Proceed only if the character was called for help
    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_CALLEDFORHELP );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_Content( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetContent( tmpargument )
    /// @details ZZ@> This function sets the content variable.  Used in conjunction with
    /// GetContent.  Content is preserved from update to update

    SCRIPT_FUNCTION_BEGIN();

    // Set the content
    pself->content = pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Killed( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfKilled()
    /// @details ZZ@> This function proceeds if the character was killed this update

    SCRIPT_FUNCTION_BEGIN();

    // Proceed only if the character's been killed
    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_KILLED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetKilled( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfTargetKilled()
    /// @details ZZ@> This function proceeds if the character's target from last update was
    /// killed during this update

    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    // Proceed only if the character's target has just died or is already dead
    returncode = ( HAS_SOME_BITS( pself->alert, ALERTIF_TARGETKILLED ) || !pself_target->alive );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ClearWaypoints( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // ClearWaypoints()
    /// @details ZZ@> This function is used to move a character around.  Do this before
    /// AddWaypoint

    SCRIPT_FUNCTION_BEGIN();

    returncode = ego_waypoint_list::clear( &( pself->wp_lst ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AddWaypoint( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // AddWaypoint( tmpx = "x position", tmpy = "y position" )
    /// @details ZZ@> This function tells the character where to move next

    fvec3_t pos;

    SCRIPT_FUNCTION_BEGIN();

    // init the vector with the desired position
    pos.x = pstate->x;
    pos.y = pstate->y;
    pos.z = 0.0f;

    // is this a safe position?
    returncode = bfalse;

#if defined(REQUIRE_GOOD_WAYPOINTS)

    /// @note ZF@> I have uncommented the "unsafe" waypoint code for now. It caused many AI problems since many AI scripts depend on actually moving
    ///            towards an unsafe point. This should not be a problem since we have a mesh collision code anwyays?
    /// @note BB@> the problem was that characters like Brom and Mim would set a waypoint inside a wall and then never be able to reach it,
    ///            so they would just keep bumping into the wall and do nothing else (unless maybe attacked or something)

    if ( pbdl_self->cap_ptr->weight == 255 || !ego_mpd::hit_wall( PMesh, pos.v, pchr->bump.size, pchr->stoppedby, nrm.v, &pressure ) )
    {
        // yes it is safe. add it.
        returncode = ego_waypoint_list::push( &( pself->wp_lst ), pstate->x, pstate->y, pchr->fly_height );
    }

#    if EGO_DEBUG && defined(DEBUG_WAYPOINTS)
    else
    {
        // no it is not safe. what to do? nothing, or add the current position?
        //returncode = ego_waypoint_list::push( &(pself->wp_lst), pchr->pos.x, pchr->pos.y, pchr->fly_height );

        if ( NULL != pbdl_self->cap_ptr )
        {
            log_warning( "scr_AddWaypoint() - failed to add a waypoint because object was \"inside\" a wall.\n"
                         "\tcharacter %d (\"%s\", \"%s\")\n"
                         "\tWaypoint index %d\n"
                         "\tWaypoint location (in tiles) <%f,%f>\n"
                         "\tWall normal <%1.4f,%1.4f>\n"
                         "\tPressure %f\n",
                         GET_REF_PCHR( pchr ), pchr->name, pbdl_self->cap_ptr->name,
                         pself->wp_lst.head,
                         pos.x / GRID_SIZE, pos.y / GRID_SIZE,
                         nrm.x, nrm.y,
                         SQRT( pressure ) / GRID_SIZE );
        }
    }
#    endif

#else

    returncode = ego_waypoint_list::push( &( pself->wp_lst ), pstate->x, pstate->y, pchr->fly_height );

#endif

    if ( returncode )
    {
        // make sure we update the waypoint, since the list changed
        ego_ai_state::get_wp( pself );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_FindPath( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // FindPath
    /// @details ZF@> This modifies the eay the character moves relative to its target. There is no pth finding at this time.
    /// @todo scr_FindPath doesn't really work yet

    SCRIPT_FUNCTION_BEGIN();

    // Yep this is it
    if ( INGAME_CHR( pself->target ) && pself->target != pself->index )
    {
        float fx, fy;

        ego_chr * pself_target = ChrObjList.get_data_ptr( pself->target );

        if ( pstate->distance != MOVE_FOLLOW )
        {
            fx = pself_target->pos.x;
            fy = pself_target->pos.y;
        }
        else
        {
            fx = generate_randmask( -512, 1023 ) + pself_target->pos.x;
            fy = generate_randmask( -512, 1023 ) + pself_target->pos.y;
        }

        pstate->turn = vec_to_facing( fx - pchr->pos.x , fy - pchr->pos.y );

        if ( pstate->distance == MOVE_RETREAT )
        {
            // flip around to the other direction and add in some randomness
            pstate->turn += ATK_BEHIND + generate_randmask( -8192, 16383 );
        }
        pstate->turn = CLIP_TO_16BITS( pstate->turn );

        if ( pstate->distance == MOVE_CHARGE || pstate->distance == MOVE_RETREAT )
        {
            reset_character_accel( pself->index ); // Force 100% speed
        }

        // Then add the waypoint
        returncode = ego_waypoint_list::push( &( pself->wp_lst ), fx, fy, pchr->fly_height );

        if ( returncode )
        {
            // return the new position
            pstate->x = fx;
            pstate->y = fy;

            if ( returncode )
            {
                // make sure we update the waypoint, since the list changed
                ego_ai_state::get_wp( pself );
            }
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Compass( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // Compass( tmpturn = "rotation", tmpdistance = "radius" )
    /// @details ZZ@> This function modifies tmpx and tmpy, depending on the setting of
    /// tmpdistance and tmpturn.  It acts like one of those Compass thing
    /// with the two little needle legs

    TURN_T turn;

    SCRIPT_FUNCTION_BEGIN();

    turn = TO_TURN( pstate->turn );

    pstate->x -= turntocos[ turn ] * pstate->distance;
    pstate->y -= turntosin[ turn ] * pstate->distance;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_get_TargetArmorPrice( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // tmpx = GetTargetArmorPrice( tmpargument = "skin" )
    /// @details ZZ@> This function returns the cost of the desired skin upgrade, setting
    /// tmpx to the price

    Uint16 sTmp = 0;
    ego_cap * pcap;

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;

    pcap = ego_chr::get_pcap( pself->target );
    if ( NULL != pcap )
    {
        sTmp = pstate->argument % MAX_SKIN;

        pstate->x = pcap->skincost[sTmp];
        returncode = btrue;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_Time( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetTime( tmpargument = "time" )
    /// @details ZZ@> This function sets the character's ai timer.  50 clicks per second.
    /// Used in conjunction with IfTimeOut

    SCRIPT_FUNCTION_BEGIN();

    if ( pstate->argument > -1 )
    {
        pself->timer = update_wld + pstate->argument;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_get_Content( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // tmpargument = GetContent()
    /// @details ZZ@> This function sets tmpargument to the character's content variable.
    /// Used in conjunction with set_Content, or as a NOP to space out an Else

    SCRIPT_FUNCTION_BEGIN();

    // Get the content
    pstate->argument = pself->content;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_JoinTargetTeam( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // JoinTargetTeam()
    /// @details ZZ@> This function lets a character join a different team.  Used
    /// mostly for pets

    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = bfalse;
    if ( INGAME_CHR( pself->target ) )
    {
        switch_team( pself->index, pself_target->team );
        returncode = btrue;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToNearbyEnemy( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetTargetToNearbyEnemy()
    /// @details ZZ@> This function sets the target to a nearby enemy, failing if there are none

    CHR_REF ichr;

    SCRIPT_FUNCTION_BEGIN();

    ichr = chr_find_target( pchr, NEARBY, IDSZ_NONE, TARGET_ENEMIES );

    if ( INGAME_CHR( ichr ) )
    {
        SET_TARGET_0( ichr );
    }
    else
    {
        returncode = bfalse;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToTargetLeftHand( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetTargetToTargetLeftHand()
    /// @details ZZ@> This function sets the target to the item in the target's left hand,
    /// failing if the target has no left hand item

    CHR_REF ichr;
    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    ichr = pself_target->holdingwhich[SLOT_LEFT];
    returncode = bfalse;
    if ( INGAME_CHR( ichr ) )
    {
        SET_TARGET( ichr, pself_target );
        returncode = btrue;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToTargetRightHand( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetTargetToTargetRightHand()
    /// @details ZZ@> This function sets the target to the item in the target's right hand,
    /// failing if the target has no right hand item

    CHR_REF ichr;
    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    ichr = pself_target->holdingwhich[SLOT_RIGHT];
    returncode = bfalse;
    if ( INGAME_CHR( ichr ) )
    {
        SET_TARGET( ichr, pself_target );
        returncode = btrue;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToWhoeverAttacked( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetTargetToWhoeverAttacked()
    /// @details ZZ@> This function sets the target to whoever attacked the character last, failing for damage tiles

    SCRIPT_FUNCTION_BEGIN();

    if ( INGAME_CHR( pself->attacklast ) )
    {
        SET_TARGET_0( pself->attacklast );
    }
    else
    {
        returncode = bfalse;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToWhoeverBumped( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetTargetToWhoeverBumped()
    /// @details ZZ@> This function sets the target to whoever bumped the character last. It never fails

    SCRIPT_FUNCTION_BEGIN();

    if ( INGAME_CHR( pself->bumplast ) )
    {
        SET_TARGET_0( pself->bumplast );
    }
    else
    {
        returncode = bfalse;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToWhoeverCalledForHelp( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetTargetToWhoeverCalledForHelp()
    /// @details ZZ@> This function sets the target to whoever called for help last.

    SCRIPT_FUNCTION_BEGIN();

    if ( TeamStack.in_range_ref( pchr->team ) )
    {
        CHR_REF isissy = TeamStack[pchr->team].sissy;

        if ( INGAME_CHR( isissy ) )
        {
            SET_TARGET_0( isissy );
        }
        else
        {
            returncode = bfalse;
        }
    }
    else
    {
        returncode = bfalse;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToOldTarget( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetTargetToOldTarget()
    /// @details ZZ@> This function sets the target to the target from last update, used to
    /// undo other set_Target functions

    SCRIPT_FUNCTION_BEGIN();

    if ( INGAME_CHR( pself->target_old ) )
    {
        SET_TARGET_0( pself->target_old );
    }
    else
    {
        returncode = bfalse;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TurnModeToVelocity( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetTurnModeToVelocity()
    /// @details ZZ@> This function sets the character's movement mode to the default

    SCRIPT_FUNCTION_BEGIN();

    pchr->turnmode = TURNMODE_VELOCITY;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TurnModeToWatch( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetTurnModeToWatch()
    /// @details ZZ@> This function makes the character look at its next waypoint, usually
    /// used with close waypoints or the Stop function

    SCRIPT_FUNCTION_BEGIN();

    pchr->turnmode = TURNMODE_ACCELERATION;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TurnModeToSpin( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetTurnModeToSpin()
    /// @details ZZ@> This function makes the character spin around in a circle, usually
    /// used for magical items and such

    SCRIPT_FUNCTION_BEGIN();

    pchr->turnmode = TURNMODE_SPIN;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_BumpHeight( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetBumpHeight( tmpargument = "height" )
    /// @details ZZ@> This function makes the character taller or shorter, usually used when
    /// the character dies

    SCRIPT_FUNCTION_BEGIN();

    ego_chr::set_height( pchr, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetHasID( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfTargetHasID( tmpargument = "idsz" )
    /// @details ZZ@> This function proceeds if the target has either a parent or type IDSZ
    /// matching tmpargument.

    SCRIPT_FUNCTION_BEGIN();

    returncode = ego_chr::is_type_idsz( pself->target, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetHasItemID( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfTargetHasItemID( tmpargument = "idsz" )
    /// @details ZZ@> This function proceeds if the target has a matching item in his/her
    /// pockets or hands.

    CHR_REF item;

    SCRIPT_FUNCTION_BEGIN();

    item = chr_has_item_idsz( pself->target, ( IDSZ ) pstate->argument, bfalse, NULL );

    returncode = INGAME_CHR( item );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetHoldingItemID( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfTargetHoldingItemID( tmpargument = "idsz" )
    /// @details ZZ@> This function proceeds if the target has a matching item in his/her
    /// hands.  It also sets tmpargument to the proper latch button to press
    /// to use that item

    CHR_REF item;

    SCRIPT_FUNCTION_BEGIN();

    item = chr_holding_idsz( pself->target, pstate->argument );

    returncode = INGAME_CHR( item );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetHasSkillID( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfTargetHasSkillID( tmpargument = "skill idsz" )
    /// @details ZZ@> This function proceeds if ID matches tmpargument

    ego_chr *pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = ( 0 != ego_chr_data::get_skill( pself_target, ( IDSZ )pstate->argument ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Else( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // Else
    /// @details ZZ@> This function fails if the last function was more indented

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( pself->indent >= pself->indent_last );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Run( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // Run()
    /// @details ZZ@> This function sets the character's maximum acceleration to its
    /// actual maximum

    SCRIPT_FUNCTION_BEGIN();

    reset_character_accel( pself->index );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Walk( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // Walk()
    /// @details ZZ@> This function sets the character's maximum acceleration to 66%
    /// of its actual maximum

    SCRIPT_FUNCTION_BEGIN();

    reset_character_accel( pself->index );

    pchr->maxaccel      = pchr->maxaccel_reset * 0.66f;
    pchr->movement_bits = CHR_MOVEMENT_BITS_WALK;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Sneak( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // Sneak()
    /// @details ZZ@> This function sets the character's maximum acceleration to 33%
    /// of its actual maximum

    SCRIPT_FUNCTION_BEGIN();

    reset_character_accel( pself->index );

    pchr->maxaccel      = pchr->maxaccel_reset * 0.33f;
    pchr->movement_bits = CHR_MOVEMENT_BITS_SNEAK | CHR_MOVEMENT_BITS_STOP;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DoAction( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // DoAction( tmpargument = "action" )
    /// @details ZZ@> This function makes the character do a given action if it isn't doing
    /// anything better.  Fails if the action is invalid or if the character is doing
    /// something else already

    int action;

    SCRIPT_FUNCTION_BEGIN();

    action = mad_get_action( pchr->mad_inst.imad, pstate->argument );

    returncode = bfalse;
    if ( rv_success == ego_chr::start_anim( pchr, action, bfalse, bfalse ) )
    {
        returncode = btrue;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_KeepAction( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // KeepAction()
    /// @details ZZ@> This function makes the character's animation stop on its last frame
    /// and stay there.  Usually used for dropped items

    SCRIPT_FUNCTION_BEGIN();

    pchr->mad_inst.action.keep = btrue;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IssueOrder( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IssueOrder( tmpargument = "order"  )
    /// @details ZZ@> This function tells all of the character's teammates to do something,
    /// though each teammate needs to interpret the order using IfOrdered in
    /// its own script.

    SCRIPT_FUNCTION_BEGIN();

    issue_order( pself->index, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DropWeapons( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // DropWeapons()
    /// @details ZZ@> This function drops the character's in-hand items.  It will also
    /// buck the rider if the character is a mount

    CHR_REF ichr;

    SCRIPT_FUNCTION_BEGIN();

    // This funtion drops the character's in hand items/riders
    ichr = pchr->holdingwhich[SLOT_LEFT];
    if ( INGAME_CHR( ichr ) )
    {
        ego_chr * tmp_pchr = ChrObjList.get_data_ptr( ichr );

        detach_character_from_mount( ichr, btrue, btrue );
        if ( pchr->ismount )
        {
            fvec3_t tmp_pos;

            tmp_pchr->vel.z    = DISMOUNTZVEL;
            tmp_pchr->jump_time = JUMP_DELAY;

            tmp_pos = ego_chr::get_pos( tmp_pchr );
            tmp_pos.z += DISMOUNTZVEL;
            ego_chr::set_pos( tmp_pchr, tmp_pos.v );
        }
    }

    ichr = pchr->holdingwhich[SLOT_RIGHT];
    if ( INGAME_CHR( ichr ) )
    {
        ego_chr * tmp_pchr = ChrObjList.get_data_ptr( ichr );

        detach_character_from_mount( ichr, btrue, btrue );
        if ( pchr->ismount )
        {
            fvec3_t tmp_pos;

            tmp_pchr->vel.z    = DISMOUNTZVEL;
            tmp_pchr->jump_time = JUMP_DELAY;

            tmp_pos = ego_chr::get_pos( tmp_pchr );
            tmp_pos.z += DISMOUNTZVEL;
            ego_chr::set_pos( tmp_pchr, tmp_pos.v );
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetDoAction( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // TargetDoAction( tmpargument = "action" )
    /// @details ZZ@> The function makes the target start a new action, if it is valid for the model
    /// It will fail if the action is invalid or if the target is doing
    /// something else already

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( INGAME_CHR( pself->target ) )
    {
        ego_chr * pself_target = ChrObjList.get_data_ptr( pself->target );

        if ( pself_target->alive )
        {
            int action = mad_get_action( pself_target->mad_inst.imad, pstate->argument );

            if ( rv_success == ego_chr::start_anim( pself_target, action, bfalse, bfalse ) )
            {
                returncode = btrue;
            }
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_OpenPassage( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // OpenPassage( tmpargument = "passage" )

    /// @details ZZ@> This function opens the passage specified by tmpargument, failing if the
    /// passage was already open.
    /// Passage areas are defined in passage.txt and set in spawn.txt for the given character

    SCRIPT_FUNCTION_BEGIN();

    returncode = PassageStack_open( PASS_REF( pstate->argument ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ClosePassage( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // ClosePassage( tmpargument = "passage" )
    /// @details ZZ@> This function closes the passage specified by tmpargument, proceeding
    /// if the passage isn't blocked.  Crushable characters within the passage
    /// are crushed.

    SCRIPT_FUNCTION_BEGIN();

    returncode = PassageStack_close_one( PASS_REF( pstate->argument ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_PassageOpen( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfPassageOpen( tmpargument = "passage" )
    /// @details ZZ@> This function proceeds if the given passage is valid and open to movement
    /// Used mostly by door characters to tell them when to run their open animation.

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( pstate->argument >= 0 && pstate->argument < MAX_PASS )
    {
        PASS_REF ipass = PASS_REF( pstate->argument );

        returncode = PassageStack[ipass].open;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GoPoof( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // GoPoof()
    /// @details ZZ@> This function flags the character to be removed from the game entirely.
    /// This doesn't work on players

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( !VALID_PLA( pchr->is_which_player ) )
    {
        returncode = btrue;
        pself->poof_time = update_wld;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_CostTargetItemID( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // CostTargetItemID( tmpargument = "idsz" )
    /// @details ZZ@> This function proceeds if the target has a matching item, and poofs
    /// that item.
    /// For one use keys and such

    CHR_REF item, pack_last, ichr;

    SCRIPT_FUNCTION_BEGIN();

    ichr = pself->target;
    item = chr_has_item_idsz( ichr, ( IDSZ ) pstate->argument, bfalse, &pack_last );

    returncode = bfalse;
    if ( INGAME_CHR( item ) )
    {
        returncode = btrue;

        if ( ChrObjList.get_data_ref( item ).ammo > 1 )
        {
            // Cost one ammo
            ChrObjList.get_data_ref( item ).ammo--;
        }
        else
        {
            // Poof the item

            // remove it from a pack, if it was packed
            pack_remove_item( &( ChrObjList.get_data_ref( ichr ).pack ), pack_last, item );

            // Drop from hand, if held
            detach_character_from_mount( item, btrue, bfalse );

            // get rid of the character, no matter what
            ego_obj_chr::request_terminate( item );
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DoActionOverride( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // DoActionOverride( tmpargument = "action" )
    /// @details ZZ@> This function makes the character do a given action no matter what
    /// It will fail if the action is invalid

    int action;

    SCRIPT_FUNCTION_BEGIN();

    action = mad_get_action( pchr->mad_inst.imad, pstate->argument );

    returncode = bfalse;
    if ( rv_success == ego_chr::start_anim( pchr, action, bfalse, btrue ) )
    {
        returncode = btrue;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Healed( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfHealed()
    /// @details ZZ@> This function proceeds if the character was healed by a healing particle

    SCRIPT_FUNCTION_BEGIN();

    // Proceed only if the character was healed
    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_HEALED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SendPlayerMessage( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SendPlayerMessage( tmpargument = "message number" )
    /// @details ZZ@> This function sends a message to the players

    SCRIPT_FUNCTION_BEGIN();

    returncode = _display_message( pself->index, pchr->profile_ref, pstate->argument, pstate );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_CallForHelp( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // CallForHelp()
    /// @details ZZ@> This function calls all of the character's teammates for help.  The
    /// teammates must use IfCalledForHelp in their scripts

    SCRIPT_FUNCTION_BEGIN();

    call_for_help( pself->index );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AddIDSZ( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // AddIDSZ( tmpargument = "idsz" )
    /// @details ZZ@> This function slaps an expansion IDSZ onto the menu.txt file.
    /// Used to show completion of special quests for a given module

    SCRIPT_FUNCTION_BEGIN();

    if ( module_add_idsz_vfs( pickedmodule.name, pstate->argument, 0, NULL ) )
    {
        // we need to invalidate the module list
        module_list_valid = bfalse;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_State( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetState( tmpargument = "state" )
    /// @details ZZ@> This function sets the character's state.
    /// VERY IMPORTANT. State is preserved from update to update

    SCRIPT_FUNCTION_BEGIN();

    // set the state
    pself->state = pstate->argument;

    // determine whether the object is hidden
    ego_chr::update_hide( pchr );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_get_State( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // tmpargument = GetState()
    /// @details ZZ@> This function reads the character's state variable

    SCRIPT_FUNCTION_BEGIN();

    pstate->argument = pself->state;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_StateIs( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfStateIs( tmpargument = "state" )
    /// @details ZZ@> This function proceeds if the character's state equals tmpargument

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( pstate->argument == pself->state );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetCanOpenStuff( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfTargetCanOpenStuff()
    /// @details ZZ@> This function proceeds if the target can open stuff ( set in data.txt )
    /// Used by chests and buttons and such so only "smart" creatures can operate
    /// them

    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();
    returncode = bfalse;

    SCRIPT_REQUIRE_TARGET( pself_target );

    if ( pself_target->ismount )
    {
        CHR_REF iheld = pself_target->holdingwhich[SLOT_LEFT];

        if ( DEFINED_CHR( iheld ) )
        {
            // can the rider open the
            returncode = ChrObjList.get_data_ref( iheld ).openstuff;
        }
    }

    if ( !returncode )
    {
        // if a rider can't openstuff, can the target openstuff?
        returncode = pself_target->openstuff;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Grabbed( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfGrabbed()
    /// @details ZZ@> This function proceeds if the character was grabbed (picked up) this update.
    /// Used mostly by item characters

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_GRABBED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Dropped( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfDropped()
    /// @details ZZ@> This function proceeds if the character was dropped this update.
    /// Used mostly by item characters

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_DROPPED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToWhoeverIsHolding( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetTargetToWhoeverIsHolding()
    /// @details ZZ@> This function sets the target to the character's holder or mount,
    /// failing if the character has no mount or holder

    SCRIPT_FUNCTION_BEGIN();

    if ( INGAME_CHR( pchr->attachedto ) )
    {
        SET_TARGET_0( pchr->attachedto );
    }
    else
    {
        returncode = bfalse;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DamageTarget( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // DamageTarget( tmpargument = "damage" )
    /// @details ZZ@> This function applies little bit of love to the character's target.
    /// The amount is set in tmpargument

    IPair tmp_damage;

    SCRIPT_FUNCTION_BEGIN();

    tmp_damage.base = pstate->argument;
    tmp_damage.rand = 1;

    damage_character( pself->target, ATK_FRONT, tmp_damage, pchr->damagetargettype, pchr->team, pself->index, DAMFX_NBLOC, btrue );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_XIsLessThanY( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfXIsLessThanY( tmpx, tmpy )
    /// @details ZZ@> This function proceeds if tmpx is less than tmpy.

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( pstate->x < pstate->y );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_WeatherTime( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetWeatherTime( tmpargument = "time" )
    /// @details ZZ@> This function can be used to slow down or speed up or stop rain and
    /// other weather effects

    SCRIPT_FUNCTION_BEGIN();

    // Set the weather timer
    weather.timer_reset = pstate->argument;
    weather.time = pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_get_BumpHeight( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // tmpargument = GetBumpHeight()
    /// @details ZZ@> This function sets tmpargument to the character's height

    SCRIPT_FUNCTION_BEGIN();

    pstate->argument = pchr->bump.height;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Reaffirmed( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfReaffirmed()
    /// @details ZZ@> This function proceeds if the character was damaged by its reaffirm
    /// damage type.  Used to relight the torch.

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_REAFFIRMED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_UnkeepAction( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // UnkeepAction()
    /// @details ZZ@> This function is the opposite of KeepAction. It makes the current animation resume.

    SCRIPT_FUNCTION_BEGIN();

    pchr->mad_inst.action.keep = bfalse;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsOnOtherTeam( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfTargetIsOnOtherTeam()
    /// @details ZZ@> This function proceeds if the target is on another team

    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = ( pself_target->alive && ego_chr::get_iteam( pself->target ) != pchr->team );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsOnHatedTeam( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfTargetIsOnHatedTeam()
    /// @details ZZ@> This function proceeds if the target is on an enemy team

    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = ( pself_target->alive && team_hates_team( pchr->team, ego_chr::get_iteam( pself->target ) ) && !IS_INVICTUS_PCHR_RAW( pself_target ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_PressLatchButton( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // PressLatchButton( tmpargument = "latch bits" )
    /// @details ZZ@> This function sets the character latch buttons

    SCRIPT_FUNCTION_BEGIN();

    ADD_BITS( pchr->latch.trans.b, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToTargetOfLeader( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetTargetToTargetOfLeader()
    /// @details ZZ@> This function sets the character's target to the target of its leader,
    /// or it fails with no change if the leader is dead

    SCRIPT_FUNCTION_BEGIN();

    if ( TeamStack.in_range_ref( pchr->team ) )
    {
        CHR_REF ileader = TeamStack[pchr->team].leader;

        if ( NOLEADER != ileader && INGAME_CHR( ileader ) )
        {
            CHR_REF itarget = ChrObjList.get_data_ref( ileader ).ai.target;

            if ( INGAME_CHR( itarget ) )
            {
                SET_TARGET_0( itarget );
            }
            else
            {
                returncode = bfalse;
            }
        }
        else
        {
            returncode = bfalse;
        }
    }
    else
    {
        returncode = bfalse;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_LeaderKilled( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfLeaderKilled()
    /// @details ZZ@> This function proceeds if the team's leader died this update

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_LEADERKILLED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_BecomeLeader( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // BecomeLeader()
    /// @details ZZ@> This function makes the character the leader of the team

    SCRIPT_FUNCTION_BEGIN();

    TeamStack[pchr->team].leader = pself->index;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ChangeTargetArmor( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // ChangeTargetArmor( tmpargument = "armor" )

    /// @details ZZ@> This function sets the target's armor type and returns the old type
    /// as tmpargument and the new type as tmpx

    int iTmp;
    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    iTmp = pself_target->skin;
    pstate->x = ego_chr::change_armor( pself->target, pstate->argument );

    pstate->argument = iTmp;  // The character's old armor

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GiveMoneyToTarget( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // GiveMoneyToTarget( tmpargument = "money" )
    /// @details ZZ@> This function increases the target's money, while decreasing the
    /// character's own money.  tmpargument is set to the amount transferred

    int tTmp;
    int iTmp;
    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    iTmp = pchr->money;
    tTmp = pself_target->money;
    iTmp -= pstate->argument;
    tTmp += pstate->argument;
    if ( iTmp < 0 ) { tTmp += iTmp;  pstate->argument += iTmp;  iTmp = 0; }
    if ( tTmp < 0 ) { iTmp += tTmp;  pstate->argument += tTmp;  tTmp = 0; }
    if ( iTmp > MAXMONEY ) { iTmp = MAXMONEY; }
    if ( tTmp > MAXMONEY ) { tTmp = MAXMONEY; }

    pchr->money = iTmp;
    pself_target->money = tTmp;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DropKeys( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // DropKeys()
    /// @details ZZ@> This function drops all of the keys in the character's inventory.
    /// This does NOT drop keys in the character's hands.

    SCRIPT_FUNCTION_BEGIN();

    drop_all_idsz( pself->index, MAKE_IDSZ( 'K', 'E', 'Y', 'A' ), MAKE_IDSZ( 'K', 'E', 'Y', 'Z' ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_LeaderIsAlive( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfLeaderIsAlive()
    /// @details ZZ@> This function proceeds if the team has a leader

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( TeamStack[pchr->team].leader != NOLEADER );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsOldTarget( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfTargetIsOldTarget()
    /// @details ZZ@> This function proceeds if the target is the same as it was last update

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( pself->target == pself->target_old );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToLeader( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetTargetToLeader()
    /// @details ZZ@> This function sets the target to the leader, proceeding if there is
    /// a valid leader for the character's team

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( TeamStack.in_range_ref( pchr->team ) )
    {
        CHR_REF ileader = TeamStack[pchr->team].leader;

        if ( NOLEADER != ileader && INGAME_CHR( ileader ) )
        {
            SET_TARGET_0( ileader );
            returncode = btrue;
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnCharacter( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SpawnCharacter( tmpx = "x", tmpy = "y", tmpturn = "turn", tmpdistance = "speed" )

    /// @details ZZ@> This function spawns a character of the same type as the spawner.
    /// This function spawns a character, failing if x,y is invalid
    /// This is horribly complicated to use, so see ANIMATE.OBJ for an example
    /// tmpx and tmpy give the coodinates, tmpturn gives the new character's
    /// direction, and tmpdistance gives the new character's initial velocity

    CHR_REF ichr;
    int tTmp;
    fvec3_t   pos;

    SCRIPT_FUNCTION_BEGIN();

    pos.x = pstate->x;
    pos.y = pstate->y;
    pos.z = 0;

    ichr = spawn_one_character( pos, pchr->profile_ref, pchr->team, 0, CLIP_TO_16BITS( pstate->turn ), NULL, CHR_REF( MAX_CHR ) );
    if ( !INGAME_CHR( ichr ) )
    {
        if ( ichr > PMod->importamount * MAXIMPORTPERPLAYER )
        {
            log_warning( "Object %s failed to spawn a copy of itself\n", ego_chr::get_obj_ref( *pchr ).base_name );
        }
    }
    else
    {
        ego_chr * pchild = ChrObjList.get_data_ptr( ichr );

        // was the child spawned in a "safe" spot?
        if ( !ego_chr::get_safe( pchild, NULL ) )
        {
            ego_obj_chr::request_terminate( ichr );
            ichr = CHR_REF( MAX_CHR );
        }
        else
        {
            pself->child = ichr;

            tTmp = TO_TURN( pchr->ori.facing_z + ATK_BEHIND );

            pchild->vel.x += turntocos[ tTmp ] * pstate->distance;
            pchild->vel.y += turntosin[ tTmp ] * pstate->distance;

            pchild->iskursed = pchr->iskursed;  /// @details BB@> inherit this from your spawner
            pchild->ai.passage = pself->passage;
            pchild->ai.owner   = pself->owner;

            pchild->dismount_timer  = PHYS_DISMOUNT_TIME;
            pchild->dismount_object = pself->index;
        }
    }

    returncode = INGAME_CHR( ichr );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_RespawnCharacter( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // RespawnCharacter()
    /// @details ZZ@> This function respawns the character at its starting location.
    /// Often used with the Clean functions

    SCRIPT_FUNCTION_BEGIN();

    ego_chr::respawn( pself->index );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ChangeTile( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // ChangeTile( tmpargument = "tile type")
    /// @details ZZ@> This function changes the tile under the character to the new tile type,
    /// which is highly module dependent

    SCRIPT_FUNCTION_BEGIN();

    returncode = ego_mpd::set_texture( PMesh, pchr->onwhichgrid, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Used( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfUsed()
    /// @details ZZ@> This function proceeds if the character was used by its holder or rider.
    /// Character's cannot be used if their reload time is greater than 0

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_USED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DropMoney( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // DropMoney( tmpargument = "money" )
    /// @details ZZ@> This function drops a certain amount of money, if the character has that
    /// much

    SCRIPT_FUNCTION_BEGIN();

    drop_money( pself->index, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_OldTarget( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetOldTarget()
    /// @details ZZ@> This function sets the old target to the current target.  To allow
    /// greater manipulations of the target

    SCRIPT_FUNCTION_BEGIN();

    pself->target_old = pself->target;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DetachFromHolder( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // DetachFromHolder()
    /// @details ZZ@> This function drops the character or makes it get off its mount
    /// Can be used to make slippery weapons, or to make certain characters
    /// incapable of wielding certain weapons. "A troll can't grab a torch"

    SCRIPT_FUNCTION_BEGIN();

    if ( INGAME_CHR( pchr->attachedto ) )
    {
        detach_character_from_mount( pself->index, btrue, btrue );
    }
    else
    {
        returncode = bfalse;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetHasVulnerabilityID( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfTargetHasVulnerabilityID( tmpargument = "vulnerability idsz" )
    /// @details ZZ@> This function proceeds if the target is vulnerable to the given IDSZ.

    ego_cap * pcap;

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;

    pcap = ego_chr::get_pcap( pself->target );
    if ( NULL != pcap )
    {
        returncode = ( pcap->idsz[IDSZ_VULNERABILITY] == ( IDSZ ) pstate->argument );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_CleanUp( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // CleanUp()
    /// @details ZZ@> This function tells all the dead characters on the team to clean
    /// themselves up.  Usually done by the boss creature every second or so

    SCRIPT_FUNCTION_BEGIN();

    issue_clean( pself->index );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_CleanedUp( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfCleanedUp()
    /// @details ZZ@> This function proceeds if the character is dead and if the boss told it
    /// to clean itself up

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_CLEANEDUP );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Sitting( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfSitting()
    /// @details ZZ@> This function proceeds if the character is riding a mount

    SCRIPT_FUNCTION_BEGIN();

    returncode = INGAME_CHR( pchr->attachedto );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsHurt( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfTargetIsHurt()
    /// @details ZZ@> This function passes only if the target is hurt and alive

    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    if ( !pself_target->alive || pself_target->life > pself_target->life_max - HURTDAMAGE )
        returncode = bfalse;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsAPlayer( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfTargetIsAPlayer()
    /// @details ZZ@> This function proceeds if the target is controlled by a human ( may not be local )

    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = VALID_PLA( pself_target->is_which_player );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_PlaySound( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // PlaySound( tmpargument = "sound" )
    /// @details ZZ@> This function plays one of the character's sounds.
    /// The sound fades out depending on its distance from the viewer

    SCRIPT_FUNCTION_BEGIN();

    if ( pchr->pos_old.z > PITNOSOUND && VALID_SND( pstate->argument ) )
    {
        sound_play_chunk( pchr->pos_old, ego_chr::get_chunk_ptr( pchr, pstate->argument ) );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnParticle( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SpawnParticle(tmpargument = "particle", tmpdistance = "character vertex", tmpx = "offset x", tmpy = "offset y" )
    /// @details ZZ@> This function spawns a particle, offset from the character's location

    PRT_REF iprt;
    CHR_REF ichr;

    SCRIPT_FUNCTION_BEGIN();

    ichr = pself->index;
    if ( INGAME_CHR( pchr->attachedto ) )
    {
        ichr = pchr->attachedto;
    }

    iprt = spawn_one_particle( pchr->pos, pchr->ori.facing_z, pchr->profile_ref, pstate->argument, pself->index, pstate->distance, pchr->team, ichr, PRT_REF( MAX_PRT ), 0, CHR_REF( MAX_CHR ) );

    ego_prt * pprt = PrtObjList.get_allocated_data_ptr( iprt );
    returncode = ( NULL != pprt );

    if ( returncode )
    {
        fvec3_t tmp_pos;

        // attach the particle
        place_particle_at_vertex( pprt, pself->index, pstate->distance );
        pprt->attachedto_ref = CHR_REF( MAX_CHR );

        tmp_pos = ego_prt::get_pos( pprt );

        // Correct X, Y, Z spacing
        tmp_pos.z += PipStack[pprt->pip_ref].spacing_vrt_pair.base;

        // Don't spawn in walls
        tmp_pos.x += pstate->x;
        if ( prt_test_wall( pprt, tmp_pos.v ) )
        {
            tmp_pos.x = pprt->pos.x;

            tmp_pos.y += pstate->y;
            if ( prt_test_wall( pprt, tmp_pos.v ) )
            {
                tmp_pos.y = pprt->pos.y;
            }
        }

        ego_prt::set_pos( pprt, tmp_pos.v );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsAlive( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfTargetIsAlive()
    /// @details ZZ@> This function proceeds if the target is alive

    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = pself_target->alive;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Stop( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // Stop()
    /// @details ZZ@> This function sets the character's maximum acceleration to 0.  Used
    /// along with Walk and Run and Sneak

    SCRIPT_FUNCTION_BEGIN();

    pchr->maxaccel      = 0;
    pchr->movement_bits = CHR_MOVEMENT_BITS_STOP;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DisaffirmCharacter( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // DisaffirmCharacter()
    /// @details ZZ@> This function removes all the attached particles from a character
    /// ( stuck arrows, flames, etc )

    SCRIPT_FUNCTION_BEGIN();

    disaffirm_attached_particles( pself->index );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ReaffirmCharacter( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // ReaffirmCharacter()
    /// @details ZZ@> This function makes sure it has all of its reaffirmation particles
    /// attached to it. Used to make the torch light again

    SCRIPT_FUNCTION_BEGIN();

    reaffirm_attached_particles( pself->index );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsSelf( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfTargetIsSelf()
    /// @details ZZ@> This function proceeds if the character is targeting itself

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( pself->target == pself->index );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsMale( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfTargetIsMale()
    /// @details ZZ@> This function proceeds only if the target is male

    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = ( pself_target->gender == GENDER_MALE );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsFemale( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfTargetIsFemale()
    /// @details ZZ@> This function proceeds if the target is female

    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = ( pself_target->gender == GENDER_FEMALE );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToSelf( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetTargetToSelf()
    /// @details ZZ@> This function sets the target to the character itself

    SCRIPT_FUNCTION_BEGIN();

    SET_TARGET_0( pself->index );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToRider( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetTargetToRider()
    /// @details ZZ@> This function sets the target to whoever is riding the character (left/only grip),
    /// failing if there is no rider

    SCRIPT_FUNCTION_BEGIN();

    if ( INGAME_CHR( pchr->holdingwhich[SLOT_LEFT] ) )
    {
        SET_TARGET_0( pchr->holdingwhich[SLOT_LEFT] );
    }
    else
    {
        returncode = bfalse;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_get_AttackTurn( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // tmpturn = GetAttackTurn()
    /// @details ZZ@> This function sets tmpturn to the direction from which the last attack
    /// came. Not particularly useful in most cases, but it could be.

    SCRIPT_FUNCTION_BEGIN();

    pstate->turn = pself->directionlast;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_get_DamageType( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // tmpargument = GetDamageType()
    /// @details ZZ@> This function sets tmpargument to the damage type of the last attack that
    /// hit the character

    SCRIPT_FUNCTION_BEGIN();

    pstate->argument = pself->damagetypelast;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_BecomeSpell( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // BecomeSpell()
    /// @details ZZ@> This function turns a spellbook character into a spell based on its
    /// content.
    /// TOO COMPLICATED TO EXPLAIN.  SHOULDN'T EVER BE NEEDED BY YOU.

    int iskin;

    SCRIPT_FUNCTION_BEGIN();

    // get the spellbook's skin
    iskin = pchr->skin;

    // change the spellbook to a spell effect
    ego_chr::change_profile( pself->index, PRO_REF( pself->content ), 0, ENC_LEAVE_NONE );

    // set the spell effect parameters
    pself->content = 0;
    pself->state   = 0;

    // have to do this every time pself->state is modified
    ego_chr::update_hide( pchr );

    // set the book icon of the spell effect if it is not already set
    if ( NULL != pbdl_self->cap_ptr )
    {
        pbdl_self->cap_ptr->spelleffect_type = iskin;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_BecomeSpellbook( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // BecomeSpellbook()
    //
    /// @details ZZ@> This function turns a spell character into a spellbook and sets the content accordingly.
    /// TOO COMPLICATED TO EXPLAIN. Just copy the spells that already exist, and don't change
    /// them too much

    PRO_REF  old_profile;
    ego_mad * pmad;
    int iskin;

    SCRIPT_FUNCTION_BEGIN();

    // Figure out what this spellbook looks like
    iskin = 0;
    if ( NULL != pbdl_self->cap_ptr ) iskin = pbdl_self->cap_ptr->spelleffect_type;

    // convert the spell effect to a spellbook
    old_profile = pchr->profile_ref;
    ego_chr::change_profile( pself->index, PRO_REF( SPELLBOOK ), iskin, ENC_LEAVE_NONE );

    // Reset the spellbook state so it doesn't burn up
    pself->state   = 0;
    pself->content = old_profile.get_value();

    // set the spellbook animations
    pmad = ego_chr::get_pmad( pself->index );

    if ( NULL != pmad )
    {
        // Do dropped animation
        int tmp_action = mad_get_action( pchr->mad_inst.imad, ACTION_JB );

        if ( rv_success == ego_chr::start_anim( pchr, tmp_action, bfalse, btrue ) )
        {
            returncode = btrue;
        }
    }

    // have to do this every time pself->state is modified
    ego_chr::update_hide( pchr );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ScoredAHit( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfScoredAHit()
    /// @details ZZ@> This function proceeds if the character damaged another character this
    /// update. If it's a held character it also sets the target to whoever was hit

    SCRIPT_FUNCTION_BEGIN();

    // Proceed only if the character scored a hit
    if ( !INGAME_CHR( pchr->attachedto ) || ChrObjList.get_data_ref( pchr->attachedto ).ismount )
    {
        returncode = HAS_SOME_BITS( pself->alert, ALERTIF_SCOREDAHIT );
    }

    // Proceed only if the holder scored a hit with the character
    else if ( ChrObjList.get_data_ref( pchr->attachedto ).ai.lastitemused == pself->index )
    {
        returncode = HAS_SOME_BITS( ChrObjList.get_data_ref( pchr->attachedto ).ai.alert, ALERTIF_SCOREDAHIT );
    }
    else returncode = bfalse;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Disaffirmed( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfDisaffirmed()
    /// @details ZZ@> This function proceeds if the character was disaffirmed.
    /// This doesn't seem useful anymore.

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_DISAFFIRMED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TranslateOrder( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // tmpx,tmpy,tmpargument = TranslateOrder()
    /// @details ZZ@> This function translates a packed order into understandable values.
    /// See CreateOrder for more.  This function sets tmpx, tmpy, tmpargument,
    /// and sets the target ( if valid )

    CHR_REF ichr;

    SCRIPT_FUNCTION_BEGIN();

    ichr = CLIP_TO_16BITS( pself->order_value >> 24 );

    if ( INGAME_CHR( ichr ) )
    {
        SET_TARGET_0( ichr );

        pstate->x        = (( pself->order_value >> 14 ) & 0x03FF ) << 6;
        pstate->y        = (( pself->order_value >>  4 ) & 0x03FF ) << 6;
        pstate->argument = (( pself->order_value >>  0 ) & 0x000F );
    }
    else
    {
        returncode = bfalse;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToWhoeverWasHit( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetTargetToWhoeverWasHit()
    /// @details ZZ@> This function sets the target to whoever was hit by the character last

    SCRIPT_FUNCTION_BEGIN();

    if ( INGAME_CHR( pself->hitlast ) )
    {
        SET_TARGET_0( pself->hitlast );
    }
    else
    {
        returncode = bfalse;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToWideEnemy( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetTargetToWideEnemy()
    /// @details ZZ@> This function sets the target to an enemy in the vicinity around the
    /// character, failing if there are none

    CHR_REF ichr;
    SCRIPT_FUNCTION_BEGIN();

    ichr = chr_find_target( pchr, WIDE, IDSZ_NONE, TARGET_ENEMIES );

    if ( INGAME_CHR( ichr ) )
    {
        SET_TARGET_0( ichr );
    }
    else
    {
        returncode = bfalse;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Changed( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfChanged()
    /// @details ZZ@> This function proceeds if the character was polymorphed.
    /// Needed for morph spells and such

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_CHANGED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_InWater( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfInWater()
    /// @details ZZ@> This function proceeds if the character has just entered into some water
    /// this update ( and the water is really water, not fog or another effect )

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_INWATER );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Bored( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfBored()
    /// @details ZZ@> This function proceeds if the character has been standing idle too long

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_BORED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TooMuchBaggage( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfTooMuchBaggage()
    /// @details ZZ@> This function proceeds if the character tries to put an item in his/her
    /// pockets, but the character already has 6 items in the inventory.
    /// Used to tell the players what's going on.

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_TOOMUCHBAGGAGE );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Confused( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfConfused() or IfGrogged() or IfDazed()
    /// @details ZZ@> This function proceeds if the character has been either grogged or dazed this update

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_CONFUSED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetHasSpecialID( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfTargetHasSpecialID( tmpargument = "special idsz" )
    /// @details ZZ@> This function proceeds if the character has a special IDSZ ( in data.txt )

    ego_cap * pcap;

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;

    pcap = ego_chr::get_pcap( pself->target );
    if ( NULL != pcap )
    {
        returncode = ( pcap->idsz[IDSZ_SPECIAL] == ( IDSZ ) pstate->argument );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_PressTargetLatchButton( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // PressTargetLatchButton( tmpargument = "latch bits" )
    /// @details ZZ@> This function mimics joystick button presses for the target.
    /// For making items force their own usage and such

    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    ADD_BITS( pself_target->latch.trans.b, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Invisible( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfInvisible()
    /// @details ZZ@> This function proceeds if the character is invisible

    SCRIPT_FUNCTION_BEGIN();

    returncode = pchr->gfx_inst.alpha <= INVISIBLE;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ArmorIs( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfArmorIs( tmpargument = "skin" )
    /// @details ZZ@> This function proceeds if the character's skin type equals tmpargument

    int tTmp;

    SCRIPT_FUNCTION_BEGIN();

    tTmp = pchr->skin;
    returncode = ( tTmp == pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_get_TargetGrogTime( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // tmpargument = GetTargetGrogTime()
    /// @details ZZ@> This function sets tmpargument to the number of updates before the
    /// character is ungrogged, proceeding if the number is greater than 0

    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    pstate->argument = pself_target->grogtime;

    returncode = ( pstate->argument != 0 );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_get_TargetDazeTime( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // tmpargument = GetTargetDazeTime()
    /// @details ZZ@> This function sets tmpargument to the number of updates before the
    /// character is undazed, proceeding if the number is greater than 0

    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    pstate->argument = pself_target->dazetime;

    returncode = ( pstate->argument != 0 );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_DamageType( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetDamageType( tmpargument = "damage type" )
    /// @details ZZ@> This function lets a weapon change the type of damage it inflicts

    SCRIPT_FUNCTION_BEGIN();

    pchr->damagetargettype = pstate->argument % DAMAGE_COUNT;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_WaterLevel( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetWaterLevel( tmpargument = "level" )
    /// @details ZZ@> This function raises or lowers the water in the module

    int iTmp;
    float fTmp;

    SCRIPT_FUNCTION_BEGIN();

    fTmp = ( pstate->argument / 10.0f ) - water.douse_level;
    water.surface_level += fTmp;
    water.douse_level += fTmp;

    for ( iTmp = 0; iTmp < MAXWATERLAYER; iTmp++ )
    {
        water.layer[iTmp].z += fTmp;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_EnchantTarget( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // EnchantTarget()
    /// @details ZZ@> This function enchants the target with the enchantment given
    /// in enchant.txt. Make sure you use set_OwnerToTarget before doing this.

    ENC_REF iTmp;

    SCRIPT_FUNCTION_BEGIN();

    iTmp = spawn_one_enchant( pself->owner, pself->target, pself->index, ENC_REF( MAX_ENC ), PRO_REF( MAX_PROFILE ) );
    returncode = INGAME_ENC( iTmp );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_EnchantChild( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // EnchantChild()
    /// @details ZZ@> This function can be used with SpawnCharacter to enchant the
    /// newly spawned character with the enchantment
    /// given in enchant.txt. Make sure you use set_OwnerToTarget before doing this.

    ENC_REF iTmp;

    SCRIPT_FUNCTION_BEGIN();

    iTmp = spawn_one_enchant( pself->owner, pself->child, pself->index, ENC_REF( MAX_ENC ), PRO_REF( MAX_PROFILE ) );
    returncode = INGAME_ENC( iTmp );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TeleportTarget( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // TeleportTarget( tmpx = "x", tmpy = "y" )
    /// @details ZZ@> This function teleports the target to the X, Y location, failing if the
    /// location is off the map or blocked

    SCRIPT_FUNCTION_BEGIN();

    returncode = chr_teleport( pself->target, pstate->x, pstate->y, pstate->distance, pstate->turn );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GiveExperienceToTarget( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // GiveExperienceToTarget( tmpargument = "amount", tmpdistance = "type" )
    /// @details ZZ@> This function gives the target some experience, xptype from distance,
    /// amount from argument.

    SCRIPT_FUNCTION_BEGIN();

    give_experience( pself->target, pstate->argument, ( xp_type )pstate->distance, bfalse );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IncreaseAmmo( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IncreaseAmmo()
    /// @details ZZ@> This function increases the character's ammo by 1

    SCRIPT_FUNCTION_BEGIN();
    if ( pchr->ammo < pchr->ammo_max )
    {
        pchr->ammo++;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_UnkurseTarget( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // UnkurseTarget()
    /// @details ZZ@> This function unkurses the target

    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    pself_target->iskursed = bfalse;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GiveExperienceToTargetTeam( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // GiveExperienceToTargetTeam( tmpargument = "amount", tmpdistance = "type" )
    /// @details ZZ@> This function gives experience to everyone on the target's team

    SCRIPT_FUNCTION_BEGIN();

    give_team_experience( ego_chr::get_iteam( pself->target ), pstate->argument, ( xp_type )pstate->distance );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Unarmed( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfUnarmed()
    /// @details ZZ@> This function proceeds if the character is holding no items in hand.

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( !INGAME_CHR( pchr->holdingwhich[SLOT_LEFT] ) && !INGAME_CHR( pchr->holdingwhich[SLOT_RIGHT] ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_RestockTargetAmmoIDAll( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // RestockTargetAmmoIDAll( tmpargument = "idsz" )
    /// @details ZZ@> This function restocks the ammo of every item the character is holding,
    /// if the item matches the ID given ( parent or child type )

    CHR_REF ichr;
    int iTmp;
    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    iTmp = 0;  // Amount of ammo given

    ichr = pself_target->holdingwhich[SLOT_LEFT];
    iTmp += restock_ammo( ichr, pstate->argument );

    ichr = pself_target->holdingwhich[SLOT_RIGHT];
    iTmp += restock_ammo( ichr, pstate->argument );

    PACK_BEGIN_LOOP( ichr, pself_target->pack.next )
    {
        iTmp += restock_ammo( ichr, pstate->argument );
    }
    PACK_END_LOOP( ichr );

    pstate->argument = iTmp;
    returncode = ( iTmp != 0 );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_RestockTargetAmmoIDFirst( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // RestockTargetAmmoIDFirst( tmpargument = "idsz" )
    /// @details ZZ@> This function restocks the ammo of the first item the character is holding,
    /// if the item matches the ID given ( parent or child type )

    int     iTmp;
    CHR_REF ichr;
    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    iTmp = 0;  // Amount of ammo given

    if ( 0 == iTmp )
    {
        PACK_BEGIN_LOOP( ichr, pself_target->holdingwhich[SLOT_LEFT] )
        {
            iTmp += restock_ammo( ichr, pstate->argument );
            if ( 0 != iTmp ) break;
        }
        PACK_END_LOOP( ichr )
    }

    if ( 0 == iTmp )
    {
        PACK_BEGIN_LOOP( ichr, pself_target->holdingwhich[SLOT_RIGHT] )
        {
            iTmp += restock_ammo( ichr, pstate->argument );
            if ( 0 != iTmp ) break;
        }
        PACK_END_LOOP( ichr )
    }

    pstate->argument = iTmp;
    returncode = ( iTmp != 0 );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_FlashTarget( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // FlashTarget()
    /// @details ZZ@> This function makes the target flash

    SCRIPT_FUNCTION_BEGIN();

    flash_character( pself->target, 255 );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_RedShift( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetRedShift( tmpargument = "red darkening" )
    /// @details ZZ@> This function sets the character's red shift ( 0 - 3 ), higher values
    /// making the character less red and darker

    SCRIPT_FUNCTION_BEGIN();

    ego_chr::set_redshift( pchr, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_GreenShift( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetGreenShift( tmpargument = "green darkening" )
    /// @details ZZ@> This function sets the character's green shift ( 0 - 3 ), higher values
    /// making the character less red and darker

    SCRIPT_FUNCTION_BEGIN();

    ego_chr::set_grnshift( pchr, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_BlueShift( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetBlueShift( tmpargument = "blue darkening" )
    /// @details ZZ@> This function sets the character's blue shift ( 0 - 3 ), higher values
    /// making the character less red and darker

    SCRIPT_FUNCTION_BEGIN();

    ego_chr::set_grnshift( pchr, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_Light( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetLight( tmpargument = "lighness" )
    /// @details ZZ@> This function alters the character's transparency ( 0 - 254 )
    /// 255 = no transparency

    SCRIPT_FUNCTION_BEGIN();

    ego_chr::set_light( pchr, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_Alpha( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetAlpha( tmpargument = "alpha" )
    /// @details ZZ@> This function alters the character's transparency ( 0 - 255 )
    /// 255 = no transparency

    SCRIPT_FUNCTION_BEGIN();

    ego_chr::set_alpha( pchr, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_HitFromBehind( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfHitFromBehind()
    /// @details ZZ@> This function proceeds if the last attack to the character came from behind

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( pself->directionlast >= ATK_BEHIND - 8192 && pself->directionlast < ATK_BEHIND + 8192 )
        returncode = btrue;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_HitFromFront( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfHitFromFront()
    /// @details ZZ@> This function proceeds if the last attack to the character came
    /// from the front

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( pself->directionlast >= ATK_LEFT + 8192 || pself->directionlast < ATK_FRONT + 8192 )
        returncode = btrue;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_HitFromLeft( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfHitFromLeft()
    /// @details ZZ@> This function proceeds if the last attack to the character came
    /// from the left

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( pself->directionlast >= ATK_LEFT - 8192 && pself->directionlast < ATK_LEFT + 8192 )
        returncode = btrue;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_HitFromRight( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfHitFromRight()
    /// @details ZZ@> This function proceeds if the last attack to the character came
    /// from the right

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( pself->directionlast >= ATK_RIGHT - 8192 && pself->directionlast < ATK_RIGHT + 8192 )
        returncode = btrue;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsOnSameTeam( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfTargetIsOnSameTeam()
    /// @details ZZ@> This function proceeds if the target is on the character's team

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( ego_chr::get_iteam( pself->target ) == pchr->team )
        returncode = btrue;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_KillTarget( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // KillTarget()
    /// @details ZZ@> This function kills the target

    CHR_REF ichr;

    SCRIPT_FUNCTION_BEGIN();

    ichr = pself->index;

    // Weapons don't kill people, people kill people...
    if ( INGAME_CHR( pchr->attachedto ) && !ChrObjList.get_data_ref( pchr->attachedto ).ismount )
    {
        ichr = pchr->attachedto;
    }

    kill_character( pself->target, ichr, bfalse );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_UndoEnchant( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // UndoEnchant()
    /// @details ZZ@> This function removes the last enchantment spawned by the character,
    /// proceeding if an enchantment was removed

    SCRIPT_FUNCTION_BEGIN();

    if ( INGAME_ENC( pchr->undoenchant ) )
    {
        returncode = remove_enchant( pchr->undoenchant, NULL );
    }
    else
    {
        pchr->undoenchant = MAX_ENC;
        returncode = bfalse;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_get_WaterLevel( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // tmpargument = GetWaterLevel()
    /// @details ZZ@> This function sets tmpargument to the current douse level for the water * 10.
    /// A waterlevel in wawalight of 85 would set tmpargument to 850

    SCRIPT_FUNCTION_BEGIN();

    pstate->argument = water.douse_level * 10;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_CostTargetMana( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // CostTargetMana( tmpargument = "amount" )
    /// @details ZZ@> This function costs the target a specific amount of mana, proceeding
    /// if the target was able to pay the price.  The amounts are 8.8-bit fixed point

    SCRIPT_FUNCTION_BEGIN();

    returncode = cost_mana( pself->target, pstate->argument, pself->index );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetHasAnyID( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfTargetHasAnyID( tmpargument = "idsz" )
    /// @details ZZ@> This function proceeds if the target has any IDSZ that matches the given one

    SCRIPT_FUNCTION_BEGIN();

    returncode = ego_chr::has_idsz( pself->target, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_BumpSize( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetBumpSize( tmpargument = "size" )
    /// @details ZZ@> This function sets the how wide the character is

    SCRIPT_FUNCTION_BEGIN();

    ego_chr::set_width( pchr, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_NotDropped( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfNotDropped()
    /// @details ZZ@> This function proceeds if the character is kursed and another character
    /// was holding it and tried to drop it

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_NOTDROPPED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_YIsLessThanX( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfYIsLessThanX()
    /// @details ZZ@> This function proceeds if tmpy is less than tmpx

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( pstate->y < pstate->x );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_FlyHeight( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetFlyHeight( tmpargument = "height" )
    /// @details ZZ@> This function makes the character fly ( or fall to ground if 0 )

    SCRIPT_FUNCTION_BEGIN();

    ego_chr_data::set_fly_height( pchr, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Blocked( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfBlocked()
    /// @details ZZ@> This function proceeds if the character blocked the attack of another
    /// character this update

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_BLOCKED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsDefending( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfTargetIsDefending()
    /// @details ZZ@> This function proceeds if the target is holding up a shield or similar
    /// defense

    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = ACTION_IS_TYPE( pself_target->mad_inst.action.which, P );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsAttacking( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfTargetIsAttacking()
    /// @details ZZ@> This function proceeds if the target is doing an attack action

    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = chr_is_attacking( pself_target );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_StateIs0( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    SCRIPT_FUNCTION_BEGIN();

    returncode = ( 0 == pself->state );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_StateIs1( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    SCRIPT_FUNCTION_BEGIN();

    returncode = ( 1 == pself->state );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_StateIs2( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    SCRIPT_FUNCTION_BEGIN();

    returncode = ( 2 == pself->state );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_StateIs3( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    SCRIPT_FUNCTION_BEGIN();

    returncode = ( 3 == pself->state );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_StateIs4( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    SCRIPT_FUNCTION_BEGIN();

    returncode = ( 4 == pself->state );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_StateIs5( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    SCRIPT_FUNCTION_BEGIN();

    returncode = ( 5 == pself->state );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_StateIs6( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    SCRIPT_FUNCTION_BEGIN();

    returncode = ( 6 == pself->state );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_StateIs7( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    SCRIPT_FUNCTION_BEGIN();

    returncode = ( 7 == pself->state );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ContentIs( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfContentIs( tmpargument = "test" )
    /// @details ZZ@> This function proceeds if the content matches tmpargument

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( pstate->argument == pself->content );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TurnModeToWatchTarget( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetTurnModeToWatchTarget()
    /// @details ZZ@> This function makes the character face its target, no matter what
    /// direction it is moving in.  Undo this with set_TurnModeToVelocity

    SCRIPT_FUNCTION_BEGIN();

    pchr->turnmode = TURNMODE_WATCHTARGET;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_StateIsNot( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfStateIsNot( tmpargument = "test" )
    /// @details ZZ@> This function proceeds if the character's state does not equal tmpargument

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( pstate->argument != pself->state );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_XIsEqualToY( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // These functions proceed if tmpx and tmpy are the same

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( pstate->x == pstate->y );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DebugMessage( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // DebugMessage()
    /// @details ZZ@> This function spits out some useful numbers

    SCRIPT_FUNCTION_BEGIN();

    debug_printf( "aistate %d, aicontent %d, target %d", pself->state, pself->content, ( pself->target ).get_value() );
    debug_printf( "tmpx %d, tmpy %d", pstate->x, pstate->y );
    debug_printf( "tmpdistance %d, tmpturn %d", pstate->distance, pstate->turn );
    debug_printf( "tmpargument %d, selfturn %d", pstate->argument, pchr->ori.facing_z );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_BlackTarget( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // BlackTarget()
    /// @details ZZ@>  The opposite of FlashTarget, causing the target to turn black

    SCRIPT_FUNCTION_BEGIN();

    flash_character( pself->target, 0 );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SendMessageNear( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SendMessageNear( tmpargument = "message" )
    /// @details ZZ@> This function sends a message if the camera is in the nearby area

    int iTmp;

    SCRIPT_FUNCTION_BEGIN();

    iTmp = fvec2_dist_abs( pchr->pos_old.v, PCamera->track_pos.v );
    if ( iTmp < MSGDISTANCE )
    {
        returncode = _display_message( pself->index, pchr->profile_ref, pstate->argument, pstate );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_HitGround( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfHitGround()
    /// @details ZZ@> This function proceeds if a character hit the ground this update.
    /// Used to determine when to play the sound for a dropped item

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_HITGROUND );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_NameIsKnown( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfNameIsKnown()
    /// @details ZZ@> This function proceeds if the character's name is known

    SCRIPT_FUNCTION_BEGIN();

    returncode = pchr->nameknown;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_UsageIsKnown( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfUsageIsKnown()
    /// @details ZZ@> This function proceeds if the character's usage is known

    ego_cap * pcap;

    SCRIPT_FUNCTION_BEGIN();

    pcap = pro_get_pcap( pchr->profile_ref );

    returncode = bfalse;
    if ( NULL != pcap )
    {
        returncode = pcap->usageknown;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_HoldingItemID( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfHoldingItemID( tmpargument = "idsz" )
    /// @details ZZ@> This function proceeds if the character is holding a specified item
    /// in hand, setting tmpargument to the latch button to press to use it

    CHR_REF item;

    SCRIPT_FUNCTION_BEGIN();

    item = chr_holding_idsz( pself->index, pstate->argument );

    returncode = INGAME_CHR( item );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_HoldingRangedWeapon( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfHoldingRangedWeapon()
    /// @details ZZ@> This function passes if the character is holding a ranged weapon, returning
    /// the latch to press to use it.  This also checks ammo/ammoknown.

    CHR_REF ichr;

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    pstate->argument = 0;

    // Check right hand
    ichr = pchr->holdingwhich[SLOT_RIGHT];
    if ( INGAME_CHR( ichr ) )
    {
        ego_cap * pcap = ego_chr::get_pcap( ichr );

        if ( NULL != pcap && pcap->isranged && ( ChrObjList.get_data_ref( ichr ).ammo_max == 0 || ( ChrObjList.get_data_ref( ichr ).ammo != 0 && ChrObjList.get_data_ref( ichr ).ammoknown ) ) )
        {
            if ( pstate->argument == 0 || ( update_wld & 1 ) )
            {
                pstate->argument = LATCHBUTTON_RIGHT;
                returncode = btrue;
            }
        }
    }

    if ( !returncode )
    {
        // Check left hand
        ichr = pchr->holdingwhich[SLOT_LEFT];
        if ( INGAME_CHR( ichr ) )
        {
            ego_cap * pcap = ego_chr::get_pcap( ichr );

            if ( NULL != pcap && pcap->isranged && ( ChrObjList.get_data_ref( ichr ).ammo_max == 0 || ( ChrObjList.get_data_ref( ichr ).ammo != 0 && ChrObjList.get_data_ref( ichr ).ammoknown ) ) )
            {
                pstate->argument = LATCHBUTTON_LEFT;
                returncode = btrue;
            }
        }

    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_HoldingMeleeWeapon( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfHoldingMeleeWeapon()
    /// @details ZZ@> This function proceeds if the character is holding a specified item
    /// in hand, setting tmpargument to the latch button to press to use it

    CHR_REF ichr;

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    pstate->argument = 0;

    if ( !returncode )
    {
        // Check right hand
        ichr = pchr->holdingwhich[SLOT_RIGHT];
        if ( INGAME_CHR( ichr ) )
        {
            ego_cap * pcap = ego_chr::get_pcap( ichr );

            if ( NULL != pcap && !pcap->isranged && pcap->weaponaction != ACTION_PA )
            {
                if ( pstate->argument == 0 || ( update_wld & 1 ) )
                {
                    pstate->argument = LATCHBUTTON_RIGHT;
                    returncode = btrue;
                }
            }
        }
    }

    if ( !returncode )
    {
        // Check left hand
        ichr = pchr->holdingwhich[SLOT_LEFT];
        if ( INGAME_CHR( ichr ) )
        {
            ego_cap * pcap = ego_chr::get_pcap( ichr );

            if ( NULL != pcap && !pcap->isranged && pcap->weaponaction != ACTION_PA )
            {
                pstate->argument = LATCHBUTTON_LEFT;
                returncode = btrue;
            }
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_HoldingShield( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfHoldingShield()
    /// @details ZZ@> This function proceeds if the character is holding a specified item
    /// in hand, setting tmpargument to the latch button to press to use it. The button will need to be held down.

    CHR_REF ichr;

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    pstate->argument = 0;

    if ( !returncode )
    {
        // Check right hand
        ichr = pchr->holdingwhich[SLOT_RIGHT];
        if ( INGAME_CHR( ichr ) )
        {
            ego_cap * pcap = ego_chr::get_pcap( ichr );

            if ( NULL != pcap && pcap->weaponaction == ACTION_PA )
            {
                pstate->argument = LATCHBUTTON_RIGHT;
                returncode = btrue;
            }
        }
    }

    if ( !returncode )
    {
        // Check left hand
        ichr = pchr->holdingwhich[SLOT_LEFT];
        if ( INGAME_CHR( ichr ) )
        {
            ego_cap * pcap = ego_chr::get_pcap( ichr );

            if ( NULL != pcap && pcap->weaponaction == ACTION_PA )
            {
                pstate->argument = LATCHBUTTON_LEFT;
                returncode = btrue;
            }
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Kursed( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfKursed()
    /// @details ZZ@> This function proceeds if the character is kursed

    SCRIPT_FUNCTION_BEGIN();

    returncode = pchr->iskursed;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsKursed( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfTargetIsKursed()
    /// @details ZZ@> This function proceeds if the target is kursed

    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = pself_target->iskursed;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsDressedUp( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfTargetIsDressedUp()
    /// @details ZZ@> This function proceeds if the target is dressed in fancy clothes

    ego_cap * pcap;

    SCRIPT_FUNCTION_BEGIN();

    pcap = pro_get_pcap( pchr->profile_ref );

    returncode = bfalse;
    if ( NULL != pcap )
    {
        returncode = HAS_SOME_BITS( pcap->skindressy, 1 << pchr->skin );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_OverWater( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfOverWater()
    /// @details ZZ@> This function proceeds if the character is on a water tile

    SCRIPT_FUNCTION_BEGIN();

    returncode = chr_is_over_water( pchr );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Thrown( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfThrown()
    /// @details ZZ@> This function proceeds if the character was thrown this update.

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_THROWN );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_MakeNameKnown( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // MakeNameKnown()
    /// @details ZZ@> This function makes the name of the character known, for identifying
    /// weapons and spells and such

    SCRIPT_FUNCTION_BEGIN();

    pchr->nameknown = btrue;
    //           pchr->icon = btrue;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_MakeUsageKnown( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // MakeUsageKnown()
    /// @details ZZ@> This function makes the usage known for this type of object
    /// For XP gains from using an unknown potion or such

    ego_cap * pcap;

    SCRIPT_FUNCTION_BEGIN();

    pcap = pro_get_pcap( pchr->profile_ref );

    returncode = bfalse;
    if ( NULL != pcap )
    {
        pcap->usageknown = btrue;
        returncode       = btrue;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_StopTargetMovement( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // StopTargetMovement()
    /// @details ZZ@> This function makes the target stop moving temporarily
    /// Sets the target's x and y velocities to 0, and
    /// sets the z velocity to 0 if the character is moving upwards.
    /// This is a special function for the IronBall object

    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    pself_target->vel.x = 0;
    pself_target->vel.y = 0;
    if ( pself_target->vel.z > 0 ) pself_target->vel.z = gravity;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_XY( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetXY( tmpargument = "index", tmpx = "x", tmpy = "y" )
    /// @details ZZ@> This function sets one of the 8 permanent storage variable slots
    /// ( each of which holds an x,y pair )

    SCRIPT_FUNCTION_BEGIN();

    pself->x[pstate->argument&STOR_AND] = pstate->x;
    pself->y[pstate->argument&STOR_AND] = pstate->y;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_get_XY( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // tmpx,tmpy = GetXY( tmpargument = "index" )
    /// @details ZZ@> This function reads one of the 8 permanent storage variable slots,
    /// setting tmpx and tmpy accordingly

    SCRIPT_FUNCTION_BEGIN();

    pstate->x = pself->x[pstate->argument&STOR_AND];
    pstate->y = pself->y[pstate->argument&STOR_AND];

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AddXY( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // AddXY( tmpargument = "index", tmpx = "x", tmpy = "y" )
    /// @details ZZ@> This function alters the contents of one of the 8 permanent storage
    /// slots

    SCRIPT_FUNCTION_BEGIN();

    pself->x[pstate->argument&STOR_AND] += pstate->x;
    pself->y[pstate->argument&STOR_AND] += pstate->y;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_MakeAmmoKnown( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // MakeAmmoKnown()
    /// @details ZZ@> This function makes the character's ammo known ( for items )

    SCRIPT_FUNCTION_BEGIN();

    pchr->ammoknown = btrue;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnAttachedParticle( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SpawnAttachedParticle( tmpargument = "particle", tmpdistance = "vertex" )
    /// @details ZZ@> This function spawns a particle attached to the character

    CHR_REF ichr, iholder;
    PRT_REF iprt;

    SCRIPT_FUNCTION_BEGIN();

    ichr    = pself->index;
    iholder = ego_chr::get_lowest_attachment( ichr, btrue );
    if ( INGAME_CHR( iholder ) )
    {
        ichr = iholder;
    }

    iprt = spawn_one_particle( pchr->pos, pchr->ori.facing_z, pchr->profile_ref, pstate->argument, pself->index, pstate->distance, pchr->team, ichr, PRT_REF( MAX_PRT ), 0, CHR_REF( MAX_CHR ) );

    returncode = VALID_PRT( iprt );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnExactParticle( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SpawnExactParticle( tmpargument = "particle", tmpx = "x", tmpy = "y", tmpdistance = "z" )
    /// @details ZZ@> This function spawns a particle at a specific x, y, z position

    PRT_REF iprt;
    CHR_REF ichr;

    SCRIPT_FUNCTION_BEGIN();

    ichr = pself->index;
    if ( INGAME_CHR( pchr->attachedto ) )
    {
        ichr = pchr->attachedto;
    }

    {
        fvec3_t vtmp = VECT3( pstate->x, pstate->y, pstate->distance );
        iprt = spawn_one_particle( vtmp, pchr->ori.facing_z, pchr->profile_ref, pstate->argument, CHR_REF( MAX_CHR ), 0, pchr->team, ichr, PRT_REF( MAX_PRT ), 0, CHR_REF( MAX_CHR ) );
    }

    returncode = VALID_PRT( iprt );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AccelerateTarget( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // AccelerateTarget( tmpx = "acc x", tmpy = "acc y" )
    /// @details ZZ@> This function changes the x and y speeds of the target

    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    pself_target->vel.x += pstate->x;
    pself_target->vel.y += pstate->y;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_distanceIsMoreThanTurn( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfdistanceIsMoreThanTurn()
    /// @details ZZ@> This function proceeds tmpdistance is greater than tmpturn

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( pstate->distance > pstate->turn );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Crushed( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfCrushed()
    /// @details ZZ@> This function proceeds if the character was crushed in a passage this
    /// update.

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_CRUSHED );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_MakeCrushValid( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // MakeCrushValid()
    /// @details ZZ@> This function makes a character able to be crushed by closing doors
    /// and such

    SCRIPT_FUNCTION_BEGIN();

    pchr->canbecrushed = btrue;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToLowestTarget( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetTargetToLowestTarget()
    /// @details ZZ@> This function sets the target to the absolute bottom character.
    /// The holder of the target, or the holder of the holder of the target, or
    /// the holder of the holder of ther holder of the target, etc.   This function never fails

    CHR_REF itarget;

    SCRIPT_FUNCTION_BEGIN();

    itarget = ego_chr::get_lowest_attachment( pself->target, bfalse );

    if ( INGAME_CHR( itarget ) )
    {
        SET_TARGET_0( itarget );
    }
    else
    {
        returncode = bfalse;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_NotPutAway( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfNotPutAway()
    /// @details ZZ@> This function proceeds if the character couldn't be put into another
    /// character's pockets for some reason.
    /// It might be kursed or too big or something

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_NOTPUTAWAY );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TakenOut( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfTakenOut()
    /// @details ZZ@> This function proceeds if the character is equiped in another's inventory,
    /// and the holder tried to unequip it ( take it out of pack ), but the
    /// item was kursed and didn't cooperate

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_TAKENOUT );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AmmoOut( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfAmmoOut()
    /// @details ZZ@> This function proceeds if the character itself has no ammo left.
    /// This is for crossbows and such, not archers.

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( pchr->ammo == 0 );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_PlaySoundLooped( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // PlaySoundLooped( tmpargument = "sound", tmpdistance = "frequency" )

    /// @details ZZ@> This function starts playing a continuous sound

    Mix_Chunk * new_chunk;

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;

    new_chunk = ego_chr::get_chunk_ptr( pchr, pstate->argument );

    if ( NULL == new_chunk )
    {
        looped_stop_object_sounds( pself->index );       // Stop existing sound loop (if any)
    }
    else
    {
        Mix_Chunk * playing_chunk = NULL;

        // check whatever might be playing on the channel now
        if ( INVALID_SOUND_CHANNEL != pchr->loopedsound_channel )
        {
            playing_chunk = Mix_GetChunk( pchr->loopedsound_channel );
        }

        if ( playing_chunk != new_chunk )
        {
            pchr->loopedsound_channel = sound_play_chunk_looped( pchr->pos_old, new_chunk, -1, pself->index );
        }
    }

    returncode = ( INVALID_SOUND_CHANNEL != pchr->loopedsound_channel );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_StopSound( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // StopSound( tmpargument = "sound" )
    /// @details ZZ@> This function stops the playing of a continuous sound!

    SCRIPT_FUNCTION_BEGIN();

    looped_stop_object_sounds( pself->index );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_HealSelf( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // HealSelf()
    /// @details ZZ@> This function gives life back to the character.
    /// Values given as 8.8-bit fixed point
    /// This does NOT remove [HEAL] enchants ( poisons )
    /// This does not set the ALERTIF_HEALED alert

    SCRIPT_FUNCTION_BEGIN();

    heal_character( pself->index, pself->index, pstate->argument, btrue );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Equip( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // Equip()
    /// @details ZZ@> This function flags the character as being equipped.
    /// This is used by equipment items when they are placed in the inventory

    SCRIPT_FUNCTION_BEGIN();

    pchr->isequipped = btrue;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetHasItemIDEquipped( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfTargetHasItemIDEquipped( tmpargument = "item idsz" )
    /// @details ZZ@> This function proceeds if the target already wearing a matching item

    CHR_REF item;

    SCRIPT_FUNCTION_BEGIN();

    item = chr_has_inventory_idsz( pself->target, pstate->argument, btrue, NULL );

    returncode = INGAME_CHR( item );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_OwnerToTarget( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetOwnerToTarget()
    /// @details ZZ@> This function must be called before enchanting anything.
    /// The owner is the character that pays the sustain costs and such for the enchantment

    SCRIPT_FUNCTION_BEGIN();

    pself->owner = pself->target;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToOwner( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetTargetToOwner()
    /// @details ZZ@> This function sets the target to whoever was previously declared as the
    /// owner.

    SCRIPT_FUNCTION_BEGIN();

    if ( INGAME_CHR( pself->owner ) )
    {
        SET_TARGET_0( pself->owner );
    }
    else
    {
        returncode = bfalse;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_Frame( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetFrame( tmpargument = "frame" )
    /// @details ZZ@> This function sets the current .MD2 frame for the character.  Values are * 4

    int iTmp;
    Uint16 sTmp = 0;

    SCRIPT_FUNCTION_BEGIN();

    sTmp = pstate->argument & 3;
    iTmp = pstate->argument >> 2;
    ego_chr::set_frame( pchr, iTmp, sTmp );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_BreakPassage( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // BreakPassage( tmpargument = "passage", tmpturn = "tile type", tmpdistance = "number of frames", tmpx = "broken tile", tmpy = "tile fx bits" )

    /// @details ZZ@> This function makes the tiles fall away ( turns into damage terrain )
    /// This function causes the tiles of a passage to increment if stepped on.
    /// tmpx and tmpy are both set to the location of whoever broke the tile if
    /// the function passed.

    SCRIPT_FUNCTION_BEGIN();

    returncode = _break_passage( pstate->y, pstate->x, pstate->distance, pstate->turn, PASS_REF( pstate->argument ), &( pstate->x ), &( pstate->y ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_ReloadTime( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetReloadTime( tmpargument = "time" )
    /// @details ZZ@> This function stops a character from being used for a while.  Used
    /// by weapons to slow down their attack rate.  50 clicks per second.

    SCRIPT_FUNCTION_BEGIN();

    if ( pstate->argument > 0 ) pchr->reloadtime = pstate->argument;
    else pchr->reloadtime = 0;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToWideBlahID( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetTargetToWideBlahID( tmpargument = "idsz", tmpdistance = "blah bits" )
    /// @details ZZ@> This function sets the target to a character that matches the description,
    /// and who is located in the general vicinity of the character

    CHR_REF ichr;

    SCRIPT_FUNCTION_BEGIN();

    // Try to find one
    ichr = chr_find_target( pchr, WIDE, pstate->argument, pstate->distance );

    if ( INGAME_CHR( ichr ) )
    {
        SET_TARGET_0( ichr );
        returncode = btrue;
    }
    else
    {
        returncode = bfalse;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_PoofTarget( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // PoofTarget()
    /// @details ZZ@> This function removes the target from the game, failing if the
    /// target is a player

    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = bfalse;
    if ( INVALID_PLA( pself_target->is_which_player ) )                // Do not poof players
    {
        returncode = btrue;
        if ( pself->target == pself->index )
        {
            // Poof self later
            pself->poof_time = update_wld + 1;
        }
        else
        {
            // Poof others now
            pself_target->ai.poof_time = update_wld;

            SET_TARGET( pself->index, pself_target );
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ChildDoActionOverride( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // ChildDoActionOverride( tmpargument = action )

    /// @details ZZ@> This function lets a character set the action of the last character
    /// it spawned.  It also sets the current frame to the first frame of the
    /// action ( no interpolation from last frame ). If the cation is not valid for the model,
    /// the function will fail

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( INGAME_CHR( pself->child ) )
    {
        int action;

        ego_chr * pchild = ChrObjList.get_data_ptr( pself->child );

        action = mad_get_action( pchild->mad_inst.imad, pstate->argument );

        if ( rv_success == ego_chr::start_anim( pchild, action, bfalse, btrue ) )
        {
            returncode = btrue;
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnPoof( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SpawnPoof
    /// @details ZZ@> This function makes a lovely little poof at the character's location.
    /// The poof form and particle types are set in data.txt

    SCRIPT_FUNCTION_BEGIN();

    spawn_poof( pself->index, pchr->profile_ref );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_SpeedPercent( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetSpeedPercent( tmpargument = "percent" )
    /// @details ZZ@> This function acts like Run or Walk, except it allows the explicit
    /// setting of the speed

    float fvalue;

    SCRIPT_FUNCTION_BEGIN();

    reset_character_accel( pself->index );

    fvalue = pstate->argument / 100.0f;
    fvalue = SDL_max( 0.0f, fvalue );

    pchr->maxaccel = pchr->maxaccel_reset * fvalue;

    if ( pchr->maxaccel < 0.33f )
    {
        // only sneak
        pchr->movement_bits = CHR_MOVEMENT_BITS_SNEAK | CHR_MOVEMENT_BITS_STOP;
    }
    else
    {
        // everything but sneak
        pchr->movement_bits = ego_uint( ~CHR_MOVEMENT_BITS_SNEAK );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_ChildState( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetChildState( tmpargument = "state" )
    /// @details ZZ@> This function lets a character set the state of the last character it
    /// spawned

    SCRIPT_FUNCTION_BEGIN();

    ChrObjList.get_data_ref( pself->child ).ai.state = pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnAttachedSizedParticle( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SpawnAttachedSizedParticle( tmpargument = "particle", tmpdistance = "vertex", tmpturn = "size" )
    /// @details ZZ@> This function spawns a particle of the specific size attached to the
    /// character. For spell charging effects

    PRT_REF iprt;
    CHR_REF ichr;

    SCRIPT_FUNCTION_BEGIN();

    ichr = pself->index;
    if ( INGAME_CHR( pchr->attachedto ) )
    {
        ichr = pchr->attachedto;
    }

    iprt = spawn_one_particle( pchr->pos, pchr->ori.facing_z, pchr->profile_ref, pstate->argument, pself->index, pstate->distance, pchr->team, ichr, PRT_REF( MAX_PRT ), 0, CHR_REF( MAX_CHR ) );

    ego_prt * pprt = PrtObjList.get_allocated_data_ptr( iprt );
    returncode = ( NULL != pprt );

    if ( returncode )
    {
        returncode = ego_prt::set_size( pprt, pstate->turn );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ChangeArmor( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // ChangeArmor( tmpargument = "time" )
    /// @details ZZ@> This function changes the character's armor.
    /// Sets tmpargument as the old type and tmpx as the new type

    int iTmp;

    SCRIPT_FUNCTION_BEGIN();

    pstate->x = pstate->argument;
    iTmp = pchr->skin;
    pstate->x = ego_chr::change_armor( pself->index, pstate->argument );
    pstate->argument = iTmp;  // The character's old armor

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ShowTimer( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // ShowTimer( tmpargument = "time" )
    /// @details ZZ@> This function sets the value displayed by the module timer.
    /// For races and such.  50 clicks per second

    SCRIPT_FUNCTION_BEGIN();

    timeron = btrue;
    timervalue = pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_FacingTarget( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfFacingTarget()
    /// @details ZZ@> This function proceeds if the character is more or less facing its
    /// target

    FACING_T sTmp = 0;
    ego_chr *  pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    sTmp = vec_to_facing( pself_target->pos.x - pchr->pos.x , pself_target->pos.y - pchr->pos.y );
    sTmp -= pchr->ori.facing_z;
    returncode = ( sTmp > 55535 || sTmp < 10000 );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_PlaySoundVolume( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // PlaySoundVolume( argument = "sound", distance = "volume" )
    /// @details ZZ@> This function sets the volume of a sound and plays it

    SCRIPT_FUNCTION_BEGIN();

    if ( pstate->distance > 0 )
    {
        if ( VALID_SND( pstate->argument ) )
        {
            int channel;
            channel = sound_play_chunk( pchr->pos_old, ego_chr::get_chunk_ptr( pchr, pstate->argument ) );

            if ( channel != INVALID_SOUND_CHANNEL )
            {
                Mix_Volume( channel, ( 128*pstate->distance ) / 100 );
            }
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnAttachedFacedParticle( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SpawnAttachedFacedParticle(  tmpargument = "particle", tmpdistance = "vertex", tmpturn = "turn" )

    /// @details ZZ@> This function spawns a particle attached to the character, facing the
    /// same direction given by tmpturn

    PRT_REF iprt;
    CHR_REF ichr;

    SCRIPT_FUNCTION_BEGIN();

    ichr = pself->index;
    if ( INGAME_CHR( pchr->attachedto ) )
    {
        ichr = pchr->attachedto;
    }

    iprt = spawn_one_particle( pchr->pos, CLIP_TO_16BITS( pstate->turn ), pchr->profile_ref, pstate->argument, pself->index, pstate->distance, pchr->team, ichr, PRT_REF( MAX_PRT ), 0, CHR_REF( MAX_CHR ) );

    returncode = VALID_PRT( iprt );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_StateIsOdd( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfStateIsOdd()
    /// @details ZZ@> This function proceeds if the character's state is 1, 3, 5, 7, etc.

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( pself->state & 1 );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToDistantEnemy( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetTargetToDistantEnemy( tmpdistance = "distance" )
    /// @details ZZ@> This function finds a character within a certain distance of the
    /// character, failing if there are none

    CHR_REF ichr;

    SCRIPT_FUNCTION_BEGIN();

    ichr = chr_find_target( pchr, pstate->distance, IDSZ_NONE, TARGET_ENEMIES );

    if ( INGAME_CHR( ichr ) )
    {
        SET_TARGET_0( ichr );
    }
    else
    {
        returncode = bfalse;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Teleport( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // Teleport( tmpx = "x", tmpy = "y" )
    /// @details ZZ@> This function teleports the character to a new location, failing if
    /// the location is blocked or off the map

    SCRIPT_FUNCTION_BEGIN();

    returncode = chr_teleport( pself->index, pstate->x, pstate->y, pchr->pos.z, pchr->ori.facing_z );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GiveStrengthToTarget( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // GiveStrengthToTarget()
    // Permanently boost the target's strength

    int iTmp;
    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    if ( pself_target->alive )
    {
        iTmp = pstate->argument;
        getadd( 0, pself_target->strength, PERFECTSTAT, &iTmp );
        pself_target->strength += iTmp;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GiveWisdomToTarget( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // GiveWisdomToTarget()
    // Permanently boost the target's wisdom

    int iTmp;
    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    if ( pself_target->alive )
    {
        iTmp = pstate->argument;
        getadd( 0, pself_target->wisdom, PERFECTSTAT, &iTmp );
        pself_target->wisdom += iTmp;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GiveIntelligenceToTarget( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // GiveIntelligenceToTarget()
    // Permanently boost the target's intelligence

    int iTmp;
    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    if ( pself_target->alive )
    {
        iTmp = pstate->argument;
        getadd( 0, pself_target->intelligence, PERFECTSTAT, &iTmp );
        pself_target->intelligence += iTmp;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GiveDexterityToTarget( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // GiveDexterityToTarget()
    // Permanently boost the target's dexterity

    int iTmp;
    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    if ( pself_target->alive )
    {
        iTmp = pstate->argument;
        getadd( 0, pself_target->dexterity, PERFECTSTAT, &iTmp );
        pself_target->dexterity += iTmp;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GiveLifeToTarget( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // GiveLifeToTarget()
    /// @details ZZ@> Permanently boost the target's life

    int iTmp;
    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    if ( pself_target->alive )
    {
        iTmp = pstate->argument;
        getadd( LOWSTAT, pself_target->life_max, PERFECTBIG, &iTmp );
        pself_target->life_max += iTmp;
        if ( iTmp < 0 )
        {
            getadd( 1, pself_target->life, PERFECTBIG, &iTmp );
        }

        pself_target->life += iTmp;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GiveManaToTarget( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // GiveManaToTarget()
    /// @details ZZ@> Permanently boost the target's mana

    int iTmp;
    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    if ( pself_target->alive )
    {
        iTmp = pstate->argument;
        getadd( 0, pself_target->mana_max, PERFECTBIG, &iTmp );
        pself_target->mana_max += iTmp;
        if ( iTmp < 0 )
        {
            getadd( 0, pself_target->mana, PERFECTBIG, &iTmp );
        }

        pself_target->mana += iTmp;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ShowMap( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // ShowMap()
    /// @details ZZ@> This function shows the module's map.
    /// Fails if map already visible

    SCRIPT_FUNCTION_BEGIN();
    if ( mapon )  returncode = bfalse;

    mapon = mapvalid;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ShowYouAreHere( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // ShowYouAreHere()
    /// @details ZZ@> This function shows the blinking white blip on the map that represents the
    /// camera location

    SCRIPT_FUNCTION_BEGIN();

    youarehereon = mapvalid;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ShowBlipXY( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // ShowBlipXY( tmpx = "x", tmpy = "y", tmpargument = "color" )

    /// @details ZZ@> This function draws a blip on the map, and must be done each update
    SCRIPT_FUNCTION_BEGIN();

    // Add a blip
    if ( blip_count < MAXBLIP )
    {
        if ( pstate->x > 0 && pstate->x < PMesh->gmem.edge_x && pstate->y > 0 && pstate->y < PMesh->gmem.edge_y )
        {
            if ( pstate->argument < COLOR_MAX && pstate->argument >= 0 )
            {
                blip_x[blip_count] = pstate->x;
                blip_y[blip_count] = pstate->y;
                blip_c[blip_count] = pstate->argument;
                blip_count++;
            }
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_HealTarget( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // HealTarget( tmpargument = "amount" )
    /// @details ZZ@> This function gives some life back to the target.
    /// Values are 8.8-bit fixed point. Any enchantments that are removed by [HEAL], like poison, go away

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( heal_character( pself->target, pself->index, pstate->argument, bfalse ) )
    {
        returncode = btrue;
        remove_all_enchants_with_idsz( pself->target, MAKE_IDSZ( 'H', 'E', 'A', 'L' ) );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_PumpTarget( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // PumpTarget( tmpargument = "amount" )
    /// @details ZZ@> This function gives some mana back to the target.
    /// Values are 8.8-bit fixed point

    int iTmp;
    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    if ( pself_target->alive )
    {
        iTmp = pstate->argument;
        getadd( 0, pself_target->mana, pself_target->mana_max, &iTmp );
        pself_target->mana += iTmp;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_CostAmmo( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // CostAmmo()
    /// @details ZZ@> This function costs the character 1 point of ammo

    SCRIPT_FUNCTION_BEGIN();
    if ( pchr->ammo > 0 )
    {
        pchr->ammo--;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_MakeSimilarNamesKnown( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // MakeSimilarNamesKnown()
    /// @details ZZ@> This function makes the names of similar objects known.
    /// Checks all 6 IDSZ types to make sure they match.

    int tTmp;
    Uint16 sTmp = 0;
    ego_cap * pcap_chr;

    SCRIPT_FUNCTION_BEGIN();

    pcap_chr = pro_get_pcap( pchr->profile_ref );
    if ( NULL == pcap_chr ) return bfalse;

    CHR_BEGIN_LOOP_PROCESSING( cnt, pchr_test )
    {
        ego_cap * pcap_test;

        pcap_test = ego_chr::get_pcap( cnt );
        if ( NULL == pcap_test ) continue;

        sTmp = btrue;
        for ( tTmp = 0; tTmp < IDSZ_COUNT; tTmp++ )
        {
            if ( pcap_chr->idsz[tTmp] != pcap_test->idsz[tTmp] )
            {
                sTmp = bfalse;
            }
        }

        if ( sTmp )
        {
            pchr_test->nameknown = btrue;
        }
    }
    CHR_END_LOOP();

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnAttachedHolderParticle( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SpawnAttachedHolderParticle( tmpargument = "particle", tmpdistance = "vertex" )

    /// @details ZZ@> This function spawns a particle attached to the character's holder, or to the character if no holder

    PRT_REF iprt;
    CHR_REF ichr;

    SCRIPT_FUNCTION_BEGIN();

    ichr = pself->index;
    if ( INGAME_CHR( pchr->attachedto ) )
    {
        ichr = pchr->attachedto;
    }

    iprt = spawn_one_particle( pchr->pos, pchr->ori.facing_z, pchr->profile_ref, pstate->argument, ichr, pstate->distance, pchr->team, ichr, PRT_REF( MAX_PRT ), 0, CHR_REF( MAX_CHR ) );

    returncode = VALID_PRT( iprt );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetReloadTime( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetTargetReloadTime( tmpargument = "time" )

    /// @details ZZ@> This function sets the target's reload time
    /// This function stops the target from attacking for a while.

    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    if ( pstate->argument > 0 )
    {
        pself_target->reloadtime = pstate->argument;
    }
    else
    {
        pself_target->reloadtime = 0;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_FogLevel( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetFogLevel( tmpargument = "level" )
    /// @details ZZ@> This function sets the level of the module's fog.
    /// Values are * 10
    /// !!BAD!! DOESN'T WORK !!BAD!!

    float fTmp;

    SCRIPT_FUNCTION_BEGIN();

    fTmp = ( pstate->argument / 10.0f ) - fog.top;
    fog.top += fTmp;
    fog.distance += fTmp;
    fog.on = cfg.fog_allowed;
    if ( fog.distance < 1.0f )  fog.on = bfalse;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_get_FogLevel( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // tmpargument = GetFogLevel()
    /// @details ZZ@> This function sets tmpargument to the level of the module's fog.
    /// Values are * 10

    SCRIPT_FUNCTION_BEGIN();

    pstate->argument = fog.top * 10;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_FogTAD( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    /// @details ZZ@> This function sets the color of the module's fog.
    /// TAD stands for <turn, argument, distance> == <red, green, blue>.
    /// Makes sense, huh?
    /// !!BAD!! DOESN'T WORK !!BAD!!

    SCRIPT_FUNCTION_BEGIN();

    fog.red = pstate->turn;
    fog.grn = pstate->argument;
    fog.blu = pstate->distance;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_FogBottomLevel( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetFogBottomLevel( tmpargument = "level" )

    /// @details ZZ@> This function sets the level of the module's fog.
    /// Values are * 10

    float fTmp;

    SCRIPT_FUNCTION_BEGIN();

    fTmp = ( pstate->argument / 10.0f ) - fog.bottom;
    fog.bottom += fTmp;
    fog.distance -= fTmp;
    fog.on = cfg.fog_allowed;
    if ( fog.distance < 1.0f )  fog.on = bfalse;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_get_FogBottomLevel( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // tmpargument = GetFogBottomLevel()

    /// @details ZZ@> This function sets tmpargument to the level of the module's fog.
    /// Values are * 10

    SCRIPT_FUNCTION_BEGIN();

    pstate->argument = fog.bottom * 10;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_CorrectActionForHand( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // CorrectActionForHand( tmpargument = "action" )
    /// @details ZZ@> This function changes tmpargument according to which hand the character
    /// is held in It turns ZA into ZA, ZB, ZC, or ZD.
    /// USAGE:  wizards casting spells

    SCRIPT_FUNCTION_BEGIN();
    if ( INGAME_CHR( pchr->attachedto ) )
    {
        if ( pchr->inwhich_slot == SLOT_LEFT )
        {
            // A or B
            pstate->argument = generate_randmask( pstate->argument, 1 );
        }
        else
        {
            // C or D
            pstate->argument = generate_randmask( pstate->argument + 2, 1 );
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsMounted( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfTargetIsMounted()
    /// @details ZZ@> This function proceeds if the target is riding a mount

    CHR_REF ichr;
    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = bfalse;

    ichr = pself_target->attachedto;
    if ( INGAME_CHR( ichr ) )
    {
        returncode = ChrObjList.get_data_ref( ichr ).ismount;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SparkleIcon( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SparkleIcon( tmpargument = "color" )
    /// @details ZZ@> This function starts little sparklies going around the character's icon

    SCRIPT_FUNCTION_BEGIN();
    if ( pstate->argument < COLOR_MAX )
    {
        if ( pstate->argument < -1 || pstate->argument >= COLOR_MAX )
        {
            pchr->sparkle = NOSPARKLE;
        }
        else
        {
            pchr->sparkle = pstate->argument;
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_UnsparkleIcon( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // UnsparkleIcon()
    /// @details ZZ@> This function stops little sparklies going around the character's icon

    SCRIPT_FUNCTION_BEGIN();

    pchr->sparkle = NOSPARKLE;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_get_TileXY( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // tmpargument = GetTileXY( tmpx = "x", tmpy = "y" )
    /// @details ZZ@> This function sets tmpargument to the tile type at the specified
    /// coordinates

    Uint32 iTmp;

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    iTmp = ego_mpd::get_tile( PMesh, pstate->x, pstate->y );
    if ( ego_mpd::grid_is_valid( PMesh, iTmp ) )
    {
        returncode = btrue;
        pstate->argument = CLIP_TO_08BITS( PMesh->tmem.tile_list[iTmp].img );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TileXY( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // scr_set_TileXY( tmpargument = "tile type", tmpx = "x", tmpy = "y" )
    /// @details ZZ@> This function changes the tile type at the specified coordinates

    Uint32 iTmp;

    SCRIPT_FUNCTION_BEGIN();

    iTmp       = ego_mpd::get_tile( PMesh, pstate->x, pstate->y );
    returncode = ego_mpd::set_texture( PMesh, iTmp, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_ShadowSize( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetShadowSize( tmpargument = "size" )
    /// @details ZZ@> This function makes the character's shadow bigger or smaller

    SCRIPT_FUNCTION_BEGIN();

    pchr->shadow_size     = pstate->argument * pchr->fat;
    pchr->shadow_size_stt = pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_OrderTarget( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // OrderTarget( tmpargument = "order" )
    /// @details ZZ@> This function issues an order to the given target
    /// Be careful in using this, always checking IDSZ first

    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    if ( !INGAME_CHR( pself->target ) )
    {
        returncode = bfalse;
    }
    else
    {
        returncode = ego_ai_state::add_order( &( pself_target->ai ), pstate->argument, 0 );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToWhoeverIsInPassage( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetTargetToWhoeverIsInPassage()
    /// @details ZZ@> This function sets the target to whoever is blocking the given passage
    /// This function lets passage rectangles be used as event triggers

    CHR_REF ichr;

    SCRIPT_FUNCTION_BEGIN();

    ichr = PassageStack_who_is_blocking( PASS_REF( pstate->argument ), pself->index, IDSZ_NONE, TARGET_SELF | TARGET_FRIENDS | TARGET_ENEMIES, IDSZ_NONE );

    if ( INGAME_CHR( ichr ) )
    {
        SET_TARGET_0( ichr );
    }
    else
    {
        returncode = bfalse;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_CharacterWasABook( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfCharacterWasABook()
    /// @details ZZ@> This function proceeds if the base model is the same as the current
    /// model or if the base model is SPELLBOOK
    /// USAGE: USED BY THE MORPH SPELL. Not much use elsewhere

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( pchr->basemodel_ref == SPELLBOOK ||
                   pchr->basemodel_ref == pchr->profile_ref );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_EnchantBoostValues( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetEnchantBoostValues( tmpargument = "owner mana regen", tmpdistance = "owner life regen", tmpx = "target mana regen", tmpy = "target life regen" )
    /// @details ZZ@> This function sets the mana and life drains for the last enchantment
    /// spawned by this character.
    /// Values are 8.8-bit fixed point

    ENC_REF iTmp;

    SCRIPT_FUNCTION_BEGIN();

    iTmp = pchr->undoenchant;

    returncode = bfalse;
    if ( INGAME_ENC( iTmp ) )
    {
        EncObjList.get_data_ref( iTmp ).owner_mana = pstate->argument;
        EncObjList.get_data_ref( iTmp ).owner_life = pstate->distance;
        EncObjList.get_data_ref( iTmp ).target_mana = pstate->x;
        EncObjList.get_data_ref( iTmp ).target_life = pstate->y;

        returncode = btrue;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnCharacterXYZ( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SpawnCharacterXYZ( tmpx = "x", tmpy = "y", tmpdistance = "z", tmpturn = "turn" )
    /// @details ZZ@> This function spawns a character of the same type at a specific location, failing if x,y,z is invalid

    fvec3_t   pos;
    CHR_REF ichr;

    SCRIPT_FUNCTION_BEGIN();

    pos.x = pstate->x;
    pos.y = pstate->y;
    pos.z = pstate->distance;

    ichr = spawn_one_character( pos, pchr->profile_ref, pchr->team, 0, CLIP_TO_16BITS( pstate->turn ), NULL, CHR_REF( MAX_CHR ) );

    if ( !INGAME_CHR( ichr ) )
    {
        if ( ichr > PMod->importamount * MAXIMPORTPERPLAYER )
        {
            log_warning( "Object %s failed to spawn a copy of itself\n", ego_chr::get_obj_ref( *pchr ).base_name );
        }
    }
    else
    {
        ego_chr * pchild = ChrObjList.get_data_ptr( ichr );

        // was the child spawned in a "safe" spot?
        if ( !ego_chr::get_safe( pchild, NULL ) )
        {
            ego_obj_chr::request_terminate( ichr );
            ichr = CHR_REF( MAX_CHR );
        }
        else
        {
            pself->child = ichr;

            pchild->iskursed   = pchr->iskursed;  /// @details BB@> inherit this from your spawner
            pchild->ai.passage = pself->passage;
            pchild->ai.owner   = pself->owner;

            pchild->dismount_timer  = PHYS_DISMOUNT_TIME;
            pchild->dismount_object = pself->index;
        }
    }

    returncode = INGAME_CHR( ichr );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnExactCharacterXYZ( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SpawnCharacterXYZ( tmpargument = "slot", tmpx = "x", tmpy = "y", tmpdistance = "z", tmpturn = "turn" )
    /// @details ZZ@> This function spawns a character at a specific location, using a
    /// specific model type, failing if x,y,z is invalid
    /// DON'T USE THIS FOR EXPORTABLE ITEMS OR CHARACTERS,
    /// AS THE MODEL SLOTS MAY VARY FROM MODULE TO MODULE.

    fvec3_t   pos;
    CHR_REF ichr;

    SCRIPT_FUNCTION_BEGIN();

    pos.x = pstate->x;
    pos.y = pstate->y;
    pos.z = pstate->distance;

    ichr = spawn_one_character( pos, PRO_REF( pstate->argument ), pchr->team, 0, CLIP_TO_16BITS( pstate->turn ), NULL, CHR_REF( MAX_CHR ) );
    if ( !INGAME_CHR( ichr ) )
    {
        if ( ichr > PMod->importamount * MAXIMPORTPERPLAYER )
        {
            ego_cap * pcap = pro_get_pcap( pchr->profile_ref );

            log_warning( "Object \"%s\"(\"%s\") failed to spawn profile index %d\n", ego_chr::get_obj_ref( *pchr ).base_name, NULL == pcap ? "INVALID" : pcap->classname, pstate->argument );
        }
    }
    else
    {
        ego_chr * pchild = ChrObjList.get_data_ptr( ichr );

        // was the child spawned in a "safe" spot?
        if ( !ego_chr::get_safe( pchild, NULL ) )
        {
            ego_obj_chr::request_terminate( ichr );
            ichr = CHR_REF( MAX_CHR );
        }
        else
        {
            pself->child = ichr;

            pchild->iskursed   = pchr->iskursed;  /// @details BB@> inherit this from your spawner
            pchild->ai.passage = pself->passage;
            pchild->ai.owner   = pself->owner;

            pchild->dismount_timer  = PHYS_DISMOUNT_TIME;
            pchild->dismount_object = pself->index;
        }
    }

    returncode = INGAME_CHR( ichr );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ChangeTargetClass( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // ChangeTargetClass( tmpargument = "slot" )

    /// @details ZZ@> This function changes the target character's model slot.
    /// DON'T USE THIS FOR EXPORTABLE ITEMS OR CHARACTERS, AS THE MODEL SLOTS MAY VARY FROM
    /// MODULE TO MODULE.
    /// USAGE: This is intended as a way to incorporate more player classes into the game.

    SCRIPT_FUNCTION_BEGIN();

    change_character_full( pself->target, PRO_REF( pstate->argument ), 0, ENC_LEAVE_ALL );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_PlayFullSound( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // PlayFullSound( tmpargument = "sound", tmpdistance = "frequency" )
    /// @details ZZ@> This function plays one of the character's sounds .
    /// The sound will be heard at full volume by all players (Victory music)

    SCRIPT_FUNCTION_BEGIN();

    if ( VALID_SND( pstate->argument ) )
    {
        sound_play_chunk_full( ego_chr::get_chunk_ptr( pchr, pstate->argument ) );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnExactChaseParticle( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SpawnExactChaseParticle( tmpargument = "particle", tmpx = "x", tmpy = "y", tmpdistance = "z" )
    /// @details ZZ@> This function spawns a particle at a specific x, y, z position,
    /// that will home in on the character's target

    PRT_REF iprt;
    CHR_REF ichr;

    SCRIPT_FUNCTION_BEGIN();

    ichr = pself->index;
    if ( INGAME_CHR( pchr->attachedto ) )
    {
        ichr = pchr->attachedto;
    }

    {
        fvec3_t vtmp = VECT3( pstate->x, pstate->y, pstate->distance );
        iprt = spawn_one_particle( vtmp, pchr->ori.facing_z, pchr->profile_ref, pstate->argument, CHR_REF( MAX_CHR ), 0, pchr->team, ichr, PRT_REF( MAX_PRT ), 0, CHR_REF( MAX_CHR ) );
    }

    returncode = VALID_PRT( iprt );

    if ( returncode )
    {
        PrtObjList.get_data_ref( iprt ).target_ref = pself->target;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_CreateOrder( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // tmpargument = CreateOrder( tmpx = "value1", tmpy = "value2", tmpargument = "order" )

    /// @details ZZ@> This function compresses tmpx, tmpy, tmpargument ( 0 - 15 ), and the
    /// character's target into tmpargument.  This new tmpargument can then
    /// be issued as an order to teammates.  TranslateOrder will undo the
    /// compression

    Uint16 sTmp = 0;

    SCRIPT_FUNCTION_BEGIN();

    sTmp = (( pself->target ).get_value() & 0x00FF ) << 24;
    sTmp |= (( pstate->x >> 6 ) & 0x03FF ) << 14;
    sTmp |= (( pstate->y >> 6 ) & 0x03FF ) << 4;
    sTmp |= ( pstate->argument & 0x000F );
    pstate->argument = sTmp;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_OrderSpecialID( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // OrderSpecialID( tmpargument = "compressed order", tmpdistance = "idsz" )
    /// @details ZZ@> This function orders all characters with the given special IDSZ.

    SCRIPT_FUNCTION_BEGIN();

    issue_special_order( pstate->argument, pstate->distance );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_UnkurseTargetInventory( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // UnkurseTargetInventory()
    /// @details ZZ@> This function unkurses all items held and in the pockets of the target

    CHR_REF ichr;
    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    ichr = pself_target->holdingwhich[SLOT_LEFT];
    if ( INGAME_CHR( ichr ) )
    {
        ChrObjList.get_data_ref( ichr ).iskursed = bfalse;
    }

    ichr = pself_target->holdingwhich[SLOT_RIGHT];
    if ( INGAME_CHR( ichr ) )
    {
        ChrObjList.get_data_ref( ichr ).iskursed = bfalse;
    }

    PACK_BEGIN_LOOP( ichr, pself_target->pack.next )
    {
        if ( INGAME_CHR( ichr ) )
        {
            ChrObjList.get_data_ref( ichr ).iskursed = bfalse;
        }
    }
    PACK_END_LOOP( ichr );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsSneaking( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfTargetIsSneaking()
    /// @details ZZ@> This function proceeds if the target is doing ACTION_WA or ACTION_DA

    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = ( pself_target->mad_inst.action.which == ACTION_DA || pself_target->mad_inst.action.which == ACTION_WA );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DropItems( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // DropItems()
    /// @details ZZ@> This function drops all of the items the character is holding

    SCRIPT_FUNCTION_BEGIN();

    drop_all_items( pself->index );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_RespawnTarget( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // RespawnTarget()
    /// @details ZZ@> This function respawns the target at its current location

    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    ego_chr::respawn( pself->target );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetDoActionSetFrame( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // TargetDoActionSetFrame( tmpargument = "action" )
    /// @details ZZ@> This function starts the target doing the given action, and also sets
    /// the starting frame to the first of the animation ( so there is no
    /// interpolation 'cause it looks awful in some circumstances )
    /// It will fail if the action is invalid

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( INGAME_CHR( pself->target ) )
    {
        int action;
        ego_chr * pself_target = ChrObjList.get_data_ptr( pself->target );

        action = mad_get_action( pself_target->mad_inst.imad, pstate->argument );

        if ( rv_success == ego_chr::start_anim( pself_target, action, bfalse, btrue ) )
        {
            // remove the interpolation
            pself_target->mad_inst.state.frame_lst = pself_target->mad_inst.state.frame_nxt;

            returncode = btrue;
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetCanSeeInvisible( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfTargetCanSeeInvisible()
    /// @details ZZ@> This function proceeds if the target can see invisible

    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = pself_target->see_invisible_level;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToNearestBlahID( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetTargetToNearestBlahID( tmpargument = "idsz", tmpdistance = "blah bits" )

    /// @details ZZ@> This function finds the NEAREST ( exact ) character that fits the given
    /// parameters, failing if it finds none

    CHR_REF ichr;

    SCRIPT_FUNCTION_BEGIN();

    // Try to find one
    ichr = chr_find_target( pchr, NEAREST, pstate->argument, pstate->distance );

    if ( INGAME_CHR( ichr ) )
    {
        SET_TARGET_0( ichr );
    }
    else
    {
        returncode = bfalse;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToNearestEnemy( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetTargetToNearestEnemy()
    /// @details ZZ@> This function finds the NEAREST ( exact ) enemy, failing if it finds none

    CHR_REF ichr;

    SCRIPT_FUNCTION_BEGIN();

    ichr = chr_find_target( pchr, NEAREST, IDSZ_NONE, TARGET_ENEMIES );

    if ( INGAME_CHR( ichr ) )
    {
        SET_TARGET_0( ichr );
    }
    else
    {
        returncode = bfalse;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToNearestFriend( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetTargetToNearestFriend()
    /// @details ZZ@> This function finds the NEAREST ( exact ) friend, failing if it finds none

    CHR_REF ichr;

    SCRIPT_FUNCTION_BEGIN();

    ichr = chr_find_target( pchr, NEAREST, IDSZ_NONE, TARGET_FRIENDS );

    if ( INGAME_CHR( ichr ) )
    {
        SET_TARGET_0( ichr );
    }
    else
    {
        returncode = bfalse;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToNearestLifeform( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetTargetToNearestLifeform()

    /// @details ZZ@> This function finds the NEAREST ( exact ) friend or enemy, failing if it
    /// finds none

    CHR_REF ichr;

    SCRIPT_FUNCTION_BEGIN();

    ichr = chr_find_target( pchr, NEAREST, IDSZ_NONE, TARGET_ITEMS | TARGET_FRIENDS | TARGET_ENEMIES );

    if ( INGAME_CHR( ichr ) )
    {
        SET_TARGET_0( ichr );
    }
    else
    {
        returncode = bfalse;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_FlashPassage( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // FlashPassage( tmpargument = "passage", tmpdistance = "color" )

    /// @details ZZ@> This function makes the given passage light or dark.
    /// Usage: For debug purposes

    SCRIPT_FUNCTION_BEGIN();

    PassageStack_flash( PASS_REF( pstate->argument ), pstate->distance );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_FindTileInPassage( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // tmpx, tmpy = FindTileInPassage( tmpargument = "passage", tmpdistance = "tile type", tmpx, tmpy )

    /// @details ZZ@> This function finds all tiles of the specified type that lie within the
    /// given passage.  Call multiple times to find multiple tiles.  tmpx and
    /// tmpy will be set to the middle of the found tile if one is found, or
    /// both will be set to 0 if no tile is found.
    /// tmpx and tmpy are required and set on return

    SCRIPT_FUNCTION_BEGIN();

    returncode = _find_grid_in_passage( pstate->x, pstate->y, pstate->distance, PASS_REF( pstate->argument ), &( pstate->x ), &( pstate->y ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_HeldInLeftHand( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfHeldInLeftHand()
    /// @details ZZ@> This function passes if another character is holding the character in its
    /// left hand.
    /// Usage: Used mostly by enchants that target the item of the other hand

    CHR_REF ichr;

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    ichr = pchr->attachedto;
    if ( INGAME_CHR( ichr ) )
    {
        returncode = ( ChrObjList.get_data_ref( ichr ).holdingwhich[SLOT_LEFT] == pself->index );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_NotAnItem( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // NotAnItem()
    /// @details ZZ@> This function makes the character a non-item character.
    /// Usage: Used for spells that summon creatures

    SCRIPT_FUNCTION_BEGIN();

    pchr->isitem = bfalse;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_ChildAmmo( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetChildAmmo( tmpargument = "none" )
    /// @details ZZ@> This function sets the ammo of the last character spawned by this character

    SCRIPT_FUNCTION_BEGIN();

    ChrObjList.get_data_ref( pself->child ).ammo = pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_HitVulnerable( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfHitVulnerable()
    /// @details ZZ@> This function proceeds if the character was hit by a weapon of its
    /// vulnerability IDSZ.
    /// For example, a werewolf gets hit by a [SILV] bullet.

    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_HITVULNERABLE );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsFlying( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfTargetIsFlying()
    /// @details ZZ@> This function proceeds if the character target is flying

    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = IS_FLYING_PCHR( pself_target );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_IdentifyTarget( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IdentifyTarget()
    /// @details ZZ@> This function reveals the target's name, ammo, and usage
    /// Proceeds if the target was unknown

    ego_cap * pcap;
    CHR_REF ichr;

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    ichr = pself->target;
    if ( ChrObjList.get_data_ref( ichr ).ammo_max != 0 )  ChrObjList.get_data_ref( ichr ).ammoknown = btrue;
    if ( 0 == strcmp( "Blah", ChrObjList.get_data_ref( ichr ).name ) )
    {
        returncode = !ChrObjList.get_data_ref( ichr ).nameknown;
        ChrObjList.get_data_ref( ichr ).nameknown = btrue;
    }

    pcap = ego_chr::get_pcap( pself->target );
    if ( NULL != pcap )
    {
        pcap->usageknown = btrue;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_BeatModule( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // BeatModule()
    /// @details ZZ@> This function displays the Module Ended message

    SCRIPT_FUNCTION_BEGIN();

    PMod->beat = btrue;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_EndModule( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // EndModule()
    /// @details ZZ@> This function presses the Escape key

    SCRIPT_FUNCTION_BEGIN();

    // This tells the game to quit
    EProc->escape_requested = btrue;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DisableExport( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // DisableExport()
    /// @details ZZ@> This function turns export off

    SCRIPT_FUNCTION_BEGIN();

    PMod->exportvalid = bfalse;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_EnableExport( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // EnableExport()
    /// @details ZZ@> This function turns export on

    SCRIPT_FUNCTION_BEGIN();

    PMod->exportvalid = btrue;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_get_TargetState( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // tmpargument = GetTargetState()
    /// @details ZZ@> This function sets tmpargument to the state of the target

    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    pstate->argument = pself_target->ai.state;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Equipped( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // This proceeds if the character is equipped

    SCRIPT_FUNCTION_BEGIN();

    returncode = pchr->isequipped;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DropTargetMoney( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // DropTargetMoney( tmpargument = "amount" )
    /// @details ZZ@> This function drops some of the target's money

    SCRIPT_FUNCTION_BEGIN();

    drop_money( pself->target, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_get_TargetContent( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // tmpargument = GetTargetContent()
    // This sets tmpargument to the current Target's content value

    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    pstate->argument = pself_target->ai.content;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DropTargetKeys( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // DropTargetKeys()
    /// @details ZZ@> This function makes the Target drops keys in inventory (Not inhand)

    SCRIPT_FUNCTION_BEGIN();

    drop_all_idsz( pself->target, MAKE_IDSZ( 'K', 'E', 'Y', 'A' ), MAKE_IDSZ( 'K', 'E', 'Y', 'Z' ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_JoinTeam( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // JoinTeam( tmpargument = "team" )
    /// @details ZZ@> This makes the character itself join a specified team (A = 0, B = 1, 23 = Z, etc.)

    SCRIPT_FUNCTION_BEGIN();

    switch_team( pself->index, TEAM_REF( pstate->argument ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetJoinTeam( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // TargetJoinTeam( tmpargument = "team" )
    /// @details ZZ@> This makes the Target join a Team specified in tmpargument (A = 0, 25 = Z, etc.)

    SCRIPT_FUNCTION_BEGIN();

    switch_team( pself->target, TEAM_REF( pstate->argument ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ClearMusicPassage( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // ClearMusicPassage( tmpargument = "passage" )
    /// @details ZZ@> This clears the music for a specified passage

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( pstate->argument >= 0 && pstate->argument < MAX_PASS )
    {
        PASS_REF ipass = PASS_REF( pstate->argument );

        PassageStack[ipass].music = NO_MUSIC;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ClearEndMessage( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // ClearEndMessage()
    /// @details ZZ@> This function empties the end-module text buffer

    SCRIPT_FUNCTION_BEGIN();

    endtext[0] = CSTR_END;
    endtext_carat = 0;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AddEndMessage( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // AddEndMessage( tmpargument = "message" )
    /// @details ZZ@> This function appends a message to the end-module text buffer

    SCRIPT_FUNCTION_BEGIN();

    returncode = _append_end_text( pchr,  pstate->argument, pstate );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_PlayMusic( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // PlayMusic( tmpargument = "song number", tmpdistance = "fade time (msec)" )
    /// @details ZZ@> This function begins playing a new track of music

    SCRIPT_FUNCTION_BEGIN();

    if ( snd.musicvalid && ( songplaying != pstate->argument ) )
    {
        sound_play_song( pstate->argument, pstate->distance, -1 );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_MusicPassage( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetMusicPassage( tmpargument = "passage", tmpturn = "type", tmpdistance = "repetitions" )

    /// @details ZZ@> This function makes the given passage play music if a player enters it
    /// tmpargument is the passage to set and tmpdistance is the music track to play.

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( pstate->argument >= 0 && pstate->argument < MAX_PASS )
    {
        PASS_REF ipass = PASS_REF( pstate->argument );

        PassageStack[ipass].music = pstate->distance;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_MakeCrushInvalid( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // MakeCrushInvalid()
    /// @details ZZ@> This function makes doors unable to close on this object

    SCRIPT_FUNCTION_BEGIN();

    pchr->canbecrushed = bfalse;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_StopMusic( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // StopMusic()
    /// @details ZZ@> This function stops the interactive music

    SCRIPT_FUNCTION_BEGIN();

    sound_stop_song();

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_FlashVariable( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // FlashVariable( tmpargument = "amount" )

    /// @details ZZ@> This function makes the character flash according to tmpargument

    SCRIPT_FUNCTION_BEGIN();

    flash_character( pself->index, pstate->argument );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AccelerateUp( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // AccelerateUp( tmpargument = "acc z" )
    /// @details ZZ@> This function makes the character accelerate up and down

    SCRIPT_FUNCTION_BEGIN();

    pchr->vel.z += pstate->argument / 100.0f;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_FlashVariableHeight( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // FlashVariableHeight( tmpturn = "intensity bottom", tmpx = "bottom", tmpdistance = "intensity top", tmpy = "top" )
    /// @details ZZ@> This function makes the character flash, feet one color, head another.

    SCRIPT_FUNCTION_BEGIN();

    flash_character_height( pself->index, CLIP_TO_16BITS( pstate->turn ), pstate->x, pstate->distance, pstate->y );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_DamageTime( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetDamageTime( tmpargument = "time" )
    /// @details ZZ@> This function makes the character invincible for a little while

    SCRIPT_FUNCTION_BEGIN();

    pchr->damagetime = pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_StateIs8( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    SCRIPT_FUNCTION_BEGIN();

    returncode = ( 8 == pself->state );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_StateIs9( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    SCRIPT_FUNCTION_BEGIN();

    returncode = ( 9 == pself->state );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_StateIs10( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    SCRIPT_FUNCTION_BEGIN();

    returncode = ( 10 == pself->state );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_StateIs11( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    SCRIPT_FUNCTION_BEGIN();

    returncode = ( 11 == pself->state );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_StateIs12( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    SCRIPT_FUNCTION_BEGIN();

    returncode = ( 12 == pself->state );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_StateIs13( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    SCRIPT_FUNCTION_BEGIN();

    returncode = ( 13 == pself->state );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_StateIs14( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    SCRIPT_FUNCTION_BEGIN();

    returncode = ( 14 == pself->state );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_StateIs15( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    SCRIPT_FUNCTION_BEGIN();

    returncode = ( 15 == pself->state );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsAMount( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfTargetIsAMount()
    /// @details ZZ@> This function passes if the Target is a mountable character

    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = pself_target->ismount;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsAPlatform( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfTargetIsAPlatform()
    /// @details ZZ@> This function passes if the Target is a platform character

    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = pself_target->platform;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AddStat( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // AddStat()
    /// @details ZZ@> This function turns on an NPC's status display

    SCRIPT_FUNCTION_BEGIN();

    StatList.add( pself->index );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DisenchantTarget( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // DisenchantTarget()
    /// @details ZZ@> This function removes all enchantments on the Target character, proceeding
    /// if there were any, failing if not

    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = ( pself_target->firstenchant != MAX_ENC );

    remove_all_character_enchants( pself->target );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DisenchantAll( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // DisenchantAll()
    /// @details ZZ@> This function removes all enchantments in the game

    ENC_REF iTmp;

    SCRIPT_FUNCTION_BEGIN();

    for ( iTmp = 0; iTmp < MAX_ENC; iTmp++ )
    {
        remove_enchant( iTmp, NULL );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_VolumeNearestTeammate( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetVolumeNearestTeammate( tmpargument = "sound", tmpdistance = "distance" )
    /// @details ZZ@> This function lets insects buzz correctly.  The closest Team member
    /// is used to determine the overall sound level.

    SCRIPT_FUNCTION_BEGIN();

    /*PORT
    if(moduleactive && pstate->distance >= 0)
    {
    // Find the closest Teammate
    iTmp = 10000;
    sTmp = 0;
    while(sTmp < MAX_CHR)
    {
    if(INGAME_CHR(sTmp) && ChrObjList.get_data_ref(sTmp).alive && ChrObjList.get_data_ref(sTmp).Team == pchr->Team)
    {
    distance = SDL_abs(PCamera->trackx-ChrObjList.get_data_ref(sTmp).pos_old.x)+SDL_abs(PCamera->tracky-ChrObjList.get_data_ref(sTmp).pos_old.y);
    if(distance < iTmp)  iTmp = distance;
    }
    sTmp++;
    }
    distance=iTmp+pstate->distance;
    volume = -distance;
    volume = volume<<VOLSHIFT;
    if(volume < VOLMIN) volume = VOLMIN;
    iTmp = CapStack[pro_get_icap(pchr->profile_ref)].wavelist[pstate->argument];
    if(iTmp < numsound && iTmp >= 0 && soundon)
    {
    lpDSBuffer[iTmp]->SetVolume(volume);
    }
    }
    */

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AddShopPassage( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // AddShopPassage( tmpargument = "passage" )
    /// @details ZZ@> This function makes a passage behave as a shop area, as long as the
    /// character is alive.

    SCRIPT_FUNCTION_BEGIN();

    ShopStack_add_one( pself->index, PASS_REF( pstate->argument ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetPayForArmor( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // tmpx, tmpy = TargetPayForArmor( tmpargument = "skin" )

    /// @details ZZ@> This function costs the Target the appropriate amount of money for the
    /// given armor type.  Passes if the character has enough, and fails if not.
    /// Does trade-in bonus automatically.  tmpy is always set to cost of requested
    /// skin tmpx is set to amount needed after trade-in ( 0 for pass ).

    int iTmp;
    ego_cap * pcap;
    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    if ( !INGAME_CHR( pself->target ) ) return bfalse;

    pself_target = ChrObjList.get_data_ptr( pself->target );

    pcap = ego_chr::get_pcap( pself->target );         // The Target's model
    if ( NULL == pcap )  return bfalse;

    iTmp = pcap->skincost[pstate->argument&3];
    pstate->y = iTmp;                             // Cost of new skin

    iTmp -= pcap->skincost[pself_target->skin];        // Refund

    if ( iTmp > pself_target->money )
    {
        // Not enough.
        pstate->x = iTmp - pself_target->money;        // Amount needed
        returncode = bfalse;
    }
    else
    {
        // Pay for it.  Cost may be negative after refund.
        pself_target->money -= iTmp;
        if ( pself_target->money > MAXMONEY )  pself_target->money = MAXMONEY;

        pstate->x = 0;
        returncode = btrue;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_JoinEvilTeam( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // JoinEvilTeam()
    /// @details ZZ@> This function adds the character to the evil Team.

    SCRIPT_FUNCTION_BEGIN();

    switch_team( pself->index, TEAM_REF( TEAM_EVIL ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_JoinNullTeam( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // JoinNullTeam()
    /// @details ZZ@> This function adds the character to the null Team.

    SCRIPT_FUNCTION_BEGIN();

    switch_team( pself->index, TEAM_REF( TEAM_NULL ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_JoinGoodTeam( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // JoinGoodTeam()
    /// @details ZZ@> This function adds the character to the good Team.

    SCRIPT_FUNCTION_BEGIN();

    switch_team( pself->index, TEAM_REF( TEAM_GOOD ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_PitsKill( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // PitsKill()
    /// @details ZZ@> This function activates pit deaths for when characters fall below a
    /// certain altitude.

    SCRIPT_FUNCTION_BEGIN();

    pits.kill = btrue;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToPassageID( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetTargetToPassageID( tmpargument = "passage", tmpdistance = "idsz" )
    /// @details ZZ@> This function finds a character who is both in the passage and who has
    /// an item with the given IDSZ

    CHR_REF ichr;

    SCRIPT_FUNCTION_BEGIN();

    ichr = PassageStack_who_is_blocking( PASS_REF( pstate->argument ), pself->index, IDSZ_NONE, TARGET_SELF | TARGET_FRIENDS | TARGET_ENEMIES, pstate->distance );

    if ( INGAME_CHR( ichr ) )
    {
        SET_TARGET_0( ichr );
    }
    else
    {
        returncode = bfalse;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_MakeNameUnknown( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // MakeNameUnknown()
    /// @details ZZ@> This function makes the name of an item/character unknown.
    /// Usage: Use if you have subspawning of creatures from a book.

    SCRIPT_FUNCTION_BEGIN();

    pchr->nameknown = bfalse;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnExactParticleEndSpawn( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SpawnExactParticleEndSpawn( tmpargument = "particle", tmpturn = "state", tmpx = "x", tmpy = "y", tmpdistance = "z" )

    /// @details ZZ@> This function spawns a particle at a specific x, y, z position.
    /// When the particle ends, a character is spawned at its final location.
    /// The character is the same type of whatever spawned the particle.

    PRT_REF iprt;
    CHR_REF ichr;

    SCRIPT_FUNCTION_BEGIN();

    ichr = pself->index;
    if ( INGAME_CHR( pchr->attachedto ) )
    {
        ichr = pchr->attachedto;
    }

    {
        fvec3_t vtmp = VECT3( pstate->x, pstate->y, pstate->distance );
        iprt = spawn_one_particle( vtmp, pchr->ori.facing_z, pchr->profile_ref, pstate->argument, CHR_REF( MAX_CHR ), 0, pchr->team, ichr, PRT_REF( MAX_PRT ), 0, CHR_REF( MAX_CHR ) );
    }

    returncode = VALID_PRT( iprt );

    if ( returncode )
    {
        PrtObjList.get_data_ref( iprt ).spawncharacterstate = pstate->turn;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnPoofSpeedSpacingDamage( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SpawnPoofSpeedSpacingDamage( tmpx = "xy speed", tmpy = "xy spacing", tmpargument = "damage" )

    /// @details ZZ@> This function makes a lovely little poof at the character's location,
    /// adjusting the xy speed and spacing and the base damage first
    /// Temporarily adjust the values for the particle type

    int   tTmp, iTmp;
    float fTmp;

    ego_cap * pcap;
    ego_pip * ppip;

    SCRIPT_FUNCTION_BEGIN();

    pcap = pro_get_pcap( pchr->profile_ref );
    if ( NULL == pcap ) return bfalse;

    ppip = pro_get_ppip( pchr->profile_ref, pcap->gopoofprt_pip );
    if ( NULL == ppip ) return bfalse;

    returncode = bfalse;
    if ( NULL != ppip )
    {
        // save some values
        iTmp = ppip->vel_hrz_pair.base;
        tTmp = ppip->spacing_hrz_pair.base;
        fTmp = ppip->damage.from;

        // set some values
        ppip->vel_hrz_pair.base     = pstate->x;
        ppip->spacing_hrz_pair.base = pstate->y;
        ppip->damage.from           = SFP8_TO_FLOAT( pstate->argument );

        spawn_poof( pself->index, pchr->profile_ref );

        // Restore the saved values
        ppip->vel_hrz_pair.base     = iTmp;
        ppip->spacing_hrz_pair.base = tTmp;
        ppip->damage.from           = fTmp;

        returncode = btrue;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GiveExperienceToGoodTeam( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // GiveExperienceToGoodTeam(  tmpargument = "amount", tmpdistance = "type" )
    /// @details ZZ@> This function gives experience to everyone on the G Team

    SCRIPT_FUNCTION_BEGIN();

    give_team_experience( TEAM_REF( TEAM_GOOD ), pstate->argument, ( xp_type )pstate->distance );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DoNothing( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // DoNothing()
    /// @details ZF@> This function does nothing
    /// Use this for debugging or in combination with a Else function

    return btrue;
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GrogTarget( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // GrogTarget( tmpargument = "amount" )
    /// @details ZF@> This function grogs the Target for a duration equal to tmpargument

    ego_cap * pcap;
    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    pcap = ego_chr::get_pcap( pself->target );

    returncode = bfalse;
    if ( NULL != pcap && pcap->canbegrogged )
    {
        pself_target->grogtime += pstate->argument;
        returncode = btrue;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DazeTarget( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // DazeTarget( tmpargument = "amount" )
    /// @details ZF@> This function dazes the Target for a duration equal to tmpargument

    ego_cap * pcap;
    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    pcap = ego_chr::get_pcap( pself->target );

    // Characters who manage to daze themselves are ignore their daze immunity
    returncode = bfalse;
    if ( NULL != pcap && ( pcap->canbedazed || pself->index == pself->target ) )
    {
        pself_target->dazetime += pstate->argument;
        returncode = btrue;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_EnableRespawn( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // EnableRespawn()
    /// @details ZF@> This function turns respawn with JUMP button on

    SCRIPT_FUNCTION_BEGIN();

    PMod->respawnvalid = btrue;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DisableRespawn( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // DisableRespawn()
    /// @details ZF@> This function turns respawn with JUMP button off

    SCRIPT_FUNCTION_BEGIN();

    PMod->respawnvalid = bfalse;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_HolderBlocked( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfHolderBlocked()
    /// @details ZF@> This function passes if the holder blocked an attack

    CHR_REF iattached;

    SCRIPT_FUNCTION_BEGIN();

    iattached = pchr->attachedto;

    if ( INGAME_CHR( iattached ) )
    {
        BIT_FIELD bits = ChrObjList.get_data_ref( iattached ).ai.alert;

        if ( HAS_SOME_BITS( bits, ALERTIF_BLOCKED ) )
        {
            CHR_REF iattacked = ChrObjList.get_data_ref( iattached ).ai.attacklast;

            if ( INGAME_CHR( iattacked ) )
            {
                SET_TARGET_0( iattacked );
            }
            else
            {
                returncode = bfalse;
            }
        }
        else
        {
            returncode = bfalse;
        }
    }
    else
    {
        returncode = bfalse;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetHasNotFullMana( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfTargetHasNotFullMana()
    /// @details ZF@> This function passes only if the Target is not at max mana and alive

    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    if ( !pself_target->alive || pself_target->mana > pself_target->mana_max - HURTDAMAGE )
    {
        returncode = bfalse;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_EnableListenSkill( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    /// @details ZF@> TODO: deprecated, replace this function with something else

    log_warning( "Deprecated script function used (EnableListenSkill) in %s.\n", pbdl_self->chr_ptr->name );
    return bfalse;
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToLastItemUsed( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetTargetToLastItemUsed()
    /// @details ZF@> This sets the Target to the last item the character used

    SCRIPT_FUNCTION_BEGIN();

    if ( pself->lastitemused != pself->index && INGAME_CHR( pself->lastitemused ) )
    {
        SET_TARGET_0( pself->lastitemused );
    }
    else
    {
        returncode = bfalse;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_FollowLink( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // FollowLink( tmpargument = "index of next module name" )
    /// @details BB@> Skips to the next module!

    int message_number, message_index;
    char * ptext;

    SCRIPT_FUNCTION_BEGIN();

    message_number = ppro->message_start + pstate->argument;
    message_index  = MessageOffset.ary[message_number];

    ptext = message_buffer + message_index;

    returncode = link_follow_modname( ptext, btrue );
    if ( !returncode )
    {
        debug_printf( "That's too scary for %s", pchr->name );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_OperatorIsLinux( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfOperatorIsLinux()
    /// @details ZF@> Proceeds if running on linux

    SCRIPT_FUNCTION_BEGIN();

#if defined(__unix__)
    returncode = btrue;
#else
    returncode = bfalse;
#endif

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsAWeapon( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfTargetIsAWeapon()
    /// @details ZF@> Proceeds if the AI Target Is a melee or ranged weapon

    ego_cap * pcap;

    SCRIPT_FUNCTION_BEGIN();

    if ( !INGAME_CHR( pself->target ) ) return bfalse;

    pcap = ego_chr::get_pcap( pself->target );
    if ( NULL == pcap ) return bfalse;

    returncode = pcap->isranged || ego_chr::has_idsz( pself->target, MAKE_IDSZ( 'X', 'W', 'E', 'P' ) );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SomeoneIsStealing( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfSomeoneIsStealing()
    /// @details ZF@> This function passes if someone stealed from its shop

    SCRIPT_FUNCTION_BEGIN();

    returncode = ( pself->order_value == SHOP_STOLEN && pself->order_counter == SHOP_THEFT );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsASpell( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfTargetIsASpell()
    /// @details ZF@> roceeds if the AI Target has any particle with the [IDAM] or [WDAM] expansion

    int iTmp;

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    for ( iTmp = 0; iTmp < MAX_PIP_PER_PROFILE; iTmp++ )
    {
        ego_pip * ppip = pro_get_ppip( pchr->profile_ref, iTmp );
        if ( NULL == ppip ) continue;

        if ( ppip->intdamagebonus || ppip->wisdamagebonus )
        {
            returncode = btrue;
            break;
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_Backstabbed( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfBackstabbed()
    /// @details ZF@> Proceeds if HitFromBehind, target has [STAB] skill and damage dealt is physical
    /// automatically fails if attacker has a code of conduct
    SCRIPT_FUNCTION_BEGIN();

    // Now check if it really was backstabbed
    returncode = bfalse;
    if ( HAS_SOME_BITS( pself->alert, ALERTIF_ATTACKED ) )
    {
        // Who is the dirty backstabber?
        ego_chr * pattacker = ChrObjList.get_data_ptr( pself->attacklast );
        if ( !PROCESSING_PCHR( pattacker ) ) return bfalse;

        // Only if hit from behind
        if ( pself->directionlast >= ATK_BEHIND - 8192 && pself->directionlast < ATK_BEHIND + 8192 )
        {
            // And require the backstab skill
            if ( ego_chr_data::get_skill( pattacker, MAKE_IDSZ( 'S', 'T', 'A', 'B' ) ) )
            {
                // Finally we require it to be physical damage!
                Uint16 sTmp = pself->damagetypelast;
                if ( sTmp == DAMAGE_CRUSH || sTmp == DAMAGE_POKE || sTmp == DAMAGE_SLASH ) returncode = btrue;
            }
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_get_TargetDamageType( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // tmpargument = GetTargetDamageType()
    /// @details ZF@> This function gets the last type of damage for the Target

    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    pstate->argument = pself_target->ai.damagetypelast;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AddQuest( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // AddQuest( tmpargument = "quest idsz" )
    /// @details ZF@> This function adds a quest idsz set in tmpargument into the targets quest.txt to 0

    int quest_level = QUEST_NONE;
    ego_chr * pself_target;
    PLA_REF ipla;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    ipla = pself_target->is_which_player;
    if ( VALID_PLA( ipla ) )
    {
        ego_player * ppla = PlaStack + ipla;

        quest_level = quest_add( ppla->quest_log, SDL_arraysize( ppla->quest_log ), pstate->argument, 0 );
    }

    returncode = ( 0 == quest_level );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_BeatQuestAllPlayers( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // BeatQuestAllPlayers()
    /// @details ZF@> This function marks a IDSZ in the targets quest.txt as beaten
    ///               returns true if at least one quest got marked as beaten.

    PLA_REF ipla;

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    for ( ipla = 0; ipla < MAX_PLAYER; ipla++ )
    {
        CHR_REF ichr;
        ego_player * ppla = PlaStack + ipla;

        if ( !ppla->valid ) continue;

        ichr = ppla->index;
        if ( !INGAME_CHR( ichr ) ) continue;

        if ( QUEST_BEATEN == quest_adjust_level( ppla->quest_log, SDL_arraysize( ppla->quest_log ), ( IDSZ )pstate->argument, QUEST_MAXVAL ) )
        {
            returncode = btrue;
        }
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetHasQuest( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // tmpdistance = IfTargetHasQuest( tmpargument = "quest idsz )
    /// @details ZF@> This function proceeds if the Target has the unfinIshed quest specified in tmpargument
    /// and sets tmpdistance to the Quest Level of the specified quest.

    int     quest_level = QUEST_NONE;
    ego_chr * pself_target = NULL;
    PLA_REF ipla;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = bfalse;
    ipla = pchr->is_which_player;
    if ( VALID_PLA( ipla ) )
    {
        ego_player * ppla = PlaStack + ipla;

        quest_level = quest_get_level( ppla->quest_log, SDL_arraysize( ppla->quest_log ), pstate->argument );
    }

    // only find active quests
    if ( quest_level >= 0 )
    {
        pstate->distance = quest_level;
        returncode       = btrue;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_QuestLevel( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetQuestLevel( tmpargument = "idsz", distance = "adjustment" )
    /// @details ZF@> This function modifies the quest level for a specific quest IDSZ
    /// tmpargument specifies quest idsz (tmpargument) and the adjustment (tmpdistance, which may be negative)

    ego_chr * pself_target;
    PLA_REF ipla;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = bfalse;
    ipla = pself_target->is_which_player;
    if ( VALID_PLA( ipla ) && pstate->distance != 0 )
    {
        int        quest_level = QUEST_NONE;
        ego_player * ppla        = PlaStack + ipla;

        quest_level = quest_adjust_level( ppla->quest_log, SDL_arraysize( ppla->quest_log ), pstate->argument, pstate->distance );

        returncode = QUEST_NONE != quest_level;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AddQuestAllPlayers( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // AddQuestAllPlayers( tmpargument = "quest idsz" )
    /// @details ZF@> This function adds a quest idsz set in tmpargument into all local player's quest logs
    /// The quest level Is set to tmpdistance if the level Is not already higher

    PLA_REF ipla;
    int success_count, player_count;

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    for ( player_count = 0, success_count = 0, ipla = 0; ipla < MAX_PLAYER; ipla++ )
    {
        int quest_level;
        ego_player * ppla = PlaStack + ipla;

        if ( !ppla->valid || !INGAME_CHR( ppla->index ) ) continue;
        player_count++;

        // Try to add it or replace it if this one is higher
        quest_level = quest_add( ppla->quest_log, SDL_arraysize( ppla->quest_log ), pstate->argument, pstate->distance );
        if ( QUEST_NONE != quest_level ) success_count++;
    }

    returncode = ( player_count > 0 ) && ( success_count >= player_count );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AddBlipAllEnemies( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // AddBlipAllEnemies()
    /// @details ZF@> show all enemies on the minimap who match the IDSZ given in tmpargument
    /// it show only the enemies of the AI Target

    SCRIPT_FUNCTION_BEGIN();

    if ( INGAME_CHR( pself->target ) )
    {
        local_stats.sense_enemy_team = ego_chr::get_iteam( pself->target );
        local_stats.sense_enemy_ID   = pstate->argument;
    }
    else
    {
        local_stats.sense_enemy_team = TEAM_REF( TEAM_MAX );
        local_stats.sense_enemy_ID   = IDSZ_NONE;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_PitsFall( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // PitsFall( tmpx = "teleprt x", tmpy = "teleprt y", tmpdistance = "teleprt z" )
    /// @details ZF@> This function activates pit teleportation.

    SCRIPT_FUNCTION_BEGIN();

    if ( pstate->x > EDGE && pstate->y > EDGE && pstate->x < PMesh->gmem.edge_x - EDGE && pstate->y < PMesh->gmem.edge_y - EDGE )
    {
        pits.teleport = btrue;
        pits.teleport_pos.x = pstate->x;
        pits.teleport_pos.y = pstate->y;
        pits.teleport_pos.z = pstate->distance;
    }
    else
    {
        pits.kill = btrue;          // make it kill instead
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsOwner( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfTargetIsOwner()
    /// @details ZF@> This function proceeds only if the Target is the character's owner

    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = ( pself_target->alive && pself->owner == pself->target );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SpawnAttachedCharacter( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SpawnAttachedCharacter( tmpargument = "profile", tmpx = "x", tmpy = "y", tmpdistance = "z" )

    /// @details ZF@> This function spawns a character defined in tmpargument to the characters AI target using
    /// the slot specified in tmpdistance (LEFT, RIGHT or INVENTORY). Fails if the inventory or
    /// grip specified is full or already in use.
    /// DON'T USE THIS FOR EXPORTABLE ITEMS OR CHARACTERS,
    /// AS THE MODEL SLOTS MAY VARY FROM MODULE TO MODULE.

    fvec3_t pos;
    CHR_REF ichr;
    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    pos.x = pstate->x;
    pos.y = pstate->y;
    pos.z = pstate->distance;

    ichr = spawn_one_character( pos, PRO_REF( pstate->argument ), pchr->team, 0, FACE_NORTH, NULL, CHR_REF( MAX_CHR ) );
    if ( !INGAME_CHR( ichr ) )
    {
        if ( ichr > PMod->importamount * MAXIMPORTPERPLAYER )
        {
            ego_cap * pcap = pro_get_pcap( pchr->profile_ref );

            log_warning( "Object \"%s\"(\"%s\") failed to spawn profile index %d\n", ego_chr::get_obj_ref( *pchr ).base_name, NULL == pcap ? "IVALID" : pcap->classname, pstate->argument );
        }
    }
    else
    {
        ego_chr * pchild = ChrObjList.get_data_ptr( ichr );

        Uint8 grip = ( Uint8 )CLIP( pstate->distance, ATTACH_INVENTORY, ATTACH_RIGHT );

        if ( grip == ATTACH_INVENTORY )
        {
            // Inventory character
            if ( chr_inventory_add_item( ichr, pself->target ) )
            {
                ego_ai_bundle tmp_bdl_ai;

                ADD_BITS( pchild->ai.alert, ALERTIF_GRABBED );  // Make spellbooks change
                pchild->attachedto = pself->target;  // Make grab work
                scr_run_chr_script( ego_ai_bundle::set( &tmp_bdl_ai, pchild ) );  // Empty the grabbed messages

                pchild->attachedto = CHR_REF( MAX_CHR );  // Fix grab

                // Set some AI values
                pself->child = ichr;
                pchild->ai.passage = pself->passage;
                pchild->ai.owner   = pself->owner;
            }

            // No more room!
            else
            {
                ego_obj_chr::request_terminate( ichr );
                ichr = CHR_REF( MAX_CHR );
            }
        }
        else if ( grip == ATTACH_LEFT || grip == ATTACH_RIGHT )
        {
            if ( !INGAME_CHR( pself_target->holdingwhich[grip] ) )
            {
                ego_ai_bundle tmp_bdl_ai;

                // Wielded character
                grip_offset_t grip_off = ( ATTACH_LEFT == grip ) ? GRIP_LEFT : GRIP_RIGHT;
                attach_character_to_mount( ichr, pself->target, grip_off );

                // Handle the "grabbed" messages
                scr_run_chr_script( ego_ai_bundle::set( &tmp_bdl_ai, pchild ) );

                // Set some AI values
                pself->child = ichr;
                pchild->ai.passage = pself->passage;
                pchild->ai.owner   = pself->owner;
            }

            // Grip is already used
            else
            {
                ego_obj_chr::request_terminate( ichr );
                ichr = CHR_REF( MAX_CHR );
            }
        }
        else
        {
            // we have been given an invalid attachment point.
            // still allow the character to spawn if it is not in an invalid area

            // technically this should never occur since we are limiting the attachment points above
            if ( !ego_chr::get_safe( pchild, NULL ) )
            {
                ego_obj_chr::request_terminate( ichr );
                ichr = CHR_REF( MAX_CHR );
            }
        }
    }

    returncode = INGAME_CHR( ichr );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToChild( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetTargetToChild()
    /// @details ZF@> This function sets the target to the character it spawned last (also called its "child")

    SCRIPT_FUNCTION_BEGIN();

    if ( INGAME_CHR( pself->child ) )
    {
        SET_TARGET_0( pself->child );
    }
    else
    {
        returncode = bfalse;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_DamageThreshold( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetDamageThreshold()
    /// @details ZF@> This sets the damage threshold for this character. Damage below the threshold is ignored

    SCRIPT_FUNCTION_BEGIN();

    if ( pstate->argument > 0 ) pchr->damagethreshold = pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_End( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // End()
    /// @details ZZ@> This Is the last function in a script

    SCRIPT_FUNCTION_BEGIN();

    pself->terminate = btrue;
    returncode       = bfalse;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TakePicture( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // TakePicture()
    /// @details ZF@> This function proceeds only if the screenshot was successful

    SCRIPT_FUNCTION_BEGIN();

    returncode = dump_screenshot();

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_Speech( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetSpeech( tmpargument = "sound" )
    /// @details ZZ@> This function sets all of the RTS speech registers to tmpargument

    Uint16 sTmp = 0;

    SCRIPT_FUNCTION_BEGIN();

    for ( sTmp = SPEECH_BEGIN; sTmp <= SPEECH_END; sTmp++ )
    {
        pchr->sound_index[sTmp] = pstate->argument;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_MoveSpeech( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetMoveSpeech( tmpargument = "sound" )
    /// @details ZZ@> This function sets the RTS move speech register to tmpargument

    SCRIPT_FUNCTION_BEGIN();

    pchr->sound_index[SPEECH_MOVE] = pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_SecondMoveSpeech( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetSecondMoveSpeech( tmpargument = "sound" )
    /// @details ZZ@> This function sets the RTS movealt speech register to tmpargument

    SCRIPT_FUNCTION_BEGIN();

    pchr->sound_index[SPEECH_MOVEALT] = pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_AttackSpeech( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetAttacksSpeech( tmpargument = "sound" )
    /// @details ZZ@> This function sets the RTS attack speech register to tmpargument

    SCRIPT_FUNCTION_BEGIN();

    pchr->sound_index[SPEECH_ATTACK] = pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_AssistSpeech( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetAssistSpeech( tmpargument = "sound" )
    /// @details ZZ@> This function sets the RTS assist speech register to tmpargument

    SCRIPT_FUNCTION_BEGIN();

    pchr->sound_index[SPEECH_ASSIST] = pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TerrainSpeech( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetTerrainSpeech( tmpargument = "sound" )
    /// @details ZZ@> This function sets the RTS terrain speech register to tmpargument

    SCRIPT_FUNCTION_BEGIN();

    pchr->sound_index[SPEECH_TERRAIN] = pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_SelectSpeech( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetSelectSpeech( tmpargument = "sound" )
    /// @details ZZ@> This function sets the RTS select speech register to tmpargument

    SCRIPT_FUNCTION_BEGIN();

    pchr->sound_index[SPEECH_SELECT] = pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_OperatorIsMacintosh( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfOperatorIsMacintosh()
    /// @details ZF@> Proceeds if the current running OS is mac

    SCRIPT_FUNCTION_BEGIN();

#if defined(__APPLE__)
    returncode = btrue;
#else
    returncode = bfalse;
#endif

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_ModuleHasIDSZ( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfModuleHasIDSZ( tmpargument = "message number with module name", tmpdistance = "idsz" )

    /// @details ZF@> Proceeds if the specified module has the required IDSZ specified in tmpdistance
    /// The module folder name to be checked is a string from message.txt

    int message_number, message_index;
    char *ptext;

    SCRIPT_FUNCTION_BEGIN();

    // use message.txt to send the module name
    message_number = ppro->message_start + pstate->argument;
    message_index  = MessageOffset.ary[message_number];
    ptext = message_buffer + message_index;

    returncode = module_has_idsz_vfs( PMod->loadname, pstate->distance, 0, ptext );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_MorphToTarget( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // MorphToTarget()
    /// @details ZF@> This morphs the character into the target
    /// Also set size and keeps the previous AI type

    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    if ( !INGAME_CHR( pself->target ) ) return bfalse;

    ego_chr::change_profile( pself->index, pself_target->basemodel_ref, pself_target->skin, ENC_LEAVE_ALL );

    // let the resizing take some time
    pchr->fat_goto      = pself_target->fat;
    pchr->fat_goto_time = SIZETIME;

    // change back to our original AI
    pself->type      = ProList.lst[pchr->basemodel_ref].iai;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GiveManaFlowToTarget( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // GiveManaFlowToTarget()
    /// @details ZF@> Permanently boost the target's mana flow

    int iTmp;
    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    if ( pself_target->alive )
    {
        iTmp = pstate->argument;
        getadd( 0, pself_target->mana_flow, PERFECTSTAT, &iTmp );
        pself_target->mana_flow += iTmp;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GiveManaReturnToTarget( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // GiveManaReturnToTarget()
    /// @details ZF@> Permanently boost the target's mana return

    int iTmp;
    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    if ( pself_target->alive )
    {
        iTmp = pstate->argument;
        getadd( 0, pself_target->mana_return, PERFECTSTAT, &iTmp );
        pself_target->mana_return += iTmp;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_Money( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetMoney()
    /// @details ZF@> Permanently sets the money for the character to tmpargument

    SCRIPT_FUNCTION_BEGIN();

    pchr->money = CLIP( pstate->argument, 0, MAXMONEY );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetCanSeeKurses( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfTargetCanSeeKurses()
    /// @details ZF@> Proceeds if the target can see kursed stuff.

    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = pself_target->see_kurse_level != 0;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DispelTargetEnchantID( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // DispelEnchantID( tmpargument = "idsz" )
    /// @details ZF@> This function removes all enchants from the target who match the specified RemovedByIDSZ

    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = bfalse;
    if ( pself_target->alive )
    {
        // Check all enchants to see if they are removed
        returncode = remove_all_enchants_with_idsz( pself->target, pstate->argument );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_KurseTarget( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // KurseTarget()
    /// @details ZF@> This makes the target kursed

    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    returncode = bfalse;
    if ( pself_target->isitem && !pself_target->iskursed )
    {
        pself_target->iskursed = btrue;
        returncode = btrue;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_ChildContent( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetChildContent( tmpargument = "content" )
    /// @details ZF@> This function lets a character set the content of the last character it
    /// spawned last

    SCRIPT_FUNCTION_BEGIN();

    ChrObjList.get_data_ref( pself->child ).ai.content = pstate->argument;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_AccelerateTargetUp( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // AccelerateTargetUp( tmpargument = "acc z" )
    /// @details ZF@> This function makes the target accelerate up and down

    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    pself_target->vel.z += pstate->argument / 100.0f;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetAmmo( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetTargetAmmo( tmpargument = "ammo" )
    /// @details ZF@> This function sets the ammo of the character's current AI target

    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    pself_target->ammo = SDL_min( pstate->argument, pself_target->ammo_max );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_EnableInvictus( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // EnableInvictus()
    /// @details ZF@> This function makes the character invulerable

    SCRIPT_FUNCTION_BEGIN();

    pchr->invictus = btrue;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DisableInvictus( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // DisableInvictus()
    /// @details ZF@> This function makes the character not invulerable

    SCRIPT_FUNCTION_BEGIN();

    pchr->invictus = bfalse;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetDamageSelf( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // TargetDamageSelf( tmpargument = "damage" )
    /// @details ZF@> This function applies little bit of hate from the character's target to
    /// the character itself. The amount is set in tmpargument

    IPair tmp_damage;

    SCRIPT_FUNCTION_BEGIN();

    tmp_damage.base = pstate->argument;
    tmp_damage.rand = 1;

    damage_character( pself->index, ATK_FRONT, tmp_damage, pstate->distance, ego_chr::get_iteam( pself->target ), pself->target, DAMFX_NBLOC, btrue );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_SetTargetSize( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetSize( tmpargument = "percent" )
    /// @details ZF@> This changes the AI target's size

    ego_chr * pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    pself_target->fat_goto *= pstate->argument / 100.0f;
    pself_target->fat_goto_time += SIZETIME;

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_DrawBillboard( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // DrawBillboard( tmpargument = "message", tmpdistance = "duration", tmpturn = "color" )
    /// @details ZF@> This function draws one of those billboards above the character

    SCRIPT_FUNCTION_BEGIN();

    returncode = bfalse;
    if ( LOADED_PRO( pchr->profile_ref ) )
    {
        SDL_Color text_color = {0xFF, 0xFF, 0xFF, 0xFF};
        int message_number, message_index;
        char *ptext;

        // List of avalible colours
        GLXvector4f tint_red  = { 1.00f, 0.25f, 0.25f, 1.00f };
        GLXvector4f tint_purple = { 0.88f, 0.75f, 1.00f, 1.00f };
        GLXvector4f tint_white = { 1.00f, 1.00f, 1.00f, 1.00f };
        GLXvector4f tint_yellow = { 1.00f, 1.00f, 0.75f, 1.00f };
        GLXvector4f tint_green = { 0.25f, 1.00f, 0.25f, 1.00f };
        GLXvector4f tint_blue = { 0.25f, 0.25f, 1.00f, 1.00f };

        // Figure out which color to use
        GLfloat *do_tint;
        switch ( pstate->turn )
        {
            default:
            case COLOR_WHITE:    do_tint = tint_white;    break;
            case COLOR_RED:        do_tint = tint_red;        break;
            case COLOR_PURPLE:    do_tint = tint_purple;    break;
            case COLOR_YELLOW:    do_tint = tint_yellow;    break;
            case COLOR_GREEN:    do_tint = tint_green;    break;
            case COLOR_BLUE:    do_tint = tint_blue;    break;
        }

        message_number = ppro->message_start + pstate->argument;
        message_index  = MessageOffset.ary[message_number];
        ptext = message_buffer + message_index;

        returncode = NULL != chr_make_text_billboard( pself->index, ptext, text_color, do_tint, pstate->distance, ( BIT_FIELD )bb_opt_all );
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_set_TargetToBlahInPassage( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // SetTargetToBlahInPassage( tmpargument = "passage", tmpdistance = "targeting_bits", tmpturn = "idsz" )
    /// @details ZF@> This function sets the target to whatever object with the specified bits
    /// in tmpdistance is blocking the given passage. This function lets passage rectangles be used as event triggers
    /// Note that this function also trigges if the character itself (passage owner) is in the passage.

    CHR_REF ichr;

    SCRIPT_FUNCTION_BEGIN();

    ichr = PassageStack_who_is_blocking( PASS_REF( pstate->argument ), pself->index, pstate->turn, TARGET_SELF | pstate->distance, IDSZ_NONE );

    if ( INGAME_CHR( ichr ) )
    {
        SET_TARGET_0( ichr );
    }
    else
    {
        returncode = bfalse;
    }

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_TargetIsFacingSelf( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfTargetIsFacingSelf()
    /// @details ZF@> This function proceeds if the target is more or less facing the character
    FACING_T sTmp = 0;
    ego_chr *  pself_target;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( pself_target );

    sTmp = vec_to_facing( pchr->pos.x - pself_target->pos.x , pchr->pos.y - pself_target->pos.y );
    sTmp -= pself_target->ori.facing_z;
    returncode = ( sTmp > 55535 || sTmp < 10000 );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_LevelUp( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // IfLevelUp()
    /// @details ZF@> This function proceeds if the character gained a new level this update
    SCRIPT_FUNCTION_BEGIN();

    returncode = HAS_SOME_BITS( pself->alert, ALERTIF_LEVELUP );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 scr_GiveSkillToTarget( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
{
    // GiveSkillToTarget( tmpargument = "skill_IDSZ", tmpdistance = "skill_level" )
    /// @details ZF@> This function permanently gives the target character a skill
    ego_chr *ptarget;
    egoboo_rv rv;

    SCRIPT_FUNCTION_BEGIN();

    SCRIPT_REQUIRE_TARGET( ptarget );

    rv = idsz_map_add( ptarget->skills, SDL_arraysize( ptarget->skills ), pstate->argument, pstate->distance );

    returncode = ( rv_success == rv );

    SCRIPT_FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t _break_passage( int mesh_fx_or, int become, int frames, int starttile, const PASS_REF & passage, int *ptilex, int *ptiley )
{
    /// @details ZZ@> This function breaks the tiles of a passage if there is a character standing
    ///               on 'em.  Turns the tiles into damage terrain if it reaches last frame.

    Uint32 endtile;
    Uint32 fan;
    bool_t useful;

    if ( INVALID_PASSAGE( passage ) ) return bfalse;

    // limit the start tile the the 256 tile images that we have
    starttile = CLIP_TO_08BITS( starttile );

    // same with the end tile
    endtile   =  starttile + frames - 1;
    endtile = CLIP( endtile, 0, 255 );

    useful = bfalse;
    CHR_BEGIN_LOOP_PROCESSING( character, pchr )
    {
        float lerp_z;

        // nothing being held
        if ( IS_ATTACHED_PCHR( pchr ) ) continue;

        // nothing flying
        if ( IS_FLYING_PCHR( pchr ) ) continue;

        lerp_z = ( pchr->pos.z - pchr->enviro.grid_level ) / DAMAGERAISE;
        lerp_z = 1.0f - CLIP( lerp_z, 0.0f, 1.0f );

        if ( pchr->phys.weight * lerp_z <= 20 ) continue;

        fan = ego_mpd::get_tile( PMesh, pchr->pos.x, pchr->pos.y );
        if ( ego_mpd::grid_is_valid( PMesh, fan ) )
        {
            Uint16 img      = PMesh->tmem.tile_list[fan].img & 0x00FF;
            int highbits = PMesh->tmem.tile_list[fan].img & 0xFF00;

            if ( img >= starttile && img < endtile )
            {
                if ( PassageStack_object_is_inside( PASS_REF( passage ), pchr->pos.x, pchr->pos.y, pchr->bump_1.size ) )
                {
                    // Remember where the hit occured.
                    *ptilex = pchr->pos.x;
                    *ptiley = pchr->pos.y;

                    useful = btrue;

                    // Change the tile image
                    img++;
                }
            }

            if ( img == endtile )
            {
                useful = ego_mpd::add_fx( PMesh, fan, mesh_fx_or );

                if ( become != 0 )
                {
                    img = become;
                }
            }

            if ( PMesh->tmem.tile_list[fan].img != ( img | highbits ) )
            {
                ego_mpd::set_texture( PMesh, fan, img | highbits );
            }
        }
    }
    CHR_END_LOOP();

    return useful;
}

//--------------------------------------------------------------------------------------------
Uint8 _append_end_text( ego_chr * pchr, const int message, ego_script_state * pstate )
{
    /// @details ZZ@> This function appends a message to the end-module text

    int read, message_offset;
    CHR_REF ichr;

    FUNCTION_BEGIN();

    if ( !LOADED_PRO( pchr->profile_ref ) ) return bfalse;

    message_offset = ProList.lst[pchr->profile_ref].message_start + message;
    ichr           = GET_REF_PCHR( pchr );

    if ( message_offset < MessageOffset.count )
    {
        char * src, * src_end;
        char * dst, * dst_end;

        // Copy the message_offset
        read = MessageOffset.ary[message_offset];

        src     = message_buffer + read;
        src_end = message_buffer + MESSAGEBUFFERSIZE;

        dst     = endtext + endtext_carat;
        dst_end = endtext + MAXENDTEXT - 1;

        expand_escape_codes( ichr, pstate, src, src_end, dst, dst_end );

        endtext_carat = SDL_strlen( endtext );
    }

    str_add_linebreaks( endtext, SDL_strlen( endtext ), 30 );

    FUNCTION_END();
}

//--------------------------------------------------------------------------------------------
Uint8 _find_grid_in_passage( const int x0, const int y0, const int tiletype, const PASS_REF & passage, int *px1, int *py1 )
{
    /// @details ZZ@> This function finds the next tile in the passage, x0 and y0
    ///    must be set first, and are set on a find.  Returns btrue or bfalse
    ///    depending on if it finds one or not

    int x, y;
    Uint32 fan;
    ego_passage * ppass;

    if ( INVALID_PASSAGE( passage ) ) return bfalse;
    ppass = PassageStack + passage;

    // Do the first row
    x = x0 >> TILE_BITS;
    y = y0 >> TILE_BITS;

    if ( x < ppass->area.xmin )  x = ppass->area.xmin;
    if ( y < ppass->area.ymin )  y = ppass->area.ymin;

    if ( y < ppass->area.ymax )
    {
        for ( /*nothing*/; x <= ppass->area.xmax; x++ )
        {
            fan = ego_mpd::get_tile_int( PMesh, x, y );

            if ( ego_mpd::grid_is_valid( PMesh, fan ) )
            {
                if ( CLIP_TO_08BITS( PMesh->tmem.tile_list[fan].img ) == tiletype )
                {
                    *px1 = ( x << TILE_BITS ) + 64;
                    *py1 = ( y << TILE_BITS ) + 64;
                    return btrue;
                }

            }
        }
        y++;
    }

    // Do all remaining rows
    for ( /* nothing */; y <= ppass->area.ymax; y++ )
    {
        for ( x = ppass->area.xmin; x <= ppass->area.xmax; x++ )
        {
            fan = ego_mpd::get_tile_int( PMesh, x, y );

            if ( ego_mpd::grid_is_valid( PMesh, fan ) )
            {
                if ( CLIP_TO_08BITS( PMesh->tmem.tile_list[fan].img ) == tiletype )
                {
                    *px1 = ( x << TILE_BITS ) + 64;
                    *py1 = ( y << TILE_BITS ) + 64;
                    return btrue;
                }
            }
        }
    }

    return bfalse;
}

//--------------------------------------------------------------------------------------------
Uint8 _display_message( const CHR_REF & ichr, const PRO_REF & iprofile, int message, ego_script_state * pstate )
{
    /// @details ZZ@> This function sticks a message_offset in the display queue and sets its timer

    int slot, read;
    int message_offset;
    Uint8 retval;

    message_offset = ProList.lst[iprofile].message_start + message;

    retval = 0;
    if ( message_offset < MessageOffset.count )
    {
        char * src, * src_end;
        char * dst, * dst_end;

        slot = DisplayMsg_get_free();
        DisplayMsg.ary[slot].time = cfg.message_duration;

        // Copy the message_offset
        read = MessageOffset.ary[message_offset];

        src     = message_buffer + read;
        src_end = message_buffer + MESSAGEBUFFERSIZE;

        dst     = DisplayMsg.ary[slot].textdisplay;
        dst_end = DisplayMsg.ary[slot].textdisplay + MESSAGESIZE - 1;

        expand_escape_codes( ichr, pstate, src, src_end, dst, dst_end );

        *dst_end = CSTR_END;

        retval = 1;
    }

    return retval;
}

//--------------------------------------------------------------------------------------------
// Uint8 scr_get_SkillLevel( ego_script_state * pstate, ego_ai_bundle * pbdl_self )
// {
//    // tmpargument = GetSkillLevel()
//    /// @details ZZ@> This function sets tmpargument to the shield proficiency level of the Target
//   SCRIPT_FUNCTION_BEGIN();
//   pstate->argument = CapStack[pchr->attachedto].shieldproficiency;
//   SCRIPT_FUNCTION_END();
// }
