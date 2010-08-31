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

/* --------- Info for preset tiles ------- */
#define WALLMAKE_PRESET_FLOOR   0
#define WALLMAKE_PRESET_TOP     1
#define WALLMAKE_PRESET_WALL    2
#define WALLMAKE_PRESET_EDGEO   3
#define WALLMAKE_PRESET_EDGEI   4
#define WALLMAKE_PRESET_MAX     4

#define WALLMAKE_NORTH  0x00
#define WALLMAKE_EAST   0x01
#define WALLMAKE_SOUTH  0x02
#define WALLMAKE_WEST   0x03

/*******************************************************************************
* TYPEDEFS								                                       *
*******************************************************************************/

typedef struct {

    int  pos;       /* Position of fan                      */
    char type;      /* Old fan type                         */
    char dir;       /* Direction the fan has to rotated to  */   
    char new_type;  /* To be filled by wallmaker            */
    char new_dir;   /* To be filled by wallmaker            */
    
} WALLMAKER_INFO_T;

/*******************************************************************************
* CODE 								                                           *
*******************************************************************************/

int wallmakeMakeTile(int fan, int is_floor, WALLMAKER_INFO_T *wi);

#endif  /* _WALLMAKE_H_ */