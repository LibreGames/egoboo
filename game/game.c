/*******************************************************************************
*  TEMPLATE.C                                                                  *
*    - EGOBOO-Game                                                             *
*                                                                              *
*    - [...]                                                                   *
*      (c) The Egoboo Team                                                     *
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

// Engine functions
#include "sdlgl.h"
#include "sdlglstr.h"
#include "sdlgl3d.h"

// Egoboo headers
#include "egomap.h"

// Own header
#include "game.h"

/*******************************************************************************
* DEFINES                                                                      *
*******************************************************************************/

#define GAME_MAXFLD   100

/* --------- The different commands -------- */
#define GAME_EXITMODULE    ((char)100)
#define GAME_CAMERA      ((char)10)          /* Move camera           */


#define COLOR_MAX 16
#define MAX_BLIP  128  

/*******************************************************************************
* TYPEDEFS                                                                     *
*******************************************************************************/

typedef struct
{
    int x, y;
    int c;    
    
} GAME_BLIP_T;

/*******************************************************************************
* DATA                                                                         *
*******************************************************************************/

static char GameTimerOn = 0;
static int  GameTimerValue = 0;
static char ShowMap = 0;
static char ShowPlayerPos = 0;
static int GameBlipCount = 0;
static GAME_BLIP_T GameBlips[MAX_BLIP + 2]; 

static SDLGL_FIELD GameMenu[GAME_MAXFLD + 2] =
{
    /*
    { EDITOR_DRAW3DMAP, {   0,   0, 800, 600 }, EDITOR_3DMAP }, // 3D-View      
    { SDLGL_TYPE_STD,   {   0, 584, 800,  16 } },               // Status bar   
    { EDITOR_DRAW2DMAP, {   0, 16, 256, 256 }, EDITOR_2DMAP, EDITOR_2DMAP_CHOOSEFAN }, //
    // 'Code' needed in menu-background' for support of 'mouse-over'    
    { SDLGL_TYPE_STD,   {   0, 0, 800, 16 }, -1 },          // Menu-Background  
    { SDLGL_TYPE_MENU,  {   4, 4, 32, 8 }, EDITOR_FILE,     0, "File" },
    { SDLGL_TYPE_MENU,  {  44, 4, 64, 8 }, EDITOR_SETTINGS, 0, "Settings" },
    { SDLGL_TYPE_MENU,  { 116, 4, 40, 8 }, EDITOR_TOOLS,    0, "Tools" },
    */
    { 0 }
};

/*
Special keys for EGOBOO documentation:
'input_handle_chat': SDLK_RETURN, SDLK_ESCAPE, SDLK_BACKSPACE
'input_handle_SDL_KEYDOWN': SDLK_ESCAPE EProc->escape_requested = btrue;
'render_prt_bbox': SDLK_F7 ==> single frame mode
'doMainMenu': SDLK_ESCAPE, SDLK_RIGHT, SDLK_LEFT, SDLK_RETURN
'cl_talkToHost': SDLK_RETURN
'camera_update_track': (CAM_FREE) SDLK_KP8, SDLK_KP2, SDLK_KP4, SDLK_KP6, SDLK_KP7, SDLK_KP9 
'chr_get_mesh_pressure': (dev_mode) SDLK_F8
'do_ego_proc_running': SDLKEYDOWN( SDLK_q ) && SDLKEYDOWN( SDLK_LCTRL ) == Process kill
( single_frame_keyready && SDLKEYDOWN( SDLK_F10 ) ) == Switch single frame
( screenshot_keyready && SDLKEYDOWN( SDLK_F11 ) == screenshot requested
cfg.dev_mode && SDLKEYDOWN( SDLK_F9 ) == // super secret "I win" button
'game.c'
SDLKEYDOWN( SDLK_f ) && SDLKEYDOWN( SDLK_LCTRL ) == Free Running
SDLKEYDOWN( SDLK_SPACE ) == Respawn Key
cfg.dev_mode && SDLKEYDOWN( SDLK_m ) && SDLKEYDOWN( SDLK_LSHIFT ) == Switch map display
cfg.dev_mode && SDLKEYDOWN( SDLK_x ) == // XP CHEAT
    SDLKEYDOWN( SDLK_1 - SDLK_4) == docheat 0..3    
cfg.dev_mode && SDLKEYDOWN( SDLK_z ) == // LIFE CHEAT 
SDLKEYDOWN( SDLK_LSHIFT ) // Display armor stats?
    SDLK_1..SDLK_8 = show_armor(0..7). Delay 1000 (1 Second...)
SDLKEYDOWN( SDLK_LCTRL )  // Display enchantment stats?
    SDLK_1..SDLK_8 = show_full_status(0..7). Delay 1000 (1 Second...)    
SDLKEYDOWN( SDLK_LALT )  // // Display character special powers?
    SDLK_1..SDLK_8 = show_magic_status(0..7). Delay 1000 (1 Second...)
// No shift status: // Display character stats?
    SDLK_1..SDLK_8 = show_magic_status(0..7). Delay 1000 (1 Second...)
'graphic.c' -- 'draw_help'
SDLKEYDOWN( SDLK_F1 ): Mouse help
SDLKEYDOWN( SDLK_F2 ): Joystick help
SDLKEYDOWN( SDLK_F3 ): Keyboard help
SDLKEYDOWN( SDLK_F5 ): Debug mode 5
SDLKEYDOWN( SDLK_F6 ): Debug mode 6
SDLKEYDOWN( SDLK_F7 ): Debug mode 7
'draw_chr_bbox':
cfg.dev_mode && SDLKEYDOWN( SDLK_F7 ) // draw the object bounding box as a part of the graphics debug mode F7

*/
static SDLGL_CMDKEY GameCmd[] =
{
    /* ---------- Camera movement ------ */
    { { SDLK_KP8 }, GAME_CAMERA, SDLGL3D_MOVE_FORWARD,   SDLGL3D_MOVE_FORWARD },
    { { SDLK_KP2 }, GAME_CAMERA, SDLGL3D_MOVE_BACKWARD,  SDLGL3D_MOVE_BACKWARD },
    { { SDLK_KP4 }, GAME_CAMERA, SDLGL3D_MOVE_LEFT,      SDLGL3D_MOVE_LEFT },
    { { SDLK_KP6 }, GAME_CAMERA, SDLGL3D_MOVE_RIGHT,     SDLGL3D_MOVE_RIGHT },
    { { SDLK_KP7 }, GAME_CAMERA, SDLGL3D_MOVE_TURNLEFT,  SDLGL3D_MOVE_TURNLEFT },
    { { SDLK_KP9 }, GAME_CAMERA, SDLGL3D_MOVE_TURNRIGHT, SDLGL3D_MOVE_TURNRIGHT },
    { { SDLK_KP_PLUS },  GAME_CAMERA, SDLGL3D_MOVE_ZOOMIN,  SDLGL3D_MOVE_ZOOMIN },
    { { SDLK_KP_MINUS }, GAME_CAMERA, SDLGL3D_MOVE_ZOOMOUT, SDLGL3D_MOVE_ZOOMOUT },
    /* Second possibility for camera movement */
    { { SDLK_LEFT },  GAME_CAMERA, SDLGL3D_MOVE_LEFT,     SDLGL3D_MOVE_LEFT },
    { { SDLK_RIGHT }, GAME_CAMERA, SDLGL3D_MOVE_RIGHT,    SDLGL3D_MOVE_RIGHT },
    { { SDLK_UP },    GAME_CAMERA, SDLGL3D_MOVE_FORWARD,  SDLGL3D_MOVE_FORWARD },
    { { SDLK_DOWN },  GAME_CAMERA, SDLGL3D_MOVE_BACKWARD, SDLGL3D_MOVE_BACKWARD },
    /* -------- Exit the game ------- */
    { { SDLK_ESCAPE }, GAME_EXITMODULE },
    { { 0 } }
};

/*******************************************************************************
* CODE									                                       *
*******************************************************************************/

/*
 * Name:
 *     gameInputHandler
 * Description:
 *     Translates all the 'human' users input (network input could be attached here)
 * Input:
 *     event *: Pointer on event to translate
 */
static int gameInputHandler(SDLGL_EVENT *event)
{
    if (event -> code > 0)
    {
        switch(event -> code)
        {
            /* @todo: Translate the other movements
            case GAME_CAMERA:
                break;
            */
            case GAME_EXITMODULE:
                return SDLGL_INPUT_EXIT;
        }
    }   
    
    return SDLGL_INPUT_OK;
}
/*
 * Name:
 *     gameDrawFunc
 * Description:
 *     Draw anything on screen
 * Input:
 *
 */
#ifdef __BORLANDC__
    #pragma argsused
#endif
static void gameDrawFunc(SDLGL_FIELD *fields, SDLGL_EVENT *event)
{
    glClear(GL_COLOR_BUFFER_BIT);
 
    // @todo: Support 'single_frame_mode'
    /* -------------- motion and control section ---------- */
      
    // @todo: First move all objects, because the camera may be attacked to a game object
    // move all objects here
    // sdlgl3dMoveObjects(event -> secondspassed);
    // @todo: Apply environtal effects on movement like 'slippery' and so on 
    
    
    // Move the camera
    sdlgl3dMoveCamera(event -> secondspassed);
    
    // @todo: Check collision with feedback to characters and AI
    //        Applies physics 
    // collisionMain();
    
    // @todo: void egomapMoveObjects(void);
    // Update of the linked lists on map, removing of killed objects
    // Applies environmental effects to all objects
    // Apply additional actions by 'particleUpdateOne()'
 
    // @todo: Do all AI-Stuff using their scripts
    // aiMain();        

    /* -------------- Drawing section ---------- */
    // Draw the 3D-View before the 2D-Screen 
    // egomapDraw(&es -> ft, &es -> cm, es -> crect);
    // void egomapDraw(FANDATA_T *fd, COMMAND_T *cd, int *crect);
    // render3dMain();

    /* ---- Prepare drawing in 2D-Mode ---- */
    glPolygonMode(GL_FRONT, GL_FILL);
    glShadeModel(GL_FLAT);		            /* No smoothing	of edges    */
    glFrontFace(GL_CCW);                    /* Draws counterclockwise   */
    
    // @todo: Draw all the 2D-Stuff 
    // render2dMain();
}

/* ========================================================================== */
/* ============================= PUBLIC FUNCTION(S) ========================= */
/* ========================================================================== */

/*
 * Name:
 *     gamePlay
 * Description:
 *     Plays the game using the default keys for the keyboard player
 * Input:
 *     None
 */
void gamePlay(void)
{
    /* -------- Initialize the 3D-Stuff --------------- */
    sdlgl3dInitCamera(0, 310, 0, 90, 0.75);
    /* Init Camera +Z is up, -Y is near plane, X is left/right */
    sdlgl3dMoveToPosCamera(0, 384.0, 384.0, 600.0, 0);

    /* -------- Now create the output screen ---------- */
    sdlglInputNew(gameDrawFunc,
                  gameInputHandler,     /* Handler for input    */
                  GameMenu,               /* Input-Fields (mouse) */
                  GameCmd,              /* Keyboard-Commands    */
                  GAME_MAXFLD);         /* Buffer size for dynamic menus    */
}


/* ============== Functions to call by scripting engine ============== */

/*
 * Name:
 *     gameSet
 * Description:
 *     Set given value
 * Input:
 *     which: Which value to set
 *     value: Value to set  
 */
void gameSet(int which, int value)
{
    // float fTmp;
    
    
    switch(which)
    {
        case GAME_FOG_BOTTOMLEVEL:
            /*
            fTmp = (value / 10.0f ) - fog.bottom;
            fog.bottom += fTmp;
            fog.distance -= fTmp;
            fog.on      = cfg.fog_allowed;
            if ( fog.distance < 1.0f ) fog.on = 0;
            */
            break;
        case GAME_FOG_TAD:
            /*
            fog.color = value;
            fog.red = CLIP(pstate->turn, 0, 0xFF);
            fog.grn = CLIP(pstate->argument, 0, 0xFF);
            fog.blu = CLIP(pstate->distance, 0, 0xFF);
            */
            break;
        case GAME_FOG_LEVEL:
            /*            
            fTmp = ( pstate->argument / 10.0f ) - fog.top;
            fog.top += fTmp;
            fog.distance += fTmp;
            fog.on = cfg.fog_allowed;
            if ( fog.distance < 1.0f )  fog.on = 0;
            */
            break;
        case GAME_SHOWPLAYER_POS:
            ShowPlayerPos = (char)value;
            break;
        case GAME_SHOWMAP:
            ShowMap = (char)value;
            break;
        case GAME_TIMER:    
            GameTimerOn = 1;
            GameTimerValue = value;
            // timeron = 1;
            // timervalue = pstate->argument;
            break;
    }
}

/*
 * Name:
 *     gameGet
 * Description:
 *     Get given value
 * Input:
 *     which: Which value to get
 */
int gameGet(int which)
{
    switch(which)
    {
        case GAME_FOG_BOTTOMLEVEL:  
            // fog.bottom * 10;
            return 1;
        case GAME_FOG_LEVEL:
            // fog.top * 10;
            break;
    }
    
    return 0;
}

/*
 * Name:
 *     gameShowBlipXY
 * Description:
 *     Shows a blip at given position
 * Input:
 *     tx, ty:   Position
 *     color_no: Color for blip
 */
 void gameShowBlipXY(int tx, int ty, int color_no)
 {
    // Add a blip
    if(GameBlipCount < MAX_BLIP)
    {
        // @todo: Clip blips to map size in tiles
        // if (tx > 0 && tx < PMesh->gmem.edge_x && ty > 0 && ty < PMesh->gmem.edge_y)
        // {
            if(color_no >= 0)
            {
                GameBlips[blip_count].x = tx;
                GameBlips[blip_count].y = ty;
                GameBlips[blip_count].c = color_no % COLOR_MAX;
                GameBlipCount++;
            }
        // }
    }
 }