/*******************************************************************************
*  WALLMAKE.H                                                                  *
*	- Draws anithing what's 3D        	                                       *
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
*******************************************************************************/

#ifndef _WALLMAKE_H_
#define _WALLMAKE_H_

/*******************************************************************************
* INCLUDES								                                       *
*******************************************************************************/

/*******************************************************************************
* DEFINES								                                       *
*******************************************************************************/

/* Now the tile numbers used by the wallmaker... */
#define WALLMAKE_FLOOR  ((char)0)
#define WALLMAKE_TOP    ((char)1)
#define WALLMAKE_WALL   ((char)8 +32)       /* Big textures */
#define WALLMAKE_EDGEO  ((char)16+32)       /* Big textures */
#define WALLMAKE_EDGEI  ((char)19+32)       /* Big textures */

#define WALLMAKE_NORTH  0x00
#define WALLMAKE_EAST   0x01
#define WALLMAKE_SOUTH  0x02
#define WALLMAKE_WEST   0x03

/*******************************************************************************
* TYPEDEFS								                                       *
*******************************************************************************/

typedef struct {

    int  fan_no;    /* Number of fan (-1: off map)          */
    char type;      /* Actual/new type of fan               */
    char dir;       /* Direction the fan has to rotated to  */   
    
} WALLMAKER_INFO_T;

/*******************************************************************************
* CODE 								                                           *
*******************************************************************************/

int wallmakeMakeTile(int is_floor, WALLMAKER_INFO_T *wi);

#endif  /* _WALLMAKE_H_ */