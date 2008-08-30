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

///
/// @file
/// @brief Script execution
/// @details Implementation of the script execution code

#include "Script_run.h"
#include "Script_compile.h"

#include <assert.h>

#include "Log.h"
#include "Mad.h"
#include "passage.h"
#include "camera.h"
#include "enchant.h"
#include "egoboo.h"
#include "passage.h"
#include "Client.h"
#include "AStar.h"
#include "sound.h"
#include "file_common.h"

#include "egoboo_utility.h"

#include "particle.inl"
#include "object.inl"
#include "input.inl"
#include "char.inl"
#include "mesh.inl"
#include "egoboo_math.inl"
#include "egoboo_types.inl"


static bool_t run_function( Game_t * gs, AI_STATE * pstate, Uint32 value );

static bool_t scr_break_passage( Game_t * gs, AI_STATE * pstate );
static bool_t scr_search_tile_in_passage( Game_t * gs, AI_STATE * pstate );
static bool_t scr_ClearWaypoints( Game_t * gs, AI_STATE * pstate );
static bool_t scr_AddWaypoint( Game_t * gs, AI_STATE * pstate );
static bool_t scr_FindPath( Game_t * gs, AI_STATE * pstate );
static bool_t scr_DoAction( Game_t * gs, AI_STATE * pstate );
static bool_t scr_DropWeapons( Game_t * gs, AI_STATE * pstate );
static bool_t scr_TargetDoAction( Game_t * gs, AI_STATE * pstate );
static bool_t scr_CostTargetItemID( Game_t * gs, AI_STATE * pstate );
static bool_t scr_DoActionOverride( Game_t * gs, AI_STATE * pstate );
static bool_t scr_GiveMoneyToTarget( Game_t * gs, AI_STATE * pstate );
static bool_t scr_SpawnCharacter( Game_t * gs, AI_STATE * pstate );
static bool_t scr_SpawnParticle( Game_t * gs, AI_STATE * pstate );
static bool_t scr_RestockTargetAmmoIDAll( Game_t * gs, AI_STATE * pstate );
static bool_t scr_RestockTargetAmmoIDFirst( Game_t * gs, AI_STATE * pstate );
static bool_t scr_ChildDoActionOverride( Game_t * gs, AI_STATE * pstate );

static retval_t run_operand( Game_t * gs, AI_STATE * pstate, Uint32 value );
static retval_t set_operand(  Game_t * gs, AI_STATE * pstate, Uint8 variable );


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool_t _DoAction( Game_t * gs, Chr_t * pchr, Mad_t * pmad, Uint16 iaction )
{
  bool_t returncode = bfalse;

  if ( iaction < MAXACTION && pchr->action.ready && pmad->actionvalid[iaction] )
  {
    pchr->action.now = (ACTION)iaction;
    pchr->action.ready = bfalse;

    pchr->anim.ilip = 0;
    pchr->anim.flip = 0.0f;
    pchr->anim.last = pchr->anim.next;
    pchr->anim.next = pmad->actionstart[iaction];

    returncode = btrue;
  }

  return returncode;
}

//------------------------------------------------------------------------------
bool_t _DoActionOverride( Game_t * gs, Chr_t * pchr, Mad_t * pmad, Uint16 iaction )
{
  bool_t returncode = bfalse;

  if ( iaction < MAXACTION && pmad->actionvalid[iaction] )
  {
    pchr->action.now = (ACTION)iaction;
    pchr->action.ready = bfalse;

    pchr->anim.ilip    = 0;
    pchr->anim.flip    = 0.0f;
    pchr->anim.next    = pmad->actionstart[iaction];
    pchr->anim.last    = pchr->anim.next;
    returncode = btrue;
  }

  return returncode;
}

//------------------------------------------------------------------------------
bool_t _Teleport( Game_t * gs, CHR_REF ichr, vect3 pos, Uint16 turn_lr )
{
  bool_t returncode = bfalse;

  Chr_t * pchr = ChrList_getPChr( gs, ichr );
  Mesh_t * pmesh = &(gs->Mesh);

  if ( mesh_check( &(pmesh->Info), pos.x, pos.y ) )
  {
    // Yeah!  We hvae a valid location.

    // save the old orientation info in case there is an error
    Orientation_t ori_save = pchr->ori;

    // rip the character off it's mount
    if( detach_character_from_mount( gs, ichr, btrue, bfalse ) )
    {
      pchr->ori_old.pos = pchr->ori.pos;
      ori_save          = pchr->ori;
    };

    pchr->ori.pos.x   = pos.x;
    pchr->ori.pos.y   = pos.y;
    pchr->ori.pos.z   = pos.z;
    pchr->ori.turn_lr = turn_lr;
    if ( 0 != chr_hitawall( gs, pchr, NULL ) )
    {
      // Teleport hits a wall, reset the values
      pchr->ori = ori_save;
      returncode = bfalse;
    }
    else
    {
      // Teleport works completely.
      // Make sure the "safe value" is at the new teleport location.
      pchr->ori_old.pos = pchr->ori.pos;
      returncode = btrue;
    }
  }

  return returncode;
}

//------------------------------------------------------------------------------
//AI Script Routines------------------------------------------------------------
//------------------------------------------------------------------------------

void scr_restart(AI_STATE * pstate, Uint32 offset)
{
  pstate->indent       = 0;
  pstate->lastindent   = 0;
  pstate->operationsum = 0;
  pstate->offset       = offset;
  pstate->active       = btrue;
}


//--------------------------------------------------------------------------------------------
bool_t run_function( Game_t * gs, AI_STATE * pstate, Uint32 value )
{
  /// @details ZZ@> This function runs a script function for the AI.
  ///     It returns bfalse if the script should jump over the
  ///     indented code that follows

  Uint16 sTmp;
  float fTmp;
  int iTmp, tTmp;
  int volume;
  Uint32 test;
  STRING cTmp;
  SearchInfo_t loc_search;
  Uint32 loc_rand;
  ENC_SPAWN_INFO enc_si;

  CHR_REF tmpchr;
  PRT_REF tmpprt;

  PChr_t chrlst      = gs->ChrList;
  size_t chrlst_size = CHRLST_COUNT;

  PPrt_t prtlst      = gs->PrtList;
  //size_t prtlst_size = PRTLST_COUNT;

  PEnc_t enclst      = gs->EncList;
  size_t enclst_size = ENCLST_COUNT;

  Graphics_Data_t * gfx = Game_getGfx( gs );

  Obj_t     * pobj = gs->ObjList + pstate->pself->model;

  CHR_REF loc_aichild = chr_get_aichild( chrlst, chrlst_size, chrlst + pstate->iself );
  Chr_t   * pchild    = chrlst + loc_aichild;

  CHR_REF loc_target = chr_get_aitarget( chrlst, chrlst_size, chrlst + pstate->iself );
  Chr_t   * ptarget  = chrlst + loc_target;

  CHR_REF loc_leader  = team_get_leader( gs, pstate->pself->team );

  //Mad_t  * pmad = ChrList_getPMad(gs, pstate->iself);
  Cap_t  * pcap = ChrList_getPCap(gs, pstate->iself);

  Mesh_t * pmesh = Game_getMesh(gs);

  // Mask out the indentation
  OPCODE opcode = (OPCODE)GET_OPCODE_BITS(value);

  // Assume that the function will pass, as most do
  pstate->returncode = btrue;

  loc_rand = gs->randie_index;

  // Figure out which function to run
  switch ( opcode )
  {
  case F_IfSpawned:
    // Proceed only if it's a new character
    pstate->returncode = HAS_SOME_BITS( pstate->alert, ALERT_SPAWNED );
    break;

  case F_IfTimeOut:
    // Proceed only if time alert is set
    pstate->returncode = ( pstate->time <= 0.0f );
    break;

  case F_IfAtWaypoint:
    // Proceed only if the character reached a waypoint
    pstate->returncode = HAS_SOME_BITS( pstate->alert, ALERT_ATWAYPOINT );
    break;

  case F_IfAtLastWaypoint:
    // Proceed only if the character reached its last waypoint
    pstate->returncode = HAS_SOME_BITS( pstate->alert, ALERT_ATLASTWAYPOINT );

    /// @todo  should this be
    //pstate->returncode = wp_list_empty( &(pstate->wp) );
    /// @todo  then we could separate PUTAWAY from ATLASTWAYPOINT
    break;

  case F_IfAttacked:
    // Proceed only if the character was damaged
    pstate->returncode = HAS_SOME_BITS( pstate->alert, ALERT_ATTACKED );
    break;

  case F_IfBumped:
    // Proceed only if the character was bumped
    pstate->returncode = HAS_SOME_BITS( pstate->alert, ALERT_BUMPED );
    break;

  case F_IfSignaled:
    // Proceed only if the character was GOrder.ed
    pstate->returncode = HAS_SOME_BITS( pstate->alert, ALERT_SIGNALED );
    break;

  case F_IfCalledForHelp:
    // Proceed only if the character was called for help
    pstate->returncode = HAS_SOME_BITS( pstate->alert, ALERT_CALLEDFORHELP );
    break;

  case F_SetContent:
    // Set the content
    pstate->content = pstate->tmpargument;
    break;

  case F_IfKilled:
    // Proceed only if the character's been killed
    pstate->returncode = HAS_SOME_BITS( pstate->alert, ALERT_KILLED );
    break;

  case F_IfTargetKilled:
    // Proceed only if the character's target has just died
    pstate->returncode = HAS_SOME_BITS( pstate->alert, ALERT_TARGETKILLED );
    break;

  case F_ClearWaypoints:
    // Clear out all waypoints
    pstate->returncode = scr_ClearWaypoints(gs,pstate);
    break;

  case F_AddWaypoint:
    // Add a waypoint to the waypoint list
    pstate->returncode = scr_AddWaypoint(gs,pstate);
    break;

  case F_FindPath:
    pstate->returncode = scr_FindPath(gs, pstate);
    break;

  case F_Compass:
    // This function changes tmpx and tmpy in a circlular manner according
    // to tmpturn and tmpdistance
    pstate->tmpx -= turntocos[( pstate->tmpturn>>2 ) & TRIGTABLE_MASK] * pstate->tmpdistance;
    pstate->tmpy -= turntosin[( pstate->tmpturn>>2 ) & TRIGTABLE_MASK] * pstate->tmpdistance;
    break;

  case F_GetTargetArmorPrice:
    // This function gets the armor cost for the given skin
    sTmp = pstate->tmpargument  % MAXSKIN;
    pstate->tmpx = ChrList_getPCap(gs, loc_target)->skin[sTmp].cost;
    break;

  case F_SetTime:
    // This function resets the time
    pstate->time = MAX(1, pstate->tmpargument);
    break;

  case F_GetContent:
    // Get the content
    pstate->tmpargument = pstate->content;
    break;

  case F_JoinTargetTeam:
    // This function allows the character to leave its own team and join another
    pstate->returncode = bfalse;
    if ( EKEY_PVALID(ptarget) )
    {
      switch_team( gs, pstate->iself, ptarget->team );
      pstate->returncode = btrue;
    }
    break;

  case F_SetTargetToNearbyEnemy:
    // This function finds a nearby enemy, and proceeds only if there is one

    pstate->returncode = chr_search_nearby( gs, SearchInfo_new(&loc_search), pstate->iself, bfalse, bfalse, btrue, bfalse, IDSZ_NONE );

    if ( pstate->returncode )
    {
      pstate->target = loc_search.besttarget;
    }

    break;

  case F_SetTargetToTargetLeftHand:
    {
      // This function sets the target to the target's left item
      tmpchr = chr_get_holdingwhich( chrlst, chrlst_size, pstate->target, SLOT_LEFT );

      pstate->returncode = bfalse;
      if ( ACTIVE_CHR( chrlst, tmpchr ) )
      {
        pstate->target = tmpchr;
        pstate->returncode = btrue;
      }
    }
    break;

  case F_SetTargetToTargetRightHand:
    {
      // This function sets the target to the target's right item
      tmpchr = chr_get_holdingwhich( chrlst, chrlst_size, pstate->target, SLOT_RIGHT );
      pstate->returncode = bfalse;
      if ( ACTIVE_CHR( chrlst, tmpchr ) )
      {
        pstate->target = tmpchr;
        pstate->returncode = btrue;
      }
    }
    break;

  case F_SetTargetToWhoeverAttacked:
    {
      // This function sets the target to whoever attacked the character last,
      // failing for damage tiles
      CHR_REF attacklast = chr_get_aiattacklast( chrlst, chrlst_size, chrlst + pstate->iself );
      if ( ACTIVE_CHR( chrlst, attacklast ) )
      {
        pstate->target = attacklast;
      }
      else
      {
        pstate->returncode = bfalse;
      }
    }
    break;

  case F_SetTargetToWhoeverBumped:
    // This function sets the target to whoever bumped into the
    // character last.  It never fails
    pstate->target = chr_get_aibumplast( chrlst, chrlst_size, chrlst + pstate->iself );
    break;

  case F_SetTargetToWhoeverCalledForHelp:
    // This function sets the target to whoever needs help
    pstate->target = team_get_sissy( gs, pstate->pself->team );
    break;

  case F_SetTargetToOldTarget:
    // This function reverts to the target with whom the script started
    pstate->target = pstate->oldtarget;
    break;

  case F_SetTurnModeToVelocity:
    // This function sets the turn mode
    pstate->turnmode = TURNMODE_VELOCITY;
    break;

  case F_SetTurnModeToWatch:
    // This function sets the turn mode
    pstate->turnmode = TURNMODE_WATCH;
    break;

  case F_SetTurnModeToSpin:
    // This function sets the turn mode
    pstate->turnmode = TURNMODE_SPIN;
    break;

  case F_SetBumpHeight:
    // This function changes a character's bump height
    //pstate->pself->bmpdata.height = pstate->tmpargument * pstate->pself->fat;
    //pstate->pself->bmpdata_save.height = pstate->tmpargument;
    break;

  case F_IfTargetHasID:
    // This function proceeds if ID matches tmpargument
    pstate->returncode = bfalse;
    if ( ACTIVE_CHR( chrlst, pstate->target ) )
    {
      pstate->returncode = CAP_INHERIT_IDSZ( gs,  ChrList_getRCap(gs, loc_target), pstate->tmpargument );
    };
    break;

  case F_IfTargetHasItemID:
    // This function proceeds if the target has a matching item in his/her pack
    pstate->returncode = bfalse;
    {
      // Check the pack
      tmpchr  = chr_get_nextinpack( chrlst, chrlst_size, pstate->target );
      while ( ACTIVE_CHR( chrlst, tmpchr ) )
      {
        if ( CAP_INHERIT_IDSZ( gs,  ChrList_getRCap(gs, tmpchr), pstate->tmpargument ) )
        {
          pstate->returncode = btrue;
          break;
        }
        tmpchr  = chr_get_nextinpack( chrlst, chrlst_size, tmpchr );
      }

      for ( _slot = SLOT_BEGIN; _slot < SLOT_COUNT; _slot = ( SLOT )( _slot + 1 ) )
      {
        tmpchr = chr_get_holdingwhich( chrlst, chrlst_size, pstate->target, _slot );
        if ( ACTIVE_CHR( chrlst, tmpchr ) )
        {
          if ( CAP_INHERIT_IDSZ( gs,  ChrList_getRCap(gs, tmpchr), pstate->tmpargument ) )
            pstate->returncode = btrue;
        }
      }
    }
    break;

  case F_IfTargetHoldingItemID:
    // This function proceeds if ID matches tmpargument and returns the latch for the
    // hand in tmpargument
    pstate->returncode = bfalse;

    if ( ACTIVE_CHR( chrlst, pstate->target ) )
    {
      for ( _slot = SLOT_LEFT; _slot <= SLOT_RIGHT; _slot = ( SLOT )( _slot + 1 ) )
      {
        tmpchr = chr_get_holdingwhich( chrlst, chrlst_size, pstate->target, _slot );
        if ( ACTIVE_CHR( chrlst, tmpchr ) && CAP_INHERIT_IDSZ( gs, ChrList_getRCap(gs, tmpchr), pstate->tmpargument ) )
        {
          pstate->tmpargument = slot_to_latch( chrlst, chrlst_size, pstate->target, _slot );
          pstate->returncode = btrue;
        }
      }
    }
    break;

  case F_IfTargetHasSkillID:
    // This function proceeds if ID matches tmpargument
    pstate->returncode = check_skills( gs, pstate->target, pstate->tmpargument );
    break;

  case F_Else:
    // This function fails if the last one was more indented
    pstate->returncode = (pstate->lastindent <= GET_INDENT( value ));
    break;

  case F_Run:
    chr_reset_accel( gs, pstate->iself );
    break;

  case F_Walk:
    chr_reset_accel( gs, pstate->iself );
    pstate->pself->skin.maxaccel *= .66;
    break;

  case F_Sneak:
    chr_reset_accel( gs, pstate->iself );
    pstate->pself->skin.maxaccel *= .33;
    break;

  case F_DoAction:
    // This function starts a new action, if it is valid for the model
    // It will fail if the action is invalid or if the character is doing
    // something else already
    scr_DoAction(gs,pstate);
    break;

  case F_KeepAction:
    // This function makes the current animation halt on the last frame
    pstate->pself->action.keep = btrue;
    break;

  case F_SignalTeam:
    // This function issues an order to all teammates
    signal_team( gs, pstate->iself, pstate->tmpargument );
    break;

  case F_DropWeapons:
    // This funtion drops the character's in hand items/riders
    pstate->returncode = scr_DropWeapons(gs, pstate);
    break;

  case F_TargetDoAction:
    // This function starts a new action, if it is valid for the model
    // It will fail if the action is invalid or if the target is doing
    // something else already
    pstate->returncode = scr_TargetDoAction(gs, pstate);
    break;

  case F_OpenPassage:
    // This function opens the passage specified by tmpargument, failing if the
    // passage was already open
    pstate->returncode = passage_open( gs, pstate->tmpargument );
    break;

  case F_ClosePassage:
    // This function closes the passage specified by tmpargument, and proceeds
    // only if the passage is clear of obstructions
    pstate->returncode = passage_close( gs, pstate->tmpargument );
    break;

  case F_IfPassageOpen:
    // This function proceeds only if the passage specified by tmpargument
    // is both valid and open
    pstate->returncode = bfalse;
    if ( pstate->tmpargument >= 0 && pstate->tmpargument < (Sint32)gs->PassList_count  )
    {
      pstate->returncode = gs->PassList[pstate->tmpargument].open;
    }
    break;

  case F_GoPoof:
    // This function flags the character to be removed from the game
    pstate->returncode = bfalse;
    if ( !chr_is_player(gs, pstate->iself) )
    {
      pstate->returncode = btrue;
      pstate->pself->gopoof = btrue;
    }
    break;

  case F_CostTargetItemID:
    // This function checks if the target has a matching item, and poofs it
    pstate->returncode = scr_CostTargetItemID(gs, pstate);
    break;

  case F_DoActionOverride:
    // This function starts a new action, if it is valid for the model
    // It will fail if the action is invalid
    pstate->returncode = scr_DoActionOverride(gs, pstate);
    break;

  case F_IfHealed:
    // Proceed only if the character was healed
    pstate->returncode = HAS_SOME_BITS( pstate->alert, ALERT_HEALED );
    break;

  case F_DisplayMessage:
    // This function sends a message to the players
    display_message( gs,  pobj->msg_start + pstate->tmpargument, pstate->iself );
    break;

  case F_CallForHelp:
    // This function issues a call for help
    call_for_help( gs,  pstate->iself );
    break;

  case F_AddIDSZ:
    // This function adds an idsz to the module's menu.txt file
    module_add_idsz( gs->mod.loadname, pstate->tmpargument );
    break;

  case F_SetState:
    // This function sets the character's state variable
    pstate->state = pstate->tmpargument;
    break;

  case F_GetState:
    // This function reads the character's state variable
    pstate->tmpargument = pstate->state;
    break;

  case F_IfStateIs:
    // This function fails if the character's state is inequal to tmpargument
    pstate->returncode = ( pstate->tmpargument == pstate->state );
    break;

  case F_IfTargetCanOpenStuff:
    // This function fails if the target can't open stuff
    pstate->returncode = ptarget->prop.canopenstuff;
    break;

  case F_IfGrabbed:
    // Proceed only if the character was picked up
    pstate->returncode = HAS_SOME_BITS( pstate->alert, ALERT_GRABBED );
    break;

  case F_IfDropped:
    // Proceed only if the character was dropped
    pstate->returncode = HAS_SOME_BITS( pstate->alert, ALERT_DROPPED );
    break;

  case F_SetTargetToWhoeverIsHolding:
    // This function sets the target to the character's mount or holder,
    // failing if the character has no mount or holder
    pstate->returncode = bfalse;
    if ( chr_attached( chrlst, chrlst_size, pstate->iself ) )
    {
      pstate->target = chr_get_attachedto( chrlst, chrlst_size, pstate->iself );
      pstate->returncode = btrue;
    }
    break;

  case F_DamageTarget:
    {
      // This function applies little bit of love to the character's target.
      // The amount is set in tmpargument
      PAIR ptemp = {pstate->tmpargument, 1};
      damage_character( gs, pstate->target, 0, &ptemp, pstate->pself->damagetargettype, pstate->pself->team, pstate->iself, DAMFX_BLOC );
    }
    break;

  case F_IfXIsLessThanY:
    // Proceed only if tmpx is less than tmpy
    pstate->returncode = ( pstate->tmpx < pstate->tmpy );
    break;

  case F_SetWeatherTime:
    // Set the weather timer
    gs->Weather.timereset =
      gs->Weather.time      = pstate->tmpargument;
    gs->Weather.active    = (0 != gs->Weather.time);  // time == 0 means weather is off
    break;

  case F_GetBumpHeight:
    // Get the characters bump height
    pstate->tmpargument = pstate->pself->bmpdata.calc_height;
    break;

  case F_IfReaffirmed:
    // Proceed only if the character was reaffirmed
    pstate->returncode = HAS_SOME_BITS( pstate->alert, ALERT_REAFFIRMED );
    break;

  case F_UnkeepAction:
    // This function makes the current animation start again
    pstate->pself->action.keep = bfalse;
    break;

  case F_IfTargetIsOnOtherTeam:
    // This function proceeds only if the target is on another team
    pstate->returncode = ( ptarget->alive && ptarget->team != pstate->pself->team );
    break;

  case F_IfTargetIsOnHatedTeam:
    // This function proceeds only if the target is on an enemy team
    pstate->returncode = ( ptarget->alive && team_is_prey( gs, pstate->pself->team, ptarget->team ) && !ptarget->prop.invictus );
    break;

  case F_PressLatchButton:
    // This function sets the latch buttons
    pstate->latch.b |= pstate->tmpargument;
    break;

  case F_SetTargetToTargetOfLeader:
    // This function sets the character's target to the target of its leader,
    // or it fails with no change if the leader is dead
    pstate->returncode = bfalse;
    if ( ACTIVE_CHR( chrlst, loc_leader ) )
    {
      pstate->target = chr_get_aitarget( chrlst, chrlst_size, chrlst + loc_leader );
      pstate->returncode = btrue;
    }
    break;

  case F_IfLeaderKilled:
    // This function proceeds only if the character's leader has just died
    pstate->returncode = HAS_SOME_BITS( pstate->alert, ALERT_LEADERKILLED );
    break;

  case F_BecomeLeader:
    // This function makes the character the team leader
    gs->TeamList[pstate->pself->team].leader = pstate->iself;
    break;

  case F_ChangeTargetArmor:
    // This function sets the target's armor type and returns the old type
    // as tmpargument and the new type as tmpx
    iTmp = ptarget->skin_ref % MAXSKIN;
    pstate->tmpx = change_armor( gs, pstate->target, pstate->tmpargument );
    pstate->tmpargument = iTmp;  // The character's old armor
    break;

  case F_GiveMoneyToTarget:
    // This function transfers money from the character to the target, and sets
    // tmpargument to the amount transferred
    pstate->returncode = scr_GiveMoneyToTarget(gs, pstate);
    break;

  case F_DropKeys:
    drop_keys( gs, pstate->iself );
    break;

  case F_IfLeaderIsAlive:
    // This function fails if there is no team leader
    pstate->returncode = ACTIVE_CHR( chrlst, loc_leader );
    break;

  case F_IfTargetIsOldTarget:
    // This function returns bfalse if the target has changed
    pstate->returncode = ( pstate->target == pstate->oldtarget );
    break;

  case F_SetTargetToLeader:
    // This function fails if there is no team leader
    if ( !ACTIVE_CHR( chrlst, loc_leader ) )
    {
      pstate->returncode = bfalse;
    }
    else
    {
      pstate->target = loc_leader;
    }
    break;

  case F_SpawnCharacter:
    pstate->returncode = scr_SpawnCharacter(gs, pstate);
    break;

  case F_RespawnCharacter:
    // This function respawns the character at its starting location
    respawn_character( gs, pstate->iself );
    break;

  case F_ChangeTile:
    // This function changes the floor image under the character
    mesh_set_tile( Game_getMesh(gs), MESH_FLOAT_TO_FAN( pstate->pself->ori.pos.x ), MESH_FLOAT_TO_FAN( pstate->pself->ori.pos.y ), pstate->tmpargument & 0xFF );
    break;

  case F_IfUsed:
    // This function proceeds only if the character has been used
    pstate->returncode = HAS_SOME_BITS( pstate->alert, ALERT_USED );
    break;

  case F_DropMoney:
    // This function drops some of a character's money
    drop_money( gs, pstate->iself, pstate->tmpargument );
    break;

  case F_SetOldTarget:
    // This function sets the old target to the current target
    pstate->oldtarget = chr_get_aitarget( chrlst, chrlst_size, chrlst + pstate->iself );
    break;

  case F_DetachFromHolder:
    // This function drops the character, failing only if it was not held
    pstate->returncode = detach_character_from_mount( gs, pstate->iself, btrue, btrue );
    break;

  case F_IfTargetHasVulnerabilityID:
    // This function proceeds if ID matches tmpargument
    pstate->returncode = ( ChrList_getPCap(gs, loc_target)->idsz[IDSZ_VULNERABILITY] == ( IDSZ ) pstate->tmpargument );
    break;

  case F_CleanUp:
    // This function issues the clean up order to all teammates
    issue_clean( gs, pstate->iself );
    break;

  case F_IfCleanedUp:
    // This function proceeds only if the character was told to clean up
    pstate->returncode = HAS_SOME_BITS( pstate->alert, ALERT_CLEANEDUP );
    break;

  case F_IfSitting:
    // This function proceeds if the character is riding another
    pstate->returncode = bfalse;
    {
      tmpchr = chr_get_attachedto( chrlst, chrlst_size, pstate->iself );
      if( ACTIVE_CHR( chrlst, tmpchr ) )
      {
        pstate->returncode = (pstate->iself == chr_get_holdingwhich(chrlst, chrlst_size, tmpchr, SLOT_SADDLE));
      };
    }
    break;

  case F_IfTargetIsHurt:
    // This function passes only if the target is hurt and alive
    pstate->returncode = bfalse;
    if ( ACTIVE_CHR( chrlst, pstate->target ) )
    {
      pstate->returncode = ptarget->alive &&  ptarget->stats.lifemax_fp8 > 0 && ptarget->stats.life_fp8 < ptarget->stats.lifemax_fp8 - DAMAGE_HURT;
    }
    break;

  case F_IfTargetIsAPlayer:
    // This function proceeds only if the target is a player ( may not be local )
    pstate->returncode = chr_is_player(gs, pstate->target);
    break;

  case F_PlaySound:
    // This function plays a sound
    pstate->returncode = bfalse;
    if ( INVALID_SOUND != pstate->tmpargument && pstate->pself->ori_old.pos.z > PITNOSOUND )
    {
      pstate->returncode = ( INVALID_SOUND != snd_play_sound( gs, 1.0f, pstate->pself->ori_old.pos, pobj->wavelist[pstate->tmpargument], 0, pstate->pself->model, pstate->tmpargument) );
    }
    break;

  case F_SpawnParticle:
    // This function spawns a particle
    pstate->returncode = scr_SpawnParticle(gs, pstate);
    break;

  case F_IfTargetIsAlive:
    // This function proceeds only if the target is alive
    pstate->returncode = ptarget->alive;
    break;

  case F_Stop:
    pstate->pself->skin.maxaccel = 0;
    break;

  case F_DisaffirmCharacter:
    disaffirm_attached_particles( gs, pstate->iself );
    break;

  case F_ReaffirmCharacter:
    reaffirm_attached_particles( gs, pstate->iself );
    break;

  case F_IfTargetIsSelf:
    // This function proceeds only if the target is the character too
    pstate->returncode = ( pstate->target == pstate->iself );
    break;

  case F_IfTargetIsMale:
    // This function proceeds only if the target is male
    pstate->returncode = ( pstate->pself->gender == GEN_MALE );
    break;

  case F_IfTargetIsFemale:
    // This function proceeds only if the target is female
    pstate->returncode = ( pstate->pself->gender == GEN_FEMALE );
    break;

  case F_SetTargetToSelf:
    // This function sets the target to the character
    pstate->target = pstate->iself;
    break;

  case F_SetTargetToRider:
    // This function sets the target to the character's left/only grip weapon,
    // failing if there is none
    pstate->returncode = bfalse;
    if ( chr_using_slot( chrlst, chrlst_size, pstate->iself, SLOT_SADDLE ) )
    {
      pstate->target = chr_get_holdingwhich( chrlst, chrlst_size, pstate->iself, SLOT_SADDLE );
      pstate->returncode = btrue;
    }
    break;

  case F_GetAttackTurn:
    // This function sets tmpturn to the direction of the last attack
    pstate->tmpturn = pstate->directionlast;
    break;

  case F_GetDamageType:
    // This function gets the last type of damage
    pstate->tmpargument = pstate->damagetypelast;
    break;

  case F_BecomeSpell:
    // This function turns the spellbook character into a spell based on its
    // content
    pstate->pself->money = pstate->pself->skin_ref % MAXSKIN;
    change_character( gs, pstate->iself, OBJ_REF(pstate->content), 0, LEAVE_NONE );
    pstate->content = 0;  // Reset so it doesn't mess up
    pstate->state   = 0;  // Reset so it doesn't mess up
    pstate->morphed = btrue;
    break;

  case F_BecomeSpellbook:
    // This function turns the spell into a spellbook, and sets the content
    // accordingly
    pstate->content = REF_TO_INT(pstate->pself->model);
    change_character( gs, pstate->iself, OBJ_REF(SPELLBOOK), pstate->pself->money % MAXSKIN, LEAVE_NONE );
    pstate->state = 0;  // Reset so it doesn't burn up
    pstate->morphed = btrue;
    break;

  case F_IfScoredAHit:
    // Proceed only if the character scored a hit
    pstate->returncode = HAS_SOME_BITS( pstate->alert, ALERT_SCOREDAHIT );
    break;

  case F_IfDisaffirmed:
    // Proceed only if the character was disaffirmed
    pstate->returncode = HAS_SOME_BITS( pstate->alert, ALERT_DISAFFIRMED );
    break;

  case F_DecodeOrder:
    // This function gets the order and sets tmpx, tmpy, tmpargument and the
    // target ( if valid )

    pstate->returncode = bfalse;
    {
      tmpchr = CHR_REF(pstate->pself->message.data >> 20);
      if ( ACTIVE_CHR( chrlst, tmpchr ) )
      {
        pstate->target      = tmpchr;
        pstate->tmpx        = (( pstate->pself->message.data >> 12 ) & 0x00FF ) << 8;
        pstate->tmpy        = (( pstate->pself->message.data >>  4 ) & 0x00FF ) << 8;
        pstate->tmpargument = pstate->pself->message.data & 0x000F;
        pstate->returncode = btrue;
      }
    }
    break;

  case F_SetTargetToWhoeverWasHit:
    // This function sets the target to whoever the character hit last,
    pstate->target = chr_get_aihitlast( chrlst, chrlst_size, chrlst + pstate->iself );
    break;

  case F_SetTargetToWideEnemy:
    // This function finds an enemy, and proceeds only if there is one

    pstate->returncode = chr_search_wide( gs, SearchInfo_new(&loc_search), pstate->iself, bfalse, bfalse, btrue, bfalse, IDSZ_NONE, bfalse );

    if ( pstate->returncode )
    {
      pstate->target = loc_search.besttarget;
    }
    break;

  case F_IfChanged:
    // Proceed only if the character was polymorphed
    pstate->returncode = HAS_SOME_BITS( pstate->alert, ALERT_CHANGED );
    break;

  case F_IfInWater:
    // Proceed only if the character got wet
    pstate->returncode = HAS_SOME_BITS( pstate->alert, ALERT_INWATER );
    break;

  case F_IfBored:
    // Proceed only if the character is bored
    pstate->returncode = HAS_SOME_BITS( pstate->alert, ALERT_BORED );
    break;

  case F_IfTooMuchBaggage:
    // Proceed only if the character tried to grab too much
    pstate->returncode = HAS_SOME_BITS( pstate->alert, ALERT_TOOMUCHBAGGAGE );
    break;

  case F_IfGrogged:
    // Proceed only if the character was grogged
    pstate->returncode = HAS_SOME_BITS( pstate->alert, ALERT_GROGGED );
    break;

  case F_IfDazed:
    // Proceed only if the character was dazed
    pstate->returncode = HAS_SOME_BITS( pstate->alert, ALERT_DAZED );
    break;

  case F_IfTargetHasSpecialID:
    // This function proceeds if ID matches tmpargument
    pstate->returncode = ( ChrList_getPCap(gs, loc_target)->idsz[IDSZ_SPECIAL] == ( IDSZ ) pstate->tmpargument );
    break;

  case F_PressTargetLatchButton:
    // This function sets the target's latch buttons
    ptarget->aistate.latch.b |= pstate->tmpargument;
    break;

  case F_IfInvisible:
    // This function passes if the character is invisible
    pstate->returncode = chr_is_invisible( chrlst, chrlst_size, pstate->iself );
    break;

  case F_IfArmorIs:
    // This function passes if the character's skin is tmpargument
    tTmp = pstate->pself->skin_ref % MAXSKIN;
    pstate->returncode = ( tTmp == pstate->tmpargument );
    break;

  case F_GetTargetGrogTime:
    // This function returns tmpargument as the grog time, and passes if it is not 0
    pstate->tmpargument = pstate->pself->grogtime;
    pstate->returncode = ( pstate->tmpargument != 0 );
    break;

  case F_GetTargetDazeTime:
    // This function returns tmpargument as the daze time, and passes if it is not 0
    pstate->tmpargument = pstate->pself->dazetime;
    pstate->returncode = ( pstate->tmpargument != 0 );
    break;

  case F_SetDamageType:
    // This function sets the bump damage type
    pstate->pself->damagetargettype = (DAMAGE)(pstate->tmpargument % MAXDAMAGETYPE);
    break;

  case F_SetWaterLevel:
    // This function raises and lowers the module's water
    fTmp = ( pstate->tmpargument / 10.0 ) - gfx->Water.douselevel;
    gfx->Water.surfacelevel += fTmp;
    gfx->Water.douselevel += fTmp;
    for ( iTmp = 0; iTmp < MAXWATERLAYER; iTmp++ )
      gfx->Water.layer[iTmp].z += fTmp;
    break;

  case F_EnchantTarget:
    // This function enchants the target
    {
      ENC_REF tmpenc;
      enc_spawn_info_init( &enc_si, gs, pstate->owner, pstate->target, pstate->iself, INVALID_ENC, INVALID_OBJ );
      tmpenc = req_spawn_one_enchant( enc_si );
      pstate->returncode = ( INVALID_ENC != tmpenc );
    }
    break;

  case F_EnchantChild:
    // This function can be used with SpawnCharacter to enchant the
    // newly spawned character
    {
      ENC_REF tmpenc;
      enc_spawn_info_init( &enc_si, gs, pstate->owner, pstate->child, pstate->iself, INVALID_ENC, INVALID_OBJ );
      tmpenc = req_spawn_one_enchant( enc_si );
      pstate->returncode = ( INVALID_ENC != tmpenc );
    }
    break;

  case F_TeleportTarget:
    // This function teleports the target to the X, Y location, failing if the
    // location is off the map or blocked. Z position is defined in tmpdistance
    {
      vect3 pos = VECT3(pstate->tmpx, pstate->tmpy, pstate->tmpdistance);
      pstate->returncode = _Teleport(gs, pstate->target, pos, pstate->tmpturn);
    }
    break;

  case F_GiveExperienceToTarget:
    // This function gives the target some experience, xptype from distance,
    // amount from argument...
    give_experience( gs, pstate->target, pstate->tmpargument, (EXPERIENCE)pstate->tmpdistance );
    break;

  case F_IncreaseAmmo:
    // This function increases the ammo by one
    if ( pstate->pself->ammo < pstate->pself->ammomax )
    {
      pstate->pself->ammo++;
    }
    break;

  case F_UnkurseTarget:
    // This function unkurses the target
    ptarget->prop.iskursed = bfalse;
    break;

  case F_GiveExperienceToTargetTeam:
    // This function gives experience to everyone on the target's team
    give_team_experience( gs, ptarget->team, pstate->tmpargument, (EXPERIENCE)pstate->tmpdistance );
    break;

  case F_IfUnarmed:
    // This function proceeds if the character has no item in hand
    pstate->returncode = !chr_using_slot( chrlst, chrlst_size, pstate->iself, SLOT_LEFT ) && !chr_using_slot( chrlst, chrlst_size, pstate->iself, SLOT_RIGHT );
    break;

  case F_RestockTargetAmmoIDAll:
    // This function restocks the ammo of every item the character is holding,
    // if the item matches the ID given ( parent or child type )
    pstate->returncode = scr_RestockTargetAmmoIDAll(gs, pstate);
    break;

  case F_RestockTargetAmmoIDFirst:
    // This function restocks the ammo of the first item the character is holding,
    // if the item matches the ID given ( parent or child type )

    pstate->returncode = scr_RestockTargetAmmoIDFirst(gs, pstate);
    break;

  case F_FlashTarget:
    // This function flashes the character
    flash_character( gs, pstate->target, 255 );
    break;

  case F_SetRedShift:
    // This function alters a character's coloration
    pstate->pself->redshift = pstate->tmpargument;
    break;

  case F_SetGreenShift:
    // This function alters a character's coloration
    pstate->pself->grnshift = pstate->tmpargument;
    break;

  case F_SetBlueShift:
    // This function alters a character's coloration
    pstate->pself->blushift = pstate->tmpargument;
    break;

  case F_SetLight:
    // This function alters a character's transparency
    pstate->pself->light_fp8 = pstate->tmpargument;
    break;

  case F_SetAlpha:
    // This function alters a character's transparency
    pstate->pself->alpha_fp8    = pstate->tmpargument;
    pstate->pself->bumpstrength = pcap->bumpstrength * FP8_TO_FLOAT( pstate->tmpargument );
    break;

  case F_IfHitFromBehind:
    // This function proceeds if the character was attacked from behind
    pstate->returncode = bfalse;
    if ( pstate->directionlast >= BEHIND - 8192 && pstate->directionlast < BEHIND + 8192 )
      pstate->returncode = btrue;
    break;

  case F_IfHitFromFront:
    // This function proceeds if the character was attacked from the front
    pstate->returncode = bfalse;
    if ( pstate->directionlast >= 49152 + 8192 || pstate->directionlast < FRONT + 8192 )
      pstate->returncode = btrue;
    break;

  case F_IfHitFromLeft:
    // This function proceeds if the character was attacked from the left
    pstate->returncode = bfalse;
    if ( pstate->directionlast >= LEFT - 8192 && pstate->directionlast < LEFT + 8192 )
      pstate->returncode = btrue;
    break;

  case F_IfHitFromRight:
    // This function proceeds if the character was attacked from the right
    pstate->returncode = bfalse;
    if ( pstate->directionlast >= RIGHT - 8192 && pstate->directionlast < RIGHT + 8192 )
      pstate->returncode = btrue;
    break;

  case F_IfTargetIsOnSameTeam:
    // This function proceeds only if the target is on another team
    pstate->returncode = bfalse;
    if ( ptarget->team == pstate->pself->team )
      pstate->returncode = btrue;
    break;

  case F_KillTarget:
    // This function kills the target
    kill_character( gs, pstate->target, pstate->iself );
    break;

  case F_UndoEnchant:
    // This function undoes the last enchant
    pstate->returncode = ( INVALID_ENC != pstate->pself->undoenchant );
    remove_enchant( gs, pstate->pself->undoenchant );
    break;

  case F_GetWaterLevel:
    // This function gets the douse level for the water, returning it in tmpargument
    pstate->tmpargument = gfx->Water.douselevel * 10;
    break;

  case F_CostTargetMana:
    // This function costs the target some mana
    pstate->returncode = cost_mana( gs, pstate->target, pstate->tmpargument, pstate->iself );
    break;

  case F_IfTargetHasAnyID:
    // This function proceeds only if one of the target's IDSZ's matches tmpargument
    pstate->returncode = bfalse;
    for ( tTmp = 0; tTmp < IDSZ_COUNT; tTmp++ )
    {
      Cap_t * trg_cap = ChrList_getPCap(gs, loc_target);
      if ( NULL != trg_cap && trg_cap->idsz[tTmp] == ( IDSZ ) pstate->tmpargument )
      {
        pstate->returncode = btrue;
        break;
      }
    }
    break;

  case F_SetBumpSize:
    // This function sets the character's bump size
    //fTmp = pstate->pself->bmpdata.calc_size_big;
    //fTmp /= pstate->pself->bmpdata.calc_size;  // 1.5 or 2.0
    //pstate->pself->bmpdata.size = pstate->tmpargument * pstate->pself->fat;
    //pstate->pself->bmpdata.sizebig = fTmp * pstate->pself->bmpdata.calc_size;
    //pstate->pself->bmpdata_save.size = pstate->tmpargument;
    //pstate->pself->bmpdata_save.sizebig = fTmp * pstate->pself->bmpdata_save.size;
    break;

  case F_IfNotDropped:
    // This function passes if a kursed item could not be dropped
    pstate->returncode = HAS_SOME_BITS( pstate->alert, ALERT_NOTDROPPED );
    break;

  case F_IfYIsLessThanX:
    // This function passes only if tmpy is less than tmpx
    pstate->returncode = ( pstate->tmpy < pstate->tmpx );
    break;

  case F_SetFlyHeight:
    // This function sets a character's fly height
    pstate->pself->flyheight = pstate->tmpargument;
    break;

  case F_IfBlocked:
    // This function passes if the character blocked an attack
    pstate->returncode = HAS_SOME_BITS( pstate->alert, ALERT_BLOCKED );
    break;

  case F_IfTargetIsDefending:
    pstate->returncode = ( ptarget->action.now >= ACTION_PA && ptarget->action.now <= ACTION_PD );
    break;

  case F_IfTargetIsAttacking:
    pstate->returncode = ( ptarget->action.now >= ACTION_UA && ptarget->action.now <= ACTION_FD );
    break;

  case F_IfStateIs0:
    pstate->returncode = ( 0 == pstate->state );
    break;

  case F_IfStateIs1:
    pstate->returncode = ( 1 == pstate->state );
    break;

  case F_IfStateIs2:
    pstate->returncode = ( 2 == pstate->state );
    break;

  case F_IfStateIs3:
    pstate->returncode = ( 3 == pstate->state );
    break;

  case F_IfStateIs4:
    pstate->returncode = ( 4 == pstate->state );
    break;

  case F_IfStateIs5:
    pstate->returncode = ( 5 == pstate->state );
    break;

  case F_IfStateIs6:
    pstate->returncode = ( 6 == pstate->state );
    break;

  case F_IfStateIs7:
    pstate->returncode = ( 7 == pstate->state );
    break;

  case F_IfContentIs:
    pstate->returncode = ( pstate->tmpargument == pstate->content );
    break;

  case F_SetTurnModeToWatchTarget:
    // This function sets the turn mode
    pstate->turnmode = TURNMODE_WATCHTARGET;
    break;

  case F_IfStateIsNot:
    pstate->returncode = ( pstate->tmpargument != pstate->state );
    break;

  case F_IfXIsEqualToY:
    pstate->returncode = ( pstate->tmpx == pstate->tmpy );
    break;

  case F_DisplayDebugMessage:
    // This function spits out a debug message
    if( CData.DevMode )
    {
      debug_message( 1, "aistate %d, aicontent %d, target %d", pstate->state, pstate->content, pstate->target );
      debug_message( 1, "tmpx %d, tmpy %d", pstate->tmpx, pstate->tmpy );
      debug_message( 1, "tmpdistance %d, tmpturn %d", pstate->tmpdistance, pstate->tmpturn );
      debug_message( 1, "tmpargument %d, selfturn %d", pstate->tmpargument, pstate->pself->ori.turn_lr );
    }
    break;

  case F_BlackTarget:
    // This function makes the target flash black
    flash_character( gs, pstate->target, 0 );
    break;

  case F_DisplayMessageNear:
    // This function sends a message if the camera is in the nearby area.
    iTmp = ABS( pstate->pself->ori_old.pos.x - GCamera.trackpos.x ) + ABS( pstate->pself->ori_old.pos.y - GCamera.trackpos.y );
    if ( iTmp < MSGDISTANCE )
    {
      display_message( gs, pobj->msg_start + pstate->tmpargument, pstate->iself );
    }
    break;

  case F_IfHitGround:
    // This function passes if the character just hit the ground
    pstate->returncode = HAS_SOME_BITS( pstate->alert, ALERT_HITGROUND );
    break;

  case F_IfNameIsKnown:
    // This function passes if the character's name is known
    pstate->returncode = pstate->pself->prop.nameknown;
    break;

  case F_IfUsageIsKnown:
    // This function passes if the character's usage is known
    pstate->returncode = pcap->prop.usageknown;
    break;

  case F_IfHoldingItemID:
    // This function passes if the character is holding an item with the IDSZ given
    // in tmpargument, returning the latch to press to use it
    pstate->returncode = bfalse;

    for ( _slot = SLOT_LEFT; _slot <= SLOT_RIGHT; _slot = ( SLOT )( _slot + 1 ) )
    {
      tmpchr = chr_get_holdingwhich( chrlst, chrlst_size, pstate->iself, _slot );
      if ( ACTIVE_CHR( chrlst, tmpchr ) )
      {
        if ( CAP_INHERIT_IDSZ( gs, ChrList_getRCap(gs, tmpchr), pstate->tmpargument ) )
        {
          pstate->tmpargument = slot_to_latch( chrlst, chrlst_size, pstate->iself, _slot );
          pstate->returncode = btrue;
        }
      }
    }
    break;

  case F_IfHoldingRangedWeapon:
    // This function passes if the character is holding a ranged weapon, returning
    // the latch to press to use it.  This also checks ammo/ammoknown.
    pstate->returncode = bfalse;
    pstate->tmpargument = 0;

    for ( _slot = SLOT_LEFT; _slot <= SLOT_RIGHT; _slot = ( SLOT )( _slot + 1 ) )
    {
      tmpchr = chr_get_holdingwhich( chrlst, chrlst_size, pstate->iself, _slot );
      if ( ACTIVE_CHR( chrlst, tmpchr ) )
      {
        if ( chrlst[tmpchr].prop.isranged && ( chrlst[tmpchr].ammomax == 0 || ( chrlst[tmpchr].ammo > 0 && chrlst[tmpchr].ammoknown ) ) )
        {
          pstate->tmpargument = slot_to_latch( chrlst, chrlst_size, pstate->iself, _slot );
          pstate->returncode = btrue;
        }
      }
    };
    break;

  case F_IfHoldingMeleeWeapon:
    // This function passes if the character is holding a melee weapon, returning
    // the latch to press to use it
    pstate->returncode = bfalse;
    pstate->tmpargument = 0;

    for ( _slot = SLOT_LEFT; _slot <= SLOT_RIGHT; _slot = ( SLOT )( _slot + 1 ) )
    {
      tmpchr = chr_get_holdingwhich( chrlst, chrlst_size, pstate->iself, _slot );
      if ( ACTIVE_CHR( chrlst, tmpchr ) )
      {
        if ( !chrlst[tmpchr].prop.isranged && ChrList_getPCap(gs, tmpchr)->weaponaction != ACTION_PA )
        {
          pstate->tmpargument = slot_to_latch( chrlst, chrlst_size, pstate->iself, _slot );
          pstate->returncode = btrue;
        }
      }
    };
    break;

  case F_IfHoldingShield:
    // This function passes if the character is holding a shield, returning the
    // latch to press to use it
    pstate->returncode = bfalse;
    pstate->tmpargument = 0;

    for ( _slot = SLOT_LEFT; _slot <= SLOT_RIGHT; _slot = ( SLOT )( _slot + 1 ) )
    {
      tmpchr = chr_get_holdingwhich( chrlst, chrlst_size, pstate->iself, _slot );
      if ( ACTIVE_CHR( chrlst, tmpchr ) && ChrList_getPCap(gs, tmpchr)->weaponaction == ACTION_PA )
      {
        pstate->tmpargument = slot_to_latch( chrlst, chrlst_size, pstate->iself, _slot );
        pstate->returncode = btrue;
      }
    };
    break;

  case F_IfKursed:
    // This function passes if the character is kursed
    pstate->returncode = pstate->pself->prop.iskursed;
    break;

  case F_IfTargetIsKursed:
    // This function passes if the target is kursed
    pstate->returncode = ptarget->prop.iskursed;
    break;

  case F_IfTargetIsDressedUp:
    // This function passes if the character's skin is dressy
    iTmp = pstate->pself->skin_ref % MAXSKIN;
    iTmp = 1 << iTmp;
    pstate->returncode = (( pcap->skindressy & iTmp ) != 0 );
    break;

  case F_IfOverWater:
    // This function passes if the character is on a water tile
    pstate->returncode = mesh_has_some_bits( pmesh->Mem.tilelst, pstate->pself->onwhichfan, MPDFX_WATER ) && gfx->Water.iswater;
    break;

  case F_IfThrown:
    // This function passes if the character was thrown
    pstate->returncode = HAS_SOME_BITS( pstate->alert, ALERT_THROWN );
    break;

  case F_MakeNameKnown:
    // This function makes the name of an item/character known.
    pstate->pself->prop.nameknown = btrue;
    pstate->pself->prop.icon = btrue;
    break;

  case F_MakeUsageKnown:
    // This function makes the usage of an item known...  For XP gains from
    // using an unknown potion or such
    pcap->prop.usageknown = btrue;
    break;

  case F_StopTargetMovement:
    // This function makes the target stop moving temporarily
    ptarget->ori.vel.x = 0;
    ptarget->ori.vel.y = 0;
    if ( ptarget->ori.vel.z > 0 ) ptarget->ori.vel.z = gs->phys.gravity;
    break;

  case F_SetXY:
    // This function stores tmpx and tmpy in the storage array
    pstate->x[pstate->tmpargument & STOR_AND] = pstate->tmpx;
    pstate->y[pstate->tmpargument & STOR_AND] = pstate->tmpy;
    break;

  case F_GetXY:
    // This function gets previously stored data, setting tmpx and tmpy
    pstate->tmpx = pstate->x[pstate->tmpargument & STOR_AND];
    pstate->tmpy = pstate->y[pstate->tmpargument & STOR_AND];
    break;

  case F_AddXY:
    // This function adds tmpx and tmpy to the storage array
    pstate->x[pstate->tmpargument & STOR_AND] += pstate->tmpx;
    pstate->y[pstate->tmpargument & STOR_AND] += pstate->tmpy;
    break;

  case F_MakeAmmoKnown:
    // This function makes the ammo of an item/character known.
    pstate->pself->ammoknown = btrue;
    break;

  case F_SpawnAttachedParticle:
    // This function spawns an attached particle
    pstate->returncode = bfalse;
    if ( ACTIVE_CHR( chrlst, pstate->iself ) && !chr_in_pack( chrlst, chrlst_size, pstate->iself ) )
    {
      tmpchr = pstate->iself;
      if ( chr_attached( chrlst, chrlst_size, pstate->iself ) )  tmpchr = chr_get_attachedto( chrlst, chrlst_size, pstate->iself );
      tmpprt = prt_spawn( gs, 1.0f, pstate->pself->ori.pos, pstate->pself->ori.vel, pstate->pself->ori.turn_lr, pstate->pself->model, PIP_REF(pstate->tmpargument), pstate->iself, (GRIP)pstate->tmpdistance, pstate->pself->team, tmpchr, 0, INVALID_CHR );
      pstate->returncode = RESERVED_PRT( prtlst, tmpprt );
    }
    break;

  case F_SpawnExactParticle:
    // This function spawns an exactly placed particle
    pstate->returncode = bfalse;
    if ( ACTIVE_CHR( chrlst, pstate->iself ) && !chr_in_pack( chrlst, chrlst_size, pstate->iself ) )
    {
      vect3 prt_pos = VECT3(pstate->tmpx, pstate->tmpy, pstate->tmpdistance);
      vect3 prt_vel = ZERO_VECT3;

      tmpchr = pstate->iself;
      if ( chr_attached( chrlst, chrlst_size, tmpchr ) )  tmpchr = chr_get_attachedto( chrlst, chrlst_size, tmpchr );

      tmpprt = prt_spawn( gs, 1.0f, prt_pos, prt_vel, pstate->pself->ori.turn_lr, pstate->pself->model, PIP_REF(pstate->tmpargument), INVALID_CHR, (GRIP)0, pstate->pself->team, tmpchr, 0, INVALID_CHR );
      pstate->returncode = RESERVED_PRT( prtlst, tmpprt );
    };
    break;

  case F_AccelerateTarget:
    // This function changes the target's speeds
    ptarget->ori.vel.x += pstate->tmpx;
    ptarget->ori.vel.y += pstate->tmpy;
    break;

  case F_IfDistanceIsMoreThanTurn:
    // This function proceeds tmpdistance is greater than tmpturn
    pstate->returncode = ( pstate->tmpdistance > ( int ) pstate->tmpturn );
    break;

  case F_IfCrushed:
    // This function proceeds only if the character was crushed
    pstate->returncode = HAS_SOME_BITS( pstate->alert, ALERT_CRUSHED );
    break;

  case F_MakeCrushValid:
    // This function makes doors able to close on this object
    pstate->pself->prop.canbecrushed = btrue;
    break;

  case F_SetTargetToLowestTarget:
    // This sets the target to whatever the target is being held by,
    // The lowest in the set.  This function never fails
    if ( ACTIVE_CHR( chrlst, pstate->target ) )
    {
      CHR_REF holder   = chr_get_attachedto(chrlst, chrlst_size, pstate->target);
      CHR_REF attached = chr_get_attachedto(chrlst, chrlst_size, holder);
      while ( ACTIVE_CHR(chrlst, attached) )
      {
        holder = attached;
        attached = chr_get_attachedto(chrlst, chrlst_size, holder);
      }
      pstate->target = holder;
    };
    break;

  case F_IfNotPutAway:
    // This function proceeds only if the character couln't be put in the pack
    pstate->returncode = HAS_SOME_BITS( pstate->alert, ALERT_NOTPUTAWAY );
    break;

  case F_IfTakenOut:
    // This function proceeds only if the character was taken out of the pack
    pstate->returncode = HAS_SOME_BITS( pstate->alert, ALERT_TAKENOUT );
    break;

  case F_IfAmmoOut:
    // This function proceeds only if the character has no ammo
    pstate->returncode = ( pstate->pself->ammo == 0 );
    break;

  case F_PlaySoundLooped:
    // This function plays a looped sound
    pstate->returncode = bfalse;
    if ( !gs->modstate.Paused && (0 <= pstate->tmpargument) && (pstate->pself->ori_old.pos.z > PITNOSOUND) && (INVALID_SOUND == pstate->pself->loopingchannel))
    {
      //You could use this, but right now there's no way to stop the sound later, so it's better not to start it
      pstate->pself->loopingchannel = snd_play_sound( gs, 1.0f, pstate->pself->ori.pos, pobj->wavelist[pstate->tmpargument], 0, pstate->pself->model, pstate->tmpargument);
      pstate->pself->loopingvolume = 1.0f;
    }
    break;

  case F_StopSoundLoop:
    // This function stops playing a sound
    if( INVALID_CHANNEL != pstate->pself->loopingchannel )
    {
      snd_stop_sound( pstate->pself->loopingchannel );
      pstate->pself->loopingchannel = INVALID_CHANNEL;
    };
    break;

  case F_HealSelf:
    // This function heals the character, without setting the alert or modifying
    // the amount
    if ( pstate->pself->alive )
    {
      iTmp = pstate->pself->stats.life_fp8 + pstate->tmpargument;
      if ( iTmp > pstate->pself->stats.lifemax_fp8 ) iTmp = pstate->pself->stats.lifemax_fp8;
      if ( iTmp < 1 ) iTmp = 1;
      pstate->pself->stats.life_fp8 = iTmp;
    }
    break;

  case F_Equip:
    // This function flags the character as being equipped
    pstate->pself->isequipped = btrue;
    break;

  case F_IfTargetHasItemIDEquipped:
    // This function proceeds if the target has a matching item equipped
    pstate->returncode = bfalse;
    {
      tmpchr  = chr_get_nextinpack( chrlst, chrlst_size, pstate->target );
      while ( ACTIVE_CHR( chrlst, tmpchr ) )
      {
        if ( tmpchr != pstate->iself && chrlst[tmpchr].isequipped && CAP_INHERIT_IDSZ( gs,  ChrList_getRCap(gs, tmpchr), pstate->tmpargument ) )
        {
          pstate->returncode = btrue;
          break;
        }
        else
        {
          tmpchr  = chr_get_nextinpack( chrlst, chrlst_size, tmpchr );
        }
      }
    }
    break;

  case F_SetOwnerToTarget:
    // This function sets the owner
    pstate->owner = pstate->target;
    break;

  case F_SetTargetToOwner:
    // This function sets the target to the owner
    pstate->target = pstate->owner;
    break;

  case F_SetFrame:
    // This function sets the character's current frame
    sTmp = pstate->tmpargument & 3;
    iTmp = pstate->tmpargument >> 2;
    set_frame( gs, pstate->iself, iTmp, sTmp );
    break;

  case F_BreakPassage:
    // This function makes the tiles fall away ( turns into damage terrain )
    pstate->returncode = scr_break_passage( gs, pstate );
    break;

  case F_SetReloadTime:
    // This function makes weapons fire slower
    pstate->pself->reloadtime = pstate->tmpargument;
    break;

  case F_SetTargetToWideBlahID:
    // This function sets the target based on the settings of tmpargument and tmpdistance

    pstate->returncode = chr_search_wide( gs, SearchInfo_new(&loc_search), pstate->iself,
      HAS_SOME_BITS( pstate->tmpdistance, SEARCH_ITEMS   ),
      HAS_SOME_BITS( pstate->tmpdistance, SEARCH_FRIENDS ),
      HAS_SOME_BITS( pstate->tmpdistance, SEARCH_ENEMIES ),
      HAS_SOME_BITS( pstate->tmpdistance, SEARCH_DEAD    ),
      pstate->tmpargument,
      HAS_SOME_BITS( pstate->tmpdistance, SEARCH_INVERT  ) );

    if ( pstate->returncode )
    {
      pstate->target = loc_search.besttarget;
    }
    break;

  case F_PoofTarget:
    // This function makes the target go away
    pstate->returncode = bfalse;
    if ( ACTIVE_CHR( chrlst, pstate->target ) && !chr_is_player(gs, pstate->target) )
    {
      // tell target to go away
      ptarget->gopoof = btrue;
      pstate->target  = pstate->iself;
      pstate->returncode      = btrue;
    }
    break;

  case F_ChildDoActionOverride:
    // This function starts a new action, if it is valid for the model
    // It will fail if the action is invalid
    pstate->returncode = scr_ChildDoActionOverride(gs, pstate);
    break;

  case F_SpawnPoof:
    // This function makes a lovely little poof at the character's location
    spawn_poof( gs, pstate->iself, pstate->pself->model );
    break;

  case F_SetSpeedPercent:
    chr_reset_accel( gs, pstate->iself );
    pstate->pself->skin.maxaccel *= pstate->tmpargument / 100.0;
    break;

  case F_SetChildState:
    // This function sets the child's state
    pchild->aistate.state = pstate->tmpargument;
    break;

  case F_SpawnAttachedSizedParticle:
    // This function spawns an attached particle, then sets its size
    pstate->returncode = bfalse;
    if ( ACTIVE_CHR( chrlst, pstate->iself ) && !chr_in_pack( chrlst, chrlst_size, pstate->iself ) )
    {
      tmpchr = pstate->iself;
      if ( chr_attached( chrlst, chrlst_size, pstate->iself ) )  tmpchr = chr_get_attachedto( chrlst, chrlst_size, pstate->iself );

      tmpprt = prt_spawn( gs, 1.0f, pstate->pself->ori.pos, pstate->pself->ori.vel, pstate->pself->ori.turn_lr, pstate->pself->model, PIP_REF(pstate->tmpargument), pstate->iself, (GRIP)pstate->tmpdistance, pstate->pself->team, tmpchr, 0, INVALID_CHR );
      if ( RESERVED_PRT( prtlst, tmpprt ) )
      {
        prtlst[tmpprt].size_fp8 = pstate->tmpturn;
        pstate->returncode = btrue;
      }
    }
    break;

  case F_ChangeArmor:
    // This function sets the character's armor type and returns the old type
    // as tmpargument and the new type as tmpx
    pstate->tmpx = pstate->tmpargument;
    iTmp = pstate->pself->skin_ref % MAXSKIN;
    pstate->tmpx = change_armor( gs, pstate->iself, pstate->tmpargument );
    pstate->tmpargument = iTmp;  // The character's old armor
    break;

  case F_ShowTimer:
    // This function turns the timer on, using the value for tmpargument
    gs->timeron = btrue;
    gs->timervalue = pstate->tmpargument;
    break;

  case F_IfFacingTarget:
    {
      // This function proceeds only if the character is facing the target
      // try to avoid using inverse trig functions
      float dx, dy, d2, ftmpx, ftmpy, ftmp;

      pstate->returncode = bfalse;
      dx = ptarget->ori.pos.x - pstate->pself->ori.pos.x;
      dy = ptarget->ori.pos.y - pstate->pself->ori.pos.y;
      d2 = dx * dx + dy * dy;

      if ( d2 > 0.0f )
      {
        // use non-inverse function to get direction vec from chrlst[].turn
        turn_to_vec( pstate->pself->ori.turn_lr, &ftmpx, &ftmpy );

        // calc the dotproduct
        ftmp = ( dx * ftmpx + dy * ftmpy );

        // negative dotprod means facing the wrong direction
        if ( ftmp > 0.0f )
        {
          // normalized dot product, squared
          ftmp *= ftmp / d2;

          // +/- 45 degrees means ftmp > 0.5f
          pstate->returncode = ftmp > 0.5f;
        }
      }

    }
    break;

  case F_PlaySoundVolume:
    // This function sets the volume of a sound and plays it
    pstate->returncode = bfalse;
    if ( INVALID_SOUND != pstate->tmpargument && pstate->tmpdistance >= 0 )
    {
      volume = pstate->tmpdistance;
      pstate->returncode = ( INVALID_SOUND != snd_play_sound( gs, MIN( 1.0f, volume / 255.0f ), pstate->pself->ori_old.pos, pobj->wavelist[pstate->tmpargument], 0, pstate->pself->model, pstate->tmpargument ) );
    }
    break;

  case F_SpawnAttachedFacedParticle:
    // This function spawns an attached particle with facing
    pstate->returncode = bfalse;
    if ( ACTIVE_CHR( chrlst, pstate->iself ) && !chr_in_pack( chrlst, chrlst_size, pstate->iself ) )
    {
      tmpchr = pstate->iself;
      if ( chr_attached( chrlst, chrlst_size, pstate->iself ) )  tmpchr = chr_get_attachedto( chrlst, chrlst_size, pstate->iself );

      tmpprt = prt_spawn( gs, 1.0f, pstate->pself->ori.pos, pstate->pself->ori.vel, pstate->tmpturn, pstate->pself->model, PIP_REF(pstate->tmpargument), pstate->iself, (GRIP)pstate->tmpdistance, pstate->pself->team, tmpchr, 0, INVALID_CHR );
      pstate->returncode = RESERVED_PRT( prtlst, tmpprt );
    }
    break;

  case F_IfStateIsOdd:
    pstate->returncode = HAS_SOME_BITS( pstate->state, 1 );
    break;

  case F_SetTargetToDistantEnemy:
    // This function finds an enemy, within a certain distance to the character, and
    // proceeds only if there is one

    pstate->returncode = chr_search_distant( gs, SearchInfo_new(&loc_search), pstate->iself, pstate->tmpdistance, btrue, bfalse );

    if ( pstate->returncode )
    {
      pstate->target = loc_search.besttarget;
    }
    break;

  case F_Teleport:
    // This function teleports the character to the X, Y location, failing if the
    // location is off the map or blocked
    {
      vect3 pos = VECT3(pstate->tmpx, pstate->tmpy, pstate->tmpdistance);
      pstate->returncode = _Teleport(gs, pstate->iself, pos, pstate->tmpturn);
    }
    break;

  case F_GiveStrengthToTarget:
    // Permanently boost the target's strength
    if ( ptarget->alive )
    {
      iTmp = pstate->tmpargument;
      getadd( 0, ptarget->stats.strength_fp8, PERFECTSTAT, &iTmp );
      ptarget->stats.strength_fp8 += iTmp;
    }
    break;

  case F_GiveWisdomToTarget:
    // Permanently boost the target's wisdom
    if ( ptarget->alive )
    {
      iTmp = pstate->tmpargument;
      getadd( 0, ptarget->stats.wisdom_fp8, PERFECTSTAT, &iTmp );
      ptarget->stats.wisdom_fp8 += iTmp;
    }
    break;

  case F_GiveIntelligenceToTarget:
    // Permanently boost the target's intelligence
    if ( ptarget->alive )
    {
      iTmp = pstate->tmpargument;
      getadd( 0, ptarget->stats.intelligence_fp8, PERFECTSTAT, &iTmp );
      ptarget->stats.intelligence_fp8 += iTmp;
    }
    break;

  case F_GiveDexterityToTarget:
    // Permanently boost the target's dexterity
    if ( ptarget->alive )
    {
      iTmp = pstate->tmpargument;
      getadd( 0, ptarget->stats.dexterity_fp8, PERFECTSTAT, &iTmp );
      ptarget->stats.dexterity_fp8 += iTmp;
    }
    break;

  case F_GiveLifeToTarget:
    // Permanently boost the target's life
    if ( ptarget->alive )
    {
      iTmp = pstate->tmpargument;
      getadd( LOWSTAT, ptarget->stats.lifemax_fp8, PERFECTBIG, &iTmp );
      ptarget->stats.lifemax_fp8 += iTmp;
      if ( iTmp < 0 )
      {
        getadd( 1, ptarget->stats.life_fp8, PERFECTBIG, &iTmp );
      }
      ptarget->stats.life_fp8 += iTmp;
    }
    break;

  case F_GiveManaToTarget:
    // Permanently boost the target's mana
    if ( ptarget->alive )
    {
      iTmp = pstate->tmpargument;
      getadd( 0, ptarget->stats.manamax_fp8, PERFECTBIG, &iTmp );
      ptarget->stats.manamax_fp8 += iTmp;
      if ( iTmp < 0 )
      {
        getadd( 0, ptarget->stats.mana_fp8, PERFECTBIG, &iTmp );
      }
      ptarget->stats.mana_fp8 += iTmp;
    }
    break;

  case F_ShowMap:
    // Show the map...  Fails if map already visible
    if ( gfx->Map_on )  pstate->returncode = bfalse;
    gfx->Map_on = btrue;
    break;

  case F_ShowYouAreHere:
    // Show the camera target location
    gfx->Map_youarehereon = btrue;
    break;

  case F_ShowBlipXY:
    // Add a blip
    if ( gfx->BlipList_count < MAXBLIP )
    {
      if ( mesh_check( &(pmesh->Info), pstate->tmpx, pstate->tmpy ) )
      {
        if ( pstate->tmpargument < BAR_COUNT && pstate->tmpargument >= 0 )
        {
          gfx->BlipList[gfx->BlipList_count].x = mesh_fraction_x( &(pmesh->Info), pstate->tmpx ) * MAPSIZE * gfx->Map_scale;
          gfx->BlipList[gfx->BlipList_count].y = mesh_fraction_y( &(pmesh->Info), pstate->tmpy ) * MAPSIZE * gfx->Map_scale ;
          gfx->BlipList[gfx->BlipList_count].c = (COLR)pstate->tmpargument;
          gfx->BlipList_count++;
        }
      }
    }
    break;

  case F_HealTarget:
    // Give some life to the target
    if ( ptarget->alive )
    {
      ENC_REF tmpenc;
      iTmp = pstate->tmpargument;
      getadd( 1, ptarget->stats.life_fp8, ptarget->stats.lifemax_fp8, &iTmp );
      ptarget->stats.life_fp8 += iTmp;

      // Check all enchants to see if they are removed
      tmpenc = ptarget->firstenchant;
      while ( INVALID_ENC != tmpenc )
      {
        ENC_REF nextenc = enclst[tmpenc].nextenchant;
        if ( MAKE_IDSZ( "HEAL" ) == gs->EveList[enclst[tmpenc].eve].removedbyidsz )
        {
          remove_enchant( gs, tmpenc );
        }
        tmpenc = nextenc;
      }
    }
    break;

  case F_PumpTarget:
    // Give some mana to the target
    if ( ptarget->alive )
    {
      iTmp = pstate->tmpargument;
      getadd( 0, ptarget->stats.mana_fp8, ptarget->stats.manamax_fp8, &iTmp );
      ptarget->stats.mana_fp8 += iTmp;
    }
    break;

  case F_CostAmmo:
    // Take away one ammo
    if ( pstate->pself->ammo > 0 )
    {
      pstate->pself->ammo--;
    }
    break;

  case F_MakeSimilarNamesKnown:
    // Make names of matching objects known
    {
      CHR_REF tmpchr;
      for ( tmpchr = 0; tmpchr < chrlst_size; tmpchr++ )
      {
        sTmp = btrue;
        for ( tTmp = 0; tTmp < IDSZ_COUNT; tTmp++ )
        {
          if ( pcap->idsz[tTmp] != ChrList_getPCap(gs, tmpchr)->idsz[tTmp] )
          {
            sTmp = bfalse;
            break;
          }
        }

        if ( sTmp )
        {
          chrlst[tmpchr].prop.nameknown = btrue;
        }
      }
    }
    break;

  case F_SpawnAttachedHolderParticle:
    // This function spawns an attached particle, attached to the holder
    pstate->returncode = bfalse;
    if ( ACTIVE_CHR( chrlst, pstate->iself ) && !chr_in_pack( chrlst, chrlst_size, pstate->iself ) )
    {
      tmpchr = pstate->iself;
      if ( chr_attached( chrlst, chrlst_size, pstate->iself ) )  tmpchr = chr_get_attachedto( chrlst, chrlst_size, pstate->iself );

      tmpprt = prt_spawn( gs, 1.0f, pstate->pself->ori.pos, pstate->pself->ori.vel, pstate->pself->ori.turn_lr, pstate->pself->model, PIP_REF(pstate->tmpargument), tmpchr, (GRIP)pstate->tmpdistance, pstate->pself->team, tmpchr, 0, INVALID_CHR );
      pstate->returncode = RESERVED_PRT( prtlst, tmpprt );
    };
    break;

  case F_SetTargetReloadTime:
    // This function sets the target's reload time
    ptarget->reloadtime = pstate->tmpargument;
    break;

  case F_SetFogLevel:
    // This function raises and lowers the module's fog
    fTmp = ( pstate->tmpargument / 10.0 ) - gfx->Fog.top;
    gfx->Fog.top += fTmp;
    gfx->Fog.distance += fTmp;
    gfx->Fog.on = CData.fogallowed;
    if ( gfx->Fog.distance < 1.0 )  gfx->Fog.on = bfalse;
    break;

  case F_GetFogLevel:
    // This function gets the fog level
    pstate->tmpargument = gfx->Fog.top * 10;
    break;

  case F_SetFogTAD:
    // This function changes the fog color
    gfx->Fog.red = pstate->tmpturn;
    gfx->Fog.grn = pstate->tmpargument;
    gfx->Fog.blu = pstate->tmpdistance;
    break;

  case F_SetFogBottomLevel:
    // This function sets the module's bottom fog level...
    fTmp = ( pstate->tmpargument / 10.0 ) - gfx->Fog.bottom;
    gfx->Fog.bottom += fTmp;
    gfx->Fog.distance -= fTmp;
    gfx->Fog.on = CData.fogallowed;
    if ( gfx->Fog.distance < 1.0 )  gfx->Fog.on = bfalse;
    break;

  case F_GetFogBottomLevel:
    // This function gets the fog level
    pstate->tmpargument = gfx->Fog.bottom * 10;
    break;

  case F_CorrectActionForHand:
    // This function turns ZA into ZA, ZB, ZC, or ZD...
    // tmpargument must be set to one of the A actions beforehand...
    if ( chr_attached( chrlst, chrlst_size, pstate->iself ) )
    {
      if ( pstate->pself->inwhichslot == SLOT_RIGHT )
      {
        // C or D
        pstate->tmpargument += 2;
      };
      pstate->tmpargument += IRAND(&loc_rand, 1);
    }
    break;

  case F_IfTargetIsMounted:
    // This function proceeds if the target is riding a mount
    pstate->returncode = bfalse;
    {
      tmpchr = chr_get_aitarget( chrlst, chrlst_size, chrlst + pstate->iself );
      if ( ACTIVE_CHR( chrlst, tmpchr ) )
      {
        tmpchr = chr_get_attachedto(chrlst, chrlst_size, tmpchr);
        if ( ACTIVE_CHR( chrlst, tmpchr ) )
        {
          pstate->returncode = chrlst[tmpchr].bmpdata.calc_is_mount;
        }
      }
    }
    break;

  case F_SparkleIcon:
    // This function makes a blippie thing go around the icon
    if ( pstate->tmpargument < BAR_COUNT && pstate->tmpargument > -1 )
    {
      pstate->pself->sparkle = pstate->tmpargument;
    }
    break;

  case F_UnsparkleIcon:
    // This function stops the blippie thing
    pstate->pself->sparkle = NOSPARKLE;
    break;

  case F_GetTileXY:
    // This function gets the tile at x,y
    pstate->tmpargument = mesh_get_tile( pmesh, MESH_FLOAT_TO_FAN( pstate->tmpx ), MESH_FLOAT_TO_FAN( pstate->tmpy ) );
    break;

  case F_SetTileXY:
    // This function changes the tile at x,y
    mesh_set_tile( pmesh, MESH_FLOAT_TO_FAN( pstate->tmpx ), MESH_FLOAT_TO_FAN( pstate->tmpy ), pstate->tmpargument & 255 );
    break;

  case F_SetShadowSize:
    // This function changes a character's shadow size
    //pstate->pself->bmpdata.shadow = pstate->tmpargument * pstate->pself->fat;
    //pstate->pself->bmpdata_save.shadow = pstate->tmpargument;
    break;

  case F_SignalTarget:
    // This function orders one specific character...  The target
    // Be careful in using this, always checking IDSZ first
    ptarget->message.type = pstate->tmpargument;
    ptarget->message.data = 0;
    ptarget->aistate.alert |= ALERT_SIGNALED;
    break;

  case F_SetTargetToWhoeverIsInPassage:
    // This function lets passage rectangles be used as event triggers
    {
      tmpchr = passage_search_blocking( gs, pstate->tmpargument );
      pstate->returncode = bfalse;
      if ( ACTIVE_CHR( chrlst, tmpchr ) )
      {
        pstate->target = tmpchr;
        pstate->returncode = btrue;
      }
    }
    break;

  case F_IfCharacterWasABook:
    // This function proceeds if the base model is the same as the current
    // model or if the base model is SPELLBOOK
    pstate->returncode = ( pstate->pself->model_base == SPELLBOOK ||
      pstate->pself->model_base == pstate->pself->model );
    break;

  case F_SetEnchantBoostValues:
    // This function sets the boost values for the last enchantment
    {
      ENC_REF tmpenc = pstate->pself->undoenchant;
      if ( INVALID_ENC != tmpenc )
      {
        enclst[tmpenc].ownermana_fp8  = pstate->tmpargument;
        enclst[tmpenc].ownerlife_fp8  = pstate->tmpdistance;
        enclst[tmpenc].targetmana_fp8 = pstate->tmpx;
        enclst[tmpenc].targetlife_fp8 = pstate->tmpy;
      }
    }
    break;

  case F_SpawnCharacterXYZ:
    {
      vect3 chr_pos = VECT3(pstate->tmpx, pstate->tmpy, pstate->tmpdistance);
      vect3 chr_vel = ZERO_VECT3;

      // This function spawns a character, failing if x,y,z is invalid
      pstate->returncode = bfalse;
      tmpchr = chr_spawn( gs, chr_pos, chr_vel, pstate->pself->model, pstate->pself->team, 0, pstate->tmpturn, NULL, INVALID_CHR );
      if ( VALID_CHR( chrlst, tmpchr ) )
      {
        if ( 0 != chr_hitawall( gs, chrlst + tmpchr, NULL ) )
        {
          chrlst[tmpchr].freeme = btrue;
        }
        else
        {
          chrlst[tmpchr].prop.iskursed = bfalse;
          chrlst[tmpchr].passage       = pstate->pself->passage;

          pstate->child                     = tmpchr;
          chrlst[tmpchr].aistate.owner = pstate->owner;

          pstate->returncode = btrue;
        }
      }
    }
    break;

  case F_SpawnExactCharacterXYZ:
    // This function spawns a character ( specific model slot ),
    // failing if x,y,z is invalid
    {
      vect3 chr_pos = VECT3(pstate->tmpx, pstate->tmpy, pstate->tmpdistance);
      vect3 chr_vel = ZERO_VECT3;

      pstate->returncode = bfalse;
      tmpchr = chr_spawn( gs, chr_pos, chr_vel, OBJ_REF(pstate->tmpargument), pstate->pself->team, 0, pstate->tmpturn, NULL, INVALID_CHR );
      if ( VALID_CHR( chrlst, tmpchr ) )
      {
        if ( 0 != chr_hitawall( gs, chrlst + tmpchr, NULL ) )
        {
          chrlst[tmpchr].freeme = btrue;
        }
        else
        {
          chrlst[tmpchr].prop.iskursed = bfalse;
          chrlst[tmpchr].passage       = pstate->pself->passage;

          pstate->child                     = tmpchr;
          chrlst[tmpchr].aistate.owner = pstate->owner;

          pstate->returncode = btrue;
        }
      }
    }
    break;

  case F_ChangeTargetClass:
    // This function changes a character's model ( specific model slot )
    pstate->returncode = bfalse;
    if ( ACTIVE_CHR( chrlst, pstate->target ) )
    {
      change_character( gs, pstate->target, OBJ_REF(pstate->tmpargument), 0, LEAVE_ALL );
      ptarget->aistate.morphed = btrue;
    };
    break;

  case F_PlayFullSound:
    // This function plays a sound loud for everyone...  Victory music
    pstate->returncode = bfalse;
    if (pstate->tmpargument>=0 && pstate->tmpargument!=MAXWAVE )
    {
      pstate->returncode = ( INVALID_SOUND != snd_play_sound( gs, 1.0f, GCamera.trackpos, pobj->wavelist[pstate->tmpargument], 0, pstate->pself->model, pstate->tmpargument) );
    }
    break;

  case F_SpawnExactChaseParticle:
    // This function spawns an exactly placed particle that chases the target
    pstate->returncode = bfalse;
    if ( ACTIVE_CHR( chrlst, pstate->iself ) && !chr_in_pack( chrlst, chrlst_size, pstate->iself ) )
    {
      vect3 prt_pos = VECT3(pstate->tmpx, pstate->tmpy, pstate->tmpdistance);
      vect3 prt_vel = ZERO_VECT3;

      tmpchr = pstate->iself;
      if ( chr_attached( chrlst, chrlst_size, pstate->iself ) )  tmpchr = chr_get_attachedto( chrlst, chrlst_size, pstate->iself );

      tmpprt = prt_spawn( gs, 1.0f, prt_pos, prt_vel, pstate->pself->ori.turn_lr, pstate->pself->model, PIP_REF(pstate->tmpargument), INVALID_CHR, (GRIP)0, pstate->pself->team, tmpchr, 0, INVALID_CHR );
      if ( RESERVED_PRT( prtlst, tmpprt ) )
      {
        prtlst[tmpprt].target = chr_get_aitarget( chrlst, chrlst_size, chrlst + pstate->iself );
        pstate->returncode = btrue;
      }
    }
    break;

  case F_EncodeOrder:
    // This function packs up an order, using tmpx, tmpy, tmpargument and the
    // target ( if valid ) to create a new tmpargument
    sTmp  = ( REF_TO_INT(pstate->target)   & 0x0FFF ) << 20;
    sTmp |= (( pstate->tmpx >> 8 ) & 0x00FF ) << 12;
    sTmp |= (( pstate->tmpy >> 8 ) & 0x00FF ) << 4;
    sTmp |=  ( pstate->tmpargument & 0x000F );
    pstate->tmpargument = sTmp;
    break;

  case F_SignalSpecialID:
    // This function issues an order to all with the given special IDSZ
    signal_idsz_index( gs, pstate->pself->team_rank, pstate->tmpargument, pstate->tmpdistance, IDSZ_SPECIAL );
    break;

  case F_UnkurseTargetInventory:
    // This function unkurses every item a character is holding
    {
      CHR_REF tmpchr;

      for ( _slot = SLOT_LEFT; _slot <= SLOT_RIGHT; _slot = ( SLOT )( _slot + 1 ) )
      {
        tmpchr = chr_get_holdingwhich( chrlst, chrlst_size, pstate->target, _slot );
        chrlst[tmpchr].prop.iskursed = bfalse;
      };

      tmpchr  = chr_get_nextinpack( chrlst, chrlst_size, pstate->target );
      while ( ACTIVE_CHR( chrlst, tmpchr ) )
      {
        chrlst[tmpchr].prop.iskursed = bfalse;
        tmpchr  = chr_get_nextinpack( chrlst, chrlst_size, tmpchr );
      }
    }
    break;

  case F_IfTargetIsSneaking:
    // This function proceeds if the target is doing ACTION_DA or ACTION_WA
    pstate->returncode = ( ptarget->action.now == ACTION_DA || ptarget->action.now == ACTION_WA );
    break;

  case F_DropItems:
    // This function drops all of the character's items
    drop_all_items( gs, pstate->iself );
    break;

  case F_RespawnTarget:
    // This function respawns the target at its current location
    {
      tmpchr = chr_get_aitarget( chrlst, chrlst_size, chrlst + pstate->iself );
      chrlst[tmpchr].ori_old.pos = chrlst[tmpchr].ori.pos;
      respawn_character( gs, tmpchr );
      chrlst[tmpchr].ori.pos = chrlst[tmpchr].ori_old.pos;
    }
    break;

  case F_TargetDoActionSetFrame:
    // This function starts a new action, if it is valid for the model and
    // sets the starting frame.  It will fail if the action is invalid
    pstate->returncode = bfalse;
    if ( pstate->tmpargument < MAXACTION )
    {
      if ( ChrList_getPMad(gs, loc_target)->actionvalid[pstate->tmpargument] )
      {
        ptarget->action.now = (ACTION)pstate->tmpargument;
        ptarget->action.ready = bfalse;

        ptarget->anim.ilip = 0;
        ptarget->anim.flip    = 0.0f;
        ptarget->anim.next    = ChrList_getPMad(gs, loc_target)->actionstart[pstate->tmpargument];
        ptarget->anim.last    = ptarget->anim.next;

        pstate->returncode = btrue;
      }
    }
    break;

  case F_IfTargetCanSeeInvisible:
    // This function proceeds if the target can see invisible
    pstate->returncode = ptarget->prop.canseeinvisible;
    break;

  case F_SetTargetToNearestBlahID:
    // This function finds the nearest target that meets the
    // requirements

    pstate->returncode = chr_search_wide_nearest( gs, SearchInfo_new(&loc_search), pstate->iself,
      HAS_SOME_BITS( pstate->tmpdistance, SEARCH_ITEMS   ),
      HAS_SOME_BITS( pstate->tmpdistance, SEARCH_FRIENDS ),
      HAS_SOME_BITS( pstate->tmpdistance, SEARCH_ENEMIES ),
      HAS_SOME_BITS( pstate->tmpdistance, SEARCH_DEAD    ),
      pstate->tmpargument );
    if ( pstate->returncode )
    {
      pstate->target = loc_search.nearest;
    }
    break;

  case F_SetTargetToNearestEnemy:
    // This function finds the nearest target that meets the
    // requirements

    pstate->returncode = chr_search_wide_nearest( gs, SearchInfo_new(&loc_search), pstate->iself, bfalse, bfalse, btrue, bfalse, IDSZ_NONE );

    if ( pstate->returncode )
    {
      pstate->target = loc_search.nearest;
    }
    break;

  case F_SetTargetToNearestFriend:
    // This function finds the nearest target that meets the
    // requirements

    pstate->returncode = chr_search_wide_nearest( gs, SearchInfo_new(&loc_search), pstate->iself, bfalse, btrue, bfalse, bfalse, IDSZ_NONE );

    if ( pstate->returncode )
    {
      pstate->target = loc_search.nearest;
    }
    break;

  case F_SetTargetToNearestLifeform:
    // This function finds the nearest target that meets the
    // requirements

    pstate->returncode = chr_search_wide_nearest( gs, SearchInfo_new(&loc_search), pstate->iself, bfalse, btrue, btrue, bfalse, IDSZ_NONE );

    if ( pstate->returncode )
    {
      pstate->target = loc_search.nearest;
    }
    break;

  case F_FlashPassage:
    // This function makes the passage light or dark...  For debug...
    passage_flash( gs, pstate->tmpargument, pstate->tmpdistance );
    break;

  case F_FindTileInPassage:
    // This function finds the next tile in the passage, tmpx and tmpy are
    // required and set on return
    pstate->returncode = scr_search_tile_in_passage( gs, pstate );

    break;

  case F_IfHeldInLeftSaddle:
    // This function proceeds if the character is in the left hand of another
    // character
    pstate->returncode = bfalse;
    {
      tmpchr = chr_get_attachedto( chrlst, chrlst_size, pstate->iself );
      if ( ACTIVE_CHR( chrlst, tmpchr ) )
      {
        pstate->returncode = ( chrlst[tmpchr].holdingwhich[SLOT_SADDLE] == pstate->iself );
      }
    }
    break;

  case F_NotAnItem:
    // This function makes the character a non-item character
    pstate->pself->prop.isitem = bfalse;
    break;

  case F_SetChildAmmo:
    // This function sets the child's ammo
    pchild->ammo = pstate->tmpargument;
    break;

  case F_IfHitVulnerable:
    // This function proceeds if the character was hit by a weapon with the
    // correct vulnerability IDSZ...  [SILV] for Werewolves...
    pstate->returncode = HAS_SOME_BITS( pstate->alert, ALERT_HITVULNERABLE );
    break;

  case F_IfTargetIsFlying:
    // This function proceeds if the character target is flying
    pstate->returncode = ( ptarget->flyheight > 0 );
    break;

  case F_IdentifyTarget:
    // This function reveals the target's name, ammo, and usage
    // Proceeds if the target was unknown
    pstate->returncode = bfalse;
    {
      tmpchr = chr_get_aitarget( chrlst, chrlst_size, chrlst + pstate->iself );
      if ( chrlst[tmpchr].ammomax != 0 )  chrlst[tmpchr].ammoknown = btrue;
      if ( 0 == strncmp(chrlst[tmpchr].name, "Blah", 4) )
      {
        pstate->returncode = !chrlst[tmpchr].prop.nameknown;
        chrlst[tmpchr].prop.nameknown = btrue;
      }
      chrlst[tmpchr].prop.usageknown = btrue;
    }
    break;

  case F_BeatModule:
    // This function displays the Module Ended message
    gs->modstate.beat = btrue;
    break;

  case F_EndModule:
    // This function presses the Escape key
    keyb.state[SDLK_ESCAPE] = 1;
    break;

  case F_DisableExport:
    // This function turns export off
    gs->modstate.exportvalid = bfalse;
    break;

  case F_EnableExport:
    // This function turns export on
    gs->modstate.exportvalid = btrue;
    break;

  case F_GetTargetState:
    // This function sets tmpargument to the state of the target
    pstate->tmpargument = ptarget->aistate.state;
    break;

  case F_ClearEndText:
    // This function empties the end-module text buffer
    gs->endtext[0]   = EOS;
    break;

  case F_AddEndText:
    // This function appends a message to the end-module text buffer
    append_end_text( gs, pobj->msg_start + pstate->tmpargument, pstate->iself );
    break;

  case F_PlayMusic:
    // This function begins playing a new track of music
    if ( CData.allow_music )
    {
      SoundState_t * snd = snd_getState(&CData);
      if( NULL != snd && snd->song_index != pstate->tmpargument )
      {
        snd_play_music( pstate->tmpargument, pstate->tmpdistance, -1 );
      }
    }
    break;

  case F_SetMusicPassage:
    // This function makes the given passage play music if a player enters it
    // tmpargument is the passage to set and tmpdistance is the music track to play...
    gs->PassList[pstate->tmpargument].music = pstate->tmpdistance;
    break;

  case F_MakeCrushInvalid:
    // This function makes doors unable to close on this object
    pstate->pself->prop.canbecrushed = bfalse;
    break;

  case F_StopMusic:
    // This function stops the interactive music
    snd_stop_music(1000);
    break;

  case F_FlashVariable:
    // This function makes the character flash according to tmpargument
    flash_character( gs, pstate->iself, pstate->tmpargument );
    break;

  case F_AccelerateUp:
    // This function changes the character's up down velocity
    pstate->pself->ori.vel.z += pstate->tmpargument / 100.0;
    break;

  case F_FlashVariableHeight:
    // This function makes the character flash, feet one color, head another...
    flash_character_height( gs, pstate->iself, pstate->tmpturn, pstate->tmpx,
      pstate->tmpdistance, pstate->tmpy );
    break;

  case F_SetDamageTime:
    // This function makes the character invincible for a little while
    pstate->pself->damagetime = pstate->tmpargument;
    break;

  case F_IfStateIs8:
    pstate->returncode = ( 8 == pstate->state );
    break;

  case F_IfStateIs9:
    pstate->returncode = ( 9 == pstate->state );
    break;

  case F_IfStateIs10:
    pstate->returncode = ( 10 == pstate->state );
    break;

  case F_IfStateIs11:
    pstate->returncode = ( 11 == pstate->state );
    break;

  case F_IfStateIs12:
    pstate->returncode = ( 12 == pstate->state );
    break;

  case F_IfStateIs13:
    pstate->returncode = ( 13 == pstate->state );
    break;

  case F_IfStateIs14:
    pstate->returncode = ( 14 == pstate->state );
    break;

  case F_IfStateIs15:
    pstate->returncode = ( 15 == pstate->state );
    break;

  case F_IfTargetIsAMount:
    pstate->returncode = ptarget->bmpdata.calc_is_mount;
    break;

  case F_IfTargetIsAPlatform:
    pstate->returncode = ptarget->bmpdata.calc_is_platform;
    break;

  case F_AddStat:
    if ( !pstate->pself->staton )
    {
      add_status( gs, pstate->iself );
    }
    break;

  case F_DisenchantTarget:
    pstate->returncode = ( INVALID_ENC != ptarget->firstenchant );
    disenchant_character( gs, pstate->target );
    break;

  case F_DisenchantAll:
    {
      ENC_REF tmpenc;
      for ( tmpenc=0; tmpenc < enclst_size; tmpenc++ )
      {
        remove_enchant( gs, tmpenc );
      }
    }
    break;

  case F_SetVolumeNearestTeammate:
    //This sets the volume for the looping sounds of all the character's teammates
    if( !gs->modstate.Paused && pstate->tmpdistance >= 0)
    {
      SoundState_t * snd = snd_getState(&CData);

      if(snd->soundActive)
      {
        //Go through all teammates
        CHR_REF tmpchr;
        for(tmpchr = 0; tmpchr < chrlst_size; tmpchr++)
        {
          if(!ACTIVE_CHR(chrlst, tmpchr) ) continue;

          if(chrlst[tmpchr].alive && chrlst[tmpchr].team == pstate->pself->team)
          {
            //And set their volume to tmpdistance
            if( pstate->tmpdistance >= 0 && INVALID_SOUND != chrlst[tmpchr].loopingchannel )
            {
              Mix_Volume(chrlst[tmpchr].loopingchannel, pstate->tmpdistance);
            }
          }
        }
      }
    }
    break;

  case F_AddShopPassage:
    // This function defines a shop area
    ShopList_add( gs, pstate->iself, pstate->tmpargument );
    break;

  case F_TargetPayForArmor:
    // This function costs the target some money, or fails if 'e doesn't have
    // enough...
    // tmpx is amount needed
    // tmpy is cost of new skin
    {
      CAP_REF tmp_icap;
      tmpchr = chr_get_aitarget( chrlst, chrlst_size, chrlst + pstate->iself );    // The target
      tmp_icap = ChrList_getRCap(gs, tmpchr);                                     // The target's model
      iTmp =  gs->CapList[tmp_icap].skin[pstate->tmpargument % MAXSKIN].cost;
      pstate->tmpy = iTmp;                // Cost of new skin
      iTmp -= gs->CapList[tmp_icap].skin[chrlst[tmpchr].skin_ref % MAXSKIN].cost;   // Refund
      if ( iTmp > chrlst[tmpchr].money )
      {
        // Not enough...
        pstate->tmpx = iTmp - chrlst[tmpchr].money;  // Amount needed
        pstate->returncode = bfalse;
      }
      else
      {
        // Pay for it...  Cost may be negative after refund...
        chrlst[tmpchr].money -= iTmp;
        if ( chrlst[tmpchr].money > MAXMONEY )  chrlst[tmpchr].money = MAXMONEY;
        pstate->tmpx = 0;
        pstate->returncode = btrue;
      }
    }
    break;

  case F_JoinEvilTeam:
    // This function adds the character to the evil team...
    switch_team( gs, pstate->iself, TEAM_REF(TEAM_EVIL) );
    break;

  case F_JoinNullTeam:
    // This function adds the character to the null team...
    switch_team( gs,  pstate->iself, TEAM_REF(TEAM_NULL) );
    break;

  case F_JoinGoodTeam:
    // This function adds the character to the good team...
    switch_team( gs,  pstate->iself, TEAM_REF(TEAM_GOOD) );
    break;

  case F_PitsKill:
    // This function activates pit deaths...
    gs->pits_kill = btrue;
    break;

  case F_SetTargetToPassageID:
    // This function finds a character who is both in the passage and who has
    // an item with the given IDSZ
    {
      tmpchr = passage_search_blocking_ID( gs, pstate->tmpargument, pstate->tmpdistance );
      pstate->returncode = bfalse;
      if ( ACTIVE_CHR( chrlst, tmpchr ) )
      {
        pstate->target = tmpchr;
        pstate->returncode = btrue;
      }
    }
    break;

  case F_MakeNameUnknown:
    // This function makes the name of an item/character unknown.
    pstate->pself->prop.nameknown = bfalse;
    break;

  case F_SpawnExactParticleEndSpawn:
    {
      vect3 prt_pos = VECT3(pstate->tmpx, pstate->tmpy, pstate->tmpdistance);
      vect3 prt_vel = ZERO_VECT3;

      // This function spawns a particle that spawns a character...
      pstate->returncode = bfalse;
      tmpchr = pstate->iself;
      if ( chr_attached( chrlst, chrlst_size, pstate->iself ) )  tmpchr = chr_get_attachedto( chrlst, chrlst_size, pstate->iself );

      tmpprt = prt_spawn( gs, 1.0f, prt_pos, prt_vel, pstate->pself->ori.turn_lr, pstate->pself->model, PIP_REF(pstate->tmpargument), INVALID_CHR, (GRIP)0, pstate->pself->team, tmpchr, 0, INVALID_CHR );
      if ( RESERVED_PRT( prtlst, tmpprt ) )
      {
        prtlst[tmpprt].spawncharacterstate = pstate->tmpturn;
        pstate->returncode = btrue;
      }
    }
    break;

  case F_SpawnPoofSpeedSpacingDamage:
    // This function makes a lovely little poof at the character's location,
    // adjusting the xy speed and spacing and the base damage first
    // Temporarily adjust the values for the particle type
    {
      PIP_REF tmp_ipip;
      CAP_REF tmp_icap = ChrList_getRCap(gs, pstate->iself);
      if ( LOADED_CAP(gs->CapList, tmp_icap) )
      {
        Cap_t * tmp_pcap = ChrList_getPCap(gs, pstate->iself);
        if ( tmp_pcap->gopoofprttype <= PRTPIP_PEROBJECT_COUNT )
        {
          tmp_ipip = ChrList_getRPip(gs, pstate->iself, REF_TO_INT(tmp_pcap->gopoofprttype) );
        }
        else
        {
          tmp_ipip = tmp_pcap->gopoofprttype;
        }

        if ( INVALID_PIP != tmp_ipip )
        {
          // store the base values
          iTmp = gs->PipList[tmp_ipip].xyvel.ibase;
          tTmp = gs->PipList[tmp_ipip].xyspacing.ibase;
          test = gs->PipList[tmp_ipip].damage_fp8.ibase;

          // set some temporary values
          gs->PipList[tmp_ipip].xyvel.ibase = pstate->tmpx;
          gs->PipList[tmp_ipip].xyspacing.ibase = pstate->tmpy;
          gs->PipList[tmp_ipip].damage_fp8.ibase = pstate->tmpargument;

          // do the poof
          spawn_poof( gs, pstate->iself, pstate->pself->model );

          // Restore the saved values
          gs->PipList[tmp_ipip].xyvel.ibase = iTmp;
          gs->PipList[tmp_ipip].xyspacing.ibase = tTmp;
          gs->PipList[tmp_ipip].damage_fp8.ibase = test;
        };
      }
    }
    break;

  case F_GiveExperienceToGoodTeam:
    // This function gives experience to everyone on the G Team
    give_team_experience( gs, TEAM_REF(TEAM_GOOD), pstate->tmpargument, (EXPERIENCE)pstate->tmpdistance );
    break;

  case F_DoNothing:
    //This function does nothing (For use with combination with Else function or debugging)
    break;

  case F_DazeTarget:
    //This function dazes the target for a duration equal to tmpargument
    ptarget->dazetime += pstate->tmpargument;
    break;

  case F_GrogTarget:
    //This function grogs the target for a duration equal to tmpargument
    ptarget->grogtime += pstate->tmpargument;
    break;

  case F_IfEquipped:
    //This proceeds if the character is equipped
    pstate->returncode = pstate->pself->isequipped;
    break;

  case F_DropTargetMoney:
    // This function drops some of the target's money
    drop_money( gs, pstate->target, pstate->tmpargument );
    break;

  case F_GetTargetContent:
    //This sets tmpargument to the current target's content value
    pstate->tmpargument = ptarget->aistate.content;
    break;

  case F_DropTargetKeys:
    //This function makes the target drops keys in inventory (Not inhand)
    drop_keys( gs, pstate->target );
    break;

  case F_JoinTeam:
    //This makes the character itself join a specified team (A = 0, B = 1, 23 = Z, etc.)
    switch_team( gs,  pstate->iself, (TEAM_REF)pstate->tmpargument );
    break;

  case F_TargetJoinTeam:
    //This makes the target join a team specified in tmpargument (A = 0, 23 = Z, etc.)
    switch_team( gs,  pstate->target, (TEAM_REF)pstate->tmpargument );
    break;

  case F_ClearMusicPassage:
    //This clears the music for an specified passage
    gs->PassList[pstate->tmpargument].music = INVALID_SOUND;
    break;


  case F_AddQuest:
    //This function adds a quest idsz set in tmpargument into the targets quest.txt
    if ( chr_is_player(gs, pstate->target) )
    {
      snprintf( cTmp, sizeof( cTmp ), "%s.obj", ptarget->name );
      pstate->returncode = add_quest_idsz( cTmp, pstate->tmpargument );
    }
    break;

  case F_BeatQuest:
    //This function marks a IDSZ in the targets quest.txt as beaten
    pstate->returncode = bfalse;
    if ( chr_is_player(gs, pstate->target) )
    {
      snprintf( cTmp, sizeof( cTmp ), "%s.obj", ptarget->name );
      if(modify_quest_idsz( cTmp, pstate->tmpargument, 0 ) == -2) pstate->returncode = btrue;
    }
    break;

  case F_IfTargetHasQuest:
    //This function proceeds if the target has the unfinished quest specified in tmpargument
    //and sets tmpx to the Quest Level of the specified quest.
    if ( chr_is_player(gs, pstate->target) )
    {
      snprintf( cTmp, sizeof( cTmp ), "%s.obj", ptarget->name );
      iTmp = check_player_quest( cTmp, pstate->tmpargument );
      if ( iTmp > QUESTBEATEN )
      {
        pstate->returncode = btrue;
        pstate->tmpx = iTmp;
      }
      else pstate->returncode = bfalse;
    }
    break;

  case F_SetQuestLevel:
    //This function modifies the quest level for a specific quest IDSZ
    //tmpargument specifies quest idsz and tmpdistance the adjustment (which may be negative)
    pstate->returncode = bfalse;
    if ( chr_is_player(gs, pstate->target) && pstate->tmpdistance != 0 )
    {
      snprintf( cTmp, sizeof( cTmp ), "%s.obj", ptarget->name );
      if(modify_quest_idsz( cTmp, pstate->tmpargument, pstate->tmpdistance ) != -1) pstate->returncode = btrue;
    }
    break;

  case F_IfTargetHasNotFullMana:
    //This function proceeds if the target has more than one point of mana and is alive
    pstate->returncode = bfalse;
    if ( ACTIVE_CHR( chrlst, pstate->target ) )
    {
      pstate->returncode = ptarget->alive && ptarget->stats.manamax_fp8 > 0 && ptarget->stats.mana_fp8 < ptarget->stats.manamax_fp8 - DAMAGE_MIN;
    };
    break;

  case F_IfDoingAction:
    //This function proceeds if the character is preforming the animation specified in tmpargument
    pstate->returncode = (pstate->pself->action.now >= pstate->tmpargument) && (pstate->pself->action.now <= pstate->tmpargument);
    break;

  case F_IfOperatorIsLinux:
    //This function proceeds if the computer is running a UNIX OS

#ifdef __unix__
    pstate->returncode = btrue;    //Player running Linux
#else
    pstate->returncode = bfalse;    //Player running something else.
#endif

    break;

  case F_IfTargetIsOwner:
    //This function proceeds if the target is the characters owner
    pstate->returncode = (pstate->target == pstate->owner);
    break;

  case F_SetCameraSwing:
    //This function sets the camera swing rate
    GCamera.swing = 0;
    GCamera.swingrate = pstate->tmpargument;
    GCamera.swingamp = pstate->tmpdistance;
    break;

  case F_EnableRespawn:
    // This function turns respawn with JUMP button on
    gs->modstate.respawnvalid = btrue;
    break;

  case F_DisableRespawn:
    // This function turns respawning with JUMP button off
    gs->modstate.respawnvalid = bfalse;
    break;

  case F_IfButtonPressed:
    // This proceeds if the character is a player and pressing the latch specified in tmpargument
    pstate->returncode = HAS_SOME_BITS( pstate->latch.b, pstate->tmpargument ) && chr_is_player(gs, pstate->iself);
    break;

  case F_IfHolderScoredAHit:
    // Proceed only if the character holder scored a hit
    pstate->returncode = bfalse;
    {
      CHR_REF iattached = chr_get_attachedto( chrlst, chrlst_size, pstate->iself );
      if ( ACTIVE_CHR( chrlst, iattached ) )
      {
        if(HAS_SOME_BITS( chrlst[pstate->target].aistate.alert, ALERT_SCOREDAHIT ))
        {
          pstate->returncode = btrue;
          pstate->target = chrlst[iattached].aistate.hitlast;
        }
      }
    }
    break;

  case F_IfHolderBlocked:
    // Proceed only if the character holder blocked an attack
    pstate->returncode = bfalse;
    {
      CHR_REF iattached = chr_get_attachedto( chrlst, chrlst_size, pstate->iself );
      if ( ACTIVE_CHR( chrlst, iattached ) )
      {
        if(HAS_SOME_BITS( chrlst[pstate->target].aistate.alert, ALERT_BLOCKED ))
        {
          pstate->returncode = btrue;
          pstate->target = chrlst[iattached].aistate.hitlast;
        }
      }
    }
    break;

  case F_GetTargetSkillLevel:
    // This function sets tmpargument to the skill level of the target
	pstate->tmpargument = check_skills( gs, pstate->target,pstate->tmpdistance);
    break;

  case F_End:
    pstate->active = bfalse;
    break;

  default:
    assert( bfalse );
  }

  return pstate->returncode;
}

//--------------------------------------------------------------------------------------------
retval_t set_operand( Game_t * gs, AI_STATE * pstate, Uint8 variable )
{
  /// @details ZZ@> This function sets one of the tmp* values for scripted AI

  retval_t retval = rv_succeed;

  ScriptInfo_t * slist = Game_getScriptInfo(gs);
  char * scriptname = slist->fname[pstate->type];

  switch ( variable )
  {
  case VAR_TMP_X:
    pstate->tmpx = pstate->operationsum;
    break;

  case VAR_TMP_Y:
    pstate->tmpy = pstate->operationsum;
    break;

  case VAR_TMP_DISTANCE:
    pstate->tmpdistance = pstate->operationsum;
    break;

  case VAR_TMP_TURN:
    pstate->tmpturn = pstate->operationsum;
    break;

  case VAR_TMP_ARGUMENT:
    pstate->tmpargument = pstate->operationsum;
    break;

  default:
    log_warning("set_operand() - \n\t\"%s\"(0x%x) unknown variable", scriptname, pstate->offset);
    retval = rv_error;
    break;
  }

  return retval;
}

//--------------------------------------------------------------------------------------------
retval_t run_operand( Game_t * gs, AI_STATE * pstate, Uint32 value )
{
  /// @details ZZ@> This function does the scripted arithmetic in operator,operand pairs

  retval_t retval = rv_succeed;
  int iTmp;

  ScriptInfo_t * slist = Game_getScriptInfo(gs);

  Cap_t * pcap = ChrList_getPCap(gs, pstate->iself);

  Chr_t    * pself = pstate->pself;
  CHR_REF    iself = pstate->iself;

  CHR_REF   loc_aitarget;
  Chr_t    * ptarget;
  Cap_t    * ptarget_cap;

  CHR_REF   loc_aiowner;
  Chr_t    * powner;
  Cap_t    * powner_cap;

  CHR_REF   loc_leader;
  Chr_t    * pleader;
  Cap_t    * pleader_cap;

  // for debugging
  char    * scriptname = slist->fname[pstate->type];
  //Uint32    offset     = pstate->offset;


  loc_aitarget = chr_get_aitarget( gs->ChrList, CHRLST_COUNT, gs->ChrList + iself );
  if(INVALID_CHR == loc_aitarget) loc_aitarget = iself;
  ptarget     = ChrList_getPChr(gs, loc_aitarget);
  ptarget_cap = ChrList_getPCap(gs, loc_aitarget);

  loc_aiowner  = chr_get_aiowner( gs->ChrList, CHRLST_COUNT, gs->ChrList + iself );
  if(INVALID_CHR == loc_aiowner) loc_aiowner = iself;
  powner       = ChrList_getPChr(gs, loc_aiowner);
  powner_cap   = ChrList_getPCap(gs, loc_aiowner);

  loc_leader   = team_get_leader( gs, pself->team );
  if(INVALID_CHR == loc_leader) loc_leader = iself;
  pleader      = ChrList_getPChr(gs, loc_leader);
  pleader_cap  = ChrList_getPCap(gs, loc_leader);

  // Get the operation code
  if ( IS_CONSTANT(value) )
  {
    // Get the working value from a constant, constants are all but high 5 bits
    iTmp = GET_CONSTANT(value);
  }
  else
  {
    // Get the working value from a register
    iTmp = 1;
    switch ( GET_VAR_BITS(value) )
    {
    case VAR_TMP_X:
      iTmp = pstate->tmpx;
      break;

    case VAR_TMP_Y:
      iTmp = pstate->tmpy;
      break;

    case VAR_TMP_DISTANCE:
      iTmp = pstate->tmpdistance;
      break;

    case VAR_TMP_TURN:
      iTmp = pstate->tmpturn;
      break;

    case VAR_TMP_ARGUMENT:
      iTmp = pstate->tmpargument;
      break;

    case VAR_RAND:
      iTmp = RANDIE(gs->randie_index);
      break;

    case VAR_SELF_X:
      iTmp = pself->ori.pos.x;
      break;

    case VAR_SELF_Y:
      iTmp = pself->ori.pos.y;
      break;

    case VAR_SELF_TURN:
      iTmp = pself->ori.turn_lr;
      break;

    case VAR_SELF_COUNTER:
      iTmp = pself->team_rank;
      break;

    case VAR_SELF_ORDER:
      iTmp = pself->message.data;
      break;

    case VAR_SELF_MORALE:
      iTmp = gs->TeamList[pself->team_base].morale;
      break;

    case VAR_SELF_LIFE:
      iTmp = pself->stats.life_fp8;
      break;

    case VAR_TARGET_X:
      iTmp = ptarget->ori.pos.x;
      break;

    case VAR_TARGET_Y:
      iTmp = ptarget->ori.pos.y;
      break;

    case VAR_TARGET_DISTANCE:
      iTmp = 0;
      if(NULL != ptarget)
      {
        iTmp = sqrt(( ptarget->ori.pos.x - pself->ori.pos.x ) * ( ptarget->ori.pos.x - pself->ori.pos.x ) +
          ( ptarget->ori.pos.y - pself->ori.pos.y ) * ( ptarget->ori.pos.y - pself->ori.pos.y ) +
          ( ptarget->ori.pos.z - pself->ori.pos.z ) * ( ptarget->ori.pos.z - pself->ori.pos.z ) );
      }
      break;

    case VAR_TARGET_TURN:
      iTmp = ptarget->ori.turn_lr;
      break;

    case VAR_LEADER_X:
      iTmp = pself->ori.pos.x;
      if ( ACTIVE_CHR( gs->ChrList, loc_leader ) )
        iTmp = pleader->ori.pos.x;
      break;

    case VAR_LEADER_Y:
      iTmp = pself->ori.pos.y;
      if ( ACTIVE_CHR( gs->ChrList, loc_leader ) )
        iTmp = pleader->ori.pos.y;
      break;

    case VAR_LEADER_DISTANCE:
      iTmp = 10000;
      if ( ACTIVE_CHR( gs->ChrList, loc_leader ) )
        iTmp = ABS(( int )( pleader->ori.pos.x - pself->ori.pos.x ) ) +
        ABS(( int )( pleader->ori.pos.y - pself->ori.pos.y ) );
      break;

    case VAR_LEADER_TURN:
      iTmp = pself->ori.turn_lr;
      if ( ACTIVE_CHR( gs->ChrList, loc_leader ) )
        iTmp = pleader->ori.turn_lr;
      break;

    case VAR_SELF_GOTO_X:
      if( wp_list_empty( &(pstate->wp) ) )
      {
        iTmp = pself->ori.pos.x;
      }
      else
      {
        iTmp = wp_list_x( &(pstate->wp) );
      }
      break;

    case VAR_SELF_GOTO_Y:
      if( wp_list_empty( &(pstate->wp) ) )
      {
        iTmp = pself->ori.pos.y;
      }
      else
      {
        iTmp = wp_list_y( &(pstate->wp) );
      }
      break;

    case VAR_SELF_GOTO_DISTANCE:
      if( wp_list_empty( &(pstate->wp) ) )
      {
        iTmp = 0;
      }
      else
      {
        iTmp = ABS(( int )( wp_list_x( &(pstate->wp) ) - pself->ori.pos.x ) ) +
          ABS(( int )( wp_list_y( &(pstate->wp) ) - pself->ori.pos.y ) );
      }
      break;

    case VAR_TARGET_TURNTO:
      iTmp = vec_to_turn( ptarget->ori.pos.x - pself->ori.pos.x, ptarget->ori.pos.y - pself->ori.pos.y );
      break;

    case VAR_SELF_PASSAGE:
      iTmp = pself->passage;
      break;

    case VAR_SELF_HOLDING_WEIGHT:
      iTmp = pself->holdingweight;
      break;

    case VAR_SELF_ALTITUDE:
      iTmp = pself->ori.pos.z - pself->level;
      break;

    case VAR_SELF_ID:
      iTmp = pcap->idsz[IDSZ_TYPE];
      break;

    case VAR_SELF_HATEID:
      iTmp = pcap->idsz[IDSZ_HATE];
      break;

    case VAR_SELF_MANA:
      iTmp = pself->stats.mana_fp8;
      if ( pself->prop.canchannel )  iTmp += pself->stats.life_fp8;
      break;

    case VAR_TARGET_STR:
      iTmp = ptarget->stats.strength_fp8;
      break;

    case VAR_TARGET_WIS:
      iTmp = ptarget->stats.wisdom_fp8;
      break;

    case VAR_TARGET_INT:
      iTmp = ptarget->stats.intelligence_fp8;
      break;

    case VAR_TARGET_DEX:
      iTmp = ptarget->stats.dexterity_fp8;
      break;

    case VAR_TARGET_LIFE:
      iTmp = ptarget->stats.life_fp8;
      break;

    case VAR_TARGET_MANA:
      iTmp = ptarget->stats.mana_fp8;
      if ( ptarget->prop.canchannel )  iTmp += ptarget->stats.life_fp8;
      break;

    case VAR_TARGET_LEVEL:
      iTmp = calc_chr_level( gs, loc_aitarget );
      break;

    case VAR_TARGET_SPEEDX:
      iTmp = ptarget->ori.vel.x;
      break;

    case VAR_TARGET_SPEEDY:
      iTmp = ptarget->ori.vel.y;
      break;

    case VAR_TARGET_SPEEDZ:
      iTmp = ptarget->ori.vel.z;
      break;

    case VAR_SELF_SPAWNX:
      iTmp = pself->spinfo.pos.x;
      break;

    case VAR_SELF_SPAWNY:
      iTmp = pself->spinfo.pos.y;
      break;

    case VAR_SELF_STATE:
      iTmp = pstate->state;
      break;

    case VAR_SELF_STR:
      iTmp = pself->stats.strength_fp8;
      break;

    case VAR_SELF_WIS:
      iTmp = pself->stats.wisdom_fp8;
      break;

    case VAR_SELF_INT:
      iTmp = pself->stats.intelligence_fp8;
      break;

    case VAR_SELF_DEX:
      iTmp = pself->stats.dexterity_fp8;
      break;

    case VAR_SELF_MANAFLOW:
      iTmp = pself->stats.manaflow_fp8;
      break;

    case VAR_TARGET_MANAFLOW:
      iTmp = ptarget->stats.manaflow_fp8;
      break;

    case VAR_SELF_ATTACHED:
      iTmp = number_of_attached_particles( gs, iself );
      break;

    case VAR_CAMERA_SWING:
      iTmp = GCamera.swing << 2;
      break;

    case VAR_XYDISTANCE:
      iTmp = sqrt( (float)(pstate->tmpx * pstate->tmpx + pstate->tmpy * pstate->tmpy) );
      break;

    case VAR_SELF_Z:
      iTmp = pself->ori.pos.z;
      break;

    case VAR_TARGET_ALTITUDE:
      iTmp = ptarget->ori.pos.z - ptarget->level;
      break;

    case VAR_TARGET_Z:
      iTmp = ptarget->ori.pos.z;
      break;

    case VAR_SELF_INDEX:
      iTmp = REF_TO_INT(iself);
      break;

    case VAR_OWNER_X:
      iTmp = powner->ori.pos.x;
      break;

    case VAR_OWNER_Y:
      iTmp = powner->ori.pos.y;
      break;

    case VAR_OWNER_TURN:
      iTmp = powner->ori.turn_lr;
      break;

    case VAR_OWNER_DISTANCE:
      iTmp = sqrt(( powner->ori.pos.x - pself->ori.pos.x ) * ( powner->ori.pos.x - pself->ori.pos.x ) +
        ( powner->ori.pos.y - pself->ori.pos.y ) * ( powner->ori.pos.y - pself->ori.pos.y ) +
        ( powner->ori.pos.z - pself->ori.pos.z ) * ( powner->ori.pos.z - pself->ori.pos.z ) );
      break;

    case VAR_OWNER_TURNTO:
      iTmp = vec_to_turn( powner->ori.pos.x - pself->ori.pos.x, powner->ori.pos.y - pself->ori.pos.y );
      break;

    case VAR_XYTURNTO:
      iTmp = vec_to_turn( pstate->tmpx - pself->ori.pos.x, pstate->tmpy - pself->ori.pos.y );
      break;

    case VAR_SELF_MONEY:
      iTmp = pself->money;
      break;

    case VAR_SELF_ACCEL:
      iTmp = ( pself->skin.maxaccel * 100.0 );
      break;

    case VAR_TARGET_EXP:
      iTmp = ptarget->experience;
      break;

    case VAR_SELF_AMMO:
      iTmp = pself->ammo;
      break;

    case VAR_TARGET_AMMO:
      iTmp = ptarget->ammo;
      break;

    case VAR_TARGET_MONEY:
      iTmp = ptarget->money;
      break;

    case VAR_TARGET_TURNAWAY:
      iTmp = vec_to_turn( pself->ori.pos.x - ptarget->ori.pos.x, pself->ori.pos.y - ptarget->ori.pos.y );
      break;

    case VAR_SELF_LEVEL:
      iTmp = calc_chr_level( gs, iself );
      break;

    case VAR_SELF_SPAWN_DISTANCE:
      iTmp = sqrt(( pself->spinfo.pos.x - pself->ori.pos.x ) * ( pself->spinfo.pos.x - pself->ori.pos.x ) +
        ( pself->spinfo.pos.y - pself->ori.pos.y ) * ( pself->spinfo.pos.y - pself->ori.pos.y ) +
        ( pself->spinfo.pos.z - pself->ori.pos.z ) * ( pself->spinfo.pos.z - pself->ori.pos.z ) );
      break;

    case VAR_TARGET_MAX_LIFE:
      iTmp = ptarget->stats.lifemax_fp8;
      break;

    case VAR_SELF_CONTENT:
      iTmp = pstate->content;
      break;

    case VAR_TARGET_RELOAD_TIME:
      iTmp = ptarget->reloadtime;
      break;

    default:
      log_warning("run_operand() - \n\t\"%s\"(0x%x) unknown variable\n", scriptname, pstate->offset);
      return rv_error;
      break;

    }
  }


  // Now do the math
  switch ( GET_OP_BITS(value) )
  {
  case OP_ADD:
    pstate->operationsum += iTmp;
    break;

  case OP_SUB:
    pstate->operationsum -= iTmp;
    break;

  case OP_AND:
    pstate->operationsum &= iTmp;
    break;

  case OP_SHR:
    pstate->operationsum >>= iTmp;
    break;

  case OP_SHL:
    pstate->operationsum <<= iTmp;
    break;

  case OP_MUL:
    pstate->operationsum *= iTmp;
    break;

  case OP_DIV:
    if ( iTmp == 0 )
    {
      log_warning("run_operand() - \n\t\"%s\"(0x%x) divide by zero", scriptname, pstate->offset);
    }
    else
    {
      pstate->operationsum /= iTmp;
    }
    break;

  case OP_MOD:
    if ( iTmp == 0 )
    {
      log_warning("run_operand() - \n\t\"%s\"(0x%x) modulus using zero", scriptname, pstate->offset);
    }
    else
    {
      pstate->operationsum %= iTmp;
    }
    break;

  default:
    log_warning("run_operand() - \n\t\"%s\"(0x%x) unknown operation", scriptname, pstate->offset);
    return rv_error;
    break;
  }

  return retval;
}

//--------------------------------------------------------------------------------------------
retval_t run_script( Game_t * gs, AI_STATE * pstate, float dUpdate )
{
  /// @details ZZ@> This function lets one character do AI stuff

  Uint16   type_stt;
  Uint32   offset_stt, offset_end, offset_last;
  Uint32   opcode;
  Uint32   iTmp;
  int      operands;
  retval_t retval = rv_succeed;

  ScriptInfo_t  * slist;

  CHR_REF iself = pstate->iself;
  Chr_t * pself = pstate->pself;

  if( !EKEY_PVALID(gs) ) return rv_error;
  slist  = Game_getScriptInfo(gs);

  if( !ACTIVE_CHR(gs->ChrList, iself) ) return rv_error;
  pself   = gs->ChrList + iself;
  pstate = &(pself->aistate);

  // Make life easier
  pstate->oldtarget = chr_get_aitarget( gs->ChrList, CHRLST_COUNT, pself );
  type_stt = pstate->type;

  // down the ai timer
  pstate->time -= dUpdate;
  if ( pstate->time < 0 ) pstate->time = 0.0f;

  // Figure out alerts that weren't already set
  set_alerts( gs, iself, dUpdate );

  // Clear the button latches

  if ( !chr_is_player(gs, iself) )
  {
    pstate->latch.b = 0;
  }

  // make sure that all of the ai "pointers" are valid
  // if a "pointer" is not valid, make it point to the character
  if( !ACTIVE_CHR(gs->ChrList, pstate->target)     ) pstate->target     = iself;
  if( !ACTIVE_CHR(gs->ChrList, pstate->oldtarget)  ) pstate->oldtarget  = iself;
  if( !ACTIVE_CHR(gs->ChrList, pstate->owner)      ) pstate->owner      = iself;
  if( !ACTIVE_CHR(gs->ChrList, pstate->child)      ) pstate->child      = iself;
  if( !ACTIVE_CHR(gs->ChrList, pstate->bumplast)   ) pstate->bumplast   = iself;
  if( !ACTIVE_CHR(gs->ChrList, pstate->attacklast) ) pstate->attacklast = iself;
  if( !ACTIVE_CHR(gs->ChrList, pstate->hitlast)    ) pstate->hitlast    = iself;

  // Reset the target if it can't be seen
  if ( !pself->prop.canseeinvisible && pself->alive && iself != pstate->oldtarget )
  {
    if ( chr_is_invisible( gs->ChrList, CHRLST_COUNT, pstate->target ) )
    {
      pstate->target = iself;
    }
  }

  // Run the AI Script
  offset_stt  = slist->offset_stt[type_stt];
  offset_end  = slist->offset_end[type_stt];
  scr_restart(pstate, offset_stt);
  while ( pstate->active && pstate->offset < offset_end )  // End Function
  {
    offset_last = pstate->offset;
    opcode = slist->buffer[pstate->offset];

    // set the indents
    pstate->lastindent = pstate->indent;
    pstate->indent     = GET_INDENT(opcode);

    // Was it a function
    if ( IS_FUNCTION( opcode ) )
    {
      // Run the function
      run_function( gs, pstate, opcode );

      // Get the jump code
      pstate->offset++;
      iTmp = slist->buffer[pstate->offset];
      if ( pstate->returncode )
      {
        // Proceed to the next function
        pstate->offset++;
      }
      else
      {
        // Jump to where the jump code says to go
        pstate->offset = iTmp;
      }
    }
    else
    {
      // Get the number of operands
      pstate->offset++;
      operands = slist->buffer[pstate->offset];

      // Now run the operation
      pstate->operationsum = 0;
      pstate->offset++;
      while ( operands > 0 )
      {
        iTmp = slist->buffer[pstate->offset];
        if(rv_error == run_operand( gs, pstate, iTmp )) // This sets pstate->operationsum
        {
          return rv_error;
        };
        operands--;
        pstate->offset++;
      }

      // Save the results in the register that called the arithmetic
      if(rv_error == set_operand( gs, pstate, opcode ))
      {
        return rv_error;
      };
    }

    // This is used by the Else function
    pstate->lastindent = GET_INDENT(opcode);
  }

  assert(pstate->offset <= offset_end);

  // Set latches
  if ( !chr_is_player(gs, iself) && 0 != type_stt )
  {
    CHR_REF rider = chr_get_holdingwhich( gs->ChrList, CHRLST_COUNT, iself, SLOT_SADDLE );

    if ( pself->prop.ismount && ACTIVE_CHR( gs->ChrList, rider ) && !gs->ChrList[rider].prop.isitem )
    {
      // Mount
      pstate->latch.x = gs->ChrList[rider].aistate.latch.x;
      pstate->latch.y = gs->ChrList[rider].aistate.latch.y;
    }
    else
    {


      // Normal AI
      if( wp_list_empty(&(pstate->wp)) )
      {
        pstate->latch.x = pstate->latch.y = 0;
      }
      else
      {
        float fnum, fden;

        pstate->latch.x = wp_list_x( &(pstate->wp) ) - pself->ori.pos.x;
        pstate->latch.y = wp_list_y( &(pstate->wp) ) - pself->ori.pos.y;

        fnum = pstate->latch.x * pstate->latch.x + pstate->latch.y * pstate->latch.y;
        fden = fnum + 25 * pself->bmpdata.calc_size * pself->bmpdata.calc_size;
        if ( fnum > 0.0f )
        {
          float ftmp = 1.0f / sqrt( fnum ) * fnum / fden;
          pstate->latch.x *= ftmp;
          pstate->latch.y *= ftmp;
        }
      };

    }
  }


  // Clear alerts for next time around
  pstate->alert = 0;
  if ( pstate->morphed || pstate->type != type_stt )
  {
    pstate->alert |= ALERT_CHANGED;
    pstate->morphed = bfalse;
  };

  return retval;
}

//--------------------------------------------------------------------------------------------
void run_all_scripts( Game_t * gs, float dUpdate )
{
  /// @details ZZ@> This function lets every computer controlled character do AI stuff

  CHR_REF character;
  bool_t allow_thinking;
  PChr_t chrlst = gs->ChrList;
  Graphics_Data_t * gfx = Game_getGfx( gs );
  AI_STATE * pstate;
  Chr_t * pchr;

  gfx->BlipList_count = 0;
  for ( character = 0; character < CHRLST_COUNT; character++ )
  {
    if ( !ACTIVE_CHR( chrlst, character ) ) continue;

    allow_thinking = bfalse;
    pchr   = chrlst + character;
    pstate = &(pchr->aistate);

    // Cleaned up characters shouldn't be alert to anything else
    if ( HAS_SOME_BITS( pstate->alert, ALERT_CRUSHED ) )
    {
      pstate->alert = ALERT_CRUSHED;
      allow_thinking = btrue;
    }

    if ( HAS_SOME_BITS( pstate->alert, ALERT_CLEANEDUP ) )
    {
      pstate->alert = ALERT_CLEANEDUP;
      allow_thinking = btrue;
    };

    // Do not exclude items in packs. In NetHack, eggs can hatch while in your pack...
    // this may need to be handled differently. Currently dead things are thinking too much...
    if ( !chr_in_pack( chrlst, CHRLST_COUNT, character ) || pchr->alive )
    {
      allow_thinking = btrue;
    }

    if ( allow_thinking )
    {
      run_script( gs, pstate, dUpdate );
    }
  }


}







bool_t scr_break_passage( Game_t * gs, AI_STATE * pstate )
{
  return passage_break_tiles(gs, pstate->tmpargument, pstate->tmpturn, pstate->tmpdistance, pstate->tmpx, pstate->tmpy, &(pstate->tmpx), &(pstate->tmpy) );
}

bool_t scr_search_tile_in_passage( Game_t * gs, AI_STATE * pstate )
{
  return passage_search_tile( gs, pstate->tmpargument, pstate->tmpdistance, pstate->tmpx, pstate->tmpy, &(pstate->tmpx), &(pstate->tmpy) );
}

bool_t scr_ClearWaypoints( Game_t * gs, AI_STATE * pstate )
{
  bool_t btemp = !wp_list_empty(&(pstate->wp));
  wp_list_clear( &(pstate->wp) );
  wp_list_add( &(pstate->wp), pstate->pself->ori.pos.x, pstate->pself->ori.pos.y );
  if(btemp)
  {
    pstate->alert |= ALERT_ATLASTWAYPOINT;
  }
  return btrue;
}

bool_t scr_AddWaypoint( Game_t * gs, AI_STATE * pstate )
{
  bool_t returncode = wp_list_add( &(pstate->wp), pstate->tmpx, pstate->tmpy);
  if(returncode) pstate->alert &= ~ALERT_ATLASTWAYPOINT;
  return returncode;
}

bool_t scr_FindPath( Game_t * gs, AI_STATE * pstate )
{
  // This function adds enough waypoints to get from one point to another
  // And only proceeds if the target is not the character himself

  int src_ix, src_iy;
  int dst_ix, dst_iy;

  Uint32 loc_rand = gs->randie_index;
  bool_t returncode = btrue;
  Chr_t * ptarget = ChrList_getPChr( gs, pstate->target );
  Mesh_t * pmesh  = Game_getMesh( gs );

  // if the src is not on the mesh, do nothing
  if( !mesh_check( &(pmesh->Info), pstate->pself->ori.pos.x, pstate->pself->ori.pos.y) )
  {
    return bfalse;
  }

  src_ix = MESH_FLOAT_TO_FAN(pstate->pself->ori.pos.x);
  src_iy = MESH_FLOAT_TO_FAN(pstate->pself->ori.pos.y);

  // clip the destination
  dst_ix = MESH_FLOAT_TO_FAN( mesh_clip_x(&(pmesh->Info), ptarget->ori.pos.x) );
  dst_iy = MESH_FLOAT_TO_FAN( mesh_clip_y(&(pmesh->Info), ptarget->ori.pos.y) );

  if(src_ix != dst_ix && src_iy != dst_iy)
  {
    bool_t try_astar;

    try_astar = ((pstate->tmpdistance == MOVE_MELEE ) ||
                 (pstate->tmpdistance == MOVE_CHARGE) ||
                 (pstate->tmpdistance == MOVE_FOLLOW)) &&
                  pstate->astar_timer <= SDL_GetTicks();


    if( try_astar && AStar_prepare_path(gs, pstate->pself->stoppedby, src_ix, src_iy, dst_ix, dst_iy) )
    {
      AStar_Node_t nbuffer[4*MAXWAY + 1];
      int        nbuffer_size = 4*MAXWAY + 1;

      nbuffer_size = AStar_get_path(src_ix, src_iy, nbuffer, nbuffer_size);
      if(nbuffer_size > 0)
      {
        nbuffer_size = AStar_Node_list_prune(nbuffer, nbuffer_size);

        if(nbuffer_size<MAXWAY)
        {
          int i;

          wp_list_clear( &(pstate->wp) );

          // copy the buffer into the waypoints
          for(i=nbuffer_size-1; i>=0; i--)
          {
            float fx = MESH_FAN_TO_FLOAT(nbuffer[i].ix + 0.5f);
            float fy = MESH_FAN_TO_FLOAT(nbuffer[i].iy + 0.5f);

            wp_list_add( &(pstate->wp), fx, fy );
          }

          //// optimize the waypoints
          //wp_list_prune(  &(pstate->wp) );
        }

        // limit the rate of AStar calculations to be less than every 10 frames.
        pstate->astar_timer = SDL_GetTicks() +  UPDATESKIP * 10;
      }
    }
    else
    {
      // just use a straight line path

      //First setup the variables needed for the target waypoint
      if(pstate->target != pstate->iself)
      {
        if(pstate->tmpdistance != MOVE_FOLLOW)
        {
          pstate->tmpx = ptarget->ori.pos.x;
          pstate->tmpy = ptarget->ori.pos.y;
        }
        else
        {
          pstate->tmpx = FRAND(&loc_rand) * 512 + ptarget->ori.pos.x;
          pstate->tmpy = FRAND(&loc_rand) * 512 + ptarget->ori.pos.y;
        }

        if(pstate->tmpdistance == MOVE_RETREAT)
        {
          pstate->tmpturn = (IRAND(&loc_rand, 15) + 16384) + pstate->tmpturn;
        }
        else
        {
          pstate->tmpturn = vec_to_turn( pstate->tmpx - pstate->pself->ori.pos.x, pstate->tmpy - pstate->pself->ori.pos.y );
        }

        //Secondly we run the Compass function (If we are not in follow mode)
        if(pstate->tmpdistance != MOVE_FOLLOW)
        {
          pstate->tmpx -= turntocos[( pstate->tmpturn>>2 ) & TRIGTABLE_MASK] * pstate->tmpdistance;
          pstate->tmpy -= turntosin[( pstate->tmpturn>>2 ) & TRIGTABLE_MASK] * pstate->tmpdistance;
        }

        //Then we add the waypoint(s), without clearing existing ones...
        returncode = wp_list_add( &(pstate->wp), pstate->tmpx, pstate->tmpy);
        if(returncode) pstate->alert &= ~ALERT_ATLASTWAYPOINT;
      }
    }

    if((pstate->tmpdistance == MOVE_CHARGE) || (pstate->tmpdistance == MOVE_RETREAT))
    {
      chr_reset_accel( gs, pstate->iself ); //Force 100% speed
    }
  }

  return returncode;
}





bool_t scr_DoAction( Game_t * gs, AI_STATE * pstate )
{
  return _DoAction(gs, pstate->pself, ChrList_getPMad(gs, pstate->iself), pstate->tmpargument);
}

bool_t scr_DropWeapons( Game_t * gs, AI_STATE * pstate )
{
  bool_t returncode = btrue;
  CHR_REF tmpchr;

  for ( _slot = SLOT_BEGIN; _slot < SLOT_COUNT; _slot = ( SLOT )( _slot + 1 ) )
  {
    tmpchr = chr_get_holdingwhich( gs->ChrList, CHRLST_COUNT, pstate->iself, _slot );
    if ( detach_character_from_mount( gs, tmpchr, btrue, btrue ) )
    {
      if ( _slot == SLOT_SADDLE )
      {
        gs->ChrList[tmpchr].ori.vel.z  = DISMOUNTZVEL;
        gs->ChrList[tmpchr].ori.pos.z += DISMOUNTZVEL;
        gs->ChrList[tmpchr].jumptime   = DELAY_JUMP;
      }
    }
  };

  return returncode;
}

bool_t scr_TargetDoAction( Game_t * gs, AI_STATE * pstate )
{
  Chr_t * pchr = ChrList_getPChr(gs, pstate->target);
  Mad_t * pmad = ChrList_getPMad(gs, pstate->target);

  return _DoAction(gs, pchr, pmad, pstate->tmpargument);
}


bool_t scr_CostTargetItemID( Game_t * gs, AI_STATE * pstate )
{
  bool_t returncode = bfalse;

  CHR_REF tmpchr1, tmpchr2, tmpchr3;
  Chr_t * ptarget = ChrList_getPChr(gs, pstate->target);

  // Check the pack
  tmpchr1 = INVALID_CHR;
  tmpchr2 = chr_get_aitarget( gs->ChrList, CHRLST_COUNT, gs->ChrList + pstate->iself );
  tmpchr3 = chr_get_nextinpack( gs->ChrList, CHRLST_COUNT, tmpchr2 );
  while ( ACTIVE_CHR( gs->ChrList, tmpchr3 ) )
  {
    if ( CAP_INHERIT_IDSZ( gs,  ChrList_getRCap(gs, tmpchr3), pstate->tmpargument ) )
    {
      returncode = btrue;
      tmpchr1 = tmpchr3;
      break;
    }
    else
    {
      tmpchr2 = tmpchr3;
      tmpchr3  = chr_get_nextinpack( gs->ChrList, CHRLST_COUNT, tmpchr3 );
    }
  }

  for ( _slot = SLOT_BEGIN; _slot < SLOT_COUNT; _slot = ( SLOT )( _slot + 1 ) )
  {
    tmpchr3 = chr_get_holdingwhich( gs->ChrList, CHRLST_COUNT, pstate->target, _slot );
    if ( ACTIVE_CHR( gs->ChrList, tmpchr3 ) )
    {
      if ( CAP_INHERIT_IDSZ( gs,  ChrList_getRCap(gs, tmpchr3), pstate->tmpargument ) )
      {
        returncode = btrue;
        tmpchr2 = INVALID_CHR;
        tmpchr1 = chr_get_holdingwhich( gs->ChrList, CHRLST_COUNT, pstate->target, _slot );
        break;
      }
    }
  };


  if ( returncode )
  {
    if ( gs->ChrList[tmpchr1].ammo <= 1 )
    {
      // Poof the item
      if ( chr_in_pack(gs->ChrList, CHRLST_COUNT,  tmpchr1 ) )
      {
        // Remove from the pack
        gs->ChrList[tmpchr2].nextinpack  = chr_get_nextinpack( gs->ChrList, CHRLST_COUNT, tmpchr1 );
        ptarget->numinpack--;
        gs->ChrList[tmpchr1].freeme = btrue;
      }
      else
      {
        // Drop from hand
        detach_character_from_mount( gs, tmpchr1, btrue, bfalse );
        gs->ChrList[tmpchr1].freeme = btrue;
      }
    }
    else
    {
      // Cost one ammo
      gs->ChrList[tmpchr1].ammo--;
    }
  }

  return returncode;
}

bool_t scr_DoActionOverride( Game_t * gs, AI_STATE * pstate )
{
  return _DoActionOverride(gs, pstate->pself, ChrList_getPMad(gs, pstate->iself), pstate->tmpargument);
}

bool_t scr_GiveMoneyToTarget( Game_t * gs, AI_STATE * pstate )
{
  int iTmp, tTmp;
  Chr_t * ptarget = ChrList_getPChr(gs, pstate->target);

  iTmp = pstate->pself->money;
  tTmp = ptarget->money;
  iTmp -= pstate->tmpargument;
  tTmp += pstate->tmpargument;
  if ( iTmp < 0 ) { tTmp += iTmp;  pstate->tmpargument += iTmp;  iTmp = 0; }
  if ( tTmp < 0 ) { iTmp += tTmp;  pstate->tmpargument += tTmp;  tTmp = 0; }
  if ( iTmp > MAXMONEY ) { iTmp = MAXMONEY; }
  if ( tTmp > MAXMONEY ) { tTmp = MAXMONEY; }
  pstate->pself->money = iTmp;
  ptarget->money = tTmp;

  return btrue;
}

bool_t scr_SpawnCharacter( Game_t * gs, AI_STATE * pstate )
{
  bool_t returncode = btrue;
  int tTmp;
  CHR_REF tmpchr;

  vect3 chr_pos = VECT3(pstate->tmpx, pstate->tmpy, 0);
  vect3 chr_vel = ZERO_VECT3;

  tTmp = pstate->pself->ori.turn_lr >> 2;
  chr_vel.x = turntocos[( tTmp+8192 ) & TRIGTABLE_MASK] * pstate->tmpdistance;
  chr_vel.y = turntosin[( tTmp+8192 ) & TRIGTABLE_MASK] * pstate->tmpdistance;

  // This function spawns a character, failing if x,y is invalid
  returncode = bfalse;
  tmpchr = chr_spawn( gs, chr_pos, chr_vel, pstate->pself->model, pstate->pself->team, 0, pstate->tmpturn, NULL, INVALID_CHR );
  if ( VALID_CHR( gs->ChrList, tmpchr ) )
  {
    if ( 0 != chr_hitawall( gs, gs->ChrList + tmpchr, NULL ) )
    {
      gs->ChrList[tmpchr].freeme = btrue;
    }
    else
    {
      gs->ChrList[tmpchr].passage       = pstate->pself->passage;
      gs->ChrList[tmpchr].prop.iskursed = bfalse;

      pstate->child                     = tmpchr;
      gs->ChrList[tmpchr].aistate.owner = pstate->owner;

      returncode = btrue;
    }
  }

  return returncode;
}

bool_t scr_SpawnParticle( Game_t * gs, AI_STATE * pstate )
{
  bool_t returncode = bfalse;
  PRT_REF tmpprt;
  CHR_REF tmpchr;

  if ( ACTIVE_CHR( gs->ChrList, pstate->iself ) && !chr_in_pack( gs->ChrList, CHRLST_COUNT, pstate->iself ) )
  {
    tmpchr = pstate->iself;
    if ( chr_attached( gs->ChrList, CHRLST_COUNT, pstate->iself ) )  tmpchr = chr_get_attachedto( gs->ChrList, CHRLST_COUNT, pstate->iself );

    tmpprt = prt_spawn( gs, 1.0f, pstate->pself->ori.pos, pstate->pself->ori.vel, pstate->pself->ori.turn_lr, pstate->pself->model, PIP_REF(pstate->tmpargument), pstate->iself, (GRIP)pstate->tmpdistance, pstate->pself->team, tmpchr, 0, INVALID_CHR );
    if ( RESERVED_PRT( gs->PrtList, tmpprt ) )
    {
      // Detach the particle
      attach_particle_to_character( gs, tmpprt, pstate->iself, pstate->tmpdistance );
      gs->PrtList[tmpprt].attachedtochr = INVALID_CHR;

      // Correct X, Y, Z spacing
      gs->PrtList[tmpprt].ori.pos.x += pstate->tmpx;
      gs->PrtList[tmpprt].ori.pos.y += pstate->tmpy;
      gs->PrtList[tmpprt].ori.pos.z += gs->PipList[gs->PrtList[tmpprt].pip].zspacing.ibase;

      // Don't spawn in walls
      if ( 0 != prt_hitawall( gs, tmpprt, NULL ) )
      {
        gs->PrtList[tmpprt].ori.pos.x = pstate->pself->ori.pos.x;
        if ( 0 != prt_hitawall( gs, tmpprt, NULL ) )
        {
          gs->PrtList[tmpprt].ori.pos.y = pstate->pself->ori.pos.y;
        }
      }
      returncode = btrue;
    }
  }

  return returncode;
}

bool_t scr_RestockTargetAmmoIDAll( Game_t * gs, AI_STATE * pstate )
{
  bool_t returncode = btrue;

  CHR_REF tmpchr;
  int iTmp = 0;  // Amount of ammo given

  for ( _slot = SLOT_LEFT; _slot <= SLOT_RIGHT; _slot = ( SLOT )( _slot + 1 ) )
  {
    tmpchr = chr_get_holdingwhich( gs->ChrList, CHRLST_COUNT, pstate->target, _slot );
    iTmp += restock_ammo( gs, tmpchr, pstate->tmpargument );
  }

  tmpchr  = chr_get_nextinpack( gs->ChrList, CHRLST_COUNT, pstate->target );
  while ( ACTIVE_CHR( gs->ChrList, tmpchr ) )
  {
    iTmp += restock_ammo( gs, tmpchr, pstate->tmpargument );
    tmpchr = chr_get_nextinpack( gs->ChrList, CHRLST_COUNT, tmpchr );
  }

  pstate->tmpargument = iTmp;

  returncode = ( iTmp != 0 );

  return returncode;
}

bool_t scr_RestockTargetAmmoIDFirst( Game_t * gs, AI_STATE * pstate )
{
  CHR_REF tmpchr;

  bool_t returncode = btrue;
  int iTmp = 0;  // Amount of ammo given

  for ( _slot = SLOT_LEFT; _slot <= SLOT_RIGHT; _slot = ( SLOT )( _slot + 1 ) )
  {
    tmpchr = chr_get_holdingwhich( gs->ChrList, CHRLST_COUNT, pstate->target, _slot );
    iTmp += restock_ammo( gs, tmpchr, pstate->tmpargument );
    if ( iTmp != 0 ) break;
  }

  if ( iTmp == 0 )
  {
    tmpchr  = chr_get_nextinpack( gs->ChrList, CHRLST_COUNT, pstate->target );
    while ( ACTIVE_CHR( gs->ChrList, tmpchr ) && iTmp == 0 )
    {
      iTmp += restock_ammo( gs, tmpchr, pstate->tmpargument );
      tmpchr  = chr_get_nextinpack( gs->ChrList, CHRLST_COUNT, tmpchr );
    }
  }

  pstate->tmpargument = iTmp;
  returncode = ( iTmp != 0 );

  return returncode;
}

bool_t scr_ChildDoActionOverride( Game_t * gs, AI_STATE * pstate )
{
  Chr_t * pchr = ChrList_getPChr(gs, pstate->child);
  Mad_t * pmad = ChrList_getPMad(gs, pstate->child);

  return _DoActionOverride(gs, pchr, pmad, pstate->tmpargument);
}
