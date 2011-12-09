/*******************************************************************************
*  EDITOR.H                                                                    *
*	- General definitions for the editor                	                   *
*									                                           *
*   Copyright (C) 2010  Paul Mueller <pmtech@swissonline.ch>                   *
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

#define COMMAND_TEXTUREHI_FLAG 0x20

// Editor modes: How to draw the map
#define EDIT_DRAWMODE_SOLID     0x01        /* Draw solid, yes/no       */
#define EDIT_DRAWMODE_TEXTURED  0x02        /* Draw textured, yes/no    */
#define EDIT_DRAWMODE_LIGHTMAX  0x04        /* Is drawn all white       */    

/* --- different edit modes --- */
#define EDITOR_TOOL_OFF     ((char)1)   /* 'View' map                       */
#define EDITOR_TOOL_MAP     ((char)2)   /* 'Carve' out map with mouse       */
                                        /* EDITMAIN_EDIT_SIMPLE             */
#define EDITOR_TOOL_FAN     ((char)3)   /* Edit fan geometry (base -types)  */
                                        /* EDITMAIN_EDIT_FREE               */
#define EDITOR_TOOL_FAN_FX  ((char)4)   /* Edit the flags of a fan          */
                                        /* EDITMAIN_EDIT_FX                 */   
#define EDITOR_TOOL_FAN_TEX ((char)5)   /* Texture of a fan                 */ 
                                        /* EDITMAIN_EDIT_TEXTURE            */
#define EDITOR_TOOL_PASSAGE ((char)6)   
#define EDITOR_TOOL_OBJECT  ((char)7)
#define EDITOR_TOOL_MODULE  ((char)8)   /* Change info in module description */
#define EDITOR_TOOL_VERTEX  ((char)9)   /* Edit vertices of a fan            */

/*******************************************************************************
* TYPEDEFS							                                           *
*******************************************************************************/

/*******************************************************************************
* CODE 								                                           *
*******************************************************************************/

#endif /* _EDITOR_H_	*/

