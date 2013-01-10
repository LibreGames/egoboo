/*******************************************************************************
*  MENU.H                                                                      *
*      - Create and handle basic menus for EGOBOO                              *
*                                                                              *
*      (c)2013 Paul Mueller <muellerp61@bluewin.ch>                            *
*                                                                              *
*  This program is free software; you can redistribute it and/or modify        *
*  it under the terms of the GNU General Public License as published by        *
*  the Free Software Foundation; either version 2 of the License, or           *
*  (at your option) any later version.                                         *
*                                                                              *
*  This program is distributed in the hope that it will be useful,             *
*  but WITHOUT ANY WARRANTY; without even the implied warranty of              *
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               *
*  GNU Library General Public License for more details.                        *
*                                                                              *
*  You should have received a copy of the GNU General Public License           *
*  along with this program; if not, write to the Free Software                 *
*  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  * 
*******************************************************************************/

#ifndef _MENU_H_
#define _MENU_H_

/*******************************************************************************
* INCLUDES         							                              *
*******************************************************************************/

/******************************************************************************
* DEFINES								      						   *
******************************************************************************/

#define MENU_MAIN          1
#define MENU_CHOOSEMODULE  2
#define MENU_CHOOSEPLAYER  3
#define MENU_OPTIONS       4
#define MENU_SINGLEPLAYER  5
#define MENU_QUIT          6

/*****************************************************************************
* TYPEDEFS    								                             *
*****************************************************************************/

/******************************************************************************
* ROUTINES								      						   *
******************************************************************************/

int  menuMain(char which);

#endif /* _MENU_H_ */

