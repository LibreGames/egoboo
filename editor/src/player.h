/*******************************************************************************
*  PLAYER.H                                                                  *
*    - EGOBOO-Editor                                                           *     
*                                                                              *
*    - [...]                                                                   *
*      (c) 2013 Paul Mueller <muellerp61@bluewin.ch>                           *
*                                                                              *
*   This program is free software; you can redistribute it and/or modify       *
*   it under the terms of the GNU General Public License as published by       *
*   the Free Software Foundation; either version 2 of the License, or          *
*   (at your option) any later version.                                        *
*                                                                              *
*   This program is distributed in the hope that it will be useful,            *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of             *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
*   GNU Library General Public License for more details.                       *
*                                                                              *
*   You should have received a copy of the GNU General Public License          *
*   along with this program; if not, write to the Free Software                *
*   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. *
*******************************************************************************/

#ifndef _PLAYER_H_
#define _PLAYER_H_

/*******************************************************************************
* INCLUDES 							                                        *
*******************************************************************************/

#include "idsz.h"

/*******************************************************************************
* DEFINES   							                                        *
*******************************************************************************/

/* -------- Game codes ---------------- */
#define EGOCODE_NOINPUT	        0
#define EGOCODE_JUMP            1
#define EGOCODE_LEFT_USE        2
#define EGOCODE_LEFT_GET        3
#define EGOCODE_LEFT_PACK       4
#define EGOCODE_RIGHT_USE       5
#define EGOCODE_RIGHT_GET       6
#define EGOCODE_RIGHT_PACK      7
#define EGOCODE_RESPAWN	        8
#define EGOCODE_MESSAGE         9
#define EGOCODE_CAMERA_LEFT    10
#define EGOCODE_CAMERA_RIGHT   11
#define EGOCODE_CAMERA_IN      12
#define EGOCODE_CAMERA_OUT     13
#define EGOCODE_ARROWUP        14
#define EGOCODE_ARROWDOWN      15
#define EGOCODE_ARROWLEFT      16
#define EGOCODE_ARROWRIGHT     17
#define EGOCODE_CAMERAMODE     18
#define EGOCODE_MOVEMENT       19
#define EGOCODE_SNEAK_ON       20
#define EGOCODE_SNEAK_OFF      21  

/* LEFT SHIFT   + 1 to 7 - Show selected character armor without magic enchants */
/* LEFT CONTROL + 1 to 7 - Show armor stats with magic enchants included */
/* LEFT ALT     + 1 to 7 - Show character magic enchants    */
#define EGOCODE_SHOWSTAT        25
// General debug and other codes
#define EGOCODE_GIVEEXPERIENCE 27	// Cheat for program test
#define EGOCODE_SCREENSHOT     28
#define EGOCODE_ESCAPE	       29
#define EGOCODE_HELPMOUSE      31	// SDLK_F1
#define EGOCODE_HELPJOYSTICK   32	// SDLK_F2
#define EGOCODE_HELPKEYBOARD   33	// SDLK_F3
#define EGOCODE_DEBUGMODE5     34	// SDLK_F5
#define EGOCODE_DEBUGMODE6     35	// SDLK_F6
#define EGOCODE_DEBUGMODE7     36	// SDLK_F7 <-> Switch textures??

#define EGOCODE_MAPCHEAT       50
#define EGOCODE_LIFECHEAT      51

/* --- Additional codes --- */
#define EGOCODE_UPDATETIMER 100
#define EGOCODE_QUITMODULE  101
#define EGOCODE_PAUSEGAME   102
#define EGOCODE_QUITTOOS    103

// Number of players
#define PLAYER_MAX 6

/*******************************************************************************
* TYPEDEFS   							                                   *
*******************************************************************************/

/// The state of a player
typedef struct 
{
    char    id;         ///< Player used? : Number of player    
    char    valid;      ///< Player used?         
    int     char_no;    ///< Which character to handle ?
    int     obj_no;     ///< Which 3D-Object to move ?
    // inventory stuff
    char    inventory_slot;     ///< Actual chosen inventory slot
    char    draw_inventory;     ///<draws the inventory
    int     inventory_cooldown; ///< In Ticks
    int     inventory_lerp;
    
    /// Local latch, set by set_one_player_latch(), read by sv_talkToRemotes()
    int     ego_cmd;            /// Next EGOCODE_... input for next frame    

    // quest log for this player
    IDSZ_T  quest_log[MAX_IDSZ_MAP_SIZE];          ///< lists all the character's quests

} PLAYER_T;

/*******************************************************************************
* CODE 								                                       *
*******************************************************************************/

// SDLGL_CMDKEY *playerInitInput(void);
char playerCreate(char player_no, int char_no);
void playerRemove(char player_no);
// Game commands
int  playerTranslateInput(char code, char sub_code, char pressed, int obj_no, int keymod);

#endif  /* #define _PLAYER_H_ */