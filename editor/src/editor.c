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
#include "editor.h"
#include "editmain.h"

/*******************************************************************************
* DEFINES								                                       *
*******************************************************************************/

#define EDITOR_CAPTION "EGOBOO - An Editor V 0.1"

#define EDITOR_MAXFLD   60

/* --------- The different commands -------- */
#define EDITOR_EXITPROGRAM    ((char)100)

/* Menu commands -- Don't collid with 3D-Commands */
#define EDITOR_FILE     ((char)101)
#define EDITOR_MAP      ((char)102)
#define EDITOR_FX       ((char)103)
#define EDITOR_STATE    ((char)104)
#define EDITOR_SETTINGS ((char)105)
#define EDITOR_2DMAP    ((char)106)           /* Displayed map */

/* Sub-commands for main commands, if needed */
#define EDITOR_STATE_SHOWMAP ((char)1)

#define EDITOR_FILE_EXIT   ((char)3)

/* Sub-Commands */
#define EDITOR_2DMAP_CHOOSEFAN  ((char)1)
#define EDITOR_2DMAP_FANINFO    ((char)2)
#define EDITOR_2DMAP_FANROTATE  ((char)4)
#define EDITOR_2DMAP_FANBROWSE  ((char)8)

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

static EDITMAIN_STATE_T *pEditState;
static char StatusBar[80];
static char Map2DState;     /* EDITOR_2DMAP_ */

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
    { { SDLK_LCTRL, SDLK_m }, EDITOR_STATE, EDITOR_STATE_SHOWMAP },
    /* { { SDLK_x }, SDLGL3D_MOVE_ROTX, +1, SDLGL3D_MOVE_ROTX }, */
    { { SDLK_i }, EDITOR_2DMAP, EDITOR_2DMAP_FANINFO, EDITOR_2DMAP },
    { { SDLK_r }, EDITOR_2DMAP, EDITOR_2DMAP_FANROTATE, EDITOR_2DMAP },
    { { SDLK_b }, EDITOR_2DMAP, EDITOR_2DMAP_FANBROWSE, EDITOR_2DMAP },
    /* -------- Switch the player with given number ------- */
    { { SDLK_ESCAPE }, EDITOR_EXITPROGRAM },
    { { 0 } }

};

static SDLGL_FIELD MainMenu[EDITOR_MAXFLD + 2] = {
    { SDLGL_TYPE_STD,  {   0, 584, 800,  16 } },            /* Status bar       */
    { SDLGL_TYPE_MENU + 10,  { 0, 16, 256, 256 }, EDITOR_2DMAP, EDITOR_2DMAP_CHOOSEFAN }, /* Map-Rectangle    */
    /* 'Code' needed in menu-background' for support of 'mouse-over'    */
    { SDLGL_TYPE_STD,  {   0, 0, 800, 16 }, -1 },           /* Menu-Background  */
    { SDLGL_TYPE_MENU, {   4, 4, 32, 8 }, EDITOR_FILE, 0, "File" },
    { SDLGL_TYPE_MENU, {  44, 4, 24, 8 }, EDITOR_MAP, 0, "Map" },
    { SDLGL_TYPE_MENU, {  76, 4, 64, 8 }, EDITOR_SETTINGS, 0, "Settings" },
    { 0 }

};

/* ------ Sub-Menus ------- */
static SDLGL_FIELD SubMenu[] = {
    /* File-Menu */
    { SDLGL_TYPE_STD,  {   0, 16, 80, 16 }, EDITOR_FILE, -1 },  /* Menu-Background */
    { SDLGL_TYPE_MENU, {   4, 20, 72,  8 }, EDITOR_FILE, EDITOR_FILE_EXIT, "Exit" },
    /* --- Maze-Menu */
    { SDLGL_TYPE_STD,  {  40, 16, 80, 72 }, EDITOR_MAP, -1 },   /* Menu-Background */
    { SDLGL_TYPE_MENU, {  44, 20, 72,  8 }, EDITOR_MAP, EDITMAIN_NEWFLATMAP,  "New Flat" },
    { SDLGL_TYPE_MENU, {  44, 36, 72,  8 }, EDITOR_MAP, EDITMAIN_NEWSOLIDMAP,  "New Solid" },
    { SDLGL_TYPE_MENU, {  44, 52, 72,  8 }, EDITOR_MAP, EDITMAIN_LOADMAP, "Load..." },
    { SDLGL_TYPE_MENU, {  44, 68, 72,  8 }, EDITOR_MAP, EDITMAIN_SAVEMAP, "Save" },
    /* ---- Settings menu for view ---- */
    { SDLGL_TYPE_STD,  {  68, 16, 136, 56 }, EDITOR_SETTINGS, -1 },   /* Menu-Background */
    { SDLGL_TYPE_MENU, {  72, 20, 128,  8 }, EDITOR_SETTINGS, EDIT_MODE_SOLID,    "[ ] Draw Solid" },
    { SDLGL_TYPE_MENU, {  72, 36, 128,  8 }, EDITOR_SETTINGS, EDIT_MODE_TEXTURED, "[ ] Draw Texture"  },
    { SDLGL_TYPE_MENU, {  72, 52, 128,  8 }, EDITOR_SETTINGS, EDIT_MODE_LIGHTMAX, "[ ] Max Light" },
    /* TODO: Add EDITOR_MAP, 'EDITMAIN_LOADSPAWN'               */
    { 0 }
};

/* Prepared Dialog for changing of Flags of a fan. TODO: Make it dynamic */
static SDLGL_FIELD FanInfoDlg[] = {
    { SDLGL_TYPE_STD, { 0,  16, 320, 208 } },        /* Background of dialog */
    { SDLGL_TYPE_STD, { 8,  24, 320, 208 }, 0, 0, "Info about Fan" },
    { SDLGL_TYPE_STD, { 8,  40, 320, 208 }, 0, 0, "" },
    { SDLGL_TYPE_STD, { 8,  56, 120, 8 }, EDITOR_FX, MPDFX_SHA,     "[ ]Reflective" },
    { SDLGL_TYPE_STD, { 8,  72, 120, 8 }, EDITOR_FX, MPDFX_DRAWREF, "[ ]Draw Reflection" },
    { SDLGL_TYPE_STD, { 8,  88, 120, 8 }, EDITOR_FX, MPDFX_ANIM,    "[ ]Animated" },
    { SDLGL_TYPE_STD, { 8, 104, 120, 8 }, EDITOR_FX, MPDFX_WATER,   "[ ]Overlay (Water)" },
    { SDLGL_TYPE_STD, { 8, 120, 120, 8 }, EDITOR_FX, MPDFX_WALL,    "[ ]Barrier (Slit)" },
    { SDLGL_TYPE_STD, { 8, 136, 120, 8 }, EDITOR_FX, MPDFX_IMPASS,  "[ ]Impassable (Wall)" },
    { SDLGL_TYPE_STD, { 8, 154, 120, 8 }, EDITOR_FX, MPDFX_DAMAGE,  "[ ]Damage" },
    { SDLGL_TYPE_STD, { 8, 170, 120, 8 }, EDITOR_FX, MPDFX_SLIPPY,  "[ ]Slippy" },
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
 *     editorSetToggleChar
 * Description:
 *     Sets or removes the char 'X' in given string at second position,
 *     based on state of given value 'is_set'
 * Input:
 *     is_set:
 *     name *:
 */
static void editorSetToggleChar(char is_set, char *name)
{
    if (is_set) {
        name[1] = 'X';
    }
    else {
        name[1] = ' ';
    }
}

/*
 * Name:
 *     editorSetMenuViewToggle
 * Description:
 *     Show toggled values as 'X' in menu string
 * Input:
 *     None
 */
static void editorSetMenuViewToggle(void)
{
    editorSetToggleChar(pEditState -> draw_mode & EDIT_MODE_SOLID, SubMenu[8].pdata);
    editorSetToggleChar(pEditState -> draw_mode & EDIT_MODE_TEXTURED, SubMenu[8 + 1].pdata);
    editorSetToggleChar(pEditState -> draw_mode & EDIT_MODE_LIGHTMAX, SubMenu[8 + 2].pdata);
}

/*
 * Name:
 *     editor2DMap
 * Description:
 *     Handles all functions for the code 'EDITOR_2DMAP' 
 * Input:
 *     event *: Pointer on event
 */
static void editor2DMap(SDLGL_EVENT *event)
{

    SDLGL_FIELD *field;


    switch(event -> sub_code) {

        case EDITOR_2DMAP_CHOOSEFAN:
            if (pEditState -> display_flags & EDITMAIN_SHOW2DMAP) {

                field = event -> field;
                editmainChooseFan(event -> mou.x, event -> mou.y,
                                  field -> rect.w, field -> rect.h);
                /* And now move camera to this position */
                sdlgl3dMoveToPosCamera(0,
                                       (pEditState -> tile_x * 128.0) - 450.0,
                                       (pEditState -> tile_y * 128.0),
                                       600.0);
                if (event -> modflags == SDL_BUTTON_LEFT) {
                    /* TODO: Replace type of chosen fan by new type of fan */
                }
                else if (event -> modflags == SDL_BUTTON_RIGHT){
                    /* TODO: Do action based on number 'Map2DState'  */
                }

            }
            break;
        case EDITOR_2DMAP_FANINFO:
            if (event -> pressed) {
                Map2DState ^= event -> sub_code;
                /* TODO: Display string in Statusbar according to action */
            }
            else {
                Map2DState = 0;
            }
            break;
        case EDITOR_2DMAP_FANROTATE:
            /* TODO: Rotate the chosen fan type */
            break;
        case EDITOR_2DMAP_FANBROWSE:
            /* TODO: Browse trough the available fan types */
            break;

    }

}

/*
 * Name:
 *     editorDrawFanInfo
 * Description:
 *     Print info about chosen fan on screen
 * Input:
 *     None
 */
static void editorDrawFanInfo(void)
{

    SDLGL_FIELD *fields;
    int i;


    if (Map2DState == EDITOR_2DMAP_FANINFO && pEditState -> fan_chosen >= 0) {

        /* Set the display of state flags */
        for (i = 3; i < 11; i++) {
            editorSetToggleChar(pEditState -> act_fan.fx & FanInfoDlg[i].sub_code,
                                FanInfoDlg[i].pdata);
        }
        /* Draw info about chosen fan: EditState.act_fan */
        fields = &FanInfoDlg[0];
        /* Draw background */
        sdlglstrDrawButton(&fields -> rect, 0, 0);
        fields++;
        /* Draw title */
        sdlglSetColor(SDLGL_COL_BLACK);
        sdlglstrString(&fields -> rect, fields -> pdata);
        fields++;
        /* Draw fan type name */
        sdlglSetColor(SDLGL_COL_GREEN);
        sdlglstrString(&fields -> rect, editmainFanTypeName(pEditState -> act_fan.type));
        fields++;
        /* Draw all strings */
        sdlglstrSetColorNo(SDLGLSTR_COL_WHITE);
        while (fields -> sdlgl_type > 0) {

            sdlglstrString(&fields -> rect, fields -> pdata);
            fields++;

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

    static SDLGL_RECT StrPos = { 10, 100, 0, 0 };

    SDLGL3D_OBJECT *cam;
    float nx[4], ny[4], dist;
    int color;
    char mouse_over;


    glClear(GL_COLOR_BUFFER_BIT);

    /* First move the camera */
    sdlgl3dMoveObjects(event -> secondspassed);

    /* Draw the 3D-View before the 2D-Screen */
    editmainMap(EDITMAIN_DRAWMAP);

    /* ---- Prepare drawing in 2D-Mode ---- */
    glPolygonMode(GL_FRONT, GL_FILL);
    glShadeModel(GL_FLAT);		            /* No smoothing	of edges    */
    glFrontFace(GL_CCW);                    /* Draws counterclockwise   */

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

    /* Draw map if needed */
    if (pEditState -> display_flags & EDITMAIN_SHOW2DMAP) {

        editmainDrawMap2D(MainMenu[1].rect.x, MainMenu[1].rect.y,
                          MainMenu[1].rect.w, MainMenu[1].rect.h);

    }

    /* Display camera position */
    cam = sdlgl3dGetCameraInfo(0, nx, nx, &dist);
    sdlglstrStringF(&StrPos, "RotX: %f\n\nRotY: %f\n\nRotZ: %f\n\n",
                    cam -> rot[0], cam -> rot[1], cam -> rot[2]);
    /* Display info about actual chosen fan, if asked for */
    editorDrawFanInfo();

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
                sdlgl3dManageCamera(0, event -> code, event -> sub_code);

            }
            else {
                /* Stop movement */
                sdlgl3dManageCamera(0, 0, 0);

            }

        }
        else {
            /* Own commands */
            switch(event -> code) {

                case EDITOR_FILE:
                    if (event -> sub_code == EDITOR_FILE_EXIT) {
                        return SDLGL_INPUT_EXIT;
                    }
                    break;
                case EDITOR_MAP:
                    if (! editmainMap(event -> sub_code)) {
                        /* TODO: Display message, what has gone wrong   */
                        /* TODO: Add 'EDITMAIN_LOADSPAWN'               */
                        /* TODO: Close menu, if menu-point is chosen    */
                    }
                    break;
                case EDITOR_STATE:
                    if (event -> sub_code == EDITOR_STATE_SHOWMAP) {

                        pEditState -> display_flags ^= EDITMAIN_SHOW2DMAP;

                    }
                    break;
                case EDITOR_SETTINGS:
                    editmainToggleFlag(EDITMAIN_TOGGLE_DRAWMODE, event -> sub_code);
                    editorSetMenuViewToggle();
                    break;

                case EDITOR_2DMAP:
                    /* TODO: Make this a 'dynamic' input field */
                    editor2DMap(event);     
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

    /* Initalize the maze editor */
    pEditState = editmainInit();
    /* Display initally chosen values */    
    editorSetMenuViewToggle();
    /* -------- Adjust the menu bar sizes to screen ------- */
    /* Status-Bar   */
    sdlglAdjustRectToScreen(&MainMenu[0].rect,
                            &MainMenu[0].rect,
                            (SDLGL_FRECT_SCRWIDTH | SDLGL_FRECT_SCRBOTTOM));
    /* Map-Rectangle    */
    sdlglAdjustRectToScreen(&MainMenu[1].rect,
                            &MainMenu[1].rect,
                            SDLGL_FRECT_SCRRIGHT);
    /* Menu-Bar     */
    sdlglAdjustRectToScreen(&MainMenu[2].rect,
                            &MainMenu[2].rect,
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

    SdlGlConfig.wincaption = EDITOR_CAPTION;

    sdlglInit(&SdlGlConfig);  


    sdlgl3dInitCamera(0, 320, 0, 90, 0.75);
    /* Init Camera +Z is up, -Y is near plane, X is left/right */
    sdlgl3dMoveToPosCamera(0, 384.0, 384.0, 600.0);

    /* Set the input handlers and do other stuff before  */
    editorStart();

    /* Now enter the mainloop */
    sdlglExecute();

    /* Do any shutdown stuff. */
    editmainExit();

    sdlglShutdown();
    return 0;

}
