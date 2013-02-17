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
#include <string.h>

#include "sdlgl3d.h"            /* DEG2RAD                              */
#include "editor.h"
#include "egodefs.h"            /* MPDFX_...                            */
#include "egomap.h"             /* Map data and handling it             */
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

#define COMMAND_TEXTUREHI_FLAG 0x20

/*******************************************************************************
* DATA							                                               *
*******************************************************************************/

static MESH_T *Mesh;            /* Pointer on actual mesh   */

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

	
    src_xy.x = fan % mesh->tiles_x;
	src_xy.y = fan / mesh->tiles_x;
    
	num_adj = 0;            /* Count them */   
            
	for (dir = 0; dir < 8; dir++) {

		adj_pos = -1;       /* Assume invalid */		
		
		dest_xy.x = src_xy.x + AdjacentXY[dir].x;
		dest_xy.y = src_xy.y + AdjacentXY[dir].y;
		
		if ((dest_xy.x >= 0) && (dest_xy.x < mesh->tiles_x)
			&& (dest_xy.y >= 0) && (dest_xy.y < mesh->tiles_y)) {

			adj_pos = (dest_xy.y * mesh->tiles_x) + dest_xy.x;
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

	for (fan_no = 0, vertex_no = 0; fan_no < mesh->numfan; fan_no++) {

		mesh->vrtstart[fan_no] = vertex_no;
		vertex_no += mesh->pcmd[mesh->fan[fan_no].type & 0x1F].numvertices;

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


	ft = &mesh->pcmd[mesh->fan[fan].type & 0x1F];

	if (mesh->numfreevert >= ft->numvertices)
	{

		vertex = mesh->numvert;

		mesh->vrtstart[fan] = vertex;

		for (cnt = 0; cnt < ft->numvertices; cnt++)
        {
			mesh->vrtx[vertex] = x + ft->vtx[cnt].x;
			mesh->vrty[vertex] = y + ft->vtx[cnt].y;
			mesh->vrtz[vertex] = ft->vtx[cnt].z;
			vertex++;
		}

		mesh->numvert     += ft->numvertices;   /* Actual number of vertices used   */
		mesh->numfreevert -= ft->numvertices;   /* Vertices left for edit           */

		return 1;
	}

	return 0;
}

/*
 * Name:
 *     editmainDoFanUpdate
 * Description:
 *     This function updates the fan at given 'fan 'with the
 *     info in 'es ->ft'.
 *     If the type has changed, the vertice data is updated.
 * Input:
 *     mesh *:       Pointer on mesh to handle
 *     edit_state *: Pointer on edit state, holding all data needed for work
 *     fan_no:       Do an update of this fan
 * Output:
 *    Fan could be updated, yes/no
 */
static int editmainDoFanUpdate(MESH_T *mesh, EDITMAIN_INFO_T *edit_state, int fan_no)
{

	COMMAND_T *act_cm;
	FANDATA_T *act_fd;
	int cnt;
	int vrt_diff, vrt_size;
	int src_vtx, dst_vtx;
	int vertex;
	int tx, ty;


	act_fd = &mesh->fan[fan_no];
	act_cm = &mesh->pcmd[act_fd->type & 0x1F];

	/* Do an update on the 'static' data of the fan */
	act_fd->tx_no    = edit_state->ft.tx_no;
	act_fd->tx_flags = edit_state->ft.tx_flags;
	act_fd->fx       = edit_state->ft.fx;

	/* Now change tile pos to position in units */
	tx = (fan_no % mesh->tiles_x) * 128;
	ty = (fan_no / mesh->tiles_x) * 128;

	if (act_fd->type == edit_state->ft.type)
    {
		vrt_diff = 0;       /* May be rotated */
	}
	else
    {
		vrt_diff = edit_state->cm.numvertices - act_cm->numvertices;
	}

	if (0 == vrt_diff)
    {
		/* Same number of vertices, only overwrite is needed */
		/* Set the new type of fan */
		act_fd->type = edit_state->ft.type;
	}
	else
    {
		if (vrt_diff > 0)
        {
			/* Insert more vertices -- check for space */
			if (vrt_diff > mesh->numfreevert)
            {
				/* Not enough vertices left in buffer */
				return 0;
			}
		}
		/* Set the new type of fan */
		act_fd->type = edit_state->ft.type;

		/* Copy from start vertex of next fan */
		src_vtx = mesh->vrtstart[fan_no] + act_cm->numvertices;
		dst_vtx = mesh->vrtstart[fan_no] + edit_state->cm.numvertices;
		/* Number of vertices to copy */
		vrt_size = (mesh->numvert - src_vtx + 1) * sizeof(int);
		/* Add/remove vertices -- update it's numbers */
		mesh->numfreevert -= vrt_diff;
		mesh->numvert     += vrt_diff;
		/* Now, clear space or new vertices or remove superfluos vertices */
		/* dest, src, size */
		memmove(&mesh->vrtx[dst_vtx], &mesh->vrtx[src_vtx], vrt_size);
		memmove(&mesh->vrty[dst_vtx], &mesh->vrty[src_vtx], vrt_size);
		memmove(&mesh->vrtz[dst_vtx], &mesh->vrtz[src_vtx], vrt_size);
	}

	/* Fill in the vertex values from type definition */
	vertex = mesh->vrtstart[fan_no];
    
	for (cnt = 0; cnt < edit_state->cm.numvertices; cnt++)
    {
		/* Replace actual values by new values */
		mesh->vrtx[vertex] = tx + edit_state->cm.vtx[cnt].x;
		mesh->vrty[vertex] = ty + edit_state->cm.vtx[cnt].y;
		mesh->vrtz[vertex] = edit_state->cm.vtx[cnt].z;
		vertex++;
	}

	/* Adjust 'vrtstart' for all following fans */
	if (vrt_diff != 0)
    {
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
	memcpy(dest, &Mesh->pcmd[type & 0x1F], sizeof(COMMAND_T));

	if (dir != EDITMAIN_NORTH)
    {
		angle  = DEG2RAD(dir * 90.0);

		for (cnt = 0; cnt < dest->numvertices; cnt++)
        {
			/* First translate it to have rotation center in middle of fan */
			dest->vtx[cnt].x -= 64.0;
			dest->vtx[cnt].y -= 64.0;
			/* And now rotate it */
			result_x = (cos(angle) * (double)dest->vtx[cnt].x
						- sin(angle) * (double)dest->vtx[cnt].y);
			result_y = (sin(angle) * (double)dest->vtx[cnt].x
						+ cos(angle) * (double)dest->vtx[cnt].y);
			/* And store it back */
			dest->vtx[cnt].x = floor(result_x);
			dest->vtx[cnt].y = floor(result_y);
			/* And move it back to start position */
			dest->vtx[cnt].x += 64.0;
			dest->vtx[cnt].y += 64.0;
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


	numvertices1 = mesh->pcmd[mesh->fan[fan_no1].type & 0x1F].numvertices;
	numvertices2 = mesh->pcmd[mesh->fan[fan_no2].type & 0x1F].numvertices;
	num_welded   = 0;

	vtx1 = mesh->vrtstart[fan_no1];

	for (cnt1 = 0; cnt1 < numvertices1; cnt1++)
    {
		/* Compare all vertices of first fan with this of second fan */
		vtx2 = mesh->vrtstart[fan_no2];

		for (cnt2 = 0; cnt2 < numvertices2; cnt2++)
        {
			if (mesh->vrtz[vtx1] == mesh->vrtz[vtx2])
            {
				/* Look for vertex in distance... */
				xdiff = mesh->vrtx[vtx1] - mesh->vrtx[vtx2];
				ydiff = mesh->vrty[vtx1] - mesh->vrty[vtx2];
				
				if (xdiff < 0) xdiff = -xdiff;
				if (ydiff < 0) ydiff = -ydiff;

				if ((xdiff + ydiff) < 12.0)
                {
					mesh->vrtx[vtx2] = mesh->vrtx[vtx1];
					mesh->vrty[vtx2] = mesh->vrty[vtx1];
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
 *     editmainCreateNewMap
 * Description:
 *     Creates a new, empty map consisting of all floor or wall types,
 *     depending on 'which'
 * Input:
 *     es *:   Editor info needed for work
 *     mesh *: Pointer on mesh  to fill with default values
 */
static int editmainCreateNewMap(EDITMAIN_INFO_T *es, MESH_T *mesh)
{
	int x, y, fan;


    /* --- Now create the vertices --- */
	mesh->numvert     = 0;                         /* Vertices used in map              */
	mesh->numfreevert = MAXTOTALMESHVERTICES - 10; /* Vertices left in memory           */

	fan = 0;
    
	for (y = 0; y < mesh->tiles_y; y++)
    {
		for (x = 0; x < mesh->tiles_x; x++)
        {
			mesh->fan[fan].type  = WALLMAKE_TOP;
			mesh->fan[fan].fx    = (MPDFX_WALL | MPDFX_IMPASS);
			mesh->fan[fan].tx_no = EDITMAIN_TOP_TILE;

			if (! editmainFanAdd(mesh, fan, x*EDITMAIN_TILEDIV, y*EDITMAIN_TILEDIV))
			{
				sprintf(es->msg, "%s", "NOT ENOUGH VERTICES!!!");
				return 0;
			}

			fan++;
		}
	}

	return 1;
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
	wi[0].type   = mesh->fan[fan].type;
	wi[0].dir    = -1;

	editmainGetAdjacent(mesh, fan, adjacent);
	for (i = 0; i < 8; i++)
    {
		wi[i + 1].fan_no = adjacent[i];
		wi[i + 1].type   = mesh->fan[adjacent[i]].type;
		wi[i + 1].dir    = -1;
	}  
}

/*                                             
 * Name:
 *     editmainTranslateWallMakeInfo
 * Description:
 *     Translates the given wallmaker info into fans on map
 * Input:
 *     es *:     Edit-Info to work with
 *     mesh*:    Pointer on mesh to get the info from
 *     wi *:     Array from wallmaker to create walls from
 *     num_tile: Number of tiles to generate
 */
static void editmainTranslateWallMakeInfo(EDITMAIN_INFO_T *es, MESH_T *mesh, WALLMAKER_INFO_T *wi, int num_tile)
{

	int i, i2;
	char type_no;


	for (i = 0; i < num_tile; i++)
    {
		if (wi[i].fan_no >= 0 && wi[i].dir >= 0)
        {
			/* -- Do update in any case */
			type_no = wi[i].type;

			es->ft.type     = type_no;
			es->ft.tx_flags = 0;
			es->ft.fx       = mesh->pcmd[type_no & 0x1F].default_fx;
			es->ft.tx_no    = mesh->pcmd[type_no & 0x1F].default_tx_no;
			es->fan_dir     = wi[i].dir;

			memcpy(&es->cm, &mesh->pcmd[type_no & 0x1F], sizeof(COMMAND_T));

			editmainFanTypeRotate(es->ft.type,
								  &es->cm,
								  es->fan_dir);

			editmainDoFanUpdate(mesh, es, wi[i].fan_no);
		}
	}

	/* Weld the new generated walls to middle tile */
	for (i = 1; i < num_tile; i++)
    {
		editmainWeldXY(mesh, wi[0].fan_no,  wi[i].fan_no);
	}

	/* Weld the new generated walls with each other */
    if (num_tile > 1)
    {
	    for (i = 0; i < num_tile; i++)
        {
		    i2 = i + 1;
		    if (i2 > (num_tile - 1))
            {
			    i2 = 0;
		    }
		    /* Only weld if both tiles are walls */
		    if (wi[i].type > 1 && wi[i2].type > 1)
            {
			    editmainWeldXY(mesh, wi[i].fan_no, wi[i2].fan_no);
		    }
	    }
    }
}

/*
 * Name:
 *     editmainUpdateChosenFreeFan
 * Description:
 *     Sets the data for the chosen tile type for 'free' mode
 * Input:
 *     es *: Pointer on Edit-State to work with
 */
static void editmainUpdateChosenFreeFan(EDITMAIN_INFO_T *es)
{
	int cnt, tx, ty;


	if (es->crect[0] > 0 && es->crect[1] > 0)
    {
		/* Get copy at original position */
		memcpy(&es->cm, &Mesh->pcmd[es->ft.type & 0x1F], sizeof(COMMAND_T));

		editmainFanTypeRotate(es->ft.type,
							  &es->cm,
							  es->fan_dir);

		/* Now translate the fan to chosen fan position */
		tx = es->crect[0] * 128;
		ty = es->crect[1] * 128;

		for (cnt = 0; cnt < es->cm.numvertices; cnt++)
        {
			es->cm.vtx[cnt].x += tx;
			es->cm.vtx[cnt].y += ty;
		}
	}
}

/*
 * Name:
 *     editmainSetTile
 * Description:
 *     Sets a fan at the actual chosen position, depending on edit_state.
 *     Does an update on given fan. Including changed number of vertices,
 *     if needed.
 * Input:
 *     es *:     Info to work with
 *     x, y:     Position of tile on map
 *     is_floor: Set floor in simple mode, else set wall
 * Output:
 *     Has done something ( valid EDITMAIN_MODE_...)
 */
static int editmainSetTile(EDITMAIN_INFO_T *es, int x, int y, int is_floor)
{
	WALLMAKER_INFO_T wi[12];            /* List of fans to create */
	int num_wall;
    int fan_no;


    /* First check if in boundary of map */
    if (x > 0 && y > 0
		&& x < (Mesh->tiles_x - 1)
		&& y < (Mesh->tiles_y - 1))
    {
        fan_no = (y * Mesh->tiles_x) + x;

		if (es->edit_mode == EDITMAIN_MODE_MAP)
        {
            /* Get a list of fans surrounding this one */
            editmainCreateWallMakeInfo(Mesh, fan_no, wi);

            num_wall = wallmakeMakeTile(wi);

            if (num_wall)
            {
                /* Create tiles from WALLMAKER_INFO_T */
                editmainTranslateWallMakeInfo(es, Mesh, wi, num_wall);
                es->cm.numvertices = 0;
            }

            return 1;
        }
		else if (es->edit_mode == EDITMAIN_MODE_FAN)
        {
			if (is_floor)
            {
				editmainUpdateChosenFreeFan(es);
			}
			else
            {
				memcpy(&es->cm, &Mesh->pcmd[es ->ft.type & 0x1F], sizeof(COMMAND_T));
				editmainFanTypeRotate(es->ft.type,
									  &es->cm,
									  es->fan_dir);
				editmainDoFanUpdate(Mesh, es, fan_no);
			}

            return 1;
		}
		else if (es->edit_mode == EDITMAIN_MODE_FAN_FX)
        {
			/* Do an update on the FX-Flags */
			Mesh->fan[fan_no].fx = es->ft.fx;
            return 1;
		}
		else if (es->edit_mode == EDITMAIN_MODE_FAN_TEX)
        {
			/* Change the texture */
			Mesh->fan[fan_no].tx_no    = es->ft.tx_no;
			Mesh->fan[fan_no].tx_flags = es->ft.tx_flags;
            return 1;
		}
	}

	return 0;
}

/*
 * Name:
 *     editmainFanTypeName
 * Description:
 *     Returns the description of given fan-type
 * Input:
 *     mesh *: This mesh
 *     es *:     Edit-State to handle
 */
static void editmainFanTypeName(MESH_T *mesh, EDITMAIN_INFO_T *es)
{

	char type_no;


	if (es->ft.type >= 0) {

		type_no = (char)(es->ft.type & 0x1F);

		if (mesh->pcmd[type_no].name != 0)
        {
			sprintf(es->fan_name, "%s", mesh->pcmd[type_no].name);
			return;
		}
	}

	es->fan_name[0] = 0;
}

/*
 * Name:
 *     editmainSetMinimapSize
 * Description:
 *     Sets the size ofthe minimap
 * Input:
 *     mesh *: This mesh
 *     es *:   Edit-State to handle
 */
static void editmainSetMinimapSize(MESH_T *mesh, EDITMAIN_INFO_T *es)
{

    /* --- Set size of minimap --- */
    es->minimap_tw = 256 / mesh->tiles_x;

    if (es->minimap_tw < 3)
    {
        /* At  least 3 pixels width for a 2D-Tile */
        es->minimap_tw  = 3;
    }

    /* --- Now calc the whole size of minimap --- */
    es->minimap_w = es->minimap_tw * mesh->tiles_x;
    es->minimap_h = es->minimap_tw * mesh->tiles_y;
}

/*
 * Name:
 *     editmainBuildMap
 * Description:
 *     Builds in map walls or floors, depending on 'is_floor'
 * Input:
 *     es *:     Pointer on edit-state with chosen area
 *     is_floor: Build floor tile(s)
 */
static void editmainBuildMap(EDITMAIN_INFO_T *es, int is_floor)
{
    int x, y;
    WALLMAKER_INFO_T wi;


    /* ------- Set one or more fans, if in 'build'-mode --------- */
    if (es->crect[0] > 0 && es->crect[1] > 0)
    {
        /* --- First step: Change fans to 'basic' wall / floor */
        if (is_floor)
        {
            wi.type = WALLMAKE_FLOOR;
        }
        else
        {
            wi.type = WALLMAKE_TOP;
        }

        wi.dir = WALLMAKE_NORTH;

        /* Change all chosen tiles to chosen type: Either 'bottom' or 'top'  */
        for (y = es->crect[1]; y <= es->crect[3]; y++)
        {
            for (x = es->crect[0]; x <= es->crect[2]; x++)
            {

                wi.fan_no = (y * Mesh->tiles_x) + x;

                editmainTranslateWallMakeInfo(es, Mesh, &wi, 1);
            }
        }

        /* Second step: For each tile, adjust the adjacent tiles which are  */
        /*              not floor tiles                                     */
        for (y = es->crect[1]; y <= es->crect[3]; y++)
        {
            for (x = es->crect[0]; x <= es->crect[2]; x++)
            {
                editmainSetTile(es, x, y, is_floor);
            }
        }

        /* Reset number of chosen tile(s), because it's done */
        es->crect[0] = 0;
        es->crect[1] = 0;
        es->crect[2] = 0;
        es->crect[3] = 0;
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
 *     es *:     Pointer on edit-state to intialize
 *     map_size: Default map size
 */
void editmainInit(EDITMAIN_INFO_T *es, int map_size)
{

	editdrawInitData();

    /* --- Initialie the edit data --- */
	memset(es, 0, sizeof(EDITMAIN_INFO_T));

	es->display_flags   |= EDITMAIN_SHOW2DMAP;
	es->map_size        = map_size;
    es->minimap_tw      = 256 / map_size;
	es->minimap_w       = map_size * es->minimap_tw;
	es->minimap_h       = map_size * es->minimap_tw;
	es->draw_mode       = (EDIT_DRAWMODE_SOLID | EDIT_DRAWMODE_TEXTURED | EDIT_DRAWMODE_LIGHTMAX);

    /* --- Do additional initialization for the map module --- */
    egomapInit();

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
 *      es *:     Pointer on edit-stat with info for commands
 *      command:  What to do
 * Output:
 *      Result of given command
 */
int editmainMap(EDITMAIN_INFO_T *es, int command)
{

	switch(command)
    {
		case EDITMAIN_DRAWMAP:
            egomapDraw(&es->ft, &es->cm, es->crect);
			return 1;

		case EDITMAIN_NEWMAP:
        case EDITMAIN_LOADMAP:
            Mesh = egomapLoad((char)(EDITMAIN_NEWMAP ? 0 : 1), es->msg, es->map_size);
			if (Mesh)
            {
                Mesh->draw_mode = es->draw_mode;

                if (command == EDITMAIN_NEWMAP)
                {
                    editmainCreateNewMap(es, Mesh);
                }

				sdlgl3dInitVisiMap(Mesh->tiles_x, Mesh->tiles_y, 128.0);
                /* And move camera to standard basis point */
                sdlgl3dMoveToPosCamera(0, 384.0, 384.0, 600.0, 0);
                /* And no fan is selected */
                es->crect[0] = 0;
                es->crect[1] = 0;
                es->map_loaded = 1;

                editmainSetMinimapSize(Mesh, es);
                
				return 1;
			}
			break;		

		case EDITMAIN_SAVEMAP:
            return egomapSave(es->msg, EGOMAP_SAVE_MAP);			

		case EDITMAIN_ROTFAN:
			if (es->edit_mode == EDITMAIN_MODE_FAN)
            {
				/* Rotate the chosen fan type, if active edit mode */
				es->fan_dir++;
				es->fan_dir &= 0x03;
				editmainUpdateChosenFreeFan(es);
			}
			return 1;
	}

	return 0;
}

/*
 * Name:
 *     editmainToggleFlag
 * Description:
 *     Toggles the given flag using the data in given edit-info 'es'
 *     Adjust accompanied
 * Input:
 *     es *:  Edit-State to handle
 *     which: Which flag to change
 *     flag:  Info about flag 
 */
char editmainToggleFlag(EDITMAIN_INFO_T *es, int which, unsigned char flag)
{
	char tex_no;
	
	
	switch(which)
    {
		case EDITMAIN_TOGGLE_DRAWMODE:
			Mesh->draw_mode ^= flag;                 /* Change it in actual map */
			es->draw_mode = Mesh->draw_mode;   /* Show it in edit_state */
			break;

		case EDITMAIN_TOGGLE_EDITSTATE:
			if (flag == 0) {
				es->edit_mode = EDITMAIN_MODE_MAP;
			}
			else {
				es->edit_mode++;
				if (es->edit_mode > EDITMAIN_MODE_FAN_TEX) {
					es->edit_mode = EDITMAIN_MODE_MAP;
				}
			}
			if (es->edit_mode == EDITMAIN_MODE_FAN) {
				/* Initialize the chosen fan    */
				editmainChooseFanType(es, 0);
			}
			else {
				es->cm.numvertices = 0;   /* Hide the chosen fan */
			}
			return es->edit_mode;
			
		case EDITMAIN_TOGGLE_FANTEXNO:
			tex_no = (char)(es->ft.tx_no >> 6);
			
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
			es->ft.tx_no &= 0x3F;
			es->ft.tx_no |= (char)((tex_no & 0x03) << 6);
			break;

	}

	return 0;

}

/*
 * Name:
 *     editmainChooseFan
 * Description:
 *     Chooses one or multiple fans
 *     Choose a fan from given position in rectangle of size w/h.
 *     Does an update on the 'EditState'.
 *     Fills info about chosen fan into Edit-State, depending on Edit-State
 *     Create map walls/floors if Edit-Mode is 'EDITMAIN_MODE_MAP'
 * Input:
 *     es *:     Edit-State to handle
 *     is_floor: Sets a fan, depending on 'es->edit_mode'
 */
void editmainChooseFan(EDITMAIN_INFO_T *es, int is_floor)
{

    int x, y;
    int fan_no;


	if (! es->display_flags & EDITMAIN_SHOW2DMAP) {

		es->cm.numvertices = 0;
		/* Map not visible, invalid command */
		return;

	}

    switch(es->edit_mode) {

        case EDITMAIN_MODE_OFF:
            /* Get the info about the chosen fan */
            /* In info-mode return the properties of the chosen fan */
            fan_no = (es->crect[1] * Mesh->tiles_x) + es->crect[0];
            if (fan_no > 0) {
                memcpy(&es->ft, &Mesh->fan[fan_no], sizeof(FANDATA_T));
            }
            break;

        case EDITMAIN_MODE_MAP:
            editmainBuildMap(es, is_floor);
            break;

        case EDITMAIN_MODE_FAN:
        case EDITMAIN_MODE_FAN_FX:
        case EDITMAIN_MODE_FAN_TEX:
            /* ------- Set one or more fans, if in 'build'-mode --------- */
            if (es->crect[0] > 0 && es->crect[1] > 0) {

                for (y = es->crect[1]; y <= es->crect[3]; y++) {

                    for (x = es->crect[0]; x <= es->crect[2]; x++) {

                        if (! editmainSetTile(es, x, y, is_floor)) {

                            return;

                        }

                    }

                }


                /* Save it for the camera */
                es->tx = es->crect[0];
                es->ty = es->crect[1];

                /* Reset number of chosen fan(s), because it's done */
                es->crect[0] = 0;
                es->crect[1] = 0;

            }
            break;

        case EDITMAIN_MODE_FREE:
            if (es->cm.numvertices > 0) {
                
    			editmainUpdateChosenFreeFan(es);
                    
    		}
            break;

    }
    
    /* And now set camera to move/look at this position */            
    editdrawAdjustCamera(es->tx, es->ty);    

}

/*
 * Name:
 *     editmainChooseFanType
 * Description:
 *     Chooses a fan type and sets it for display.
 *     TODO: Ignore 'empty' fan types
 * Input:
 *     es *:     Edit-State to handle  
 *     dir:       Direction to browse trough list
 */
void editmainChooseFanType(EDITMAIN_INFO_T *es, int dir)
{

	char i;
	int  x, y;


	if (es->edit_mode != EDITMAIN_MODE_FAN) {
    
		/* Do that only in 'Free' Mode */
		es->fan_name[0] = 0;
		return;
        
	}
	if (dir == 0) {
		/* Start browsing */
		es->bft_no = 0;
	}
	else {
		if (es->bft_no < EDITMAIN_PRESET_MAX) {
			es->bft_no++;
		}
		else {  /* Wrap around */
			es ->bft_no = 0;
		}
	}

	es->fan_dir = 0;

	memcpy(&es->ft, &PresetTiles[es->bft_no], sizeof(FANDATA_T));
	memcpy(&es->cm, &Mesh->pcmd[es->ft.type & 0x1F], sizeof(COMMAND_T));

	/* Now move it to the chosen position */
	x = es->tx * 128;
	y = es->ty * 128;

	for (i = 0; i < es->cm.numvertices; i++) {

		es->cm.vtx[i].x += x;
		es->cm.vtx[i].y += y;
        
	}

	editmainFanTypeName(Mesh, es);

}

/*
 * Name:
 *     editmain2DTex
 * Description:
 *     Draws the texture and chosen texture-part of actual chosen fan
 *     into given rectangle. Uses data from EditState
 * Input:
 *     es *:     Edit-State to handle
 *     x, y, w, h: Rectangle to draw into
 */
void editmain2DTex(EDITMAIN_INFO_T *es, int x, int y, int w, int h)
{

	if (es->crect[0] >= 0) {

		editdraw2DTex(x, y, w, h,
					  es->ft.tx_no,
					  (char)(es->ft.type & COMMAND_TEXTUREHI_FLAG));

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
 *     es *:   Edit-State to handle   
 *     cx, cy: Chosen point in rectangle
 *     w,h:    Extent of rectangle
 */
void editmainChooseTex(EDITMAIN_INFO_T *es, int cx, int cy, int w, int h)
{

	int tex_x, tex_y;
	char tex_no;
	char new_tex;
 
	
	/* Save it as x/y-position, too */
	tex_x = 8 * cx / w;
	tex_y = 8 * cy / h;
	
	if (es ->ft.type & 0x20) {
    
		if (tex_x > 6) tex_x = 6;
		if (tex_y > 6) tex_y = 6;
        
	}

	tex_no = (char)((tex_y * 8) + tex_x);
	if (tex_no >= 0 && tex_no < 63) {
    
		/* Valid texture_no, merge it with number of main texture */
		new_tex = (char)(es ->ft.tx_no & 0xC0);
		new_tex |= tex_no;
        
		/* Do an update of the edit-state and the mesh */
		es->ft.tx_no = new_tex;
        
	}      
  
}

/*
 * Name:
 *     editmainMoveCamera
 * Description:
 *     Moves the camera to look at given position
 * Input:
 *     x, y: Position of tile the camera should look to
 */
void editmainMoveCamera(int x, int y)
{
    /* And now set camera to move/look at this position */
    editdrawAdjustCamera(x, y);
}
