/*******************************************************************************
*  RENDER3D.C                                                                  *
*    - EGOBOO-Editor                                                           *
*                                                                              *
*    - All functionality to render the 3D-Part of the game                     *
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
#include "egofile.h"    // Get the path for the textures

// Own header
#include "render3d.h"

/*******************************************************************************
* TYPEDEFS                                                                     *
*******************************************************************************/

/*******************************************************************************
* DEFINES                                                                      *
*******************************************************************************/

#define RENDER3D_MAXWALLSUBTEX   64     /* Images per wall texture   */
#define RENDER3D_MAXWALLTEX       4     /* Maximum wall textures     */ 
#define RENDER3D_TILEDIV        128

/*******************************************************************************
* DATA                                                                         *
*******************************************************************************/

static GLuint PartTex;                          // Texture for particles
static GLuint WallTex[RENDER3D_MAXWALLTEX];     // Texture for tiles

static float  DefaultUV[] = { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00 };
static float  MeshTileOffUV[RENDER3D_MAXWALLSUBTEX * 2];    /* Offset into wall texture */
static GLuint WallTex[RENDER3D_MAXWALLTEX];

/*******************************************************************************
* CODE                                                                         *
*******************************************************************************/

/*
 *  Name:
 *	    render3dSingleFan
 *  Description:
 *	    Draws a single fan with given number from given mesh
 * Input:
 *      mesh *: Pointer on mesh info
 *      fan_no: Number of fan to draw
 */
static void render3dSingleFan(MESH_T *mesh, int fan_no)
{
    COMMAND_T *mc;
    MESH_VTX_T *vert_3d;
    int cnt, tnc, entry, *vertexno;
    int vert_base;
    char type, bigtex;
    int actvertex;
    float *uv;			/* Pointer on Texture - coordinates	        */
    float *offuv;		/* Pointer on additional offset for start   */
                        /* of image in texture			            */
    unsigned char color[3];


    type   = (char)mesh -> fan[fan_no].type;
    bigtex = (char)(type & 0x20);
    type   &= 0x1F;     /* Maximum 31 fan types for drawing         */

    
    mc   = &mesh -> pcmd[type];

    vert_base = mesh -> vrtstart[fan_no];
    vert_3d   = &mesh -> vrt[vert_base];

    // Allways draw textured
    
    if (bigtex)
    {               /* It's one with hi res texture */
        uv = &mc -> biguv[0];
    }
    else
    {
        uv = &mc -> uv[0];
    }

    offuv = &MeshTileOffUV[(mesh -> fan[fan_no].tx_no & 0x3F) * 2];

    /* Now bind the texture */
    glBindTexture(GL_TEXTURE_2D, WallTex[((mesh -> fan[fan_no].tx_no >> 6) & 3)]);

    entry    = 0;
    vertexno = mc -> vertexno;

    for (cnt = 0; cnt < mc -> count; cnt++)
    {
        glBegin (GL_TRIANGLE_FAN);

            for (tnc = 0; tnc < mc -> size[cnt]; tnc++)
            {
                actvertex = vertexno[entry]; 	/* Number of vertex to draw */

                if (mesh -> draw_mode & RENDER3D_LIGHTMAX || mesh -> vrt[actvertex].a == 0)
                {
                    color[0] = color[1] = color[2] = 255;
                }
                else
                {
                    /* Show ambient lighting */
                    color[0] = color[1] = color[2] = mesh -> vrt[actvertex].a;
                }
                // Set the light color     
                glColor3ubv(&color[0]);
                // Draw the vertex
                glTexCoord2f(uv[(actvertex * 2) + 0] + offuv[0], uv[(actvertex * 2) + 1] + offuv[1]);
                glVertex3fv(&vert_3d[actvertex].x);

                entry++;
            }

        glEnd();
    } 
}

/*
 * Name:
 *     render3dBoundingBox
 * Description:
 *     Renders a the boundig box for an object
 * Input:
 *     pobj *: Pointer on object to render bounding box for 
 */
static void render3dBoundingBox(SDLGL3D_OBJECT *pobj)
{
    int color;
    
    
    if(pobj->obj_type == EGOMAP_OBJ_CHAR)
    {
        color = SDLGL_COL_WHITE;
    }
    else if(pobj->obj_type == EGOMAP_OBJ_PART)
    {
        color = SDLGL_COL_YELLOW;
    }
    else
    {
        color = SDLGL_COL_GREEN;
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
}

/*
 * Name:
 *     render3dParticle
 * Description:
 *     Renders a particle in 'Billboard Mode'. Draws the bounding box
 * Input:
 *     pobj *: Pointer on object to render  
 */
static void render3dParticle(SDLGL3D_OBJECT *pobj)
{
    // Basix extent of a particle before resize, counter-clockwisw
    static float pextent[8] = 
        { -8.0, -8.0, -8.0, +8.0, +8.0, +8.0, +8.0, -8.0 };
    
    float uv[8];
    float modelview[16];
    int i,j;
    int x, y;
    
    // Get the UV-Indices into particle texture, Sub-Textures 16 by 16: 256 Textures
    x = pobj->cur_frame % 16;
    y = pobj->cur_frame / 16;
    // Fill in the UV-Values
    uv[0] = (float)x * 0.0625; 
    uv[1] = (float)y * 0.0625; 
    uv[2] = uv[0] + 0.0625; 
    uv[3] = uv[1]; 
    uv[4] = uv[0] + 0.0625; 
    uv[5] = uv[1] + 0.0625; 
    uv[6] = uv[0]; 
    uv[7] = uv[1] + 0.0625; 
    
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
        
        // Attach texture to particle, clockwise
        glPolygonMode(GL_FRONT, GL_FILL);
        glColor3f(1.0, 1.0, 1.0);   // Set white color
        glBegin(GL_TRIANGLE_FAN);
            glTexCoord2fv(&uv[4]);
            glVertex2fv(&pextent[0]);
            glTexCoord2fv(&uv[6]);
            glVertex2fv(&pextent[6]);
            glTexCoord2fv(&uv[0]);
            glVertex2fv(&pextent[4]);
            glTexCoord2fv(&uv[2]);
            glVertex2fv(&pextent[2]);
        glEnd();
        
        // Draw the bounding box
        render3dBoundingBox(pobj);
    // restores the modelview matrix       
    glPopMatrix();
}

/*
 *  Name:
 *	   render3dMap
 *  Description:
 *	   Draws the whole map. Doesn't support any culling code.
 * Input:
 *     pmesh *:  Pointer on mesh to draw
 */
static void render3dMap(MESH_T *pmesh)
{
    SDLGL3D_FRUSTUM f;          /* Information about the frustum        */
    int fan_no, x, y;
    int tx1, ty1, tx2, ty2;


    glPolygonMode(GL_FRONT, GL_FILL);
    glFrontFace(GL_CW);


    // Get camera for drawing edges
    // Calculate edges in tiles
    tx1 = f.bf.min_x / EGOMAP_TILE_SIZE;
    if(tx1 < 0) tx1 = 0;
    ty1 = f.bf.min_y / EGOMAP_TILE_SIZE;
    if(ty1 < 0) ty1 = 0;
    // Get bottom edges
    tx2 = (f.bf.max_x / EGOMAP_TILE_SIZE) + 1;
    if(tx2 >= pmesh->tiles_x) tx2 = pmesh->tiles_x;
    ty2 = (f.bf.max_y / EGOMAP_TILE_SIZE) + 1;
    if(ty2 >= pmesh->tiles_y) ty2 = pmesh->tiles_y;

    // Use draw edges from actual used camera
    /* Draw the map, using different edit flags           */
    /* Needs list of visible tiles
       ( which must be built every time after the camera moved) */
    /* TODO:
        Draw first bottom tiles (and transparent fan, if on same fan)
        Second draw walls (and transparent fan, if on same fan)
        After that draw ordered all tiles
            if (wall) draw wall
                ==> objects on same tile
                ==> particles on same tile
    */
    /* =============== First draw bottom tiles ============== */
    for(y = ty1; y < ty2; y++)
    {
        for(x = tx1; x < tx2; x++)
        {
            fan_no = (y * pmesh->tiles_x) + y;

            if (pmesh -> fan[fan_no].type == 0)
            {
                /* --- Draw the basic fan, solid --- */
                render3dSingleFan(pmesh, fan_no);
                /* --- Draw overlays, if needed --- */
                // render3dOverlay3D(mesh, fan_no, crect);
                // @todo: Draw object on tile
            }
        }
    }

    /* =============== Second draw wall tiles ============== */
    for(y = ty1; y < ty2; y++)
    {
        for(x = tx1; x < tx2; x++)
        {
            fan_no = (y * pmesh->tiles_x) + y;

            if (pmesh -> fan[fan_no].type != 0)
            {
                /* --- Draw the basic fan, solid --- */
                render3dSingleFan(pmesh, fan_no);
                /* --- Draw overlays, if needed --- */
                // render3dOverlay3D(mesh, fan_no, crect);
            }

            // Draw objects on bottom tiles
            /*
            if (mesh -> fan[fan_no].type == 0 && pmesh -> fan[fan_no].obj_no > 0)
            {
                render3dObject(pmesh -> fan[fan_no].obj_no);
            }
            */
        }
    }
}

/* ========================================================================== */
/* ============================= PUBLIC FUNCTION(S) ========================= */
/* ========================================================================== */

/*
 * Name:
 *     render3dLoad
 * Description:
 *     Load the textures needed for 3D-Display 
 * Input:
 *     globals: Load globals yes(no
 */
void render3dLoad(char globals)
{
    static char loaded_glob = 0;
    
    char texturename[128];
    char *fname;
    int entry, cnt;
    
    if(globals && !loaded_glob)
    {
        // Make tile texture start offsets
        for (entry = 0; entry < (RENDER3D_MAXWALLSUBTEX * 2); entry += 2)
        {
            MeshTileOffUV[entry]     = (((entry / 2) & 7) * 0.125);
            MeshTileOffUV[entry + 1] = (((entry / 2) / 8) * 0.125);
        }
        // Load texture for particles once
        PartTex = sdlgltexLoadSingleA("data/particle.bmp", 0);
    }
    
    /* Load the basic textures */
    for (cnt = 0; cnt < 4; cnt++)
    {
        sprintf(texturename, "tile%d.bmp", cnt);

        fname = egofileMakeFileName(EGOFILE_GAMEDATDIR, texturename);

        WallTex[cnt] = sdlgltexLoadSingle(fname);
    }
}

/*
 * Name:
 *     render3dCleanup
 * Description:
 *     Releases all data needed for display of the 3D-Part
 * Input:
 *     globals: Cleanup globals too yes/no  
 */
void render3dCleanup(char globals)
{
    // Free the texture for particles 
    
    if(globals)
    {
        glDeleteTextures(1, &PartTex);
    }
    
    glDeleteTextures(RENDER3D_MAXWALLTEX, &WallTex[0]);
}

/*
 * Name:
 *     render3dMain
 * Description:
 *     Renders the complete 3d-View (@todo: For multiple cameras)
 * Input:
 *     pmesh *: Pointer on map to render  
 */
void render3dMain(MESH_T *pmesh)
{
    SDLGL3D_OBJECT *pobj;


    // Start 3D-Mode
    sdlgl3dBegin(0, 0);        /* Draw solid */
    glEnable(GL_TEXTURE_2D);
    
    // Draw the map
    render3dMap(pmesh);   
    
    // Draw the objects
    // Get the pointer on the list of objects   
    pobj = sdlgl3dGetObject(0);
       
    
    while(pobj->id != 0)
    {
        if(pobj->obj_type == EGOMAP_OBJ_PART)
        {
            glBindTexture(GL_TEXTURE_2D, PartTex);
            // do not display the completely transparent portion
            glEnable(GL_ALPHA_TEST);            // ENABLE_BIT
            glAlphaFunc(GL_GREATER, 0.0f);      // GL_COLOR_BUFFER_BIT
            render3dParticle(pobj);
        }
        else if(pobj->obj_type == EGOMAP_OBJ_CHAR)
        {   
            glDisable(GL_ALPHA_TEST);            // ENABLE_BIT
            // @todo: Render the model
            render3dBoundingBox(pobj);
        }

        pobj++;
    }
    
    glDisable(GL_TEXTURE_2D);    
    
    // End 3D-Mode
    sdlgl3dEnd();
}