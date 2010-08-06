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

#ifndef _EDITDRAW_H_
#define _EDITDRAW_H_

/*******************************************************************************
* INCLUDES								                                       *
*******************************************************************************/

#include "sdlgl.h"      /* OpenGL-Stuff                                 */
#include "sdlgl3d.h"    /* Helper routines for drawing of 3D-Screen     */    
#include "editor.h"     /* Definition of map-mesh MESH_T and COMMAND_T  */

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
 *     editdraw3DView
 * Description:
 *     Draws the whole 3D-View  
 * Input:
 *     None 
 */
void editdraw3DView(void)
{

    int w, h;
    int x, y, x2, y2;


    sdlgl3dBegin(1);

    /* Draw a grid 64 x 64 squares for test of the camera view */
    glColor3f(1.0, 1.0, 1.0);

    for (h = 0; h < 64; h++) {

        for (w = 0; w < 64; w++) {

            x = w * 128;
            y = h * 128;
            x2 = x + 128;
            y2 = y + 128;
            glBegin(GL_LINE_LOOP);
                glVertex2i(x, y2);
                glVertex2i(x2, y2);
                glVertex2i(x2, y);
    	        glVertex2i(x, y);
            glEnd();

        }

    }

    sdlgl3dEnd();

}

#endif  /* _EDITDRAW_H_ */