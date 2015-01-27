/*******************************************************************************
*  TEMPLATE.C                                                                  *
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

#include <stdio.h>		/* sprintf()    */
#include <string.h>     /* strcpy()     */
#include <memory.h>     /* memset()     */


/* ----- Own headers --- */
#include "sdlgl.h"          /* OpenGL and SDL-Stuff             */
#include "sdlglcfg.h"       // Read configuration file
#include "sdlglstr.h"
#include "egofile.h"        // Set basic directories for the game
#include "menu.h"           // Main menu, choose a module

// #include "egofont.h"        // Draw strings for menu

/*******************************************************************************
* TYPEDEFS								                                   *
*******************************************************************************/

/*******************************************************************************
* DEFINES								                                   *
*******************************************************************************/

#define EGOBOO_CAPTION "EGOBOO - V 0.2.1"

#define EGOBOO_MAXFLD   10

#define EGOBOO_NEWGAME  ((char)1)
#define EGOBOO_LOADGAME ((char)2)
#define EGOBOO_SETTINGS ((char)3)
#define EGOBOO_EXIT     ((char)10)

/*******************************************************************************
* DATA									                                       *
*******************************************************************************/

static char GameDir[120 + 2];
static char ModuleDir[32 + 2];
static char SavegameDir[120 + 2];

static SDLGL_CONFIG SdlGlConfig =
{
    EGOBOO_CAPTION,
    800, 600,           /* scrwidth, scrheight: Size of screen  */
    24,                 /* colordepth: Colordepth of screen     */
    0,                  /* wireframe                            */
    0,                  /* hidemouse                            */
    SDLGL_SCRMODE_WINDOWED, /* screenmode                       */
    1,                  /* debugmode                            */
    0                   /* zbuffer                              */
};

static SDLGLCFG_NAMEDVALUE CfgValues[] =
{
    { SDLGLCFG_VAL_INT, &SdlGlConfig.scrwidth, 4, "scrwidth" },
    { SDLGLCFG_VAL_INT, &SdlGlConfig.scrheight, 4, "scrheight" },
    { SDLGLCFG_VAL_INT, &SdlGlConfig.colordepth, 2, "colordepth" },
    { SDLGLCFG_VAL_INT, &SdlGlConfig.screenmode, 2, "screenmode" },
    { SDLGLCFG_VAL_INT, &SdlGlConfig.debugmode, 1, "debugmode" },
    { SDLGLCFG_VAL_STRING, &GameDir[0], 120, "egoboodir"  },
    { SDLGLCFG_VAL_STRING, &ModuleDir[0], 32, "moduledir"  },
    { SDLGLCFG_VAL_STRING, &SavegameDir[0], 32, "savegamedir"  },
    { 0 }
};

static SDLGL_FIELD GameTitle[] =
{
    { SDLGL_TYPE_LABEL,  {   0,  4,  48, 16 }, 0, 0, "EGOBOO" },
    { SDLGL_TYPE_LABEL,  {   4, 20, 256, 16 }, 0, 0, "The Legend of the Sporks of Yore" },
    { 0 }
};

static SDLGL_FIELD EgobooMenu[6] =
{
    { SDLGL_TYPE_MENU,  {   0, 0, 176, 16 }, EGOBOO_NEWGAME,  0, "Start a new game" },
    { SDLGL_TYPE_MENU,  {   4, 4, 176, 16 }, EGOBOO_LOADGAME, 0, "Load game in progress" },
    { SDLGL_TYPE_MENU,  {  44, 4, 176, 16 }, EGOBOO_SETTINGS, 0, "Settings" },
    { SDLGL_TYPE_MENU,  { 116, 4, 176, 16 }, EGOBOO_EXIT,     0, "Exit to OS" },
    { 0 }
};

static SDLGL_CMDKEY EgobooCmd[] =
{
    // @todo: Support choosing menu with arrow keys ?
    { { SDLK_ESCAPE }, EGOBOO_EXIT },
    { { 0 } }
};


/*******************************************************************************
* CODE									                                   *
*******************************************************************************/

/*
 * Name:
 *     egobooCalcMenuPos
 * Description:
 *     Draw anything on screen
 * Input:
 *
 */
static void egobooCalcMenuPos(SDLGL_FIELD *fields)
{
    SDLGL_FIELD *f;
    int height, tx, ty;


    height = -4;
    f = fields;

    while(f->sdlgl_type > 0)
    {
        height += f->rect.h+4;
        f++;
    }

    tx = (SdlGlConfig.scrwidth - fields->rect.w) / 2;
    ty = (SdlGlConfig.scrheight - height) / 2;

    f = fields;

    while(f->sdlgl_type > 0)
    {
        f->rect.x = tx;
        f->rect.y = ty;

        ty += f->rect.h + 4;
        f++;
    }
}

/*
 * Name:
 *     egobooDrawFunc
 * Description:
 *     Draw anything on screen
 * Input:
 *
 */
#ifdef __BORLANDC__
    #pragma argsused
#endif
static void egobooDrawFunc(SDLGL_FIELD *fields, SDLGL_EVENT *event)
{
    glClear(GL_COLOR_BUFFER_BIT);

    /* ---- Prepare drawing in 2D-Mode ---- */
    glPolygonMode(GL_FRONT, GL_FILL);
    glShadeModel(GL_FLAT);		            /* No smoothing	of edges    */
    glFrontFace(GL_CCW);                    /* Draws counterclockwise   */

    sdlglstrSetFont(SDLGLSTR_FONT8);

    // @todo: Draw the menu fields using bitmapped font 'egofont'
    sdlglstrDrawField(fields);
    // Write the title strings
    sdlglSetColor(SDLGL_COL_BROWN);
    sdlglstrDrawField(GameTitle);
}

/*
 * Name:
 *     egobooInputHandler
 * Description:
 *     Handler for the start screen
 * Input:
 *     event *: Pointer on event
 */
static int egobooInputHandler(SDLGL_EVENT *event)
{
    // SDLGL_FIELD *field;


    if (event -> code > 0)
    {
        // field = event -> field;
        
        switch(event -> code)
        {
            case EGOBOO_NEWGAME:
                // @todo: Choose modules with no IDSZ                
                menuMain(MENU_CHOOSEMODULE);
                break;
            case EGOBOO_LOADGAME:
                // @todo: Choose modules based on 'savegame'
                // menuMain(MENU_CHOOSEPLAYER);
                break;
            case EGOBOO_SETTINGS:
                // @todo: Display menu for settings
                // menuMain(MENU_OPTIONS);
                break;
            case EGOBOO_EXIT:
                return SDLGL_INPUT_EXIT;
        }
    }
    
    return SDLGL_INPUT_OK;
}

/*
 * Name:
 *     egobooStart
 * Description:
 *     Draw anything on screen
 * Input:
 *     None
 */
static void egobooStart(void)
{
    /* ------- Set the basic directories for all file-actions ---------- */
    egofileSetDir(EGOFILE_EGOBOODIR, GameDir);
    egofileSetDir(EGOFILE_MODULEDIR, ModuleDir);
    egofileSetDir(EGOFILE_SAVEGAMEDIR, SavegameDir);

    // Calc absolute position of menu fields
    egobooCalcMenuPos(EgobooMenu);

    // @todo: Load all basic stuff needed. Basic directory is known

    /* -------- Now create the output screen ---------- */
    sdlglInputNew(egobooDrawFunc,
                  egobooInputHandler,     /* Handler for input    */
                  EgobooMenu,               /* Input-Fields (mouse) */
                  EgobooCmd,              /* Keyboard-Commands    */
                  EGOBOO_MAXFLD);         /* Buffer size for dynamic menus    */
}

/*
 * Name:
 *     egobooExit
 * Description:
 *     Do all the cleanup stuff before exiting SDLGL
 * Input:
 *      None
 */
 static void egobooExit(void)
{
    // @todo: Fill in what is needed
} 

/* ========================================================================== */
/* ============================= PUBLIC FUNCTION(S) ========================= */
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
    sdlglcfgReadSimple("egoboo.cfg", CfgValues);
    // Set the configuration and the directories
    sdlglInit(&SdlGlConfig);
    
    // Center the title strings
    sdlglAdjustRectToScreen(&GameTitle[0].rect, &GameTitle[0].rect, SDLGL_FRECT_HCENTER);
    sdlglAdjustRectToScreen(&GameTitle[1].rect, &GameTitle[1].rect, SDLGL_FRECT_HCENTER);
    /* Set the input handlers and do other stuff before  */
    egobooStart();

    /* Now enter the mainloop */
    sdlglExecute();

    /* Do any shutdown stuff. */
    egobooExit();

    sdlglShutdown();
    
    return 0;
}
