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
#include <memory.h>


#include "sdlgl.h"      /* OpenGL-Stuff                                 */
#include "sdlgl3d.h"    /* Helper routines for drawing of 3D-Screen     */
#include "sdlgltex.h"   /* Texture handling                             */
#include "sdlglstr.h"
#include "editor.h"
#include "editfile.h"   /* Make the file name for reading textures      */
#include "egodefs.h"    /* MPDFX_...                                    */                              

#include "editdraw.h"   /* Own header                                   */   

/*******************************************************************************
* DEFINES								                                       *
*******************************************************************************/

#define EDITDRAW_MAXWALLSUBTEX   64     /* Images per wall texture   */
#define EDITDRAW_MAXWALLTEX       4     /* Maximum wall textures     */ 
#define EDITDRAW_TILEDIV        128

/*******************************************************************************
* TYPEDEFS                                                                     *
*******************************************************************************/

static void editdrawTransparentFan3D(MESH_T *mesh, int fan_no);

/*******************************************************************************
* DATA                                                                         *
*******************************************************************************/

static float  DefaultUV[] = { 0.00, 0.00,  1.00, 0.00,  1.00, 1.00,  0.00, 1.00 };
static float  MeshTileOffUV[EDITDRAW_MAXWALLSUBTEX * 2];    /* Offset into wall texture */
static GLuint WallTex[EDITDRAW_MAXWALLTEX];

/*******************************************************************************
* CODE 								                                           *
*******************************************************************************/

/*
 * Name:
 *     editdrawSetTransColor
 * Description:
 *     Sets the transparent color with given transparency.
 * Input:
 *     col_no: Set this color 
 *     trans:   Transparency-Value
 * Output:
 *     Has a color at all yes/no
 */
static void editdrawSetTransColor(int col_no, char trans)
{
    unsigned char color[5];   

    
    sdlglGetColor(col_no, color);
    color[3] = trans;

    glColor4ubv(color);
}

/*
 * Name:
 *     editdrawObject
 * Description:
 *     Draws an object with given object number.
 *     -For test purposes: Draws its bounding box
 * Input:
 *     obj_no: Number of object to draw 
 */
static void editdrawObject(int obj_no)
{
    int i, j;
    SDLGL3D_OBJECT *pobj;
    float bbox[2][3];

    
    if(obj_no > 0)
    {
        // If object is valid at all        
        pobj = sdlgl3dGetObject(obj_no);
        // @todo: Use OpenGL-Matrix for drawing
        // Translate bounding box to position of
        for(i = 0; i < 2; i++)
        {
            for(j = 0; j < 3; j++)
            {
                bbox[i][j] = pobj -> bbox[i][j] + pobj -> pos[j];
            }
        }
        // Now draw the cube of the bounding box
        sdlglSetColor(pobj -> type_no & 0x0F);  
        glBegin(GL_LINE_LOOP);
            // Draw top of cube
            glVertex3f(bbox[0][0], bbox[0][1], bbox[0][2]); // x, y, z
            glVertex3f(bbox[1][0], bbox[0][1], bbox[0][2]); // x, y, z
            glVertex3f(bbox[1][0], bbox[1][1], bbox[0][2]); // x, y, z
            glVertex3f(bbox[0][0], bbox[1][1], bbox[0][2]); // x, y, z
        glEnd();
        glBegin(GL_LINE_LOOP);
            // Draw bottom of cube
            glVertex3f(bbox[0][0], bbox[0][1], bbox[1][2]);
            glVertex3f(bbox[1][0], bbox[0][1], bbox[1][2]);
            glVertex3f(bbox[1][0], bbox[1][1], bbox[1][2]);
            glVertex3f(bbox[0][0], bbox[1][1], bbox[1][2]);
        glEnd();
        // Draw edges of cube
        glBegin(GL_LINES);
            // Edge one
            glVertex3f(bbox[0][0], bbox[0][1], bbox[0][2]); /* x, y, z */    
            glVertex3f(bbox[0][0], bbox[0][1], bbox[1][2]); /* x, y, z */    
            // Edge two
            glVertex3f(bbox[1][0], bbox[0][1], bbox[0][2]); /* x, y, z */ 
            glVertex3f(bbox[1][0], bbox[0][1], bbox[1][2]); /* x, y, z */ 
            // Edge three
            glVertex3f(bbox[1][0], bbox[1][1], bbox[0][2]); /* x, y, z */  
            glVertex3f(bbox[1][0], bbox[1][1], bbox[1][2]); /* x, y, z */  
            // Edge four
            glVertex3f(bbox[0][0], bbox[1][1], bbox[0][2]); /* x, y, z */   
            glVertex3f(bbox[0][0], bbox[1][1], bbox[1][2]); /* x, y, z */   
        glEnd();    
    }
}
 

/*
 * Name:
 *     editdrawTileIsChosen
 * Description:
 *     Checks if the given tile position is in the chosen rect
 * Input:
 *     ty, ty:  Position of tile
 *     crect *: Pointer on description of chosen rectangle
 * Output:
 *     Tile is chosen yes/no
 */
static int editdrawTileIsChosen(int tx, int ty, int *crect)
{
    /* Check if it's chosen */
    if (crect[0] > 0 && crect[1] > 0)
    {
        if (tx >= crect[0] && tx <= crect[2] && ty >= crect[1] && ty <= crect[3])
        {
            /* -- It's a tile in the chosen area of the input -- */
            return 1;
        }
    }  
    
    return 0;
 }

/*
 * Name:
 *     editdrawOverlay3D
 * Description:
 *     Draws transparent tile(s) with given rectangle
 *     Depending on info about fan  
 * Input:
 *     mesh *:  Info about mesh 
 *     fan_no:  For this fan 
 *     crect *: Edges of chosen rectngle, if any
 */
static void editdrawOverlay3D(MESH_T *mesh, int fan_no, int *crect)
{
    FANDATA_T *fd;  
    int tx, ty;    
    

    fd = &mesh -> fan[fan_no];    
    ty = fan_no / mesh -> tiles_x; 
    tx = fan_no % mesh -> tiles_x;
    
    if (fd -> psg_no > 0)
    {
        /* --- It's a passage --- */
        editdrawSetTransColor(SDLGL_COL_BLUE, 128);
        editdrawTransparentFan3D(mesh, fan_no);
    }
    
    if (fd -> obj_no > 0)
    {
        /* --- It's a spawn point --- */
        editdrawSetTransColor(SDLGL_COL_RED, 128);
        editdrawTransparentFan3D(mesh, fan_no);
        // @todo: Draw the objects bounding box reps. object itself */
    }

    if (editdrawTileIsChosen(tx, ty, crect))
    {
        /* -- It's a tile chosen by the input -- */
        editdrawSetTransColor(SDLGL_COL_YELLOW, 128);
        editdrawTransparentFan3D(mesh, fan_no);
    }    
    
}

/*
 *  Name:
 *	    editdrawChosenFanType
 *  Description:
 *      Draws the fan 'ChosenFanType' in wireframe-mode in white color.
 *      Elevated by 10 Units above Z=0
 * Input:
 *     ft *: Fantype holding possible texture number of this fan
 *     fd *: Data of fan to draw 
 */
static void editdrawChosenFanType(FANDATA_T *ft, COMMAND_T *fd)
{
    int cnt, tnc;
    int entry;
    
    
    /* FIXME: Save and restore state */
    glPolygonMode(GL_FRONT, GL_LINE);
    sdlglSetColor(SDLGL_COL_WHITE);

    if (ft -> tx_no > 0)
    {
        /* Empty code to keep compiler quiet: 2010-08-18 */
    }
    
    entry = 0;
    
    for (cnt = 0; cnt < fd -> count; cnt++)
    {
        glBegin (GL_TRIANGLE_FAN);

            for (tnc = 0; tnc < fd -> size[cnt]; tnc++)
            {
                glVertex3f(fd -> vtx[fd -> vertexno[entry]].x,
                           fd -> vtx[fd -> vertexno[entry]].y,
                           fd -> vtx[fd -> vertexno[entry]].z + 10);

                entry++;
            }

        glEnd();
    } 
}

/*
 *  Name:
 *	    editdrawOverlay2D
 *  Description:
 *      Draws transparent fans on minimap with different colors depending
 *      on the type of 'overlay' on map 
 * Input:
 *      mesh *:  Pointer on mesh info
 *      rect *:  Rectangle size of tile on minimap 
 *      crect *: Rectangle of chosen tiles
 */
static void editdrawOverlay2D(MESH_T *mesh, SDLGL_RECT *rect, int *crect)
{
    SDLGL_RECT draw_rect;
    FANDATA_T *fd;
    
    int fan_no, x, y;
    int rx2, ry2;


    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPolygonMode(GL_FRONT, GL_FILL);


    fan_no = 0;

    draw_rect.x = rect -> x;
    draw_rect.y = rect -> y;
    draw_rect.w = rect -> w;
    draw_rect.h = rect -> h;

    glBegin(GL_QUADS);
        for (y = 0; y < mesh -> tiles_y; y++)
        {
            for (x = 0; x < mesh -> tiles_x; x++)
            {
                rx2 = draw_rect.x + draw_rect.w;
                ry2 = draw_rect.y + draw_rect.h;
                
                fd = &mesh -> fan[fan_no];
                    
                if (fd -> psg_no > 0)
                {
                    /* --- It's a passage --- */
                    editdrawSetTransColor(SDLGL_COL_BLUE, 128);
                    /* -- Draw the passage --- */
                    glVertex2i(draw_rect.x,  ry2);
                    glVertex2i(rx2, ry2);
                    glVertex2i(rx2, draw_rect.y);
                    glVertex2i(draw_rect.x, draw_rect.y);
                }
                
                if (fd -> obj_no > 0)
                {
                    /* --- It's a spawn point/object --- */
                    editdrawSetTransColor(SDLGL_COL_RED, 128);
                    /* -- Draw the spawn point --- */
                    glVertex2i(draw_rect.x,  ry2);
                    glVertex2i(rx2, ry2);
                    glVertex2i(rx2, draw_rect.y);
                    glVertex2i(draw_rect.x, draw_rect.y);
                }

                /* Check if it's chosen */
                if (editdrawTileIsChosen(x, y, crect))
                {
                    /* -- It's a tile in the chosen area of the input -- */
                    editdrawSetTransColor(SDLGL_COL_YELLOW, 128);
                    /* -- Draw the chosen tile --- */
                    glVertex2i(draw_rect.x,  ry2);
                    glVertex2i(rx2, ry2);
                    glVertex2i(rx2, draw_rect.y);
                    glVertex2i(draw_rect.x, draw_rect.y);
                }                

                draw_rect.x += draw_rect.w;
                fan_no++;
            }
            /* Next line    */
            draw_rect.x = rect -> x;
            draw_rect.y += draw_rect.h;
    }
    glEnd();

    glDisable(GL_BLEND);
}

/*
 *  Name:
 *	    editdrawTransparentFan3D
 *  Description:
 *      Draws a single transparent fan
 *	    Draws a list of fans with given number on 3D-Mesh
 *      Transparent, with no texture
 * Input:
 *      mesh *:   Pointer on mesh info
 *      fan_no:   Number of fan to draw
 */
static void editdrawTransparentFan3D(MESH_T *mesh, int fan_no)
{
    COMMAND_T *mc;
    float *vert_x, *vert_y, *vert_z;
    int cnt, tnc, entry, *vertexno;
    int vert_base;
    char type;
    int actvertex;

    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);     
    glPolygonMode(GL_FRONT, GL_FILL);      
        
    type = (char)(mesh -> fan[fan_no].type & 0x1F);  /* Maximum 31 fan types */
                                                     /* Others are flags     */

    mc   = &mesh -> pcmd[type];

    vert_base = mesh -> vrtstart[fan_no];

    vert_x = &mesh -> vrtx[vert_base];
    vert_y = &mesh -> vrty[vert_base];
    vert_z = &mesh -> vrtz[vert_base];   

    entry    = 0;
    vertexno = mc -> vertexno;

    for (cnt = 0; cnt < mc -> count; cnt++)
    {
        glBegin (GL_TRIANGLE_FAN);

            for (tnc = 0; tnc < mc -> size[cnt]; tnc++)
            {
                actvertex = vertexno[entry]; 	/* Number of vertex to draw */

                glVertex3f(vert_x[actvertex], vert_y[actvertex], vert_z[actvertex] + 2);

                entry++;
            }

        glEnd();
    } 
    
    glDisable(GL_BLEND);
}

/*
 *  Name:
 *	    editdrawSingleFan
 *  Description:
 *	    Draws a single fan with given number from given mesh
 * Input:
 *      mesh *: Pointer on mesh info
 *      fan_no: Number of fan to draw
 *      col_no: Override for setting by texture-flag
 */
static void editdrawSingleFan(MESH_T *mesh, int fan_no, int col_no)
{
    COMMAND_T *mc;
    float *vert_x, *vert_y, *vert_z;
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

    vert_x = &mesh -> vrtx[vert_base];
    vert_y = &mesh -> vrty[vert_base];
    vert_z = &mesh -> vrtz[vert_base];

    if (! (mesh -> draw_mode & EDIT_DRAWMODE_SOLID))
    {
        /* Set color depending on texturing type */
        if (bigtex)
        {                           /* It's one with hi res texture */
            sdlglSetColor(SDLGL_COL_LIGHTBLUE); /* color like cartman           */
        }
        else
        {
            sdlglSetColor(SDLGL_COL_LIGHTRED);
        }
        if (col_no > 0)
        {
            sdlglSetColor(col_no);
        }
    }
    
    if (mesh -> draw_mode & EDIT_DRAWMODE_TEXTURED)
    {
        if (bigtex)
        {               /* It's one with hi res texture */
            uv = &mc -> biguv[0];
        }
        else
        {
            uv = &mc -> uv[0];
        }
    }

    offuv = &MeshTileOffUV[(mesh -> fan[fan_no].tx_no & 0x3F) * 2];

    /* Now bind the texture */
    if (mesh -> draw_mode & EDIT_DRAWMODE_TEXTURED)
    {
        glBindTexture(GL_TEXTURE_2D, WallTex[((mesh -> fan[fan_no].tx_no >> 6) & 3)]);
    }

    entry    = 0;
    vertexno = mc -> vertexno;

    for (cnt = 0; cnt < mc -> count; cnt++)
    {
        glBegin (GL_TRIANGLE_FAN);

            for (tnc = 0; tnc < mc -> size[cnt]; tnc++)
            {
                actvertex = vertexno[entry]; 	/* Number of vertex to draw */

                if (mesh -> draw_mode & EDIT_DRAWMODE_SOLID)
                {
                    /* Show ambient lighting */
                    color[0] = color[1] = color[2] = mesh -> vrta[actvertex];
                    
                    if (mesh -> draw_mode & EDIT_DRAWMODE_LIGHTMAX)
                    {
                        color[0] = color[1] = color[2] = 255;
                    }
                    
                    glColor3ubv(&color[0]);		/* Set the light color */
                }
                if (mesh -> draw_mode & EDIT_DRAWMODE_TEXTURED)
                {
                    glTexCoord2f(uv[(actvertex * 2) + 0] + offuv[0], uv[(actvertex * 2) + 1] + offuv[1]);
                }
                
                glVertex3f(vert_x[actvertex], vert_y[actvertex], vert_z[actvertex]);

                entry++;
            }

        glEnd();
    } 
}

/*
 *  Name:
 *	   editdrawMap
 *  Description:
 *	   Draws the whole map. Doen's support any culling code.
 * Input:
 *     mesh *:   Pointer on mesh to draw
 *     ft *:     Pointer on description of chosen fan type
 *     fd *:     ft -> type >= 0 Display it over the position of chosen fan
 *     crect *: Info about rectangle of chosen tiles [cx1, cy1, cx2, cy2]
 */
static void editdrawMap(MESH_T *mesh, FANDATA_T *ft, COMMAND_T *fd, int *crect)
{
    int fan_no;


    glPolygonMode(GL_FRONT, mesh -> draw_mode & EDIT_DRAWMODE_SOLID ? GL_FILL : GL_LINE);
    glFrontFace(GL_CW);

    if (mesh -> draw_mode & EDIT_DRAWMODE_TEXTURED)
    {
        glEnable(GL_TEXTURE_2D);
    }

    // @todo: Use draw edges from actual used camera
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
    for (fan_no = 0; fan_no < mesh -> numfan;  fan_no++)
    {
        if (mesh -> fan[fan_no].type == 0)
        {
            /* --- Draw the basic fan, solid --- */
            editdrawSingleFan(mesh, fan_no, 0);
            /* --- Draw overlays, if needed --- */
            editdrawOverlay3D(mesh, fan_no, crect);
            
        }
    }

    /* =============== Second draw wall tiles ============== */
    for (fan_no = 0; fan_no < mesh -> numfan; fan_no++)
    {
        if (mesh -> fan[fan_no].type != 0)
        {
            /* --- Draw the basic fan, solid --- */
            editdrawSingleFan(mesh, fan_no, 0);
            /* --- Draw overlays, if needed --- */
            editdrawOverlay3D(mesh, fan_no, crect);
            // Draw object on tile
        }
        
        // Draw objects on bottom tiles
        if (mesh -> fan[fan_no].type == 0 && mesh -> fan[fan_no].obj_no > 0)
        {
            editdrawObject(mesh -> fan[fan_no].obj_no);
        }
    }      
    
    if (mesh -> draw_mode & EDIT_DRAWMODE_TEXTURED)
    {
        glDisable(GL_TEXTURE_2D);
    }
    
    /* Draw the chosen fan type, if any */
    if (fd -> numvertices > 0)
    {
        editdrawChosenFanType(ft, fd);
    } 
}

/*
 *  Name:
 *	    editdraw2DCameraPos
 *  Description:
 *	    Draw the Camera-Frustum in size od 2D-Map
 * Input:
 *      x, y:   Positon in 2D
 *      df:     Downsize-Factor from map-tile to minimap-tile
 */
static void editdraw2DCameraPos(int x, int y, int df)
{
    SDLGL3D_OBJECT *campos;
    SDLGL3D_FRUSTUM f;          /* Information about the frustum        */
    SDLGL_RECT draw_rect;
    int zmax;


    campos = sdlgl3dGetCameraInfo(0, &f);

    draw_rect.x = x + (campos -> pos[0] / df);
    draw_rect.y = y + (campos -> pos[1] / df);
    zmax = f.zmax / df;       

    /* Now draw the camera angle */
    glBegin(GL_LINES);
        sdlglSetColor(SDLGL_COL_LIGHTGREEN);
        glVertex2i(draw_rect.x, draw_rect.y);       /* Right edge of frustum */
        glVertex2i(draw_rect.x + (f.nx[0] * zmax),
                   draw_rect.y + (f.ny[0] * zmax));

        sdlglSetColor(SDLGL_COL_LIGHTRED);          /* Left edge of frustum */
        glVertex2i(draw_rect.x, draw_rect.y);
        glVertex2i(draw_rect.x + (f.nx[1] * zmax),
                   draw_rect.y + (f.ny[1] * zmax));
        sdlglSetColor(SDLGL_COL_WHITE);
        glVertex2i(draw_rect.x, draw_rect.y);       /* Direction            */
        glVertex2i(draw_rect.x + (f.nx[2] * zmax),
                   draw_rect.y + (f.ny[2] * zmax));
    glEnd();
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
 *     None
 */
void editdrawInitData(void)
{
    char texturename[128];
    char *fname;
    int entry, cnt;
    

    // Make tile texture start offsets
    for (entry = 0; entry < (EDITDRAW_MAXWALLSUBTEX * 2); entry += 2)
    {
        MeshTileOffUV[entry]     = (((entry / 2) & 7) * 0.125);
        MeshTileOffUV[entry + 1] = (((entry / 2) / 8) * 0.125);
    }

    /* Load the basic textures */
    for (cnt = 0; cnt < 4; cnt++)
    {
        sprintf(texturename, "tile%d.bmp", cnt);

        fname = editfileMakeFileName(EDITFILE_GAMEDATDIR, texturename);

        WallTex[cnt] = sdlgltexLoadSingle(fname);
    }
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
 *     mesh *:  Pointer on mesh to draw
 *     ft *:    Pointer on chosen fan type
 *     fd *:    ft -> type >= 0 Display it over the position of chosen fan     
 *     crect *: Info about rectangle of chosen tiles [cx1, cy1, cx2, cy2]              
 */
void editdraw3DView(MESH_T *mesh, FANDATA_T *ft, COMMAND_T *fd, int *crect)
{
    int w, h;
    int x, y, x2, y2;


    sdlgl3dBegin(0, 0);        /* Draw not solid */

    /* Draw a grid 64 x 64 squares for test of the camera view */
    /* Draw only if no map is loaded */
    if (! mesh -> map_loaded)
    {
        sdlglSetColor(SDLGL_COL_LIGHTGREEN);

        for (h = 0; h < 64; h++)
        {
            for (w = 0; w < 64; w++)
            {
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
    }
    else
    {
        editdrawMap(mesh, ft, fd,crect);     
    }

    sdlgl3dEnd();
}

/*
 * Name:
 *     editdraw2DMap
 * Description:
 *     Draws the map as 2D-View at given position, using size info of
 *     minimap held in the mesh info 
 * Input:
 *     mesh *:  Pointer on mesh to draw
 *     mx, my:  Draw at this position on screen
 *     mw, mh:  Size of map on Screen   
 *     tw:      Size of a tile of minimap 
 *     crect *: Info about rectangle of chosen tiles [cx1, cy1, cx2, cy2]
 */
void editdraw2DMap(MESH_T *mesh, int mx, int my, int mw, int mh, int tw, int *crect)
{
    SDLGL_RECT draw_rect, draw_rect2;
    int fan_no;
    int rx2, ry2;
    int w, h;

    
    draw_rect.x = mx;
    draw_rect.y = my;
    draw_rect.w = tw;
    draw_rect.h = tw;

    fan_no = 0;
    glBegin(GL_QUADS);    
        for (h = 0; h < mesh -> tiles_y; h++)
        {
            for (w = 0; w < mesh -> tiles_x; w++)
            {
                /* Set color depeding on WALL-FX-Flags    */
                if (mesh -> fan[fan_no].fx & MPDFX_WALL)
                {
                    sdlglSetColor(SDLGL_COL_BLACK);
                }
                else if (0 == mesh -> fan[fan_no].fx)
                {
                    sdlglSetColor(SDLGL_COL_LIGHTGREY);
                }
                else
                {
                    /* Colors >= 15: Others then standard colors */
                    sdlglSetColor(mesh -> fan[fan_no].type + 15);
                }

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
            draw_rect.x = mx;
            draw_rect.y += draw_rect.h;
        }

    glEnd();

    draw_rect.x = mx;
    draw_rect.y = my;

    /* ---- Draw all special fans, if asked for ---- */
    editdrawOverlay2D(mesh, &draw_rect, crect);

    /* ------------ Draw grid for easier editing ------- */
    draw_rect.x = mx;
    draw_rect.y = my;

    for (h = 0; h < mesh -> tiles_y; h++)
    {
        for (w = 0; w < mesh -> tiles_x; w++)
        {
            sdlglstrDrawRectColNo(&draw_rect, SDLGL_COL_GREEN, 0);
            draw_rect.x += draw_rect.w;
        }

        /* Next line    */
        draw_rect.x = mx;
        draw_rect.y += draw_rect.h;
    }
    
    /* ---------- Draw border for 2D-Map ---------- */
    draw_rect2.x = mx;
    draw_rect2.y = my;
    draw_rect2.w = mw;
    draw_rect2.h = mh;
    sdlglstrDrawRectColNo(&draw_rect2, SDLGL_COL_WHITE, 0);

    /* ------------ Draw the camera as last one -------- */
    editdraw2DCameraPos(mx, my, 128 / tw);
}


/*
 * Name:
 *     editdraw2DTex
 * Description:
 *     Draws the texture into chosen rectangle and draws a rectangle
 *     around the texture part used.
 * Input:
 *     x, y, w, h: Draw into this rectangle
 *     tx_no:      This texture
 *     tx_big:     Is a big texture
 */
void editdraw2DTex(int x, int y, int w, int h, unsigned char tx_no, char tx_big)
{
    int x2, y2;
    int spart_size, part_size;
    unsigned char main_tx_no, subtex_no;
    
    
    main_tx_no = (unsigned char)((tx_no >> 6) & 3);
    subtex_no  = (unsigned char)(tx_no & 0x3F);

    x2 = x + w;
    y2 = y + h;
    /* Draw the texture, counter-clockwise */
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, WallTex[main_tx_no]);
    sdlglSetColor(SDLGL_COL_WHITE);
    glBegin(GL_TRIANGLE_FAN);
        glTexCoord2fv(&DefaultUV[0]);
        glVertex2i(x, y);
        glTexCoord2fv(&DefaultUV[6]);
        glVertex2i(x, y2);
        glTexCoord2fv(&DefaultUV[4]);
        glVertex2i(x2, y2);
        glTexCoord2fv(&DefaultUV[2]);
        glVertex2i(x2, y);
    glEnd();
    glDisable(GL_TEXTURE_2D);
    /* ---- Draw a rectangle around the chosen texture-part */
    spart_size = w / 8;
    part_size = w / 8;
    if (tx_big)
    {
        part_size *= 2;
    }

    x += (subtex_no % 8) * spart_size;
    y += (subtex_no / 8) * spart_size;
    x2 = x + part_size;
    y2 = y + part_size;
    glBegin(GL_LINE_LOOP);
        glVertex2i(x, y);
        glVertex2i(x, y2);
        glVertex2i(x2, y2);
        glVertex2i(x2, y);
    glEnd();
}

/*
 * Name:
 *     editdrawAdjustCamera
 * Description:
 *     Adjusts camera to view at given tile at tx/ty
 * Input:
 *     tx, ty:    Where to look
 */
void editdrawAdjustCamera(int tx, int ty)
{
    sdlgl3dMoveToPosCamera(0, tx * 128, ty * 128, 0, 1);
}

