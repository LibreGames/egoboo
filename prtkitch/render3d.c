/*******************************************************************************
*  RENDER3D.C                                                                  *
*    - EGOBOO-Editor                                                           *     
*                                                                              *
*    - [...]                                                                   *
*      (c) 2013 Paul Mueller <muellerp61@bluewin.ch>                           *
*                                                                              *
*  This program is free software; you can redistribute it and/or modify        *
*  it under the terms of the GNU General Public License as published by        *
*  the Free Software Foundation; either version 2 of the License, or           *
*  (at your option) any later version.                                         *
*                                                                              *
*  This program is distributed in the hope that it will be useful,             *
*  but WITHOUT ANY WARRANTY; without even the implied warranty of              *
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               *
*  GNU Library General Public License for more details.                        *
*                                                                              *
*  You should have received a copy of the GNU General Public License           *
*  along with this program; if not, write to the Free Software                 *
*  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  *
*******************************************************************************/


/*******************************************************************************
* INCLUDES								                                   *
*******************************************************************************/

#include "sdlgl.h"      // OpenGL-Stuff
#include "sdlgltex.h"  // Load textures form file
#include "sdlgl3d.h"

#include "egodefs.h"    // Definition of object-types

/*******************************************************************************
* TYPEDEFS								                                   *
*******************************************************************************/

/*******************************************************************************
* DATA									                                   *
*******************************************************************************/

static GLuint PartTex;      // Texture for particles

/*******************************************************************************
* CODE									                                   *
*******************************************************************************/

/*
 * Name:
 *     render3DParticle
 * Description:
 *     Renders a particle in 'Billboard Mode'. Draws the bounding box
 * Input:
 *     pobj *: Pointer on object to render  
 */
void render3DParticle(SDLGL3D_OBJECT *pobj)
{
    // Basix extent of a particle before resize, counter-clockwisw
    static float pextent[8] = 
        { -8.0, -8.0, -8.0, +8.0, +8.0, +8.0, +8.0, -8.0 };
    
    int color;
    float modelview[16];
    int i,j;

    
    // save the current modelview matrix
    glPushMatrix();
        // Move the particle from origin to map position
        glTranslatef(pobj -> pos[0], pobj -> pos[1], pobj -> pos[2]);        
        // get the current modelview matrix
        glGetFloatv(GL_MODELVIEW_MATRIX , modelview);
        
        // undo all rotations, beware all scaling is lost as well 
        for(i = 0; i < 3; i++)
        {    
        	for(j = 0; j < 3; j++ )
            {
        		if(i==j)
                {
        			modelview[i*4+j] = 1.0;
                }
        		else
                {
        			modelview[i*4+j] = 0.0;
                }
        	}
        }

        // set the modelview with no rotations and scaling
        glLoadMatrixf(modelview);

        // Set the color to yellow, if its an emitter
        color = (pobj->type_no > 0) ? (pobj->type_no & 0x0F) : SDLGL_COL_YELLOW;
        if(0 == color)
        {
            color = 1;
        }

        // Now draw the cube of the bounding box
        glPolygonMode(GL_FRONT, GL_LINE);
        sdlglSetColor(color);
        glBegin(GL_LINE_LOOP);
            // Draw top of cube
            glVertex3f(pobj -> bbox[0][0], pobj -> bbox[0][1], pobj -> bbox[0][2]); // x, y, z
            glVertex3f(pobj -> bbox[1][0], pobj -> bbox[0][1], pobj -> bbox[0][2]); // x, y, z
            glVertex3f(pobj -> bbox[1][0], pobj -> bbox[1][1], pobj -> bbox[0][2]); // x, y, z
            glVertex3f(pobj -> bbox[0][0], pobj -> bbox[1][1], pobj -> bbox[0][2]); // x, y, z
        glEnd();
        glBegin(GL_LINE_LOOP);
            // Draw bottom of cube
            glVertex3f(pobj -> bbox[0][0], pobj -> bbox[0][1], pobj -> bbox[1][2]);
            glVertex3f(pobj -> bbox[1][0], pobj -> bbox[0][1], pobj -> bbox[1][2]);
            glVertex3f(pobj -> bbox[1][0], pobj -> bbox[1][1], pobj -> bbox[1][2]);
            glVertex3f(pobj -> bbox[0][0], pobj -> bbox[1][1], pobj -> bbox[1][2]);
        glEnd();
        // Draw edges of cube
        glBegin(GL_LINES);
            // Edge one
            glVertex3f(pobj -> bbox[0][0], pobj -> bbox[0][1], pobj -> bbox[0][2]); /* x, y, z */    
            glVertex3f(pobj -> bbox[0][0], pobj -> bbox[0][1], pobj -> bbox[1][2]); /* x, y, z */    
            // Edge two
            glVertex3f(pobj -> bbox[1][0], pobj -> bbox[0][1], pobj -> bbox[0][2]); /* x, y, z */ 
            glVertex3f(pobj -> bbox[1][0], pobj -> bbox[0][1], pobj -> bbox[1][2]); /* x, y, z */ 
            // Edge three
            glVertex3f(pobj -> bbox[1][0], pobj -> bbox[1][1], pobj -> bbox[0][2]); /* x, y, z */  
            glVertex3f(pobj -> bbox[1][0], pobj -> bbox[1][1], pobj -> bbox[1][2]); /* x, y, z */  
            // Edge four
            glVertex3f(pobj -> bbox[0][0], pobj -> bbox[1][1], pobj -> bbox[0][2]); /* x, y, z */   
            glVertex3f(pobj -> bbox[0][0], pobj -> bbox[1][1], pobj -> bbox[1][2]); /* x, y, z */   
        glEnd();
        // @todo: Attach texture to particle, clockwise
        glPolygonMode(GL_FRONT, GL_FILL);
        glColor3f(1.0, 1.0, 1.0);   // Set white color
        glBegin(GL_TRIANGLE_FAN);
            glVertex2fv(&pextent[0]);
            glVertex2fv(&pextent[6]);
            glVertex2fv(&pextent[4]);
            glVertex2fv(&pextent[2]);
        glEnd();
    // restores the modelview matrix       
    glPopMatrix();
}    

/* ========================================================================== */
/* ============================= PUBLIC FUNCTION(S) ========================= */
/* ========================================================================== */

/*
 * Name:
 *     render3dInit
 * Description:
 *     Loads all data needed to display the 3D-Part
 * Input:
 *     None 
 */
void render3dInit(void)
{
    PartTex = sdlgltexLoadSingleA("data/particle.bmp", 0);
}

/*
 * Name:
 *     render3dCleanup
 * Description:
 *     Releases all data needed for display of the 3D-Part
 * Input:
 *     None  
 */
void render3dCleanup(void)
{
    // Free the texture for 
    glDeleteTextures(1, &PartTex);
}


/* ==== Main function ==== */

/*
 * Name:
 *     render3dMain
 * Description:
 *     Renders the 3D-View
 * Input:
 *     None 
 */
void render3dMain(void)
{
    SDLGL3D_OBJECT *obj_list;
    

    sdlgl3dBegin(0, 0);        /* Draw not solid */

    // Draw a white rectangle with center at 200, 200 (Emitter is at 200, 200, 100)
    // This is the 'bottom' of the view
    sdlglSetColor(SDLGL_COL_WHITE);
    glBegin(GL_LINE_LOOP);        
        glVertex2f(100, 100);
        glVertex2f(300, 100);
        glVertex2f(300, 300);
        glVertex2f(100, 300);
    glEnd();

    // Get the pointer on the list of objects    
    obj_list = sdlgl3dGetObject(0);
    
    while(obj_list->id > 0)
    {
        if(obj_list->obj_type == EGOMAP_OBJ_PART)
        {
            render3DParticle(obj_list);
        }

        obj_list++;
    }   
    
    // End 3D-Mode
    sdlgl3dEnd();
}