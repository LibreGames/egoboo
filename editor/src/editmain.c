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

#include <stdio.h>
#include <memory.h>
#include <math.h>

#include "sdlgl3d.h"            /* DEG2RAD                              */
#include "sdlglcfg.h"           /* Reading passages and spawn points    */
#include "editor.h"             /* Global needed definitions            */
#include "editfile.h"           /* Load and save map files              */
#include "editdraw.h"           /* Draw anything                        */
#include "wallmake.h"           /* Create walls/floors simple           */


#include "editmain.h"           /* My own header                */

/*******************************************************************************
* DEFINES								                                       *
*******************************************************************************/

#define EDITMAIN_MAX_COMMAND 30

#define EDITMAIN_DEFAULT_TILE   ((char)1)
#define EDITMAIN_TOP_TILE       ((char)63)  /* Black texture    */
#define EDITMAIN_WALL_HEIGHT    192
#define EDITMAIN_TILEDIV        128         /* Size of tile     */

#define EDITMAIN_PRESET_MAX 4

#define EDITMAIN_NORTH  0x00
#define EDITMAIN_EAST   0x01
#define EDITMAIN_SOUTH  0x02
#define EDITMAIN_WEST   0x03

#define EDITMAIN_MAXSPAWN    500        /* Maximum Lines in spawn list  */
#define EDITMAIN_MAXPASSAGE   50

#define EDITMAIN_MAXLIGHT    100        /* Maximum number of lights     */  
#define EDITMAIN_FADEBORDER   64        /* Darkness at the edge of map  */
  
/*******************************************************************************
* DATA							                                               *
*******************************************************************************/

/* ----------- Lights ---------- */
static int LightAmbi    = 22;
static int LightAmbiCut = 1;
static int LightDirect  = 16;
static int NumLight     = 0;
static LIGHT_T LightList[EDITMAIN_MAXLIGHT + 2];

static MESH_T Mesh;
static COMMAND_T *pCommands;

static EDITMAIN_STATE_T EditState;

static PassageFan[EDITMAIN_MAXSELECT + 2];  /* List of fans of actual chosen passage */
static SpawnFan[EDITMAIN_MAXSELECT + 2];    /* List of fans of actual chosen spawn point */

/* -------------Data for Spawn-Points ---------------- */
static EDITMAIN_SPAWNPT_T SpawnObjects[EDITMAIN_MAXSPAWN + 2];

static SDLGLCFG_VALUE SpawnVal[] = {
	{ SDLGLCFG_VAL_STRING,  &SpawnObjects[0].line_name, 24 },
	{ SDLGLCFG_VAL_STRING,  &SpawnObjects[0].item_name, 20 },
	{ SDLGLCFG_VAL_INT,     &SpawnObjects[0].slot_no },
	{ SDLGLCFG_VAL_FLOAT,   &SpawnObjects[0].x_pos },
	{ SDLGLCFG_VAL_FLOAT,   &SpawnObjects[0].y_pos },
	{ SDLGLCFG_VAL_FLOAT,   &SpawnObjects[0].z_pos },
	{ SDLGLCFG_VAL_ONECHAR, &SpawnObjects[0].view_dir },
	{ SDLGLCFG_VAL_INT,     &SpawnObjects[0].money },
	{ SDLGLCFG_VAL_INT,     &SpawnObjects[0].skin },
	{ SDLGLCFG_VAL_INT,     &SpawnObjects[0].pas },
	{ 0 }
};

static SDLGLCFG_LINEINFO SpawnRec = {
	&SpawnObjects[0],
	EDITMAIN_MAXSPAWN,
	sizeof(EDITMAIN_SPAWNPT_T),
	&SpawnVal[0]
};

/* ------------ Data for passages -------------------- */
static EDITMAIN_PASSAGE_T Passages[EDITMAIN_MAXPASSAGE + 2];

static SDLGLCFG_VALUE PassageVal[] = {
	{ SDLGLCFG_VAL_STRING,  &Passages[0].line_name, 24 },
	{ SDLGLCFG_VAL_INT,     &Passages[0].topleft.x },
	{ SDLGLCFG_VAL_INT,     &Passages[0].topleft.y },
	{ SDLGLCFG_VAL_INT,     &Passages[0].bottomright.x },
	{ SDLGLCFG_VAL_INT,     &Passages[0].bottomright.y },
	{ SDLGLCFG_VAL_ONECHAR, &Passages[0].open },
	{ SDLGLCFG_VAL_ONECHAR, &Passages[0].shoot_trough },
	{ SDLGLCFG_VAL_ONECHAR, &Passages[0].slippy_close },
	{ 0 }
};

static SDLGLCFG_LINEINFO PassageRec = {
	&Passages[0],
    EDITMAIN_MAXPASSAGE,
	sizeof(EDITMAIN_PASSAGE_T),
	&PassageVal[0]
};

/* --- Definition of preset tiles for 'simple' mode -- */
static FANDATA_T PresetTiles[] = {

    {  EDITMAIN_DEFAULT_TILE, 0, 0,  WALLMAKE_FLOOR },
    {  EDITMAIN_TOP_TILE,     0, (MPDFX_WALL | MPDFX_IMPASS), WALLMAKE_TOP },
    /* Walls, x/y values are rotated, if needed */
    {  63 + 33, 0, (MPDFX_WALL | MPDFX_IMPASS), WALLMAKE_WALL  },   /* Wall north            */
    {  63 + 51, 0, (MPDFX_WALL | MPDFX_IMPASS), WALLMAKE_EDGEO },   /* Outer edge north/east */
    {  63 +  1, 0, (MPDFX_WALL | MPDFX_IMPASS), WALLMAKE_EDGEI },   /* Inner edge north/west */
    { 0 }

};

/* ------ Data for checking of adjacent tiles -------- */
static EDITMAIN_XY AdjacentXY[8] = {

    {  0, -1 }, { +1, -1 }, { +1,  0 }, { +1, +1 },
    {  0, +1 }, { -1, +1 }, { -1,  0 }, { -1, -1 }

};

/*******************************************************************************
* CODE 								                                           *
*******************************************************************************/

/*
 * Name:
 *     editmainGetAdjacent
 * Description:
 *     Creates a list of fans which are adjacent to given 'fan'. 
 *     If a fan is of map, the field is filled by a value of -1.
 *     The list starts from 'North', clockwise.
 *     -------
 *     |7|0|1|
 *     |-+-+-|
 *     |6| |2|
 *     |-+-+-|
 *     |5|4|3|
 *     -------
 * Input:
 *     mesh*:      Pointer on mesh with info about map size
 *     fan:        To find the adjacent tiles for  
 *     adjacent *: Where to return the list of fan-positions 
 * Output: 
 *     Number of adjacent tiles 
 */
static int editmainGetAdjacent(MESH_T *mesh, int fan, int adjacent[8])
{

    int dir, adj_pos;
    int num_adj;
    EDITMAIN_XY src_xy, dest_xy;

    
    num_adj = 0;            /* Count them */
    for (dir = 0; dir < 8; dir++) {
    
        adj_pos = -1;       /* Assume invalid */

        src_xy.x = fan % mesh -> tiles_x;
        src_xy.y = fan / mesh -> tiles_x;
        
        dest_xy.x = src_xy.x + AdjacentXY[dir].x;
        dest_xy.y = src_xy.y + AdjacentXY[dir].y;
        
        if ((dest_xy.x >= 0) && (dest_xy.x < mesh -> tiles_x)
            && (dest_xy.y >= 0) && (dest_xy.y < mesh -> tiles_y)) {

            adj_pos = (dest_xy.y * mesh -> tiles_x) + dest_xy.x;
            num_adj++;

        }

        adjacent[dir] = adj_pos;       /* Starting north */

    }

    return num_adj;

}

/*
 * Name:
 *     editmainSetFanStart
 * Description:
 *     Sets the 'vrtstart' for all fans in given map
 * Input:
 *     mesh*: Pointer on mesh to handle
 */
static void editmainSetFanStart(MESH_T *mesh)
{

    int fan_no, vertex_no;

    for (fan_no = 0, vertex_no = 0; fan_no < mesh -> numfan; fan_no++) {

        mesh -> vrtstart[fan_no] = vertex_no;
        vertex_no += pCommands[mesh -> fan[fan_no].type & 0x1F].numvertices;

    }

}

/*
 * Name:
 *     editmainFanAdd
 * Description:
 *     This functionsa adds a new fan to the actual map, if there are enough
 *     vertices left for it
 * Input:
 *     mesh*: Pointer on mesh to handle
 *     fan:   Number of fan to allocate vertices for
 *     x, y:  Position of fan
 * Output:
 *    Fan could be added, yes/no
 */
static int editmainFanAdd(MESH_T *mesh, int fan, int x, int y)
{

    COMMAND_T *ft;
    int cnt;
    int vertex;


    ft = &pCommands[mesh -> fan[fan].type & 0x1F];

    if (mesh -> numfreevert >= ft -> numvertices)
    {

        vertex = mesh -> numvert;

        mesh -> vrtstart[fan] = vertex;

        for (cnt = 0; cnt < ft -> numvertices; cnt++) {

            mesh -> vrtx[vertex] = x + ft -> vtx[cnt].x;
            mesh -> vrty[vertex] = y + ft -> vtx[cnt].y;
            mesh -> vrtz[vertex] = ft -> vtx[cnt].z;
            vertex++;

        }

        mesh -> numvert     += ft -> numvertices;   /* Actual number of vertices used   */
        mesh -> numfreevert -= ft -> numvertices;   /* Vertices left for edit           */

        return 1;

    }

    return 0;
}

/*
 * Name:
 *     editmainDoFanUpdate
 * Description:
 *     This function updates the fan at given 'fan 'with the
 *     info in 'EditState.ft'.
 *     If the type has changed, the vertice data is updated.
 * Input:
 *     mesh *:       Pointer on mesh to handle
 *     edit_state *: Pointer on edit state, holding all data needed for work
 *     fan_no:       Do an update of this fan
 * Output:
 *    Fan could be updated, yes/no
 */
static int editmainDoFanUpdate(MESH_T *mesh, EDITMAIN_STATE_T *edit_state, int fan_no)
{

    COMMAND_T *act_fd;
    FANDATA_T *act_ft;
    int cnt;
    int vrt_diff, vrt_size;
    int src_vtx, dst_vtx;
    int vertex;
    int tx, ty;


    act_ft = &mesh -> fan[fan_no];
    act_fd = &pCommands[act_ft -> type & 0x1F];

    /* Do an update on the 'static' data of the fan */
    act_ft -> tx_no    = edit_state -> ft.tx_no;
    act_ft -> tx_flags = edit_state -> ft.tx_flags;
    act_ft -> fx       = edit_state -> ft.fx;

    /* Now change tile pos to position in units */
    tx = (fan_no % mesh -> tiles_x) * 128;
    ty = (fan_no / mesh -> tiles_x) * 128;

    if (act_ft -> type == edit_state -> ft.type) {

        vrt_diff = 0;       /* May be rotated */

    }
    else {

        vrt_diff = edit_state -> fd.numvertices - act_fd -> numvertices;

    }

    if (0 == vrt_diff) {
        /* Same number of vertices, only overwrite is needed */
        /* Set the new type of fan */
        act_ft -> type = edit_state -> ft.type;

    }
    else {

        if (vrt_diff > 0) {
            /* Insert more vertices -- check for space */
            if (vrt_diff > mesh -> numfreevert) {
                /* Not enough vertices left in buffer */
                return 0;
            }

        }
        /* Set the new type of fan */
        act_ft -> type = edit_state -> ft.type;

        /* Copy from start vertex of next fan */
        src_vtx = mesh -> vrtstart[fan_no] + act_fd -> numvertices;
        dst_vtx = mesh -> vrtstart[fan_no] + edit_state -> fd.numvertices;
        /* Number of vertices to copy */
        vrt_size = (mesh -> numvert - src_vtx + 1) * sizeof(int);
        /* Add/remove vertices -- update it's numbers */
        mesh -> numfreevert -= vrt_diff;
        mesh -> numvert     += vrt_diff;
        /* Now, clear space or new vertices or remove superfluos vertices */
        /* dest, src, size */
        memmove(&mesh -> vrtx[dst_vtx], &mesh -> vrtx[src_vtx], vrt_size);
        memmove(&mesh -> vrty[dst_vtx], &mesh -> vrty[src_vtx], vrt_size);
        memmove(&mesh -> vrtz[dst_vtx], &mesh -> vrtz[src_vtx], vrt_size);

    }

    /* Fill in the vertex values from type definition */
    vertex = mesh -> vrtstart[fan_no];
    for (cnt = 0; cnt < edit_state -> fd.numvertices; cnt++) {
        /* Replace actual values by new values */
        mesh -> vrtx[vertex] = tx + edit_state -> fd.vtx[cnt].x;
        mesh -> vrty[vertex] = ty + edit_state -> fd.vtx[cnt].y;
        mesh -> vrtz[vertex] = edit_state -> fd.vtx[cnt].z;
        vertex++;
    }

    /* Adjust 'vrtstart' for all following fans */
    if (vrt_diff != 0) {
        editmainSetFanStart(mesh);
    }

    return 1;

}

/*
 * Name:
 *     editmainFanTypeRotate
 * Description:
 *     For 'simple' mode. Rotates the given fan type and returns it in
 *     'dest'. The translation has to be done by the caller
 * Input:
 *     type:   Number of fan type to rotate
 *     dest *: Copy of base type, rotated
 *     dir:    Into which direction to rotate
 */
static void editmainFanTypeRotate(int type, COMMAND_T *dest, char dir)
{

    int cnt;
    double angle;
    double result_x, result_y;



    /* Get a copy from  the fan type list, get only the 'lower' numbers */
    memcpy(dest, &pCommands[type & 0x1F], sizeof(COMMAND_T));

    if (dir != EDITMAIN_NORTH) {

        angle  = DEG2RAD(dir * 90.0);

        for (cnt = 0; cnt < dest -> numvertices; cnt++) {

            /* First translate it to have rotation center in middle of fan */
            dest -> vtx[cnt].x -= 64.0;
            dest -> vtx[cnt].y -= 64.0;
            /* And now rotate it */
            result_x = (cos(angle) * (double)dest -> vtx[cnt].x
                        - sin(angle) * (double)dest -> vtx[cnt].y);
            result_y = (sin(angle) * (double)dest -> vtx[cnt].x
                        + cos(angle) * (double)dest -> vtx[cnt].y);
            /* And store it back */
            dest -> vtx[cnt].x = floor(result_x);
            dest -> vtx[cnt].y = floor(result_y);
            /* And move it back to start position */
            dest -> vtx[cnt].x += 64.0;
            dest -> vtx[cnt].y += 64.0;

        }

    }

}

/*
 * Name:
 *     editmainWeldXY
 * Description:
 *     'Welds' the nearest vertices of both given fans.
 *     The vertice-values of 'fan_no2' are adjusted to these of 'fan_no1'
 * Input:
 *     mesh *: Pointer on mesh
 *     fan_no1,
 *     fan__no2: Number of fans to weld the vertices for
 * Output:
 *     Number of Vertices welded 
 */
static int editmainWeldXY(MESH_T *mesh, int fan_no1, int fan_no2)
{

    int cnt1, cnt2;
    float xdiff, ydiff;     /* Difference of vertices */
    int vtx1, vtx2;
    int numvertices1, numvertices2;
    int num_welded;


    numvertices1 = pCommands[mesh -> fan[fan_no1].type & 0x1F].numvertices;
    numvertices2 = pCommands[mesh -> fan[fan_no2].type & 0x1F].numvertices;
    num_welded   = 0;

    vtx1 = mesh -> vrtstart[fan_no1];

    for (cnt1 = 0; cnt1 < numvertices1; cnt1++) {
        /* Compare all vertices of first fan with this of second fan */
        vtx2 = mesh -> vrtstart[fan_no2];

        for (cnt2 = 0; cnt2 < numvertices2; cnt2++) {
            if (mesh -> vrtz[vtx1] == mesh -> vrtz[vtx2]) {
                /* Look for vertex in distance... */
                xdiff = mesh -> vrtx[vtx1] - mesh -> vrtx[vtx2];
                ydiff = mesh -> vrty[vtx1] - mesh -> vrty[vtx2];
                
                if (xdiff < 0) xdiff = -xdiff;
                if (ydiff < 0) ydiff = -ydiff;

                if ((xdiff + ydiff) < 12.0) {
                    mesh -> vrtx[vtx2] = mesh -> vrtx[vtx1];
                    mesh -> vrty[vtx2] = mesh -> vrty[vtx1];
                    num_welded++;
                }
            }
            vtx2++;
        }
        vtx1++;
    }
    
    return num_welded;

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
    editmainSetFanStart(mesh);      

    mesh -> numfreevert = (MAXTOTALMESHVERTICES - 10) - mesh -> numvert;
    /* Set flag that map has been loaded */
    mesh -> map_loaded = 1;
    mesh -> draw_mode  = EditState.draw_mode;

}

/*
 * Name:
 *     editmainCreateNewMap
 * Description:
 *     Creates a new, empty map consisting of all floor or wall types,
 *     depending on 'which'
 * Input:
 *     mesh *: Pointer on mesh  to fill with default values
 */
static int editmainCreateNewMap(MESH_T *mesh)
{

    int x, y, fan;


    memset(mesh, 0, sizeof(MESH_T));

    mesh -> tiles_x     = EditState.map_size;
    mesh -> tiles_y     = EditState.map_size;
    mesh -> minimap_w   = EditState.minimap_w;
    mesh -> minimap_h   = EditState.minimap_h;
    
    mesh -> numvert     = 0;                         /* Vertices used in map    */
    mesh -> numfreevert = MAXTOTALMESHVERTICES - 10; /* Vertices left in memory */

    fan = 0;
    for (y = 0; y < mesh -> tiles_y; y++) {

        for (x = 0; x < mesh -> tiles_x; x++) {

            mesh -> fan[fan].type  = WALLMAKE_TOP;
            mesh -> fan[fan].fx    = (MPDFX_WALL | MPDFX_IMPASS);
            mesh -> fan[fan].tx_no = EDITMAIN_TOP_TILE;

            if (! editmainFanAdd(mesh, fan, x*EDITMAIN_TILEDIV, y*EDITMAIN_TILEDIV))
            {
                sprintf(EditState.msg, "%s", "NOT ENOUGH VERTICES!!!");
                return 0;
            }

            fan++;

        }

    }

    return 1;

}

/*
 * Name:
 *     editmainSetVrta
 * Description:
 *     Set the 'vrta'-value for given vertex
 * Input:
 *     mesh *: Pointer on mesh  to handle
 *     vert:   Number of vertex to set
 */
static int editmainSetVrta(MESH_T *mesh, int vert)
{
    /* TODO: Get all needed functions from cartman code */
    int newa, x, y, z;
    int brz, deltaz, cnt;
    int newlevel, distance, disx, disy;
    /* int brx, bry, dist; */ 

    /* To make life easier  */
    x = mesh -> vrtx[vert];
    y = mesh -> vrty[vert];
    z = mesh -> vrtz[vert];

    // Directional light
    /*
    brx = x + 64;
    bry = y + 64;

    brz = get_level(brx, y) +
          get_level(x, bry) +
          get_level(x + 46, y + 46);
    */
    
    brz = z;
    if (z < -128) z = -128;
    if (brz < -128) brz = -128;
    deltaz = z + z + z - brz;
    newa = (deltaz * LightDirect >> 8);

    // Point lights !!!BAD!!!
    newlevel = 0;
    for (cnt = 0; cnt < NumLight; cnt++) {
        disx = x - LightList[cnt].x;
        disy = y - LightList[cnt].y;
        distance = (disx * disx) + (disy * disy);
        if (distance < (LightList[cnt].radius * LightList[cnt].radius))
        {
            newlevel += ((LightList[cnt].level * (LightList[cnt].radius - distance)) / LightList[cnt].radius);
        }
    }
    newa += newlevel;

    // Bounds
    if (newa < -LightAmbiCut) newa = -LightAmbiCut;
    newa += LightAmbi;
    if (newa <= 0) newa = 1;
    if (newa > 255) newa = 255;
    mesh -> vrta[vert] = (unsigned char)newa;

    // Edge fade
    /*
    dist = dist_from_border(mesh  -> vrtx[vert], mesh -> vrty[vert]);
    if (dist <= EDITMAIN_FADEBORDER)
    {
        newa = newa * dist / EDITMAIN_FADEBORDER;
        if (newa == 0)  newa = 1;
        mesh  ->  vrta[vert] = newa;
    }
    */

    return newa;

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
        
            editmainSetVrta(mesh, vert);
            vert++;
            
        }

    }

}

/*
 * Name:
 *     editmainCreateWallMakeInfo
 * Description:
 *     Fills the given list in a square 5 x 5 from left top to right bottom
 *     with infos about fans. 'fan is the center of the square.
 *     Tiles off the map are signed as 'top' tiles
 *     Generates the 'a'  numbers for all files
 * Input:
 *     mesh*: Pointer on mesh to get the info from
 *     fan:   This is the center fan
 *     wi *:  Array to fill with info needed by the wallmaker-code
 *
 */
static void editmainCreateWallMakeInfo(MESH_T *mesh, int fan, WALLMAKER_INFO_T *wi)
{

    int adjacent[8];
    int i;
    
    
    wi[0].fan_no = fan;
    wi[0].type   = mesh -> fan[fan].type;
    wi[0].dir    = 0;
    
    editmainGetAdjacent(mesh, fan, adjacent);
    for (i = 0; i < 8; i++) {
    
        wi[i + 1].fan_no = adjacent[i];
        wi[i + 1].type   = mesh -> fan[adjacent[i]].type;
        wi[i + 1].dir    = 0;

    }  
   
}

/*                                             
 * Name:
 *     editmainTranslateWallMakeInfo
 * Description:
 *     Translates the given wallmaker info into fans on map 
 * Input:
 *     mesh*:    Pointer on mesh to get the info from
 *     wi *:     Array from wallmaker to create walls from
 *     num_tile: Number of tiles to generate
 */
static void editmainTranslateWallMakeInfo(MESH_T *mesh, WALLMAKER_INFO_T *wi, int num_tile)
{

    int i, i2;
    char type_no;
    

    for (i = 0; i < num_tile; i++) {

        if (wi[i].fan_no >= 0) {
        
            /* -- Do update in any case */
			type_no = wi[i].type;

            EditState.ft.type     = type_no;
            EditState.ft.tx_flags = 0;
            EditState.ft.fx       = pCommands[type_no & 0x1F].default_fx;
            EditState.ft.tx_no    = pCommands[type_no & 0x1F].default_tx_no;
            EditState.fan_dir     = wi[i].dir;

            memcpy(&EditState.fd, &pCommands[type_no & 0x1F], sizeof(COMMAND_T));

            editmainFanTypeRotate(EditState.ft.type,
                                  &EditState.fd,
                                  EditState.fan_dir);

            editmainDoFanUpdate(mesh, &EditState, wi[i].fan_no);

        }

    }

    /* Weld the new generated walls to middle tile */
    for (i = 1; i < num_tile; i++) {
        editmainWeldXY(mesh, wi[0].fan_no,  wi[i].fan_no);

    }

    /* Weld the new generated walls with each other */
    for (i = 0; i < 9; i++) {
        i2 = i + 1;
        if (i2 > 8) {
            i2 = 0;
        }
        /* Only weld if both tiles are walls */
        if (wi[i].type > 1 && wi[i2].type > 1) {
            editmainWeldXY(mesh, wi[i].fan_no, wi[i2].fan_no);
        }

    }

}

/*
 * Name:
 *     editmainLoadAdditionalData
 * Description:
 *     Loads additional data needed for map. SPAWN-Points an Passages 
 * Input:
 *     mesh*: Pointer on mesh to get the info from
 *     wi *:  Array from wallmaker to create walls from
 *
 */
static void editmainLoadAdditionalData(void)
{   
    
    sdlglcfgReadEgoboo("module/passage.txt", &PassageRec);
    sdlglcfgReadEgoboo("module/spawn.txt", &SpawnRec);
    /* --- Set flag, if passages / spawn points are loaded at all   */
    if (Passages[1].line_name[0] > 0) {
        EditState.psg_no = 1;
    }
    else {
        EditState.psg_no = 1;
    }
    
    if (SpawnObjects[1].line_name[0] > 0) {
        EditState.spawnpos_no = 1;
    }
    else {
        EditState.spawnpos_no = 0;
    }          

}

/*                                             
 * Name:
 *     editmainChoosePassage
 * Description:
 *     If 'dir' = 0, then the list index is reset 
 *     Loads additional data needed for map. SPAWN-Points an Passages
 * Input:
 *     dir:   Move index into this direction 
 */
static int editmainChoosePassage(int dir)
{

    if (EditState.psg_no > 0) {
        /* If a passage at all */
        if (dir == 0) {
            EditState.psg_no = 1;
        }
        else if (dir > 0) {
            if (Passages[EditState.psg_no + 1].line_name[0] > 0) {
                EditState.psg_no++;
            }
        }
        else if (dir < 0) {
            if (EditState.psg_no > 1) {
                EditState.psg_no--;
            }
        }
        if (dir != 0) {
            /* TODO: Fill the list of fan numbers for this passage */
            PassageFan[0] = -1;
        }
        return 1;
    }
    
    PassageFan[0] = -1;
    
    return 0;
    
}

/*                                             
 * Name:
 *     editmainChooseSpawnPos
 * Description:
 *     Loads additional data needed for map. SPAWN-Points an Passages 
 * Input:
 *     dir:   Move index into this direction
 */
static int editmainChooseSpawnPos(int dir)
{

    if (EditState.spawnpos_no > 0) {
        /* If a spawn point at all */
        if (dir == 0) {
            EditState.spawnpos_no = 1;
        }
        else if (dir > 0) {
            if (SpawnObjects[EditState.spawnpos_no + 1].line_name[0] > 0) {
                EditState.spawnpos_no++;
            }            
        }
        else if (dir < 0) {
            if (EditState.spawnpos_no > 1) {
                EditState.spawnpos_no--;
            }
        }
        if (dir != 0) {
            /* TODO: Fill the list of fan numbers for this spawn pos */

            SpawnFan[0] = -1;       /* No fan at all */
        }
        return 1;
    }

    SpawnFan[0] = -1;       /* No fan at all */
    
    return 0;    
    
}

/*
 * Name:
 *     editmainUpdateChosenFreeFan
 * Description:
 *     Sets the data for the chosen tile type for 'free' mode  
 * Input:
 *     None, uses dataa in'EditState' and 'Mesh' for work 
 */
static void editmainUpdateChosenFreeFan(void)
{

    int cnt, tx, ty;
    
    
    if (-1 != EditState.fan_selected[0]) {
                    
        /* Get copy at original position */
        memcpy(&EditState.fd, &pCommands[EditState.ft.type & 0x1F], sizeof(COMMAND_T));
        editmainFanTypeRotate(EditState.ft.type,
                              &EditState.fd,
                              EditState.fan_dir);
        /* Now translate the fan to chosen fan position */
        tx = (EditState.fan_selected[0] % Mesh.tiles_x) * 128;
        ty = (EditState.fan_selected[0] / Mesh.tiles_x) * 128;

        for (cnt = 0; cnt < EditState.fd.numvertices; cnt++) {
            EditState.fd.vtx[cnt].x += tx;
            EditState.fd.vtx[cnt].y += ty;
        }

    }

}

/*
 * Name:
 *     editmainFanSet
 * Description:
 *     Sets a fan at the actual chosen position, depending on edit_state.
 *     Does an update on given fan. Including changed number of vertices,
 *     if needed.
 * Input:
 *     fan_no:   Set this fan
 *     is_floor: Set floor in simple mode, else set wall
 */
static int editmainFanSet(int fan_no, int is_floor)
{

    WALLMAKER_INFO_T wi[12];            /* List of fans to create */
    int num_wall;


    if (fan_no >= 0) {

        if (EditState.edit_mode == EDITMAIN_EDIT_NONE) {
            EditState.fd.numvertices = 0;   /* But set fan type to invalid  */
            return 1;                       /* Do nothing, is view-mode     */
        }

        if (EditState.edit_mode == EDITMAIN_EDIT_SIMPLE) {

            if (EditState.tx > 0 && EditState.ty > 0
                && EditState.tx < (Mesh.tiles_x - 1)
                && EditState.ty < (Mesh.tiles_y - 1))  {

                /* Get a list of fans surrounding this one */
                editmainCreateWallMakeInfo(&Mesh, fan_no, wi);

                num_wall = wallmakeMakeTile(is_floor, wi);
                if (num_wall) {
                    /* Create tiles from WALLMAKER_INFO_T */
                    editmainTranslateWallMakeInfo(&Mesh, wi, num_wall);
                    EditState.fd.numvertices = 0;
                    return 1;
                }
            }

        }
        else if (EditState.edit_mode == EDITMAIN_EDIT_FREE) {
            if (is_floor) {
                editmainUpdateChosenFreeFan();
            }
            else {
                memcpy(&EditState.fd, &pCommands[EditState.ft.type & 0x1F], sizeof(COMMAND_T));
                editmainFanTypeRotate(EditState.ft.type,
                                      &EditState.fd,
                                      EditState.fan_dir);
                editmainDoFanUpdate(&Mesh, &EditState, EditState.fan_selected[0]);
            }

        }
        else if (EditState.edit_mode == EDITMAIN_EDIT_FX) {
            /* Do an update on the FX-Flags */
            Mesh.fan[fan_no].fx = EditState.ft.fx;

        }
        else if (EditState.edit_mode == EDITMAIN_EDIT_TEXTURE) {
            /* Change the texture */
            Mesh.fan[fan_no].tx_no    = EditState.ft.tx_no;
            Mesh.fan[fan_no].tx_flags = EditState.ft.tx_flags;
        }
    }

    return 0;
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
 *     map_size: Default map size
 * Output:
 *     Pointer on EditState
 */
EDITMAIN_STATE_T *editmainInit(int map_size, int minimap_w, int minimap_h)
{

    pCommands = editdrawInitData();
     
    memset(&EditState, 0, sizeof(EDITMAIN_STATE_T));

    EditState.display_flags   |= EDITMAIN_SHOW2DMAP;
    EditState.fan_selected[0] = -1;   /* No fan chosen          */
    EditState.fan_selected[0] = -1;   /* No multiple fan chosen */
    EditState.ft.type         = -1;   /* No fan-type chosen     */
    EditState.map_size        = map_size;
    EditState.minimap_w       = minimap_w;
    EditState.minimap_h       = minimap_h;
    EditState.draw_mode       = (EDIT_MODE_SOLID | EDIT_MODE_TEXTURED | EDIT_MODE_LIGHTMAX);

    /* Additional fans, that can be chosen */
    PassageFan[0] = -1;
    SpawnFan[0]   = -1;

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
 *      command:  What to do
 *      info:     additional info for command
 * Output:
 *      Result of given command
 */
int editmainMap(int command, int info)
{
                                       
    switch(command) {

        case EDITMAIN_DRAWMAP:
            editdraw3DView(&Mesh, &EditState.ft, &EditState.fd,
                           EditState.fan_selected,
                           PassageFan,
                           SpawnFan);
            return 1;

        case EDITMAIN_NEWMAP:
            if (editmainCreateNewMap(&Mesh)) {

                editmainCompleteMapData(&Mesh);
                sdlgl3dInitVisiMap(Mesh.tiles_x, Mesh.tiles_y, 128.0);
                /* Maybe initialize here the  chosen fan to
                   none or 0 instead by caller */
                return 1;

            }
            break;

        case EDITMAIN_LOADMAP:
            memset(&Mesh, 0, sizeof(MESH_T));
            if (editfileLoadMapMesh(&Mesh, EditState.msg)) {

                editmainCompleteMapData(&Mesh);
                sdlgl3dInitVisiMap(Mesh.tiles_x, Mesh.tiles_y, 128.0);
                /* -------- Now read the data for spawn points and passages */
                editmainLoadAdditionalData();                
                return 1;

            }
            break;

        case EDITMAIN_SAVEMAP:
            editmainCalcVrta(&Mesh);
            return editfileSaveMapMesh(&Mesh, EditState.msg);

        case EDITMAIN_ROTFAN:
            if (EditState.edit_mode == EDITMAIN_EDIT_FREE) {
                /* Rotate the chosen fan type, if active edit mode */
                EditState.fan_dir++;
                EditState.fan_dir &= 0x03;
                editmainUpdateChosenFreeFan();
            }
            return 1;
            
        case EDITMAIN_CHOOSEPASSAGE:
            return editmainChoosePassage(info);
            
        case EDITMAIN_CHOOSESPAWNPOS:
            return editmainChooseSpawnPos(info);
 
    }

    return 0;

}

/*
 * Name:
 *     editmainDrawMap2D
 * Description:
 *      Draws the map as 2D-Map into given rectangle
 * Input:
 *      x, y: Where to draw on screen
 * Output:
 *      Result of given command
 */
void editmainDrawMap2D(int x, int y)
{ 
    if (EditState.display_flags & EDITMAIN_SHOW2DMAP) {
    
        Mesh.minimap_w = EditState.minimap_w;
        Mesh.minimap_h = EditState.minimap_h;

        editdraw2DMap(&Mesh, x, y,
                      EditState.fan_selected,
                      PassageFan,
                      SpawnFan);
    }

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
char editmainToggleFlag(int which, unsigned char flag)
{

    char tex_no;
    char fan_name[255];
    
    
    switch(which) {

        case EDITMAIN_TOGGLE_DRAWMODE:
            Mesh.draw_mode ^= flag;                 /* Change it in actual map */
            EditState.draw_mode = Mesh.draw_mode;   /* Show it in edit_state */
            break;

        case EDITMAIN_TOGGLE_EDITSTATE:
            if (flag == 0) {
                EditState.edit_mode = 0;
            }
            else {
                EditState.edit_mode++;
                if (EditState.edit_mode > EDITMAIN_EDIT_MAX) {
                    EditState.edit_mode = 0;
                }
            }
            if (EditState.edit_mode == EDITMAIN_EDIT_FREE) {
                /* Initialize the chosen fan    */
                editmainChooseFanType(0, fan_name);
            }
            else {
                EditState.fd.numvertices = 0;   /* Hide the chosen fan */
            }
            return EditState.edit_mode;
            
        case EDITMAIN_TOGGLE_FANTEXSIZE:
            EditState.ft.type ^= 0x20;  /* Switch 'size' flag */
            break;
            
        case EDITMAIN_TOGGLE_FANTEXNO:
            tex_no = (char)(EditState.ft.tx_no >> 6);
            
            if (flag == 0xFF) {
                tex_no--;
                if (tex_no < 0) {
                    tex_no = 3;
                }
            }
            else if (flag == 0x01) {
                tex_no++;
                if (tex_no > 3) {
                    tex_no = 0;
                }
            }
            EditState.ft.tx_no &= 0x3F;
            EditState.ft.tx_no |= (char)((tex_no & 0x03) << 6);
            break;

        case EDITMAIN_TOGGLE_FANFX:
            EditState.ft.fx ^= flag;
            break;

    }

    return 0;

}

/*
 * Name:
 *     editmainChooseFan
 * Description:
 *     Choose a fan from given position in rectangle of size w/h.
 *     Does an update on the 'EditState'.
 *     Fills info about chosen fan into Edit-State, depending on Edit-State
 *     Uses 'minimap_w' and 'minimap_h' for choosing the value
 *     Set a an, if 'is_floor' >= 0
 * Input:
 *     cx, cy:   Position chosen
 *     is_floor: Sets a fan, depending on 'EditState.edit_mode'
 */
void editmainChooseFan(int cx, int cy, int is_floor)
{

    int fan_no, *pfan;
    int tw, th;


    if (! EditState.display_flags & EDITMAIN_SHOW2DMAP) {
        EditState.fd.numvertices = 0;
        /* Map not visible, invalid command */
        return;
    }

    /* ------- Set  multiple tiles, if more the one is chosen --------- */
    if (EditState.fan_selected[0] >= 0 && EditState.fan_selected[1] >= 0) {
        pfan = EditState.fan_selected;
        while(*pfan >= 0) {
            editmainFanSet(*pfan, is_floor);
            pfan++;
        }
        EditState.fan_selected[1] = -1;
        /* Reset number of chosen fan, because it's done */
        return;
    }
    /* ------------------------- */

    /* Save it as x/y-position, too */
    tw = Mesh.minimap_w / Mesh.tiles_x;      /* Calculate rectangle size for mouse */
    th = Mesh.minimap_h / Mesh.tiles_y;
    EditState.tx = cx / tw;
    EditState.ty = cy / th;

    fan_no = (EditState.ty * Mesh.tiles_x) + EditState.tx;

    if (fan_no >= 0 && fan_no < Mesh.numfan) {

        EditState.fan_selected[0] = fan_no;
        EditState.fan_selected[1] = -1;

        /* Get the info about the chosen fan */
        if (EditState.edit_mode == EDITMAIN_EDIT_NONE) {
            /* In info-mode return the properties of the chosen fan */
            memcpy(&EditState.ft, &Mesh.fan[fan_no], sizeof(FANDATA_T));
        }
        else {
            /* Adjust position of display for 'Free' Mode */
            if (EditState.fd.numvertices > 0) {
                editmainUpdateChosenFreeFan();
            }
        }

        if (is_floor >= 0) {
            editmainFanSet(fan_no, is_floor);
        }

        /* And now set camera to move/look at this position */
        editdrawAdjustCamera(EditState.tx, EditState.ty);

    }

}

/*
 * Name:
 *     editmainChooseFanExt
 * Description:
 *     Choose multiple fans from given position in rectangle of size w/h.
 *     Does an update on the 'EditState'.
 *     Fills info about chosen fan into Edit-State, depending on Edit-State
 *     Uses 'minimap_w' and 'minimap_h' for choosing the value
 *     What is set is 'triggered' on release of mouse (editmainChooseFan)
 * Input:
 *     cx, cy:   Position chosen
 *     cw, ch:   Rectangle chosen in minimap
 */
void editmainChooseFanExt(int cx, int cy, int cw, int ch)
{

    int tw, th;
    int tx1, ty1, tx2, ty2;
    int ltx;
    int num_select;


    tw = Mesh.minimap_w / Mesh.tiles_x;      /* Calculate rectangle size for mouse */
    th = Mesh.minimap_h / Mesh.tiles_y;

    tx1 = cx / tw;
    ty1 = cy / th;
    tx2 = (cx + cw) / tw;
    ty2 = (cy + ch) / th;

    num_select = 0;

    while(ty1 <= ty2) {
        if (ty1 > 0 && ty1 < (Mesh.tiles_y -1)) {
            /* If 'ty' in map */
            ltx = tx1;
            while(ltx < tx2) {
                if (ltx > 0 && ltx < (Mesh.tiles_x - 1) && ty1 > 0 && ty1 < (Mesh.tiles_y - 1)) {

                    EditState.fan_selected[num_select] = (ty1 * Mesh.tiles_x) + ltx;
                    num_select++;
                    if (num_select >= EDITMAIN_MAXSELECT) {
                        EditState.fan_selected[num_select] = -1;    /* Sign end of list */
                        return;
                    }

                }
                ltx++;
            } /* while(lcx < cx2) */
        } /* if 'ty' in map */
        ty1++;
    }

    EditState.fan_selected[num_select] = -1;

}

/*
 * Name:
 *     editmainFanTypeName
 * Description:
 *     Returns the description of given fan-type 
 * Input:
 *     fan_name *: Where to return the name of the actual fan type
 * Output:
 *     Pointer on a valid string, may be empty
 */
void editmainFanTypeName(char *fan_name)
{

    char type_no;


    if (EditState.ft.type >= 0) {

        type_no = (char)(EditState.ft.type & 0x1F);

        if (pCommands[type_no & 0x1F].name != 0) {
        
            sprintf(fan_name, "%s", pCommands[type_no & 0x1F].name);
            return;

        }

    }

    fan_name[0] = 0;

}

/*
 * Name:
 *     editmainChooseFanType
 * Description:
 *     Chooses a fan type and sets it for display.
 *     TODO: Ignore 'empty' fan types
 * Input:
 *     dir:       Direction to browse trough list
 *     fan_name *: Where to print the name of chosen fan
 */
void editmainChooseFanType(int dir, char *fan_name)
{

    char i;
    int  x, y;


    if (EditState.edit_mode != EDITMAIN_EDIT_FREE) {
        /* Do that only in 'Free' Mode */
        fan_name[0] = 0;
        return;
    }
    if (dir == 0) {
        /* Start browsing */
        EditState.bft_no = 0;
    }
    else {
        if (EditState.bft_no < EDITMAIN_PRESET_MAX) {
            EditState.bft_no++;
        }
        else {  /* Wrap around */
            EditState.bft_no = 0;
        }
    }

    EditState.fan_dir = 0;

    memcpy(&EditState.ft, &PresetTiles[EditState.bft_no], sizeof(FANDATA_T));
    memcpy(&EditState.fd, &pCommands[EditState.ft.type & 0x1F], sizeof(COMMAND_T));

    /* Now move it to the chosen position */
    x = EditState.tx * 128;
    y = EditState.ty * 128;

    for (i = 0; i < EditState.fd.numvertices; i++) {
        EditState.fd.vtx[i].x += x;
        EditState.fd.vtx[i].y += y;
    }

    editmainFanTypeName(fan_name);

}

/*
 * Name:
 *     editmain2DTex
 * Description:
 *     Draws the texture and chosen texture-part of actual chosen fan
 *     into given rectangle. Uses data from EditState
 * Input:
 *     x, y, w, h: Rectangle to draw into
 */
void editmain2DTex(int x, int y, int w, int h)
{

    if (EditState.fan_selected[0] >= 0) {

        editdraw2DTex(x, y, w, h,
                      EditState.ft.tx_no,
                      (char)(EditState.ft.type & COMMAND_TEXTUREHI_FLAG));

    }
    
}

/*
 * Name:
 *     editmainChooseTex
 * Description:
 *     Choses Texture from square with given coordinates 'cx/cy' from
 *     Rectangle with given size 'w/h'
 *     The texture extent to choose is allways 8x8 squares
 * Input:
 *     cx, cy: Chosen point in rectangle
 *     w,h:    Extent of rectangle
 */
void editmainChooseTex(int cx, int cy, int w, int h)
{

    int tex_x, tex_y;
    char tex_no;
    char new_tex;
 
    
    /* Save it as x/y-position, too */
    tex_x = 8 * cx / w;
    tex_y = 8 * cy / h;
    
    if (EditState.ft.type & 0x20) {
        if (tex_x > 6) tex_x = 6;
        if (tex_y > 6) tex_y = 6;
    }

    tex_no = (char)((tex_y * 8) + tex_x);
    if (tex_no >= 0 && tex_no < 63) {
        /* Valid texture_no, merge it with number of main texture */
        new_tex = (char)(EditState.ft.tx_no & 0xC0);
        new_tex |= tex_no;
        /* Do an update of the edit-state and the mesh */
        EditState.ft.tx_no = new_tex;
    }      
  
}
