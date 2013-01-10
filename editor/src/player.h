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

#define PLAYER_MAX 6

/*******************************************************************************
* TYPEDEFS   							                                   *
*******************************************************************************/

/// The state of a player
typedef struct 
{
    char    id;         ///< Player used? : Number of player    
    char    valid;      ///< Player used?         
    int     char_no;    ///< Which character?
    int     obj_no;     ///< Which 3D-Object 
    // inventory stuff
    char    inventory_slot; ///< Actual chosen inventory slot
    char    draw_inventory; ///<draws the inventory
    int     inventory_cooldown; ///< In Ticks
    int     inventory_lerp;

    /// Local latch, set by set_one_player_latch(), read by sv_talkToRemotes()
    unsigned int local_latch;

    // quest log for this player
    IDSZ_T  quest_log[MAX_IDSZ_MAP_SIZE];          ///< lists all the character's quests

} PLAYER_T;

/*******************************************************************************
* CODE 								                                       *
*******************************************************************************/


#endif  /* #define _PLAYER_H_ */