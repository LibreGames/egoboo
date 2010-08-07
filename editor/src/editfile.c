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

#define MAPID     0x4470614d        /*  The string... MapD  */

/*******************************************************************************
* CODE 								                                           *
*******************************************************************************/

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
 * Output:
 *     Mesh could be loaded yes/no
 */
int editfileLoadMapMesh(MESH_T *mesh)
{
    
    FILE* fileread;
    unsigned int uiTmp32;
    int cnt;
    float ftmp;
    int fan;


    fileread = fopen("level.mpd", "rb");

    if (NULL == fileread)
    {
        return 0;
    }

    if (fileread)
    {
        /* free_vertices(); */

        fread( &uiTmp32, 4, 1, fileread );  
        if ( uiTmp32 != MAPID ) {
        
            return 0;
            
        }
        
        fread( &mesh -> numvert, 4, 1, fileread ); 
        fread( &mesh -> tiles_x, 4, 1, fileread );  
        fread( &mesh -> tiles_y, 4, 1, fileread );  
        
        // Load fan data
        fan = 0;
        while (fan < mesh -> numfan) {

            fread( &uiTmp32, 4, 1, fileread );

            mesh -> fantype[fan] = (unsigned char) ((uiTmp32 >> 24) & 0x00FF);
            mesh -> fx[fan]      = (unsigned char) ((uiTmp32 >> 16) & 0x00FF);
            mesh -> tx_bits[fan] = (unsigned short)((uiTmp32 >>  0) & 0xFFFF);

            fan++;
        }

        // Load normal data
        // Load fan data
        fread(&mesh -> twist[0], 1, mesh -> numfan, fileread);

        // Load vertex x data
        cnt = 0;
        while (cnt < mesh -> numvert) {

            fread(&ftmp, 4, 1, fileread);
            mesh -> vrtx[cnt] = ftmp;
            cnt++;
        }

        // Load vertex y data
        cnt = 0;
        while (cnt < mesh -> numvert) {

            fread(&ftmp, 4, 1, fileread);
            mesh -> vrty[cnt] = ftmp;
            cnt++;
        }

        // Load vertex z data
        cnt = 0;
        while (cnt < mesh -> numvert) {

            fread(&ftmp, 4, 1, fileread);
            mesh -> vrtz[cnt] = ftmp;
            cnt++;
        }

        // Load vertex a data
        fread(&mesh -> vrta[0], 1, mesh -> numvert, fileread);   // !!!BAD!!!

        return 1;

    }

    return 0;

}

/*
 * Name:
 *     editfileLoadMapMesh
 * Description:
 *     Saves the mesh in the data pointed on.
 *     Name of the save file is always 'level.mpd' 
 * Input:
 *     mesh* : Pointer on mesh to save 
 */
int editfileSaveMapMesh(MESH_T *mesh)
{
    /* TODO: Save map to file */
    if (mesh -> map_loaded) {

        /* TODO: Save the mesh */

    }

	return 0;

}
