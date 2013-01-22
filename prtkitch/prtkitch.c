/*******************************************************************************
*  PRTKITCH.C                                                                  *
*      - prtkitch Code for use of SDLGL main functions                          *
*                                                                              *
*  EGOBOO - Particle kitchen, Testbed for loading/displaying particles         *
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
* INCLUDES								                                       *
*******************************************************************************/

#include <stdio.h>		/* sprintf()    */
#include <memory.h>     /* memset()     */


/* ----- Own headers --- */
#include "sdlgl.h"          /* OpenGL and SDL-Stuff             */
#include "sdlglstr.h"
#include "sdlgl3d.h"        // Display in three dimensions and 3D-Objects


#include "egodefs.h"        // Definition of object-types
#include "render3d.h"       // Display the particle objects
#include "particle.h"       /* This is to be tested -- */

/*******************************************************************************
* DEFINES								                                       *
*******************************************************************************/

/* --------- The different commands -------- */
#define PRTKITCH_SPAWNPRT       ((char)1)
#define PRTKITCH_CAMERA         ((char)2)
#define PRTKITCH_EXITPROGRAM    ((char)100)


/*******************************************************************************
* FORWARD DECLARATIONS      			                                       *
*******************************************************************************/

static int  prtkitchInputHandler(SDLGL_EVENT *event);
static void prtkitchSpawnPrt(int pip_no);

/*******************************************************************************
* TYPEDEFS  							                                       *
*******************************************************************************/

/*******************************************************************************
* DATA									                                       *
*******************************************************************************/

static SDLGL_CONFIG SdlGlConfig =
{
    "EGOBOO-Editor - Particle kitchen V 0.1",
    800, 600,           /* scrwidth, scrheight: Size of screen  */
    24,                 /* colordepth: Colordepth of screen     */
    0,                  /* wireframe                            */
    0,                  /* hidemouse                            */
    SDLGL_SCRMODE_WINDOWED, /* screenmode                       */
    1,                  /* debugmode                            */
    0                   /* zbuffer                              */

};

static SDLGL_CMDKEY prtkitchCmd[] =
{
    // Camera movement
    { { SDLK_KP8 }, PRTKITCH_CAMERA, SDLGL3D_MOVE_FORWARD,   SDLGL3D_MOVE_FORWARD },
    { { SDLK_KP2 }, PRTKITCH_CAMERA, SDLGL3D_MOVE_BACKWARD,  SDLGL3D_MOVE_BACKWARD },
    { { SDLK_KP4 }, PRTKITCH_CAMERA, SDLGL3D_MOVE_LEFT,      SDLGL3D_MOVE_LEFT },
    { { SDLK_KP6 }, PRTKITCH_CAMERA, SDLGL3D_MOVE_RIGHT,     SDLGL3D_MOVE_RIGHT },
    { { SDLK_KP7 }, PRTKITCH_CAMERA, SDLGL3D_MOVE_TURNLEFT,  SDLGL3D_MOVE_TURNLEFT },
    { { SDLK_KP9 }, PRTKITCH_CAMERA, SDLGL3D_MOVE_TURNRIGHT, SDLGL3D_MOVE_TURNRIGHT },
    { { SDLK_KP_PLUS },  PRTKITCH_CAMERA, SDLGL3D_MOVE_ZOOMIN,  SDLGL3D_MOVE_ZOOMIN },
    { { SDLK_KP_MINUS }, PRTKITCH_CAMERA, SDLGL3D_MOVE_ZOOMOUT, SDLGL3D_MOVE_ZOOMOUT },
    /* -------- Switch the player with given number ------- */
    { { SDLK_0 }, PRTKITCH_SPAWNPRT, 0 },
    { { SDLK_1 }, PRTKITCH_SPAWNPRT, 1 },
    { { SDLK_2 }, PRTKITCH_SPAWNPRT, 2 },
    { { SDLK_3 }, PRTKITCH_SPAWNPRT, 3 },
    { { SDLK_4 }, PRTKITCH_SPAWNPRT, 4 },
    { { SDLK_5 }, PRTKITCH_SPAWNPRT, 5 },
    { { SDLK_RETURN }, PRTKITCH_EXITPROGRAM },
    { { SDLK_ESCAPE }, PRTKITCH_EXITPROGRAM },
    { { 0 } }

};

// Mouse Input-Fields
static SDLGL_FIELD PrtKitchFields[20];

static int emitter_obj_no = 0;      // Number of the Emitter-Object
static int PrtList[50];             // List of particles with numbers

/*******************************************************************************
* CODE									                                       *
*******************************************************************************/

/*
 * Name:
 *     prtkitchSpawnPrt
 * Description:
 *     Spawns a particle of this type from the position of the 'EmitterObj'
 * Input:
 *     pip_no:  Number of particle descriptor
 *
 */
static void prtkitchSpawnPrt(int pip_no)
{
    particleSpawn(pip_no, emitter_obj_no, 0, 0);
}

/*
 * Name:
 *     prtkitchDrawFunc
 * Description:
 *     Drawing of start screen
 * Input:
 *
 */
#ifdef __BORLANDC__
    #pragma argsused
#endif
static void prtkitchDrawFunc(SDLGL_FIELD *fields, SDLGL_EVENT *event)
{
    glClear(GL_COLOR_BUFFER_BIT);

    sdlglstrSetFont(SDLGLSTR_FONT8);
    sdlglSetColor(SDLGL_COL_YELLOW);


    /* First move the camera */
    sdlgl3dMoveCamera(event -> secondspassed);

    // Do all the movement code before drawing
    sdlgl3dMoveObjects(event -> secondspassed);

    /* Draw the 3D-View before the 2D-Screen */
    // @todo: Draw the particle(s)
    // @todo: Loop trough all objects and display their bounding box
    render3dMain();

    /* ---- Prepare drawing in 2D-Mode ---- */
    glPolygonMode(GL_FRONT, GL_FILL);
    glShadeModel(GL_FLAT);		            /* No smoothing	of edges    */
    glFrontFace(GL_CCW);                    /* Draws counterclockwise   */

    // @todo: Draw the 2D-Part of the screen
}

/*
 * Name:
 *     prtkitchInputHandler
 * Description:
 *     Handler for the start screen
 * Input:
 *     event *: Pointer on event
 */
static int prtkitchInputHandler(SDLGL_EVENT *event)
{
    if (event -> code > 0) {

        switch(event -> code) {

            case PRTKITCH_SPAWNPRT:
                // Spawn a particle                
                prtkitchSpawnPrt(PrtList[event -> sub_code]);
                break;

            case PRTKITCH_CAMERA:
                /* Move the camera */
                if (event -> pressed)
                {
                    /* Start movement */
                    sdlgl3dManageCamera(0, event -> sub_code, 1,
                                        (char)(event -> modflags & KMOD_LSHIFT ? 3 : 1));
                }
                else
                {
                    /* Stop movement */
                    sdlgl3dManageCamera(0, event -> sub_code, 0, 1);
                }
                break;

            case PRTKITCH_EXITPROGRAM:
                return SDLGL_INPUT_EXIT;

        }

    } /* if (event -> code > 0) */

    return SDLGL_INPUT_OK;
}

/*
 * Name:
 *     prtkitchStart
 * Description:
 *      Sets up the first input screen
 * Input:
 *     None
 */
static void prtkitchStart(void)
{
    char fname[64];
    SDLGL3D_OBJECT emitter;
    int i;


    // --- Create and initalize the Emitter-Object
    memset(&emitter, 0, sizeof(SDLGL3D_OBJECT));

    // Describe it as particle, otherwise its not drawn
    emitter.obj_type = EGOMAP_OBJ_PART;      
    // It's position
    emitter.pos[0] = 200;
    emitter.pos[1] = 200;
    emitter.pos[2] = 100;
    // Its bounding box: Left top / right bottom (Cube 20 by 20 pixels)
    emitter.bbox[0][0] = -4;
    emitter.bbox[0][1] = -4;
    emitter.bbox[0][2] = -4;
    emitter.bbox[1][0] = 4;
    emitter.bbox[1][1] = 4;
    emitter.bbox[1][2] = 4;
    
    emitter_obj_no = sdlgl3dCreateObject(&emitter, 0);    

    // Load some particles
    for (i = 0; i < 6; i++)
    {
        sprintf(fname, "data/part%d.txt", i);
        PrtList[i] = particleLoad(fname);
    }

    /* -------- Initialize the 3D-Stuff --------------- */
    sdlgl3dInitCamera(0, 310, 0, 90, 0.75);
    /* Init Camera +Z is up, -Y is near plane, X is left/right */
    sdlgl3dMoveToPosCamera(0, -50.0, 150.0, 300.0, 0);

    // Emit one particle type 2 (Animated fire)
    particleSpawn(PrtList[2], emitter_obj_no, 0, 0);

    /* -------- Initialize the dialog ------------------- */
    /* -------- Now create the output screen ---------- */
    sdlglInputNew(prtkitchDrawFunc,
                  prtkitchInputHandler,     /* Handler for input    */
                  PrtKitchFields,           /* Input-Fields (mouse) */
                  prtkitchCmd,              /* Keyboard-Commands    */
                  19);                      /* Buffer size for dynamic menus    */

}

/* ========================================================================== */
/* ============================= THE MAIN ROUTINE(S) ======================== */
/* ========================================================================== */

/*
 * Name:
 *     main
 * Description:
 *     Main function
 * Input:
 *     argc:
 *     argv **:
 */
#ifdef __BORLANDC__
    #pragma argsused
#endif
int main(int argc, char **argv)
{
    /* Now read the configuration file and the global data, if available */
    sdlglInit(&SdlGlConfig);

    glPolygonMode(GL_FRONT, GL_FILL);
    glShadeModel(GL_FLAT);		/* No smoothing	of edges */

    /* Set the input handlers and do other stuff before  */
    prtkitchStart();
    // Load the 3D-Resources
    render3dInit();

    /* Now enter the mainloop */
    sdlglExecute();

    /* Do any shutdown stuff. */
    // Clean up the 3D-Resources
    render3dCleanup();
    
    // SDL-GL
    sdlglShutdown();
    return 0;
}
