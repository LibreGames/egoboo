/*******************************************************************************
*  GAME.H                                                                      *
*    - EGOBOO-Game                                                             *     
*                                                                              *
*    - Main functions for running the game                                     *
*      (c) 2013 The Egoboo-Team                                                *
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

#ifndef _GAME_H_
#define _GAME_H_

/*******************************************************************************
* ENUMS 								                                        *
*******************************************************************************/

// Game-Values to set and get by script
enum
{
    GAME_FOG_BOTTOMLEVEL = 1,
    GAME_FOG_TAD,
    GAME_FOG_LEVEL,
    GAME_SHOWPLAYER_POS,
    GAME_SHOWMAP,
    GAME_TIMER
    
} GAME_VALUES;

/*******************************************************************************
* CODE 								                                        *
*******************************************************************************/

void gameMain(char *mod_name, int savegame_no);

#endif  /* #define _GAME_H_ */