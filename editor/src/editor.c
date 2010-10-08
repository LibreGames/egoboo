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

#define EDITOR_CAPTION "EGOBOO - Map Editor V 0.1.1"

#define EDITOR_MAXFLD   60

/* --------- The different commands -------- */
#define EDITOR_EXITPROGRAM    ((char)100)

/* Menu commands -- Don't collid with 3D-Commands */
#define EDITOR_FILE     ((char)101)
#define EDITOR_FANFX    ((char)102)
#define EDITOR_TOOLS    ((char)103)         /* Tools for the map    */
#define EDITOR_SETTINGS ((char)104)
#define EDITOR_2DMAP    ((char)105)         /* Displayed map */
#define EDITOR_FANTEX   ((char)106)
#define EDITOR_FANDLG   ((char)107)         /* Result of fan dialog         */
#define EDITOR_CAMERA   ((char)108)         /* Movement of camera           */
#define EDITOR_MAPDLG   ((char)109)         /* Settings for new map         */
#define EDITOR_FANPROPERTY  ((char)110)     /* Properties of chosen fan(s)  */
#define EDITOR_SHOWMAP      ((char)111)

/* Sub-Commands */
#define EDITOR_FILE_LOAD  ((char)1)
#define EDITOR_FILE_SAVE  ((char)2)
#define EDITOR_FILE_NEW   ((char)3) 
#define EDITOR_FILE_EXIT  ((char)4)

#define EDITOR_2DMAP_CHOOSEFAN  ((char)1)
#define EDITOR_2DMAP_FANINFO    ((char)2)
#define EDITOR_2DMAP_FANROTATE  ((char)4)
#define EDITOR_2DMAP_FANBROWSE  ((char)8)

#define EDITOR_FANTEX_CHOOSE    ((char)1)
#define EDITOR_FANTEX_SIZE      ((char)2)
#define EDITOR_FANTEX_DEC       ((char)3)
#define EDITOR_FANTEX_INC       ((char)4)

#define EDITOR_FANPROPERTY_EDITMODE ((char)1)   /* Changes the edit-mode    */
#define EDITOR_FANPROPERTY_UPDATE   ((char)2)   /* Depending on actual mode */
#define EDITOR_FANPROPERTY_CLOSE    ((char)3)   /* Close properties dialog  */

#define EDITOR_MAPDLG_SOLID     ((char)1)
#define EDITOR_MAPDLG_SIZE      ((char)2)
#define EDITOR_MAPDLG_DECSIZE   ((char)3)
#define EDITOR_MAPDLG_INCSIZE   ((char)4)
#define EDITOR_MAPDLG_CANCEL    ((char)5)
#define EDITOR_MAPDLG_OK        ((char)6)

#define EDITOR_TOOL_TILE        ((char)1)

/* ------- Drawing types ----- */
#define EDITOR_DRAW2DMAP    ((char)SDLGL_TYPE_MENU + 10)
#define EDITOR_DRAWTEXTURE  ((char)SDLGL_TYPE_MENU + 11)

/*******************************************************************************
* DATA									                                       *
*******************************************************************************/

static SDLGL_RECT DragRect;
static int  NewMapSize = 32;         /* Inital mapsize for new maps */
static char EditorWorkDir[256];

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
    { SDLGLCFG_VAL_INT, &NewMapSize, "mapsize" },
    { SDLGLCFG_VAL_STRING, &EditorWorkDir[0], "workdir", 120 },
    { 0 }

};

static EDITMAIN_STATE_T *pEditState;
static char StatusBar[80];
static char EditActFanName[80];
static char Map2DState;         /* EDITOR_2DMAP_ */
static char EditTypeStr[30];    /* Actual string    */
static char *EditTypeNames[EDITMAIN_EDIT_MAX + 1] = {
    "Fan Info (No Edit)",
    "Edit Simple Mode",
    "Edit Single Fan",
    "Edit Fan-FX",
    "Edit Fan-Texture"
};

static SDLGL_CMDKEY EditorCmd[] = {

    /* ---------- Camera movement ------ */
    { { SDLK_KP8 }, EDITOR_CAMERA, SDLGL3D_MOVE_FORWARD,   SDLGL3D_MOVE_FORWARD },
    { { SDLK_KP2 }, EDITOR_CAMERA, SDLGL3D_MOVE_BACKWARD,  SDLGL3D_MOVE_BACKWARD },
    { { SDLK_KP4 }, EDITOR_CAMERA, SDLGL3D_MOVE_LEFT,      SDLGL3D_MOVE_LEFT },
    { { SDLK_KP6 }, EDITOR_CAMERA, SDLGL3D_MOVE_RIGHT,     SDLGL3D_MOVE_RIGHT },
    { { SDLK_KP7 }, EDITOR_CAMERA, SDLGL3D_MOVE_TURNLEFT,  SDLGL3D_MOVE_TURNLEFT },
    { { SDLK_KP9 }, EDITOR_CAMERA, SDLGL3D_MOVE_TURNRIGHT, SDLGL3D_MOVE_TURNRIGHT },
    { { SDLK_KP_PLUS },  EDITOR_CAMERA, SDLGL3D_MOVE_ZOOMIN,  SDLGL3D_MOVE_ZOOMIN },
    { { SDLK_KP_MINUS }, EDITOR_CAMERA, SDLGL3D_MOVE_ZOOMOUT, SDLGL3D_MOVE_ZOOMOUT },
    { { SDLK_LCTRL, SDLK_m }, EDITOR_SHOWMAP },
    { { SDLK_i }, EDITOR_2DMAP, EDITOR_2DMAP_FANINFO },
    { { SDLK_r }, EDITOR_2DMAP, EDITOR_2DMAP_FANROTATE },
    { { SDLK_SPACE }, EDITOR_2DMAP,  EDITOR_2DMAP_FANBROWSE },   /* Browse trough fan types */
    { { SDLK_LEFT },  EDITOR_CAMERA, SDLGL3D_MOVE_LEFT,     SDLGL3D_MOVE_LEFT },
    { { SDLK_RIGHT }, EDITOR_CAMERA, SDLGL3D_MOVE_RIGHT,    SDLGL3D_MOVE_RIGHT },
    { { SDLK_UP },    EDITOR_CAMERA, SDLGL3D_MOVE_FORWARD,  SDLGL3D_MOVE_FORWARD },
    { { SDLK_DOWN },  EDITOR_CAMERA, SDLGL3D_MOVE_BACKWARD, SDLGL3D_MOVE_BACKWARD },
    /* -------- Switch the player with given number ------- */
    { { SDLK_ESCAPE }, EDITOR_EXITPROGRAM },
    { { 0 } }

};

static SDLGL_FIELD MainMenu[EDITOR_MAXFLD + 2] = {
    { SDLGL_TYPE_STD,   {   0, 584, 800,  16 } },            /* Status bar       */
    { EDITOR_DRAW2DMAP, { 0, 16, 256, 256 }, EDITOR_2DMAP, EDITOR_2DMAP_CHOOSEFAN }, /* Map-Rectangle    */
    /* 'Code' needed in menu-background' for support of 'mouse-over'    */
    { SDLGL_TYPE_STD,   {   0, 0, 800, 16 }, -1 },           /* Menu-Background  */
    { SDLGL_TYPE_MENU,  {   4, 4, 32, 8 }, EDITOR_FILE,     0, "File" },
    { SDLGL_TYPE_MENU,  {  44, 4, 64, 8 }, EDITOR_SETTINGS, 0, "Settings" },
    { SDLGL_TYPE_MENU,  { 116, 4, 40, 8 }, EDITOR_TOOLS,    0, "Tools" },
    { 0 }

};

/* ------ Sub-Menus ------- */
static SDLGL_FIELD SubMenu[] = {
    /* File-Menu */
    { SDLGL_TYPE_STD,  {   0, 16, 88, 72 }, EDITOR_FILE, -1 },  /* Menu-Background */
    { SDLGL_TYPE_MENU, {   4, 20, 64,  8 }, EDITOR_FILE, EDITOR_FILE_LOAD, "Load..." },
    { SDLGL_TYPE_MENU, {   4, 36, 64,  8 }, EDITOR_FILE, EDITOR_FILE_SAVE, "Save" },
    { SDLGL_TYPE_MENU, {   4, 52, 64,  8 }, EDITOR_FILE, EDITOR_FILE_NEW, "New..." },
    { SDLGL_TYPE_MENU, {   4, 68, 64,  8 }, EDITOR_FILE, EDITOR_FILE_EXIT, "Exit" },   
    /* Settings menu for view */
    { SDLGL_TYPE_STD,  {  40, 16, 136, 56 }, EDITOR_SETTINGS, -1 },   /* Menu-Background */
    { SDLGL_TYPE_MENU, {  44, 20, 128,  8 }, EDITOR_SETTINGS, EDIT_MODE_SOLID,    "[ ] Draw Solid" },
    { SDLGL_TYPE_MENU, {  44, 36, 128,  8 }, EDITOR_SETTINGS, EDIT_MODE_TEXTURED, "[ ] Draw Texture"  },
    { SDLGL_TYPE_MENU, {  44, 52, 128,  8 }, EDITOR_SETTINGS, EDIT_MODE_LIGHTMAX, "[ ] Max Light" },
    /* Tools-Menu */
    { SDLGL_TYPE_STD,  { 116, 16, 128, 16 }, EDITOR_TOOLS, -1 },   /* Menu-Background */
    { SDLGL_TYPE_MENU, { 124, 20, 120,  8 }, EDITOR_TOOLS, EDITOR_TOOL_TILE, "Tile Properties" },
    { 0 }
};

/* Prepared Dialog for changing of Flags of a fan. TODO: Make it dynamic */
static SDLGL_FIELD FanInfoDlg[] = {
    { SDLGL_TYPE_BUTTON,   {   0,  16, 320, 224  }, 0, 0, EditTypeStr },
    { SDLGL_TYPE_STRING,   {   0,  32, 320, 224  }, 0, 0, EditActFanName },
    { SDLGL_TYPE_CHECKBOX, {   8,  56, 120, 8 }, EDITOR_FANFX, MPDFX_SHA,     "Reflective" },
    { SDLGL_TYPE_CHECKBOX, {   8,  72, 120, 8 }, EDITOR_FANFX, MPDFX_DRAWREF, "Draw Reflection" },
    { SDLGL_TYPE_CHECKBOX, {   8,  88, 120, 8 }, EDITOR_FANFX, MPDFX_ANIM,    "Animated" },
    { SDLGL_TYPE_CHECKBOX, {   8, 104, 120, 8 }, EDITOR_FANFX, MPDFX_WATER,   "Overlay (Water)" },
    { SDLGL_TYPE_CHECKBOX, {   8, 120, 120, 8 }, EDITOR_FANFX, MPDFX_WALL,    "Barrier (Slit)" },
    { SDLGL_TYPE_CHECKBOX, {   8, 136, 120, 8 }, EDITOR_FANFX, MPDFX_IMPASS,  "Impassable (Wall)" },
    { SDLGL_TYPE_CHECKBOX, {   8, 154, 120, 8 }, EDITOR_FANFX, MPDFX_DAMAGE,  "Damage" },
    { SDLGL_TYPE_CHECKBOX, {   8, 170, 120, 8 }, EDITOR_FANFX, MPDFX_SLIPPY,  "Slippy" },
    { EDITOR_DRAWTEXTURE,  { 190,  78, 128, 128 }, EDITOR_FANTEX, EDITOR_FANTEX_CHOOSE },  /* The actuale Texture  */
    { SDLGL_TYPE_CHECKBOX, { 190,  62, 128,  16 }, EDITOR_FANTEX, EDITOR_FANTEX_SIZE, "Big Texture"  },
    { SDLGL_TYPE_SLI_AL,   { 190, 210,  16,  16 }, EDITOR_FANTEX, EDITOR_FANTEX_DEC },
    { SDLGL_TYPE_SLI_AR,   { 222, 210,  16,  16 }, EDITOR_FANTEX, EDITOR_FANTEX_INC },
    /* -------- Buttons for 'Cancel' and 'Update' ----- */
    { SDLGL_TYPE_BUTTON,   { 220,  20, 96, 16 }, EDITOR_FANPROPERTY, EDITOR_FANPROPERTY_EDITMODE, "Change Mode"},
    { SDLGL_TYPE_BUTTON,   { 268, 220, 48, 16 }, EDITOR_FANPROPERTY, EDITOR_FANPROPERTY_CLOSE, "Close"},
    { 0 }
};

/* Prepared dialog for setting values for maps */
static SDLGL_FIELD MapDlg[] = {
    { SDLGL_TYPE_BUTTON,   {   0,  16, 240, 112 }, 0, 0, "New Map" },
    { SDLGL_TYPE_EDIT,     {   8,  72,  40,  16 }, EDITOR_MAPDLG, 0, "Size" },
    { SDLGL_TYPE_SLI_AL,   {   8,  72,  16,  16 }, EDITOR_MAPDLG, EDITOR_MAPDLG_DECSIZE },
    { SDLGL_TYPE_SLI_AR,   { 120,  72,  16,  16 }, EDITOR_MAPDLG, EDITOR_MAPDLG_INCSIZE },
    { SDLGL_TYPE_BUTTON,   {   8,  88,  56,  16 }, EDITOR_MAPDLG, EDITOR_MAPDLG_CANCEL, "Cancel"  },
    { SDLGL_TYPE_BUTTON,   { 112,  88,  24,  16 }, EDITOR_MAPDLG, EDITOR_MAPDLG_OK, "Ok" },
    { 0 }
};

/*******************************************************************************
* CODE									                                       *
*******************************************************************************/

/* =================== Map-Settings-Dialog ================ */



/* =================== Main-Screen ======================== */

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

                sdlglInputAdd(which, pstart, 0, 0);

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
static void editorSetToggleChar(unsigned char is_set, char *name)
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
    editorSetToggleChar((char)(pEditState -> draw_mode & EDIT_MODE_SOLID), SubMenu[6].pdata);
    editorSetToggleChar((char)(pEditState -> draw_mode & EDIT_MODE_TEXTURED), SubMenu[6 + 1].pdata);
    editorSetToggleChar((char)(pEditState -> draw_mode & EDIT_MODE_LIGHTMAX), SubMenu[6 + 2].pdata);
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

    switch(event -> sub_code) {

        case EDITOR_2DMAP_CHOOSEFAN:
            if (event -> sdlcode == SDLGL_KEY_MOULEFT) {
                DragRect.w = 0;
                DragRect.h = 0;
                editmainChooseFan(event -> mou.x, event -> mou.y, 0);
            }
            else if (event -> sdlcode == SDLGL_KEY_MOURIGHT) {
                DragRect.w = 0;
                DragRect.h = 0;
                editmainChooseFan(event -> mou.x, event -> mou.y, 1);
            }   /* Choose dragging mouse */
            else if (event -> sdlcode == SDLGL_KEY_MOULDRAG) {
                DragRect.w += event -> mou.w;
                DragRect.h += event -> mou.h;
                DragRect.x = event -> mou.x - DragRect.w;
                DragRect.y = event -> mou.y - DragRect.h;
                editmainChooseFanExt(DragRect.x, DragRect.y,
                                     DragRect.w, DragRect.h);
            }
            else if (event -> sdlcode == SDLGL_KEY_MOURDRAG)  {
                DragRect.w += event -> mou.w;
                DragRect.h += event -> mou.h;
                DragRect.x = event -> mou.x - DragRect.w;
                DragRect.y = event -> mou.y - DragRect.h;
                editmainChooseFanExt(DragRect.x, DragRect.y,
                                     DragRect.w, DragRect.h);
            }
            break;
        case EDITOR_2DMAP_FANINFO:
            if (event -> pressed) {
                Map2DState ^= event -> sub_code;
            }
            /* Add this Dialog at position */
            if (Map2DState & event -> sub_code) {
                sdlglInputAdd(EDITOR_FANDLG, FanInfoDlg, 20, 20);
            }
            else {
                sdlglInputRemove(EDITOR_FANDLG);
            }
            break;
        case EDITOR_2DMAP_FANROTATE:
            editmainMap(EDITMAIN_ROTFAN, 0);
            break;

        case EDITOR_2DMAP_FANBROWSE:
            editmainChooseFanType(+1, EditActFanName);
            break;

    }

}

/*
 * Name:
 *     editorFileMenu
 * Description:
 *     Handles the input of the editors file menu
 * Input:
 *     which: Menu-Point to translate
 * Output:
 *     Result for SDLGL-Handler  
 */
static int editorFileMenu(char which)
{

    int edit_mode;


    switch(which) {
        case EDITOR_FILE_LOAD:
            editmainMap(EDITMAIN_LOADMAP, 0);
            break;
            
        case EDITOR_FILE_SAVE:
            editmainMap(EDITMAIN_SAVEMAP, 0);
            break;

        case EDITOR_FILE_NEW:
            /* TODO: Open Dialog for map size */
            /*
            if (Map2DState & event -> sub_code) {
                sdlglInputAdd(EDITOR_MAPDLG, MapDlg, 20, 20);
            }
            else {
                sdlglInputRemove(EDITOR_FANDLG);
            }
            */
            if (! editmainMap(EDITMAIN_NEWMAP, 0)) {
                /* TODO: Display message, what has gone wrong   */
                /* TODO: Close menu, if menu-point is chosen    */
            }
            else {
                /* Do additional actions */
                editmainChooseFan(1, 1, -1);
                Map2DState |= EDITOR_2DMAP_FANINFO;
                sdlglInputAdd(EDITOR_FANDLG, FanInfoDlg, 20, 20);
                edit_mode = editmainToggleFlag(EDITMAIN_TOGGLE_EDITSTATE, 1);
                sprintf(EditTypeStr, "%s", EditTypeNames[edit_mode]);
                editmainFanTypeName(EditActFanName);
            }
            break;
            
        case EDITOR_FILE_EXIT:
            return SDLGL_INPUT_EXIT;

    }

    return SDLGL_INPUT_OK;

}

/*
 * Name:
 *     editorDrawCheckBox
 * Description:
 *     Draws the push  button with state taken based on fields -> code
 * Input:
 *     field *: Pointer on field to draw
 */
static void editorDrawCheckBox(SDLGL_FIELD *field)
{

    int state;


    switch(field -> code) {

        case EDITOR_FANTEX:
            state = pEditState -> ft.type & 0x20 ? SDLGLSTR_FBUTTONSTATE : 0;
            break;

        case EDITOR_FANFX:
            state = pEditState -> ft.fx & field -> sub_code ? SDLGLSTR_FBUTTONSTATE : 0;
            break;

    }  

    sdlglstrDrawSpecial(&field -> rect, field -> pdata, SDLGL_TYPE_CHECKBOX, state);

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

    /* First move the camera */
    sdlgl3dMoveObjects(event -> secondspassed);

    /* Draw the 3D-View before the 2D-Screen */
    editmainMap(EDITMAIN_DRAWMAP, 0);

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

        switch(fields -> sdlgl_type) {

            case SDLGL_TYPE_STD:
                /* Draw the background of the menu and the status bar */
                sdlglstrDrawSpecial(&fields -> rect, 0, SDLGL_TYPE_STD, 0);
                break;

            case SDLGL_TYPE_STRING:
                sdlglstrSetColorNo(SDLGL_COL_WHITE);
                sdlglstrString(&fields -> rect, fields -> pdata);
                break;
                
            case SDLGL_TYPE_MENU:                    
                if (fields -> fstate & SDLGL_FSTATE_MOUSEOVER) {

                    color = SDLGL_COL_RED;

                    if (fields -> code > 0 && fields -> sub_code == 0) {

                        editorSetMenu(fields -> code);

                    }

                }
                else {

                    color = SDLGL_COL_WHITE;

                }

                sdlglstrSetColorNo(color);
                sdlglstrString(&fields -> rect, fields -> pdata);
                break;

            case EDITOR_DRAW2DMAP:  
                editmainDrawMap2D(fields -> rect.x, fields -> rect.y);
                break;
                
            case EDITOR_DRAWTEXTURE:
                editmain2DTex(fields -> rect.x, fields -> rect.y,
                              fields -> rect.w, fields -> rect.h);
                break;
                
            case SDLGL_TYPE_CHECKBOX:
                editorDrawCheckBox(fields);
                break;
                
            case SDLGL_TYPE_BUTTON:
                sdlglstrDrawButton(&fields -> rect, fields -> pdata, 0);
                break;
                
            case SDLGL_TYPE_SLI_AL:
                sdlglstrDrawSpecial(&fields -> rect, 0, SDLGL_TYPE_SLI_AL, 0);
                break;
                
            case SDLGL_TYPE_SLI_AR:
                sdlglstrDrawSpecial(&fields -> rect, 0, SDLGL_TYPE_SLI_AR, 0);
                break;
                
        }

        fields++;

    }

    /* Draw the string in the status bar */
    sdlglstrDrawButton(&MainMenu[0].rect, StatusBar, 0);

    if (! mouse_over) {

        editorSetMenu(0);

    }

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

    SDLGL_FIELD *field;
    char edit_mode;

    
    if (event -> code > 0) {

        switch(event -> code) {

            case EDITOR_CAMERA:
                /* Move the camera */
                if (event -> pressed) {
                    /* Start movement */                    
                    sdlgl3dManageCamera(0, event -> sub_code, 1, 
                                        event -> modflags & KMOD_LSHIFT ? 3 : 1);
                }
                else {
                    /* Stop movement */
                    sdlgl3dManageCamera(0, event -> sub_code, 0, 1);
                }
                break;

            case EDITOR_FILE:
                return editorFileMenu(event -> sub_code);
                
            case EDITOR_SHOWMAP:
                pEditState -> display_flags ^= EDITMAIN_SHOW2DMAP;
                break;

            case EDITOR_FANFX:
                /* Change the FX of the actual fan-info */
                editmainToggleFlag(EDITMAIN_TOGGLE_FANFX, event -> sub_code);
                break;

            case EDITOR_FANTEX:
                /* Change the Texture of the chosen fan   */
                field = event -> field;
                if (event -> sub_code == EDITOR_FANTEX_CHOOSE) {
                    editmainChooseTex(event -> mou.x, event -> mou.y,
                                      field -> rect.w, field -> rect.h);
                }
                else if (event -> sub_code == EDITOR_FANTEX_SIZE){
                    editmainToggleFlag(EDITMAIN_TOGGLE_FANTEXSIZE, 0);
                }
                else if (event -> sub_code == EDITOR_FANTEX_DEC) {
                    editmainToggleFlag(EDITMAIN_TOGGLE_FANTEXNO, 0xFF);                   
                }
                else if (event -> sub_code == EDITOR_FANTEX_INC) {
                    editmainToggleFlag(EDITMAIN_TOGGLE_FANTEXNO, 0x01);
                }
                break;

            case EDITOR_FANPROPERTY:
                if (event -> sub_code == EDITOR_FANPROPERTY_EDITMODE) {
                    edit_mode = editmainToggleFlag(EDITMAIN_TOGGLE_EDITSTATE, 1);
                    sprintf(EditTypeStr, "%s", EditTypeNames[edit_mode]);
                }                
                else {
                    sdlglInputRemove(EDITOR_FANDLG);
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

            case EDITOR_TOOLS:
                /* Add this Dialog at position */
                Map2DState |= event -> sub_code;
                sdlglInputAdd(EDITOR_FANDLG, FanInfoDlg, 20, 20);
                break;
                
            case EDITOR_EXITPROGRAM:
                return SDLGL_INPUT_EXIT;

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
    pEditState = editmainInit(NewMapSize, 256, 256);
    
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
    /* Edit-Menu */
    sprintf(EditTypeStr, "%s", EditTypeNames[EDITMAIN_EDIT_NONE]);
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


    sdlgl3dInitCamera(0, 310, 0, 90, 0.75);
    /* Init Camera +Z is up, -Y is near plane, X is left/right */
    sdlgl3dMoveToPosCamera(0, 384.0, 384.0, 600.0, 0);

    /* Set the input handlers and do other stuff before  */
    editorStart();

    /* Now enter the mainloop */
    sdlglExecute();

    /* Do any shutdown stuff. */
    editmainExit();

    sdlglShutdown();
    return 0;

}
