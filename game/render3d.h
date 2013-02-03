/*******************************************************************************
*  RENDER3D.H                                                                  *
*    - EGOBOO-Game                                                             *
*                                                                              *
*    - All functionality to render the 3D-Part of the game                     *
*      (c) 2013 The Egoboo Team                                                *
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

#ifndef _RENDER3D_H_
#define _RENDER3D_H_

/*******************************************************************************
* INCLUDES                                                                     *
*******************************************************************************/

#include "egofile.h"        // MESH_T;

/*******************************************************************************
* DEFINES                                                                      *
*******************************************************************************/

#define RENDER3D_SOLID      0x01
#define RENDER3D_LIGHTMAX   0x02    // Draw the map full lighted
#define RENDER3D_TEXTURED   0x04

/*******************************************************************************
* CODE                                                                         *
*******************************************************************************/

void render3dLoad(char globals);
void render3dCleanup(char globals);
void render3dMain(MESH_T *pmesh);

#endif  /* #define _RENDER3D_H_ */