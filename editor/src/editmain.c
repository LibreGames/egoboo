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
#define DEFAULT_TILE         62   

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
 *     editmainAddFan
 * Description:
 *     This function allocates the vertices needed for a fan
 *     -- TODO: Split this one into 'AddFan' and 'ReplaceFan' 
 * Input:
 *     mesh*: Pointer on mesh to handle
 *     fan:   Number of fan to allocate vertices for
 *     x, y:  Position of fan   
 * Output:
 *    None
 */
static int editmainAddFan(MESH_T *mesh, int fan, int x, int y)
{
     
    int cnt;
    int fan_type;
    int numvert;
    int vertex;
    int vertexlist[17];
    

    /* 1. Get number of vertices of existing fan    */
    fan_type = mesh -> fan[fan].type;
    numvert  = Commands[fan_type].numvertices;
    
    /* 1. Remove old fan (count it's vertices)  */
    /* 2. Replace it by fan with new type  ?    */      
    
    /* mesh -> vrta[vertexlist[cnt]] = 60; --- FOR each Vertex --- */
    /* 
    if (NumFreeVertices >= numvert)
    {
        mesh -> fan[fan].fx = MPDFX_SHA;
        for (cnt = 0; cnt < numvert; cnt++) {
        
            if (get_free_vertex() == bfalse)
            {
                // Reset to unused
                numvert = cnt;
                for (cnt = 0; cnt < numvert;  cnt++) {
                    mesh.vrta[vertexlist[cnt]] = 60;

                }
                return 0;
            }
            vertexlist[cnt] = atvertex;
            
        }
        vertexlist[cnt] = CHAINEND;

        for (cnt = 0; cnt < numvert; cnt++;) {
            vertex = vertexlist[cnt];
            mesh.vrtx[vertex] = x + (Commands[fan_type].x[cnt] >> 2);
            mesh.vrty[vertex] = y + (Commands[fan_type].y[cnt] >> 2);
            mesh.vrtz[vertex] = 0;
            mesh.vrtnext[vertex] = vertexlist[cnt+1];

        }
        mesh.vrtstart[fan] = vertexlist[0];
        NumFreeVertices -= numvert;
        return 1;
    }
    */
    return 0;
}

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

    int fan_no, vertex_no;


    mesh -> numfan  = mesh -> tiles_x * mesh -> tiles_y;

    mesh -> edgex = (mesh -> tiles_x * EDITMAIN_TILEDIV) - 1;
    mesh -> edgey = (mesh -> tiles_y * EDITMAIN_TILEDIV) - 1;
    mesh -> edgez = 180 * 16;
    
    mesh -> watershift = 3;
    if (mesh -> tiles_x > 16)   mesh -> watershift++;
    if (mesh -> tiles_x > 32)   mesh -> watershift++;
    if (mesh -> tiles_x > 64)   mesh -> watershift++;
    if (mesh -> tiles_x > 128)  mesh -> watershift++;
    if (mesh -> tiles_x > 256)  mesh -> watershift++;

    /* Now set the number of first vertex for each fan */
    for (fan_no = 0, vertex_no = 0; fan_no < mesh -> numfan; fan_no++) {

        mesh -> vrtstart[fan_no] = vertex_no;		/* meshvrtstart       */
        mesh -> visible[fan_no]  = 1;
        vertex_no += Commands[mesh -> fan[fan_no].type & 0x1F].numvertices;

    }

    NumFreeVertices = MAXTOTALMESHVERTICES - mesh -> numvert;
    /* Set flag that map has been loaded */
    mesh -> map_loaded = 1;

}

/*
 * Name:
 *     editmainCreateNewMap
 * Description:
 *     Does the work for editing and sets edit states, if needed
 * Input:
 *     mesh *: Pointer on mesh  to fill with default values
 */
static int editmainCreateNewMap(MESH_T *mesh)
{

    int x, y, fan;


    memset(mesh, 0, sizeof(MESH_T));

    mesh -> tiles_x = EDITFILE_MAX_MAPSIZE;
    mesh -> tiles_y = EDITFILE_MAX_MAPSIZE;


    /* Fill the map with flag tiles */
    for (y = 0; y < mesh -> tiles_y; y++) {

        for (x = 0; x < mesh -> tiles_x; x++) {

            /* TODO: Generate empty map */
            mesh -> fan[fan].type    = 0;
            mesh -> fan[fan].tx_bits = (((x & 1) + (y & 1)) & 1) + DEFAULT_TILE;

            /* TODO:
            if ( !editmainAddFan(fan, x*TILEDIV, y*TILEDIV) )
            {
                printf("NOT ENOUGH VERTICES!!!\n\n");
                exit(-1);
            }
            */
            fan++;

        }

    }

    return 0;   /* TODO: Return 1 if mesh is created */

}

static int editfileSetVrta(MESH_T *mesh, int vert)
{
    /* TODO: Get all needed functions from cartman code */
    /*
    int newa, x, y, z, brx, bry, brz, deltaz, dist, cnt;
    int newlevel, distance, disx, disy;

    // To make life easier
    x = mesh -> vrtx[vert];
    y = mesh -> vrty[vert];
    z = mesh -> vrtz[vert];

    // Directional light
    brx = x + 64;
    bry = y + 64;
    brz = get_level(brx, y) +
          get_level(x, bry) +
          get_level(x + 46, y + 46);
    if (z < -128) z = -128;
    if (brz < -128) brz = -128;
    deltaz = z + z + z - brz;
    newa = (deltaz * direct >> 8);

    // Point lights !!!BAD!!!
    newlevel = 0;
    cnt = 0;
    while (cnt < numlight)
    {
        disx = x - light_lst[cnt].x;
        disy = y - light_lst[cnt].y;
        distance = sqrt(disx * disx + disy * disy);
        if (distance < light_lst[cnt].radius)
        {
            newlevel += ((light_lst[cnt].level * (light_lst[cnt].radius - distance)) / light_lst[cnt].radius);
        }
        cnt++;
    }
    newa += newlevel;

    // Bounds
    if (newa < -ambicut) newa = -ambicut;
    newa += ambi;
    if (newa <= 0) newa = 1;
    if (newa > 255) newa = 255;
    mesh -> vrta[vert] = newa;

    // Edge fade
    dist = dist_from_border(mesh  -> vrtx[vert], mesh -> vrty[vert]);
    if (dist <= FADEBORDER)
    {
        newa = newa * dist / FADEBORDER;
        if (newa == VERTEXUNUSED)  newa = 1;
        mesh  ->  vrta[vert] = newa;
    }

    return newa;
    */
    return 60;
}

/*
 * Name:
 *     editmainCalcVrta
 * Description:
 *     Generates the 'a'  numbers for all files
 * Input:
 *     mesh*: Pointer on mesh to set twists for
 */
static void editmainCalcVrta(MESH_T *mesh)
{
    int num, cnt;
    int vert;
    int fan;


    for (fan = 0; fan < mesh -> numfan; fan++) {

        vert = mesh -> vrtstart[fan];
        num  = Commands[mesh -> fan[fan].type].numvertices;
        
        for (cnt = 0; cnt < num; cnt++) {
        
            editfileSetVrta(mesh, vert);
            vert++;
            
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
 *     editmainExit
 * Description:
 *     Frees all data set an initialized by init code
 * Input:
 *     None
 */
void editmainExit(void)
{

    /* Free all data initialized by draw code */
    editdrawFreeData();

}

/*
 * Name:
 *     editmainMap
 * Description:
 *      Does the work for editing and sets edit states, if needed 
 * Input:
 *      command: What to do
 * Output:
 *      Result of given command
 */
int editmainMap(int command)
{

    switch(command) {

        case EDITMAIN_DRAWMAP:
            editdraw3DView(&Mesh);
            return 1;

        case EDITMAIN_NEWMAP:
            if (editmainCreateNewMap(&Mesh)) {

                editmainCompleteMapData(&Mesh);
                return 1;

            }
            break;

        case EDITMAIN_LOADMAP:
            memset(&Mesh, 0, sizeof(MESH_T));
            if (editfileLoadMapMesh(&Mesh)) {

                editmainCompleteMapData(&Mesh);

                return 1;

            }
            break;

        case EDITMAIN_SAVEMAP:
            editmainCalcVrta(&Mesh);
            return editfileSaveMapMesh(&Mesh);

    }

    return 0;

}