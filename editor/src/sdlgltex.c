/*******************************************************************************
*  SDLGLTEX.C                                                                  *
*      - Declarations and functions for loading and rendering of pictures      *
*                                                                              *
*  SDLGL - Library                                                             *
*  Copyright (C) 2002  Paul Mueller <pmtech@swissonline.ch>                    *
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
* HEADER FILES							      	                               *
*******************************************************************************/

#include <SDL.h>
#ifdef SDLGLTEX_USER_SDLIMAGE
        #include <SDL_image.h>
#endif

#include <memory.h>

/* ----- Own headers ---- */
#include "sdlgl.h"
#include "sdlgltex.h"		/* The definitions for this module */

/*******************************************************************************
* DEFINES							      	                                   *
*******************************************************************************/

#define SDLGLTEX_MAXICON 1024
#define SDLGLTEX_TEXX1	    2
#define SDLGLTEX_TEXY1      1
#define SDLGLTEX_TEXX2      4
#define SDLGLTEX_TEXY2      3

/*******************************************************************************
* TYPEDEFS							      	                                   *
*******************************************************************************/

typedef struct {

    char number;			/* Number of icons in this list		         */
    int  border;			/* Border in pixels 0 or 1		             */
    int  flags;			    /* Has an alpha channel, yes/no		         */
    				        /* Is transparent yes/no		             */
    SDLGL_RECT rect;        /* Size of texture(s) in pixels              */
                            /* Used for display of tiled textures        */
    GLuint  *texid;		    /* Pointer on a list of OpenGL texture names */
    GLuint  singleid;       /* Used, if single texture. In this case     */
                            /* texid = &singleid                         */
} SDLGLTEX_ICONLIST;        /* List of icons for rendering with	         */
                		    /* the same size/teture coordinates	         */


/*******************************************************************************
* DATA								      	                                   *
*******************************************************************************/

		/* Standard texture coordinates */
static GLfloat StdTexCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0 };

static SDLGLTEX_ICONLIST IconList[SDLGLTEX_MAXICONLIST + 2];
static GLuint   TextureList[SDLGLTEX_MAXICON + 2];
static GLuint   *TexListPointer = &TextureList[0];  /* Start of buffer	    */

/*******************************************************************************
* CODE								      	                                   *
*******************************************************************************/

/*
 * Name:
 *     power_of_two
 * Description:
 *     Quick utility function for texture creation
 * Input:
 *      value:
 * Output:
 *      Value adjusted to next power of two.
 */
static int power_of_two(int input)
{
	int value = 1;

	while ( value < input ) {
		value <<= 1;
	}
	return value;
}

/*
 * Name:
 *    sdlgltexCreateIconList
 * Description:
 *    Extracts a list of icons from the actually loaded surface. It is
 *    assumed, that all icons are of the same size.
 * Input:
 *     src *:   Pointer on SDL_Surface to extract the icons from
 *     icd *: Pointer on a SDLGLTEX_ICONCREATEINFO
 *            Field-description:
 *              type:   Type of icon to create e. g. SDLGLTEX_CARGO
 *              number: Number of icons to create.
 *              rect:   Position in bitmap and size of icon to extract
 *                      from bitmap, including border
 *                      xstart, ystart, width, height
 *              xcount: Number of icons / row
 *              flags:  SDLGLTEX_ICON...
 * Output:
 *     0: Loading of icons failed
 */
static int sdlgltexCreateIconList(SDL_Surface *src, SDLGLTEX_ICONCREATEINFO *icd)
{

    SDLGLTEX_ICONLIST  *il;
    SDL_Surface *dst;
    SDL_Rect blitrect;
    int i, column, border;
    int x, y;
    Uint32 *p;
    Uint32 rmask, gmask, bmask, amask;


    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;

    glGenTextures(icd -> number, TexListPointer);  /* Get the textures */

    /* Create pointer on iconlist to make it more readable */
    il = &IconList[icd -> listno];

    /* Now fill the basic descriptor for the icon */
    il -> number = icd -> number;	/* Set the number of icons */
    il -> texid  = TexListPointer;	/* Set pointer on textures */
    
    border = (icd -> flags & SDLGLTEX_ICONBORDER) ? 1 : 0;
    
    /* Now copy the size of the texture to the icon descriptor  */
    /* Is start of first rectangle in source image, too         */
    il -> rect.x = icd -> rect.x + border;
    il -> rect.y = icd -> rect.y + border;
    il -> rect.w = icd -> rect.w;
    il -> rect.h = icd -> rect.h;

    dst = SDL_CreateRGBSurface(SDL_SWSURFACE,
                               il -> rect.w,
                               il -> rect.h,
                               32,
                               rmask, gmask, bmask, amask);

    if (dst == NULL) {
        return 0;
    }

    icd -> key |= 0xFF000000;   /* All pixels are set to opaque */


    column = 0;					/* Count icons / row 	    */
    blitrect.x = (short int)il -> rect.x;
    blitrect.y = (short int)il -> rect.y;
    blitrect.w = (unsigned short int)il -> rect.w;
    blitrect.h = (unsigned short int)il -> rect.h;
    for (i = 0; i < icd -> number; i++) {	/* Create the list of icons */

        /* Extract the icon from surface */
        SDL_BlitSurface(src, &blitrect, dst, NULL);
        /* Fix the alpha values, if needed  */
        if (icd -> flags & SDLGLTEX_ICONALPHA) {

            p = dst -> pixels;

            for (y = dst -> h - 1; y >= 0; y--) {
	            for (x = dst -> w - 1; x >= 0; x--) {
                    if (p[x + (y * dst -> w)] == icd -> key) {
                        p[x + (y * dst -> w)] &= 0x00FFFFFF;
                    }
                }
            }
        }

        /* Now hand it over to OpenGL */
        glBindTexture(GL_TEXTURE_2D, il -> texid[i] );

        /* Set texture parameters	*/
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

        glTexImage2D(GL_TEXTURE_2D,
       		         0,
                     GL_RGBA,		/* RGBA in any case	*/
                     il -> rect.w,
                     il -> rect.h,
		             0,			
                     GL_RGBA,
   		             GL_UNSIGNED_BYTE,
		             dst -> pixels);

        blitrect.x += (short int)(il -> rect.w + border);

        column++;
        if (column >= icd -> xcount) {

            column = 0;

            /* Move to the next line */
            blitrect.x =  (short int)(icd -> rect.x + border);
            blitrect.y += (short int)(il -> rect.h + border);

        }

    }

    TexListPointer += icd -> number;  		/* Move forward in list */

    il -> rect.x = 0;
    il -> rect.y = 0;

    SDL_FreeSurface(dst);

    return 1;

}

/*
 * Name:
 *     sdlgltexLoadSurfaceA
 * Description:
 *     Loads the file with the given name into a SDL-Software-Surface.
 *     Uses the key to set the color as alpha-channel
 * Input:
 *      filename: Name of file to load
 *      key:      Color to use as alpha value
 *      topow2:   Adjust the width and height of the loaded pic to a power of two
 * Output:
 *      Pointer on SDL-Surface, if successfull, else NULL
 */
static SDL_Surface *sdlgltexLoadSurfaceA(const char *filename, unsigned int key, int topow2)
{

    int tw, th;
    SDL_Surface	*tempSurface, *imageSurface;
    int x, y;
    Uint32 *p;

    
    /* Load the bitmap into an SDL_Surface */
    #ifdef SDLGLTEX_USER_SDLIMAGE
    imageSurface = IMG_Load(filename);
    #else
    imageSurface = SDL_LoadBMP(filename);
    #endif

    /* Make sure a valid SDL_Surface was returned */
    if ( imageSurface != NULL ) {

        tw = imageSurface -> w;
        th = imageSurface -> h;
        /* Set the original image's size (in case it's not an exact square of a power of two) */
        if (topow2) {
        
            tw = power_of_two(tw);
            th = power_of_two(th);
        
        }

        /* Create a blank SDL_Surface (of the smallest size to fit the image) & copy imageSurface into it*/
        tempSurface = SDL_CreateRGBSurface( SDL_SWSURFACE, tw, th, 32, imageSurface->format->Rmask, imageSurface->format->Gmask, imageSurface->format->Bmask, imageSurface->format->Amask );
  

        SDL_BlitSurface( imageSurface, &imageSurface -> clip_rect,
                         tempSurface, &tempSurface -> clip_rect );

        /* Fix the alpha values */
        p = tempSurface->pixels;

        for (y=th-1; y>=0; y--) {
            for (x=tw-1; x>=0; x--) {
                if (p[x+(y*tw)] != key) {
                    p[x+(y*tw)] |= 0xFF000000;
                }
                else {
                    p[x+(y*tw)] &= 0x00FFFFFF;
                }
            }
        }

        SDL_FreeSurface(imageSurface);

        return tempSurface;

    }

    return NULL;

}


/* ========================================================================== */
/* ======================== PUBLIC FUNCTIONS	============================= */
/* ========================================================================== */

/*
 * Name:
 *     sdlgltexLoadSingle
 * Description:
 *     Loads the file with the given name into a single texture
 *     The key param indicates which color to set alpha to 0.
 *      All other values are 0xFF.
 * Input:
 *     filename: Name of file to load
 * Output:
 *     Zero if failed, otherwise the number of the texture as generated
 *     by Open-GL
 */
GLuint sdlgltexLoadSingle(const char *filename)
{

    GLuint texID = 0;
    int tw, th;
    SDL_Surface	*tempSurface, *imageSurface;
    Uint32 rmask, gmask, bmask, amask;

    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0x00000000;

    /* Load the bitmap into an SDL_Surface */
    #ifdef SDLGLTEX_USER_SDLIMAGE
    imageSurface = IMG_Load(filename);
    #else
    imageSurface = SDL_LoadBMP(filename);
    #endif

    /* Make sure a valid SDL_Surface was returned */
    if ( imageSurface != NULL ) {

        /* Enable texturing */
        glEnable(GL_TEXTURE_2D);

	    /* Set the original image's size (increase it's not an exact square of a power of two) */
        tw = power_of_two(imageSurface -> w);
	    th = power_of_two(imageSurface -> h);

	    /* Create a blank SDL_Surface (of the smallest size to fit the image) & copy imageSurface into it*/
	    if (imageSurface->format->Gmask) {
            tempSurface = SDL_CreateRGBSurface( SDL_SWSURFACE, tw, th, 24, imageSurface->format->Rmask, imageSurface->format->Gmask, imageSurface->format->Bmask, imageSurface->format->Amask );
        }
	    else {
	        tempSurface = SDL_CreateRGBSurface( SDL_SWSURFACE, tw, th, 24, rmask, gmask, bmask, amask);
   	    }

	    SDL_BlitSurface( imageSurface, &imageSurface -> clip_rect,
                         tempSurface, &tempSurface -> clip_rect );

        /* Generate an OpenGL texture */
	    glGenTextures( 1, &texID );

	    /* Set up some parameters for the format of the OpenGL texture */
	    glBindTexture( GL_TEXTURE_2D, texID );					/* Bind Our Texture */
	    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );	/* Linear Filtered */
	    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );	/* Linear Filtered */

	    /* actually create the OpenGL textures */
	    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, tw, th, 0, GL_RGB, GL_UNSIGNED_BYTE, tempSurface->pixels );

	    /* get rid of our SDL_Surfaces now that we're done with them */
	    SDL_FreeSurface( tempSurface );
	    SDL_FreeSurface( imageSurface );

        glDisable(GL_TEXTURE_2D);       /* Disable it again */

        return texID;

    }

    return 0;

}


/*
 * Name:
 *     sdlgltexLoadSingleA
 * Description:
 *     Loads the file with the given name into a single texture
 *     The key param indicates which color to set alpha to 0.
 *      All other values are 0xFF.
 * Input:
 *     filename: Name of file to load
 *     key:      Color to use as alpha key
 * Output:
 *     Zero if failed, otherwise the number of the texture as generated
 *     by Open-GL
 */
GLuint sdlgltexLoadSingleA(const char *filename, unsigned int key)
{

    SDL_Surface *image;
    GLuint texID = 0;

    image = sdlgltexLoadSurfaceA(filename, key, 1);


    if (image != NULL) {

        /* Enable texturing */
        glEnable(GL_TEXTURE_2D); 

        /* Generate an OpenGL texture */
        glGenTextures( 1, &texID );

	    /* Set up some parameters for the format of the OpenGL texture */
	    glBindTexture( GL_TEXTURE_2D, texID );					/* Bind Our Texture */
	    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );	/* Linear Filtered */
	    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );	/* Linear Filtered */

	    /* actually create the OpenGL textures */
	    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA,
                      image -> w,
                      image -> h,
                      0,
                      GL_RGBA,
                      GL_UNSIGNED_BYTE,
                      image -> pixels);

	    /* get rid of SDL_Surface now that we're done with it */
	    SDL_FreeSurface(image);

        glDisable(GL_TEXTURE_2D);       /* Disable it again */

        return texID;

    }

    return 0;

}

/*
 * Name:
 *     sdlgltexLoadIcons
 * Description:
 *     Loads a set of icon lists of icons, basing on the data in the
 *     vector with SDLGLTEX_ICONCREATEINFO's.
 * Input:
 *     filename:    Name of file to load
 *     key:         Color of alphachannel
 *     icd:         Pointer on a vector with SDLGLTEX_ICONCREATEINFO's
 * Output:
 *     Zero if failed
 */
int sdlgltexLoadIcons(const char *filename, unsigned int key, SDLGLTEX_ICONCREATEINFO *icd)
{

    SDL_Surface	*image;

    image = sdlgltexLoadSurfaceA(filename, key, 0);

    /* Make sure a valid SDL_Surface was returned */
    if (image != NULL) {

        while (icd -> listno) {

            icd -> key = key; 
            sdlgltexCreateIconList(image, icd);
            icd++;

        }

        SDL_FreeSurface(image);    /* Free the loaded pic	*/

        return 1;

    }

    return 0;

}

/*
 * Name:
 *     sdlgltexFreeIcons
 * Description:
 *     Frees all created icons for the created iconlists
 * Input:
 *     None.
 */
void sdlgltexFreeIcons(void)
{
    int i;


    for (i = 0; i < SDLGLTEX_MAXICONLIST; i++) {

        if (IconList[i].number > 0) { /* if there's a list at all */

		    glDeleteTextures(IconList[i].number, IconList[i].texid);

        }

    }

}


/*
 * Name:
 *     sdlgltexDisplayIcon
 * Description:
 *     Displays an icon of the given type, with the given number at the
 *     given absolute screen position.
 * Input:
 *     type:   Type of icon to draw (number of iconlist)
 *     number: Number of icon in list
 *     rect:   Rectangle to draw the texture on
 * Output:
 *     None
 */
void sdlgltexDisplayIcon(int type, int number, SDLGL_RECT *rect)
{

    int x2, y2;
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0);

    x2 = rect -> x + rect -> w; /* IconList[type].rect.w; */
    y2 = rect -> y + rect -> h; /* IconList[type].rect.h; */

    if (glIsTexture(IconList[type].texid[number]) == GL_FALSE) {
        return;
    }

    glBindTexture(GL_TEXTURE_2D, IconList[type].texid[number]);
    glColor4f(1.0, 1.0, 1.0, 1.0);
    glBegin(GL_QUADS);
        glTexCoord2fv(&StdTexCoords[0]);
        glVertex2i( rect -> x, y2 );
        glTexCoord2fv(&StdTexCoords[2]);
        glVertex2i( x2, y2 );
        glTexCoord2fv(&StdTexCoords[4]);
        glVertex2i( x2,  rect -> y );
        glTexCoord2fv(&StdTexCoords[6]);
        glVertex2i( rect -> x, rect -> y );
    glEnd();

    /* Disable it  again */
    /* glDisable(GL_BLEND);	*/
    glDisable(GL_ALPHA_TEST);
    glDisable(GL_TEXTURE_2D);

}

/*
 * Name:
 *     sdlgltexDisplayIconTiled
 * Description:
 *     Displays an icon of the given type, with the given number at the
 *     given absolute screen position. Fills the regtangle tiled with the
 *     given icon. No alpha blending is supported.
 * Input:
 *     type:   Type of icon to draw (number of iconlist)
 *     number: Number of icon in list
 *     x, y:   Where to draw the icon.
 *     width,
 *     height: Size of the retangle to be tiled by the
 *     zoom:   zoom value
 * Output:
 *     None
 */
void sdlgltexDisplayIconTiled(int type, int number, SDLGL_RECT *rect)
{

    int x2, y2;
    GLfloat texcoords[12];


    x2 = rect -> x + rect -> w;
    y2 = rect -> y + rect -> h;

    /* Now set the extent of the textures */
    memcpy(texcoords, StdTexCoords, sizeof(StdTexCoords));
    texcoords[SDLGLTEX_TEXX1] = (GLfloat)rect -> w / (GLfloat)IconList[type].rect.w;
    texcoords[SDLGLTEX_TEXX2] = texcoords[SDLGLTEX_TEXX1];
    texcoords[SDLGLTEX_TEXY1] = (GLfloat)rect -> h / (GLfloat)IconList[type].rect.h;
    texcoords[SDLGLTEX_TEXY2] = texcoords[SDLGLTEX_TEXY1];


    glEnable(GL_TEXTURE_2D);

    /* Now draw the texture */
    glBindTexture(GL_TEXTURE_2D, IconList[type].texid[number]);

    /* Set texture parameters	*/
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    /* Generate the texture coordinates for the given rectangle */
    glColor4f(1.0, 1.0, 1.0, 1.0);
    glBegin(GL_QUADS);
        glTexCoord2fv(&texcoords[0]);
        glVertex2i(  rect -> x, y2 );
        glTexCoord2fv(&texcoords[2]);
        glVertex2i( x2, y2 );
        glTexCoord2fv(&texcoords[4]);
        glVertex2i( x2,  rect -> y );
        glTexCoord2fv(&texcoords[6]);
        glVertex2i( rect -> x,  rect -> y );
    glEnd();


    /* Set texture parameters	*/
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glDisable(GL_TEXTURE_2D);

}


/*
 * Name:
 *     sdlgltexDisplayIcon
 * Description:
 *     Displays an list of icons. The end of list has to be signed
 *     by a listno of 0.
 * Input:
 *	   fields *:  Pointer on fields to draw
 *     stop_type: Type of field where to stop display 
 * Output:
 *     None
 */
void sdlgltexDisplayIconList(SDLGL_FIELD *fields, char stop_type)
{

    float x, y, x2, y2;
    GLuint texno;


    glEnable(GL_TEXTURE_2D);
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0);

    glColor4f(1.0, 1.0, 1.0, 1.0);

    while (fields -> sdlgl_type != stop_type) {

        /* Only draw types which have a texture */
        if (fields -> tex <= 0 || fields -> subtex < 0) {

            fields++;
            continue;

        }

        /* Catch wrong icon numbers */
        if (fields -> subtex > IconList[fields -> tex].number) {

            fields++;
            continue;	/* Keep it save */

        }

        texno = IconList[fields -> tex].texid[fields -> subtex];
        if (texno >= SDLGLTEX_MAXICON || texno < 1) {

            fields++;
            continue;

        }

        x  = fields -> rect.x;
        y  = fields -> rect.y;
        x2 = fields -> rect.x + fields -> rect.w;
        y2 = fields -> rect.y + fields -> rect.h;

        if (fields -> color[0] > 0) {

            glColor3ubv(&fields -> color[0]);

        }

        glBindTexture(GL_TEXTURE_2D, texno);
        glBegin(GL_QUADS);
            glTexCoord2fv(&StdTexCoords[0]);
            glVertex2f( x, y2 );
            glTexCoord2fv(&StdTexCoords[2]);
            glVertex2f( x2, y2 );
            glTexCoord2fv(&StdTexCoords[4]);
            glVertex2f( x2,  y );
            glTexCoord2fv(&StdTexCoords[6]);
            glVertex2f( x,  y );
        glEnd();

        /* Reset color, if color was used */
        if (fields -> color[0] > 0) {

            glColor4f(1.0, 1.0, 1.0, 1.0);

        }

        fields++;

    }

    glDisable(GL_ALPHA_TEST);
    glDisable(GL_TEXTURE_2D);

}

