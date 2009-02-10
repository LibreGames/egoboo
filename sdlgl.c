/*******************************************************************************
*   SDLGL.C                      	                                           *
*       A simple Main-Function to separate calls to SDL from the rest of       *
*	the code. Initializes and maintains An OpenGL window                       *
*                                                                              *
*   Initial code: January 2001, Paul Mueller                                   *
*									                                           *
*   Copyright (C) 2001-2008  Paul Müller <pmtech@swissonline.ch>                    *
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
*                                                                              *
*                                                                              *
* Last change: 2008-06-21                                                      *
*******************************************************************************/


/******************************************************************************
* INCLUDES                                                                    *
******************************************************************************/

/* Make sure that _MACOS is defined in MetroWerks compilers */

#if defined( __MWERKS__ ) && defined( macintosh )
	#define	_MACOS
#endif


#include <SDL.h>
#include <stdlib.h>                     /* malloc */
#include <memory.h>


/* --- Include own header -------- */
#include "sdlgl.h"


/******************************************************************************
* DEFINES                                                                     *
******************************************************************************/

#define SDLGL_STANDARD_INPUT  1
#define SDLGL_POPUP_INPUT     2
#define SDLGL_MAXINPUTSTACK   32   	/* Maximum depth of stack 	       */

#define SDLGL_DBLCLICKTIME    56    /* Ticks                           */
#define SDLGL_INPUTTIME       56    /* Ticks to wait between input     */
                                    /* Translation                     */

/******************************************************************************
* TYPEDEFS                                                                    *
******************************************************************************/


/******************************************************************************
* FORWARD DECLARATIONS                                                        *
******************************************************************************/

static void sdlglIDefaultDrawingFunc(SDLGL_FIELD *fields, SDLGL_EVENT *event);
static int  sdlglIDefaultInputFunc(SDLGL_EVENT *event);

/******************************************************************************
* DATA                                                                        *
******************************************************************************/

static GLubyte stdcolors[256 * 4] = {

    0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0xAA, 0xFF,  0x00, 0xAA, 0x00, 0xFF, /* 0..2 */
    0x00, 0xAA, 0xAA, 0xFF,  0xAA, 0x00, 0x00, 0xFF,  0xAA, 0x00, 0xAA, 0xFF,
    0xAA, 0xAA, 0x00, 0xFF,  0xAA, 0xAA, 0xAA, 0xFF,  0x55, 0x55, 0x55, 0xFF,
    0x55, 0x55, 0xFF, 0xFF,  0x55, 0xFF, 0x55, 0xFF,  0x55, 0xFF, 0xFF, 0xFF,
    0xFF, 0x55, 0x55, 0xFF,  0xFF, 0x55, 0xFF, 0xFF,  0xFF, 0xFF, 0x55, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF,  0xFC, 0xC8, 0x64, 0xFF,  0xF0, 0x48, 0x44, 0xFF, /* 15..17 */
    0xDC, 0x30, 0x2C, 0xFF,  0xCC, 0x1C, 0x18, 0xFF,  0xB8, 0x08, 0x08, 0xFF,
    0xFC, 0x64, 0x64, 0xFF,  0x8C, 0x04, 0x14, 0xFF,  0x70, 0x08, 0x20, 0xFF,
    0x54, 0x08, 0x24, 0xFF,  0x38, 0x08, 0x08, 0xFF,  0x1C, 0x04, 0x04, 0xFF,
    0xFC, 0xE4, 0xA0, 0xFF,  0xF4, 0xCC, 0x78, 0xFF,  0xE4, 0xBC, 0x68, 0xFF,
    0xD0, 0xA8, 0x54, 0xFF,  0xC0, 0x94, 0x48, 0xFF,  0xE4, 0x90, 0x74, 0xFF,
    0xD0, 0x78, 0x64, 0xFF,  0xBC, 0x64, 0x54, 0xFF,  0xA0, 0x4C, 0x44, 0xFF,
    0x88, 0x38, 0x38, 0xFF,  0x78, 0x34, 0x34, 0xFF,  0x64, 0x28, 0x2C, 0xFF,
    0x50, 0x20, 0x24, 0xFF,  0x40, 0x18, 0x1C, 0xFF,  0x00, 0x18, 0x0C, 0xFF, /* 39..41 */
    0xFC, 0xE8, 0xC0, 0xFF,  0xB4, 0x88, 0x3C, 0xFF,  0xF0, 0xEC, 0x44, 0xFF,
    0xE0, 0xD8, 0x30, 0xFF,  0xD4, 0xC4, 0x20, 0xFF,  0xC8, 0xB4, 0x10, 0xFF,
    0xC4, 0xE0, 0x8C, 0xFF,  0xA8, 0xCC, 0x78, 0xFF,  0x90, 0xBC, 0x68, 0xFF,
    0x78, 0xA8, 0x58, 0xFF,  0x60, 0x98, 0x4C, 0xFF,  0x4C, 0x84, 0x3C, 0xFF,
    0x38, 0x74, 0x34, 0xFF,  0x28, 0x64, 0x28, 0xFF,  0xD8, 0xD8, 0xFC, 0xFF,
    0xCC, 0xCC, 0xFC, 0xFF,  0xBC, 0xBC, 0xF4, 0xFF,  0xB8, 0x94, 0x0C, 0xFF, /* 57..59 */
    0xA4, 0x84, 0x0C, 0xFF,  0x90, 0x74, 0x08, 0xFF,  0x7C, 0x64, 0x08, 0xFF,
    0x68, 0x50, 0x04, 0xFF,  0xD0, 0xFC, 0xD0, 0xFF,  0x4C, 0xE8, 0x40, 0xFF,
    0x44, 0xD4, 0x34, 0xFF,  0x40, 0xC0, 0x28, 0xFF,  0x3C, 0xB0, 0x1C, 0xFF,
    0xA0, 0xFC, 0xA0, 0xFF,  0x00, 0x8C, 0x10, 0xFF,  0x04, 0x74, 0x18, 0xFF,
    0x04, 0x5C, 0x20, 0xFF,  0x04, 0x44, 0x1C, 0xFF,  0x04, 0x2C, 0x18, 0xFF,
    0xF8, 0xCC, 0xCC, 0xFF,  0xF4, 0xAC, 0xAC, 0xFF,  0xF0, 0x90, 0x90, 0xFF,
    0xFC, 0x74, 0x74, 0xFF,  0xCC, 0xEC, 0x90, 0xFF,  0xFC, 0xAC, 0x68, 0xFF,
    0xF0, 0xA0, 0x58, 0xFF,  0xD8, 0x90, 0x50, 0xFF,  0xC4, 0x84, 0x48, 0xFF,
    0xB0, 0x78, 0x40, 0xFF,  0x9C, 0x68, 0x34, 0xFF,  0x8C, 0x58, 0x30, 0xFF,  /* 84..86 */
    0x7C, 0x48, 0x2C, 0xFF,  0x68, 0x38, 0x2C, 0xFF,  0x54, 0x30, 0x24, 0xFF,  /* 87..89 */
    0x40, 0x20, 0x20, 0xFF,  0xFC, 0xA0, 0xCC, 0xFF,  0xE8, 0x74, 0xB8, 0xFF,  /* 90..92 */
    0xD8, 0x4C, 0xAC, 0xFF,  0xC4, 0x2C, 0xA8, 0xFF,  0xB4, 0x10, 0xA8, 0xFF,  /* 93..95 */
    0xD8, 0xB0, 0x8C, 0xFF,  0xC8, 0xA0, 0x80, 0xFF,  0xB8, 0x94, 0x78, 0xFF,  /* 96..98 */
    0xA8, 0x88, 0x6C, 0xFF,  0x9C, 0x7C, 0x64, 0xFF,  0x8C, 0x70, 0x5C, 0xFF,  /* 99..102 */
    0x7C, 0x64, 0x50, 0xFF,  0x6C, 0x54, 0x44, 0xFF,  0x5C, 0x44, 0x38, 0xFF,  /* 102..104*/
    0x4C, 0x38, 0x2C, 0xFF,  0xDC, 0x58, 0x1C, 0xFF,  0xF8, 0xAC, 0xD0, 0xFF,  /* 105..107 */
    0x84, 0x04, 0x94, 0xFF,  0x68, 0x08, 0x80, 0xFF,  0x50, 0x08, 0x6C, 0xFF,
    0x3C, 0x0C, 0x58, 0xFF,  0xFC, 0xB4, 0x2C, 0xFF,  0xEC, 0xA4, 0x24, 0xFF,
    0xDC, 0x94, 0x1C, 0xFF,  0xCC, 0x88, 0x18, 0xFF,  0xBC, 0x7C, 0x14, 0xFF,
    0xA4, 0x6C, 0x1C, 0xFF,  0x94, 0x5C, 0x0C, 0xFF,  0x7C, 0x50, 0x18, 0xFF,
    0x64, 0x40, 0x18, 0xFF,  0x50, 0x34, 0x18, 0xFF,  0x38, 0x24, 0x14, 0xFF,
    0x24, 0x18, 0x0C, 0xFF,  0x08, 0x04, 0x00, 0xFF,  0xF4, 0xC0, 0xA0, 0xFF,
    0xE0, 0xA0, 0x84, 0xFF,  0xCC, 0x84, 0x6C, 0xFF,  0xAC, 0xB4, 0xE0, 0xFF,
    0x9C, 0xA0, 0xD4, 0xFF,  0x88, 0x90, 0xCC, 0xFF,  0x7C, 0x80, 0xC4, 0xFF,
    0x64, 0x68, 0xB4, 0xFF,  0x50, 0x54, 0xA4, 0xFF,  0x3C, 0x40, 0x94, 0xFF,
    0x2C, 0x30, 0x88, 0xFF,  0xE4, 0xC8, 0x38, 0xFF,  0xC8, 0xA0, 0x34, 0xFF,  /* 142..144 */
    0xAC, 0x7C, 0x30, 0xFF,  0x90, 0x60, 0x2C, 0xFF,  0x78, 0x48, 0x28, 0xFF,  /* 145..147 */
    0xC8, 0x8C, 0x68, 0xFF,  0xA8, 0x78, 0x54, 0xFF,  0x8C, 0x60, 0x44, 0xFF,
    0x50, 0xC4, 0xFC, 0xFF,  0x50, 0xA4, 0xFC, 0xFF,  0x50, 0x88, 0xFC, 0xFF,
    0x50, 0x6C, 0xFC, 0xFF,  0xA4, 0xFC, 0xFC, 0xFF,  0x38, 0x3C, 0xDC, 0xFF,
    0x20, 0x28, 0xC8, 0xFF,  0x50, 0xE0, 0xFC, 0xFF,  0x00, 0x04, 0x80, 0xFF,
    0x00, 0x04, 0x58, 0xFF,  0x00, 0x04, 0x30, 0xFF,  0x28, 0x10, 0x10, 0xFF,
    0x10, 0x04, 0x04, 0xFF,  0x98, 0x68, 0x4C, 0xFF,  0x7C, 0x50, 0x3C, 0xFF,
    0x60, 0x3C, 0x2C, 0xFF,  0xF0, 0xE8, 0xE8, 0xFF,  0xE0, 0xD8, 0xD0, 0xFF,
    0xD0, 0xC4, 0xBC, 0xFF,  0xBC, 0xAC, 0xA4, 0xFF,  0xA4, 0x98, 0x90, 0xFF,
    0x8C, 0x80, 0x80, 0xFF,  0x74, 0x70, 0x70, 0xFF,  0x5C, 0x5C, 0x58, 0xFF,
    0x44, 0x44, 0x44, 0xFF,  0x2C, 0x2C, 0x2C, 0xFF,  0x18, 0x18, 0x18, 0xFF,
    0x0C, 0x0C, 0x0C, 0xFF,  0x00, 0x00, 0x1C, 0xFF,  0xF4, 0xA4, 0x74, 0xFF,
    0xDC, 0x94, 0x5C, 0xFF,  0xC8, 0x84, 0x48, 0xFF,  0xE0, 0xE0, 0xE0, 0xFF,
    0xD0, 0xD0, 0xD8, 0xFF,  0xC0, 0xC4, 0xD0, 0xFF,  0xB4, 0xB8, 0xC8, 0xFF,
    0xA4, 0xA4, 0xB8, 0xFF,  0x94, 0x94, 0xAC, 0xFF,  0x7C, 0x7C, 0x98, 0xFF,
    0x6C, 0x6C, 0x88, 0xFF,  0x58, 0x58, 0x74, 0xFF,  0x44, 0x44, 0x60, 0xFF,
    0x34, 0x34, 0x50, 0xFF,  0x24, 0x24, 0x3C, 0xFF,  0x18, 0x18, 0x2C, 0xFF,
    0xB0, 0x74, 0x40, 0xFF,  0x94, 0x6C, 0x40, 0xFF,  0x7C, 0x60, 0x3C, 0xFF,
    0xB8, 0x98, 0x98, 0xFF,  0xAC, 0x88, 0x84, 0xFF,  0x98, 0x74, 0x6C, 0xFF,
    0x88, 0x60, 0x58, 0xFF,  0x74, 0x4C, 0x44, 0xFF,  0x64, 0x3C, 0x34, 0xFF,
    0x54, 0x30, 0x28, 0xFF,  0x3C, 0x20, 0x18, 0xFF,  0x28, 0x14, 0x0C, 0xFF,
    0x14, 0x08, 0x04, 0xFF,  0x08, 0x04, 0x00, 0xFF,  0x00, 0x00, 0x0C, 0xFF,
    0xFC, 0xFC, 0xFC, 0xFF,  0x70, 0x58, 0x38, 0xFF,  0x60, 0x48, 0x28, 0xFF,
    0x50, 0x3C, 0x1C, 0xFF,  0xD4, 0x84, 0x60, 0xFF,  0xC0, 0x78, 0x58, 0xFF,
    0xB8, 0x6C, 0x4C, 0xFF,  0xAC, 0x64, 0x44, 0xFF,  0x9C, 0x5C, 0x40, 0xFF,
    0x8C, 0x54, 0x3C, 0xFF,  0x80, 0x50, 0x38, 0xFF,  0x78, 0x48, 0x34, 0xFF,
    0x6C, 0x40, 0x2C, 0xFF,  0x60, 0x38, 0x24, 0xFF,  0x54, 0x2C, 0x1C, 0xFF,
    0x4C, 0x28, 0x18, 0xFF,  0xFC, 0xFC, 0xFC, 0xFF,  0xFC, 0xFC, 0xFC, 0xFF,
    0xFC, 0xFC, 0xFC, 0xFF,  0xFC, 0xFC, 0xFC, 0xFF,
    /* ---------- */
    0xCC, 0xF0, 0xD0, 0xFF,  0xBC, 0xDC, 0xBC, 0xFF,  0xB0, 0xC8, 0xAC, 0xFF,
    0xA0, 0xB4, 0x98, 0xFF,  0x88, 0xA0, 0x84, 0xFF,  0x78, 0x8C, 0x74, 0xFF,
    0x64, 0x78, 0x60, 0xFF,  0x54, 0x64, 0x54, 0xFF,  0x40, 0x50, 0x40, 0xFF,
    0x2C, 0x38, 0x2C, 0xFF,  0x14, 0x18, 0x14, 0xFF,  0x08, 0x0C, 0x04, 0xFF,
    0xFC, 0xFC, 0xFC, 0xFF,  0xFC, 0xFC, 0xFC, 0xFF,  0xFC, 0xFC, 0xFC, 0xFF,
    0xFC, 0xFC, 0xFC, 0xFF,  0x84, 0x68, 0x58, 0xFF,  0x7C, 0x44, 0x24, 0xFF,
    0x70, 0x58, 0x48, 0xFF,  0x68, 0x34, 0x14, 0xFF,  0x5C, 0x48, 0x38, 0xFF,
    0x54, 0x2C, 0x0C, 0xFF,  0x48, 0x38, 0x2C, 0xFF,  0x40, 0x20, 0x04, 0xFF,
    0x34, 0x28, 0x20, 0xFF,  0x2C, 0x14, 0x00, 0xFF,  0x20, 0x18, 0x14, 0xFF,
    0x1C, 0x0C, 0x00, 0xFF,  0x10, 0x0C, 0x08, 0xFF,  0x0C, 0x04, 0x00, 0xFF,
    0x00, 0x00, 0x00, 0xFF,  0xA8, 0xA8, 0xA8, 0xFF

};


static SDLGL_CONFIG mainConfig = {

    /* If no user input is given, use standard values */
    "sdlgl-library V 0.8 - Build(" __DATE__  ")", /* Caption for window, if any */
    640, 480,			/* Size of screen	                 */
    32,					/* Colordepth of screen	             */
    0,					/* Draw mode: wireframe or filled    */
    0, 					/* Hide mouse, if asked for	         */
    0,					/* Windowed                          */
    1,					/* Use standard color settings       */
    					/* If debugmode yes/no	      	     */
    0,                  /* No Z-Buffer                       */
    SDLGL_DBLCLICKTIME, /* Time for sensing of double clicks */
    640, 480

};

/* -------- Data for inputstack ------------- */
static SDLGL_CMDKEY DefaultCommandKeys[2];
static SDLGL_FIELD  DefaultFields[2];

static SDLGL_FIELD DefaultFocusField;
static SDLGL_FIELD *InputFocus = &DefaultFocusField;    /* For focus    */
static SDLGL_FIELD *MouseOver  = &DefaultFocusField;    /* Never NULL   */
static SDLGL_EVENT ActualEvent;

static struct {

    int type;                           /* Type of input, abse or popup     */
    SDLGL_FIELD        *InputFields;    /* Actual base of input fields      */
                                        /* used for input checking          */
    SDLGL_CMDKEY       *CmdKeys;	    /* Actual base of shortcut keys	    */
    SDLGL_DRAWFUNC     DrawFunc;        /* Function to use for drawing      */
    SDLGL_INPUTFUNC    InputFunc;       /* Function for input translation   */
    int numfields;                      /* Number of fields in 'InputFields'*/  

} InputStack[SDLGL_MAXINPUTSTACK + 1] = {

    { SDLGL_STANDARD_INPUT,
      &DefaultFields[0],		/* No inputdesc defined		        */
      &DefaultCommandKeys[0],   /* No shortcut keys defined		    */
      sdlglIDefaultDrawingFunc, /* is available.			            */
      sdlglIDefaultInputFunc

    },
    { SDLGL_STANDARD_INPUT,
      &DefaultFields[0],		 /* No inputdesc defined		        */
      &DefaultCommandKeys[0],    /* No shortcut keys defined		    */
      sdlglIDefaultDrawingFunc,  /* is available.			            */
      sdlglIDefaultInputFunc

    }

};

static int InputStackIdx = 1;	    /* StackIndex 0 is the default handler */

/*******************************************************************************
*  CODE									                                       *
*******************************************************************************/


/*
 * Name:
 *     mainIPosInRect
 * Description:
 *     Checks if the given x,y position is in the given mouvec-rect
 * Input:
 *     ia:     Pointer on a menuinfo structure
 *     inpvec: Pointer on inputvec which holds the rectangle to check
 * Output:
 *     1: if Position in rect
 *     0: if not
 *     inpevent: the xrel + yrel are set to the relative position in the
 *		 checked rectangle.
 */
static int mainIPosInRect(SDLGL_FIELD *id, SDLGL_EVENT *event)
{

    int relx,
        rely;		/* Relative position */


    /* First check horizontally */
    relx = event -> relx - id -> rect.x;


    if (relx >= 0) {			  /* Is right of X 		*/

        if (relx > id -> rect.w) {

            return 0;			  /* Right side out of Rectangle */
            
        }

        /* Now check vertically */
        rely = event -> rely - id -> rect.y;

    	if (rely >= 0) {		  /* Is lower of Y-Pos */

            if (rely <= id -> rect.h) {

                event -> relx = relx;
                event -> rely = rely;
            	return 1;			    /* Is in Rectangle   */

            }

    	}

    }
    
    return 0;

}


/*
 * Name:
 *     mainloopITranslateCmdKey
 * Name:
 *     Checks the key on validity, including check for modifier keys.
 *     If the key in SDLGL_EVENT is the one looked for, the SDLGL_EVENT
 *     struct is modified to type "SDLGL_INPUT_COMMAND".
 * Input:
 *     event:   Pointer on SDLGL_FIELD which holds the actual key and the
 *  	        key modifiers.
 *     sk *:    Pointer on command key to check against sdlcode
 * Output:
 *     > o if a valid command was generated
 */
static int mainloopITranslateCmdKey(SDLGL_EVENT *event, SDLGL_CMDKEY *sk)
{

    int i;
    unsigned char mask;


    /* Check for special keys */
    switch(event -> sdlcode) {

        case SDLK_RCTRL:
        case SDLK_LCTRL:
             event -> sdlcode = SDLGL_KEY_CTRL;
             break;
        case SDLK_RALT:
        case SDLK_LALT:
             event -> sdlcode = SDLGL_KEY_ALT;
             break;

        case SDLK_RSHIFT:
        case SDLK_LSHIFT:
             event -> sdlcode = SDLGL_KEY_SHIFT;
             break;

    }

    /* Now compare the command keys */
    for (i = 0, mask = 0x01; i < 4; i++, mask <<= 1) {

        if (sk -> keys[i] > 0) {

            sk -> mask |= mask;     /* Set the test mask */

            if (sk -> keys[i] == event -> sdlcode) {

                if (event -> pressed) {

                    sk -> pressed |= mask;

                }
                else {

                    /* Combined key becomes invalid */
                    sk -> pressed &= (unsigned char)~mask;
                    if (sk -> releasecode) {

                        event -> type    = SDLGL_INPUT_COMMAND;
                        event -> code    = sk -> releasecode;
                        event -> subcode = sk -> subcode;
                        return 1;

                    }
                    else {

                        event -> type = 0;
                        return 0;

                    }

                }

            }   /* if (sk -> keys[i] == event -> sdlcode)  */

        }

    } /* for i */

    if (sk -> pressed == (sk -> mask & 0x0F)) {

        event -> type    = SDLGL_INPUT_COMMAND;
        event -> code    = sk -> code;
        event -> subcode = sk -> subcode;
        return 1;

    }

    return 0;

}


/*
 * Name:
 *     mainloopITranslateKeyboardInput
 * Description:
 *     Translates the keyboard input, using the inputdescs, if any are
 *     available. Otherwise the "event" is left unchanged.
 * Input:
 *     event: Pointer on an inputevent, holding the key variables.
 */
static int mainloopITranslateKeyboardInput(SDLGL_EVENT *event)
{

    SDLGL_CMDKEY *cmdkeys;
    SDLGL_FIELD  *fields;

    /* First look for shortcuts from SDLGL_FIELD fields */
    fields = InputStack[InputStackIdx].InputFields;
    if (fields && event -> pressed) {

        while(fields -> type) {

            if (fields -> shortcut > 0) {

                if (event -> sdlcode == fields -> shortcut) {

                    /* Key is single shortcut for this  field */ 
                    event -> type    = SDLGL_INPUT_COMMAND;
                    event -> code    = fields -> code;
                    event -> subcode = fields -> subcode;
                    event -> field   = fields;
                    return 1;

                }

            }

            fields++;

        }

    }
    /* Second look for shortcut keys */
    cmdkeys = InputStack[InputStackIdx].CmdKeys;

    if (cmdkeys) {

        while (cmdkeys -> code > 0) {

            if (mainloopITranslateCmdKey(event, cmdkeys)) {

                return 1;

            }

            cmdkeys++;

        }

    }

    event -> type    = SDLGL_INPUT_KEYBOARD;
    event -> code    = 0;
    event -> subcode = 0;
    return 0;

}

/*
 * Name:
 *     mainloopITranslateMouseInput
 * Description:
 *     Translates the mouse input, using the inputdescs, if any are
 *     available. Otherwise the "event" is left unchanged. If an overlapped
 *     rectangle is hit by the mouse, the first one which has a value in
 *     the 'code' field is  the one which generates the command.
 * Input:
 *     event:     Pointer on an inputevent, holding the mouse variables.
 *     inputdesc: Pointer onSDLGL_INPUT's to check for mouse input.
 *     moved:     Called by mouse move, yes/no
 * Output:
 *     Return code
 */
static void mainloopITranslateMouseInput(SDLGL_EVENT *event, SDLGL_FIELD *fields, int moved)
{

    SDLGL_FIELD *base;              /* Save base    */


    event -> field = 0;             /* Assume none  */
    /* Translate it as key, too. For combined input */
    switch(event -> modflags) {

        case SDL_BUTTON_LEFT:
            event -> sdlcode = SDLGL_KEY_MOULEFT;
            break;
        case SDL_BUTTON_MIDDLE:
            event -> sdlcode = SDLGL_KEY_MOUMIDDLE;
            break;

        case SDL_BUTTON_RIGHT:
            event -> sdlcode = SDLGL_KEY_MOURIGHT;
            break;

    }

    /* Adjust the mouse position to the graphics size of screen */
    event -> relx = event -> relx * mainConfig.displaywidth / mainConfig.scrwidth;
    event -> rely = event -> rely * mainConfig.displayheight / mainConfig.scrheight;

    base = fields;

    if (fields) {

        /* Search from top to bottom */
        while (fields -> type != 0) {

            fields++;       /* Points on trailing type 0 */

        }

        while (fields > base) {

            fields--;

            /* if the mouse has to be checked */
            if (fields -> code && mainIPosInRect(fields, event)) {

                event -> code    = fields -> code;
                event -> subcode = fields -> subcode;

                /* Reset flag for mouse over */
                MouseOver -> fstate &= (unsigned char)(~SDLGL_FSTATE_CLEAR);
                /* Set new mouse over flag   */
                MouseOver = fields;

                MouseOver -> fstate |= SDLGL_FSTATE_MOUSEOVER;
                if (moved) {

                    MouseOver -> fstate |= SDLGL_FSTATE_MOUDRAG; /* Set drag */

                }

                switch (event -> modflags) {  	        /* button       */

                  	case SDL_BUTTON_LEFT:

                        if (event -> pressed)  {

                            event -> type     = SDLGL_INPUT_COMMAND;

                            fields -> fstate |= (SDLGL_FSTATE_MOULEFT | SDLGL_FSTATE_HASFOCUS);
                            InputFocus        = fields;
                            event -> field    = fields;	/* Save pointer on actual SDLGL_FIELD */

                        } /* if (event -> pressed) */
                        break;

                    case SDL_BUTTON_RIGHT:

                        if (event -> pressed)  {

                            event -> type     = SDLGL_INPUT_COMMAND;

                            fields -> fstate |= (SDLGL_FSTATE_MOURIGHT | SDLGL_FSTATE_HASFOCUS);
                            InputFocus        = fields;
                            event -> field    = fields;	/* Save pointer on actual SDLGL_FIELD */

                        } /* if (event -> pressed) */
                        break;

                    case SDL_BUTTON_MIDDLE:
                        MouseOver -> fstate |= SDLGL_FSTATE_MOUMIDDLE;
                        break;

                } /* switch buttons */

                return; /* Event generated */

            } /* if in rectangle */

        }

    }

    if (event -> sdlcode) {       /* If mouse click, handle it as key */

        if (mainloopITranslateKeyboardInput(event)) {

            return;

        }

    }

    event -> sdlcode  = 0;

}

/*
 * Name:
 *     sdlglIDefaultDrawingFunc
 * Description:
 *     Simply clears the screen
 * Input:
 *     fields: Pointer on fields to draw
 *     fps:    Actual number of fps
 */
#ifdef __BORLANDC__
        #pragma argsused
#endif
static void sdlglIDefaultDrawingFunc(SDLGL_FIELD *fields, SDLGL_EVENT *event)
{

    glClear(GL_COLOR_BUFFER_BIT); /* Simply clear the screen */

}

/*
 * Name:
 *     mainloopICheckInput
 * Description:
 *     Polls all input devices and stores the input in SDLGL_EVENT.
 *     Merges the different types of input into this one single type.
 *     Translates the first input-event returned from SDL. All the others
 *     are ignored.
 * Input:
 *     inpevent *:  Pointer on struct to fill with command info
 * Output:
 *      Result for reaction
 */
static int mainloopICheckInput(SDLGL_EVENT *inpevent)
{

    SDL_Event   event;


    memset(inpevent, 0, sizeof(SDLGL_EVENT));

    while ( SDL_PollEvent(&event) ) {

        if (! inpevent -> code) {

            switch(event.type) {

                case SDL_QUIT:
                	return SDLGL_INPUT_EXIT;

                case SDL_MOUSEMOTION:

                     inpevent -> type  = SDLGL_INPUT_MOUSE;
                     inpevent -> relx  = event.motion.x;
                     inpevent -> rely  = event.motion.y;
                     inpevent -> movex = event.motion.xrel;
                     inpevent -> movey = event.motion.yrel;
                     /* Now translate the input */
                     if (event.motion.state & SDL_BUTTON_LMASK) {

                         inpevent -> pressed  = 1;
                         inpevent -> modflags = SDL_BUTTON_LEFT;

                     }
                     else if (event.motion.state & SDL_BUTTON_RMASK) {

                         inpevent -> pressed  = 1;
                         inpevent -> modflags = SDL_BUTTON_RIGHT;

                     }
                     else {

                         /* Button don't care */
                         inpevent -> modflags = 0;

                     }

                     mainloopITranslateMouseInput(inpevent,
                     				              InputStack[InputStackIdx].InputFields,
                                                  1);
                     break;

                case SDL_MOUSEBUTTONDOWN:
                case SDL_MOUSEBUTTONUP:

                    inpevent -> type     = SDLGL_INPUT_MOUSE;
                    inpevent -> relx     = event.button.x;
                    inpevent -> rely     = event.button.y;
                    inpevent -> modflags = event.button.button;
                    if (event.type == SDL_MOUSEBUTTONDOWN) {

                        inpevent -> pressed = 1;

                    }
                    else {

                        inpevent -> pressed = 0;

                    }
                    /* Now translate the input */
                    mainloopITranslateMouseInput(inpevent,
                    				             InputStack[InputStackIdx].InputFields,
                                                 0);
                    break;

                case SDL_KEYDOWN:
                case SDL_KEYUP:
                    /* Do Keyboard-Function */
                    inpevent -> type     = SDLGL_INPUT_KEYBOARD;
                    inpevent -> relx     = 0;
                    inpevent -> rely     = 0;
                    inpevent -> sdlcode  = event.key.keysym.sym;
                    inpevent -> modflags = event.key.keysym.mod;

                    if (event.type == SDL_KEYDOWN) {

                        inpevent -> pressed = 1;

                    }
                    else {

                        inpevent -> pressed = 0;

                    }
                    /* Now translate the input */
                    mainloopITranslateKeyboardInput(inpevent);
    	            break;

                default:
                     inpevent -> type = 0;	/* Ignore it */

            }   /* switch(event.type) */

        }
        else {  /* if (! inpevent -> code) { */

            /* Do nothing at all, if already a code has been generated */
            /* Just remove it from buffer                              */ 

        }

    }  	/* while (SDL_PollEvent(&event)) */

    return SDLGL_INPUT_OK;

}

/*
 * Name:
 *     sdlglIResetInput
 * Description:
 *     Resets the inputstack to the base and removes all possible
 *     inputdescs.
 * Input:
 *     None
 */
static void sdlglIResetInput(void)
{

    SDL_Event event;


    /* Input stack */
    InputStackIdx = 1;
    memcpy(&InputStack[1], &InputStack[0], sizeof(InputStack[0]));
    sdlglInputSetFocus(0, 0);

    /* Clear any input anyway */
    memset(&ActualEvent, 0, sizeof(SDLGL_EVENT));
    while ( SDL_PollEvent(&event) );                /* Remove all input */

}

/*
 * Name:
 *     sdlglIDefaultInputFunc
 * Description:
 *     Makes a minimum translation of user input.
 * Input:
 *     event:     Pointer on pretranslated input.
 *
 */
static int sdlglIDefaultInputFunc(SDLGL_EVENT *event)
{

    switch (event -> sdlcode) {

        case SDLK_ESCAPE:
            return SDLGL_INPUT_EXIT;

    }

    return SDLGL_INPUT_OK;

}

/*
 * Name:
 *     sdlglICountFields
 * Description:
 *     Returns the number of fields with types != 0 in given fields.
 * Input:
 *     fields *: Buffer to count the valid fields in
 */
static int sdlglICountFields(SDLGL_FIELD *fields)
{

    int count;


    count = 0;
    while(fields -> type != 0) {

        fields++;
        count++;

    }

    return count;

}

/*
 * Name:
 *     sdlglIPushDisplay
 * Description:
 *     Increments the "InputStack", if the top isn't reached yet. Fills
 *     the new position with default values from base default handler if
 *     top isn't reached yet.
 * Input:
 *     None
 */
static void sdlglIPushDisplay(void)
{

    SDL_Event event;


    if ((InputStackIdx + 2) < SDLGL_MAXINPUTSTACK) {

    	/* Increment stackpointer only if possible */
        ++InputStackIdx;
        /* ------ Set the standard values ------ */
        memcpy(&InputStack[InputStackIdx], &InputStack[0], sizeof(InputStack[0]));

    }

    /* Clear any input anyway */
    memset(&ActualEvent, 0, sizeof(SDLGL_EVENT));
    while ( SDL_PollEvent(&event) );                /* Remove all input */

}

/*
 * Name:
 *     sdlglIPopDisplay
 * Description:
 *     Removes a handler and its inputdescs from the internal buffer.
 * Input:
 *     None
 */
static void sdlglIPopDisplay(void)
{

    SDLGL_CMDKEY *cmdkeys;
    SDLGL_EVENT sdlglevent;
    SDL_Event event;


    if (InputStackIdx > 1) {

        /* Call for possible release of memory */
        memset(&sdlglevent, 0, sizeof(SDLGL_EVENT));
        sdlglevent.type    = SDLGL_INPUT_CLEANUP;

        mainloopICheckInput(&sdlglevent);
        /* Decrement stack pointer */
        InputStackIdx--;

    }

    /* Clear any input anyway */
    memset(&ActualEvent, 0, sizeof(SDLGL_EVENT));
    while ( SDL_PollEvent(&event) );                /* Remove all input */

    /* Clear possible set command keys */
    cmdkeys = InputStack[InputStackIdx].CmdKeys;

    if (cmdkeys) {

        while (cmdkeys -> code > 0) {

            cmdkeys -> pressed = 0;
            cmdkeys++;

        }

    }

}

/*
 * Name:
 *     sdlglIFindTypeInFields
 * Description:
 *     Finds first occurence of field with given 'typeno' in given 'fields'.
 *     If 'fields' == 0 then use 'display' - fields.
 *     If not found, a pointer on a field with the type == 0 is returned
 *     (poniter on end of array).
 * Input:
 *     typeno: Number of type to find
 *     fields *: Where to start searching
 */
SDLGL_FIELD *sdlglIFindTypeInFields(int typeno, SDLGL_FIELD *fields)
{

    /* 1) Set fields pointer                    */
    if (! fields) {

        /* If not given, use internal array */
        fields = InputStack[InputStackIdx].InputFields;

    }

    while(fields -> type != 0) {

        if (fields -> type == typeno) {

            return fields;

        }

        fields++;

    }

    return fields;      /* Point on end of array, if not found */

}

/*
 * Name:
 *     sdlglCallDrawFunctions
 * Description:
 *     Call all drawing functions from 'top' to 'bottom'. Will say
 *     first 'SDLGL_STANDARD_INPUT' and all following 'SDLGL_POPUP_INPUT'
 * Input:
 *     None
 */
static void sdlglCallDrawFunctions(void)
{

    int stackno;


    if (InputStack[InputStackIdx].type == SDLGL_STANDARD_INPUT) {

        /* It's only one time to call the drawing function */
        InputStack[InputStackIdx].DrawFunc(InputStack[InputStackIdx].InputFields, &ActualEvent);

    }
    else {

        stackno = InputStackIdx;
        while(stackno > 0) {

            if (InputStack[stackno].type == SDLGL_STANDARD_INPUT) {

                /* Found it */
                while(stackno <= InputStackIdx) {

                    /* Call all drawings for further inputs */
                    InputStack[stackno].DrawFunc(InputStack[InputStackIdx].InputFields, &ActualEvent);
                    stackno++;

                }

                return;

            }

            stackno--;

        }

    }

}

/* ========================================================================== */
/* ========================= PUBLIC FUNCTIONS =============================== */
/* ========================================================================== */


/*
 * Name:
 *     sdlglCreateDisplay
 * Description:
 *     Adds a the Draw Function, the Inputhandler and a possible array of
 *     SDLGL_FIELD's and SDLGL_CMDKEY's on the inputstack. For every argument
 *     set to NULL the internal default is used.
 *     If the stack is full, the topmost data will be overwritten.
 *     The INPUT_DESC's set for drawing are the same as these given for input.
 * Input:
 *     drawfunc:       A SDLGL_DRAWFUNC function pointer
 *     timefunc:       A SDLGL_TIMEFUNC function pointer (may be NULL)
 *     inpdesc:        A pointer on an array of SDLGL_FIELD's (may be NULL)
 *     cmdkeys: A pointer on an array of SDLGL_CMDKEY's (may be NULL)
 */
void sdlglCreateDisplay(SDLGL_DRAWFUNC     drawfunc,
                        SDLGL_INPUTFUNC    inputfunc,
                        SDLGL_FIELD        *fields,
                        SDLGL_CMDKEY       *cmdkeys,
                        int numfields)
{

    sdlglIPushDisplay();

    /* Now replace these functions / arrays, wich are given */
    if (drawfunc) {

        InputStack[InputStackIdx].DrawFunc = drawfunc;

    }

    if (! inputfunc) {

        inputfunc = sdlglIDefaultInputFunc;

    }

    InputStack[InputStackIdx].InputFunc = inputfunc;

    if (! fields) {

        fields = &DefaultFields[0];

    }

    /* Set inputdescs and drawdesc to same base */
    InputStack[InputStackIdx].type        = SDLGL_STANDARD_INPUT;
    InputStack[InputStackIdx].InputFields = fields;
    InputStack[InputStackIdx].numfields   = numfields;

    if (! cmdkeys) {

        cmdkeys = &DefaultCommandKeys[0];

    }

    InputStack[InputStackIdx].CmdKeys = cmdkeys;

    /* ------ Let OpenGL clear the display -------- */
    glClear(GL_COLOR_BUFFER_BIT);

}

/*
 * Name:
 *     sdlglSetInput
 * Description:
 *     Every non-NULL argument replaces the actual inputs in the actual display
 *     Drawing handler is taken from actual handler.
 * Input:
 *     fields:  A pointer on an array of SDLGL_FIELD's (may be NULL)
 *     cmdkeys: A pointer on an array of SDLGL_CMDKEY's (may be NULL)
 */
void sdlglSetInput(SDLGL_FIELD *fields, SDLGL_CMDKEY *cmdkeys)
{

    if (! cmdkeys) {

        cmdkeys = &DefaultCommandKeys[0];

    }

    InputStack[InputStackIdx].CmdKeys = cmdkeys;

    if (! fields) {

        fields = &DefaultFields[0];

    }

    InputStack[InputStackIdx].InputFields = fields;

}

/*
 * Name:
 *     sdlglPopUpFields
 * Description:
 *     Saves the previous state and uses the new handler and the new data for
 *     translation of input.
 *     Only the new given fields and commands are checked for input. But all
 *     previous inputs on stack are called for drawing (bottom to top)
 * Input:
 *     inputfunc: Function to call for this input
 *     fields:    A pointer on an array of SDLGL_FIELD's (may be NULL)
 *     cmdkeys:   A pointer on an array of SDLGL_CMDKEY's (may be NULL)
 */
void sdlglPopUpFields(SDLGL_INPUTFUNC inputfunc, SDLGL_FIELD *fields, SDLGL_CMDKEY *cmdkeys)
{

    if ((InputStackIdx + 2) < SDLGL_MAXINPUTSTACK) {    /* Push input */

    	/* Increment stackpointer only if possible */
        ++InputStackIdx;
        /* ------ Copy from previous input handler if one available ------ */
        if (InputStackIdx > 0) {

            memcpy(&InputStack[InputStackIdx],
                   &InputStack[-1], sizeof(InputStack[0]));
            InputStack[InputStackIdx].type = SDLGL_POPUP_INPUT;

        }

    }

    if (! cmdkeys) {

        cmdkeys = &DefaultCommandKeys[0];

    }

    InputStack[InputStackIdx].CmdKeys = cmdkeys;

    if (! fields) {

        fields = &DefaultFields[0];

    }

    InputStack[InputStackIdx].InputFields = fields; /* Only changed mouse rectangles, same draw */

    if (! inputfunc) {

        inputfunc = sdlglIDefaultInputFunc;

    }

    InputStack[InputStackIdx].InputFunc = inputfunc;

}

/*
 * Name:
 *     sdlglInit
 * Description:
 *     - sets up an 2D-OpenGL screen with the size and colordepth
 *	 given in the config struct.
 * Input:
 *     configdata:
 *     	   The graphics configuration.
 * Output:
 *	errorcode as defined in mainloop.h
 */
int sdlglInit(SDLGL_CONFIG *configdata)
{

    char  errmsg[256];
    int   eachcolordepth;
    int   scrflags;

    /* Initialize SDL for video output */
    if ( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
        sprintf(errmsg, "Unable to initialize SDL: %s\n", SDL_GetError());
        puts(errmsg);
        return INIT_ERROR_SDL_INIT;	/* Return value for users "main()" function */
    }

    /* Now copy the users config data to internal structure, if any available */
    /* Otherwise use the standard values 				      */
    if (configdata) {

    	memcpy(&mainConfig, configdata, sizeof (SDLGL_CONFIG));

        if (mainConfig.dblclicktime == 0) {
            mainConfig.dblclicktime = SDLGL_DBLCLICKTIME;       /* Set default */
        }

        if (mainConfig.displaywidth == 0) {

            mainConfig.displaywidth  = mainConfig.scrwidth;
            mainConfig.displayheight = mainConfig.scrheight;

        }

    }

    if (mainConfig.debugmode) {

        mainConfig.colordepth = 0;		/* Use standard setings of screen     */
        SDL_WM_GrabInput(SDL_GRAB_ON);  /* Grab all input if in windowed mode */

    }
    else {

        /* Set Colordepth */

        switch (mainConfig.colordepth) {

            case 16: eachcolordepth = 5;
        	    break;

            case 24:
            case 32: eachcolordepth = 8;
        	    break;

            default:
            	eachcolordepth = mainConfig.colordepth / 3;

    	} /* switch */


        SDL_GL_SetAttribute( SDL_GL_RED_SIZE,   eachcolordepth );
        SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, eachcolordepth  );
        SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE,  eachcolordepth );
        if (mainConfig.zbuffer > 0) {

            SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, mainConfig.zbuffer );

        }

        SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

    }

    scrflags = SDL_DOUBLEBUF | SDL_OPENGL;

    if (mainConfig.screenmode == SDLGL_SCRMODE_FULLSCREEN) {

    	scrflags |= SDL_FULLSCREEN;

    }

    /* Set the video mode with the data from configOpenGL screen */
    if ( SDL_SetVideoMode(mainConfig.scrwidth, mainConfig.scrheight,
    			          mainConfig.colordepth, scrflags ) == NULL ) {

        sprintf(errmsg, "Unable to create OpenGL screen: %s\n", SDL_GetError());
    	puts(errmsg);
    	SDL_Quit();
        return INIT_ERROR_SDL_SCREEN;

    }


    glClearColor(0.0, 0.0, 0.0, 0.0);			 /* Set Clear-Color */
    glViewport(0, 0, mainConfig.scrwidth,
    		         mainConfig.scrheight); /* Set the Viewport to whole Screen */

    /* Set Up An Ortho Screen */
    if (mainConfig.displaywidth == 0) {

        mainConfig.displaywidth  = mainConfig.scrwidth;
        mainConfig.displayheight = mainConfig.scrheight;

    }

    sdlglSetViewSize(mainConfig.displaywidth, mainConfig.displayheight);

    glEnable(GL_CULL_FACE);             /* Set backface-culling 		           */
    glPolygonMode(GL_FRONT, GL_FILL);	/* Draw only the front-polygons, fill-mode */
    glClear(GL_COLOR_BUFFER_BIT);	    /* Clear the screen first time for simple  */
    					                /* usages.				                   */

    /* Init the input stuff */
    sdlglIResetInput();		/* Reset the internal handler stack */
        					/* to basic values		            */

    /* Set the title bar in environments that support it */
    SDL_WM_SetCaption(mainConfig.wincaption, NULL);

    if (mainConfig.hidemouse) {

        /* Hide the mouse cursor */
        SDL_ShowCursor(0);

    }

    /* Move the mouse to the middle of the screen */
    SDL_WarpMouse((unsigned short)(mainConfig.scrwidth / 2),
                  (unsigned short)(mainConfig.scrheight / 2));

    return INIT_ERROR_NONE;

}


/*
 * Name:
 *     sdlglExecute
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
int sdlglExecute(void)
{

    static int fpscount = 0;	/* FPS-Counter 						*/
    static int fpsclock = 0;	/* Clock for FPS-Counter			*/
    static int fps = 0;			/* Holds FPS until next FPS known	*/

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

        /* ----------------- Draw the screen ------------------ */
        memset(&ActualEvent, 0, sizeof(SDLGL_EVENT));                   /* Clear it        */
        ActualEvent.type          = 0;
        ActualEvent.tickspassed   = tickspassed;
        ActualEvent.secondspassed = (float)tickspassed / (float)SDLGL_TICKS_PER_SECOND;
        ActualEvent.fps           = fps;
        ActualEvent.cmd           = InputStack[InputStackIdx].CmdKeys;    /* For key release check */


        /* Now get the actual position of the mouse. Always return it */
        SDL_GetMouseState(&ActualEvent.relx, &ActualEvent.rely);
        ActualEvent.relx = ActualEvent.relx * mainConfig.displaywidth / mainConfig.scrwidth;
        ActualEvent.rely = ActualEvent.rely * mainConfig.displayheight / mainConfig.scrheight;

        /* Now call the drawing function */
        sdlglCallDrawFunctions();

        /* ------ Gather and translate user input -------- */
        if (mainloopICheckInput(&ActualEvent) == SDLGL_INPUT_EXIT) {

            done = 1;       /* Exit loop  */

        }
        else {

            /* Set values for timed input ... */
            ActualEvent.tickspassed = tickspassed;
            ActualEvent.secondspassed = (float)tickspassed / (float)SDLGL_TICKS_PER_SECOND;
            ActualEvent.fps         = fps;
            ActualEvent.cmd         = InputStack[InputStackIdx].CmdKeys;     /* For key release check */
            ActualEvent.fieldlist   = InputStack[InputStackIdx].InputFields; /* For key state         */
            ActualEvent.field       = InputFocus;                            /* For handling          */

            switch(InputStack[InputStackIdx].InputFunc(&ActualEvent)) {

                case SDLGL_INPUT_EXIT:
                    done = 1;
                    break;

                case SDLGL_INPUT_REMOVE:
                	/* Remove an inputhandler and it's inputdescs */
                    /* from inputstack			                  */
                    sdlglIPopDisplay();
                    break;

                case SDLGL_INPUT_RESET:
                	sdlglIResetInput();
                    break;

            }

        }

        glFlush();

        /* swap buffers to display, since we're double buffered.	*/
        SDL_GL_SwapBuffers();

        mainclock = newmainclock;

    }

    return 1;

}

/*
 * Name:
 *     sdlglShutdown
 * Description:
 *     Makes the cleanup work for the mainloop and closes the OpenGL
 *     screen.
 *     Any users cleanup work for the graphics has to be done before this
 *     procedure is called.
 * Output:
 *	exitcode for users "main()" function
 */
void sdlglShutdown(void)
{

    SDL_WM_GrabInput(SDL_GRAB_OFF);
    SDL_Quit();

}

/*
 * Name:
 *     sdlglSetColor
 * Description:
 *     Set the standard color with the given number.
 * Last change:
 *      2008-06-21 / bitnapper ( Standard-Palette with 256 colors)
 */
void sdlglSetColor(int colno)
{

    colno &= 0xFF;	/* Prevent wrong numbers */

    glColor3ubv(&stdcolors[colno * 4]);

}

/*
 * Name:
 *     sdlglGetColor
 * Description:
 *     Gets the color with given number from the palette (including alpha channel)
 * Input:
 *     color:      Number of color to fetch
 *     col_buf *: Where to return the color
 */
void sdlglGetColor(int colno, unsigned char *col_buf)
{

    colno &= 0xFF;	/* Prevent wrong numbers */
    colno *= 4;     /* Get index into array  */
    
    col_buf[0] = stdcolors[colno + 0];
    col_buf[1] = stdcolors[colno + 1];
    col_buf[2] = stdcolors[colno + 2];
    col_buf[3] = stdcolors[colno + 3];
    
}

/*
 * Name:
 *     sdlglSetPalette
 * Description:
 *     Sets the part of the palette with given color info
 * Input:
 *     from: Start in palette 
 *     to:   Last color to set
 *
 */
void sdlglSetPalette(int from, int to, unsigned char *colno)
{
    
    int index;
    
    
    /* Prevent wrong numbers */
    from &= 0xFF;
    to   &= 0xFF;
    from *= 4;
    to   *= 4;
    
    for (index = from; index <= to; index++) {
    
        stdcolors[index] = *colno;
        colno++;
    
    }

} 

/*
 * Name:
 *     sdlglAdjustRectToScreen
 * Description:
 *     Adjusts the given rectangle to screen, depending on given flags
 * Input:
 *     src:   Pointer on rectangle to adjust
 *     dest:  Where to put the adjusted values. If NULL, change source.
 *     flags: What to adjust
 */
void sdlglAdjustRectToScreen(SDLGL_RECT *src, SDLGL_RECT *dest, int flags)
{

    int i;
    int testflag;
    int diffx, diffy;   /* Difference between rectangele an screen size */

    if (dest == NULL) {

        dest = src;
        
    }
    else {

        dest -> x = src -> x;
        dest -> y = src -> y;
        dest -> w = src -> w;
        dest -> h = src -> h;

    }

    i        = 10;          /* Maximum number of flags */
    testflag = 1;
    diffx    = mainConfig.scrwidth  - dest -> w;
    diffy    = mainConfig.scrheight - dest -> h;

    while (i > 0) {

        switch (flags & testflag) {

            case SDLGL_FRECT_HCENTER:
                dest -> x = (diffx / 2);
                break;

            case SDLGL_FRECT_VCENTER:
                dest -> y = (diffy / 2);
                break;

            case SDLGL_FRECT_SCRLEFT:           /* On left side of screen     */
                dest -> x = 0;
                break;

            case SDLGL_FRECT_SCRRIGHT:          /* On right side of screen    */
                dest -> x = (diffx - 1);
                break;

            case SDLGL_FRECT_SCRTOP:            /* At top of screen           */
                dest -> y = 0;
                break;

            case SDLGL_FRECT_SCRBOTTOM:         /* At bottom of screen        */
                dest -> y = (diffy - 1);
                break;

            case SDLGL_FRECT_SCRWIDTH:          /* Set to screen width        */
                dest -> x = 0;
                dest -> w = mainConfig.scrwidth;
                break;

            case SDLGL_FRECT_SCRHEIGHT:         /* Set to screen height       */
                dest -> y = 0;
                dest -> h = mainConfig.scrheight;
                break;

        }  /* switch (flags & testflag) */

        i--;
        testflag <<= 1;

    } /* while (i > 0) */

}

/*
 * Name:
 *     sdlglScreenShot
 * Description:
 *     dumps the current screen (GL context) to a new bitmap file
 *     right now it dumps it to whatever the current directory is
 *     returns TRUE if successful, FALSE otherwise
 * Input:
 *     filename *:   name of file to save the screenshot to
 * Output:
 *     Suceeded Yes/no
 */
int sdlglScreenShot(const char *filename)
{

    SDL_Surface *screen, *temp;
    unsigned char *pixels;
    int i;

    screen = SDL_GetVideoSurface();
    temp = SDL_CreateRGBSurface(SDL_SWSURFACE, screen->w, screen->h, 24,
       #if SDL_BYTEORDER == SDL_LIL_ENDIAN
           0x000000FF, 0x0000FF00, 0x00FF0000, 0
       #else
           0x00FF0000, 0x0000FF00, 0x000000FF, 0
       #endif
           );

    if (temp == NULL) {
        return 0;
    }

    pixels = malloc(3*screen->w * screen->h);
    if (pixels == NULL) {

        SDL_FreeSurface(temp);
        return 0;
        
    }

    glReadPixels(0, 0, screen->w, screen->h, GL_RGB, GL_UNSIGNED_BYTE, pixels);

    for (i=0; i < screen->h; i++) {
        memcpy(((char *) temp->pixels) + temp->pitch * i, pixels + 3 * screen->w * (screen->h-i-1), screen->w * 3);
    }
    free(pixels);

    SDL_SaveBMP(temp, filename);
    SDL_FreeSurface(temp);

    return 1;

}

/*
 * Name:
 *     sdlglSetViewSize
 * Description:
 *     Sets the size for the view to the given value. The mouse position
 *     value will be adjusted to this one appropriatly.
 * Input:
 *      width, height:  Siez to set the view to
 */
void sdlglSetViewSize(int width, int height)
{

    glLoadIdentity();
    glOrtho(0.0,  width, height, 0.0, -100.0, 100.0 );

    /* --- Save it for adjustement of mouse position ----- */
    mainConfig.displaywidth  = width;
    mainConfig.displayheight = height;
    
}

/*
 * Name:
 *     sdlglInputSetFocus
 * Description:
 *     Sets the input focus for the keyboard to given input field.
 *     If given input-field is 0, then the focus is set to the internal
 *     dummy input field.
 *     If 'field' is NULL, then the focus is moved into given direction 'dir'.
 * Input:
 *     field *: Pointer on SDLGL_FIELD to set focus to
 *     dir:     Direction to move focus to, if no 'field *' is given
 * Last change:
 *           2008-06-14: bitnapper
 */
void sdlglInputSetFocus(SDLGL_FIELD *field, int dir)
{

    SDLGL_FIELD *fptr, *fbase;


    /* 1) Get list of fields */
    fptr  = InputStack[InputStackIdx].InputFields;
    if (! fptr) {

        return;     /* No fields available at all */

    }

    /* 2) Remove old focus flag and set new, if field available */
    if (field) {
    
        while(fptr -> type != 0) {

            fptr -> fstate = 0;
            if (fptr == field) {

                InputFocus = fptr;
                fptr -> fstate |= SDLGL_FSTATE_HASFOCUS;
                return;     /* New focus is set */

            }

            fptr++;

        }
        
        return;

    }

    /* 3) Find actual field having focus and set new one */
    fptr  = InputStack[InputStackIdx].InputFields;
    fbase = InputStack[InputStackIdx].InputFields;
    if (dir != 0) {
    
        while(fptr -> type != 0) {
    
            /* Find the actual field having the focus */
            if (fptr -> fstate & SDLGL_FSTATE_HASFOCUS) {
        
                fptr -> fstate = 0; /* Clear the focus */
                if (dir > 0) {
            
                    fptr++;
                    if (fptr -> type == 0) {    /* Wrap around */
                 
                        fptr = fbase;
                    
                    }
                    
                }
                else if (dir < 0) {
            
                    if (fptr == fbase) {    /* Wrap around */
                    
                        fptr = sdlglIFindTypeInFields(0, InputStack[InputStackIdx].InputFields);
                        
                    }
                    else {
                    
                        fptr--;
                        
                    }
                        
                }
                
                /* Set new Focus */
                InputFocus = fptr;
                fptr -> fstate |= SDLGL_FSTATE_HASFOCUS;
                 
            }    
            
            fptr++;
            
        } /* while(fptr -> type != 0)  */ 
                   
    } /* if (dir != 0) */

}

/*
 * Name:
 *     sdlglFindType 
 * Description:
 *     Finds first occurence of field with given 'typeno' in given 'fields'.
 *     If 'fields' == 0 then use 'display' - fields.
 *     If not found, a pointer on a field with the type == 0 is returned
 *     (poniter on end of array).
 * Input:
 *     typeno: Number of type to find
 */
SDLGL_FIELD *sdlglFindType(int typeno)
{

    SDLGL_FIELD *fields;


    /* 1) Set fields pointer                    */
    fields = InputStack[InputStackIdx].InputFields;
    if (! fields) {

        return &DefaultFields[0];   /* fields; */

    }

    return sdlglIFindTypeInFields(typeno, fields);

}

/*
 * Name:
 *     sdlglRemoveFieldBlock
 * Description:
 *     Removes the block of fields in actual Input-Fields, signed by
 *     'blockno', if available at all.
 * Input:
 *     blocksign: Value which signs start an end of block
 */
void sdlglRemoveFieldBlock(int blocksign)
{

    SDLGL_FIELD *fields, *pstart, *pend;


    /* 1) Set fields pointer                    */
    fields = InputStack[InputStackIdx].InputFields;
    if (! fields) {

        return;

    }

    /* 2) Find block start, if it exists at all */
    pstart = sdlglFindType(blocksign);
    if (pstart -> type == 0) {

        return;     /* Block not available */

    }

    /* 3) ind end of block, if available        */
    pend = sdlglIFindTypeInFields(blocksign, &pstart[1]);
    if (pend -> type == 0) {

        /* It's a single block sign */
        pend = pstart;

    }

    /* 4) Point behind end                      */
    pend++;

    /* 5) Remove the buffer, copy at minimum the end sign   */
    do {

        memcpy(pstart, pend, sizeof(SDLGL_FIELD));
        pstart++;
        pend++;

    }
    while(pend -> type != 0);

}

/*
 * Name:
 *     sdlglSetFieldBlock
 * Description:
 *     Adds the given fields to actual Input-Fields. Signs it with 'blocksign'.
 *     If a field block signed with 'blocksign' already exists, it is replaced
 *     by the new field block.
 *     If the block doesn't fit into destination, the block isn't copied.
 * Input:
 *     blocksign: Value which signs start an end of block
 *     src *:     Fields to add to '*fields' with sign 'blocksign'.
 */
void sdlglSetFieldBlock(int blocksign, SDLGL_FIELD *src)
{

    SDLGL_FIELD *fields, *pend;
    int countact, countsrc, countleft, numdest;


    /* 1) Set the field pointer                     */
    fields = InputStack[InputStackIdx].InputFields;
    if (! fields) {

        return;

    }

    /* 2) First remove a possible available block   */
    sdlglRemoveFieldBlock(blocksign);
    numdest = InputStack[InputStackIdx].numfields;

    /* 3) Find end of fields, count the number      */
    countact = 0;
    pend     = fields;
    while(pend -> type != 0) {

        pend++;
        countact++;

    }

    /* 4) Count the fields, that should be copied   */
    countsrc  = sdlglICountFields(src);
    countsrc  += 2;                 /* Inculding fields needed for blocksign */
    countleft = numdest - countact;

    /* 5) Copy the fields, if there's enough space  */
    if (countleft >= countsrc) {

        /* If there is space left */
        /* Set the field for the block sign */
        memset(pend, 0, sizeof(SDLGL_FIELD));
        pend -> type = blocksign;
        pend++;
        while(src -> type != 0) {

            memcpy(pend, src,  sizeof(SDLGL_FIELD));
            pend++;
            src++;

        }

        memset(pend, 0, sizeof(SDLGL_FIELD));
        pend -> type = blocksign;
        pend++;
        pend -> type = 0;

    }

}
