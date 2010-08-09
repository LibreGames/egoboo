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

#include <stdio.h>

#include "editdraw.h"


#include "editfile.h"     /* Own header */

/*******************************************************************************
* DEFINES								                                       *
*******************************************************************************/

#define EDITFILE_SLOPE  50          /* Twist stuff          */
#define MAPID     0x4470614d        /*  The string... MapD  */

/*******************************************************************************
* CODE 								                                           *
*******************************************************************************/

/*
 * Name:
 *     editfileMakeTwist
 * Description:
 *     Generates the number of the 'up'-vector for given fan
 * Input:
 *     mesh*: Pointer on mesh to set twists for
 */
static unsigned char editfileGetFanTwist(MESH_T *mesh, int fan)
{

    int zx, zy, vt0, vt1, vt2, vt3;
    unsigned char twist;


    vt0 = mesh -> vrtstart[fan];
    vt1 = vt0 + 1;
    vt2 = vt0 + 2;
    vt3 = vt0 + 3;

    zx = (mesh-> vrtz[vt0] + mesh -> vrtz[vt3] - mesh -> vrtz[vt1] - mesh -> vrtz[vt2]) / EDITFILE_SLOPE;
    zy = (mesh-> vrtz[vt2] + mesh -> vrtz[vt3] - mesh -> vrtz[vt0] - mesh -> vrtz[vt1]) / EDITFILE_SLOPE;

    /* x and y should be from -7 to 8   */
    if (zx < -7) zx = -7;
    if (zx > 8)  zx = 8;
    if (zy < -7) zy = -7;
    if (zy > 8)  zy = 8;

    /* Now between 0 and 15 */
    zx = zx + 7;
    zy = zy + 7;
    twist = (unsigned char)((zy << 4) + zx);

    return twist;

}

/*
 * Name:
 *     editfileMakeTwist
 * Description:
 *     Generates the number of the 'up'-vector for all fans
 * Input:
 *     mesh*: Pointer on mesh to set twists for
 */
static void editfileMakeTwist(MESH_T *mesh)
{

    int fan;


    fan = 0;
    while (fan < mesh -> numfan)
    {
        mesh -> twist[fan] = editfileGetFanTwist(mesh, fan);
        fan++;
    }

}

/* ========================================================================== */
/* ============================= THE PUBLIC ROUTINES ======================== */
/* ========================================================================== */

/*
 * Name:
 *     editfileLoadMapMesh
 * Description:
 *     Loads the map mesh into the data pointed on
 * Input:
 *     mesh*: Pointer on mesh to load
 *     msg *: Pointer on buffer for a message 
 * Output:
 *     Mesh could be loaded yes/no
 */
int editfileLoadMapMesh(MESH_T *mesh, char *msg)
{

    FILE* fileread;
    unsigned int uiTmp32;
    int cnt;
    float ftmp;
    int numfan;


    fileread = fopen("module/level.mpd", "rb");

    if (NULL == fileread)
    {
        sprintf(msg, "%s", "Map file not found.");
        return 0;
    }

    if (fileread)
    {

        fread( &uiTmp32, 4, 1, fileread );
        if ( uiTmp32 != MAPID ) 
        {
            sprintf(msg, "%s", "Map file has invalid Map-ID.");
            return 0;

        }

        fread(&mesh -> numvert, 4, 1, fileread);
        fread(&mesh -> tiles_x, 4, 1, fileread);
        fread(&mesh -> tiles_y, 4, 1, fileread);

        numfan = mesh -> tiles_x * mesh -> tiles_y;
        
        /* Load fan data    */
        fread(&mesh -> fan[0], sizeof(FANDATA_T), numfan, fileread);
        /* TODO: Check if enough space is available for loading map */
        /* Load normal data */
        fread(&mesh -> twist[0], 1, numfan, fileread);

        /* Load vertex x data   */
        for (cnt = 0; cnt < mesh -> numvert; cnt++) {
            /* TODO: Convert float to int for editing */
            fread(&ftmp, 4, 1, fileread);
            mesh -> vrtx[cnt] = ftmp;
        }

        /* Load vertex y data   */
        for (cnt = 0; cnt < mesh -> numvert; cnt++) {
            /* TODO: Convert float to int for editing */
            fread(&ftmp, 4, 1, fileread);
            mesh -> vrty[cnt] = ftmp;
        }

        /* Load vertex z data   */
        for (cnt = 0; cnt < mesh -> numvert; cnt++) {
            /* TODO: Convert float to int for editing */
            fread(&ftmp, 4, 1, fileread);
            mesh -> vrtz[cnt] = ftmp / 32;  /* Z is fixpoint int in cartman */
        }

        fread(&mesh -> vrta[0], 1, mesh -> numvert, fileread);   // !!!BAD!!!

        fclose(fileread);

        return 1;

    }

    return 0;

}

/*
 * Name:
 *     editfileSaveMapMesh
 * Description:
 *     Saves the mesh in the data pointed on.
 *     Name of the save file is always 'level.mpd'
 * Input:
 *     mesh* : Pointer on mesh to save
 * Output:
 *     Save worked yes/no
 */
int editfileSaveMapMesh(MESH_T *mesh, char *msg)
{

    FILE* filewrite;
    int itmp;
    float ftmp;
    int cnt;
    int numwritten;


    if (mesh -> map_loaded) {

        /* Generate the 'up'-vector-numbers */
        editfileMakeTwist(mesh);

        numwritten = 0;

        filewrite = fopen("module/level.mpd", "wb");
        if (filewrite) {

            itmp = MAPID;

            numwritten += fwrite(&itmp, 4, 1, filewrite);
            numwritten += fwrite(&mesh -> numvert, 4, 1, filewrite);
            numwritten += fwrite(&mesh -> tiles_x, 4, 1, filewrite);
            numwritten += fwrite(&mesh -> tiles_y, 4, 1, filewrite);

            /* Write tiles data    */
            numwritten += fwrite(&mesh -> fan[0], sizeof(FANDATA_T), mesh -> numfan, filewrite);
            /* Write twist data */
            numwritten += fwrite(&mesh -> twist[0], 1, mesh -> numfan, filewrite);

            /* Write x-vertices */
            for (cnt = 0; cnt < mesh -> numvert; cnt++) {
                /* Change int to floatfor game */
                ftmp = mesh -> vrtx[cnt];
                numwritten += fwrite(&ftmp, 4, 1, filewrite);

            }

            /* Write y-vertices */
            for (cnt = 0; cnt < mesh -> numvert; cnt++) {
                /* Change int to float for game */
                ftmp = mesh -> vrtz[cnt];
                numwritten += fwrite(&ftmp, 4, 1, filewrite);

            }

            /* Write z-vertices */
            for (cnt = 0; cnt < mesh -> numvert; cnt++) {
                /* Change int to float for game */
                ftmp = mesh -> vrtz[cnt];   /* Multiply with 64 again ? */
                numwritten += fwrite(&ftmp, 4, 1, filewrite);

            }

            return numwritten;

        }

    }

    sprintf(msg, "%s", "Saving map has failed.");

	return 0;

}
