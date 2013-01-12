/*******************************************************************************
*  INPUT.C                                                                     *
*	   - Loads all data needed for input and does the translation for the game *
*                                                                              *
*   EGOBOO			                                                           *
*       Copyright (C) 2009  The Egoboo Team                                    *
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
* INCLUDES								                                   *
*******************************************************************************/

#include <memory.h>     /* memset()     */
#include <string.h>     /* strcmp()     */


#include "sdlgl.h"
#include "sdlglcfg.h"
// #include "sdlgl3d.h"    // Camera movement


/* --- Own header --- */
#include "player.h"

/*******************************************************************************
* DEFINES 								                                  *
*******************************************************************************/

#define INPUT_MAXCOMMAND 128   /* Maximum number of commands for players    */

/*******************************************************************************
* TYPEDEFS								                                       *
*******************************************************************************/

typedef struct
{
    char name[18];  /* replaces: char tagname[MAXTAG][TAGSIZE]; -> Scancode names  */
    int  sdl_code;  /* replaces: unsigned int tagvalue[MAXTAG]; -> Scancode values */
                    /* The scancode or mask ==> SDL-Code				           */

} CONTROL_NAME;

typedef struct
{
    int sdlkey;
    char shiftkey;

} INPUT_SHIFTVAL;

/*******************************************************************************
* DATA								                                       *
*******************************************************************************/

/* Buffer for key descriptors */
static char KeyNames[20*32];

/* Definition of the names values to read from file */
static SDLGLCFG_NAMEDVALUE CtrlGame[] =
{
    { SDLGLCFG_VAL_STRING, &KeyNames[0],   31, "Jump", EGOCODE_JUMP },
    { SDLGLCFG_VAL_STRING, &KeyNames[32],  31, "Left_Hand_Use", EGOCODE_LEFT_USE },
    { SDLGLCFG_VAL_STRING, &KeyNames[64],  31, "Left_Hand_Get_Drop", EGOCODE_LEFT_GET },
    { SDLGLCFG_VAL_STRING, &KeyNames[96],  31, "Left_Hand_Inventory", EGOCODE_LEFT_PACK },
    { SDLGLCFG_VAL_STRING, &KeyNames[128], 31, "Right_Hand_Use", EGOCODE_RIGHT_USE },
    { SDLGLCFG_VAL_STRING, &KeyNames[160], 31, "Right_Hand_Get_Drop", EGOCODE_RIGHT_GET },
    { SDLGLCFG_VAL_STRING, &KeyNames[192], 31, "Right_Hand_Inventory", EGOCODE_RIGHT_PACK },
    { SDLGLCFG_VAL_STRING, &KeyNames[224], 31, "Sneak", EGOCODE_SNEAK_ON },
    { SDLGLCFG_VAL_STRING, &KeyNames[256], 31, "Send_Message", EGOCODE_MESSAGE },
    { SDLGLCFG_VAL_STRING, &KeyNames[288], 31, "Camera_Rotate_Left", EGOCODE_CAMERA_LEFT },
    { SDLGLCFG_VAL_STRING, &KeyNames[320], 31, "Camera_Rotate_Right", EGOCODE_CAMERA_RIGHT },
    { SDLGLCFG_VAL_STRING, &KeyNames[352], 31, "Camera_Zoom_In", EGOCODE_CAMERA_IN },
    { SDLGLCFG_VAL_STRING, &KeyNames[384], 31, "Camera_Zoom_Out", EGOCODE_CAMERA_OUT },
    { SDLGLCFG_VAL_STRING, &KeyNames[416], 31, "Up", EGOCODE_ARROWUP },
    { SDLGLCFG_VAL_STRING, &KeyNames[448], 31, "Down", EGOCODE_ARROWDOWN },
    { SDLGLCFG_VAL_STRING, &KeyNames[480], 31, "Left", EGOCODE_ARROWLEFT },
    { SDLGLCFG_VAL_STRING, &KeyNames[512], 31, "Right", EGOCODE_ARROWRIGHT },
    { SDLGLCFG_VAL_STRING, &KeyNames[544], 31, "Camera_Control_Mode", EGOCODE_CAMERAMODE },
    { SDLGLCFG_VAL_STRING, &KeyNames[576], 31, "Move", EGOCODE_MOVEMENT },
    { 0 }
    
};

/* ------- Table for special cases of shift-values ------ */
/*
static INPUT_SHIFTVAL Shift_Char_Lut[] =
{
    { SDLK_1, '!' },
    { SDLK_2, '@' },
    { SDLK_3, '#' },
    { SDLK_4, '$' },
    { SDLK_5, '%' },
    { SDLK_6, '^' },
    { SDLK_7, '&' },
    { SDLK_8, '*' },
    { SDLK_9, '(' },
    { SDLK_0, ')' },
    { SDLK_QUOTE, '\"' },
    { SDLK_SPACE, ' ' },
    { SDLK_SEMICOLON, ':' },
    { SDLK_PERIOD, '>' },
    { SDLK_COMMA, '<' },
    { SDLK_BACKQUOTE, '`' },
    { SDLK_MINUS, '_' },
    { SDLK_EQUALS, '+' },
    { SDLK_LEFTBRACKET, '{' },
    { SDLK_RIGHTBRACKET, '}' },
    { SDLK_BACKSLASH, '\\' },
    { SDLK_SLASH, '?' },
    { 0 }
};
*/

static PLAYER_T Player[PLAYER_MAX + 2];    /* List of players      */
                                                  
/* Control Table needed to translate from CONTROLS.CFG to SDL-Codes */
static CONTROL_NAME Control_Table[] =
{
    { "KEY_ESCAPE", SDLK_ESCAPE },
    { "KEY_1", SDLK_1 },
    { "KEY_2", SDLK_2 },
    { "KEY_3", SDLK_3 },
    { "KEY_4", SDLK_4 },
    { "KEY_5", SDLK_5 },
    { "KEY_6", SDLK_6 },
    { "KEY_7", SDLK_7 },
    { "KEY_8", SDLK_8 },
    { "KEY_9", SDLK_9  },
    { "KEY_0", SDLK_0 },
    { "KEY_MINUS", SDLK_MINUS },
    { "KEY_EQUALS", SDLK_EQUALS  },
    { "KEY_BACK_SPACE", SDLK_BACKSPACE },
    { "KEY_TAB", SDLK_TAB },
    { "KEY_Q", SDLK_q },
    { "KEY_W", SDLK_w },
    { "KEY_E", SDLK_e },
    { "KEY_R", SDLK_r },
    { "KEY_T", SDLK_t },
    { "KEY_Y", SDLK_y },
    { "KEY_U", SDLK_u },
    { "KEY_I", SDLK_i },
    { "KEY_O", SDLK_o },
    { "KEY_P", SDLK_p },
    { "KEY_LEFT_BRACKET", SDLK_LEFTBRACKET },
    { "KEY_RIGHT_BRACKET", SDLK_RIGHTBRACKET },
    { "KEY_RETURN", SDLK_RETURN },
    { "KEY_ENTER", SDLK_KP_ENTER },
    { "KEY_LEFT_CONTROL", SDLK_LCTRL },
    { "KEY_A", SDLK_a },
    { "KEY_S", SDLK_s },
    { "KEY_D", SDLK_d },
    { "KEY_F", SDLK_f },
    { "KEY_G", SDLK_g },
    { "KEY_H", SDLK_h },
    { "KEY_J", SDLK_j },
    { "KEY_K", SDLK_k },
    { "KEY_L", SDLK_l },
    { "KEY_SEMICOLON", SDLK_SEMICOLON },
    { "KEY_APOSTROPHE", SDLK_QUOTE },
    { "KEY_GRAVE", SDLK_BACKQUOTE },
    { "KEY_LEFT_SHIFT", SDLK_LSHIFT },
    { "KEY_BACKSLASH",  SDLK_BACKSLASH },
    { "KEY_Z", SDLK_z },
    { "KEY_X", SDLK_x },
    { "KEY_C", SDLK_c },
    { "KEY_V", SDLK_v },
    { "KEY_B", SDLK_b },
    { "KEY_N", SDLK_n },
    { "KEY_M", SDLK_m },
    { "KEY_COMMA", SDLK_COMMA },
    { "KEY_PERIOD", SDLK_PERIOD },
    { "KEY_SLASH", SDLK_SLASH },
    { "KEY_RIGHT_SHIFT", SDLK_RSHIFT },
    { "KEY_PAD_ASTERISK", SDLK_KP_MULTIPLY},
    { "KEY_LEFT_ALT", SDLK_LALT },
    { "KEY_SPACE", SDLK_SPACE },
    { "KEY_CAPITAL", SDLK_CAPSLOCK },
    { "KEY_F1", SDLK_F1 },
    { "KEY_F2", SDLK_F2 },
    { "KEY_F3", SDLK_F3 },
    { "KEY_F4", SDLK_F4 },
    { "KEY_F5", SDLK_F5 },
    { "KEY_F6", SDLK_F6 },
    { "KEY_F7", SDLK_F7 },
    { "KEY_F8", SDLK_F8 },
    { "KEY_F9", SDLK_F9 },
    { "KEY_F10", SDLK_F10 },
    { "KEY_NUM_LOCK", SDLK_NUMLOCK },
    { "KEY_SCROLL_LOCK", SDLK_SCROLLOCK },
    { "KEY_PAD_7", SDLK_KP7 },
    { "KEY_PAD_8", SDLK_KP8 },
    { "KEY_PAD_9", SDLK_KP9 },
    { "KEY_PAD_MINUS", SDLK_KP_MINUS },
    { "KEY_PAD_4", SDLK_KP4 },
    { "KEY_PAD_5", SDLK_KP5 },
    { "KEY_PAD_6", SDLK_KP6 },
    { "KEY_PAD_PLUS", SDLK_KP_PLUS },
    { "KEY_PAD_1", SDLK_KP1 },
    { "KEY_PAD_2", SDLK_KP2 },
    { "KEY_PAD_3", SDLK_KP3 },
    { "KEY_PAD_0", SDLK_KP0 },
    { "KEY_PAD_PERIOD",SDLK_KP_PERIOD },
    { "KEY_F11", SDLK_F11 },
    { "KEY_F12", SDLK_F12 },
    { "KEY_PAD_ENTER", SDLK_KP_ENTER },
    { "KEY_RIGHT_CONTROL", SDLK_RCTRL },
    { "KEY_PAD_SLASH", SDLK_KP_DIVIDE },
    { "KEY_RIGHT_ALT", SDLK_RALT },
    { "KEY_HOME", SDLK_HOME },
    { "KEY_UP", SDLK_UP },
    { "KEY_PAGE_UP", SDLK_PAGEUP },
    { "KEY_LEFT", SDLK_LEFT },
    { "KEY_RIGHT", SDLK_RIGHT },
    { "KEY_END", SDLK_END },
    { "KEY_DOWN", SDLK_DOWN },
    { "KEY_PAGE_DOWN", SDLK_PAGEDOWN },
    { "KEY_INSERT", SDLK_INSERT },
    { "KEY_DELETE", SDLK_DELETE },
    /* Mouse 'keys' */
    { "MOS_MIDDLE", SDLGL_KEY_MOUMIDDLE },
    { "MOS_RIGHT", SDLGL_KEY_MOURIGHT },
    { "MOS_LEFT", SDLGL_KEY_MOULEFT },
    { "MOS_MOVE", SDLGL_KEY_MOUMOVE },
    /* @todo: Support joysticks
    // Joystick A 'keys'
    { "JOY_A_0", SDLGL_KEY_JOY_A_0 },
    { "JOY_A_1", SDLGL_KEY_JOY_A_1 },
    { "JOY_A_2", SDLGL_KEY_JOY_A_2 },
    { "JOY_A_3", SDLGL_KEY_JOY_A_3 },
    { "JOY_A_4", SDLGL_KEY_JOY_A_4 },
    { "JOY_A_5", SDLGL_KEY_JOY_A_5 },
    { "JOY_A_6", SDLGL_KEY_JOY_A_6 },
    { "JOY_A_7", SDLGL_KEY_JOY_A_7 },
    { "JOY_A_MOVE", SDLGL_KEY_A_JOY_M },
    // Joystick B 'keys' 
    { "JOY_B_0", SDLGL_KEY_JOY_B_0 },
    { "JOY_B_1", SDLGL_KEY_JOY_B_1 },
    { "JOY_B_2", SDLGL_KEY_JOY_B_2 },
    { "JOY_B_3", SDLGL_KEY_JOY_B_3 },
    { "JOY_B_4", SDLGL_KEY_JOY_B_4 },
    { "JOY_B_5", SDLGL_KEY_JOY_B_5 },
    { "JOY_B_6", SDLGL_KEY_JOY_B_6 },
    { "JOY_B_7", SDLGL_KEY_JOY_B_7 },
    { "JOY_B_MOVE", INPUT_JOY_B_M },
    */
    { "" }			/* Empty string signs end of array */

};

static SDLGL_CMDKEY CmdList[INPUT_MAXCOMMAND + 2] = {

    /* --------- First are the standard-Commands --------- */
    { { SDLK_ESCAPE }, EGOCODE_QUITMODULE },
    { { SDLK_F11 }, EGOCODE_SCREENSHOT },
    { { SDLK_F8 }, EGOCODE_PAUSEGAME },
    { { SDLK_SPACE }, EGOCODE_RESPAWN },
    { { SDLK_z, SDLK_1 }, EGOCODE_LIFECHEAT },
    { { SDLK_x, SDLK_1 }, EGOCODE_GIVEEXPERIENCE, 1 },
    { { SDLK_x, SDLK_2 }, EGOCODE_GIVEEXPERIENCE, 2 },
    { { SDLK_LSHIFT, SDLK_m }, EGOCODE_MAPCHEAT },
    { { SDLK_1 }, EGOCODE_SHOWSTAT, 1 },
    { { SDLK_2 }, EGOCODE_SHOWSTAT, 2 },
    { { SDLK_3 }, EGOCODE_SHOWSTAT, 3 },
    { { SDLK_4 }, EGOCODE_SHOWSTAT, 4 },
    { { SDLK_5 }, EGOCODE_SHOWSTAT, 5 },
    { { SDLK_6 }, EGOCODE_SHOWSTAT, 6 },
    { { SDLK_7 }, EGOCODE_SHOWSTAT, 7 },
    { { SDLK_7 }, EGOCODE_SHOWSTAT, 8 },
    /* Rest of buffer is for user-configured input */
    /* Fill in starts at first 'EGOCODE_JUMP' */
    /* Default values for Player[1], in case of error reading the 'controls' */
    { { SDLK_KP0 }, EGOCODE_JUMP,       1 },
    { { SDLK_t  }, EGOCODE_LEFT_USE,    1 },
    { { SDLK_g }, EGOCODE_LEFT_GET,     1 },
    { { SDLK_b }, EGOCODE_LEFT_PACK,    1 },
    { { SDLK_y }, EGOCODE_RIGHT_USE,    1 },
    { { SDLK_h }, EGOCODE_RIGHT_GET,    1 },
    { { SDLK_n }, EGOCODE_RIGHT_PACK,   1 },
    { { SDLK_LSHIFT }, EGOCODE_SNEAK_ON, 0, EGOCODE_SNEAK_OFF },
    { { SDLK_KP7 }, EGOCODE_CAMERA_LEFT,  1 },
    { { SDLK_KP9 }, EGOCODE_CAMERA_RIGHT, 1 },
    { { SDLK_m }, EGOCODE_MESSAGE,      1 },
    { { SDLK_KP_PLUS },  EGOCODE_CAMERA_IN ,  1 },
    { { SDLK_KP_MINUS }, EGOCODE_CAMERA_OUT , 1 },
    { { SDLK_KP8 }, EGOCODE_ARROWUP ,     1 },
    { { SDLK_KP2 }, EGOCODE_ARROWDOWN ,   1 },
    { { SDLK_KP4 }, EGOCODE_ARROWLEFT ,   1 },
    { { SDLK_KP6 }, EGOCODE_ARROWRIGHT ,  1 },
    /* ---------- Default values for Player[2] -------- */
    { { SDLGL_KEY_MOUMIDDLE }, EGOCODE_JUMP,         2 },
    { { SDLK_a }, EGOCODE_LEFT_USE, 2 },
    { { SDLGL_KEY_MOURIGHT, SDLGL_KEY_MOUMIDDLE }, EGOCODE_LEFT_GET, 2 },
    { { SDLK_s }, EGOCODE_LEFT_PACK, 2 },
    { { SDLGL_KEY_MOULEFT }, EGOCODE_RIGHT_USE, 2 },
    { { SDLGL_KEY_MOULEFT, SDLGL_KEY_MOURIGHT  }, EGOCODE_RIGHT_GET, 2 },
    { { SDLK_INSERT }, EGOCODE_RIGHT_PACK, 2 },
    { { SDLK_RSHIFT }, EGOCODE_SNEAK_ON, 0, EGOCODE_SNEAK_OFF },
    { { SDLGL_KEY_MOURIGHT }, EGOCODE_CAMERAMODE, 2 },
    { { SDLGL_KEY_MOUMOVE },  EGOCODE_MOVEMENT,   2 },
    /* TODO: Fill in default values for the Joysticks
             in case of error reading the 'controls' */
   /*  {   , 0, {  } }, */
   { { 0 } }

};

/*******************************************************************************
* CODE								                                           *
*******************************************************************************/

/*
 * Name:
 *     inputStr2SDLkey
 * Description:
 *     Translates a given 'key_name' into an SDLK_-value 
 * Input:
 *     key_name *: Pointer on name of key  
 */
static int inputStr2SDLkey(char *key_name)
{ 
    CONTROL_NAME *ct;
    

    ct = &Control_Table[0];
    
    while (ct -> name[0] > 0)
    {
        if (! strcmp(ct -> name, key_name))
        {
            return ct -> sdl_code;
        }

        ct++;
    }
    
    return 0;       /* No valid code */
}

/* ========================================================================== */
/* ======================== PUBLIC FUNCTIONS	============================= */
/* ========================================================================== */

/*
 * Name:
 *     playerInitInput
 * Description:
 *     Fills in the user keys read from file with its name
 * Input:
 *     None
 * Output:
 *     Pointer on command list for handing over to SDLGL 
 */
SDLGL_CMDKEY *playerInitInput(void)
{
    char block_name[20]; 
    char key1[20];
    char key2[20];
    int numkey;
    SDLGL_CMDKEY *ck;
    SDLGLCFG_NAMEDVALUE *nv;    
    int cnt;            /* Number of commands already used  */
    int i;              /* Read four input sets             */
    
    
    /* --- Find the start of the buffer -- */
    ck  = CmdList;
    cnt = 0;

    while(ck -> code > 0)
    {
        if (ck -> code == EGOCODE_JUMP)
        {
            break;
        }

        cnt++;
        ck++;
    }

    /* --- No we have the first position in the command buffer to put the keys */
    sdlglcfgOpenFile("controls.cfg", "[];");

    while (sdlglcfgReadNamedValues(CtrlGame))
    {
        for (i = 0; i < 4; i++)
        {
            sprintf(block_name, "PLAYER%d");

            if (sdlglcfgIsActualBlockName(block_name))
            {
                nv = CtrlGame;

                while(nv -> type > 0)
                {
                    /* Fill in the values into the CMDKEY-Buffer */
                    if (cnt < INPUT_MAXCOMMAND)
                    {
                        numkey = sscanf(nv -> data, "%s %s", key1, key2);     /* Get the keys */

                        ck -> keys[0] = inputStr2SDLkey(key1);

                        if  (numkey > 1)
                        {
                            ck -> keys[1] = inputStr2SDLkey(key2);
                        }

                        /* Now set the input code for this one, and the set the input set  */
                        ck -> code         = (char)nv -> info;
                        ck -> sub_code     = (char)i;
                        ck -> release_code = ck -> code;

                        /* --- Next buffer -- */
                        cnt++;
                        ck++;
                    }
                }

                break;
            }
        }
    }

    return CmdList;
}

/*
 * Name:
 *     playerCreate
 * Description:
 *     Creates a new player
 * Input:
 *     player_no: Number of player to create (init)
 *     char_no:   ///< Which character to handle ?
 * Output:
 *     Success yes/no
 */
char playerCreate(char player_no, int char_no)
{
    if(player_no > 0 && player_no <= PLAYER_MAX)
    {
        Player[player_no].id      = player_no;
        Player[player_no].valid   = player_no;
        Player[player_no].char_no = char_no;
        // Player[player_no].obj_no = charGet()->obj_no;

        return 1;
    }

    return 0;
}

/*
 * Name:
 *     playerRemove
 * Description:
 *     Removes the player from game
 *     @todo: Switch character to AI
 * Input:
 *     player_no: Number of player to remove
 * Output:
 *     Success yes/no
 */
void playerRemove(char player_no)
{
    if(player_no > 0 && player_no <= PLAYER_MAX)
    {
        memset(&Player[player_no], 0, sizeof(PLAYER_T));
    }
}

/*
 * Name:
 *     playerTranslateInput
 * Description:
 *     Translates the input to game commands and calls the game functions
 *     depending on the command.
 *     Uses the pointers set using procedure 'inputSetGameInputTable()'
 * Input:
 *     code:      Input translated from input-set
 *     sub_code:  Number of player/camera to send the codes to
 *     pressed:   Key is pressed yes/no
 *     obj_no:    Number of object to handle
 *     key_mod:   Key modifiers for 'EGOCODE_SHOWSTAT'
 * Output:
 *     Done signal, yes/no
 */
int playerTranslateInput(char code, char sub_code, char pressed, int obj_no, int key_mod)
{


    if (code > 0)
    {
        switch(code)
        {
            case EGOCODE_QUITMODULE:
                return 1;           /* Done with module */
            case EGOCODE_JUMP:
                // @todo: Calculate jump, add ZADD_CODE to manage code
                // sdlgl3dManageObject(Player[input -> subcode].obj_no, char move_cmd, char set);
                break;
            case EGOCODE_LEFT_USE:
            case EGOCODE_LEFT_GET:
            case EGOCODE_LEFT_PACK:
            case EGOCODE_RIGHT_USE:
            case EGOCODE_RIGHT_GET:
            case EGOCODE_RIGHT_PACK:
            case EGOCODE_RESPAWN:
                // @todo: charSetCommand();
                break;

            case EGOCODE_MESSAGE:
                /* Call 'message'-Code */
                break;

            case EGOCODE_CAMERA_LEFT:
            case EGOCODE_CAMERA_RIGHT:
            case EGOCODE_CAMERA_IN:
            case EGOCODE_CAMERA_OUT:
                /* sdlgl3dManageCamera(int camera_no, char move_cmd, char set, char speed_modifier) */
                break;

            case EGOCODE_CAMERAMODE:
                /* Set flag for this player press/release */
                // Player[input -> subcode].cameramode = input -> pressed;
                break;

            case EGOCODE_MOVEMENT:
                /* Mouse or joystick generate this one */
                // @todo: playerTranslateMoveToCamera() ==> camera_move
                /* FIXME: Adjust movement for mouse/joystick */
                // if (Player[input -> subcode].cameramode)
                // {
                    /* sdlgl3dManageCamera(int camera_no, char move_cmd, char set, char speed_modifier) */
                    /* camera_move(input -> movex, input -> movey, input -> subcode);      *
                    /* camera_move(int movex, int movey, int playerno/camerano);      */
                //}
                //else
                // {
                    /* set_one_player_latch(Player[input -> subcode].index, 0, 0, input -> movex, input -> movey) */
                    /* set_one_player_latch(int character_no, unsigned char latchbutton, int movex, int movey); */

                // }
                break;
            case EGOCODE_ARROWUP:
            case EGOCODE_ARROWDOWN:
            case EGOCODE_ARROWLEFT:
            case EGOCODE_ARROWRIGHT:
                /* @todo: Calculate the  movement */
                /* Move the player: Set the objects movement */
                // sdlgl3dManageObject(Player[input -> subcode].obj_no, char move_cmd, char set);
                break;
            case EGOCODE_SHOWSTAT:
                // @todo: Check the 'key modifier'
                /* LEFT SHIFT   + 1 to 7 - Show selected character armor without magic enchants */
                /* render2dShowArmor(input -> subcode) */
                /* LEFT CONTROL + 1 to 7 - Show armor stats with magic enchants included */
                /* render2dShowFullStatus(input -> subcode) */
                /* LEFT ALT     + 1 to 7 - Show character magic enchants    */
                /* render2dShowMagicStatus(input -> subcode) */
                /* Show character detailed stats */
                /* render2dShowStat(input -> subcode) */
                break;
            case EGOCODE_GIVEEXPERIENCE:
                /* SDLK_x: SDLK_1 / SDLK_2 */
                /* Experience cheat */
                /* charGiveExperience(Player[sub_code].char_no, 25, XPDIRECT); */
                break;
            case EGOCODE_MAPCHEAT:
                /* ----------- */
                /* SDLKEYDOWN( SDLK_m ) && SDLKEYDOWN( SDLK_LSHIFT ) */
                /* mapon = mapvalid; youarehereon = btrue; */
                break;
            case EGOCODE_LIFECHEAT:
                /* SDLKEYDOWN( SDLK_z ) + SDLKEYDOWN( SDLK_1 ) */
                /* @todo: charApply(Player[sub_code].char_no, CHAR_ADDLIFE, 128); */
                break;
        } /* switch */
    } /* if (code > 0) */
    
    return 0;
}


