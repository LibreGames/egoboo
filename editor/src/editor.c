/*******************************************************************************
*  EDITOR.C                                                                    *
*      - Main loop and menu of editor for EGOBOO-Modules                       *
*                                                                              *
*  EGOBOO - Editor                                                             *
*      (c) 2010 Paul Mueller <pmtech@swissonline.ch>                           *
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
#include "sdlglcfg.h"
#include "sdlgl3d.h"
#include "editdraw.h"

/*******************************************************************************
* DEFINES								                                       *
*******************************************************************************/

#define EDITOR_CAPTION "EGOBOO - Editor"

#define EDITOR_MAXFLD   60

/* --------- The different commands -------- */
#define EDITOR_EXITPROGRAM    ((char)100)

/* Menu commands -- Don't collid with 3D-Commands */
#define EDITOR_FILE       ((char)101)
#define EDITOR_MAP        ((char)102)

/*******************************************************************************
* DATA									                                       *
*******************************************************************************/

static SDLGL_CONFIG SdlGlConfig = {

    0,
    800, 600,           /* scrwidth, scrheight: Size of screen  */
    24,                 /* colordepth: Colordepth of screen     */
    0,                  /* wireframe                            */
    0,                  /* hidemouse                            */
    SDLGL_SCRMODE_WINDOWED, /* screenmode                       */
    1,                  /* debugmode                            */
    0                   /* zbuffer                              */

};

static SDLGLCFG_NAMEDVALUE CfgValues[] = {

    { SDLGLCFG_VAL_INT, &SdlGlConfig.scrwidth, "scrwidth" },
    { SDLGLCFG_VAL_INT, &SdlGlConfig.scrheight, "scrheight" },
    { SDLGLCFG_VAL_INT, &SdlGlConfig.colordepth, "colordepth" },
    { SDLGLCFG_VAL_INT, &SdlGlConfig.screenmode, "screenmode" },
    { SDLGLCFG_VAL_INT, &SdlGlConfig.debugmode, "debugmode" },
    { 0 }
    
};

static char StatusBar[80];

static SDLGL_CMDKEY EditorCmd[] = {

    /* ---------- Camera movement ------ */
    { { SDLK_KP8 }, SDLGL3D_MOVE_AHEAD,  +1, SDLGL3D_MOVE_AHEAD },
    { { SDLK_KP2 }, SDLGL3D_MOVE_AHEAD,  -1, SDLGL3D_MOVE_AHEAD },
    { { SDLK_KP4 }, SDLGL3D_MOVE_STRAFE, -1, SDLGL3D_MOVE_STRAFE },
    { { SDLK_KP6 }, SDLGL3D_MOVE_STRAFE, +1, SDLGL3D_MOVE_STRAFE },
    { { SDLK_KP7 }, SDLGL3D_MOVE_TURN,   -1, SDLGL3D_MOVE_TURN },
    { { SDLK_KP9 }, SDLGL3D_MOVE_TURN,   +1, SDLGL3D_MOVE_TURN },
    { { SDLK_KP_PLUS },  SDLGL3D_CAMERA_ZOOM, +1, SDLGL3D_CAMERA_ZOOM },
    { { SDLK_KP_MINUS }, SDLGL3D_CAMERA_ZOOM, -1, SDLGL3D_CAMERA_ZOOM },
    /* -------- Switch the player with given number ------- */
    { { SDLK_ESCAPE }, EDITOR_EXITPROGRAM },
    { { 0 } }

};

static SDLGL_FIELD MainMenu[EDITOR_MAXFLD + 2] = {
    { SDLGL_TYPE_STD,  {   0, 584, 800, 16 } },
    /* 'Code' needed in menu-background' for support of 'mouse-over' */
    { SDLGL_TYPE_STD,  {   0, 0, 800, 16 }, -1 }, /* Menu-Background:  */
    { SDLGL_TYPE_MENU, {   4, 4, 32, 8 }, EDITOR_FILE, 0, "File" },
    { SDLGL_TYPE_MENU, {  44, 4, 24, 8 }, EDITOR_MAP, 0, "Map" },
    { 0 }

};

/* ------ Sub-Menus ------- */
static SDLGL_FIELD SubMenu[] = {
    /* File-Menu */
    { SDLGL_TYPE_STD,  {   0, 16, 80, 16 }, EDITOR_FILE, -1 },     /* Menu-Background */
    { SDLGL_TYPE_MENU, {   4, 20, 72,  8 }, EDITOR_FILE, 3, "Exit" },
    /* --- Maze-Menu */
    { SDLGL_TYPE_STD,  {  40, 16, 64, 56 }, EDITOR_MAP, -1 },    /* Menu-Background */
    { SDLGL_TYPE_MENU, {  44, 20, 56,  8 }, EDITOR_MAP, 1, "New" },
    { SDLGL_TYPE_MENU, {  44, 36, 56,  8 }, EDITOR_MAP, 2, "Load..." },
    { SDLGL_TYPE_MENU, {  44, 52, 56,  8 }, EDITOR_MAP, 3, "Save" },
    { 0 }
};

/*******************************************************************************
* CODE									                                       *
*******************************************************************************/

/*
 * Name:
 *     editorSetMenu
 * Description:
 *     Drawing of start screen
 * Input:
 *     which: Which menu to open
 */
static void editorSetMenu(char which)
{

    static char MenuActualOpen = 0;
    
    SDLGL_FIELD *pstart, *pend;
    char save_type;


    if (MenuActualOpen != which) {

        sdlglInputRemove(MenuActualOpen);

        if (which == 0) {

            /* Close dropdown-menu */
            MenuActualOpen = 0;
            return;

        }

        /* --- Find sub-menu --- */
        pstart = SubMenu;

        while (pstart -> sdlgl_type != 0) {

            if (pstart -> code == which) {

                /* Found it */
                pend = pstart;
                while (pend -> sdlgl_type != 0 && pend -> code == which) {

                    pend++;

                }

                /* Menu has changed, set it */
                save_type = pend -> sdlgl_type;
                pend -> sdlgl_type = 0;

                sdlglInputAdd(which, pstart);

                pend -> sdlgl_type = save_type;

                MenuActualOpen = which;
                return;

            }

            pstart++;

        }

    }

}

/*
 * Name:
 *     editorDrawFunc
 * Description:
 *     Draw anything on screen
 * Input:
 *
 */
#ifdef __BORLANDC__
    #pragma argsused
#endif
static void editorDrawFunc(SDLGL_FIELD *fields, SDLGL_EVENT *event)
{

    int color;
    char mouse_over;


    glClear(GL_COLOR_BUFFER_BIT);

    /* Draw the 3D-View before the 2D-Screen */
    editdraw3DView();
    
    /* Draw the menu -- open/close dropdown, if needed */
    mouse_over = 0;
    sdlglstrSetFont(SDLGLSTR_FONT8);

    while (fields -> sdlgl_type > 0) {

        if (fields -> fstate & SDLGL_FSTATE_MOUSEOVER) {

            mouse_over = 1;

        }


        if (fields -> sdlgl_type == SDLGL_TYPE_STD) {
            /* Draw the background of the menu and the status bar */
            sdlglstrDrawButton(&fields -> rect, 0, 0);
        }
        else if (fields -> sdlgl_type == SDLGL_TYPE_MENU) {

            if (fields -> fstate & SDLGL_FSTATE_MOUSEOVER) {

                color = SDLGLSTR_COL_RED;

                if (fields -> code > 0 && fields -> sub_code == 0) {

                    editorSetMenu(fields -> code);

                }

            }
            else {

                color = SDLGLSTR_COL_WHITE;

            }

            sdlglstrSetColorNo(color);
            sdlglstrString(&fields -> rect, fields -> pdata);

        }

        fields++;

    }

    /* Draw the string in the status bar */
    sdlglstrSetColorNo(SDLGLSTR_COL_WHITE);
    sdlglstrString(&fields -> rect, StatusBar);


    if (! mouse_over) {

        editorSetMenu(0);

    }

    sdlgl3dMoveObjects(event -> secondspassed); /* Move camera */

}

/*
 * Name:
 *     editorInputHandler
 * Description:
 *     Handler for the start screen
 * Input:
 *     event *: Pointer on event
 */
static int editorInputHandler(SDLGL_EVENT *event)
{

    if (event -> code > 0) {

        if (event -> code < 0x15) {
            /* Move the camera */
            if (event -> pressed) {
                /* Start movement */
                sdlgl3dManageObject(-1, event -> code, event -> sub_code);

            }
            else {
                /* Stop movement */
                sdlgl3dManageObject(-1, 0, 0);

            }

        }
        else {
            /* Own commands */
            switch(event -> code) {

                case EDITOR_FILE:
                    if (event -> sub_code == 3) {
                        return SDLGL_INPUT_EXIT;
                    }
                 break;
                case EDITOR_EXITPROGRAM:
                    return SDLGL_INPUT_EXIT;
                    
            }

        }

    } /* if (event -> type == SDLGL_INPUT_COMMAND) */

    return SDLGL_INPUT_OK;

}

/*
 * Name:
 *     editorStart
 * Description:
 *      Sets up the first input screen
 * Input:
 *     None
 */
static void editorStart(void)
{

    /* -------- Adjust the menu bar sizes to screen ------- */
    /* Status-Bar   */
    sdlglAdjustRectToScreen(&MainMenu[0].rect,
                            &MainMenu[0].rect,
                            (SDLGL_FRECT_SCRWIDTH | SDLGL_FRECT_SCRBOTTOM));
    /* Menu-Bar     */
    sdlglAdjustRectToScreen(&MainMenu[1].rect,
                            &MainMenu[1].rect,
                            SDLGL_FRECT_SCRWIDTH);
    /* -------- Now create the output screen ---------- */
    sdlglInputNew(editorDrawFunc,
                  editorInputHandler,     /* Handler for input    */
                  MainMenu,               /* Input-Fields (mouse) */
                  EditorCmd,              /* Keyboard-Commands    */
                  EDITOR_MAXFLD);         /* Buffer size for dynamic menus    */

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

    /* Read configuration from file */
    sdlglcfgReadSimple("data/editor.cfg", CfgValues);

    /* Now read the configuration file and the global data, if available */

    /* Now overwrite the win caption, because in this stage it displays */
    /* the version number and the compile data.				            */

    SdlGlConfig.wincaption = EDITOR_CAPTION;

    sdlglInit(&SdlGlConfig);

    glPolygonMode(GL_FRONT, GL_FILL);
    glShadeModel(GL_FLAT);		/* No smoothing	of edges */

    /* Init Camera +Z is up, -Y is near plane, X is left/right */
    sdlgl3dInitCamera(32 * 128.0, -128.0, 64.0, 270, 0, 0);

    /* Set the input handlers and do other stuff before  */
    editorStart();

    /* Now enter the mainloop */
    sdlglExecute();

    /* Do any shutdown stuff. */

    sdlglShutdown();
    return 0;

}
