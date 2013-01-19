/*******************************************************************************
*  SDLGLSTR.C                                                                  *
*      - Output of text in an OpenGL-Window.                                   *
*                                                                              *
*   SDLGL - Library                                                            *
*      Copyright (c) 2004-2010 Paul Mueller <pmtech@swissonline.ch>            *
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
* INCLUDES							                                           *
*******************************************************************************/

/* The bitmaps are integrated. Makes it easier to print out some text   */
/*  without thinking of it to have loaded some stuff. E. g. for debug   */

#include <stdio.h>              /* sprintf */
#include <stdarg.h>
#include <string.h>
#include <memory.h>		        /* memcpy */


/* ---- Own headers --- */
#include "sdlglstr.h"


/*******************************************************************************
* DEFINES								                                       *
*******************************************************************************/

#define SDLGLSTR_MAXFONT  5
#define SDLGLSTR_TABSIZE      8

#define SDLGLSTR_MAXTEXCHAR 512	/* Four fonts with 127 chars */

/* Internal colors */
#define SDLGLSTR_MENUCOL1  16
#define SDLGLSTR_MENUCOL2  17
#define SDLGLSTR_MENUCOL3  18
#define SDLGLSTR_MENUCOL4  19
#define SDLGLSTR_MENUCOL5  20
#define SDLGLSTR_USERCOLOR 21
#define SDLGLSTR_MAXCOLOR  20

/* Internal constants */
#define SDLGLSTR_FIELDBORDER  4	/* Width of border in pixels 		        */
#define SDLGLSTR_SPECIALDIST 12	/* Distance between start of special field  */
            				/* and its text 			                */

/*******************************************************************************
* TYPEDEFS								                                       *
*******************************************************************************/

typedef struct {

    GLubyte *font;
    int	 fontw,		/* fontb */
    	 fonth;		/* Width, heigth of font */
    int  cwidth;	/* Width of Char-Bitmap */
    char *xadd;     /* List for xadd for every char */

    /* Extensions */
    GLuint textureID;

} FONT;


/******************************************************************************
* DATA  		      						      *
******************************************************************************/

static int ActFont  = 0;
static int ActColor = SDLGL_COL_WHITE;

static GLubyte fontcolors[][3] = {

    { 0,   0,   0   },  /* 0. BLACK */
    { 0,   0,   170 },  /* 1. BLUE */
    { 0,   170, 0   },  /* 2. GREEN */
    { 0,   170, 170 },  /* 3. CYAN */
    { 170, 0,   0   },  /* 4. RED */
    { 170, 0,   170 },  /* 5. MAGENTA */
    { 170, 170, 0   },  /* 6. BROWN */
    { 170, 170, 170 },  /* 7. LIGHTGREY */
    { 85,  85,  85  },  /* 8. DARKGRAY */
    { 85,  85,  255 },  /* 9. LIGHTBLUE */
    { 85,  255, 85  },  /* 10. LIGHTGREEN */
    { 85,  255, 255 },  /* 11. LIGHTCYAN */
    { 255, 85,  85  },  /* 12. LIGHTRED */
    { 255, 85,  255 },  /* 13. LIGHTMAGENTA */
    { 255, 255, 85  },  /* 14. YELLOW */
    { 255, 255, 255 },  /* 15. WHITE */
    /* The special menu/dialog colors */
    { 128,  64,  64 }, 	   /*  16. MENUCOL1 */
    { 154,  78,  78 }, 	   /*  17. MENUCOL2 */
    { 180,  92,  92 },     /*  18. MENUCOL3 */
    { 0xFF, 0xFF, 0xFF },  /* 19. MENUCOL4 */
    { 0x28, 0x20, 0xAC },   /* 20. MENUCOL5 */	/* Slider background */
    { 0xFF, 0xFF, 0xFF }    /* 21. USER DEFINED COLOR   */

};

static GLubyte _Font6[] = {

    /*  C = 0 chars 0..7, holding ü, ö, ä */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x88, 0xF8, 0x88, 0x70, 0x50,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x88, 0xF8, 0x88, 0x70, 0x50,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    /* 8..15 -------------------	*/
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    /* 16..23 -------------------	*/
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x70, 0x88, 0x88, 0x70, 0x50,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    /* 24..31 -------------------	*/
    /* Arrows		*/
    0x00, 0x20, 0x20, 0xF8, 0x70, 0x20,
    0x00, 0x20, 0x70, 0xF8, 0x20, 0x20,
    0x00, 0x20, 0x30, 0xF8, 0x30, 0x20,
    0x00, 0x20, 0x60, 0xF8, 0x60, 0x20,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    /* 32..39 -------------------	*/
    /* Start with SPACE at 32		*/
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x20, 0x00, 0x20, 0x20, 0x20,
    0x00, 0x00, 0x00, 0x00, 0x90, 0x90,
    0x00, 0x50, 0xF8, 0x50, 0xF8, 0x50,
    0x00, 0x60, 0x30, 0x70, 0x60, 0x30,
    0x00, 0x88, 0x40, 0x20, 0x10, 0x88,
    0x00, 0x74, 0xD8, 0x60, 0x50, 0x70,
    0x18, 0x20, 0x00, 0x00, 0x00, 0x00,

    // 40..47 -------------------
    0x00, 0x20, 0x40, 0x40, 0x40, 0x20,
    0x00, 0x40, 0x20, 0x20, 0x20, 0x40,
    0x00, 0xA8, 0x70, 0xF8, 0x70, 0xA8,
    0x00, 0x20, 0x20, 0xF8, 0x20, 0x20,
    0x00, 0x20, 0x10, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0xF8, 0x00, 0x00,
    0x00, 0x20, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x80, 0x40, 0x20, 0x10, 0x08,

    // 48..55 -------------------
    0x00, 0x70, 0xC8, 0xA8, 0x98, 0x70,
    0x00, 0x70, 0x20, 0x20, 0x60, 0x20,
    0x00, 0xF8, 0x80, 0x70, 0x08, 0xF0,
    0x00, 0xF0, 0x08, 0x78, 0x08, 0xF0,
    0x00, 0x10, 0xF8, 0x90, 0x50, 0x30,
    0x00, 0xF0, 0x08, 0xF0, 0x80, 0xF0,
    0x00, 0x70, 0x88, 0xF0, 0x80, 0x70,
    0x00, 0x20, 0x20, 0x10, 0x08, 0xF8,

    // 56..63 -------------------
    0x00, 0x70, 0x88, 0x70, 0x88, 0x70,
    0x00, 0x70, 0x08, 0x78, 0x88, 0x70,
    0x00, 0x00, 0x10, 0x00, 0x10, 0x00,
    0x00, 0x20, 0x10, 0x00, 0x10, 0x00,
    0x00, 0x20, 0x40, 0x80, 0x40, 0x20,
    0x00, 0x00, 0xF8, 0x00, 0xF8, 0x00,
    0x00, 0x40, 0x20, 0x10, 0x20, 0x40,
    0x00, 0x20, 0x00, 0x30, 0x08, 0x70,

    // 64..71 -------------------
    0x00, 0xF8, 0x80, 0xB8, 0xA8, 0x78,
    0x00, 0x88, 0x88, 0xF8, 0x88, 0x70,
    0x00, 0xF0, 0x88, 0xF0, 0x88, 0xF0,
    0x00, 0x70, 0x88, 0x80, 0x88, 0x70,
    0x00, 0xF0, 0x88, 0x88, 0x88, 0xF0,
    0x00, 0xF8, 0x80, 0xF0, 0x80, 0xF8,
    0x00, 0x80, 0x80, 0xF0, 0x80, 0xF8,
    0x00, 0x70, 0x88, 0x98, 0x80, 0x70,

    // 72..79 -------------------
    0x00, 0x88, 0x88, 0xF8, 0x88, 0x88,
    0x00, 0x70, 0x20, 0x20, 0x20, 0x70,
    0x00, 0x70, 0x88, 0x08, 0x08, 0x08,
    0x00, 0x88, 0x90, 0xE0, 0xA0, 0x90,
    0x00, 0xF8, 0x80, 0x80, 0x80, 0x80,
    0x00, 0x88, 0x88, 0xA8, 0xD8, 0x88,
    0x00, 0x88, 0x98, 0xA8, 0xC8, 0x88,
    0x00, 0x70, 0x88, 0x88, 0x88, 0x70,

    // 80..87 -------------------
    0x00, 0x80, 0x80, 0xF0, 0x88, 0xF0,
    0x00, 0x68, 0x90, 0x88, 0x88, 0x70,
    0x00, 0x88, 0x88, 0xF0, 0x88, 0xF0,
    0x00, 0xF0, 0x08, 0x70, 0x80, 0x78,
    0x00, 0x20, 0x20, 0x20, 0x20, 0xF8,
    0x00, 0x70, 0x88, 0x88, 0x88, 0x88,
    0x00, 0x20, 0x50, 0x88, 0x88, 0x88,
    0x00, 0x50, 0xA8, 0xA8, 0x88, 0x88,

    // 88..95 -------------------
    0x00, 0x88, 0x50, 0x20, 0x50, 0x88,
    0x00, 0x20, 0x20, 0x70, 0x88, 0x88,
    0x00, 0xF8, 0x40, 0x20, 0x10, 0xF8,
    0x00, 0xE0, 0x80, 0x80, 0x80, 0xE0,
    0x00, 0x08, 0x10, 0x20, 0x40, 0x80,
    0x00, 0xE0, 0x20, 0x20, 0x20, 0xE0,
    0x1C, 0x14, 0x1C, 0xE0, 0xA0, 0xE0,
    0xFC, 0x00, 0x00, 0x00, 0x00, 0x00,

    // 96..103 -------------------
    0x00, 0x00, 0x00, 0x00, 0x10, 0x20,
    0x00, 0x88, 0x88, 0xF8, 0x88, 0x70,
    0x00, 0xF0, 0x88, 0xF0, 0x88, 0xF0,
    0x00, 0x70, 0x88, 0x80, 0x88, 0x70,
    0x00, 0xF0, 0x88, 0x88, 0x88, 0xF0,
    0x00, 0xF8, 0x80, 0xF0, 0x80, 0xF8,
    0x00, 0x80, 0x80, 0xF0, 0x80, 0xF8,
    0x00, 0x70, 0x88, 0x98, 0x80, 0x70,

    // 104..111 -------------------
    0x00, 0x88, 0x88, 0xF8, 0x88, 0x88,
    0x00, 0x70, 0x20, 0x20, 0x20, 0x70,
    0x00, 0x70, 0x88, 0x08, 0x08, 0x08,
    0x00, 0x88, 0x90, 0xE0, 0xA0, 0x90,
    0x00, 0xF8, 0x80, 0x80, 0x80, 0x80,
    0x00, 0x88, 0x88, 0xA8, 0xD8, 0x88,
    0x00, 0x88, 0x98, 0xA8, 0xC8, 0x88,
    0x00, 0x70, 0x88, 0x88, 0x88, 0x70,

    // 112..119 -------------------
    0x00, 0x80, 0x80, 0xF0, 0x88, 0xF0,
    0x00, 0x68, 0x90, 0x88, 0x88, 0x70,
    0x00, 0x88, 0x88, 0xF0, 0x88, 0xF0,
    0x00, 0xF0, 0x08, 0x70, 0x80, 0x78,
    0x00, 0x20, 0x20, 0x20, 0x20, 0xF8,
    0x00, 0x70, 0x88, 0x88, 0x88, 0x88,
    0x00, 0x20, 0x50, 0x88, 0x88, 0x88,
    0x00, 0x50, 0xA8, 0xA8, 0x88, 0x88,

    // 120..127 -------------------
    0x00, 0x88, 0x50, 0x20, 0x50, 0x88,
    0x00, 0x20, 0x20, 0x70, 0x88, 0x88,
    0x00, 0xF8, 0x40, 0x20, 0x10, 0xF8,
    0x00, 0x30, 0x40, 0x80, 0x40, 0x30,
    0x00, 0x20, 0x20, 0x20, 0x20, 0x20,
    0x00, 0xC0, 0x20, 0x30, 0x20, 0xC0,
    0x00, 0x00, 0x10, 0xA8, 0x40, 0x00,
    0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC,

};

static GLubyte _Font8[] = {

    // 1..7 ------------------- // 8 dup (0)

    // db 16 dup (0) //  '„'
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x3C, 0x66, 0x66, 0x66, 0x66, 0x00, 0x6C,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x3C, 0x66, 0x3E, 0x06, 0x3C, 0x00, 0x34,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x20, 0x30, 0x38, 0x3C, 0x38, 0x30, 0x20, /* Arrow right	    */
    0x00, 0x04, 0x0C, 0x1C, 0x3C, 0x1C, 0x0C, 0x04, /* Arrow left	    */

    /* 8..15 ------------------- */
    0x00, 0x00, 0x10, 0x38, 0x7C, 0xFE, 0x00, 0x00, /* Arrow down	    */
    0x00, 0x00, 0xFE, 0x7C, 0x38, 0x10, 0x00, 0x00, /* Arrow up		    */
    0x3C, 0x42, 0x99, 0xBD, 0xBD, 0x99, 0x42, 0x3C, /* Radio button set	    */
    0x3C, 0x42, 0x81, 0x81, 0x81, 0x81, 0x42, 0x3C, /* Radio button empty   */
    0x3C, 0x7E, 0xFF, 0xFF, 0xFF, 0xFF, 0x7E, 0x3C, /* Backgd. Radio button */
    0xFF, 0xC3, 0xA5, 0x99, 0x99, 0xA5, 0xC3, 0xFF, /* Pushbutton activated */
    0xFF, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0xFF, /* Pushbutton empty     */
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, /* Filled - Background  */

    /* 16..23 ------------------- */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x3C, 0x66, 0x66, 0x66, 0x3C, 0x00, 0x6C,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    /* 24..31 -------------------       */
    /*  Arrow Up / ArrowDown / ArrowLeft / ArrowRight        */

    0x00, 0x10, 0x10, 0x10, 0x92, 0x7C, 0x38, 0x10,
    0x00, 0x10, 0x38, 0x7C, 0x92, 0x10, 0x10, 0x10,
    0x00, 0x10, 0x20, 0x60, 0xFE, 0x60, 0x20, 0x10,
    0x00, 0x10, 0x08, 0x0C, 0xFE, 0x0C, 0x08, 0x10,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    // 32..39 ------------------- Start with Blank
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x18, 0x00, 0x18, 0x18, 0x18, 0x18, 0x18,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x66, 0x66, 0x66,
    0x00, 0x08, 0xFE, 0x08, 0x24, 0xFE, 0x24, 0x00,
    0x00, 0x18, 0x3C, 0x4C, 0x18, 0x32, 0x3C, 0x18,
    0x00, 0x46, 0x66, 0x30, 0x18, 0x6C, 0x64, 0x00,
    0x00, 0x76, 0xEC, 0xDC, 0x72, 0x38, 0x6C, 0x38,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x18, 0x18,

    // 40..47 -------------------
    0x00, 0x0E, 0x18, 0x30, 0x30, 0x30, 0x18, 0x0E,
    0x00, 0x70, 0x18, 0x0C, 0x0C, 0x0C, 0x18, 0x70,
    0x00, 0x10, 0x54, 0x08, 0xC6, 0x08, 0x54, 0x10,
    0x00, 0x18, 0x18, 0x7E, 0x7E, 0x18, 0x18, 0x00,
    0x00, 0x60, 0x30, 0x30, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x00,
    0x00, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x40, 0x60, 0x30, 0x18, 0x0C, 0x06, 0x02,

    // 48..55 -------------------
    0x00, 0x3C, 0x66, 0x76, 0x6E, 0x66, 0x66, 0x3C,
    0x00, 0x38, 0x18, 0x18, 0x18, 0x18, 0x38, 0x18,
    0x00, 0x7E, 0x66, 0x30, 0x0C, 0x46, 0x66, 0x3C,
    0x00, 0x3C, 0x46, 0x06, 0x0C, 0x18, 0x4C, 0x3E,
    0x00, 0x1C, 0x0C, 0x7E, 0x64, 0x34, 0x1C, 0x3C,
    0x00, 0x3C, 0x66, 0x46, 0x0E, 0x7C, 0x60, 0x7E,
    0x00, 0x3C, 0x66, 0x66, 0x7C, 0x60, 0x62, 0x3C,
    0x00, 0x30, 0x30, 0x30, 0x18, 0x0C, 0x66, 0x7E,

    // 56..63 -------------------
    0x00, 0x3C, 0x66, 0x6E, 0x3C, 0x76, 0x66, 0x3C,
    0x00, 0x3C, 0x46, 0x06, 0x3E, 0x66, 0x66, 0x3C,
    0x00, 0x00, 0x18, 0x18, 0x00, 0x18, 0x18, 0x00,
    0x00, 0x30, 0x18, 0x18, 0x00, 0x18, 0x18, 0x00,
    0x00, 0x00, 0x12, 0x24, 0x6C, 0x24, 0x12, 0x00,
    0x00, 0x00, 0x7E, 0x00, 0x00, 0x7E, 0x00, 0x00,
    0x00, 0x00, 0x08, 0x24, 0x36, 0x24, 0x08, 0x00,
    0x00, 0x18, 0x00, 0x18, 0x0C, 0x06, 0x66, 0x3C,

    // 64..71 -------------------
    0x00, 0x3E, 0x60, 0x66, 0x6A, 0x6E, 0x66, 0x3C,
    0x00, 0xCC, 0x46, 0x7E, 0x26, 0x36, 0x1E, 0x0E,
    0x00, 0xF8, 0x66, 0x66, 0x7C, 0x66, 0x66, 0xFC,
    0x00, 0x3C, 0x66, 0xC0, 0xC0, 0xC0, 0x66, 0x3C,
    0x00, 0xF0, 0x6C, 0x66, 0x66, 0x66, 0x6C, 0xF8,
    0x00, 0xF8, 0x66, 0x60, 0x78, 0x60, 0x66, 0xFC,
    0x00, 0x40, 0x60, 0x60, 0x78, 0x60, 0x66, 0x7C,
    0x00, 0x38, 0x66, 0xC6, 0xCE, 0xC0, 0x66, 0x3C,

    // 72..79 -------------------
    0x00, 0xE4, 0x66, 0x66, 0x7E, 0x66, 0x66, 0xE6,
    0x00, 0x30, 0x18, 0x18, 0x18, 0x18, 0x18, 0x38,
    0x00, 0x38, 0x6C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1C,
    0x00, 0xE4, 0x6E, 0x78, 0x70, 0x78, 0x6C, 0xE6,
    0x00, 0xFC, 0x66, 0x60, 0x60, 0x60, 0x60, 0xE0,
    0x00, 0xE4, 0x62, 0x62, 0x6A, 0x7E, 0x76, 0xE2,
    0x00, 0xE4, 0x66, 0x6E, 0x7A, 0x72, 0x72, 0xE2,
    0x00, 0x38, 0x6C, 0xC6, 0xC6, 0xC6, 0x6C, 0x38,

    /* 80..87 -------------------       */
    0x00, 0xC0, 0x60, 0x60, 0x7C, 0x66, 0x66, 0xFC,
    0x00, 0x0C, 0x68, 0xD4, 0xC6, 0xC6, 0x6C, 0x38,
    0x00, 0xE4, 0x66, 0x6C, 0x7C, 0x66, 0x66, 0xFC,
    0x00, 0x78, 0x66, 0x06, 0x1C, 0x30, 0x66, 0x3C,
    0x00, 0x60, 0x30, 0x30, 0x30, 0x30, 0xB2, 0xFE,
    0x00, 0x38, 0x74, 0x62, 0x62, 0x62, 0x62, 0xE6,
    0x00, 0x10, 0x18, 0x34, 0x34, 0x62, 0x62, 0xC6,
    0x00, 0xC4, 0xEE, 0xFA, 0xD2, 0xC2, 0xC2, 0xC6,

    /* 88..95 -------------------       */
    0x00, 0xC4, 0x66, 0x3C, 0x18, 0x3C, 0x66, 0xC6,
    0x00, 0x30, 0x18, 0x18, 0x18, 0x34, 0x62, 0xC6,
    0x00, 0x7C, 0x66, 0x30, 0x18, 0x0C, 0x66, 0x3E,
    0x00, 0x1E, 0x18, 0x10, 0x10, 0x10, 0x18, 0x1E,
    0x00, 0x02, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x40,
    0x00, 0x78, 0x18, 0x08, 0x08, 0x08, 0x18, 0x78,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // List is underscore

    /* 96..103 -------------------      */
    0x00, 0x00, 0x00, 0x00, 0x08, 0x18, 0x18, 0x00,
    0x00, 0x3C, 0x66, 0x3E, 0x06, 0x3C, 0x00, 0x00,
    0x00, 0xFC, 0x66, 0x66, 0x66, 0x7C, 0x60, 0xE0,
    0x00, 0x38, 0x64, 0x60, 0x64, 0x38, 0x00, 0x00,
    0x00, 0x3C, 0x66, 0x66, 0x66, 0x3E, 0x06, 0x0E,
    0x00, 0x3C, 0x60, 0x7E, 0x66, 0x3C, 0x00, 0x00,
    0x00, 0x60, 0x30, 0x30, 0x30, 0x7C, 0x30, 0x1C,
    0x00, 0x38, 0x04, 0x3E, 0x66, 0x3E, 0x00, 0x00,

    /* 104..111  -------------------    */
    0x00, 0xE4, 0x66, 0x66, 0x66, 0x7C, 0x60, 0xE0,
    0x00, 0x30, 0x18, 0x18, 0x18, 0x38, 0x00, 0x18,
    0x00, 0x60, 0x90, 0x18, 0x18, 0x38, 0x00, 0x18,
    0x00, 0xE6, 0x6C, 0x78, 0x6C, 0x66, 0x60, 0xE0,
    0x00, 0x30, 0x18, 0x18, 0x18, 0x18, 0x18, 0x38,
    0x00, 0xC4, 0xC6, 0xD6, 0xFE, 0xEC, 0x00, 0x00,
    0x00, 0x64, 0x66, 0x66, 0x66, 0x7C, 0x00, 0x00,
    0x00, 0x3C, 0x66, 0x66, 0x66, 0x3C, 0x00, 0x00,

    /* 112..119  -------------------            */
    0x00, 0x60, 0x78, 0x66, 0x66, 0xFC, 0x00, 0x00,
    0x00, 0x06, 0x3E, 0x66, 0x66, 0x3E, 0x00, 0x00,
    0x00, 0x40, 0x60, 0x60, 0x6C, 0x78, 0x00, 0x00,
    0x00, 0x7C, 0xC6, 0x3C, 0x60, 0x3E, 0x00, 0x00,
    0x00, 0x30, 0x18, 0x18, 0x18, 0x7E, 0x18, 0x00,
    0x00, 0x3C, 0x66, 0x66, 0x66, 0x66, 0x00, 0x00,
    0x00, 0x18, 0x3C, 0x66, 0x66, 0x66, 0x00, 0x00,
    0x00, 0x68, 0x7C, 0xD6, 0xC6, 0xC6, 0x00, 0x00,

    /*   120..127  -------------------  */
    0x00, 0x66, 0x3C, 0x18, 0x3C, 0x66, 0x00, 0x00,
    0x00, 0x78, 0x84, 0x3E, 0x66, 0x66, 0x00, 0x00,
    0x00, 0x7C, 0x32, 0x18, 0x8C, 0x7E, 0x00, 0x00,
    0x00, 0x0E, 0x18, 0x18, 0x30, 0x18, 0x18, 0x0E,
    0x00, 0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x18,
    0x00, 0x70, 0x18, 0x18, 0x0C, 0x18, 0x18, 0x70,
    0x00, 0x00, 0x00, 0x0C, 0x9E, 0xF2, 0x60, 0x00,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,

    /* 128 ----------                   */
    0x00, 0x00, 0x00, 0x00, 0xC6, 0x6C, 0x38, 0x10,

};


/* --------------- */
static FONT _wFonts[SDLGLSTR_MAXFONT] = {

    /* _Font6	*/
    { _Font6, 6, 6, 8 },
    /* --------------- */
    /* _Font8	*/
    { _Font8, 8, 8, 8 },
    /* ---------------	*/
    /* Installed Font(s)	*/
    { _Font8, 8, 8, 8 }

};

static SDLGLSTR_STYLE BasicStyle = {

    SDLGLSTR_FONT8,
    {  64, 157, 201 },  /* Color of buttons mid            */
    {  64, 109, 201 },  /* Color of buttons topside        */
    {  45,  48, 176 },  /* Color of buttons bottomside     */
    { 255, 255, 255 },	/* Color of non highlighted text   */
    { 100, 100, 100 },  /* Color of non highlighted hotkey */
    { 170,   0,   0 },	/* Color of highlighted text       */
    { 170,   0, 100 },	/* Color of highlighted hotkey     */
    {   0,   0,   0 },	/* Color for label text in dialog  */
    {  40,  32, 172 },	/* Color for scrollbox background  */
    { 255, 255, 255 },	/* Color for labels in map	       */
    {   0,   0,   0 },  /* Color for the shadow for mapl.  */
    { 255, 255, 255 },  /* Background for edit fields      */
    {   0,   0,   0 }   /* Color of edit text              */

};

static SDLGLSTR_STYLE ActStyle = {

    SDLGLSTR_FONT8,
    {  64, 157, 201 },  /* Color of buttons mid            */
    {  64, 109, 201 },  /* Color of buttons topside        */
    {  45,  48, 176 },  /* Color of buttons bottomside     */
    { 255, 255, 255 },	/* Color of non highlighted text   */
    { 100, 100, 100 },  /* Color of non highlighted hotkey */
    { 170,   0,   0 },	/* Color of highlighted text       */
    { 170,   0, 100 },	/* Color of highlighted hotkey     */
    {   0,   0,   0 },	/* Color for label text in dialog  */
    {  40,  32, 172 },	/* Color for scrollbox background  */
    { 255, 255, 255 },	/* Color for labels in map	       */
    {   0,   0,   0 },  /* Color for the shadow for mapl.  */
    { 255, 255, 255 },  /* Background for edit fields      */
    {   0,   0,   0 }   /* Color of edit text              */

};

/*******************************************************************************
* CODE								                                           *
*******************************************************************************/

/*
 * Name:
 *     sdlglstrIString
 * Description:
 *     Prints a string, using actual font set
 * Input:
 *     pos *:     Pointer on SDLGL_RECT, holding drawing position
 *     text:      Text to print.
 *     textcolor: Pointer on vector with color description ( RGBA )
 *     hicolor:   Pointer on vector with color description for highlighting
 */
static void sdlglstrIString(SDLGL_RECT *pos, char *text, unsigned char *textcolor,
						unsigned char *hicolor)
{

    static int secondtime = 0;
    FONT *fptr;
    GLubyte *ptext;	/* Pointer on thext to display 	*/
    int  cx,
    	 cy;		/* Actual char position 	*/
    GLubyte c;
    int sx, sy;         /*


    /* first test for NULL-Pointer */
    if (text == 0) {

        return;

    }

    if (! secondtime) {     /* Set pixelstore only once */

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        secondtime++;
        
    }
    
    glColor3ubv(textcolor);

    fptr = &_wFonts[ActFont];

    /* Because OpenGL draws the bitmaps from bottom to top, we have to  */
    /* add the heigth of the font to the position			*/
    sx = pos -> x;
    sy = pos -> y + fptr -> fonth;

    cx = sx;
    cy = sy;				/* Save for Scrolling 		*/

    glRasterPos2i(cx, cy);		/* Set Start-Position for draw	*/

    /****** Put Text itself ***********************************/
    ptext = (GLubyte *)text;


    while (*ptext != 0) {     /* Get char */

        c = (char)(*ptext & 0x7F);  /* Max. 127 */
        switch (c) {
            case '\n':
            case '\r':		/* Make CR */
                cy += fptr -> fonth;	/* Next line downwards */
                cx = sx;		/* Start of next line  */
                glRasterPos2i(cx, cy);
                break;


            case 6:
            	/* Set Color */               
				glColor3ubv(fontcolors[*ptext]);
                glRasterPos2i(cx, cy);
                ptext++;
                break;

            case '{':

                /* Set Color */
					 glColor3ubv(hicolor);
                glRasterPos2i(cx, cy);
                break;

            case '~':
                ptext++;				/* Don't draw 	    */
					 glColor3ubv(hicolor);
                glRasterPos2i(cx, cy);			/* Force set color  */

                /* Draw the followin char in highlighted color 	*/
                c = (char)(*ptext & 0x7F); 		/* Max. 127 	*/

                glBitmap(fptr -> cwidth, fptr -> fonth,
	                 0, 0,
                         fptr -> fontw, 0,
                         &fptr -> font[c * fptr -> fonth]);

                /* Set color back to normal */
                glColor3ubv(textcolor);
                glRasterPos2i(cx + fptr -> fontw, cy); /* Force set color  */
                break;

            case '}':
                ptext++;			/* Don't draw 	     */
                glColor3ubv(textcolor);		/* Switch back color */
                glRasterPos2i(cx, cy);		/* Force set color   */
                break;

            case '\t':

            	/* Ignore tab */
                cx += fptr -> fontw;
                break;

            default:

		glBitmap(fptr -> cwidth, fptr -> fonth,
                	 0, 0,
                	 fptr -> fontw, 0,
                	 &fptr -> font[c * fptr -> fonth]);


        }

        ptext++;		/* next char */

        cx += fptr -> fontw;	/* hold charpos for Carriage-Return */

    }

}

/*
 * Name:
 *     sdlglstrChar
 * Description:
 *     Output of one single char.
 * Input:
 *     pos *:     Pointer on SDLGL_RECT, holding drawing position
 *     c:         The char to put out
 *     textcolor: Color of char
 *     fontno:    Number of font to use
 */
static void sdlglstrChar(SDLGL_RECT *pos, char c)
{

    FONT * fptr;


    fptr = &_wFonts[ActFont];

    /* Because OpenGL draws the bitmaps from bottom to top, we have to  */
    /* add the heigth of the font to the position			*/
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glColor3ubv(fontcolors[ActColor]);

    glRasterPos2i(pos -> x, pos -> y + fptr -> fonth);

    glBitmap(fptr -> cwidth, fptr -> fonth,
             0, 0,
             fptr -> fontw, 0,
             &fptr -> font[c * fptr -> fonth]);

}


/*
 * Name:
 *     sdlglstrIDrawShadowedRect
 * Description:
 *     Draws a simple filled 2D-Rectangle, with a "Shadow"
 * Input:
 *     rect:    Pointer on a text rectangle.
 *     style:   Pointer on style to use for drawing
 *     flags:   Draw-Flags
 */
static void sdlglstrIDrawShadowedRect(SDLGL_RECT *rect, SDLGLSTR_STYLE *style, int flags)
{

    int x, y, x2, y2;
    unsigned char *topcolor,
                  *bottomcolor;


    x = rect -> x;
    y = rect -> y;
    x2 = x + rect -> w - 1;
    y2 = y + rect -> h - 1;


    if (flags & SDLGLSTR_FEMPTYBUTTON) {
    
        glColor3ubv(style -> edit_bk);
        
    }
    else {
    
        glColor3ubv(style -> buttonmid);
        
    }
    
    glBegin(GL_QUADS);	/* First the Background */            
        glVertex2i(x, y2);
        glVertex2i(x2, y2);
        glVertex2i(x2, y);
        glVertex2i(x, y);
    glEnd();

    if (flags & SDLGLSTR_FINVERTED) {

        topcolor    = style -> buttonbottom;
        bottomcolor = style -> buttontop;

    }
    else {

        topcolor    = style -> buttontop;
        bottomcolor = style -> buttonbottom;
        
    }

    glBegin(GL_LINES);
        /* Top Edge Line */
        glColor3ubv(topcolor);
        glVertex2i(x, y);
        glVertex2i(x2, y);
        glVertex2i(x2, y);
        glVertex2i(x2, y2);

    	/* Bottom Edge Line */
    	glColor3ubv(bottomcolor);
        glVertex2i(x2, y2);
        glVertex2i(x, y2);
        glVertex2i(x, y2);
        glVertex2i(x, y);

        x  += 1;
    	y  += 1;
    	x2 -= 1;
        y2 -= 1;

        /* Top Edge Line */
    	glColor3ubv(topcolor);
        glVertex2i(x, y);
        glVertex2i(x2, y);
        glVertex2i(x2, y);
        glVertex2i(x2, y2);

        /* Bottom Edge Line */
        glColor3ubv(bottomcolor); 
        glVertex2i(x2, y2);
        glVertex2i(x, y2);
        glVertex2i(x, y2);
        glVertex2i(x, y);
    glEnd();

}

/*
 * Name:
 *     sdlglstrPrintValue
 * Description:
 *     Prints a value given in 'value *' in format 'which'
 * Input:
 *     rect *:    Position where to print
 *     data *:    Pointer on value to print
 *     which:     Type of value to print 'SDLGL_VAL_...'
 *     color *:   Color to use for printing string   
 */ 
static void sdlglstrPrintValue(SDLGL_RECT *rect, void *data, int which, unsigned char *color)
{
    
    char val_str[100];
    char *pdata, **ppchar;
    int value;


    pdata = (char *)data;
    if (! pdata)
    {
        /* play it save */
        return;
    }

    val_str[0] = 0;
    
    switch(which)
    {
        case SDLGL_VAL_PSTR:
            // Pointer on a pchar
            ppchar = (char *)((void *)pdata);            
            if (! *ppchar)
            {
                /* play it save */
                return;
            }
            pdata = *ppchar;
        case SDLGL_VAL_NONE:
        case SDLGL_VAL_STRING:
            /* Print given string */
            sdlglstrIString(rect, pdata, color, color);
            return;               

        case SDLGL_VAL_CHAR:
            value  = *pdata;
            sprintf(val_str, "%d", value);
            break;

        case SDLGL_VAL_SHORT:
            value = *(short int *)pdata;       /* Extend to bigger value */
            sprintf(val_str, "%d", value);
            break;

        case SDLGL_VAL_INT:
            value = *(int *)pdata;
            sprintf(val_str, "%d", value);
            break;

        case SDLGL_VAL_FLOAT:
            sprintf(val_str, "%.2f", *(float *)pdata);
            break;
            
        case SDLGL_VAL_ONECHAR:
            val_str[0] = *pdata;
            val_str[1] = 0;
            break;
            
        default:
            return;

    }

    sdlglstrIString(rect, val_str, color, color);
}

/*
 * Name:
 *     sdlglstrDrawEditField
 * Description:
 *     Draws an "EditField "button" with the given text. The text starts
 *     half font size form left and half font size from top from he given
 *     rectangle.
 *     Draws a "shadowed" background with black chars on white background
 *     in it. If "curpos" is >= 0, the an underscore is drawn at the position
 *     of the cursor "curpos" chars from left.
 *     Always FONT8 is used.
 *     curpos:  Where to draw the cursor (char position !) 
 * Input:
 *     field *: Description of edit-field to print   
 */
static void sdlglstrDrawEditField(SDLGL_FIELD *field)
{

    SDLGL_RECT sizerect;
    SDLGLSTR_STYLE *style = &ActStyle;


    /* First draw the background */
    sdlglstrIDrawShadowedRect(&field -> rect, style, SDLGLSTR_FINVERTED | SDLGLSTR_FEMPTYBUTTON);  

    memcpy(&sizerect, &field -> rect, sizeof(SDLGL_RECT));

    sizerect.x += SDLGLSTR_FIELDBORDER;
    sizerect.y += SDLGLSTR_FIELDBORDER;
    ActFont = style -> fontno;

    /* Draw the edited value -- Sub-Code: Kind of edited value in 'pdata' */
    sdlglstrPrintValue(&sizerect, field -> pdata, field -> sub_code, style -> edit_text);

    /* Now draw the cursor, if needed */
    if (field -> edit_cur >= 0 && (field -> fstate & SDLGL_FSTATE_HASFOCUS)) {

        sizerect.x += (_wFonts[style -> fontno].fontw * field -> edit_cur);
        sizerect.y += 1;
        sdlglstrIString(&sizerect, "_", style -> edit_text, style -> edit_text);

    }

}

/*
 * Name:
 *     sdlglstrDrawSpecial
 * Description:
 *     Draws standard dialog fields in font 8, using special bitmaps from
 *     font. The text is always rectangle.
 *     Settings as font and color  are taken from 'ActStyle' 
 * Input:
 *     rect:        Rectangle holding the size of the special
 *     text:        Text to print into the button. If 0, no text is drawn.
 *     which:       Which special to draw
 *     info:        Additional flags for drawing info
 */
static void sdlglstrDrawSpecial(SDLGL_RECT *rect, char *text, int which, int info)
{

    static char buttonchars[] = { 15, 14, 13,	   /* Push button  */
    				              12, 11, 10,	   /* Radio button */
				                   9,  8,  7, 6 }; /* Arrows	   */


    SDLGL_RECT sizerect;
    char *pbc;
    char percenttext[30];
    SDLGLSTR_STYLE *style = &ActStyle;
    unsigned char *textcolor, *hotkeycolor;
    int save_color;


    if (info & SDLGLSTR_FHIGHLIGHT) {

        textcolor   = style -> texthi;
        hotkeycolor = style -> hotkeyhi;

    }
    else {

        textcolor   = style -> textlo;
        hotkeycolor = style -> hotkeylo;

    }

    memcpy(&sizerect ,rect, sizeof(SDLGL_RECT));  /* Hold copy for later use */
    ActFont = SDLGLSTR_FONT8;
    
    switch(which) {         
               
        case SDLGL_TYPE_CHECKBOX:
        case SDLGL_TYPE_RB:        

            pbc = buttonchars;

            if (which == SDLGL_TYPE_RB) {

            	pbc += 3;	/* Point on radio button bitmaps */

            }

            /* 1. Fill the background */
            ActColor = SDLGL_COL_WHITE;
            sdlglstrChar(rect, *pbc);

            /* 2: Draw the state */
            pbc++;		/* Point on state bitmaps	*/

            if (info) {

                pbc++;		/* Point on number of bitmap for state set */

            }

            save_color = ActColor;
            ActColor = SDLGL_COL_BLACK;
            sdlglstrChar(rect, *pbc);
            ActColor = save_color;
            break;

        case SDLGL_TYPE_RBGROUP:

            /* 1. Draw the rectangle 	     */
            sdlglstrDrawRect(rect, fontcolors[SDLGL_COL_BLACK], 0);

            /* Calculate the position of the type */
            sdlglstrGetStringSize(text, &sizerect);
            sizerect.x += rect -> x + ((rect -> w - sizerect.w) / 2);
            sizerect.y += rect -> y - 4;

            /* 3: Draw the background for the text */
            sdlglstrStringBackground(&sizerect, strlen(text), SDLGLSTR_MENUCOL1);

            /* 4: Draw the title */
            ActFont = style -> fontno;
            sdlglstrIString(&sizerect, text,textcolor, hotkeycolor);
            break;      
        
        case SDLGL_TYPE_SLI_BK:			/* Background of a scroll box */
            sdlglstrDrawRect(rect, style -> scrollbk, 1);
            break;

        case SDLGL_TYPE_SLI_BOX:
            ActFont = style -> fontno;
            /* 1: Draw the title, if any is given */
            if (text) {
                sdlglstrGetStringSize(text, &sizerect);
                sizerect.x += rect -> x + ((rect -> w - sizerect.w) / 2);
                sizerect.y += rect -> y + 4;
                sdlglstrIString(&sizerect, text, textcolor, hotkeycolor);
            }

            /* 2: Draw the box */
    	    sdlglstrIDrawShadowedRect(rect, style, SDLGLSTR_FINVERTED);
            break;

        case SDLGL_TYPE_PROGBAR:

            /* 1: Draw the background*/
            sdlglstrIDrawShadowedRect(rect, style, SDLGLSTR_FINVERTED);

            /* 2: Draw the progress bar */
            sizerect.x += 2;
            sizerect.y += 2;
            sizerect.w -= 4;
            sizerect.h -= 4;
            sdlglstrDrawRect(&sizerect, fontcolors[SDLGL_COL_BLUE], 1);

            /* 3: Draw the progress in percent as text, "info" holds the number */
            sprintf(percenttext, "%d%%", info);
            sdlglstrDrawButton(rect, percenttext,
                              SDLGLSTR_FHCENTER | SDLGLSTR_FVCENTER | SDLGLSTR_FHIDEBUTTON);
            break;

    } /* switch(which) */

}

/* ========================================================================= */
/* ============================= THE PUBLIC ROUTINES ======================= */
/* ========================================================================= */

/*
 * Name:
 *     sdlglstrDrawRect
 * Description:
 *     Draws a simple filled/non filles 2D-Rectangle with the given color
 *     number
 * Input:
 *     rect:   Pointer on a text rectangle.
 *     color:  Pointer on a vector with RGB-color
 *     filled: Draw it filled 1 / 0
 */
void sdlglstrDrawRect(SDLGL_RECT *rect, unsigned char *color, int filled)
{

    int x, y, x2, y2;

    x = rect -> x;
    y = rect -> y;
    x2 = x + rect -> w;    /* Remove subtraction of 1: 27.06.2008 / PAM */
    y2 = y + rect -> h;


    glColor3ubv(color);
    glBegin(filled ? GL_QUADS : GL_LINE_LOOP);
        glVertex2i(x, y2);
        glVertex2i(x2, y2);
        glVertex2i(x2, y);
    	glVertex2i(x, y);
    glEnd();

}

/*
 * Name:
 *     sdlglstrDrawRectColNo
 * Description:
 *     Draws a rectangle with the given font color number
 *
 */
void sdlglstrDrawRectColNo(SDLGL_RECT *rect, int colorno, int filled)
{

    sdlglstrDrawRect(rect, fontcolors[colorno], filled);
    
}

/*
 * Name:
 *     sdlglstrGetCharPos
 * Description:
 *     Returns the position of a char (in chars) in the given string,
 *     calculated from the given pixel position.
 *     The position on the first char behind the string is returned if
 *     the position is right of the string.
 * Input:
 *     text:       String to calculate the char position for.
 *     pixelx:     The position in pixels from start of string.
 *     fontno:     Number of font used
 */
int  sdlglstrGetCharPos(char *text, int pixelx)
{

    SDLGL_RECT rect;


    sdlglstrGetStringSize(text, &rect);

    if (pixelx >= rect.w) {	    /* Mouse is clicked right of the string.     */

        return strlen(text);	/* Return index of first char behind string. */

    }


    /* Now calculate the position of the char */
    return (rect.w / _wFonts[ActFont].fontw);

}



/*
 * Name:
 *     sdlglstrGetStringSize
 * Description:
 *     Fills the given rectangle with the measure the text needs in pixels.
 *     Uses font actually set.
 * Input:
 *     string:     String to return the rectangle for
 *     rect:       Pointer on rect, whichs width and height are filled with
 *                 the strings size. X and y left unchanged
 */
void sdlglstrGetStringSize(char *string, SDLGL_RECT *rect)
{

    int len, lines;

    len   = 0;
    lines = 1;		/* Minimum one line */

    while (*string != 0) {

        if ((*string >= ' ') && (*string != '~')
             && (*string != '{') && (*string != '}') && (*string != '\n' )) {

            len++;

        }

        if (*string == '\n') {

	    lines++;

        }

        string++;
    }

    rect -> w = (len * _wFonts[ActFont].fontw);
    rect -> h = (lines * _wFonts[ActFont].fonth);

}

/*
 * Name:
 *     sdlglstrGetButtonSize
 * Description:
 *     Returns a pointer on a rectangle holding the width and the height
 *     needed to display the given string.
 *     Uses font actually set.
 *     Adds a half of font size on all sides of given rectangle
 * Input:
 *     string:     String to return the rectangle for
 *     rect:       Pointer on rect, whichs width and height are filled with
 *                 the strings size. X and y left unchanged
 */
void sdlglstrGetButtonSize(char *string, SDLGL_RECT *rect)
{

    sdlglstrGetStringSize(string, rect);

    rect -> w += _wFonts[ActFont].fontw;
    rect -> h += _wFonts[ActFont].fonth;

}


/*
 * Name:
 *     sdlglstrString
 * Description:
 *     Prints a string at given position, using actually set font and color.
 * Input:
 *     pos *:     Pointer on SDLGL_RECT, holding drawing position
 *     text:      Text to print.
 */
void sdlglstrString(SDLGL_RECT *pos, char *text)
{

     sdlglstrIString(pos, text, fontcolors[ActColor], fontcolors[ActColor]);

}

/*
 * Name:
 *     sdlglstrStringF
 * Description:
 *     Prints a string at given position, using actually set font and color.
 *     Gets formatted text 
 * Input:
 *     pos *:     Pointer on SDLGL_RECT, holding drawing position
 *     text:      Text to print.
 */
void sdlglstrStringF(SDLGL_RECT *pos, char *text, ...)
{

    char buffer[512];
    va_list ap;


    va_start(ap, text);

    vsprintf(buffer, text, ap);

    sdlglstrIString(pos, buffer, fontcolors[ActColor], fontcolors[ActColor]);
    va_end(ap);

}

/*
 * Name:
 *     sdlglstrStringToRect
 * Description:
 *     Prints the string into the given rectangle. In case the string
 *     doesn't fit into the width of the rectangle, it's wrapped onto the
 *     next line(s).
 *     Words are assumed to end with one of the the following characters:
 *		space   (' ')
 *	        tab     ('\t')
 *		CR      ('\r')
 *		newline ('\n')
 *	 	and	('-')
 *     The standard white color is used.
 * Input:
 *     rect *:   Pointer on rect to draw the string within
 *     string:   String to print
 */
void sdlglstrStringToRect(SDLGL_RECT *rect, char *string)
{

    SDLGLSTR_STYLE *style;
    SDLGL_RECT sizerect;
    SDLGL_RECT argrect;
    char *pnextbreak, *pprevbreak;
    char prevsavechar, nextsavechar = 0;


    if (! string) {

        return;	/* Just for the case ... */

    }


    style = &ActStyle;
    ActFont = style -> fontno;


    /* First try if the whole string fits into the rectangles width */
    sdlglstrGetStringSize(string, &sizerect);

    if (sizerect.w <= rect -> w) {

        /* Print the string */
        sdlglstrIString(rect, string, style -> textlo, style -> hotkeylo);

    }
    else {

        /* Drawing position if more then one line   */
        memcpy(&argrect, rect, sizeof(SDLGL_RECT));

        pprevbreak = string;	/* Run pointer into the string 		    */

        /* We have to scan word by word */

        do {

            pnextbreak = strpbrk(pprevbreak, " \t\r\n-");

            if (pnextbreak) {

                /* The end of string is not reached yet */
                pnextbreak++;  	      	    /* Include the breaking character */
                nextsavechar = *pnextbreak; /* Save the break char.	       */
                *pnextbreak   = 0;	    /* Create a string out of it.     */

            }

            /* Now get the size, of the string */
            sdlglstrGetStringSize(string, &sizerect);

            if (sizerect.w > rect -> w) {

                /* One word to much, use previous break. */
                prevsavechar = *pprevbreak;
                *pprevbreak  = 0;	/* Create the string */

                /* Print the string... 	 		 */
                sdlglstrIString(&argrect, string, style -> textlo, style -> hotkeylo);

                /* Move string to new start */
                string  = pprevbreak;
                *string = prevsavechar;   	/* Get back the prev char */

                /* Move to next line... */
                argrect.y += sizerect.h;

            }
            else {

            	/* Check for end of string */

                if (! pnextbreak) {

                    sdlglstrIString(&argrect, string, style -> textlo, style -> hotkeylo);
                    return;

                }

                pprevbreak  = pnextbreak;    /* Move one word further.   */

            }

            if (pnextbreak) {

            	*pnextbreak = nextsavechar;  /* bring the next char back */

            }

    	}
        while (*string != 0);

    }

}

/*
 * Name:
 *     sdlglstrStringBackground
 * Description:
 *     Draws a string with filled chars in the given color, with the given
 *     font number.
 * Input:
 *     pos *:     Pointer on SDLGL_RECT, holding drawing position
 *     len:    Length of string
 *     color:  Number of text color to use for drawing
 *     fontno: Number of font to write the background for
 */
void sdlglstrStringBackground(SDLGL_RECT *pos, int len, int color)
{

    static char strepbuf[121];
    int oldcolor;


    if (len > 120) len = 120;		/* Might not be longer	 */

    memset(strepbuf, 127, len);         /* Fill with filled char */
    strepbuf[len] = 0;			/* Terminate it		 */

    oldcolor = ActColor;
    ActColor = color;
    sdlglstrString(pos, strepbuf);
    ActColor = oldcolor;

}

/*
 * Name:
 *     sdlglstrStringPos
 * Description:
 *     Prints a string at the given position using the flags.
 * Input:
 *     rect *: Pointer on SDLGL_RECT, to print text in
 *     text*:  Text to print out positioned
 *     flags:  Position flags  
 */
void sdlglstrStringPos(SDLGL_RECT *rect, char *text, int flags)
{

    SDLGL_RECT sizerect;


    memcpy(&sizerect, rect, sizeof(SDLGL_RECT));

    if (flags & (SDLGLSTR_FCENTERED | SDLGLSTR_FRIGHTADJUST)) {

        /* Get the size of the string in pixels */
        sdlglstrGetStringSize(text, &sizerect);

        if (flags & SDLGLSTR_FRIGHTADJUST) {

            sizerect.x = rect -> x + rect -> w - sizerect.w;

        }
        else {

            if (flags & SDLGLSTR_FHCENTER) {

                 /* Center the text horizontally. Text may overlap rect  */
                 sizerect.x += ((rect -> w - sizerect.w) / 2);

            }

            if (flags & SDLGLSTR_FVCENTER) {

                /* Center the text vertically. Text may overlap rect */
                sizerect.y += ((rect -> h - sizerect.h) / 2);

            }

        }

    } /* if (flags & SDLGLSTR_FCENTERED)  */

    sdlglstrString(&sizerect, text);

}

/*
 * Name:
 *     sdlglstrDrawButton
 * Description:
 *     Draws a "button" with the given text. The text starts half font
 *     size form left and half font size from top from he given rectangle.
 *     Draws a standard background.
 * Input:
 *     text:    Text to print into the button. If 0, no text is drawn.
 *     rect:    Rectangle holding the size of the button
 *     flags:   Additional drawing flags
 */
void sdlglstrDrawButton(SDLGL_RECT *rect, char *text, int flags)
{

    SDLGLSTR_STYLE *style = &ActStyle;
    SDLGL_RECT sizerect;      /* int width, height; -> Size of string in pixels */
    unsigned char *textcolor,
         	      *hotkeycolor;

    /* First draw the background */
    if (! (flags & SDLGLSTR_FHIDEBUTTON)) {

    	if ((flags & SDLGLSTR_FTEXTURED) && (style -> texbklistno)){

            /*
            sdlgltexDisplayIconTiled(style -> texbklistno,
            			     style -> texbkno,
                                     rect);
            */

            flags |= SDLGLSTR_FEMPTYBUTTON;	/* Background is textured */

        }

        sdlglstrIDrawShadowedRect(rect, style, flags);

    }

    /* Now draw the text */
    if (text) {

        ActFont = style -> fontno;
        memcpy(&sizerect, rect, sizeof(SDLGL_RECT));
        
        /* First get choose the text colors */
        if (flags & SDLGLSTR_FHIGHLIGHTED) {

            textcolor   = style -> texthi;
            hotkeycolor = style -> hotkeyhi;

        }
        else {

            textcolor   = style -> textlo;
            hotkeycolor = style -> hotkeylo;

        }

        if (flags & SDLGLSTR_FCENTERED) {

            /* Get the size of the string in pixels */
            sdlglstrGetStringSize(text, &sizerect);

    	    if (flags & SDLGLSTR_FHCENTER) {

                 /* Center the text horizontally. Text may overlap rect  */
                 sizerect.x += ((rect -> w - sizerect.w) / 2);

            }

            if (flags & SDLGLSTR_FVCENTER) {

            	/* Center the text vertically. Text may overlap rect */
                sizerect.y += ((rect -> h - sizerect.h) / 2);

            }

            if (flags & SDLGLSTR_FRIGHTADJUST) {

               sizerect.x += (rect -> w - sizerect.w);

            }

        }
        else {

            sizerect.x += SDLGLSTR_FIELDBORDER;
            sizerect.y += SDLGLSTR_FIELDBORDER;

        }

        sdlglstrIString(&sizerect, text, textcolor, hotkeycolor);

    }

}

/*
 * Name:
 *     sdlglstrDrawField
 * Description:
 *     Draw standard fields for SDLGL. Uses the colors defined in 'SDLGLSTR_STYLE'   
 * Input:
 *     f * : Pointer on field to print
 * Output:
 *     Field was drawn yes / no
 */
int sdlglstrDrawField(SDLGL_FIELD *f)
{

    static char buttonchars[] = { 15, 14, 13,	   /* Push button  */
    				              12, 11, 10,	   /* Radio button */
				                   9,  8,  7, 6 }; /* Arrows	   */

    unsigned char *textcolor;
    SDLGL_RECT sizerect;

    
    ActFont = ActStyle.fontno;

    switch(f -> sdlgl_type) {
    
        case SDLGL_TYPE_MENU:
            sizerect.x = f -> rect.x;
            sizerect.y = f -> rect.y;
            if (f -> fstate & SDLGL_FSTATE_MOUSEOVER) {
                textcolor = ActStyle.texthi;
            }
            else {
                textcolor = ActStyle.textlo;
            }
            if (f -> workval & 0x80) {
                /* Flag, if this is a checked menu point */
                if (f -> workval & f -> sub_code) {
                    /* Field is checked */
                    sdlglstrChar(&sizerect, buttonchars[2]);
                    sizerect.x += SDLGLSTR_SPECIALDIST;
                }

            }
            sdlglstrIString(&sizerect, f -> pdata, textcolor, textcolor);
            break;

        case SDLGL_TYPE_STD:
        case SDLGL_TYPE_BUTTON:
            sdlglstrDrawButton(&f -> rect, f -> pdata, f -> fstate);
            break;
            
        case SDLGL_TYPE_EDIT:
            sdlglstrDrawEditField(f);
            break; 
                
        case SDLGL_TYPE_VALUE:
            /* Uses subcode, because maincode generates mouse-event */
            sdlglstrPrintValue(&f -> rect, f -> pdata, f -> sub_code, ActStyle.textlo);
            break;

        case SDLGL_TYPE_LABEL:
            sdlglstrIString(&f -> rect, f -> pdata, ActStyle.textlo, ActStyle.textlo);
            break;
            
        case SDLGL_TYPE_SLI_AU:   /* Arrow up                     */
        case SDLGL_TYPE_SLI_AD:   /* Arrow Down                   */
        case SDLGL_TYPE_SLI_AL:   /* Arrow left                   */
        case SDLGL_TYPE_SLI_AR:   /* Arrow right                  */
            /* 1: Draw the button */
            sdlglstrDrawButton(&f -> rect, 0, 0);
            sizerect.x = f -> rect.x + (f -> rect.w - 8) / 2;
            sizerect.y = f -> rect.y + (f -> rect.h - 8) / 2;
            /* 2. Draw the arrow in chosen direction */
            ActColor = SDLGL_COL_BLACK;
            sdlglstrChar(&sizerect, buttonchars[6 + (f -> sdlgl_type - SDLGL_TYPE_SLI_AU)]);
            break;
         
        
        case SDLGL_TYPE_CHECKBOX:
        case SDLGL_TYPE_RB: 
            sdlglstrDrawSpecial(&f -> rect, f -> pdata, f -> sdlgl_type, *f -> pdata & f -> sub_code);
            break; 
            
        default:
            /* Unknown type, to be drawn by caller  */
            return 0;
            
    }
    
    return 1;

}

/****************************** SETFONT ************************	*/

/*
 * Name:
 *     sdlglstrSetFont
 * Function:
 *     Sets the font to use for output
 * Input:
 *     fontno:   Number of font to set
 * Output:
 *     Numebr of previously used font
 */
int  sdlglstrSetFont(int fontno)
{

    int oldfont;


    oldfont = ActFont;
    if (fontno < 0 || fontno > SDLGLSTR_MAXFONT) {

        fontno = 0;

    }

    ActFont = fontno;
    
    return oldfont;

}

/*
 * Name:
 *     sdlglstrAddFont
 * Function:
 *     Sets a font for "TextOut"
 * Input:
 *     font:   Font-Bitmap(s)
 *     fontw:  Width of font
 *     fonth:  Height of font
 *     fontno: Number to use for font
 */
void sdlglstrAddFont(unsigned char * font, int fontw, int fonth, int fontno)
{

    FONT *fptr;


    if (fontno < 0 ||fontno > SDLGLSTR_MAXFONT) {
    	fontno = 2;
    }

    fptr = &_wFonts[fontno];

    fptr -> font = font;
    fptr -> fontw = fontw;
    fptr -> fonth = fonth;

}


/*
 * Name:
 *     sdlglstrSetDrawStyle
 * Function:
 *     Overwrites the 'ActStyle' with the style goven in argument
 * Input:
 *     style:   Pointer on a struct with style data.
 */
void sdlglstrSetDrawStyle(SDLGLSTR_STYLE *style)
{
    if (style) {
        memcpy(&ActStyle, style, sizeof(SDLGLSTR_STYLE));
    }
    else {
        /* Reset it to default */
        memcpy(&ActStyle, &BasicStyle, sizeof(SDLGLSTR_STYLE));
    }
}

