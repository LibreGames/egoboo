/*******************************************************************************
*   SDLGL.C                      	                                           *
*       A simple Main-Function to separate calls to SDL from the rest of       *
*	the code. Initializes and maintains an OpenGL window                       *
*   Merges inputs for mouse and keyboard                                       *
*                                                                              *
*   Initial code: January 2001, Paul Mueller                                   *
*									                                           *
*   Copyright (C) 2001-2010  Paul Müller <pmtech@swissonline.ch>               *
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
* Last change: 2010-08-01                                                      *
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

#define SDLGL_MAXINPUTSTACK 32   	/* Maximum depth of stack 	       */

#define SDLGL_DBLCLICKTIME  56      /* Ticks                           */
#define SDLGL_TOOLTIPTIME   500     /* Ticks until tooltip-flag is set */
                                    /* Translation                     */

/******************************************************************************
* TYPEDEFS                                                                    *
******************************************************************************/


/******************************************************************************
* FORWARD DECLARATIONS                                                        *
******************************************************************************/

static void sdlglIDefaultDrawFunc(SDLGL_FIELD *fields, SDLGL_EVENT *event);
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
    "sdlgl-library V 0.9 - Build(" __DATE__  ")", /* Caption for window, if any */
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
static SDLGL_CMDKEY DefaultCmds[2];
static SDLGL_FIELD  DefaultArea[2];

static SDLGL_FIELD *FocusArea = DefaultArea; /* Never NULL   */
static SDLGL_EVENT ActualEvent;

static struct {

    SDLGL_FIELD        *DrawArea;       /* Returned to user for drawing     */
    SDLGL_FIELD        *InputArea;      /* Areas checked for input          */
    SDLGL_CMDKEY       *CmdKeys;	    /* Shortcut keys	                */
    SDLGL_DRAWFUNC     DrawFunc;        /* Function to use for drawing      */
    SDLGL_INPUTFUNC    InputFunc;       /* Function for input translation   */
    int num_total;                      /* Number of fields in 'DrawArea'   */  
    int num_remaining;                  /* Remaining fields for var. areas  */

} InputStack[SDLGL_MAXINPUTSTACK + 1] = {

    { DefaultArea,		        /* No draw-areas defined    */  
      DefaultArea,		        /* No inputdesc defined		*/
      DefaultCmds,              /* No shortcut keys defined	*/
      sdlglIDefaultDrawFunc, 
      sdlglIDefaultInputFunc
    },
    { DefaultArea,		        /* No draw-vectors defined      */  
      DefaultArea,		        /* No inputdesc defined		    */
      DefaultCmds,              /* No shortcut keys defined		*/
      sdlglIDefaultDrawFunc, 
      sdlglIDefaultInputFunc
    }

};

static int  InputStackIdx = 1;  /* StackIndex 0 is the default handler */

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
    relx = event -> mou.x - id -> rect.x;


    if (relx >= 0) {			  /* Is right of X 		*/

        if (relx > id -> rect.w) {

            return 0;			  /* Right side out of Rectangle */
            
        }

        /* Now check vertically */
        rely = event -> mou.y - id -> rect.y;

    	if (rely >= 0) {		  /* Is lower of Y-Pos */

            if (rely <= id -> rect.h) {

                event -> mou.x = relx;
                event -> mou.y = rely;
            	return 1;			    /* Is in Rectangle   */

            }

    	}

    }
    
    return 0;

}

/*
 * Name:
 *     sdlglITranslateKeyboardInput
 * Name:
 *     Checks the key on validity, including check for modifier keys.
 *     If the key in SDLGL_EVENT is the one looked for, the SDLGL_EVENT
 *     struct is modified to type "SDLGL_INPUT_COMMAND".
 * Input:
 *     event:     Pointer on SDLGL_FIELD which holds the actual key and the
 *  	          key modifiers.
 *     pressed:   yes/no
 * Output:
 *     > 0 if a valid command was generated
 */
static int sdlglITranslateKeyboardInput(SDLGL_EVENT *event, char pressed)
{

    SDLGL_CMDKEY *sk, *sk_list;
    int i;
    unsigned char mask;
    

    /* 1) Check if there are command keys to translate */
    sk_list = InputStack[InputStackIdx].CmdKeys;
    if (! sk_list) {

        return 0;
        
    }
    
    /* 2) Check if there's a code to translate */
    if (! event -> sdlcode) {
    
        return 0;
        
    }
        
    /* 3) Do manage 'pressed'  */
    event -> sdlgl_type = 0;
    event -> pressed    = pressed;    
    
    if (event -> pressed) {
    
        /* 3a) Fill in the keymask everywhere it is used    */ 
        /* Check from 'right' to 'left' for combination     */
        for (i = 1, mask = 0x02; i >= 0; i--, mask >>= 1) {
        
            sk = sk_list;
            while (sk -> keys[0] > 0) {
        
                if (sk -> keys[i] == event -> sdlcode) {                
                    
                    sk -> pressed |= mask;            
                    /* Found a candidate, check combined keys */    
                    if (sk -> pressed == sk -> keymask) {
                        
                        /* Set the command */
                        event -> code       = sk -> code;
                        event -> sub_code   = sk -> sub_code;
                        
                        /* ---- Throw away other candidates --- */
                        sk--;
                        while (sk >= sk_list) {

                            if (sk -> keys[i] == event -> sdlcode) { 
                            
                                sk -> pressed &= (char)~mask; 
                                
                            }
                            
                            sk--;
                            
                        }
                        
                        return 1;
                    
                    }                       
                    
                }

                sk++;                
                
            } /* while (sk -> keys[i] > 0) */     
                        
        }        
        
    } 
    else {  
    
        /* Find 'best' key released (most flags)    */    
        sk = sk_list;
        while (sk -> keys[0] > 0) {
        
            if (sk -> release_code) {
            
                for (i = 0; i < 2; i++) {
                
                    if (sk -> keys[i] > 0) {   

                        if (sk -> keys[i] == event -> sdlcode) {  

                            if (sk -> pressed == sk -> keymask) {
                            
                                /* Send code for released key */                                
                                event -> code       = sk -> code;
                                event -> sub_code   = sk -> release_code;
                                break;
                                
                            }

                        }            
                    
                    }
                    
                }
            
            }
            
            sk++;
        
        }
        
        /* Remove the key everywhere in array */
        sk = sk_list;
        while (sk -> keys[0] > 0) {
        
            for (i = 0, mask = 0x01; i < 2; i++, mask <<= 1) {
                    
                if (sk -> keys[i] == event -> sdlcode) {  
                
                    sk -> pressed &= (char)~mask; 
                    
                }
                
            } 

            sk++;            
        
        }
        
    }
    
    if (event -> code == 0) {
    
        return 0;
        
    }
    
    return 1;

}

/*
 * Name:
 *     sdlglITranslateMouseInput
 * Description:
 *     Translates the mouse input, using the inputdescs, if any are
 *     available. Otherwise the "event" is left unchanged. If an overlapped
 *     rectangle is hit by the mouse, the first one which has a value in
 *     the 'code' field is  the one which generates the command.
 * Input:
 *     event:     Pointer on an inputevent, holding the mouse variables.
 *     inputdesc: Pointer onSDLGL_INPUT's to check for mouse input.
 */
static void sdlglITranslateMouseInput(SDLGL_EVENT *event, SDLGL_FIELD *fields)
{

    static SDLGL_FIELD *MouseArea = DefaultArea; /* Never NULL   */ 
    static tooltipticks = 0;
    
    SDLGL_FIELD *base;              /* Save base    */


    event -> field = 0;             /* Assume none  */

    /* Adjust the mouse position to the graphics size of screen */
    event -> mou.x = event -> mou.x * mainConfig.displaywidth / mainConfig.scrwidth;
    event -> mou.y = event -> mou.y * mainConfig.displayheight / mainConfig.scrheight;
    /* Adjust the movement value, too */
    event -> mou.w = event -> mou.w * mainConfig.displaywidth / mainConfig.scrwidth;
    event -> mou.h = event -> mou.h * mainConfig.displayheight / mainConfig.scrheight;
    
    switch(event -> sdlcode) {

        case SDL_BUTTON_LEFT:
            event -> sdlcode = SDLGL_KEY_MOULEFT;
            break;
            
        case SDL_BUTTON_MIDDLE:
            event -> sdlcode = SDLGL_KEY_MOUMIDDLE;
            break;

        case SDL_BUTTON_RIGHT:
            event -> sdlcode = SDLGL_KEY_MOURIGHT;
            break;
            
        default:
            /* Is a mouse drag event */
            break;

    }

    base = fields;

    if (fields) {

        /* Search from top to bottom */
        while (fields -> sdlgl_type != 0) {

            fields++;       /* Points on trailing type 0 */

        }

        while (fields > base) {

            fields--;

            /* FIXME: Add NEW handling of keyboard based on fields -> sdlgl_type
               E.g. SDLGL_TYPE_EDIT + SDLGL_TYPE_DRAG */

            /* if the mouse has to be checked */
            if (fields -> code && mainIPosInRect(fields, event)) {

                /* Support-flag for tooltips */
                if (MouseArea != fields) {

                    /* Change active mouse-over-area */
                    MouseArea -> fstate &= (unsigned char)(~SDLGL_FSTATE_CLEAR);
                    MouseArea = fields;
                    MouseArea -> fstate |= SDLGL_FSTATE_MOUSEOVER;
                    tooltipticks = 0;

                }
                else {
                    /* Same area as last call: Dragged in this Input-Rectangle... */

                    tooltipticks += event -> tickspassed;

                    if (tooltipticks >= mainConfig.tooltiptime) {

                        MouseArea -> fstate |= SDLGL_FSTATE_TOOLTIP;

                    }

                }

                if (event -> pressed && event -> sdlcode) { /* it's an active field */

                    if (FocusArea != fields) {
                                /* Change focus */
                        FocusArea -> fstate &= (unsigned char)(~SDLGL_FSTATE_HASFOCUS);
                        fields -> fstate    |= SDLGL_FSTATE_HASFOCUS;
                        FocusArea           = fields;
                    }
                    event -> field      = fields;	/* Save pointer on actual SDLGL_FIELD */
                    event -> sdlgl_type = fields -> sdlgl_type;
                    event -> code       = fields -> code;
                    event -> sub_code   = fields -> sub_code;
                }

                return; /* Event generated */

            } /* if in rectangle */

        } /* while (fields > base) */

        /* Mouse off all fields -- Reset 'mouse-over' */
        MouseArea -> fstate &= (unsigned char)(~SDLGL_FSTATE_CLEAR);
        MouseArea = DefaultArea;
        event -> field = DefaultArea;        

    }  /* if (fields) */ 
    
    /* Translate it as key if no event-code is generated. For combined input */
    sdlglITranslateKeyboardInput(event, event -> pressed);
}

/*
 * Name:
 *     sdlglIDefaultDrawFunc
 * Description:
 *     Simply clears the screen
 * Input:
 *     fields: Pointer on fields to draw
 *     fps:    Actual number of fps
 */
#ifdef __BORLANDC__
        #pragma argsused
#endif
static void sdlglIDefaultDrawFunc(SDLGL_FIELD *fields, SDLGL_EVENT *event)
{

    glClear(GL_COLOR_BUFFER_BIT); /* Simply clear the screen */

}

/*
 * Name:
 *     sdlglICheckInput
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
static int sdlglICheckInput(SDLGL_EVENT *inpevent)
{

    SDL_Event   event;


    memset(inpevent, 0, sizeof(SDLGL_EVENT));

    while ( SDL_PollEvent(&event) ) {
        
        if (! inpevent -> code) {

            switch(event.type) {

                case SDL_QUIT:
                	return SDLGL_INPUT_EXIT;

                case SDL_MOUSEMOTION:
                    inpevent -> mou.x = event.motion.x;
                    inpevent -> mou.y = event.motion.y;
                    inpevent -> mou.w = event.motion.xrel;
                    inpevent -> mou.h = event.motion.yrel;
                    /* TODO: Set 'inpevent -> modflags = ' */
                    /* Now translate the input */
                    if (event.motion.state & SDL_BUTTON_LMASK) {

                        inpevent -> pressed  = 1;
                        inpevent -> sdlcode  = SDLGL_KEY_MOULDRAG;

                    }
                    else if (event.motion.state & SDL_BUTTON_RMASK) {

                        inpevent -> pressed  = 1;
                        inpevent -> sdlcode  = SDLGL_KEY_MOURDRAG;

                    }
                    else {

                        /* Button don't care, no command, but mark 'mouse over' */
                        inpevent -> pressed  = 0;
                        inpevent -> sdlcode  = SDLGL_KEY_MOUMOVE;

                    }
                    sdlglITranslateMouseInput(inpevent,
                     				          InputStack[InputStackIdx].InputArea);
                    break;

                case SDL_MOUSEBUTTONDOWN:
                    /* Click only on mouse release ==> Button up */
                    /* May be start of drag action               */
                    break;
                case SDL_MOUSEBUTTONUP:
                    inpevent -> mou.x   = event.button.x;
                    inpevent -> mou.y   = event.button.y;
                    inpevent -> mou.w   = 0;
                    inpevent -> mou.h   = 0;
                    inpevent -> sdlcode = event.button.button;
                    inpevent -> pressed = 1;

                    sdlglITranslateMouseInput(inpevent,
                    				          InputStack[InputStackIdx].InputArea);
                    break;

                case SDL_KEYDOWN:
                    /* Check debug-state */
                    if (mainConfig.debugmode > 0 && event.key.keysym.sym == SDLK_ESCAPE) {

                        return SDLGL_INPUT_EXIT;

                    }  
                case SDL_KEYUP:                                      
                    /* Translate Keyboard-Input */
                    inpevent -> mou.x    = 0;
                    inpevent -> mou.y    = 0;
                    inpevent -> sdlcode  = event.key.keysym.sym;
                    inpevent -> modflags = event.key.keysym.mod;
                    
                    /* TODO: Do special handling if actual inputfield is  'sdlgl_type' == SDLGL_TYPE_EDIT   */                    
                    sdlglITranslateKeyboardInput(inpevent, event.type == SDL_KEYDOWN ? 1 : 0);
    	            break;

                default:
                    inpevent -> sdlgl_type = 0;
                    inpevent -> code       = 0;   /* Ignore it */

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

    if (event -> sdlcode == SDLK_ESCAPE) {

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
    while(fields -> sdlgl_type != 0) {

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
    while (SDL_PollEvent(&event));                /* Remove all input */

}

/*
 * Name:
 *     sdlglIPopInput
 * Description:
 *     Removes a handler and its inputdescs from the internal buffer.
 *     Includes possible  
 * Input:
 *     None
 */
static void sdlglIPopInput(void)
{

    SDLGL_CMDKEY *cmdkeys;
    SDLGL_EVENT sdlglevent;
    SDL_Event event;


    if (InputStackIdx > 1) {

        /* Call for possible release of memory */
        memset(&sdlglevent, 0, sizeof(SDLGL_EVENT));
        sdlglevent.code  = SDLGL_INPUT_CLEANUP;

        InputStack[InputStackIdx].InputFunc(&sdlglevent);    
        
        /* Remove possible popup fields */
        InputStack[InputStackIdx].InputArea -> sdlgl_type = 0;
               
        /* Decrement stack pointer */
        InputStackIdx--;

    }

    /* Clear any input anyway */
    memset(&ActualEvent, 0, sizeof(SDLGL_EVENT));
    while (SDL_PollEvent(&event));                /* Remove all input */

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
 *     sdlglIFindTypeInAreas
 * Description:
 *     Finds first occurence of field with given 'typeno' in given 'fields'.
 *     If 'fields' == 0 then use 'display' - fields.
 *     If not found, a pointer on a field with the type == 0 is returned
 *     (poniter on end of array).
 * Input:
 *     sdlgl_type: Number of sdlgl_type to find: 0: Don't check 
 *     fields *:   Where to start searching
 */
SDLGL_FIELD *sdlglIFindTypeInAreas(char sdlgl_type, SDLGL_FIELD *fields)
{

    if (! fields) {

        /* If not given, use internal array */
        fields = InputStack[InputStackIdx].InputArea;

    }

    while(fields -> sdlgl_type != 0) {

        if (fields -> sdlgl_type == sdlgl_type) {

            return fields;

        }

        fields++;

    }

    return fields;      /* Point on end of array, if not found */

}

/* ========================================================================== */
/* ========================= PUBLIC FUNCTIONS =============================== */
/* ========================================================================== */


/*
 * Name:
 *     sdlglInputNew
 * Description:
 *     Adds a the Draw Function, the Inputhandler and a possible array of
 *     SDLGL_FIELD's and SDLGL_CMDKEY's on the inputstack. For every argument
 *     set to NULL the internal default is used.
 *     If the stack is full, the topmost data will be overwritten.
 *     The INPUT_DESC's set for drawing are the same as these given for input.
 * Input:
 *     drawfunc:    A SDLGL_DRAWFUNC function pointer
 *     inputfunc:   A SDLGL_INPUTFUNC function pointer (may be NULL)
 *     fields:      A pointer on an array of SDLGL_FIELD's (may be NULL)
 *     cmdkeys:     A pointer on an array of SDLGL_CMDKEY's (may be NULL)
 *     numfields:   Number of fiels in vector 'fields'
 */
void sdlglInputNew(SDLGL_DRAWFUNC     drawfunc,
                   SDLGL_INPUTFUNC    inputfunc,
                   SDLGL_FIELD        *fields,
                   SDLGL_CMDKEY       *cmdkeys,
                   int numfields)
{

    sdlglIPushDisplay();

    /* Now replace these functions / arrays, wich are given */
    sdlglInputSet(fields, cmdkeys, numfields);
    
    if (drawfunc) {

        InputStack[InputStackIdx].DrawFunc = drawfunc;

    }

    if (! inputfunc) {

        inputfunc = sdlglIDefaultInputFunc;

    }
    InputStack[InputStackIdx].InputFunc   = inputfunc;
    
    /* Set inputdescs and drawdesc to same base */
    
    
    /* ------ Let OpenGL clear the display -------- */
    glClear(GL_COLOR_BUFFER_BIT);

}

/*
 * Name:
 *     sdlglInputSet
 * Description:
 *     Every non-NULL argument replaces the actual inputs in the actual display
 *     Drawing handler is taken from actual handler.
 * Input:
 *     fields:    A pointer on an array of SDLGL_FIELD's (may be NULL)
 *     cmdkeys:   A pointer on an array of SDLGL_CMDKEY's (may be NULL)
 *     numfields: Numner of fields in gieven array of fields 
 */
void sdlglInputSet(SDLGL_FIELD *fields, SDLGL_CMDKEY *cmdkeys, int numfields)
{
  
    SDLGL_CMDKEY *ck;
    int i;
    unsigned char mask;

    
    if (! cmdkeys) {

        cmdkeys = DefaultCmds;

    }
    else {
    
        /* Prepare the comparision mask for combined keys */
        ck = cmdkeys;
        while (ck -> keys[0] > 0) {

            ck -> keymask = 0;      
            for (i = 0, mask = 0x01; i < 2; i++, mask <<= 1) {
        
                if (ck -> keys[i] > 0) {
                
                    ck -> keymask |= mask;
                                    
                } 
                
            } 
        
            ck++;
            
        }
    
    }
    
    if (! fields) {

        fields = DefaultArea;

    }

    InputStack[InputStackIdx].DrawArea  = fields;
    InputStack[InputStackIdx].InputArea = fields;
    InputStack[InputStackIdx].CmdKeys   = cmdkeys;
    InputStack[InputStackIdx].num_total = numfields;

}

/*
 * Name:
 *     sdlglInputPopup
 * Description:
 *     Saves the previous state and uses the new handler and the new data for
 *     translation of input.
 *     Only the new given fields and commands are checked for input. 
 *     But all fields since last 'InputNew' are handed over to caller for drawing
 * Input:
 *     inputfunc: Function to call for this input
 *     fields:    A pointer on an array of SDLGL_FIELD's (may be NULL)
 *     cmdkeys:   A pointer on an array of SDLGL_CMDKEY's (may be NULL)
 * Output:
 *     None
 */
void sdlglInputPopup(SDLGL_INPUTFUNC inputfunc, SDLGL_FIELD *fields, SDLGL_CMDKEY *cmdkeys)
{

    if ((InputStackIdx + 2) < SDLGL_MAXINPUTSTACK) {    /* Push input */

    	/* Increment stackpointer only if possible */
        ++InputStackIdx;
        /* ------ Copy from previous input handler if one available ------ */
        if (InputStackIdx > 0) {

            memcpy(&InputStack[InputStackIdx],
                   &InputStack[InputStackIdx-1], sizeof(InputStack[0]));            

        }
        
        sdlglInputSet(fields, cmdkeys, 0);        
        
        if (! inputfunc) {

            inputfunc = sdlglIDefaultInputFunc;

        }
        InputStack[InputStackIdx].InputFunc = inputfunc;    
        /* Only the new fields are used for input, drawn ar all */
        if (! fields) {
            
            fields = DefaultArea;
            
        }
        
        InputStack[InputStackIdx].InputArea = fields;

    }
    
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
        
        if (mainConfig.tooltiptime == 0) {
        
            mainConfig.tooltiptime = SDLGL_TOOLTIPTIME;
            
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
    sdlglIResetInput();		/* Reset the internal handler stack to basic values     */

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
        ActualEvent.tickspassed   = tickspassed;
        ActualEvent.secondspassed = (float)tickspassed / (float)SDLGL_TICKS_PER_SECOND;
        ActualEvent.fps           = fps;

        /* Now get the actual position of the mouse. Always return it */
        SDL_GetMouseState(&ActualEvent.mou.x, &ActualEvent.mou.y);
        ActualEvent.mou.x = ActualEvent.mou.x * mainConfig.displaywidth / mainConfig.scrwidth;
        ActualEvent.mou.y = ActualEvent.mou.y * mainConfig.displayheight / mainConfig.scrheight;

        /* Now call the drawing function */
        InputStack[InputStackIdx].DrawFunc(InputStack[InputStackIdx].DrawArea, &ActualEvent);

        /* ------ Gather and translate user input -------- */
        if (sdlglICheckInput(&ActualEvent) == SDLGL_INPUT_EXIT) {

            done = 1;       /* Exit loop  */

        }
        else {

            /* Set values for timed input ... */
            ActualEvent.tickspassed = tickspassed;
            ActualEvent.secondspassed = (float)tickspassed / (float)SDLGL_TICKS_PER_SECOND;
            ActualEvent.fps         = fps;
            ActualEvent.field       = FocusArea;                            /* For handling          */

            switch(InputStack[InputStackIdx].InputFunc(&ActualEvent)) {

                case SDLGL_INPUT_EXIT:
                    done = 1;
                    break;

                case SDLGL_INPUT_REMOVE:
                	/* Remove an inputhandler and it's fields from inputstack */
                    sdlglIPopInput();
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
    fptr  = InputStack[InputStackIdx].InputArea;
    if (! fptr) {

        return;     /* No fields available at all */

    }

    /* 2) Remove old focus flag and set new, if field available */
    if (field) {
    
        while(fptr -> sdlgl_type != 0) {

            fptr -> fstate = 0;

            if (fptr == field) {
                            
                FocusArea = fptr;
                fptr -> fstate |= SDLGL_FSTATE_HASFOCUS;
                return;     /* New focus is set */

            }

            fptr++;

        }
        
        return;

    }

    /* 3) Find actual field having focus and set new one */
    fptr  = InputStack[InputStackIdx].InputArea;
    fbase = InputStack[InputStackIdx].InputArea;
    if (dir != 0) {
    
        while(fptr -> sdlgl_type != 0) {
    
            /* Find the actual field having the focus */
            if (fptr -> fstate & SDLGL_FSTATE_HASFOCUS) {
        
                fptr -> fstate = 0; /* Clear the focus */
                if (dir > 0) {
            
                    fptr++;
                    if (fptr -> sdlgl_type == 0) {    /* Wrap around */
                 
                        fptr = fbase;
                    
                    }
                    
                }
                else if (dir < 0) {
            
                    if (fptr == fbase) {    /* Wrap around */
                    
                        fptr = sdlglIFindTypeInAreas(0, InputStack[InputStackIdx].InputArea);
                        
                    }
                    else {
                    
                        fptr--;
                        
                    }
                        
                }
                
                /* Set new Focus */
                FocusArea -> fstate &= (unsigned char)~SDLGL_FSTATE_HASFOCUS;
                FocusArea = fptr;
                fptr -> fstate |= SDLGL_FSTATE_HASFOCUS;
                 
            }    
            
            fptr++;
            
        } /* while(fptr -> sdlgl_type != 0)  */ 
                   
    } /* if (dir != 0) */

}

/*
 * Name:
 *     sdlglInputRemove
 * Description:
 *     Removes input with given 
 *     Removes the block of fields in actual Input-Fields, signed by
 *     'block_sign', if available at all.
 * Input:
 *     block_sign: Value which signs the Input-Fields to remove
 */
void sdlglInputRemove(char block_sign)
{

    SDLGL_FIELD *pstart, *pend, *fields;
    
    
    /* 1) Check for valid 'block_sign'          */
    if (! block_sign) {
    
        return;
    
    }
    
    /* 2) Get fields pointer                    */
    fields = InputStack[InputStackIdx].InputArea;
    if (! fields) {

        return;

    }
    
    /* 1) Find block start, if it exists at all */
    while (fields -> sdlgl_type != 0) {

        if (fields -> block_sign == block_sign) {
        
            pstart = fields;
            
            while(fields -> block_sign == block_sign) {
                /* Find end of block */
                fields++;
                
            }
            
            pend = fields;
            /* Remove the buffer, copy at minimum the end sign   */
            do {

                memcpy(pstart, pend, sizeof(SDLGL_FIELD));
                pstart++;
                pend++;

            }
            while(pend -> sdlgl_type != 0);
            
            return;
            
        }

        fields++;

    }   
    
}

/*
 * Name:
 *     sdlglInputAdd
 * Description:
 *     Adds the given fields to actual Input-Fields. 
 *     If a field block signed with 'block_sign' already exists, it is replaced
 *     by the given field block.
 *     If the block doesn't fit into destination, the block isn't copied.
 * Input:
 *     block_sign: To set, replaces possible existing block
 *     psrc *:     Fields to add to Input-Fields.
 *     x, y:       Start at this position on screen (add to given pos)
 *                 (< 0: center it)
 */
void sdlglInputAdd(char block_sign, SDLGL_FIELD *psrc, int x, int y)
{

    SDLGL_FIELD *pdest, *pend;
    SDLGL_RECT center_pos;
    int num_act, num_src, num_total;


    /* 1) Check for valid 'block_sign'          */
    if (! block_sign) {

        return;

    }

    /* 2) Get fields pointer                    */
    pdest = InputStack[InputStackIdx].InputArea;
    if (! pdest ) {

        return;

    }

    /* 3) Remove possible existing input fields of same type */
    sdlglInputRemove(block_sign);

    /* 4) Check, if theres enough space left to copy new Input-Fields  */
    num_act   = sdlglICountFields(pdest);
    num_total = InputStack[InputStackIdx].num_total;

    /* 5) Count the fields, that should be copied   */
    num_src = sdlglICountFields(psrc);

    /* 6) Copy the fields, if there's enough space  */
    if ((num_total - num_act) >= num_src) {

        sdlglAdjustRectToScreen(&psrc -> rect, &center_pos, SDLGL_FRECT_SCRCENTER);
        if (x < 0) {
            x = center_pos.x;
        }
        if (y < 0) {
            y = center_pos.y;
        }
    
        pend = sdlglIFindTypeInAreas(0, pdest);
        
        while(psrc -> sdlgl_type != 0) {

            memcpy(pend, psrc,  sizeof(SDLGL_FIELD));
            /* Set block_sign' for handling 'remove' */
            pend -> block_sign = block_sign;
            pend -> rect.x += x;
            pend -> rect.y += y;
            pend++;
            psrc++;

        }   

        pend -> sdlgl_type = 0;   /* Sign end of buffer */
        
    }

}

