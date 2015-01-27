/*******************************************************************************
*  EDITOR.H                                                                    *
*	- General definitions for the editor                	                     *
*									                                       *
*      (c) 2010-2013 Paul Mueller <muellerp61@bluewin.ch>                      *
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
*                                                                              *
*                                                                              *
*******************************************************************************/

#ifndef _EDITOR_H_
#define _EDITOR_H_

/*******************************************************************************
* DEFINES								                                       *
*******************************************************************************/

#define MAXLEVEL 255
#define MINLEVEL 50

#define MAXTILE 256             //

#define MAXSELECT 2560          // Max points that can be select_vertsed

#define FADEBORDER 64           // Darkness at the edge of map
#define ONSIZE 264              // Max size of raise mesh

// Editor modes: How to draw the map
#define EDIT_DRAWMODE_SOLID     0x01        /* Draw solid, yes/no       */
#define EDIT_DRAWMODE_TEXTURED  0x02        /* Draw textured, yes/no    */
#define EDIT_DRAWMODE_LIGHTMAX  0x04        /* Is drawn all white       */  

/*******************************************************************************
* TYPEDEFS							                                           *
*******************************************************************************/

/*******************************************************************************
* CODE 								                                           *
*******************************************************************************/

#endif /* _EDITOR_H_	*/

