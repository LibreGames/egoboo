/*******************************************************************************
*  EDITFILE.C                                                                  *
*	- Load and write files for the editor	                                   *
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

/*******************************************************************************
* INCLUDES								                                       *
*******************************************************************************/

#include "editfile.h"     /* Own header */

/*******************************************************************************
* DEFINES								                                       *
*******************************************************************************/

/*******************************************************************************
* CODE 								                                           *
*******************************************************************************/

/* ========================================================================= */
/* ============================= THE PUBLIC ROUTINES ======================= */
/* ========================================================================= */

/*
 * Name:
 *     editfileLoadMapMesh
 * Description:
 *     Loads the map mesh into the data pointed on 
 * Input:
 *     mesh *: Pointer on mesh to load 
 */
int editfileLoadMapMesh(MESH_T *mesh)
{
	return 0;			//todo
}

/*
 * Name:
 *     editfileLoadMapMesh
 * Description:
 *     Saves the mesh in the data pointed on.
 *     Name of the save file is always 'level.mpd' 
 * Input:
 *     mesh *: Pointer on mesh to save 
 */
int editfileSaveMapMesh(MESH_T *mesh)
{
	return 0;		//todo
}
