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
#include <string.h>     /* strcpy()     */
#include <memory.h>     /* memset()     */


/* ----- Own headers --- */
#include "sdlgl.h"          /* OpenGL and SDL-Stuff             */
#include "sdlglstr.h"
#include "sdlglcfg.h"
#include "sdlgl3d.h"
#include "sdlglfld.h"       /* Handle 'Standard-Fields          */
#include "editor.h"
#include "editmain.h"
#include "editfile.h"       /* Description of Spawn-Points and passages */

/*******************************************************************************
* DEFINES								                                       *
*******************************************************************************/

#define EDITOR_CAPTION "EGOBOO - Map Editor V 0.1.7"

#define EDITOR_MAXFLD   100

/* --------- The different commands -------- */
#define EDITOR_EXITPROGRAM    ((char)100)

/* Menu commands -- Don't collide with 3D-Commands */
#define EDITOR_FILE     ((char)10)
#define EDITOR_SETTINGS ((char)11)
#define EDITOR_TOOLS    ((char)12)          /* Tools for the map    */
/* === Special dialog main functions === */
#define EDITOR_CAMERA   ((char)13)          /* Move camera           */
#define EDITOR_SHOWMAP  ((char)14)
#define EDITOR_FANFX    ((char)15)
#define EDITOR_FANTEX   ((char)16)
#define EDITOR_FANPROPERTY  ((char)17)       /* Properties of chosen fan(s)  */

#define EDITOR_MODULEDLG    ((char)18)  /* Settings for new map         */
#define EDITOR_2DMAP        ((char)19)  /* Displayed map                */
#define EDITOR_3DMAP        ((char)20)
#define EDITOR_DIALOG       ((char)21)  /* 'block_sign' for general dialog      */
#define EDITOR_INVENTORY    ((char)22)  /* A players inventory from spawn point */
#define EDITOR_CHOOSE_NEW   ((char)23)  /* Ask for something new on map         */

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
#define EDITOR_FANTEX_DEC       ((char)2)
#define EDITOR_FANTEX_INC       ((char)3)

#define EDITOR_FANPROPERTY_EDITMODE ((char)1)   /* Changes the edit-mode    */
#define EDITOR_FANPROPERTY_UPDATE   ((char)2)   /* Depending on actual mode */
#define EDITOR_FANPROPERTY_CLOSE    ((char)3)   /* Close properties dialog  */

#define EDITOR_MODULEDLG_DECSIZE   ((char)1)
#define EDITOR_MODULEDLG_INCSIZE   ((char)2)
#define EDITOR_MODULEDLG_MODTYPE   ((char)3)
#define EDITOR_MODULEDLG_RATEDEC   ((char)4)
#define EDITOR_MODULEDLG_RATEINC   ((char)5)
#define EDITOR_MODULEDLG_CANCEL    ((char)6)
#define EDITOR_MODULEDLG_OK        ((char)7)

#define EDITOR_INVENTORY_LEFT      ((char)1)
#define EDITOR_INVENTORY_RIGHT     ((char)2)
#define EDITOR_INVENTORY_1         ((char)3)
#define EDITOR_INVENTORY_2         ((char)4)
#define EDITOR_INVENTORY_3         ((char)5) 
#define EDITOR_INVENTORY_4         ((char)6)
#define EDITOR_INVENTORY_5         ((char)7)
#define EDITOR_INVENTORY_6         ((char)8)
#define EDITOR_INVENTORY_CLOSE     ((char)9) 

/* ------------ General dialog codes ---- */
#define EDITOR_DLG_NEW          ((char)1)
#define EDITOR_DLG_DELETE       ((char)2)
#define EDITOR_DLG_SAVE         ((char)3)
#define EDITOR_DLG_CLOSE        ((char)4)
#define EDITOR_DLG_INVENTORY    ((char)5)

/* ------- Drawing types ----- */
#define EDITOR_DRAW2DMAP   ((char)SDLGL_TYPE_MENU + 10)
#define EDITOR_DRAWTEXTURE ((char)SDLGL_TYPE_MENU + 11)
#define EDITOR_DRAW3DMAP   ((char)SDLGL_TYPE_MENU + 12)
#define EDITOR_DRAWICON    ((char)SDLGL_TYPE_MENU + 13)

/*******************************************************************************
* DATA									                                       *
*******************************************************************************/

static EDITMAIN_INFO_T EditInfo;      /* Actual state of editor, and its info */

static char EditorWorkDir[256];
static char EditorActDlg  = 0;

static EDITFILE_PASSAGE_T ActPsg;       /* Holds data about chosen passage      */
static EDITFILE_SPAWNPT_T ActSpt;       /* Holds data about chosen spawn point  */
static EDITFILE_MODULE_T ModuleDesc;    /* Holds data about actual module desc  */

/* ============= Declaration of configuration data ================= */

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

    { SDLGLCFG_VAL_INT, &SdlGlConfig.scrwidth, 4, "scrwidth" },
    { SDLGLCFG_VAL_INT, &SdlGlConfig.scrheight, 4, "scrheight" },
    { SDLGLCFG_VAL_INT, &SdlGlConfig.colordepth, 2, "colordepth" },
    { SDLGLCFG_VAL_INT, &SdlGlConfig.screenmode, 2, "screenmode" },
    { SDLGLCFG_VAL_INT, &SdlGlConfig.debugmode, 1, "debugmode" },
    { SDLGLCFG_VAL_INT, &EditInfo.map_size, 2, "mapsize" },
    { SDLGLCFG_VAL_STRING, &EditorWorkDir[0], 120, "workdir"  },
    { 0 }

};

static char StatusBar[80];
static char Map2DState;         /* EDITOR_2DMAP_ */
static char EditTypeStr[30];    /* Actual string    */

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

/* ============= Declaration of menues ================= */

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
    { SDLGL_TYPE_MENU, {   4, 52, 64,  8 }, EDITOR_FILE, EDITOR_FILE_NEW,  "New..." },
    { SDLGL_TYPE_MENU, {   4, 68, 64,  8 }, EDITOR_FILE, EDITOR_FILE_EXIT, "Exit" },
    /* Settings menu for view */
    { SDLGL_TYPE_STD,  {  40, 16, 156, 56 }, EDITOR_SETTINGS, -1 },   /* Menu-Background */
    { SDLGL_TYPE_MENU, {  44, 20, 148,  8 }, EDITOR_SETTINGS, EDIT_DRAWMODE_SOLID,    "Draw Solid" },
    { SDLGL_TYPE_MENU, {  44, 36, 148,  8 }, EDITOR_SETTINGS, EDIT_DRAWMODE_TEXTURED, "Draw Texture"  },
    { SDLGL_TYPE_MENU, {  44, 52, 148,  8 }, EDITOR_SETTINGS, EDIT_DRAWMODE_LIGHTMAX, "Max Light" },
    /* Tools-Menu */
    { SDLGL_TYPE_STD,  { 116,  16, 128, 136 }, EDITOR_TOOLS, -1 },   /* Menu-Background */
    { SDLGL_TYPE_MENU, { 120,  20, 120,   8 }, EDITOR_TOOLS, EDITOR_TOOL_OFF,     "None / View" },
    { SDLGL_TYPE_MENU, { 120,  36, 120,   8 }, EDITOR_TOOLS, EDITOR_TOOL_MAP,     "Map" },
    { SDLGL_TYPE_MENU, { 120,  52, 120,   8 }, EDITOR_TOOLS, EDITOR_TOOL_FAN,     "Tile" },
    { SDLGL_TYPE_MENU, { 120,  68, 120,   8 }, EDITOR_TOOLS, EDITOR_TOOL_FAN_FX,  "Tile-Fx" },
    { SDLGL_TYPE_MENU, { 120,  84, 120,   8 }, EDITOR_TOOLS, EDITOR_TOOL_FAN_TEX, "Tile-Texture" },      
    { SDLGL_TYPE_MENU, { 120, 100, 120,   8 }, EDITOR_TOOLS, EDITOR_TOOL_PASSAGE, "Passage" },
    { SDLGL_TYPE_MENU, { 120, 116, 120,   8 }, EDITOR_TOOLS, EDITOR_TOOL_OBJECT,  "Object" },
    { SDLGL_TYPE_MENU, { 120, 132, 120,   8 }, EDITOR_TOOLS, EDITOR_TOOL_MODULE,  "Main-Info" },
    { 0 }
};

/* === Dialog: Edit properties of a fan === */
static SDLGL_FIELD FanInfoDlg[] = {
    { SDLGL_TYPE_BUTTON,   {   0,  16, 320, 224  }, 0, 0, EditTypeStr },
    { SDLGL_TYPE_LABEL,    {   0,  32, 320, 224  }, 0, 0, EditInfo.fan_name },
	/* Use copy of actual fan descriptor */
	{ SDLGL_TYPE_CHECKBOX, {   8,  56, 120, 8 }, EDITOR_FANFX, MPDFX_SHA, (char *)&EditInfo.ft.fx },
    { SDLGL_TYPE_LABEL,    {  24,  56, 120, 8 }, 0, 0, "Reflective" },
    { SDLGL_TYPE_CHECKBOX, {   8,  72, 120, 8 }, EDITOR_FANFX, MPDFX_DRAWREF, (char *)&EditInfo.ft.fx },
    { SDLGL_TYPE_LABEL,    {  24,  72, 120, 8 }, 0, 0, "Draw Reflection" },
    { SDLGL_TYPE_CHECKBOX, {   8,  88, 120, 8 }, EDITOR_FANFX, MPDFX_ANIM, (char *)&EditInfo.ft.fx },
    { SDLGL_TYPE_LABEL,    {  24,  88, 120, 8 }, 0, 0, "Animated" },
    { SDLGL_TYPE_CHECKBOX, {   8, 104, 120, 8 }, EDITOR_FANFX, MPDFX_WATER, (char *)&EditInfo.ft.fx },
    { SDLGL_TYPE_LABEL,    {  24, 104, 120, 8 }, 0, 0, "Overlay (Water)" },
    { SDLGL_TYPE_CHECKBOX, {   8, 120, 120, 8 }, EDITOR_FANFX, MPDFX_WALL, (char *)&EditInfo.ft.fx },
    { SDLGL_TYPE_LABEL,    {   8, 120, 120, 8 }, 0, 0, "Barrier (Slit)" },
    { SDLGL_TYPE_CHECKBOX, {   8, 136, 120, 8 }, EDITOR_FANFX, MPDFX_IMPASS, (char *)&EditInfo.ft.fx },
    { SDLGL_TYPE_LABEL,    {   8, 136, 120, 8 }, 0, 0, "Impassable (Wall)" },
    { SDLGL_TYPE_CHECKBOX, {   8, 154, 120, 8 }, EDITOR_FANFX, MPDFX_DAMAGE, (char *)&EditInfo.ft.fx },
    { SDLGL_TYPE_LABEL,    {   8, 154, 120, 8 }, 0, 0, "Damage" },
    { SDLGL_TYPE_CHECKBOX, {   8, 170, 120, 8 }, EDITOR_FANFX, MPDFX_SLIPPY, (char *)&EditInfo.ft.fx },
    { SDLGL_TYPE_LABEL,    {   8, 170, 120, 8 }, 0, 0, "Slippy" },
    { EDITOR_DRAWTEXTURE,  { 190,  78, 128, 128 }, EDITOR_FANTEX, EDITOR_FANTEX_CHOOSE },  /* The actuale Texture  */
    { SDLGL_TYPE_CHECKBOX, { 190,  62, 128,  16 }, EDITOR_FANTEX, 0x20, (char *)&EditInfo.ft.type },
    { SDLGL_TYPE_LABEL,    { 214,  62, 128,  16 }, 0, 0, "Big Texture"  },
    { SDLGL_TYPE_SLI_AL,   { 190, 210,  16,  16 }, EDITOR_FANTEX, EDITOR_FANTEX_DEC },
    { SDLGL_TYPE_SLI_AR,   { 222, 210,  16,  16 }, EDITOR_FANTEX, EDITOR_FANTEX_INC },
    /* -------- Buttons for 'Cancel' and 'Update' ----- */
    { SDLGL_TYPE_BUTTON,   { 220,  20, 96, 16 }, EDITOR_FANPROPERTY, EDITOR_FANPROPERTY_EDITMODE, "Change Mode"},
    { SDLGL_TYPE_BUTTON,   { 268, 220, 48, 16 }, EDITOR_FANPROPERTY, EDITOR_FANPROPERTY_CLOSE, "Close"},
    { 0 }
};

/* === Dialog: Edit a passage === */
static SDLGL_FIELD PassageDlg[] = {
    { SDLGL_TYPE_BUTTON, {   0,   0, 280, 134 }, 0, 0, "Passage" },
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
    { SDLGL_TYPE_BUTTON, {   8, 114,  32,  16 }, EDITOR_TOOL_PASSAGE, EDITOR_DLG_NEW,   "New"   },
    { SDLGL_TYPE_BUTTON, {  48, 114,  56,  16 }, EDITOR_TOOL_PASSAGE, EDITOR_DLG_DELETE, "Delete"   },
    { SDLGL_TYPE_BUTTON, { 176, 114,  40,  16 }, EDITOR_TOOL_PASSAGE, EDITOR_DLG_SAVE, "Save"  },
    { SDLGL_TYPE_BUTTON, { 224, 114,  48,  16 }, EDITOR_TOOL_PASSAGE, EDITOR_DLG_CLOSE, "Close" },
    { 0 }
};

/* === Dialog: Edit a spawn point === */
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
    { SDLGL_TYPE_LABEL,  {   8, 176,  96,   8 }, 0, 0, "Is Player:" },
    { SDLGL_TYPE_CHECKBOX,  {  96, 176,  16,   8 }, 0x01, 0, &ActSpt.stt },
    { SDLGL_TYPE_LABEL,  {   8, 192,  96,   8 }, 0, 0, "Team:" },
    { SDLGL_TYPE_VALUE,  {  96, 192,  16,   8 }, 0, SDLGL_VAL_ONECHAR, &ActSpt.team },
    { SDLGL_TYPE_BUTTON, {   8, 224,  32,  16 }, EDITOR_TOOL_OBJECT, EDITOR_DLG_NEW, "New"   },
    { SDLGL_TYPE_BUTTON, {  48, 224,  56,  16 }, EDITOR_TOOL_OBJECT, EDITOR_DLG_DELETE, "Delete"   },
    { SDLGL_TYPE_BUTTON, { 176, 224,  40,  16 }, EDITOR_TOOL_OBJECT, EDITOR_DLG_SAVE, "Save"  },
    { SDLGL_TYPE_BUTTON, { 224, 224,  48,  16 }, EDITOR_TOOL_OBJECT, EDITOR_DLG_CLOSE, "Close" },
    { SDLGL_TYPE_BUTTON, { 266,   4,  80,  16 }, EDITOR_TOOL_OBJECT, EDITOR_DLG_INVENTORY, "Inventory" },
    { 0 }
};

/* === Dialog: Edit the inventory of an object === */
static SDLGL_FIELD DlgInventory[] = {   /* EDITOR_INVENTORY */  /*  */
    { SDLGL_TYPE_BUTTON, {   0,   0,  76, 136 }, 0, 0, "Inventory" },
    /* TODO: Add fields with type 'EDITOR_DRAWICON' or draw icons as 'overlay' */
    { SDLGL_TYPE_BUTTON, {   4,  20,  16,  16 }, EDITOR_INVENTORY, EDITOR_INVENTORY_LEFT },
    { SDLGL_TYPE_BUTTON, {  52,  20,  16,  16 }, EDITOR_INVENTORY, EDITOR_INVENTORY_RIGHT },
    { SDLGL_TYPE_BUTTON, {   4,  44,  16,  16 }, EDITOR_INVENTORY, EDITOR_INVENTORY_1 },
    { SDLGL_TYPE_BUTTON, {  28,  44,  16,  16 }, EDITOR_INVENTORY, EDITOR_INVENTORY_2 },
    { SDLGL_TYPE_BUTTON, {  52,  44,  16,  16 }, EDITOR_INVENTORY, EDITOR_INVENTORY_3 },
    { SDLGL_TYPE_BUTTON, {   4,  68,  16,  16 }, EDITOR_INVENTORY, EDITOR_INVENTORY_4 },
    { SDLGL_TYPE_BUTTON, {  28,  68,  16,  16 }, EDITOR_INVENTORY, EDITOR_INVENTORY_5 },
    { SDLGL_TYPE_BUTTON, {  52,  68,  16,  16 }, EDITOR_INVENTORY, EDITOR_INVENTORY_6 },
    { SDLGL_TYPE_BUTTON, {  52,  68,  16,  16 }, EDITOR_INVENTORY, EDITOR_INVENTORY_6 },
    { SDLGL_TYPE_BUTTON, {  84, 116,  48,  16 }, EDITOR_INVENTORY, EDITOR_INVENTORY_CLOSE, "Close" },    
    { 0 }
};

/* === Dialog: Edit the base values and module description of a map === */

static char *QuestNames[] = {
    "MAINQUEST", 
    "SIDEQUEST",
    "TOWN",
    ""
};

static SDLGL_FIELD ModuleDlg[] = {
    { SDLGL_TYPE_BUTTON, {   0,   0, 380, 560 }, 0, 0, "Module description" },
    /* ---- Size of modul-map --- */
    { SDLGL_TYPE_LABEL,  {   8,  16,  80,  16 }, EDITOR_MODULEDLG, 0, "Map-Size:" },
    { SDLGL_TYPE_SLI_AL, {  92,  16,  16,  16 }, EDITOR_MODULEDLG, EDITOR_MODULEDLG_DECSIZE },
    { SDLGL_TYPE_VALUE,  { 112,  20,  16,   8 }, 0, SDLGL_VAL_INT, (char *)&EditInfo.map_size },
    { SDLGL_TYPE_SLI_AR, { 132,  16,  16,  16 }, EDITOR_MODULEDLG, EDITOR_MODULEDLG_INCSIZE },
    /* --- Description itself --- */
    { SDLGL_TYPE_LABEL,  {   8,  44,  48,   8 }, 0, 0, "Name:" },
    { SDLGL_TYPE_EDIT,   {  64,  40, 200,  16 }, 24, SDLGL_VAL_STRING, &ModuleDesc.mod_name[0] },
    { SDLGL_TYPE_LABEL,  {   8,  64, 160,   8 }, 0, 0, "Referenced Module:" },
    { SDLGL_TYPE_EDIT,   { 176,  60, 200,  16 }, 24, SDLGL_VAL_STRING, &ModuleDesc.ref_mod[0] },
    { SDLGL_TYPE_LABEL,  {   8,  84, 160,   8 }, 0, 0, "Reference IDSZ:" },
    { SDLGL_TYPE_EDIT,   { 176,  80,  72,  16 }, 11, SDLGL_VAL_STRING, &ModuleDesc.ref_idsz[0] },
    { SDLGL_TYPE_LABEL,  {   8, 104, 160,   8 }, 0, 0, "Number of Imports:" },
    { SDLGL_TYPE_EDIT,   { 176, 100,  16,  16 }, 1, SDLGL_VAL_CHAR, &ModuleDesc.number_of_imports },
    { SDLGL_TYPE_LABEL,  {   8, 124, 160,   8 }, 0, 0, "Allow Export:" },
    { SDLGL_TYPE_EDIT,   { 176, 120,  16,  16 }, 1, SDLGL_VAL_ONECHAR, &ModuleDesc.allow_export },
    { SDLGL_TYPE_LABEL,  {   8, 144, 160,   8 }, 0, 0, "Min. Players:" },
    { SDLGL_TYPE_EDIT,   { 176, 140,  16,  16 }, 1, SDLGL_VAL_CHAR, &ModuleDesc.min_player },
    { SDLGL_TYPE_LABEL,  {   8, 164, 160,   8 }, 0, 0, "Max. Players:" },
    { SDLGL_TYPE_EDIT,   { 176, 160,  16,  16 }, 1, SDLGL_VAL_CHAR, &ModuleDesc.max_player },
    { SDLGL_TYPE_LABEL,  {   8, 184, 160,   8 }, 0, 0, "Allow respawn:" },
    { SDLGL_TYPE_CHECKBOX, { 176, 180,  16,  16 }, 0, 1, &ModuleDesc.allow_respawn },
    { SDLGL_TYPE_LABEL,  {   8, 204, 160,   8 }, 0, 0, "Module Type:" },
    { SDLGL_TYPE_LABEL,  { 128, 204, 160,   8 }, 0, 0, &ModuleDesc.mod_type[0] },
    { SDLGL_TYPE_SLI_AR, { 224, 200,  16,  16 }, EDITOR_MODULEDLG, EDITOR_MODULEDLG_MODTYPE },
    { SDLGL_TYPE_LABEL,  {   8, 224, 112,   8 }, 0, 0, "Level Rating:" },
    { SDLGL_TYPE_SLI_AL, { 120, 220,  16,  16 }, EDITOR_MODULEDLG, EDITOR_MODULEDLG_RATEDEC },
    { SDLGL_TYPE_LABEL,  { 144, 224,  72,  16 }, 0, 0, &ModuleDesc.lev_rating[0] },
    { SDLGL_TYPE_SLI_AR, { 224, 220,  16,  16 }, EDITOR_MODULEDLG, EDITOR_MODULEDLG_RATEINC },
    { SDLGL_TYPE_LABEL,  {   8, 240, 112,   8 }, 0, 0, "Module Summary:" },
    { SDLGL_TYPE_EDIT,   {   8, 260, 328,  16 }, 40, SDLGL_VAL_STRING, ModuleDesc.summary[0] },
    { SDLGL_TYPE_EDIT,   {   8, 280, 328,  16 }, 40, SDLGL_VAL_STRING, ModuleDesc.summary[1] },
    { SDLGL_TYPE_EDIT,   {   8, 300, 328,  16 }, 40, SDLGL_VAL_STRING, ModuleDesc.summary[2] },
    { SDLGL_TYPE_EDIT,   {   8, 320, 328,  16 }, 40, SDLGL_VAL_STRING, ModuleDesc.summary[3] },
    { SDLGL_TYPE_EDIT,   {   8, 340, 328,  16 }, 40, SDLGL_VAL_STRING, ModuleDesc.summary[4] },
    { SDLGL_TYPE_EDIT,   {   8, 360, 328,  16 }, 40, SDLGL_VAL_STRING, ModuleDesc.summary[5] },
    { SDLGL_TYPE_EDIT,   {   8, 380, 328,  16 }, 40, SDLGL_VAL_STRING, ModuleDesc.summary[6] },
    { SDLGL_TYPE_EDIT,   {   8, 400, 328,  16 }, 40, SDLGL_VAL_STRING, ModuleDesc.summary[7] },
    { SDLGL_TYPE_LABEL,  {   8, 420,  96,   8 }, 0, 0, "Expansions:" },
    /* ---- Maximum five expansions --- */
    { SDLGL_TYPE_EDIT,   {   8, 440, 152,  16 }, 18, SDLGL_VAL_STRING, ModuleDesc.exp_idsz[0] },
    { SDLGL_TYPE_EDIT,   {   8, 460, 152,  16 }, 18, SDLGL_VAL_STRING, ModuleDesc.exp_idsz[1] },
    { SDLGL_TYPE_EDIT,   {   8, 480, 152,  16 }, 18, SDLGL_VAL_STRING, ModuleDesc.exp_idsz[2] },
    { SDLGL_TYPE_EDIT,   {   8, 500, 152,  16 }, 18, SDLGL_VAL_STRING, ModuleDesc.exp_idsz[3] },
    { SDLGL_TYPE_EDIT,   {   8, 520, 152,  16 }, 18, SDLGL_VAL_STRING, ModuleDesc.exp_idsz[4] },
    /* ------- Buttons ------ */
    { SDLGL_TYPE_BUTTON, {   8, 540,  56,  16 }, EDITOR_MODULEDLG, EDITOR_MODULEDLG_CANCEL, "Cancel"  },
    { SDLGL_TYPE_BUTTON, { 302, 540,  24,  16 }, EDITOR_MODULEDLG, EDITOR_MODULEDLG_OK, "Ok" },
    { 0 }
};

/* === Dialog: Ask for something new on map === */
static SDLGL_FIELD DlgNew[] = {
    { SDLGL_TYPE_STD,   {   0, 0, 800, 16 }, -1 },          /* Menu-Background  */
    { SDLGL_TYPE_MENU,  {   4, 4, 32, 8 }, EDITOR_FILE,     0, "File" },
    { SDLGL_TYPE_MENU,  {  44, 4, 64, 8 }, EDITOR_SETTINGS, 0, "Settings" },
    { SDLGL_TYPE_MENU,  { 116, 4, 40, 8 }, EDITOR_TOOLS,    0, "Tools" },
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
 *     editorSetMiniMap
 * Description:
 *     Sets size and position of minimap, depending on given map size
 * Input:
 *     map_size: Size of map in tiles x/y (tile-size is always 7)
*/
static void editorSetMiniMap(int map_size)
{

    /* Map-Rectangle    */
    MainMenu[2].rect.w = map_size * 7;
    MainMenu[2].rect.h = map_size * 7;
    sdlglAdjustRectToScreen(&MainMenu[2].rect, 0, SDLGL_FRECT_SCRRIGHT);

}

/*
 * Name:
 *     editorSetDialog
 * Description:
 *     Opens/Closes dialog with given number.
 * Input:
 *     which: Which dialog to open/close
 *     open:  Open it yes/no
 *     x, y:  Position on screen for dialog
 */
static void editorSetDialog(char which, char open, int x, int y)
{

    SDLGL_FIELD *dlg;


    if (open) {

        switch(which) {
            case EDITOR_TOOL_MODULE:
            case EDITOR_MODULEDLG:
                dlg = &ModuleDlg[0];
                break;
            case EDITOR_TOOL_FAN:
                dlg = &FanInfoDlg[0];
                break;
                /* TODO: Add dialogs for Passages and Spawn Points */
            case EDITOR_TOOL_PASSAGE:
                dlg = &PassageDlg[0];
                break;
            case EDITOR_TOOL_OBJECT:
                dlg = &SpawnPtDlg[0];
                break;

            case EDITOR_INVENTORY:
                dlg = &DlgInventory[0];
                break;

            case EDITOR_CHOOSE_NEW:
                dlg = &DlgNew[0];
                break;

            default:
                return;
        }

        /* Close possible opened menu  */
        editorSetMenu(0);
        /* Now open the dialog */
        sdlglInputAdd(EDITOR_DIALOG, dlg, x, y);
        EditorActDlg = EDITOR_DIALOG;

    }
    else {
        sdlglInputRemove(EDITOR_DIALOG);
        EditorActDlg = 0;
    }

}

/* =================== Main-Screen ======================== */

/*
 * Name:
 *     editorOpenToolDialog
 * Description:
 *     Opens the tool dialog for given position on map,
 *     depending on actual 'EditToolState'
 * Input:
 *     es *: Edit-Info with data about chosen tile
 */
static void editorOpenToolDialog(EDITMAIN_INFO_T *es)
{

    if (EditInfo.edit_mode > 0) {

        switch(EditInfo.edit_mode) {

            case EDITOR_TOOL_MAP:
                /* === No tool, only set info about edit-state === */
                break;

            case EDITOR_TOOL_FAN:
                /* Only display tiles and 'chosen' tiles */
                editorSetDialog(EditInfo.edit_mode, 1, 20, 20);
                break;

            case EDITOR_TOOL_PASSAGE:
                /* Get info about passage from dialog, if available */
                if (es -> mi_t_number > 0 && (es -> mi_type & MAP_INFO_PASSAGE)) {

                    editfilePassage(EDITFILE_ACT_GETDATA, es -> mi_t_number, &ActPsg);
                    editorSetDialog(EditInfo.edit_mode, 1, 20, 20);

                }
                break;

            case EDITOR_TOOL_OBJECT:
                /* Get info about objects if available */
                if (es -> mi_t_number > 0 && (es -> mi_type & MAP_INFO_SPAWN)) {

                    editfileSpawn(EDITFILE_ACT_GETDATA, es -> mi_t_number, &ActSpt);
                    editorSetDialog(EditInfo.edit_mode, 1, 20, 20);

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

    switch(event -> sub_code) {

        case EDITOR_2DMAP_CHOOSEFAN:
            editmainGetMapInfo(&EditInfo, event -> mou.x, event -> mou.y);

            if (event -> sdlcode == SDLGL_KEY_MOULEFT) {
                EditInfo.drag_w = 0;
                EditInfo.drag_h = 0;
                editmainChooseFan(&EditInfo, event -> mou.x, event -> mou.y, 0);
            }
            else if (event -> sdlcode == SDLGL_KEY_MOURIGHT) {
                EditInfo.drag_w = 0;
                EditInfo.drag_h = 0;
                editmainChooseFan(&EditInfo, event -> mou.x, event -> mou.y, 1);
                /* Check for special info (passages ( spawn points), if switched on */
                editorOpenToolDialog(&EditInfo);
            }   /* Choose dragging mouse */
            else if (event -> sdlcode == SDLGL_KEY_MOULDRAG) {
                EditInfo.drag_w += event -> mou.w;
                EditInfo.drag_h += event -> mou.h;
                EditInfo.drag_x = event -> mou.x - EditInfo.drag_w;
                EditInfo.drag_y = event -> mou.y - EditInfo.drag_h;
                editmainChooseFanExt(&EditInfo);
            }
            else if (event -> sdlcode == SDLGL_KEY_MOURDRAG)  {
                EditInfo.drag_w += event -> mou.w;
                EditInfo.drag_h += event -> mou.h;
                EditInfo.drag_x = event -> mou.x - EditInfo.drag_w;
                EditInfo.drag_y = event -> mou.y - EditInfo.drag_h;
                editmainChooseFanExt(&EditInfo);
            }
            break;
        case EDITOR_2DMAP_FANINFO:
            if (event -> pressed) {
                Map2DState ^= event -> sub_code;
            }
            /* Add this Dialog at position */
            if (Map2DState & event -> sub_code) {
                editorSetDialog(EDITOR_TOOL_FAN, 1, event -> mou.x, event -> mou.y);
            }
            else {
                editorSetDialog(EDITOR_TOOL_FAN, 0, event -> mou.x, event -> mou.y);
            }
            break;
        case EDITOR_2DMAP_FANROTATE:
            editmainMap(&EditInfo, EDITMAIN_ROTFAN);
            break;

        case EDITOR_2DMAP_FANBROWSE:
            editmainChooseFanType(&EditInfo, +1);
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

    switch(which) {
        case EDITOR_FILE_LOAD:
            editmainMap(&EditInfo, EDITMAIN_LOADMAP);
            /* Load additional data for this module */
            editfileSpawn(EDITFILE_ACT_LOAD, 1, &ActSpt);
            editfilePassage(EDITFILE_ACT_LOAD, 1, &ActPsg);
            editfileModuleDesc(EDITFILE_ACT_LOAD, &ModuleDesc);
            /* Init the size if the minimap display */
            editorSetMiniMap(EditInfo.map_size);
            break;

        case EDITOR_FILE_SAVE:
            editmainMap(&EditInfo, EDITMAIN_SAVEMAP);
            /* Save Module description, too */
            editfileModuleDesc(EDITFILE_ACT_SAVE, &ModuleDesc);
            break;

        case EDITOR_FILE_NEW:
            EditInfo.map_size = 32;
            editorSetMiniMap(32);
            /* --- Get a new, preinitialized module descriptor --- */
            editfileModuleDesc(EDITFILE_ACT_NEW, &ModuleDesc);

            /* Open Dialog for map size and other info about this module */
            editorSetDialog(EDITOR_TOOL_MODULE, 1, 20, 20);
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
    
    switch(event -> sub_code) {

        case EDITOR_DLG_DELETE:
            /* Delete chosen passage info */
            break;

        case EDITOR_DLG_NEW:
            /* --- TODO: Get it from 'editfile...' --- */
            break;

        case EDITOR_DLG_SAVE:
            break;

        case EDITOR_DLG_CLOSE:
            editorSetDialog(0, 0, 0, 0);
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

    switch(event -> sub_code) {

        case EDITOR_DLG_NEW:
            /* --- TODO: Get it from 'editfile...' --- */
            break;

        case EDITOR_DLG_DELETE:
            /* Delete chosen passage info */
            break;

        case EDITOR_DLG_SAVE:
            /* --- Write it with 'editfile...' --- */
            break;

        case EDITOR_DLG_CLOSE:
            editorSetDialog(0, 0, 0, 0);
            break;
    }

    return SDLGL_INPUT_OK;

}

/*
 * Name:
 *     editorModuleDlg
 * Description:
 *     Handles the input of the editors module dialog
 * Input:
 *     event *: Pointer on event to handle
 * Output:
 *     Result for SDLGL-Handler
 */
static int editorModuleDlg(SDLGL_EVENT *event)
{

    int i;


    switch(event -> sub_code) {
            
        case EDITOR_MODULEDLG_DECSIZE:
            if (EditInfo.map_size > 32) {
                EditInfo.map_size--;
            }
            break;

        case EDITOR_MODULEDLG_INCSIZE:
            if (EditInfo.map_size < 64) {
                EditInfo.map_size++;
            }
            break;

        case EDITOR_MODULEDLG_MODTYPE:
            ModuleDesc.mod_type_no++;
            if (ModuleDesc.mod_type_no > 2) {
                ModuleDesc.mod_type_no = 0;
            }
            strcpy(ModuleDesc.mod_type, QuestNames[ModuleDesc.mod_type_no]);
            break;

        case EDITOR_MODULEDLG_RATEINC:
            if (ModuleDesc.lev_rating_no < 5) {
                ModuleDesc.lev_rating_no++;
            }
            for (i = 0; i < ModuleDesc.lev_rating_no; i++) {
                ModuleDesc.lev_rating[i] = '*';
            }
            ModuleDesc.lev_rating[i] = 0;
            break;

        case EDITOR_MODULEDLG_RATEDEC:
            if (ModuleDesc.lev_rating_no > 0) {
                ModuleDesc.lev_rating_no--;
            }
            ModuleDesc.lev_rating[ModuleDesc.lev_rating_no] = 0;
            break;

        case EDITOR_MODULEDLG_OK:
            /* --- Save the info to file before closing --- */
            editfileModuleDesc(EDITFILE_ACT_SAVE, &ModuleDesc);
            /* Create the map -- TODO: Only create it if it not already exists ? */
            if (! editmainMap(&EditInfo, EDITMAIN_NEWMAP)) {
                /* TODO: Display message, what has gone wrong   */

            }
            /* Save the map for all cases */
            editmainMap(&EditInfo, EDITMAIN_SAVEMAP);
        case EDITOR_MODULEDLG_CANCEL:
            editorSetDialog(0, 0, 0, 0);
            break;

    }

    return SDLGL_INPUT_OK;

}

/*
 * Name:
 *     editorMenuTool
 * Description:
 *     Chooses the tool to work with on map
 * Input:
 *     event *: Event from menu
 */
static void editorMenuTool(SDLGL_EVENT *event)
{

    int rec_no;


    /* Set the number of tool to use, invoked by right click on map */
    EditInfo.edit_mode = event -> sub_code;

    if (event -> field -> pdata && event -> sub_code > 1) {
        /* Display it */
        sprintf(StatusBar, "Edit-State: %s", event -> field -> pdata);

    }

    if (EditInfo.edit_mode == EDITOR_TOOL_OFF)
    {
        /* Close actual active dialog, if any */
        editorSetDialog(0, 0, 0, 0);
        
    }
    else {

        editmainClearMapInfo();

        switch(EditInfo.edit_mode) {

            case EDITOR_TOOL_MAP:
            case EDITOR_TOOL_FAN:
                /* --- This is only changes the state of the editor: How to handle mouse input --- */
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

            case EDITOR_TOOL_MODULE:
                /* Open Dialog for editing module info */
                editorSetDialog(EDITOR_TOOL_MODULE, 1, 20, 20);
                break;                

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

    glClear(GL_COLOR_BUFFER_BIT);

    /* First move the camera */
    sdlgl3dMoveObjects(event -> secondspassed);

    /* Draw the 3D-View before the 2D-Screen */
    editmainMap(&EditInfo, EDITMAIN_DRAWMAP);

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
                    editmainDrawMap2D(&EditInfo, fields -> rect.x, fields -> rect.y);
                    break;

                case EDITOR_DRAWTEXTURE:
                    editmain2DTex(&EditInfo, fields -> rect.x, fields -> rect.y,
                                             fields -> rect.w, fields -> rect.h);
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

        field = event -> field;

        switch(event -> code) {

            case EDITOR_CAMERA:
                /* Move the camera */
                if (event -> pressed) {
                    /* Start movement */
                    sdlgl3dManageCamera(0, event -> sub_code, 1,
                                        (char)(event -> modflags & KMOD_LSHIFT ? 3 : 1));
                }
                else {
                    /* Stop movement */
                    sdlgl3dManageCamera(0, event -> sub_code, 0, 1);
                }
                break;

            case EDITOR_FILE:
                return editorFileMenu(event -> sub_code);

            case EDITOR_SHOWMAP:
                EditInfo.display_flags ^= EDITMAIN_SHOW2DMAP;
                break;

            case EDITOR_FANFX:
                /* Change the FX of the actual fan-info */
                EditInfo.ft.fx ^= event -> sub_code;
                field -> workval = (char)(EditInfo.ft.fx & event -> sub_code);
                break;

            case EDITOR_FANTEX:
                /* Change the Texture of the chosen fan   */
                if (event -> sub_code == EDITOR_FANTEX_CHOOSE) {
                    editmainChooseTex(&EditInfo, event -> mou.x, event -> mou.y,
                                                 field -> rect.w, field -> rect.h);
                }
                else if (event -> sub_code == EDITOR_FANTEX_DEC) {
                    editmainToggleFlag(&EditInfo, EDITMAIN_TOGGLE_FANTEXNO, 0xFF);
                }
                else if (event -> sub_code == EDITOR_FANTEX_INC) {
                    editmainToggleFlag(&EditInfo, EDITMAIN_TOGGLE_FANTEXNO, 0x01);
                }
                break;

            case EDITOR_FANPROPERTY:
                if (event -> sub_code == EDITOR_FANPROPERTY_EDITMODE) {
                    edit_mode = editmainToggleFlag(&EditInfo, EDITMAIN_TOGGLE_EDITSTATE, 1);
                    sprintf(EditTypeStr, "%s", SubMenu[9 + edit_mode].pdata);
                }
                else {
                    editorSetDialog(EDITOR_TOOL_FAN, 0, 20, 20);
                }
                break;

            case EDITOR_SETTINGS:
                editmainToggleFlag(&EditInfo, EDITMAIN_TOGGLE_DRAWMODE, event -> sub_code);
                field -> workval &= 0x7F;
                field -> workval |= (EditInfo.draw_mode & field -> sub_code);
                break;

            case EDITOR_2DMAP:
                /* TODO: Make this a 'dynamic' input field */
                editor2DMap(event);
                break;

            case EDITOR_MODULEDLG:
            case EDITOR_TOOL_MODULE:
                /* Handle module dialog input */
                editorModuleDlg(event);
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

            default:
                /* ------- Handle 'Standard'-Fields ------ */
                sdlglfldHandle(event);
                break;

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

    SDLGL_FIELD *field;


    /* Initalize the maze editor */
    editmainInit(&EditInfo, EditInfo.map_size, 7);

    /* Display initally chosen values */
    field = &SubMenu[0];

    while(field -> sdlgl_type != 0) {
        /* === Tell the drawing code that this one's a checkd menu === */
        if (field -> code == EDITOR_SETTINGS && field -> sub_code != -1) {

            field -> workval = (char)0x80 | (EditInfo.draw_mode & field -> sub_code);

        }

        field++;

    }

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
    editorSetMiniMap(EditInfo.map_size);

    sdlglAdjustRectToScreen(&MainMenu[2].rect, 0, SDLGL_FRECT_SCRRIGHT);
    /* Menu-Bar     */
    sdlglAdjustRectToScreen(&MainMenu[3].rect, 0, SDLGL_FRECT_SCRWIDTH);

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

    /* ------- Set the actual work directory ---------- */
    editfileSetWorkDir(EditorWorkDir);

    /* Set the input handlers and do other stuff before  */
    editorStart();

    /* Now enter the mainloop */
    sdlglExecute();

    /* Do any shutdown stuff. */
    editmainExit();

    sdlglShutdown();
    return 0;

}
