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

#include "sdlgl3d.h"
#include "editor.h"             /* Global needed definitions    */
#include "editfile.h"           /* Load and save map files      */
#include "editdraw.h"           /* Draw anything                */


#include "editmain.h"           /* My own header                */

/*******************************************************************************
* DEFINES								                                       *
*******************************************************************************/

#define EDITMAIN_MAX_COMMAND 30

#define EDITMAIN_MAX_MAPSIZE    64
#define EDITMAIN_DEFAULT_TILE   ((char)1)
#define EDITMAIN_TOP_TILE       ((char)63)  /* Black texture    */
#define EDITMAIN_WALL_HEIGHT    192
#define EDITMAIN_TILEDIV        128         /* Size of tile     */

/* --------- Info for preset tiles ------- */
#define EDITMAIN_PRESET_FLOOR   0
#define EDITMAIN_PRESET_TOP     1
#define EDITMAIN_PRESET_WALL    2
#define EDITMAIN_PRESET_EDGEO   3
#define EDITMAIN_PRESET_EDGEI   4
#define EDITMAIN_PRESET_MAX     5

#define EDITMAIN_NORTH  0x00
#define EDITMAIN_EAST   0x01
#define EDITMAIN_SOUTH  0x02
#define EDITMAIN_WEST   0x03

/*******************************************************************************
* TYPEDEFS							                                           *
*******************************************************************************/

typedef struct {

    int x, y;

} EDITMAIN_XY;

typedef struct {

     char center_shape;     /* Shape of tile in center                      */
     char center_rotdir;    /* Turn-Dir for center shape, if any            */
     char adj_shapes[3];    /* Shapes to set adjacent from left to right    */
     char adj_rotdir[3];    /* Rotation angle of 'adj_shape'                */

} EDITMAIN_AUTO_LUT;        /* Lookup-Table for auto-placement of walls     */

/*******************************************************************************
* DATA							                                               *
*******************************************************************************/

static MESH_T Mesh;
static COMMAND_T *pCommands;
static EDITMAIN_STATE_T EditState;
static SPAWN_OBJECT_T   SpawnObjects[EDITMAIN_MAXSPAWN + 2];
static EDITOR_PASSAGE_T Passages[EDITMAIN_MAXPASSAGE + 2];

/* --- Definition of preset tiles for 'simple' mode -- */
static FANDATA_T PresetTiles[] = {
    {  EDITMAIN_DEFAULT_TILE, 0, 0,  0 },                          /* Floor    */
    {  EDITMAIN_TOP_TILE,     0, (MPDFX_WALL | MPDFX_IMPASS), 1 }, /* Top      */
    /* Walls, x/y values are rotated, if needed */
    {  64 + 10, 0, (MPDFX_WALL | MPDFX_IMPASS), 8  },   /* Wall north */
    {  64 + 1,  0, (MPDFX_WALL | MPDFX_IMPASS), 16 },   /* Outer edge north/east */
    {  64 + 3,  0, (MPDFX_WALL | MPDFX_IMPASS), 19 },   /* Inner edge north/west */
    { 0 }
};

static EDITMAIN_AUTO_LUT LutFloor[] = {
    { EDITMAIN_PRESET_FLOOR, EDITMAIN_NORTH, { -1, -1, -1 } },          /* Set a floor at 'my_shape'    */
    { EDITMAIN_PRESET_FLOOR, EDITMAIN_NORTH, { -1, -1, EDITMAIN_PRESET_EDGEO  },
                                             { -1, -1, EDITMAIN_WEST          } },
    { EDITMAIN_PRESET_FLOOR, EDITMAIN_NORTH, { -1, EDITMAIN_PRESET_EDGEO, -1  },
                                             { -1, EDITMAIN_SOUTH, -1         } },
    { EDITMAIN_PRESET_FLOOR, EDITMAIN_NORTH, { -1, EDITMAIN_PRESET_WALL, EDITMAIN_PRESET_WALL },
                                             { -1, EDITMAIN_WEST, EDITMAIN_WEST } },
	{ EDITMAIN_PRESET_FLOOR, EDITMAIN_NORTH, { EDITMAIN_PRESET_EDGEO, -1, -1  },
                                             { EDITMAIN_EAST, -1, -1          } },
    { EDITMAIN_PRESET_FLOOR, EDITMAIN_NORTH, { EDITMAIN_PRESET_EDGEO, -1, EDITMAIN_PRESET_EDGEO },
                                             { EDITMAIN_EAST, -1, EDITMAIN_WEST                 } },
    { EDITMAIN_PRESET_FLOOR, EDITMAIN_NORTH, { EDITMAIN_PRESET_WALL, EDITMAIN_PRESET_WALL, -1   },
                                             { EDITMAIN_SOUTH, EDITMAIN_SOUTH, -1               } },
    { EDITMAIN_PRESET_FLOOR, EDITMAIN_NORTH, { EDITMAIN_PRESET_WALL, EDITMAIN_PRESET_EDGEI, EDITMAIN_PRESET_WALL },
                                             { EDITMAIN_SOUTH, EDITMAIN_WEST, EDITMAIN_WEST } }
};

static EDITMAIN_AUTO_LUT LutWall[] = {
    { EDITMAIN_PRESET_EDGEO, EDITMAIN_NORTH, { -1, -1, -1 } },
    { EDITMAIN_PRESET_WALL,  EDITMAIN_NORTH, { EDITMAIN_PRESET_WALL, -1, -1  },
                                             { EDITMAIN_NORTH, -1, -1        } },
    { EDITMAIN_PRESET_EDGEO, EDITMAIN_NORTH, { -1, EDITMAIN_PRESET_EDGEO, -1 },
                                             { -1, EDITMAIN_SOUTH, -1        } },
    { EDITMAIN_PRESET_WALL,  EDITMAIN_NORTH, { -1, EDITMAIN_PRESET_WALL, EDITMAIN_PRESET_EDGEI },
                                             { -1, EDITMAIN_WEST, EDITMAIN_NORTH               } },
    { EDITMAIN_PRESET_WALL,  EDITMAIN_EAST,  { EDITMAIN_PRESET_WALL, -1, -1 }, 
                                             { EDITMAIN_EAST, -1, -1        } }, 
    { EDITMAIN_PRESET_EDGEI, EDITMAIN_EAST,  { EDITMAIN_PRESET_WALL, -1, EDITMAIN_PRESET_WALL  },
                                             { EDITMAIN_EAST, -1, EDITMAIN_NORTH               } },
    { EDITMAIN_PRESET_WALL,  EDITMAIN_EAST,  { EDITMAIN_PRESET_EDGEI, EDITMAIN_PRESET_WALL, -1 },    
                                             { EDITMAIN_SOUTH, EDITMAIN_SOUTH, -1              } }, 
    { EDITMAIN_PRESET_TOP,   EDITMAIN_NORTH, { -1, -1, -1 } }
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
 *     The list starts from North', clockwise.  
 * Input:
 *     mesh*:      Pointer on mesh with info about map size
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
        mesh -> visible[fan_no]  = 1;
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
 *     zadd:  Elevation to add to given default z-value
 * Output:
 *    Fan could be added, yes/no
 */
static int editmainFanAdd(MESH_T *mesh, int fan, int x, int y, int zadd)
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
            mesh -> vrtz[vertex] = zadd + ft -> vtx[cnt].z;
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
 *     edit_state *: Pointer on edit state, holding all data neede for work
 *     tx, ty:       Position of tile in units
 * Output:
 *    Fan could be updated, yes/no
 */
static int editmainDoFanUpdate(MESH_T *mesh, EDITMAIN_STATE_T *edit_state, int tx, int ty)
{

    COMMAND_T *act_fd;
    FANDATA_T *act_ft;
    int cnt;
    int vrt_diff, vrt_size;
    int src_vtx, dst_vtx;
    int vertex;
    

    act_ft = &mesh -> fan[edit_state -> fan_chosen];
    act_fd = &pCommands[act_ft -> type];

    /* Do an update on the 'static' data of the fan */
    act_ft -> tx_no    = edit_state -> ft.tx_no;
    act_ft -> tx_flags = edit_state -> ft.tx_flags;
    act_ft -> fx       = edit_state -> ft.fx;

    if (act_ft -> type == edit_state -> ft.type) {

        return 1;       /* No vertices to change */

    }

    vrt_diff = edit_state -> fd.numvertices - act_fd -> numvertices;

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
        src_vtx = mesh -> vrtstart[edit_state -> fan_chosen] + act_fd -> numvertices;
        dst_vtx = mesh -> vrtstart[edit_state -> fan_chosen] + edit_state -> fd.numvertices;
        /* Number of vertices to copy */
        vrt_size = (mesh -> numvert - src_vtx) * sizeof(int);
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
    vertex = mesh -> vrtstart[edit_state -> fan_chosen];
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



    /* Get a copy from  the fan type list */
    memcpy(dest, &pCommands[type], sizeof(COMMAND_T));

    if (dir != EDITMAIN_NORTH) {

        angle  = DEG2RAD(dir * 90.0);

        for (cnt = 0; cnt < dest -> numvertices; cnt++) {

            /* First translate it to have rotation center in middle of fan */
            dest -> vtx[cnt].x -= 64.0;
            dest -> vtx[cnt].y -= 64.0;
            /* And now rotate it */
            result_x = (cos(angle) * dest -> vtx[cnt].x - sin(angle) * dest -> vtx[cnt].y);
            result_y = (sin(angle) * dest -> vtx[cnt].x + cos(angle) * dest -> vtx[cnt].y);
            /* And store it back */
            dest -> vtx[cnt].x = result_x;
            dest -> vtx[cnt].y = result_y;
            /* And move it back to start position */
            dest -> vtx[cnt].x += 64.0;
            dest -> vtx[cnt].y += 64.0;

        }

    }

    /* Otherwise no rotation is needed */
}

/*
 * Name:
 *     editmainSetFanSimple
 * Description:
 *     For 'simple' mode. Sets a tile, if possible .
 *     Adjusts the adjacent tiles accordingly, using the 'default' fan set.
 * Input:
 *     mesh *:       Pointer on mesh to handle
 *     edit_state *: Pointer on Edit-state holding all info needed
 *     is_floor:     Is it a floor yes/no
 * Output:
 *     Success yes/no
 */
static int editmainSetFanSimple(MESH_T *mesh, EDITMAIN_STATE_T *edit_state, int is_floor)
{

    static EDITMAIN_XY adj_xy[] =
            { { +0, -128 } , { +128, -128 }, { +128, +0 }, { +128, +128 },
              { +0, +128 } , { -128, +128 }, { -128 + 0 }, { -128, -128 },
              { +0, -128 } , { +128, -128 } };

    int adjacent[8];
    char shape_no, rotdir;
    int lut_idx;
    int dir, i;
    int check_flags, flags;
    int base_x, base_y;


    editmainGetAdjacent(mesh, edit_state -> fan_chosen, adjacent);

    /* Fill in the flags for the look-up-table */
    check_flags = 0;
    for (dir = 0, flags = 0x8000; dir < 8; dir++, flags >>= 0x01) {
        if (-1 == adjacent[dir]) {
            check_flags |= flags;       /* Handle it like a wall */
        }
        else if (mesh -> fan[adjacent[dir]].type != PresetTiles[EDITMAIN_PRESET_FLOOR].type) {
            check_flags |= flags;       /* It's a wall          */
        }
    }

    check_flags |= (check_flags >> 8);      /* Dupe it for end of radius */

    base_x = edit_state -> tx * EDITMAIN_TILEDIV;
    base_y = edit_state -> ty * EDITMAIN_TILEDIV;

    if (is_floor) {

        if (mesh -> fan[edit_state -> fan_chosen].type == PresetTiles[EDITMAIN_PRESET_FLOOR].type) {
            /* No change at all */
            return 1;
        }

        /* Change me to floor, set the fan-description  */
        memcpy(&edit_state -> ft, &PresetTiles[EDITMAIN_PRESET_FLOOR], sizeof(FANDATA_T));
        memcpy(&edit_state -> fd, &pCommands[edit_state -> ft.type], sizeof(COMMAND_T));

        editmainDoFanUpdate(mesh, edit_state, base_x, base_y);

        for (dir = 0; dir < 8; dir += 2, check_flags <<= 2) {

            lut_idx = (check_flags & 0xE000) >> 13;

            for (i = 0; i < 3; i++) {
                /* Set all adjacent fields according to lookup-table */
                edit_state -> fan_no = adjacent[dir + i];
                if (edit_state -> fan_no >= 0) {

                    shape_no = LutFloor[lut_idx].adj_shapes[i];
                    rotdir   = LutFloor[lut_idx].adj_rotdir[i];

                    if (shape_no >= 0) {
                        memcpy(&edit_state -> ft, &PresetTiles[shape_no], sizeof(FANDATA_T));
                        /* Rotate and the set the shape */
                        editmainFanTypeRotate(edit_state -> ft.type,
                                              &edit_state -> fd,
                                              rotdir);
                        editmainDoFanUpdate(mesh,
                                            edit_state,
                                            base_x + adj_xy[dir + i].x,
                                            base_y + adj_xy[dir + i].y);
                    }
                }
            }
        }

    }
    else {

        if (mesh -> fan[edit_state -> fan_chosen].type != PresetTiles[EDITMAIN_PRESET_FLOOR].type) {
            /* No change at all */
            return 1;

        }

        /* Set the middle position depending on the adjacent tiles 'North-East' */
        lut_idx = (check_flags & 0xE000) >> 13;

        shape_no = LutWall[lut_idx].center_shape;
        rotdir   = LutWall[lut_idx].center_rotdir;

        memcpy(&edit_state -> ft, &PresetTiles[shape_no], sizeof(FANDATA_T));
        /* Rotate and the set the shape */
        editmainFanTypeRotate(edit_state -> ft.type,
                              &edit_state -> fd,
                              rotdir);
        edit_state -> ft.type |= 0x20;    /* Walls have 'big' textures */
        editmainDoFanUpdate(mesh, edit_state, base_x, base_y);

        for (dir = 0; dir < 8; dir += 2, check_flags <<= 2) {

            lut_idx = (check_flags & 0xE000) >> 13;

            for (i = 0; i < 3; i++) {
                /* Set all adjacent fields according to lookup-table */
                edit_state -> fan_no = adjacent[dir + i];
                if (edit_state -> fan_no >= 0) {

                    shape_no = LutWall[lut_idx].adj_shapes[i];
                    rotdir   = LutWall[lut_idx].adj_rotdir[i];

                    if (shape_no >= 0) {
                        memcpy(&edit_state -> ft, &PresetTiles[shape_no], sizeof(FANDATA_T));
                        /* Rotate and the set the shape */
                        editmainFanTypeRotate(edit_state -> ft.type,
                                              &edit_state -> fd,
                                              rotdir);
                        /* Walls have 'big' textures */
                        edit_state -> ft.type |= 0x20;
                        editmainDoFanUpdate(mesh,
                                            edit_state,
                                            base_x + adj_xy[dir + i].x,
                                            base_y + adj_xy[dir + i].y);
                    }
                }
            }
        }
    }
    
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
 *     Does the work for editing and sets edit states, if needed
 * Input:
 *     mesh *: Pointer on mesh  to fill with default values
 *     which:  Type of map to create : EDITMAIN_NEWFLATMAP / EDITMAIN_NEWSOLIDMAP
 */
static int editmainCreateNewMap(MESH_T *mesh, int which)
{

    int x, y, fan, zadd;
    char fan_fx;


    memset(mesh, 0, sizeof(MESH_T));

    /* Size of 8 for test purposes -- 2010-08-19 / bitnapper */
    mesh -> tiles_x     = 8;    /* EDITMAIN_MAX_MAPSIZE;    */
    mesh -> tiles_y     = 8;    /* EDITMAIN_MAX_MAPSIZE;    */
    mesh -> numvert     = 0;                         /* Vertices used in map    */
    mesh -> numfreevert = MAXTOTALMESHVERTICES - 10; /* Vertices left in memory */

    zadd   = 0;
    fan_fx = 0;

    if (which == EDITMAIN_NEWSOLIDMAP) {
        zadd   = 192;                           /* All walls resp. top tiles  */
        fan_fx = (MPDFX_WALL | MPDFX_IMPASS);   /* All impassable walls       */
    }

    fan = 0;
    for (y = 0; y < mesh -> tiles_y; y++) {

        for (x = 0; x < mesh -> tiles_x; x++) {

            mesh -> fan[fan].type  = 0;
            if (which != EDITMAIN_NEWSOLIDMAP) {
                mesh -> fan[fan].tx_no = (unsigned char)((((x & 1) + (y & 1)) & 1) + EDITMAIN_DEFAULT_TILE);
            }
            else {
                mesh -> fan[fan].tx_no = EDITMAIN_TOP_TILE;
            }

            mesh -> fan[fan].fx = fan_fx;

            if (! editmainFanAdd(mesh, fan, x*EDITMAIN_TILEDIV, y*EDITMAIN_TILEDIV, zadd))
            {
                sprintf(EditState.msg, "%s", "NOT ENOUGH VERTICES!!!");
                return 0;
            }

            fan++;

        }

    }

    return 1;   /* TODO: Return 1 if mesh is created */

}

/*
 * Name:
 *     editfileSetVrta
 * Description:
 *     Set the 'vrta'-value for given vertex
 * Input:
 *     mesh *: Pointer on mesh  to handle
 *     vert:   Number of vertex to set
 */
static int editfileSetVrta(MESH_T *mesh, int vert)
{
    /* TODO: Get all needed functions from cartman code */
    int newa, x, y, z;
    int brx, bry;
    /*
    int , brz, deltaz, dist, cnt;
    int newlevel, distance, disx, disy;
    */

    /* To make life easier  */
    x = mesh -> vrtx[vert];
    y = mesh -> vrty[vert];
    z = mesh -> vrtz[vert];

    // Directional light
    brx = x + 64;
    bry = y + 64;
    /*
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
    EditState.ft.type       = -1;   /* No fan-type chosen                   */    
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
 *      command:  What to do
 * Output:
 *      Result of given command
 */
int editmainMap(int command)
{

    int cnt, x, y;


    switch(command) {

        case EDITMAIN_DRAWMAP:
            editdraw3DView(&Mesh, EditState.fan_chosen, &EditState.ft, &EditState.fd);
            return 1;

        case EDITMAIN_NEWFLATMAP:
        case EDITMAIN_NEWSOLIDMAP:
            if (editmainCreateNewMap(&Mesh, command)) {

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

        case EDITMAIN_ROTFAN:
            if (-1 != EditState.fan_chosen) {
                EditState.fan_dir++;
                EditState.fan_dir &= 0x03;

                editmainFanTypeRotate(EditState.bft_type,
                                      &EditState.fd,
                                      EditState.fan_dir);
                /* Now translate the fan to chosen fan position */
                x = EditState.tx * 128;
                y = EditState.ty * 128;

                for (cnt = 0; cnt < EditState.fd.numvertices; cnt++) {
                    EditState.fd.vtx[cnt].x += x;
                    EditState.fd.vtx[cnt].y += y;
                }

            }
            return 1;
            
        case EDITMAIN_UPDATEFAN:
            editmainDoFanUpdate(&Mesh,
                                &EditState,
                                EditState.tx * 128.0,
                                EditState.ty * 128.0);
            break;

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
                EditState.ft.fx = Mesh.fan[EditState.fan_chosen].fx;
            }
            else {
                EditState.ft.fx = 0;
            }
            break;

        case EDITMAIN_TOGGLE_TXHILO:
            if (EditState.fan_chosen >= 0) {
                Mesh.fan[EditState.fan_chosen].tx_flags ^= flag;
                /* And copy it for display */
                EditState.ft.tx_flags = Mesh.fan[EditState.fan_chosen].tx_flags;
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
    EditState.tx = Mesh.tiles_x * cx / w;
    EditState.ty = Mesh.tiles_y * cy / h;

    fan_no = (EditState.ty * Mesh.tiles_x) + EditState.tx;
    
    if (fan_no >= 0 && fan_no < Mesh.numfan) {
    
        EditState.fan_chosen = fan_no;
        /* And fill it into 'EditState' for display */
        memcpy(&EditState.ft, &Mesh.fan[fan_no], sizeof(FANDATA_T));
        
        /* And now set camera to move/look at this position */
        sdlgl3dMoveToPosCamera(0, EditState.tx * 128.0, EditState.ty * 128.0, 0, 1);
            
    }
 
}

/*
 * Name:
 *     editmainFanTypeName
 * Description:
 *     Returns the description of given fan-type 
 * Input:
 *     type_no: Number of fan type
 * Output:
 *     Pointer on a valid string, may be empty 
 */
char *editmainFanTypeName(int type_no)
{

    if (type_no >= 0 && type_no < MAXMESHTYPE) {

        if (pCommands[type_no & 0x1F].name != 0) {
        
            return pCommands[type_no & 0x1F].name;
        
        }
    
    }
    return "";

}

/*
 * Name:
 *     editmainChooseFanType
 * Description:
 *     Chooses a fan type and sets it for display.
 *     TODO: Ignore 'empty' fan types
 * Input:
 *     type_no:    Number of fan type  (-2: Switch off, 0: Switch on)
 *     direction:  != 0: Browsing direction trough list...
 *     fan_name *: Where to print hte name of chosen fan
 */
void editmainChooseFanType(int dir, char *fan_name)
{

    char i;
    int  x, y;


    if (dir == -2) {
        /* Reset browsing */
        EditState.bft_type = -1;
        fan_name[0] = 0;
        return;
    }
    else if (dir == 0) {
        /* Start browsing */
        EditState.bft_type = 0;
    }
    else if (dir == -1) {
        if (EditState.bft_type > 0) {
            EditState.bft_type--;
        }
    }
    else {
        if (EditState.bft_type < 30) {
            EditState.bft_type++;
        }
    }

    EditState.bft_type &= 0x1F;    /* Be on the save side with MAXTYPES */

    memcpy(&EditState.fd, &pCommands[EditState.bft_type], sizeof(COMMAND_T));
    /* Now move it to the chosen position */
    x = EditState.tx * 128;
    y = EditState.ty * 128;

    for (i = 0; i < EditState.fd.numvertices; i++) {
        EditState.fd.vtx[i].x += x;
        EditState.fd.vtx[i].y += y;
    }

    sprintf(fan_name, "%s", editmainFanTypeName(EditState.bft_type));

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

    if (EditState.fan_chosen >= 0) {

        editdraw2DTex(x, y, w, h,
                      EditState.ft.tx_no,
                      EditState.ft.type & COMMAND_TEXTUREHI_FLAG);
        
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
 *     edit_state: How to handle the command 
 *     is_floor:   Set floor in simple mode       
 */
int editmainFanSet(char edit_state, char is_floor)
{

    if (EditState.fan_chosen >= 0) {
        
        if (edit_state == EDITMAIN_EDIT_NONE) {
            /* Do nothing, is view-mode */
            return 1;
        }
        else if (edit_state == EDITMAIN_EDIT_SIMPLE) {
 
            return editmainSetFanSimple(&Mesh, &EditState, is_floor);
                                        
        }
        else if (edit_state == EDITMAIN_EDIT_FULL) {
            /* Do 'simple' editing */
            /*
            return editmainDoFanUpdate(&Mesh, &EditState);
            */
        }
    }

    return 0;
}