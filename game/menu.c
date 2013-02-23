/*******************************************************************************
*  MENU.C                                                                      *
*      - Egoboo: The menu for the game                                         *
*                                                                              *
*  SDLGL - Template                                                            *
*      (c) 2013 Paul Mueller <bitnapper@sourceforge.net>                       *
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
#include <memory.h>     /* memset()     */
#include <string.h>


#include "egofile.h"    // Definition of a module description
#include "sdlglcfg.h"   // Read raw text file with module Names
#include "sdlglstr.h"

/* ----- Own header --- */
#include "menu.h"

/*******************************************************************************
* DEFINES								                                       *
*******************************************************************************/


/*
#define MENU_MAIN               1
#define MENU_CHOOSEMODULE       2
#define MENU_CHOOSEPLAYER       3
#define MENU_OPTIONS            4 */


/* #define MENU_SINGLEPLAYER       5 */
#define MENU_SINGLEPLAYER_NEW   1
#define MENU_SINGLEPLAYER_LOAD  2
#define MENU_SINGLEPLAYER_BACK  3

/* Special draw types */
#define MENU_DRAW_MODULEDESC    40

#define MENU_CHOOSEMODULE_FILTERFWD ((char)112)
#define MENU_CHOOSEMODULE_FWD       ((char)113)
#define MENU_CHOOSEMODULE_BACK      ((char)114)
#define MENU_CHOOSEMODULE_SELECT    ((char)115)
#define MENU_CHOOSEMODULE_CHOOSE    ((char)116)
#define MENU_CHOOSEMODULE_EXIT      ((char)117)

// Maximum number of modules
#define MENU_MAX_MODULE 100

/* Menu tree -->
    MenuMain[]
        New Game
            --> Choose Module 
        Load
            --> Choose Player
        Options 
            --> Game Options
            --> Audio Options
            --> Input Controls
            --> Video Settings 
        Quit        
    
*/    

/*******************************************************************************
* FORWARD DECLARATIONS      			                                       *
*******************************************************************************/

static int  menuInputHandler(SDLGL_EVENT *event);

/*******************************************************************************
* DATA									                                       *
*******************************************************************************/

/* === Filter-Descriptions === */
static char *FilterNames[] =
{
    "Main Quest",           // FILTER_MAIN
    "Sidequests",           // FILTER_SIDE
    "Towns and Cities",     // FILTER_TOWN
    "Fun Modules",          // FILTER_FUN
    "Starter Modules",      // FILTER_STARTER
    "All Modules",          // FILTER_OFF
    ""
};
                            
/* === Types of Quests === */
static char *QuestNames[] =
{
    "MAINQUEST",            // FILTER_MAIN
    "SIDEQUEST",            // FILTER_SIDE
    "TOWN",                 // FILTER_TOWN
    ""
};

static char *MenuDiffName[] =
{
    "FORGIVING (Easy)",     // GAME_EASY
    "CHALLENGING (Normal)", // GAME_NORMAL
    "PUNISHING (Hard)",     // GAME_HARD
    ""
};

static char *MenuDiffDesc[] =
{
    /* GAME_EASY */
    " - Players gain no bonus XP \n"
    " - 15%% XP loss upon death\n"
    " - Monsters take 25%% extra damage by players\n"
    " - Players take 25%% less damage by monsters\n"
    " - Halves the chance for Kursed items\n"
    " - Cannot unlock the final level in this mode\n"
    " - Life and Mana is refilled after quitting a module",
    /* GAME_NORMAL */
    " - Players gain 10%% bonus XP \n"
    " - 15%% XP loss upon death \n"
    " - 15%% money loss upon death",
    /* GAME_HARD */
    " - Monsters award 20%% extra xp! \n"
    " - 15%% XP loss upon death\n"
    " - 15%% money loss upon death\n"
    " - No respawning\n"
    " - Channeling life can kill you\n"
    " - Players take 25%% more damage\n"
    " - Doubles the chance for Kursed items"
    ""
};

/* ===> doOptions() === */
static SDLGL_FIELD MenuOptions[] =
{
    { SDLGL_TYPE_BUTTON, { 0, 0, 0, 0 }, MENU_OPTIONS, 1, "Game Options" },
    { SDLGL_TYPE_BUTTON, { 0, 0, 0, 0 }, MENU_OPTIONS, 2, "Audio Options" },
    { SDLGL_TYPE_BUTTON, { 0, 0, 0, 0 }, MENU_OPTIONS, 3, "Input Controls" },
    { SDLGL_TYPE_BUTTON, { 0, 0, 0, 0 }, MENU_OPTIONS, 4, "Video Settings" },
    { SDLGL_TYPE_BUTTON, { 0, 0, 0, 0 }, MENU_OPTIONS, 5, "Back" },
    { 0 }
};

/* ==> doSinglePlayerMenu() */    
static SDLGL_FIELD MenuSinglePlayer[] =
{   
    { SDLGL_TYPE_BUTTON, { 0, 0, 0, 0 }, MENU_SINGLEPLAYER, MENU_SINGLEPLAYER_NEW,  "New Player"  },
    { SDLGL_TYPE_BUTTON, { 0, 0, 0, 0 }, MENU_SINGLEPLAYER, MENU_SINGLEPLAYER_LOAD, "Load Saved Player" },
    { SDLGL_TYPE_BUTTON, { 0, 0, 0, 0 }, MENU_SINGLEPLAYER, MENU_SINGLEPLAYER_BACK, "Back" },
    { 0 }
};

/* ====> doChooseModule() */
static SDLGL_FIELD MenuChooseModule[16] =
{
    { SDLGL_TYPE_LABEL,  { 0, 0, 0, 0 }, -1 /* Show filter string */  },
    { SDLGL_TYPE_BUTTON, { 0, 0, 0, 0 }, MENU_CHOOSEMODULE, MENU_CHOOSEMODULE_FILTERFWD, ">" },
    { SDLGL_TYPE_BUTTON, { 0, 0, 0, 0 }, MENU_CHOOSEMODULE, MENU_CHOOSEMODULE_FWD, "->" },
    { SDLGL_TYPE_BUTTON, { 0, 0, 0, 0 }, MENU_CHOOSEMODULE, MENU_CHOOSEMODULE_BACK, "<-" },
    { SDLGL_TYPE_BUTTON, { 0, 0, 0, 0 }, MENU_CHOOSEMODULE, 0, /* Number */ },  // @todo: Number/name of module
    { SDLGL_TYPE_BUTTON, { 0, 0, 0, 0 }, MENU_CHOOSEMODULE, 1, /* Number */ },  // @todo: Number/name of module
    { SDLGL_TYPE_BUTTON, { 0, 0, 0, 0 }, MENU_CHOOSEMODULE, 2, /* Number */ },  // @todo: Number/name of module
    { SDLGL_TYPE_BUTTON, { 0, 0, 0, 0 }, MENU_CHOOSEMODULE, MENU_CHOOSEMODULE_SELECT, "Select Module" },
    { SDLGL_TYPE_BUTTON, { 0, 0, 0, 0 }, MENU_CHOOSEMODULE, MENU_CHOOSEMODULE_EXIT, "Back" },
    { 0 }
};

/* @todo: ===> doChoosePlayer()    */
/* @todo: ===> doOptionsInput()    */

/* ===> doOptionsGame()     */
static SDLGL_FIELD MenuOptionsGame[] =
{
    { SDLGL_TYPE_LABEL, { 0, 0, 0, 0 }, 0, 0, "Game Difficulty:" },
    { SDLGL_TYPE_VALUE, { 0, 0, 0, 0 }, 0, 0, 0 },  // lab_difficulty   --> doOptionsGame_update_difficulty()
    { SDLGL_TYPE_LABEL, { 0, 0, 0, 0 }, 0, 0, "Max  Messages:" },
    { SDLGL_TYPE_VALUE, { 0, 0, 0, 0 }, 0, 0, 0 },  // lab_msg_count    --> doOptionsGame_update_message_count
    { SDLGL_TYPE_LABEL, { 0, 0, 0, 0 }, 0, 0, "Message Duration:" },
    { SDLGL_TYPE_VALUE, { 0, 0, 0, 0 }, 0, 0, 0 },  // lab_msg_duration --> doOptionsGame_update_message_duration
    { SDLGL_TYPE_LABEL, { 0, 0, 0, 0 }, 0, 0, "Autoturn Camera:" },
    { SDLGL_TYPE_CHECKBOX, { 0, 0, 0, 0 }, 0, 0, 0 },   // lab_autoturn     --> doOptionsGame_update_cam_autoturn
    { SDLGL_TYPE_LABEL, { 0, 0, 0, 0 }, 0, 0, "Display FPS:" },
    { SDLGL_TYPE_CHECKBOX, { 0, 0, 0, 0 }, 0, 0, 0 },   // lab_fps          --> doOptionsGame_update_fps
    { SDLGL_TYPE_LABEL, { 0, 0, 0, 0 }, 0, 0, "Floating Text:" },
    { SDLGL_TYPE_VALUE, { 0, 0, 0, 0 }, 0, 0, 0 }, // lab_feedback     --> doOptionsGame_update_feedback
    { SDLGL_TYPE_LABEL, { 0, 0, 0, 0 }, 0, 0, "Game diffculty:" },
    { SDLGL_TYPE_VALUE, { 0, 0, 0, 0 }, 0, 0, 0 },  // lab_diff_desc --> set difficulty  GAME_EASY, GAME_NORMAL, GAME_HARD
    { SDLGL_TYPE_BUTTON, { 0, 0, 0, 0 }, 0, 0, "Save Settings" }, // but_save
    { 0 }
};

/* ===> doOptionsAudio() */
static SDLGL_FIELD MenuOptionsAudio[] =
{
    { SDLGL_TYPE_LABEL, { 0, 0, 0, 0 }, 0, 0, "Sound:" },
    { SDLGL_TYPE_CHECKBOX, { 0, 0, 0, 0 }, 0, 0, 0 },  // lab_on        - Enable sound
    { SDLGL_TYPE_LABEL, { 0, 0, 0, 0 }, 0, 0, "Sound Volume:" },
    { SDLGL_TYPE_VALUE, { 0, 0, 0, 0 }, 0, 0, 0 },  // lab_vol       - Sound volume
    { SDLGL_TYPE_LABEL, { 0, 0, 0, 0 }, 0, 0, "Music:" },
    { SDLGL_TYPE_CHECKBOX, { 0, 0, 0, 0 }, 0, 0, 0 },  // lab_mus_on    - Enable music
    { SDLGL_TYPE_LABEL, { 0, 0, 0, 0 }, 0, 0, "Music Volume:" },
    { SDLGL_TYPE_VALUE, { 0, 0, 0, 0 }, 0, 0, 0 },  // lab_mus_vol   - Music volume
    { SDLGL_TYPE_LABEL, { 0, 0, 0, 0 }, 0, 0, "Sound Channels:" },
    { SDLGL_TYPE_VALUE, { 0, 0, 0, 0 }, 0, 0, 0 },  // lab_channels  - Sound channels
    { SDLGL_TYPE_LABEL, { 0, 0, 0, 0 }, 0, 0, "Buffer Size:" },
    { SDLGL_TYPE_VALUE, { 0, 0, 0, 0 }, 0, 0, 0 },  // lab_buffer    - Sound buffer
    { SDLGL_TYPE_LABEL, { 0, 0, 0, 0 }, 0, 0, "Sound Quality:" },
    { SDLGL_TYPE_VALUE, { 0, 0, 0, 0 }, 0, 0, 0 },  // lab_quality   - Sound quality
    { SDLGL_TYPE_LABEL, { 0, 0, 0, 0 }, 0, 0, "Footstep Sounds:" },
    { SDLGL_TYPE_CHECKBOX, { 0, 0, 0, 0 }, 0, 0, 0 },  // lab_footsteps - Play footsteps
    { 0 }
};

/* TODO: Input-Sets for all four players */
/*
static SDLGL_FIELD MenuOptionsInput[] =
{

    { 0 }
};
*/

/* ===> doOptionsVideo() */
static SDLGL_FIELD MenuOptionsVideo[] =
{
    { SDLGL_TYPE_LABEL, { 0, 0, 0, 0 }, 0, 0, "Antialiasing:" },
    { SDLGL_TYPE_VALUE, { 0, 0, 0, 0 }, 0, 0, 0 },  // lab_antialiasing
    { SDLGL_TYPE_LABEL, { 0, 0, 0, 0 }, 0, 0, "Dithering:" },
    { SDLGL_TYPE_CHECKBOX, { 0, 0, 0, 0 }, 0, 0, 0 },  // lab_dither
    { SDLGL_TYPE_LABEL, { 0, 0, 0, 0 }, 0, 0, "Fullscreen:" },
    { SDLGL_TYPE_CHECKBOX, { 0, 0, 0, 0 }, 0, 0, 0 },  // lab_fullscreen
    { SDLGL_TYPE_LABEL, { 0, 0, 0, 0 }, 0, 0, "Reflections:" },
    { SDLGL_TYPE_CHECKBOX, { 0, 0, 0, 0 }, 0, 0, 0 },  // lab_reflections
    { SDLGL_TYPE_LABEL, { 0, 0, 0, 0 }, 0, 0, "Texture Filtering:" },
    { SDLGL_TYPE_VALUE, { 0, 0, 0, 0 }, 0, 0, 0 },  // lab_filtering
    { SDLGL_TYPE_LABEL, { 0, 0, 0, 0 }, 0, 0, "Shadows:" },
    { SDLGL_TYPE_CHECKBOX, { 0, 0, 0, 0 }, 0, 0, 0 },  // lab_shadow
    { SDLGL_TYPE_LABEL, { 0, 0, 0, 0 }, 0, 0, "Z Bit:" },
    { SDLGL_TYPE_VALUE, { 0, 0, 0, 0 }, 0, 0, 0 },  // lab_zbuffer
    { SDLGL_TYPE_LABEL, { 0, 0, 0, 0 }, 0, 0, "Max Lights:" },
    { SDLGL_TYPE_VALUE, { 0, 0, 0, 0 }, 0, 0, 0 },  // lab_maxlights
    { SDLGL_TYPE_LABEL, { 0, 0, 0, 0 }, 0, 0, "Special Effects:" },
    { SDLGL_TYPE_CHECKBOX, { 0, 0, 0, 0 }, 0, 0, 0 },  // lab_3dfx
    { SDLGL_TYPE_LABEL, { 0, 0, 0, 0 }, 0, 0, "Water Quality:" },
    { SDLGL_TYPE_CHECKBOX, { 0, 0, 0, 0 }, 0, 0, 0 },  // lab_multiwater
    { SDLGL_TYPE_LABEL, { 0, 0, 0, 0 }, 0, 0, "Widescreen:" },
    { SDLGL_TYPE_CHECKBOX, { 0, 0, 0, 0 }, 0, 0, 0 },  // lab_widescreen
    { SDLGL_TYPE_LABEL, { 0, 0, 0, 0 }, 0, 0, "Resolution:" },
    { SDLGL_TYPE_VALUE, { 0, 0, 0, 0 }, 0, 0, 0 },  // lab_screensize
    { SDLGL_TYPE_LABEL, { 0, 0, 0, 0 }, 0, 0, "Max Particles:" },
    { SDLGL_TYPE_VALUE, { 0, 0, 0, 0 }, 0, 0, 0 },  // lab_maxparticles
    { 0 }
};

/* ===> doGamePaused() @todo: Call this one in the 'play-mode' direct*/
static SDLGL_FIELD MenuGamePaused[] =
{
    /* Has to be added as 'overlay' resp. 'popup' onto actual 'sdlgl_input */ 
    /*
    "Quit Module",
    "Restart Module",
    "Return to Module",
    "Options",
    ""
    */
    { 0 }
};

// The module descriptors
static NumModuleDesc = 0;
static EGOFILE_MODULE_T ModuleDesc[MENU_MAX_MODULE + 2];

/*******************************************************************************
* CODE									                                       *
*******************************************************************************/

/*
 * Name:
 *     menuChooseModuleInput
 * Description:
 *     Handles the options for the single player menu
 * Input:
 *     event: To execute 
 */
static int menuChooseModuleInput(SDLGL_EVENT *event)
{

    if (event -> code > 0)
    {
        switch(event -> sub_code) {
        
            case MENU_CHOOSEMODULE:
                break;
            case MENU_CHOOSEMODULE_FILTERFWD:
                /*  ">" */
                break;
            case MENU_CHOOSEMODULE_FWD:
                /* "->" */
                break;
            case MENU_CHOOSEMODULE_BACK:
                /* "<-" */
                break;
            case MENU_CHOOSEMODULE_SELECT:
                /* "Select Module" */
                break;
            case MENU_CHOOSEMODULE_EXIT:
                /* "Back" */
                return SDLGL_INPUT_REMOVE;
                
            default:
                /* Number of module, choose it */
                break;
        }
    }

    return SDLGL_INPUT_OK;
}

/*
 * Name:
 *     menuSinglePlayerInput
 * Description:
 *     Handles the options for the single player menu
 * Input:
 *     event: To execute 
 */
static int menuSinglePlayerInput(SDLGL_EVENT *event)
{
    if (event -> code > 0)
    {
        switch(event -> sub_code)
        {
            case MENU_SINGLEPLAYER_NEW:
                /* "New Player"   */
                break;
                
            case MENU_SINGLEPLAYER_LOAD:    
                /* "Load Saved Player"   */
                break;
                
            case MENU_SINGLEPLAYER_BACK:
                return SDLGL_INPUT_REMOVE;
        }
    }
    
    return SDLGL_INPUT_OK;
}


/*
 * Name:
 *     menuOptionsInput
 * Description:
 *     Handles the options menu
 * Input:
 *     event: To execute 
 */
static int menuOptionsInput(SDLGL_EVENT *event)
{
    if (event -> code > 0)
    {
        switch(event -> sub_code)
        {
            case 1:
                /* "Game Options" */
                break;
            case 2:
                /* "Audio Options"  */
                break;
            case 3:
                /* "Input Controls" */
                break;
            case 4:  
                /* "Video Settings" */
                break;
            case 5: /* "Back" */
                return SDLGL_INPUT_REMOVE;
        }
    } /* if (event -> code > 0) */

    return SDLGL_INPUT_OK;
}

/*
 * Name:
 *     menuDrawFunc
 * Description:
 *     Drawing of start screen
 * Input:
 *
 */
#ifdef __BORLANDC__
    #pragma argsused
#endif
static void menuDrawFunc(SDLGL_FIELD *fields, SDLGL_EVENT *event)
{

    static SDLGL_RECT MousePos  = { 10, 10 };
    

    glClear(GL_COLOR_BUFFER_BIT);
    sdlglstrSetFont(SDLGLSTR_FONT8);
    sdlglSetColor(SDLGL_COL_YELLOW);

    /* -------- Display mouse position ------- */
    sdlglstrStringF(&MousePos, "Mouse Pos: X: %d / Y: %d", event -> mou.x, event -> mou.y);

    while(fields -> sdlgl_type  > 0)
    {
        sdlglstrDrawField(fields);
        // @todo: Additional drawing functionality for special menus
        fields++;
    }

    /* Draw a rectangle at the Mouse-position */
    event -> mou.w = 16;
    event -> mou.h = 16;
    sdlglstrDrawRectColNo(&event -> mou, SDLGL_COL_WHITE, 0);
}

/*
 * Name:
 *     menuLoadModuleDesc
 * Description:
 *     Load all the module descriptors
 * Input:
 *     None
 * Output: Number of modules
 */
int menuLoadModuleDesc(void)
{   
    int i;
    char *fname;    
    char read_buf[MENU_MAX_MODULE + 2][24];
    
    
    fname = egofileMakeFileName(EGOFILE_EGOBOODIR, "modlist.txt");
    // Read in list of all module names
    sdlglcfgRawLines(fname, read_buf[0], (MENU_MAX_MODULE * 24), 24, 0);
    
    for(i = 0; i < MENU_MAX_MODULE; i++)
    {
        if(read_buf[i][0] != 0)
        {
            // Set name of module to read the info for
            egofileSetDir(EGOFILE_MODULEDIR, read_buf[i]);
            // Now read the modules descriptor
            egofileModuleDesc(&ModuleDesc[i + 1], EGOFILE_ACT_LOAD);
            // Save the name of the directory
            strcpy(ModuleDesc[i + 1].dir_name, read_buf[i]);
            // @todo: Load the "title" texture for this module @note: (may be a 'png')
            // Directory '/gamedat/title. (bmp | png)
        }
        else
        {
            return (i + 1); // Count starts at one
        }
    }
    
    return 0;
}

/* ========================================================================== */
/* ============================= THE MAIN ROUTINE(S) ======================== */
/* ========================================================================== */

/*
 * Name:
 *     menuAdjustPos
 * Description:
 *     Places the menu fields horizontal and vertical centered on the screen
 * Input:
 *     fmenu *: Pointer on all menu fields
 *     vdist:   Vertical distance between buttons
 */
void menuAdjustPos(SDLGL_FIELD *fmenu, int vdist)
{
    SDLGL_FIELD *f;
    SDLGL_RECT mrect;
    int count;

    
    count   = 0;
    mrect.w = 0;
    f       = fmenu;

    /* 1. Get number of buttons and maximum size   */
    while (f -> code > 0)
    {
        sdlglstrGetButtonSize(f -> pdata, &f -> rect);

        if (mrect.w < f -> rect.w)
        {
            mrect.w = f -> rect.w;
        }

        count++;
        f++;
    }

    f = fmenu;
    
    mrect.h = (count * fmenu -> rect.h) + (vdist * (count - 1));;

    /* Now set the values left to set */
    sdlglAdjustRectToScreen(&mrect, &mrect, SDLGL_FRECT_SCRCENTER);

    while (f -> code > 0)
    {
        f -> rect.x = mrect.x;
        f -> rect.y = mrect.y;
        f -> rect.w = mrect.w;

        mrect.y += (f -> rect.h + vdist);
        f++;
    }
}

/*
 * Name:
 *     menuMain
 * Description:
 *
 * Input:
 *     which: Which menu to open
 */
int menuMain(char which)
{
    switch(which)
    {
        case MENU_CHOOSEMODULE:
            // Load the module descriptors
            if(!NumModuleDesc)
            {
                NumModuleDesc = menuLoadModuleDesc();
            }
            /* -------- Create the choose module screen ---------- */
            /*
                sdlglInputNew(menuDrawFunc,
                            menuChooseModuleInput,
                            MenuChooseModule,
                            0,
                            0);
                */
            break;

        case MENU_CHOOSEPLAYER:
            /* -------- Create the choose player screen ---------- */
            /*
            menuAdjustPos(MenuSinglePlayer, 4);
            sdlglInputNew(menuDrawFunc,
                          menuSinglePlayerInput,
                          MenuSinglePlayer,
                          0,
                          0);
                */
            break;

        case MENU_OPTIONS:
            /* -------- Create the options screen ---------- */
            menuAdjustPos(MenuOptions, 4);
            sdlglInputNew(menuDrawFunc,
                          menuOptionsInput,
                          MenuOptions,
                          0,
                          0);
            break;

        case MENU_QUIT:
            return SDLGL_INPUT_EXIT;
    }

    return SDLGL_INPUT_OK;
}
