/*******************************************************************************
*  EGOEVENT.H                                                                  *
*	   - Translates the input from different SDL-devices to Egoboo-Events      *
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

#ifndef _EVENT_H_
#define _EVENT_H_

/*******************************************************************************
* DEFINES 								                                       *
*******************************************************************************/

/* ----- Focusstate for focus field -------- */
#define EVENT_FSTATE_HASFOCUS   0x01
#define EVENT_FSTATE_MOUSEOVER  0x02
#define EVENT_FSTATE_MOUPRESSED 0x03

#define EGOCODE_UPDATETIMER 100
#define EGOCODE_QUITMODULE  101
#define EGOCODE_PAUSEGAME   102
#define EGOCODE_QUITTOOS    103


/* -------- Game codes ---------------- */
#define EGOCODE_NOINPUT	        0
#define EGOCODE_JUMP            1
#define EGOCODE_LEFT_USE        2
#define EGOCODE_LEFT_GET        3
#define EGOCODE_LEFT_PACK       4
#define EGOCODE_RIGHT_USE       5
#define EGOCODE_RIGHT_GET       6
#define EGOCODE_RIGHT_PACK      7
#define EGOCODE_RESPAWN	        8
#define EGOCODE_MESSAGE         9
#define EGOCODE_CAMERA_LEFT    10
#define EGOCODE_CAMERA_RIGHT   11
#define EGOCODE_CAMERA_IN      12
#define EGOCODE_CAMERA_OUT     13
#define EGOCODE_ARROWUP        14
#define EGOCODE_ARROWDOWN      15
#define EGOCODE_ARROWLEFT      16
#define EGOCODE_ARROWRIGHT     17
#define EGOCODE_CAMERAMODE     18
#define EGOCODE_MOVEMENT       19

/* LEFT SHIFT   + 1 to 7 - Show selected character armor without magic enchants */
#define EGOCODE_SHOWSTAT_LSHIFT 21
/* LEFT CONTROL + 1 to 7 - Show armor stats with magic enchants included */
#define EGOCODE_SHOWSTAT_LCTRL  22
/* LEFT ALT     + 1 to 7 - Show character magic enchants    */
#define EGOCODE_SHOWSTAT_LALT   23
#define EGOCODE_SHOWSTAT        24

#define EGOCODE_GIVEEXPERIENCE 27	// , Cheat for program test
#define EGOCODE_SCREENSHOT     28
#define EGOCODE_ESCAPE	       29
#define EGOCODE_HELPMOUSE      31	// SDLK_F1
#define EGOCODE_HELPJOYSTICK   32	// SDLK_F2
#define EGOCODE_HELPKEYBOARD   33	// SDLK_F3
#define EGOCODE_DEBUGMODE5     34	// SDLK_F5
#define EGOCODE_DEBUGMODE6     35	// SDLK_F6
#define EGOCODE_DEBUGMODE7     36	// SDLK_F7 <-> Switch textures??

#define EGOCODE_MAPCHEAT       50
#define EGOCODE_LIFECHEAT      51

/******************************************************************************
* TYPEDEFS                                                                    *
******************************************************************************/

typedef struct {

    int x, y;
    int w, h;

} EVENT_RECT;               /* General rectangle for mouse input     */

typedef struct {

    int code;		/* Is returned if the given key(s) is/are pressed, 
                       0 signs end of array     */
    int subcode;    /* Is returned on press and release               */
    int keys[2];	        /* Code for keys SDLK_...                 */
                            /* 0: Preselect / single key
                               1: Postselect / combined key           */
    char send_release;      /* Send code on key release yes/no        */  
    unsigned char pressed;  /* Flags set if keys[] is pressed         */
                            /* For internal use                       */

} EVENT_CMD;

typedef struct {

    int type;           /* If 0, signs the end of an range-array    	*/
    			        /* type for drawing und handling (user-defined) */
    int code;	        /* 'code' and 'subcode' are returned if a mouse */
                        /* button is pressed on rectangle or 'shortcut' */
    int subcode;        /* is pressed.                                  */
    EVENT_RECT rect;    /* Rectangle to be checked for mouse position.	*/
    int shortcut;       /* Key which generates an 'event' for this one  */
    char fstate;        /* Pointing device over (and clicked)           */
    /* -------  General purpose data for user. Not used by sdlgl. ----- */
    char *caption; 	    /* Caption of this input, if any(user-defined) 	*/
    
} EVENT_RANGE;		 

typedef struct {

    int code;	    /* > 0, if an event is generated by input 	*/
    int subcode;    /* Additional info from command-definition  */
    int sdlcode;	/* SDLK_... code			                */
    int modflags;   /* Flags of mod keys			            */
    			    /* SDL_BUTTON_... if input from mouse	    */
    			    /* ------ Mouse related stuff ------------- */
    int posx,       /* Position ( from upper left )      	    */
        posy;       /* into the EVENT_RECT-rectangle       	    */
        		    /* if on such a rectagle.		            */
                    /* Else position from left top of           */
                    /* screen.		 		                    */
                    /* Joystick: Joystick movement              */
    int movex,      /* Distance the mouse has moved since       */
        movey;      /* last event. For drag events.             */
			        /* --------- Additional info -------------- */
    int   tickspassed;	  /* Ticks passed since last call of input  */
    float secondspassed;  /* Seconds passed since last call         */   
    int   clicktime;	  /* How long a mouse button is pressed	    */
                          /* In ticks                               */
    EVENT_RANGE *range;   /* Points on the chosen range, if any     */
                          /* Points on dummy field, if none         */
    char pressed;         /* For handling of release codes          */
    char textkey;         /* Key as char for writing of text        */
    int  mousex, mousey;  /* Position of mouse pointer at call      */                       
    int  fps;
      

} EVENT_INFO;

/*******************************************************************************
* CODE 								                                           *
*******************************************************************************/

void eventInit(void);
void eventSetInputTable(EVENT_RANGE *ranges, EVENT_CMD *cmd);
int  eventGetInput(EVENT_INFO *event);
void eventSetGameInputTable(void);
int  eventTranslateGameInput(EVENT_INFO *event);
void eventReadControls(void);


#endif /* _EVENT_H_ */