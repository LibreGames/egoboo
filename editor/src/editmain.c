/*******************************************************************************
*  EDITMAIN.C                                                                  *
*	- Main edit functions for map and other things      	                   *
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
* Last change: 2008-06-21                                                      *
*******************************************************************************/

/*******************************************************************************
* INCLUDES								                                       *
*******************************************************************************/

#include <memory.h>

#include "editor.h"             /* Global needed definitions    */
#include "editfile.h"           /* Load and save map files      */
#include "editdraw.h"           /* Draw anything                */


#include "editmain.h"           /* My own header                */

/*******************************************************************************
* DEFINES								                                       *
*******************************************************************************/

#define EDITFILE_MAX_COMMAND 30

#define EDITFILE_MAX_MAPSIZE 64

/*******************************************************************************
* TYPEDEFS							                                           *
*******************************************************************************/

/*******************************************************************************
* DEFINES								                                       *
*******************************************************************************/

#define EDITMAIN_TILEDIV   128           // SMALLXY

/*******************************************************************************
* DATA							                                               *
*******************************************************************************/

static int NumFreeVertices;
static MESH_T Mesh;
static COMMAND_T *Commands;

/*******************************************************************************
* CODE 								                                           *
*******************************************************************************/

/*
 * Name:
 *     editmainCompleteMapData
 * Description:
 *     Completes loaded / generated map data with additional values needed
 *     for work.
 *       - Set number of fans
 *       - Set size of complete mesh
 *       - Set number of first vertex for fans  
 * Input:
 *     mesh*: Pointer on mesh to handle
 * Output:
 *    None
 */
void editmainCompleteMapData(MESH_T *mesh)
{

    int cnt, vertex_no;


    mesh -> numfan  = mesh -> tiles_x * mesh -> tiles_y;

    mesh -> edgex = (mesh -> tiles_x * EDITMAIN_TILEDIV) - 1;
    mesh -> edgey = (mesh -> tiles_y * EDITMAIN_TILEDIV) - 1;
    mesh -> edgez = 180 * 16;

    /* Now set the number of first vertex for each fan */
    vertex_no = 0;

    for (cnt = 0; cnt < mesh -> numvert; cnt++) {

        mesh -> vrtstart[cnt] = vertex_no;		/* meshvrtstart       */
        mesh -> visible[cnt] = 1;
        vertex_no += Commands[mesh -> fantype[cnt]].numvertices;

    }

    NumFreeVertices = MAXTOTALMESHVERTICES - Mesh.numvert;

}

/*
 * Name:
 *     editmainCreateNewMap
 * Description:
 *     Does the work for editing and sets edit states, if needed 
 * Input:
 *     mesh *: Pointer on mesh  to fill with default values 
 */
static void editmainCreateNewMap(MESH_T *mesh)
{
    
    int x, y;


    memset(mesh, 0, sizeof(MESH_T));

    mesh -> tiles_x = EDITFILE_MAX_MAPSIZE;
    mesh -> tiles_y = EDITFILE_MAX_MAPSIZE;


    mesh -> edgex = (mesh -> tiles_x * EDITMAIN_TILEDIV) - 1;
    mesh -> edgey = (mesh -> tiles_y * EDITMAIN_TILEDIV) - 1;
    mesh -> edgez = 180 * 16;

    mesh -> watershift = 3;

    NumFreeVertices = MAXTOTALMESHVERTICES - Mesh.numvert;
    if (mesh -> tiles_x > 16)   mesh -> watershift++;
    if (mesh -> tiles_x > 32)   mesh -> watershift++;
    if (mesh -> tiles_x > 64)   mesh -> watershift++;
    if (mesh -> tiles_x > 128)  mesh -> watershift++;
    if (mesh -> tiles_x > 256)  mesh -> watershift++;

    /* Fill the map with flag tiles */
    for (y = 0; y < EDITFILE_MAX_MAPSIZE; y++) {

        for (x = 0; x < EDITFILE_MAX_MAPSIZE; x++) {

            /* TODO: Generate empty map */

        }
    
    }



}

/* ========================================================================== */
/* ========================= PUBLIC FUNCTIONS =============================== */
/* ========================================================================== */

/*
 * Name:
 *     editmainInit
 * Description:
 *     Does all initalizations for the editor
 * Input:
 *     None    
 */
void editmainInit(void)
{

    Commands = editdrawInitData();

}

/*
 * Name:
 *     editmainMap
 * Description:
 *      Does the work for editing and sets edit states, if needed 
 * Input:
 *      command: What to do    
 */
void editmainMap(int command)
{

    switch(command) {

        case EDITMAIN_DRAWMAP:
            editdraw3DView(&Mesh);
            break;

        case EDITMAIN_NEWMAP:
            editmainCreateNewMap(&Mesh);
            editmainCompleteMapData(&Mesh);
            break;

        case EDITMAIN_LOADMAP:
            memset(&Mesh, 0, sizeof(MESH_T));
            editfileLoadMapMesh(&Mesh);
            editmainCompleteMapData(&Mesh);
            break;

        case EDITMAIN_SAVEMAP:
            editfileSaveMapMesh(&Mesh);
            break;

    }

}