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
static COMMAND_T *pCommands;
static SPAWN_OBJECT_T SpawnObjects[EDITMAIN_MAXSPAWN + 2];
static EDITMAIN_STATE_T EditState;

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
    numvert  = pCommands[fan_type].numvertices;
    
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
            mesh.vrtx[vertex] = x + (pCommands[fan_type].x[cnt] >> 2);
            mesh.vrty[vertex] = y + (pCommands[fan_type].y[cnt] >> 2);
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
        vertex_no += pCommands[mesh -> fan[fan_no].type & 0x1F].numvertices;

    }

    NumFreeVertices = MAXTOTALMESHVERTICES - mesh -> numvert;
    /* Set flag that map has been loaded */
    mesh -> map_loaded = 1;
    mesh -> draw_mode  = EditState.draw_mode;

}

/*
 * Name:
 *     editmainCreateNewMap
 * Description:
 *     Does the work for editing and sets edit states, if needed
 * Input:
 *     mesh *: Pointer on mesh  to fill with default values
 *     which:  Type of map to create : EDITMAIN_NEWFLATMAP / EDITMAIN_NEWSOLIDMAP 
 */
static int editmainCreateNewMap(MESH_T *mesh, int which)
{

    int x, y, fan;


    memset(mesh, 0, sizeof(MESH_T));

    mesh -> tiles_x = EDITFILE_MAX_MAPSIZE;
    mesh -> tiles_y = EDITFILE_MAX_MAPSIZE;


    if (which == EDITMAIN_NEWFLATMAP) {
    
    }
    else if (which == EDITMAIN_NEWSOLIDMAP) {
    
    
    }
    else {
    
    }
    /* Fill the map with flag tiles */
    fan = 0;
    for (y = 0; y < mesh -> tiles_y; y++) {

        for (x = 0; x < mesh -> tiles_x; x++) {

            /* TODO: Generate empty map */
            mesh -> fan[fan].type  = 0;
            mesh -> fan[fan].tx_no = (char)((((x & 1) + (y & 1)) & 1) + DEFAULT_TILE);
            
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

/*
 * Name:
 *     editfileSetVrta
 * Description:
 *     Does the work for editing and sets edit states, if needed
 * Input:
 *     mesh *: Pointer on mesh  to handle
 *     vert:   Number of vertex to set 
 */
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
        num  = pCommands[mesh -> fan[fan].type].numvertices;
        
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
 * Output:
 *     Pointer on EditState
 */
EDITMAIN_STATE_T *editmainInit(void)
{

    pCommands = editdrawInitData();
     
    memset(&EditState, 0, sizeof(EDITMAIN_STATE_T));

    EditState.display_flags |= EDITMAIN_SHOW2DMAP;
    EditState.fan_chosen    = -1;   /* No fan chosen                        */
    EditState.brush_size    = 3;    /* Size of raise/lower terrain brush    */
    EditState.brush_amount  = 50;   /* Amount of raise/lower                */

    EditState.draw_mode     = (EDIT_MODE_SOLID | EDIT_MODE_TEXTURED | EDIT_MODE_LIGHTMAX);

    return &EditState;
    
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
            editdraw3DView(&Mesh, EditState.fan_chosen);
            return 1;

        case EDITMAIN_NEWFLATMAP:
        case EDITMAIN_NEWSOLIDMAP:
            if (editmainCreateNewMap(&Mesh, EDITMAIN_NEWFLATMAP)) {

                editmainCompleteMapData(&Mesh);
                return 1;

            }
            break;

        case EDITMAIN_LOADMAP:
            memset(&Mesh, 0, sizeof(MESH_T));
            if (editfileLoadMapMesh(&Mesh, EditState.msg)) {

                editmainCompleteMapData(&Mesh);

                return 1;

            }
            break;

        case EDITMAIN_SAVEMAP:
            editmainCalcVrta(&Mesh);
            return editfileSaveMapMesh(&Mesh, EditState.msg);

    }

    return 0;

}

/*
 * Name:
 *     editmainDrawMap2D
 * Description:
 *      Draws the map as 2D-Map into given rectangle
 * Input:
 *      command: What to do
 * Output:
 *      Result of given command
 */
void editmainDrawMap2D(int x, int y, int w, int h)
{ 
    
    editdraw2DMap(&Mesh, x, y, w, h, EditState.fan_chosen);

}

/*
 * Name:
 *     editmainLoadSpawn
 * Description:
 *     Load the 'spawn.txt' list into given list
 * Input:
 *     None
 * Output:
 *     spawn_list *: Pointer on list of objects to be spawned
 */
SPAWN_OBJECT_T *editmainLoadSpawn(void)
{

    memset(SpawnObjects, 0, EDITMAIN_MAXSPAWN * sizeof(SPAWN_OBJECT_T));

    /* TODO: Load this list, if available */

    return SpawnObjects;

}

/*
 * Name:
 *     editmainToggleFlag
 * Description:
 *     Toggles the given flag using 'EditInfo'.
 *     Adjust accompanied
 * Input:
 *     edit_state *: To return the toggle states to caller
 *     which:        Which flag to change
 *
 */
void editmainToggleFlag(int which, unsigned char flag)
{

    switch(which) {            

        case EDITMAIN_TOGGLE_DRAWMODE:
            /* Change it in actual map */
            Mesh.draw_mode ^= flag;
            /* Show it in edit_state */
            EditState.draw_mode = Mesh.draw_mode;
            break;

        case EDITMAIN_TOGGLE_FX:
            if (EditState.fan_chosen >= 0 && EditState.fan_chosen < Mesh.numfan){
                /* Toggle it in chosen fan */
                Mesh.fan[EditState.fan_chosen].fx ^= flag;
                /* Now copy the actual state for display    */
                EditState.act_fan.fx = Mesh.fan[EditState.fan_chosen].fx;
            }
            else {
                EditState.act_fan.fx = 0;
            }
            break;

        case EDITMAIN_TOGGLE_TXHILO:
            if (EditState.fan_chosen >= 0) {
                Mesh.fan[EditState.fan_chosen].tx_flags ^= flag;
                /* And copy it for display */
                EditState.act_fan.tx_flags = Mesh.fan[EditState.fan_chosen].tx_flags;
            }
            break;

    }

}

/*
 * Name:
 *     editmainChooseFan
 * Description:
 *     Choose a fan from given position in rectangle of size w/h.
 *     Does an update on the 'EditState'
 * Input:
 *     cx, cy: Position chosen
 *     w, h:   Extent of rectangle
 */
void editmainChooseFan(int cx, int cy, int w, int h)
{

    int fan_no;
    

    /* Save it as x/y-position, too */
    EditState.tile_x = Mesh.tiles_x * cx / w;
    EditState.tile_y = Mesh.tiles_y * cy / h;
    
    fan_no = (EditState.tile_y * Mesh.tiles_x) + EditState.tile_x;
    
    if (fan_no >= 0 && fan_no < Mesh.numfan) {
    
        EditState.fan_chosen = fan_no;
        /* And fill it into 'EditState' for display */
        memcpy(&EditState.act_fan, &Mesh.fan[fan_no], sizeof(FANDATA_T));
    
    }
 
}