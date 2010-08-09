/*******************************************************************************
*  EDITDRAW.H                                                                  *
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
#include "sdlgl.h"      /* OpenGL-Stuff                                 */
#include "sdlgl3d.h"    /* Helper routines for drawing of 3D-Screen     */    
#include "sdlgltex.h"   /* Texture handling                             */


#include "editdraw.h"   /* Own header                                   */   

/*******************************************************************************
* DEFINES								                                       *
*******************************************************************************/

#define EDITDRAW_MAXWALLSUBTEX  64     /* Images per wall texture   */
#define EDITDRAW_MAXWALLTEX      4     /* Maximum wall textures     */ 
  
/*******************************************************************************
* TYPEDEFS                                                                     *
*******************************************************************************/

/*******************************************************************************
* DATA                                                                         *
*******************************************************************************/

static COMMAND_T MeshCommand[MAXMESHTYPE] = {
    { /* 0:  Two Faced Ground... */
        4,		    /* Total number of vertices */
    	1,    		/*  1 Command */
    	{ 4 },		/* Commandsize (how much vertices per command)  */
    	{ 1, 2, 3, 0 },
    	{ 0.001, 0.001,  0.124, 0.001,  0.124, 0.124,  0.001, 0.124 },
        { 0.001, 0.001,  0.249, 0.001,  0.249, 0.249,  0.001, 0.249 }
    },
    { /* 1: Two Faced Ground... */
    	4,
        1,
        { 4 },
        { 0, 1, 2, 3 },
        { 0.001, 0.001,  0.124, 0.001,  0.124, 0.124,  0.001, 0.124 },
        { 0.001, 0.001,  0.249, 0.001,  0.249, 0.249,  0.001, 0.249 }
    },
    { /* 2: Four Faced Ground... */
        5,
        1,
        { 6 },
        { 4, 3, 0, 1, 2, 3 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,  0.50, 0.50 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,  0.50, 0.50 }
    },
    { /* 3:  Eight Faced Ground... */
        9,
        1,
	{ 10 },
        { 8, 3, 7, 0, 4, 1, 5, 2, 6, 3 }, /* Number of vertices */
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,  0.50, 0.00,
          1.00, 0.50,  0.50, 1.00,  0.00, 0.50,  0.50, 0.50 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,  0.50, 0.00,
          1.00, 0.50,  0.50, 1.00,  0.00, 0.50,  0.50, 0.50 }
    },
    {  /* 4:  Ten Face Pillar... */
        8,
        2,
        { 8, 6 },
        {  7, 3, 0, 4, 5, 6, 2, 3,
           5, 4, 0, 1, 2, 6 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
          0.33, 0.33,  0.66, 0.33,  0.66, 0.66,  0.33, 0.66 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
          0.33, 0.33,  0.66, 0.33,  0.66, 0.66,  0.33, 0.66 }
    },
    { /* 5  Eighteen Faced Pillar... */
  	16,
        4,
        { 10, 8, 4, 4 },
        { 15, 3, 10, 11, 12, 13, 14, 8, 9, 3,
          13, 12, 4, 5, 1, 6, 7, 14,
          12, 11, 0, 4,
          14, 7, 2, 8 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
          0.33, 0.00,  0.66, 0.00,  1.00, 0.33,  1.00, 0.66,
          0.66, 1.00,  0.33, 1.00,  0.00, 0.66,  0.00, 0.33,
          0.33, 0.33,  0.66, 0.33,  0.66, 0.66,  0.33, 0.66 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
          0.33, 0.00,  0.66, 0.00,  1.00, 0.33,  1.00, 0.66,
          0.66, 1.00,  0.33, 1.00,  0.00, 0.66,  0.00, 0.33,
          0.33, 0.33,  0.66, 0.33,  0.66, 0.66,  0.33, 0.66 }
    },
    { /* 6  Blank... */
      	0,
      	0
    },
    { /* 7  Blank... */
      	0,
      	0
    },
    { /* 8 Six Faced Wall (WE)... */
	8,
      	2,
      	{ 6, 4 },
      	{  5, 2, 3, 6, 7, 4,
           4, 7, 0, 1 },
      	{ 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
          1.00, 0.33,  1.00, 0.66,  0.00, 0.66,  0.00, 0.33 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
          1.00, 0.33,  1.00, 0.66,  0.00, 0.66,  0.00, 0.33 }

    },
    { /* 9  Six Faced Wall (NS)... */
	8,
      	2,
      	{ 6, 4 },
      	{ 7, 3, 0, 4, 5, 6,
          6, 5, 1, 2 },
      	{ 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
          0.33, 0.00,  0.66, 0.00,  0.66, 1.00,  0.33, 1.00 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
          0.33, 0.00,  0.66, 0.00,  0.66, 1.00,  0.33, 1.00 }
    },
    { /* 10  Blank... */
      	0,
      	0
    },
    { /* 11  Blank... */
    	0,
        0
    },
    { /* 12  Eight Faced Wall (W)... */
	8,
        2,
        { 8, 4 },
        { 7, 3, 4, 5, 6, 1, 2, 3,
 	  1, 6, 5, 0 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
          0.00, 0.66,  0.00, 0.33,  0.66, 0.33,  0.66, 0.66 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
          0.00, 0.66,  0.00, 0.33,  0.66, 0.33,  0.66, 0.66 }
    },
    {  /* 13  Eight Faced Wall (N)... */
    	8,
        2,
        { 8, 4 },
        { 7, 3, 0, 4, 5, 6, 2, 3,
          2, 6, 5, 1 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
          0.33, 0.00,  0.66, 0.00,  0.66, 0.66,  0.33, 0.66 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
          0.33, 0.00,  0.66, 0.00,  0.66, 0.66,  0.33, 0.66 }
    },
    {  /* 14  Eight Faced Wall (E)... */
       8,
       2,
       { 8, 4 },
       { 6, 3, 0, 1, 4, 5, 7, 3,
         3, 7, 5, 2 },
       { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
         1.00, 0.33,  1.00, 0.66,  0.33, 0.33,  0.33, 0.66 },
       { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
         1.00, 0.33,  1.00, 0.66,  0.33, 0.33,  0.33, 0.66 }
    },
    { /* 15  Eight Faced Wall (S)... */
    	8,
       	2,
       	{ 8, 4 },
       	{ 7, 5, 6, 0, 1, 2, 4, 5,
          0, 6, 5, 3 },
       	{ 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
          0.66, 1.00,  0.33, 1.00,  0.33, 0.33,  0.66, 0.33 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
          0.66, 1.00,  0.33, 1.00,  0.33, 0.33,  0.66, 0.33 }
    },
    { /* 16  Ten Faced Wall (WS)... */
    	10,
         2,
        { 8, 6 },
        { 9, 3, 6, 7, 8, 4, 5, 3,
          8, 7, 0, 1, 2, 4 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
	  0.66, 1.00,  0.33, 1.00,  0.00, 0.66,  0.00, 0.33,
          0.66, 0.33,  0.33, 0.66 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
	  0.66, 1.00,  0.33, 1.00,  0.00, 0.66,  0.00, 0.33,
          0.66, 0.33,  0.33, 0.66 }
    },
    { /* 17  Ten Faced Wall (NW)... */
	10,
        2,
        { 8, 6 },
        { 8, 6, 7, 0, 4, 5, 9, 6,
          9, 5, 1, 2, 3, 6 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
          0.33, 0.00,  0.66, 0.00,  0.00, 0.66,  0.00, 0.33,
          0.33, 0.33,  0.66, 0.66 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
          0.33, 0.00,  0.66, 0.00,  0.00, 0.66,  0.00, 0.33,
          0.33, 0.33,  0.66, 0.66 }
    },
    { /* 18  Ten Faced Wall (NE)... */
    	10,
        2,
        { 8, 6 },
        { 8, 9, 4, 5, 1, 6, 7, 9,
          9, 7, 2, 3, 0, 4 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
          0.33, 0.00,  0.66, 0.00,  1.00, 0.33,  1.00, 0.66,
          0.66, 0.33,  0.33, 0.66 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
          0.33, 0.00,  0.66, 0.00,  1.00, 0.33,  1.00, 0.66,
          0.66, 0.33,  0.33, 0.66 }
    },
    { /* 19  Ten Faced Wall (ES)... */
	10,
        2,
        { 8, 6 },
        { 9, 7, 8, 4, 5, 2, 6, 7,
          8, 7, 3, 0, 1, 4 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
          1.00, 0.33,  1.00, 0.66,  0.66, 1.00,  0.33, 1.00,
	  0.33, 0.33,  0.66, 0.66 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
          1.00, 0.33,  1.00, 0.66,  0.66, 1.00,  0.33, 1.00,
	  0.33, 0.33,  0.66, 0.66 }
    },
    { /* 20  Twelve Faced Wall (WSE)... */
        12,
        3,
        { 9, 5, 4 },
        { 11, 3, 8, 9, 4, 10, 6, 7, 3,
          10, 4, 5, 2, 6,
          4, 9, 0, 1 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
          1.00, 0.33,  1.00, 0.66,  0.66, 1.00,  0.33, 1.00,
          0.00, 0.66,  0.00, 0.33,  0.66, 0.66,  0.33, 0.66 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
          1.00, 0.33,  1.00, 0.66,  0.66, 1.00,  0.33, 1.00,
          0.00, 0.66,  0.00, 0.33,  0.66, 0.66,  0.33, 0.66 }
    },
    {  /* 21  Twelve Faced Wall (NWS)... */
	12,
        3,
        { 9, 5, 4 },
        { 10, 8, 9, 0, 4, 5, 6, 11, 8,
          11, 6, 7, 3, 8,
          6, 5, 1, 2 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
	  0.33, 0.00,  0.66, 0.00,  0.66, 1.00,  0.33, 1.00,
	  0.00, 0.66,  0.00, 0.33,  0.33, 0.33,  0.33, 0.66 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
	  0.33, 0.00,  0.66, 0.00,  0.66, 1.00,  0.33, 1.00,
	  0.00, 0.66,  0.00, 0.33,  0.33, 0.33,  0.33, 0.66 }
    },
    { /* 22  Twelve Faced Wall (ENW)... */
	12,
         3,
         { 9, 5, 4 },
         { 11, 8, 10, 4, 5, 1, 6, 7, 8,
           10, 8, 9, 0, 4,
           8, 7, 2, 3 },
         { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
           0.33, 0.00,  0.66, 0.00,  1.00, 0.33,  1.00, 0.66,
           0.00, 0.66,  0.00, 0.33,  0.33, 0.33,  0.66, 0.33 },
         { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
	  0.33, 0.00,  0.66, 0.00,  0.66, 1.00,  0.33, 1.00,
	  0.00, 0.66,  0.00, 0.33,  0.33, 0.33,  0.33, 0.66 }
    },
    { /* 23  Twelve Faced Wall (SEN)... */
	12,
        3,
        { 9, 5, 4 },
        { 11, 9, 4, 10, 6, 7, 2, 8, 9,
          10, 4, 5, 1, 6,
          4, 9, 3, 0 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
          0.33, 0.00,  0.66, 0.00,  1.00, 0.33,  1.00, 0.66,
          0.66, 1.00,  0.33, 1.00,  0.66, 0.33,  0.66, 0.66 },
        { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
          0.33, 0.00,  0.66, 0.00,  1.00, 0.33,  1.00, 0.66,
          0.66, 1.00,  0.33, 1.00,  0.66, 0.33,  0.66, 0.66 }
    },
    { /* 24  Twelve Faced Stair (WE)... */
	14,
        3,
        { 6, 6, 6 },
        { 13, 3, 0, 4, 5, 12,
          11, 12, 5, 6, 7, 10,
          9, 10, 7, 8, 1, 2 },
          { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
            0.16, 0.00,  0.33, 0.00,  0.50, 0.00,  0.66, 0.00,
            0.83, 0.00,  0.83, 1.00,  0.66, 1.00,  0.50, 1.00,
            0.33, 1.00,  0.16, 1.00 },
          { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
            0.16, 0.00,  0.33, 0.00,  0.50, 0.00,  0.66, 0.00,
            0.83, 0.00,  0.83, 1.00,  0.66, 1.00,  0.50, 1.00,
            0.33, 1.00,  0.16, 1.00 }
    },
    { /* 25  Twelve Faced Stair (NS)... */
    	14,
        3,
        { 6, 6, 6 },
        { 13, 0, 1, 4, 5, 12,
          11, 12, 5, 6, 7, 10,
          9, 10, 7, 8, 2, 3 },
          { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
            1.00, 0.16,  1.00, 0.33,  1.00, 0.50,  1.00, 0.66,
            1.00, 0.83,  0.00, 0.83,  0.00, 0.66,  0.00, 0.50,
            0.00, 0.33,  0.00, 0.16 },
          { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00,
            1.00, 0.16,  1.00, 0.33,  1.00, 0.50,  1.00, 0.66,
            1.00, 0.83,  0.00, 0.83,  0.00, 0.66,  0.00, 0.50,
            0.00, 0.33,  0.00, 0.16 }
    }
};

static float  MeshTileOffUV[EDITDRAW_MAXWALLSUBTEX * 2];
static GLuint WallTex[EDITDRAW_MAXWALLTEX];

/*******************************************************************************
* CODE 								                                           *
*******************************************************************************/

/*
 *  Name:
 *	    editdrawSingleFan
 *  Description:
 *	    Draws a single fan with given number from given mesh
 * Input:
 *      mesh *: Pointer on mesh info
 *      fan_no: Number of fan to draw
 */
static void editdrawSingleFan(MESH_T *mesh, int fan_no)
{

    COMMAND_T *mc;
    int *vert_x, *vert_y, *vert_z;
    int cnt, tnc, entry, *vertexno;
    int vert_base;
    char type;
    int actvertex;
    float *uv;			/* Pointer on Texture - coordinates	        */
    float *offuv;		/* Pointer on additional offset for start   */
                        /* of image in texture			            */
    unsigned char color[3];


    type = (char)(mesh -> fan[fan_no].type & 0x1F);  /* Maximum 31 fan types */
                                                     /* Others are flags     */

    mc   = &MeshCommand[type];

    vert_base = mesh -> vrtstart[fan_no];

    vert_x = &mesh -> vrtx[vert_base];
    vert_y = &mesh -> vrty[vert_base];
    vert_z = &mesh -> vrtz[vert_base];

    if (mesh -> draw_mode & EDIT_MODE_SOLID) {
        /* Draw solid */
        sdlglSetColor(SDLGL_COL_WHITE);
    }
    else {

        if (type & COMMAND_TEXTUREHI_FLAG) {/* It's one with hi res texture */
            sdlglSetColor(SDLGL_COL_BLUE);  /* color like cartman           */
    	}
    	else {
            sdlglSetColor(SDLGL_COL_RED);
        }

    }
    if (mesh -> draw_mode & EDIT_MODE_TEXTURED ) {

        if (type & COMMAND_TEXTUREHI_FLAG) {	/* It's one with hi res texture */
            uv = &mc -> biguv[0];
    	}
    	else {
            uv = &mc -> uv[0];
        }

    }

    offuv = &MeshTileOffUV[(mesh -> fan[fan_no].tx_no & 0x3F) * 2];

    /* Now bind the texture */
    if (mesh -> draw_mode & EDIT_MODE_TEXTURED) {

        glBindTexture(GL_TEXTURE_2D, WallTex[((mesh -> fan[fan_no].tx_no >> 6) & 3)]);

    }

    entry    = 0;
    vertexno = mc -> vertexno;

    for (cnt = 0; cnt < mc -> count; cnt++) {

        glBegin (GL_TRIANGLE_FAN);

            for (tnc = 0; tnc < mc -> size[cnt]; tnc++) {

                actvertex = vertexno[entry]; 	/* Number of vertex to draw */

                if (mesh -> draw_mode & EDIT_MODE_SOLID) {

                    if (mesh -> vrta[actvertex] > 60) {
                        color[0] = color[1] = color[2] = mesh -> vrta[actvertex];

                    }
                    else {
                        color[0] = color[1] = color[2] = 60;
                    }

                    if (mesh -> draw_mode & EDIT_MODE_LIGHTMAX) {
                        color[0] = color[1] = color[2] = 255;
                    }
                    glColor3ubv(&color[0]);		/* Set the light color */

                }
                if (mesh -> draw_mode & EDIT_MODE_TEXTURED) {
                    glTexCoord2f(uv[(actvertex * 2) + 0] + offuv[0], uv[(actvertex * 2) + 1] + offuv[1]);
                }
                
                glVertex3i(vert_x[actvertex], vert_y[actvertex], vert_z[actvertex]);

                entry++;

            }

        glEnd();

    } 

}

/*
 *  Name:
 *	    editdrawMap
 *  Description:
 *	    Draws a single fan with given number from given mesh
 * Input:
 *      mesh *:     Pointer on mesh to draw
 *      chosen_fan: Sign this fan as chosen in 3D-Map
 */
static void editdrawMap(MESH_T *mesh, int chosen_fan)
{

    int fan_no;


    if (! mesh -> map_loaded) {

        return;

    }

    glPolygonMode(GL_FRONT, mesh -> draw_mode & EDIT_MODE_SOLID ? GL_FILL : GL_LINE);
    if (mesh -> draw_mode & EDIT_MODE_TEXTURED) {
        glEnable(GL_TEXTURE_2D);
    }
    /* TODO: Draw the map, using different edit flags           */
    /* Needs list of visible tiles
       ( which must be built every time after the camera moved) */
    for (fan_no = 0; fan_no < mesh -> numfan;  fan_no++) {

        editdrawSingleFan(mesh, fan_no);

        if (chosen_fan == fan_no) {
            /* TODO: Sign fan, if it's chosen */
        }

    }
    
    if (mesh -> draw_mode & EDIT_MODE_TEXTURED) {
        glDisable(GL_TEXTURE_2D);
    }

}

/* ========================================================================== */
/* ============================= THE PUBLIC ROUTINES ======================== */
/* ========================================================================== */

/*
 * Name:
 *     editdrawInitData
 * Description:
 *     Initializes the command data needed for drawing
 * Input:
 *     None
 * Output:
 *     Pointer on command list
 */
COMMAND_T *editdrawInitData(void)
{

    char texturename[128];
    COMMAND_T *mcmd;
    int entry, cnt;


    mcmd = &MeshCommand[2];

    /* Correct all of them silly texture positions for seamless tiling */
    for (entry = 2; entry < (MAXMESHTYPE/2); entry++) {

        for (cnt = 0; cnt < (mcmd -> numvertices * 2); cnt += 2) {

            mcmd -> uv[cnt]        = ((.6/32)+(mcmd -> uv[cnt]*30.8/32))/8;
            mcmd -> uv[cnt + 1]    = ((.6/32)+(mcmd -> uv[cnt + 1]*30.8/32))/8;
            /* Do for big tiles too */
            mcmd -> biguv[cnt]	   = ((.6/64)+(mcmd -> biguv[cnt]*62.8/64))/4;
            mcmd -> biguv[cnt + 1] = ((.6/64)+(mcmd -> biguv[cnt + 1]*62.8/64))/4;

        }

        mcmd++;

    }

    // Make tile texture offsets
    for (entry = 0; entry < (EDITDRAW_MAXWALLSUBTEX * 2); entry += 2) {

        // Make tile texture offsets
        MeshTileOffUV[entry]     = (((entry / 2) & 7) * 0.125) + 0.001;
        MeshTileOffUV[entry + 1] = (((entry / 2) / 8) * 0.125) + 0.001;

    }

    /* TODO: Load the basic textures */
    for (cnt = 0; cnt < 4; cnt++) {
    
        sprintf(texturename, "module/tile%d.bmp", cnt);

        WallTex[cnt] = sdlgltexLoadSingle(texturename);
        
    }

    return MeshCommand;

}

/*
 * Name:
 *     editdrawFreeData
 * Description:
 *     Freses all data loaded by initialization code
 * Input:
 *     None
 */
void editdrawFreeData(void)
{
    
    /* Free the basic textures */
    glDeleteTextures(4, &WallTex[0]);

}

/*
 * Name:
 *     editdraw3DView
 * Description:
 *     Draws the whole 3D-View  
 * Input:
 *      mesh *:     Pointer on mesh to draw
 *      chosen_fan: Sign this fan as chosen in 3D-Map
 */
void editdraw3DView(MESH_T *mesh, int chosen_fan)
{

    int w, h;
    int x, y, x2, y2;


    sdlgl3dBegin(0);        /* Draw not solid */

    /* Draw a grid 64 x 64 squares for test of the camera view */
    sdlglSetColor(SDLGL_COL_CYAN);

    for (h = 0; h < 64; h++) {

        for (w = 0; w < 64; w++) {

            x = w * 128;
            y = h * 128;
            x2 = x + 128;
            y2 = y + 128;
            glBegin(GL_TRIANGLE_FAN);  /* Draw clockwise */
                glVertex2i(x, y);
                glVertex2i(x2, y);
                glVertex2i(x2, y2);
                glVertex2i(x, y2);
            glEnd();

        }

    }

    editdrawMap(mesh, chosen_fan);

    sdlgl3dEnd();

}

/*
 * Name:
 *     editdraw2DMap
 * Description:
 *     Draws the map as 2D-View into given rectangle
 * Input:
 *     mesh *:     Pointer on mesh to draw
 *     x, y, w, h: Draw into this rectangle
 *     chosen_fan: This fan is chosen by user
 */
void editdraw2DMap(MESH_T *mesh, int x, int y, int w, int h, int chosen_fan)
{

    static int mapcolors[] = { SDLGL_COL_BLUE, SDLGL_COL_BLUE, SDLGL_COL_BLUE, SDLGL_COL_BLUE, /* Ground */
                               1, 1,	  /* Pillar */
                               2, 2,	  /* ------ */
                               SDLGL_COL_BLACK, SDLGL_COL_BLACK,	  /* Wall   */
                               2, 2,	  /* ------ */
                               3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
                               4, 4,	  /* Stair  */
                               5, 5, 5, 5, 5, 5 };   /* Unknown */	 
    SDLGL_RECT draw_rect;
    int rx2, ry2;  
    int fan_no;


    if (! mesh -> map_loaded) {

        return;         /* Draw only if a mpa is loaded */

    }

    draw_rect.x = x;
    draw_rect.y = y;
    draw_rect.w = w / mesh -> tiles_x;   /* With generates size of square */
    draw_rect.h = draw_rect.w;
    
    fan_no = 0;
    glBegin(GL_QUADS);
        for (h = 0; h < mesh -> tiles_y; h++) {

            for (w = 0; w < mesh -> tiles_x; w++) {
                            
                sdlglSetColor(mapcolors[mesh -> fan[fan_no].type & 0x1F]);
                
                rx2 = draw_rect.x + draw_rect.w;
                ry2 = draw_rect.y + draw_rect.h;
                
                glVertex2i(draw_rect.x,  ry2);
                glVertex2i(rx2, ry2);
                glVertex2i(rx2, draw_rect.y);
            	glVertex2i(draw_rect.x,  draw_rect.y);
            
                draw_rect.x += draw_rect.w;
                fan_no++;
        
            }
            
            /* Next line    */
            draw_rect.x = x;
            draw_rect.y += draw_rect.h;
        
        }
    
    glEnd();

    /* TODO: Draw chosen tile                   */
    if (chosen_fan >= 0) {
        
        draw_rect.x = (chosen_fan % mesh -> tiles_x) * draw_rect.w;
        draw_rect.y = (chosen_fan / mesh -> tiles_x) * draw_rect.h;
        
        rx2 = draw_rect.x + draw_rect.w;
        ry2 = draw_rect.y + draw_rect.h;
                
        sdlglSetColor(SDLGL_COL_YELLOW);
        glBegin(GL_LINE_LOOP);
            glVertex2i(draw_rect.x,  ry2);
            glVertex2i(rx2, ry2);
            glVertex2i(rx2, draw_rect.y);
            glVertex2i(draw_rect.x,  draw_rect.y);
        glEnd(); 

    }
    
    /* TODO: Draw the Camera-Frustum over map   */


}