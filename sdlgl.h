/*******************************************************************************
*  SDLGL.H                                                                     *
*	- Entrypoint for the library, starts the menu-loop.		                   *
*									                                           *
*   Copyright (C) 2001  Paul Mueller <pmtech@swissonline.ch>                   *
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

#ifndef _SDLGL_MAIN_H_
#define _SDLGL_MAIN_H_

/*******************************************************************************
* INCLUDES								                                       *
*******************************************************************************/

#include <SDL.h>

/*******************************************************************************
* DEFINES AND INCLUDES FOR OpenGL                                              *
*******************************************************************************/

/* XXX This is from Win32's <windef.h> */
#  ifndef APIENTRY
#   if (_MSC_VER >= 800) || defined(_STDCALL_SUPPORTED) || defined(__BORLANDC__) /* Borland is needed */
#    define APIENTRY    __stdcall
#   else
#    define APIENTRY
#   endif
#  endif

   /* XXX This is from Win32's <winnt.h> */
#  ifndef CALLBACK
#   if (defined(_M_MRX000) || defined(_M_IX86) || defined(_M_ALPHA) || defined(_M_PPC)) && !defined(MIDL_PASS)
#    define CALLBACK __stdcall
#   else
#    define CALLBACK
#   endif
#  endif

   /* XXX This is from Win32's <wingdi.h> and <winnt.h> */
#  ifndef WINGDIAPI
#   define WINGDIAPI __declspec(dllimport)
#  endif
   /* XXX This is from Win32's <ctype.h> */
#  ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#   define _WCHAR_T_DEFINED
#  endif

#include <GL/gl.h> 		/* Header File For The OpenGL32 Library	*/
#include <GL/glu.h>     /* Header File For The GLu32 Library    *//*******************************************************************************
* DEFINES								                                       *
*******************************************************************************/

/* ------ Different screen modes ------ */
#define SDLGL_SCRMODE_WINDOWED   0
#define SDLGL_SCRMODE_FULLSCREEN 1
#define SDLGL_SCRMODE_AUTO       2              /* Not supported yet */

/* -- Flags for rectangle functions, the flags are tested from low to high -- */
#define SDLGL_FRECT_HCENTER     0x0001          /* Center the rectangle hor.  */
#define SDLGL_FRECT_VCENTER     0x0002          /* Center vertically          */
#define SDLGL_FRECT_SCRLEFT     0x0004          /* On left side of screen     */
#define SDLGL_FRECT_SCRRIGHT    0x0008          /* On right side scr          */
#define SDLGL_FRECT_SCRTOP      0x0010          /* At top of screen           */
#define SDLGL_FRECT_SCRBOTTOM   0x0020          /* At bottom of screen        */
#define SDLGL_FRECT_SCRWIDTH    0x0040          /* Set to screen width        */
#define SDLGL_FRECT_SCRHEIGHT   0x0080          /* Set to screen height       */
#define SDLGL_FRECT_SCRCENTER   (SDLGL_FRECT_HCENTER | SDLGL_FRECT_VCENTER)
#define SDLGL_FRECT_SCRSIZE     (SDLGL_FRECT_SCRWIDTH | SDLGL_FRECT_SCRHEIGHT)

/* Types of input - source */
#define SDLGL_INPUT_NONE	  0
#define SDLGL_INPUT_KEYBOARD  1 /* Untranslated				                */
#define SDLGL_INPUT_MOUSE     2 /* Untranslated				                */
#define SDLGL_INPUT_COMMAND   3 /* Is already translated:  	  	            */
                                /* "event -> code" holds generated code     */
#define SDLGL_INPUT_CLEANUP  10 /* Caller is sked for cleanup before pop    */  
/* Clock */
#define SDLGL_TICKS_PER_SECOND 1000

/* ------ Standard - Colors ----- */
#define SDLGL_COL_BLACK         0
#define SDLGL_COL_BLUE	        1
#define SDLGL_COL_GREEN         2
#define SDLGL_COL_CYAN 	        3
#define SDLGL_COL_RED 	        4
#define SDLGL_COL_MAGENTA       5
#define SDLGL_COL_BROWN         6
#define SDLGL_COL_LIGHTGREY     7
#define SDLGL_COL_DARKGRAY      8
#define SDLGL_COL_LIGHTBLUE     9
#define SDLGL_COL_LIGHTGREEN   10
#define SDLGL_COL_LIGHTCYAN    11
#define SDLGL_COL_LIGHTRED     12
#define SDLGL_COL_LIGHTMAGENTA 13
#define SDLGL_COL_YELLOW       14
#define SDLGL_COL_WHITE        15

/* Input results to be returned by the SDLGL_INPUTFUNC */
#define SDLGL_INPUT_OK       0  /* Anything else >= 0                       */
#define SDLGL_INPUT_EXIT    -1  /* Exits the library and ends process       */
#define SDLGL_INPUT_REMOVE  -2	/* Removes the actual display and its       */
                                /* input descriptors and handler            */
#define SDLGL_INPUT_RESET   -3	/* Reset to default input handler		    */

/* Error codes for sdlglInit. */
#define INIT_ERROR_NONE       0
#define INIT_ERROR_SDL_INIT   1
#define INIT_ERROR_SDL_SCREEN 2

/* ------- Special key definitions ------- */
#define SDLGL_KEY_CTRL          2000
#define SDLGL_KEY_ALT           2001
#define SDLGL_KEY_SHIFT         2002
#define SDLGL_KEY_MOULEFT       2003
#define SDLGL_KEY_MOUMIDDLE     2004
#define SDLGL_KEY_MOURIGHT      2005
#define SDLGL_KEY_MOUMOVE       2012
#define SDLGL_KEY_MOULDBLCLICK  2013
#define SDLGL_KEY_MOURDBLCLICK  2014
#define SDLGL_KEY_ANY           2030    /* Hand any key to given function */

/* ----- Focusstate for focus field -------- */
#define SDLGL_FSTATE_HASFOCUS  0x01
#define SDLGL_FSTATE_MOUSEOVER 0x02
#define SDLGL_FSTATE_MOULEFT   0x04     /* Left mouse key pressed in rect   */
#define SDLGL_FSTATE_MOURIGHT  0x08     /* Right mouse key pressed in rect  */
#define SDLGL_FSTATE_MOUMIDDLE 0x10
#define SDLGL_FSTATE_MOUDRAG   0x20     /* Mouse is dragged                 */ 
#define SDLGL_FSTATE_CLEAR     0xFE     /* Clear all, except focusflag      */

/*******************************************************************************
* GLOBAL TYPEDEFS							                                   *
*******************************************************************************/

typedef struct {

    int x, y;
    int w, h;

} SDLGL_RECT;               /* General rectangle                */

typedef struct {

    char *wincaption;		/* Caption for window, if any	    */
    int scrwidth, scrheight;/* Size of screen to set	        */
    int colordepth;		    /* Colordepth of screen		        */
    int wireframe;		    /* Draw mode: wireframe or filled   */
    int hidemouse;		    /* Hide mouse, if asked for	        */
    int screenmode;		    /* SDLGL_SCRMODE*       	        */
    int debugmode;		    /* Debugmode yes/no		            */
    int zbuffer;            /* > 0: Set the zbuffer             */
    int dblclicktime;       /* Time for sensing doubleclicks    */
    int displaywidth,
        displayheight;      /* Width and height of display to set */

} SDLGL_CONFIG;

typedef struct {

    int code;		/* Is returned if the given key(s) is/are pressed, 0 signs end of array     */
    int subcode;    /* Is returned if the given key(s) is/are pressed    */
    int keys[4];	/* Code for keys SDLK_...                         */
                    /* 'keys[0]' == 0 signs end of array              */
    int releasecode;/* Code to send if key is released                */  
    unsigned char mask;     /* The mask for the number of keys        */
                            /* Also for checking on RELEASED          */
    unsigned char pressed;  /* Flags if indexed key is pressed        */

} SDLGL_CMDKEY;

typedef struct {

    int type;           /* If 0, signs the end of an inputdesc-array	*/
    			        /* type for drawing und handling.		        */
    int code;	        /* 'code' and 'subcode' are returned if a mouse */
                        /* button is pressed on rectangle or 'shortcut' */
    int subcode;        /* is pressed.                                  */
    SDLGL_RECT rect;    /* Rectangle to be checked for mouse position.	*/
    int shortcut;       /* Key which generates an 'event' for this one  */
    /* -------  General purpose data for user. Not used by sdlgl. ----- */
    char *caption; 	    /* Caption of this input, if any.		        */
                        /* Pointer  on data used for 'SDLGLFLD'         */
    int  displaytype;   /* For following textures                       */
    int  tex;           /* Texture to draw                              */
    int  subtex;        /* Number of subpart of texture, if any         */
    int  colorno;       /* Number of color to draw                       */
    unsigned char color[4]; /* If color included here                   */
    int value;              /* Rotation angle, if rotated               */
                            /* or what ever else                        */
    unsigned char fstate;   /* SDLGL_FSTATE_*                           */

    } SDLGL_FIELD;		    /* Info returned from input for drawing.	    */


typedef struct {

    int type;		/* Type of input SDLGL_INPUT_*		        */
    int code;	    /* Code from INPUTVEC, if in a rect	        */
    			    /* 0, if no code generated by input         */
                    /* translation				                */
    int subcode;    /* Additional code for input                */
    int sdlcode;	/* SDLK_... code			                */
    int modflags;   /* Flags of mod keys			            */
    			    /* SDL_BUTTON_... if SDLGL_INP_MOUSE	    */
    			    /* ------ Mouse related stuff ------------- */
    int relx,       /* Relative position ( from upper     	    */
        rely;       /* left ) into the SDLGL_FIELD-rectangle 	*/
        		    /* if on such a rectagle.		            */
                    /* Relative mouse movement since last	    */
                    /* call if mouse movement		            */
                    /* Else position from left top of           */
                    /* screen.		 		                    */
    int movex,      /* Distance the mouse has moved since       */
        movey;      /* last event. For drag events.             */
			        /* --------- Additional info -------------- */
    int   tickspassed;	  /* Ticks passed since last call of input  */
    float secondspassed;  /* Seconds passed since last call         */   
    int   clicktime;	  /* How long a mouse button is pressed	    */
                          /* In ticks                               */
    SDLGL_FIELD  *field;  /* Points on the chosen field, if any     */
                          /* Points on dummy field, if none         */
    SDLGL_CMDKEY *cmd;    /* Points on the list of actual commands  */
    SDLGL_FIELD  *fieldlist;    /* Points on list of input fields   */
    int  fps;
    unsigned char pressed;/* For untranslated input                 */

} SDLGL_EVENT;

/* ------ Drawing and input functions -------- */
typedef void (* SDLGL_DRAWFUNC)(SDLGL_FIELD *fields, SDLGL_EVENT *event);
    /* Type definition of inputhandler: Is called for every input generated
       by a definition of a SDLGL_FIELD or SDLGL_CMDKEY */
typedef int  (* SDLGL_INPUTFUNC)(SDLGL_EVENT *event);

/*******************************************************************************
* CODE 								                                           *
*******************************************************************************/

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/* -------- Creating displays and setting inputvectors ---------- */
void sdlglCreateDisplay(SDLGL_DRAWFUNC  drawfunc,
                        SDLGL_INPUTFUNC inputfunc,
                        SDLGL_FIELD     *fields,
                        SDLGL_CMDKEY    *cmdkeys,
                        int numfields);

void sdlglSetInput(SDLGL_FIELD *fields, SDLGL_CMDKEY *cmdkeys);
void sdlglPopUpFields(SDLGL_INPUTFUNC inputfunc,
                      SDLGL_FIELD *fields, SDLGL_CMDKEY *cmdkeys);

/* To be called first. After this one is called, the graphics screen is set */
/* up and all OpenGL functions are available 				                */
int  sdlglInit(SDLGL_CONFIG *configdata);

/* Main loop itself. Never returns, until the users (or the standard)  */
/* inputhandler function returns "SDLGL_FIELD_EXIT"		       */
int  sdlglExecute(void);

/* Procedure to be called after the user made all cleanup work for his  */
/* calls of the OpenGL graphics interface				*/
void sdlglShutdown(void);

/* ----------- Some little help functions -----------  */
void sdlglSetColor(int colno);	                      /* Some standard colors */
void sdlglGetColor(int colno, unsigned char *col_buf);
void sdlglSetPalette(int from, int to, unsigned char *colno);
void sdlglAdjustRectToScreen(SDLGL_RECT *src, SDLGL_RECT *dst, int flags); 
int  sdlglScreenShot(const char *filename);
void sdlglSetViewSize(int width, int height);

void sdlglInputSetFocus(SDLGL_FIELD *field, int dir);
SDLGL_FIELD *sdlglFindType(int typeno);
void sdlglRemoveFieldBlock(int blocksign);
void sdlglSetFieldBlock(int blocksign, SDLGL_FIELD *src);

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _SDLGL_MAIN_H_	*/

