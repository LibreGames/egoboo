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
#include "editfile.h"       /* Description of Spawn-Points and passages */

/*******************************************************************************
* DEFINES								                                       *
*******************************************************************************/

#define EDITOR_CAPTION "EGOBOO - Map Editor V 0.1.4"

#define EDITOR_MAXFLD   100

/* --------- The different commands -------- */
#define EDITOR_EXITPROGRAM    ((char)100)

/* Menu commands -- Don't collid with 3D-Commands */
#define EDITOR_FILE     ((char)101)
#define EDITOR_FANFX    ((char)102)
#define EDITOR_TOOLS    ((char)103)         /* Tools for the map    */
#define EDITOR_SETTINGS ((char)104)
#define EDITOR_2DMAP    ((char)105)         /* Displayed map */
#define EDITOR_FANTEX   ((char)106)
#define EDITOR_TOOL_TILE ((char)107)         /* Result of fan dialog         */
#define EDITOR_CAMERA   ((char)108)         /* Movement of camera           */
#define EDITOR_MAPDLG   ((char)109)         /* Settings for new map         */
#define EDITOR_FANPROPERTY ((char)110)      /* Properties of chosen fan(s)  */
#define EDITOR_SHOWMAP     ((char)111)
#define EDITOR_TOOL_PASSAGE  ((char)113)
#define EDITOR_TOOL_OBJECT  ((char)114)
#define EDITOR_3DMAP    ((char)115)
#define EDITOR_DIALOG   ((char)116)         /* 'block_sign' for general dialog   */

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

#define EDITOR_TOOL_OFF         ((char)1)
#define EDITOR_TOOL_WAWALITE    ((char)5)

/* ------------ General dialog codes ---- */
#define EDITOR_DLG_PREV  ((char)1)
#define EDITOR_DLG_NEXT  ((char)2)
#define EDITOR_DLG_NEW   ((char)3)
#define EDITOR_DLG_SAVE  ((char)4)
#define EDITOR_DLG_CLOSE ((char)5)

/* ------- Drawing types ----- */
#define EDITOR_DRAW2DMAP   ((char)SDLGL_TYPE_MENU + 10)
#define EDITOR_DRAWTEXTURE ((char)SDLGL_TYPE_MENU + 11)
#define EDITOR_DRAW3DMAP   ((char)SDLGL_TYPE_MENU + 12)

/*******************************************************************************
* DATA									                                       *
*******************************************************************************/

static SDLGL_RECT DragRect;
static int  EditorMapSize = 32;             /* Inital mapsize for new maps      */
static char EditorWorkDir[256];
static char EditorActDlg = 0;
static char EditorActTool = 0;            /* Number of tool to work with      */
static EDITFILE_PASSAGE_T ActPsg = {    /* Holds data about chosen passage  */
    "Passage-Name"
};
static EDITFILE_SPAWNPT_T ActSpt = {    /* Holds data about chosen spawn point  */
    "Object-Name"
};

static SDLGL_CONFIG SdlGlConfig = {

    EDITOR_CAPTION,
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
    { SDLGLCFG_VAL_INT, &EditorMapSize, "mapsize" },
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
    { EDITOR_DRAW3DMAP, {   0,   0, 800, 600 }, EDITOR_3DMAP }, /* 3D-View      */
    { SDLGL_TYPE_STD,   {   0, 584, 800,  16 } },               /* Status bar   */
    { EDITOR_DRAW2DMAP, {   0, 16, 256, 256 }, EDITOR_2DMAP, EDITOR_2DMAP_CHOOSEFAN }, /* Map-Rectangle    */
    /* 'Code' needed in menu-background' for support of 'mouse-over'    */
    { SDLGL_TYPE_STD,   {   0, 0, 800, 16 }, -1 },          /* Menu-Background  */
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
    { SDLGL_TYPE_STD,  { 116, 16, 128, 72 }, EDITOR_TOOLS, -1 },   /* Menu-Background */
    { SDLGL_TYPE_MENU, { 120, 20, 120,  8 }, EDITOR_TOOLS, EDITOR_TOOL_OFF,     "None / View" },
    { SDLGL_TYPE_MENU, { 120, 36, 120,  8 }, EDITOR_TOOLS, EDITOR_TOOL_TILE,    "Tile" },
    { SDLGL_TYPE_MENU, { 120, 52, 120,  8 }, EDITOR_TOOLS, EDITOR_TOOL_PASSAGE, "Passage" },
    { SDLGL_TYPE_MENU, { 120, 68, 120,  8 }, EDITOR_TOOLS, EDITOR_TOOL_OBJECT,  "Object" },
    { 0 }
};

/* Prepared Dialog for changing of Flags of a fan. TODO: Make it dynamic */
static SDLGL_FIELD FanInfoDlg[] = {
    { SDLGL_TYPE_BUTTON,   {   0,  16, 320, 224  }, 0, 0, EditTypeStr },
    { SDLGL_TYPE_LABEL,   {   0,  32, 320, 224  }, 0, 0, EditActFanName },
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
    { SDLGL_TYPE_BUTTON, {   0,   0, 240, 112 }, 0, 0, "New Map" },
    { SDLGL_TYPE_EDIT,   {   8,  72,  40,  16 }, EDITOR_MAPDLG, 0, "Size" },
    { SDLGL_TYPE_SLI_AL, {   8,  72,  16,  16 }, EDITOR_MAPDLG, EDITOR_MAPDLG_DECSIZE },
    { SDLGL_TYPE_SLI_AR, { 120,  72,  16,  16 }, EDITOR_MAPDLG, EDITOR_MAPDLG_INCSIZE },
    { SDLGL_TYPE_BUTTON, {   8,  88,  56,  16 }, EDITOR_MAPDLG, EDITOR_MAPDLG_CANCEL, "Cancel"  },
    { SDLGL_TYPE_BUTTON, { 112,  88,  24,  16 }, EDITOR_MAPDLG, EDITOR_MAPDLG_OK, "Ok" },
    { 0 }
};

/* Prepared dialog for spawn points and passages */
static SDLGL_FIELD PassageDlg[] = {
    { SDLGL_TYPE_BUTTON, {   0,   0, 280, 150 }, 0, 0, "Passage" },
    { SDLGL_TYPE_LABEL,  {   8,  16,  48,   8 }, 0, 0, "Name:" },
    { SDLGL_TYPE_LABEL,  {  56,  16, 192,   8 }, 0, SDLGL_VAL_STRING, &ActPsg.line_name[0] },
    { SDLGL_TYPE_LABEL,  {   8,  32, 112,   8 }, 0, 0, "Top left:" },
    { SDLGL_TYPE_LABEL,  { 120,  32,  24,   8 }, 0, 0, "X:" },
    { SDLGL_TYPE_VALUE,  { 144,  32,  32,   8 }, 0, SDLGL_VAL_INT, (char *)&ActPsg.topleft[0] },
    { SDLGL_TYPE_LABEL,  { 176,  32,  24,   8 }, 0, 0, "Y:" },
    { SDLGL_TYPE_VALUE,  { 208,  32,  32,   8 }, 0, SDLGL_VAL_INT, (char *)&ActPsg.topleft[1] },
    { SDLGL_TYPE_LABEL,  {   8,  48, 112,   8 }, 0, 0, "Bottom right:" },
    { SDLGL_TYPE_LABEL,  { 120,  48,  24,   8 }, 0, 0, "X:" },
    { SDLGL_TYPE_VALUE,  { 144,  48,  32,   8 }, 0, SDLGL_VAL_INT, (char *)&ActPsg.bottomright[0] },
    { SDLGL_TYPE_LABEL,  { 176,  48,  24,   8 }, 0, 0, "Y:" },
    { SDLGL_TYPE_VALUE,  { 208,  48,  32,   8 }, 0, SDLGL_VAL_INT, (char *)&ActPsg.bottomright[1] },
    { SDLGL_TYPE_LABEL,  {   8,  64,  48,   8 }, 0, 0, "Open:" },
    { SDLGL_TYPE_VALUE,  {  56,  64,  64,   8 }, 0, SDLGL_VAL_ONECHAR, &ActPsg.open },
    { SDLGL_TYPE_LABEL,  {   8,  80, 120,   8 }, 0, 0, "Shoot Through:" },
    { SDLGL_TYPE_VALUE,  { 128,  80,  64,   8 }, 0, SDLGL_VAL_ONECHAR, &ActPsg.shoot_trough },
    { SDLGL_TYPE_LABEL,  {   8,  96, 120,   8 }, 0, 0, "Slippy Close:" },
    { SDLGL_TYPE_VALUE,  { 128,  96,  64,   8 }, 0, SDLGL_VAL_ONECHAR, &ActPsg.slippy_close },
    { SDLGL_TYPE_BUTTON, {   8, 130,  32,  16 }, EDITOR_TOOL_PASSAGE, EDITOR_DLG_NEW, "New"   },
    { SDLGL_TYPE_BUTTON, { 176, 130,  40,  16 }, EDITOR_TOOL_PASSAGE, EDITOR_DLG_SAVE, "Save"  },
    { SDLGL_TYPE_BUTTON, { 224, 130,  48,  16 }, EDITOR_TOOL_PASSAGE, EDITOR_DLG_CLOSE, "Close" },
    { 0 }
};

static SDLGL_FIELD SpawnPtDlg[] = {
    { SDLGL_TYPE_BUTTON, {   0,   0, 350, 260 }, 0, 0, "Object" },
    { SDLGL_TYPE_LABEL,  {   8,  16,  64,   8 }, 0, 0, "Load-Name:" },
    { SDLGL_TYPE_VALUE,  {  96,  16, 192,   8 }, 0, SDLGL_VAL_STRING, &ActSpt.line_name[0] },
    { SDLGL_TYPE_LABEL,  {   8,  32,  64,   8 }, 0, 0, "Game-Name:" },
    { SDLGL_TYPE_VALUE,  {  96,  32,  96,   8 }, 0, SDLGL_VAL_STRING, &ActSpt.item_name[0] },
    { SDLGL_TYPE_LABEL,  {   8,  48,  64,   8 }, 0, 0, "Slot:" },
    { SDLGL_TYPE_VALUE,  {  72,  48,  64,   8 }, 0, SDLGL_VAL_INT, (char *)&ActSpt.slot_no },
    { SDLGL_TYPE_LABEL,  {   8,  64,  80,   8 }, 0, 0, "Position:" },
    { SDLGL_TYPE_LABEL,  {  88,  64,  24,   8 }, 0, 0, "X:" },
    { SDLGL_TYPE_VALUE,  { 112,  64,  64,   8 }, 0, SDLGL_VAL_FLOAT, (char *)&ActSpt.x_pos },
    { SDLGL_TYPE_LABEL,  { 176,  64,  24,   8 }, 0, 0, "Y:" },
    { SDLGL_TYPE_VALUE,  { 200,  64,  64,   8 }, 0, SDLGL_VAL_FLOAT, (char *)&ActSpt.y_pos },
    { SDLGL_TYPE_LABEL,  { 264,  64,  24,   8 }, 0, 0, "Z:" },
    { SDLGL_TYPE_VALUE,  { 288,  64,  64,   8 }, 0, SDLGL_VAL_FLOAT, (char *)&ActSpt.z_pos },
    { SDLGL_TYPE_LABEL,  {   8,  80,  88,   8 }, 0, 0, "Direction:" },
    { SDLGL_TYPE_VALUE,  {  96,  80,  16,   8 }, 0, SDLGL_VAL_ONECHAR, &ActSpt.view_dir },
    { SDLGL_TYPE_LABEL,  {   8,  96,  88,   8 }, 0, 0, "Money:" },  /* 0 .. 9999 */
    { SDLGL_TYPE_VALUE,  {  96,  96,  16,   8 }, 0, SDLGL_VAL_INT, (char *)&ActSpt.money },
    { SDLGL_TYPE_LABEL,  {   8, 112,  88,   8 }, 0, 0, "Skin:" },   /* 0 .. 5 (5: Random) */
    { SDLGL_TYPE_VALUE,  {  96, 112,  16,   8 }, 0, SDLGL_VAL_CHAR, &ActSpt.skin },
    { SDLGL_TYPE_LABEL,  {   8, 128,  88,   8 }, 0, 0, "Passage:" },
    { SDLGL_TYPE_VALUE,  {  96, 128,  16,   8 }, 0, SDLGL_VAL_CHAR, &ActSpt.pas },
    { SDLGL_TYPE_LABEL,  {   8, 144,  88,   8 }, 0, 0, "Content:" },
    { SDLGL_TYPE_VALUE,  {  96, 144,  16,   8 }, 0, SDLGL_VAL_CHAR, &ActSpt.con },
    { SDLGL_TYPE_LABEL,  {   8, 160,  88,   8 }, 0, 0, "Level:" },  /* 0 .. 20 */
    { SDLGL_TYPE_VALUE,  {  96, 160,  16,   8 }, 0, SDLGL_VAL_CHAR, &ActSpt.lvl },
    { SDLGL_TYPE_LABEL,  {   8, 176,  96,   8 }, 0, 0, "Status-Bar:" },
    { SDLGL_TYPE_VALUE,  { 104, 176,  16,   8 }, 0, SDLGL_VAL_ONECHAR, &ActSpt.stt },
    { SDLGL_TYPE_LABEL,  {   8, 192,  96,   8 }, 0, 0, "Ghost:" },
    { SDLGL_TYPE_VALUE,  { 104, 192,  16,   8 }, 0, SDLGL_VAL_ONECHAR, &ActSpt.gho },
    { SDLGL_TYPE_LABEL,  {   8, 208,  96,   8 }, 0, 0, "Team:" },
    { SDLGL_TYPE_VALUE,  { 104, 208,  16,   8 }, 0, SDLGL_VAL_ONECHAR, &ActSpt.team },
    { SDLGL_TYPE_BUTTON, {   8, 240,  32,  16 }, EDITOR_TOOL_OBJECT, EDITOR_DLG_NEW, "New"   },
    { SDLGL_TYPE_BUTTON, { 176, 240,  40,  16 }, EDITOR_TOOL_OBJECT, EDITOR_DLG_SAVE, "Save"  },
    { SDLGL_TYPE_BUTTON, { 224, 240,  48,  16 }, EDITOR_TOOL_OBJECT, EDITOR_DLG_CLOSE, "Close" },
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


    /* -------- Don't handle menu, if a dialog is opened ---- */
    if (EditorActDlg > 0) {
        return;
    }

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
 *     editorSetDialog
 * Description:
 *     Opens/Closes dialog with given number.
 * Input:
 *     which: Which dialog to open/close
 *     open:  Open it yes/no
 */
static void editorSetDialog(char which, char open)
{

    SDLGL_FIELD *dlg;


    if (open) {

        switch(which) {
            case EDITOR_MAPDLG:
                dlg = &MapDlg[0];
                break;
            case EDITOR_TOOL_TILE:
                dlg = &FanInfoDlg[0];
                break;
                /* TODO: Add dialogs for Passages and Spawn Points */
            case EDITOR_TOOL_PASSAGE:
                dlg = &PassageDlg[0];
                break;
            case EDITOR_TOOL_OBJECT:
                dlg = &SpawnPtDlg[0];
                break;
            default:
                return;
        }

        /* Close possible opened menu  */
        editorSetMenu(0);
        /* Now open the dialog */
        sdlglInputAdd(EDITOR_DIALOG, dlg, 20, 20);
        EditorActDlg = EDITOR_DIALOG;

    }
    else {
        sdlglInputRemove(EDITOR_DIALOG);
        EditorActDlg = 0;
    }

}

/* =================== Map-Settings-Dialog ================ */



/* =================== Main-Screen ======================== */

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
 *     editorOpenToolDialog
 * Description:
 *     Opens the tool dialog for given position on map,
 *     depending on actual 'EditToolState'
 * Input:
 *     ti *: Pointer on info about tile
 */
static void editorOpenToolDialog(EDITMAIN_INFO_T *ti)
{

    if (EditorActTool > 0) {

        switch(EditorActTool) {

            case EDITOR_TOOL_TILE:
                /* Only display tiles and 'chosen' tiles */
                break;

            case EDITOR_TOOL_PASSAGE:
                /* Get info about passage from dialog, if available */
                if (ti -> t_number > 0 && (ti -> type & MAP_INFO_PASSAGE)) {

                    editfilePassage(EDITFILE_ACT_GETDATA, ti -> t_number, &ActPsg);
                    editorSetDialog(EditorActTool, 1);

                }
                break;

            case EDITOR_TOOL_OBJECT:
                /* Get info about objects if available */
                if (ti -> t_number > 0 && (ti -> type & MAP_INFO_SPAWN)) {

                    editfileSpawn(EDITFILE_ACT_GETDATA, ti -> t_number, &ActSpt);
                    editorSetDialog(EditorActTool, 1);

                }
                break;

        }

    }

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

    EDITMAIN_INFO_T tile_info;


    switch(event -> sub_code) {

        case EDITOR_2DMAP_CHOOSEFAN:
            editmainGetMiniMapInfo(event -> mou.x, event -> mou.y, &tile_info);

            if (event -> sdlcode == SDLGL_KEY_MOULEFT) {
                DragRect.w = 0;
                DragRect.h = 0;
                editmainChooseFan(event -> mou.x, event -> mou.y, 0);
            }
            else if (event -> sdlcode == SDLGL_KEY_MOURIGHT) {
                DragRect.w = 0;
                DragRect.h = 0;
                editmainChooseFan(event -> mou.x, event -> mou.y, 1);
                /* Check for special info (passages ( spawn points), if switched on */
                editorOpenToolDialog(&tile_info);
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
                editorSetDialog(EDITOR_TOOL_TILE, 1);
            }
            else {
                editorSetDialog(EDITOR_TOOL_TILE, 0);
            }
            break;
        case EDITOR_2DMAP_FANROTATE:
            editmainMap(EDITMAIN_ROTFAN);
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
            editmainMap(EDITMAIN_LOADMAP);
            /* Load additional data for this module */
            editfileSpawn(EDITFILE_ACT_LOAD, 1, &ActSpt);
            editfilePassage(EDITFILE_ACT_LOAD, 1, &ActPsg);

            
            break;

        case EDITOR_FILE_SAVE:
            editmainMap(EDITMAIN_SAVEMAP);
            break;

        case EDITOR_FILE_NEW:
            /* TODO: Open Dialog for map size */
            /*
            if (Map2DState & event -> sub_code) {
                editorSetDialog(EDITOR_MAPDLG, 1);
            }
            else {
                editorSetDialog(EDITOR_MAPDLG, 0);
            }
            */
            if (! editmainMap(EDITMAIN_NEWMAP)) {
                /* TODO: Display message, what has gone wrong   */
                /* TODO: Close menu, if menu-point is chosen    */
            }
            else {
                /* Do additional actions */
                editmainChooseFan(1, 1, -1);
                Map2DState |= EDITOR_2DMAP_FANINFO;
                sdlglInputAdd(EDITOR_TOOL_TILE, FanInfoDlg, 20, 20);
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
 *     editorSpawnDlg
 * Description:
 *     Handles the input of the editors file menu
 * Input:
 *     which: Menu-Point to translate
 * Output:
 *     Result for SDLGL-Handler
 */
static int editorSpawnDlg(SDLGL_EVENT *event)
{

    static int act_rec = 0;
    
    
    switch(event -> sub_code) {
        case EDITOR_DLG_PREV:
            if (act_rec > 1) {
                act_rec--;
            }
            /* --- TODO: Get it from 'editfile...' --- */
            break;
        case EDITOR_DLG_NEXT:
            act_rec++;
            /* --- TODO: Get it from 'editfile...' --- */
            break;
        case EDITOR_DLG_NEW:
            /* --- TODO: Get it from 'editfile...' --- */
            break;
        case EDITOR_DLG_SAVE:
            break;
        case EDITOR_DLG_CLOSE:
            editorSetDialog(0, 0);
            break;
    }
    return SDLGL_INPUT_OK;

}

/*
 * Name:
 *     editorPassageDlg
 * Description:
 *     Handles the input of the editors file menu
 * Input:
 *     which: Menu-Point to translate
 * Output:
 *     Result for SDLGL-Handler
 */
static int editorPassageDlg(SDLGL_EVENT *event)
{

    static int act_rec = 0;


    switch(event -> sub_code) {
        case EDITOR_DLG_PREV:
            if (act_rec > 1) {
                act_rec--;
            }
            /* --- TODO: Get it from 'editfile...' --- */
            break;
        case EDITOR_DLG_NEXT:
            act_rec++;
            /* --- TODO: Get it from 'editfile...' --- */
            break;
        case EDITOR_DLG_NEW:
            /* --- TODO: Get it from 'editfile...' --- */
            break;
        case EDITOR_DLG_SAVE:
            /* --- TODO: Get it from 'editfile...' --- */
            break;
        case EDITOR_DLG_CLOSE:
            editorSetDialog(0, 0);
            break;
    }

    return SDLGL_INPUT_OK;

}

/*
 * Name:
 *     editorMenuTool
 * Description:
 *     Opens the tool dialog for given 'fan_no', depending on actual
 *     'EditToolState'
 * Input:
 *     event *: Event from menu
 */
static void editorMenuTool(SDLGL_EVENT *event)
{

    int rec_no;


    /* Set the number of tool to use, invoked by right click on map */
    EditorActTool = event -> sub_code;

    if (event -> field -> pdata) {
        /* Display it */
        sprintf(StatusBar, "Edit-State: %s", event -> field -> pdata);

    }

    if (EditorActTool == EDITOR_TOOL_OFF)
    {
        /* Close actual active dialog, if any */
        editorSetDialog(0, 0);
    }
    else {

        editmainClearMapInfo();

        switch(EditorActTool) {
        
            case EDITOR_TOOL_TILE:
                break;

            case EDITOR_TOOL_PASSAGE:
                /* Write the data about passages to edit-map  */
                rec_no = 1;
                while(editfilePassage(EDITFILE_ACT_GETDATA, rec_no, &ActPsg))
                {
                    editmainSetMapInfo(MAP_INFO_PASSAGE,
                                       rec_no,
                                       ActPsg.topleft[0],
                                       ActPsg.topleft[1],
                                       ActPsg.bottomright[0],
                                       ActPsg.bottomright[1]);
                    rec_no++;
                }
                break;
            case EDITOR_TOOL_OBJECT:
                /* Write the data about objects points to edit-map  */
                rec_no = 1;
                while(editfileSpawn(EDITFILE_ACT_GETDATA, rec_no, &ActSpt))
                {
                    editmainSetMapInfo(MAP_INFO_SPAWN,
                                       rec_no,
                                       ActSpt.x_pos,
                                       ActSpt.y_pos,
                                       ActSpt.x_pos,
                                       ActSpt.y_pos);
                    rec_no++;
                }
                break;

        }

    }

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

    glClear(GL_COLOR_BUFFER_BIT);

    /* First move the camera */
    sdlgl3dMoveObjects(event -> secondspassed);

    /* Draw the 3D-View before the 2D-Screen */
    editmainMap(EDITMAIN_DRAWMAP);

    /* ---- Prepare drawing in 2D-Mode ---- */
    glPolygonMode(GL_FRONT, GL_FILL);
    glShadeModel(GL_FLAT);		            /* No smoothing	of edges    */
    glFrontFace(GL_CCW);                    /* Draws counterclockwise   */

    sdlglstrSetFont(SDLGLSTR_FONT8);

    while (fields -> sdlgl_type > 0) {

        if (fields -> fstate & SDLGL_FSTATE_MOUSEOVER) {

            /* Draw the menu -- open/close dropdown, if needed */
            if (fields -> code == EDITOR_3DMAP) {

                editorSetMenu(0);

            }
            else {

                if (fields -> sdlgl_type == SDLGL_TYPE_MENU
                    && fields -> code > 0
                    && fields -> sub_code == 0) {

                    editorSetMenu(fields -> code);

                }

            }

        }

        if (! sdlglstrDrawField(fields)) {
        
            switch(fields -> sdlgl_type) {                         

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

                case SDLGL_TYPE_SLI_AL:
                    sdlglstrDrawSpecial(&fields -> rect, 0, SDLGL_TYPE_SLI_AL, 0);
                    break;

                case SDLGL_TYPE_SLI_AR:
                    sdlglstrDrawSpecial(&fields -> rect, 0, SDLGL_TYPE_SLI_AR, 0);
                    break;

            }

        }

        fields++;

    }

    /* Draw the string in the status bar */
    sdlglstrDrawButton(&MainMenu[1].rect, StatusBar, 0);

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
                    sdlglInputRemove(EDITOR_TOOL_TILE);
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

            case EDITOR_MAPDLG:
                /* TODO: Handle map dialog input */
                break;
            case EDITOR_TOOL_PASSAGE:
                /* Handle passage dialog input */
                editorPassageDlg(event);
                break;
            case EDITOR_TOOL_OBJECT:
                /* Handle spawn point dialog input */
                editorSpawnDlg(event);
                break;
            case EDITOR_TOOLS:
                editorMenuTool(event);
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
    pEditState = editmainInit(EditorMapSize, 256, 256);
    
    /* Display initally chosen values */    
    editorSetMenuViewToggle();
    /* Get the screen size for the 3D-View  */
    sdlglAdjustRectToScreen(&MainMenu[0].rect,
                            0,
                            (SDLGL_FRECT_SCRWIDTH | SDLGL_FRECT_SCRHEIGHT));
    /* -------- Adjust the menu bar sizes to screen ------- */
    /* Status-Bar   */
    sdlglAdjustRectToScreen(&MainMenu[1].rect,
                            0,
                            (SDLGL_FRECT_SCRWIDTH | SDLGL_FRECT_SCRBOTTOM));
    /* Map-Rectangle    */
    sdlglAdjustRectToScreen(&MainMenu[2].rect,
                            0,
                            SDLGL_FRECT_SCRRIGHT);
    /* Menu-Bar     */
    sdlglAdjustRectToScreen(&MainMenu[3].rect,
                            0,
                            SDLGL_FRECT_SCRWIDTH);
    /* Edit-Menu */
    sprintf(EditTypeStr, "%s", EditTypeNames[EDITMAIN_EDIT_NONE]);
    /* -------- Initialize the 3D-Stuff --------------- */
    sdlgl3dInitCamera(0, 310, 0, 90, 0.75);
    /* Init Camera +Z is up, -Y is near plane, X is left/right */
    sdlgl3dMoveToPosCamera(0, 384.0, 384.0, 600.0, 0);
    
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

    sdlglInit(&SdlGlConfig);    

    /* Set the input handlers and do other stuff before  */
    editorStart();

    /* Now enter the mainloop */
    sdlglExecute();

    /* Do any shutdown stuff. */
    editmainExit();

    sdlglShutdown();
    return 0;

}
