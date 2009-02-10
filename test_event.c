/*******************************************************************************
*  TESTBED.C                                                                   *
*	   - Main Function with Open-GL for Tests with new Egoboo-Modules          *
*                                                                              *
*   EGOBOO			                                                         *
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

/******************************************************************************
* INCLUDES                                                                    *
******************************************************************************/

#include <SDL.h>
#include <stdlib.h>                     /* malloc */
#include <memory.h>


/* --- Include own headers -------- */
#include "sdlgl.h"          /* Init and shutdown of SDL using OpenGL */
#include "sdlglstr.h"       /* Drawing strings with bitmapped fonts  */


#include "event.h"          /* This one has to be tested */


/******************************************************************************
* DEFINES                                                                     *
******************************************************************************/

#define SDLGL_DBLCLICKTIME    56    /* Ticks                           */
#define SDLGL_INPUTTIME       56    /* Ticks to wait between input     */
                                    /* Translation                     */


/******************************************************************************
* DATA                                                                        *
******************************************************************************/

static char Edit_Input[256];
static int Cursor = 0;


/******************************************************************************
* CODE                                                                        *
******************************************************************************/

/*
 * Name:
 *     testbedPrintString
 * Description:
 *     Prints a string
 * Input:
 *      buffer *: Draw this string
 */
static void testbedPrintString(int ypos, char *buffer)
{

    static SDLGL_RECT StringPos = { 40, 40, 500, 30 };


    StringPos.y = ypos;

    sdlglstrDrawRectColNo(&StringPos, SDLGLSTR_COL_BLACK, 1);
    sdlglstrSetColorNo(SDLGLSTR_COL_YELLOW);
    sdlglstrString(&StringPos, buffer);

}

/*
 * Name:
 *     testbedEditString
 * Description:
 *     Adds a char from event to the edit string.
 * Input:
 *     event *: Take char from this one
 */
static void testbedEditString(EVENT_INFO *event)
{

    if (Cursor < 254) {

        Edit_Input[Cursor] = event -> textkey;
        Cursor++;

    }

}

/*
 * Name:
 *     testbedDrawScreen
 * Description:
 *     Draws the screen
 * Input:
 *      event *: For output, of generated events
 */
static void testbedDrawScreen(EVENT_INFO *event)
{

    static PauseGame = 0;

    char buffer[256];


    /* glClear(GL_COLOR_BUFFER_BIT);  */         /* Simply clear the screen  */

    if (event -> code > 0) {

        switch(event -> code) {

            case EGOCODE_JUMP:
                sprintf(buffer, "EGOCODE_JUMP: Player[%d]", event -> subcode);
                break;
            case EGOCODE_LEFT_USE:
                sprintf(buffer, "EGOCODE_LEFT_USE: Player[%d]", event -> subcode);
                break;
            case EGOCODE_LEFT_GET:
                sprintf(buffer, "EGOCODE_LEFT_GET: Player[%d]", event -> subcode);
                break;
            case EGOCODE_LEFT_PACK:
                sprintf(buffer, "EGOCODE_LEFT_PACK: Player[%d]", event -> subcode);
                break;
            case EGOCODE_RIGHT_USE:
                sprintf(buffer, "EGOCODE_RIGHT_USE: Player[%d]", event -> subcode);
                break;
            case EGOCODE_RIGHT_GET:
                sprintf(buffer, "EGOCODE_RIGHT_GET: Player[%d]", event -> subcode);
                break;
            case EGOCODE_RIGHT_PACK:
                sprintf(buffer, "EGOCODE_RIGHT_PACK: Player[%d]", event -> subcode);
                break;
            case EGOCODE_RESPAWN:
                sprintf(buffer, "EGOCODE_RESPAWN");
                break;
            
            case EGOCODE_MESSAGE:  
                /* Call 'message'-Code */
                sprintf(buffer, "EGOCODE_MESSAGE: %d", event -> code);
                break;
            case EGOCODE_CAMERA_LEFT:
                sprintf(buffer, "EGOCODE_CAMERA_LEFT: %d", event -> code);
                break;
            case EGOCODE_CAMERA_RIGHT:
                sprintf(buffer, "EGOCODE_CAMERA_RIGHT: %d", event -> code);
                break;
            case EGOCODE_CAMERA_IN:
                sprintf(buffer, "EGOCODE_CAMERA_IN: %d", event -> code);
                break;
            case EGOCODE_CAMERA_OUT:
                sprintf(buffer, "EGOCODE_CAMERA_OUT: %d", event -> code);
                break;
            case EGOCODE_CAMERAMODE:
                sprintf(buffer, "EGOCODE_CAMERAMODE: %d", event -> pressed);
                break;
            case EGOCODE_MOVEMENT:
                sprintf(buffer, "EGOCODE_MOVEMENT: Player[%d]: move X: %d move Y: %d ",
                        event -> subcode, event -> movex, event -> movey);
                break;
            case EGOCODE_ARROWUP:
                sprintf(buffer, "EGOCODE_ARROWUP");
                break;
            case EGOCODE_ARROWDOWN:
                sprintf(buffer, "EGOCODE_ARROWDOWN");
                break;
            case EGOCODE_ARROWLEFT:
                sprintf(buffer, "EGOCODE_ARROWLEFT");
                break;
            case EGOCODE_ARROWRIGHT:
                sprintf(buffer, "EGOCODE_ARROWRIGHT");
                /* TODO: Calculate the  movement */
                /* set_one_player_latch(int character_no, unsigned char latchbutton, int movex, int movey); */
                /* set_one_player_latch(Player[event.subcode].index, 0, event.movex, event.movey); */
                break;
            case EGOCODE_SHOWSTAT_LSHIFT:
                sprintf(buffer, "EGOCODE_SHOWSTAT_LSHIFT: %d", event -> subcode);
                break;
            case EGOCODE_SHOWSTAT_LCTRL:
                sprintf(buffer, "EGOCODE_SHOWSTAT_LCTRL: %d", event -> subcode);
                break;
            case EGOCODE_SHOWSTAT_LALT:
                sprintf(buffer, "EGOCODE_SHOWSTAT_LALT: %d", event -> subcode);
                break;
            case EGOCODE_SHOWSTAT:
                sprintf(buffer, "EGOCODE_SHOWSTAT: %d", event -> subcode);
                break;
            case EGOCODE_GIVEEXPERIENCE:
                sprintf(buffer, "EGOCODE_GIVEEXPERIENCE %d", event -> subcode);
                /* SDLK_x: SDLK_1 / SDLK_2 */
                break;
            case EGOCODE_MAPCHEAT:
                /* ----------- */
                /* SDLKEYDOWN( SDLK_m ) && SDLKEYDOWN( SDLK_LSHIFT ) */
                /* mapon = mapvalid; youarehereon = btrue; */
                sprintf(buffer, "EGOCODE_MAPCHEAT");
                break;
            case EGOCODE_LIFECHEAT:
                /* SDLKEYDOWN( SDLK_z ) + SDLKEYDOWN( SDLK_1 ) */
                /* chrlife[plaindex[0]] += 128; */
                sprintf(buffer, "EGOCODE_LIFECHEAT");
                break;

            case EGOCODE_PAUSEGAME:
                PauseGame ^= 1;
                sprintf(buffer, "EGOCODE_PAUSEGAME: %d", PauseGame);
                break;

        } /* switch */

        testbedPrintString(40, buffer);

    } /* if (event.code) */

    /* Print the 'Edit_String' */
    if (event -> textkey) {

        testbedEditString(event);
        testbedPrintString(80, Edit_Input);

    }
    
    buffer[0] = event -> textkey;
    buffer[1] = 0;
    testbedPrintString(120, buffer);         /* Print the char generated */

}

/*
 * Name:
 *     testbedMainLoop
 * Description:
 *     - sets up an 2D-OpenGL screen with the size and colordepth
 *	 given in the config struct.
 *     - Enters a draw / get-input loop until the used function
 *	 (the installed inputhandler returns "SDLGL_INPUTEXIT") asks
 * 	 for the exit of the loop.
 * Input:
 *	config:       The graphics configuration.
 * Output:
 *	exitcode for users "main()" function
 */
static int testbedMainLoop(void)
{

    static int fpscount = 0;	/* FPS-Counter 						*/
    static int fpsclock = 0;	/* Clock for FPS-Counter			*/
    static int fps = 0;			/* Holds FPS until next FPS known	*/

    EVENT_INFO event;
    int   done;
    unsigned int mainclock,
	             newmainclock;

    int    tickspassed; /* To hand to the Main-Function. Helps the */
                        /* main function to hold animations and so */
                        /* on frame independent			  */


    /* Set the flag to not done */
    done = 0;

    /* Now init the clock */
    mainclock = SDL_GetTicks();
    /* ---------- Set the Input-Table -------- */
    eventSetGameInputTable();
    sdlglstrSetFont(SDLGLSTR_FONT8);

    /* Loop, drawing and checking events */
    while (! done) {

    	newmainclock = SDL_GetTicks();

        /* Update the clock  */
        tickspassed = newmainclock - mainclock;
        /* ------------ Count the frames per second ----------- */
        fpscount++;
        fpsclock += tickspassed;

        if (fpsclock >= SDLGL_TICKS_PER_SECOND) {

            fps      = fpscount;
            fpscount = 0;
            fpsclock -= SDLGL_TICKS_PER_SECOND;

        }
          
        
        /* ------ Gather and translate user input -------- */
        done = eventTranslateGameInput(&event);
        
        event.tickspassed   = tickspassed;
        event.secondspassed = (float)tickspassed / (float)SDLGL_TICKS_PER_SECOND;
        event.fps           = fps;
        /* FIRST: Check for emergency exit */
        if (event.sdlcode == SDLK_ESCAPE) {

            done = 1;

        }

        /* ----------------- Do the 'Edit'-Thing --------------- */
        testbedEditString(&event);
        /* ----------------- Draw the screen  ------------------ */
        testbedDrawScreen(&event);
                        
        /* ----------------- Display it ------------------------ */
        glFlush();

        /* swap buffers to display, since we're double buffered.	*/
        SDL_GL_SwapBuffers();

        mainclock = newmainclock;

    }

    return 1;

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
int main(int argc, char **argv)
{

    sdlglInit(0);
    testbedMainLoop();
    sdlglShutdown();

    return 0;
    
}

