/*
 *  SDLGLTEX.H -
 *	Declarations and functions for loading and rendering of pictures
 *
 *  Copyright (C) 2002  Paul Mueller <pmtech@swissonline.ch>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _SDLGLTEX_H_
#define _SDLGLTEX_H_

/*******************************************************************************
* INCLUDES							      	                                   *
*******************************************************************************/

#include "sdlgl.h"      /* OpenGL and SDL Stuff */

/*******************************************************************************
* DEFINES							      	                                   *
*******************************************************************************/

#define SDLGLTEX_MAXICONLIST    ((char)64)  /* Maximum of icon lists        */

/* Flags for icons */
#define SDLGLTEX_ICONBORDER 	 0x01	/* Icon has a border (extract only) */
#define SDLGLTEX_ICONALPHA       0x02	/* Icon has an alpha channel	    */

/* Types of ICON-Textures */
#define SDLGLTEX_ICON_TEXLIST    0x01   /* List of textures extracted from picture */
#define SDLGLTEX_ICON_SUBTEX     0x02   /* Single texture, holding subtextures of same size  */
#define SDLGLTEX_CON_SUBTEXT_VAR 0x03   /* Single texture, holding subtextures
                                           of variable size */
                                           
/*******************************************************************************
* TYPEDEFS							      	                                   *
*******************************************************************************/

typedef struct {

    char listno;		/* An integer number between 1..SDLGLTEX_MAXICONLIST  */
    char number;		/* Total number of icons in this list                 */
    SDLGL_RECT rect;    /* Position in bitmap and size of first icon to       */
                        /* extract from bitmap, excluding possible border     */
                        /* xstart, ystart, width, height                      */
    int  xcount;		/* Number of icons side by side on the same line      */
    char flags;		    /* Additional info for creation of icons	          */
    unsigned int key;   /* Alpha key color, if SDLGLTEX_ICONALPHA             */

} SDLGLTEX_ICONCREATEINFO; /* Holds all the arguments needed for extracting   */
                           /* icons from a bitmap - picture		              */

/******************************************************************************
* CODE								      	      *
******************************************************************************/

/* Loaders for single textures */
GLuint sdlgltexLoadSingle(const char *filename);
GLuint sdlgltexLoadSingleA(const char *filename, unsigned int key);

/* Now the "short" function */
int  sdlgltexLoadIcons(const char *filename, unsigned int key, SDLGLTEX_ICONCREATEINFO *icd);
void sdlgltexFreeIcons(void);

/* Now the display functions */
void sdlgltexDisplayIcon(int type, int number, SDLGL_RECT *rect);
void sdlgltexDisplayIconTiled(int type, int number, SDLGL_RECT *rect);
void sdlgltexDisplayIconList(SDLGL_FIELD *fields, char stop_type);

#endif /* _SDLGLTEX_H_	*/
