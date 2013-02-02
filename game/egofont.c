/*******************************************************************************
*  EGOFONT.C                                                                   *
*    - EGOBOO-Editor                                                           *     
*                                                                              *
*    - Egoboo Bitmapped Font                                                   *
*      (c) 2013 Paul Mueller <muellerp61@bluewin.ch>                           *
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

#include <math.h>   // ceil()
#include <string.h>

#include "sdlgl.h"
#include "sdlgltex.h"

// own header
#include "egofont.h"

/******************************************************************************
* DEFINES                                                                     *
******************************************************************************/

#define EGOFONT_TABSIZE    32           // Tab size in pixels 
#define EGOFONT_MAX         4
#define EGOFONT_HEIGHT     30
#define EGOFONT_ADD         4           // Vertical Gap between letters
#define NUMFONTX	    16           // Number of egofonts in the bitmap
#define NUMFONTY         6           //
#define NUMFONT         (NUMFONTX*NUMFONTY)


/******************************************************************************
* TYPEDEFS                                                                    *
******************************************************************************/

typedef struct
{
    GLfloat measure[8];	/* Four pairs of edge values            */
    GLfloat uv[8]; 	    /* Four pairs of texture coordinates    */
    int     xspacing;	/* The horizontal distance              */
    
} FONT_CHAR_T;

static int   FontYSpacing = 0;
static float EgoFontColor[] = { 1.0, 1.0, 1.0 };  

/* The egofont rectangles with texture coordinates and spacing measure*/
static FONT_CHAR_T  EgoFontChars[NUMFONT];
static GLuint TxNewFont[EGOFONT_MAX];	/* Four different egofonts ... */

/******************************************************************************
* FORWARD DECLARATIONS                                                        *
******************************************************************************/

static int egofontLengthOfWord(char *szText);

/******************************************************************************
* CODE                                                                        *
******************************************************************************/

/*
 * Name:
 *     egofontLengthOfWord
 * Description:
 *     This function returns the number of pixels the
 *     next word will take on screen in the x direction
 * Input:
 *     szText: Pointer on text to get the words length from.
 * Replaces:
 *     length_of_word
 */
static int egofontLengthOfWord(char *szText)
{
    int length = 0;
    int cnt = 0;
    unsigned char cTmp = szText[cnt];

    /* First count all preceeding spaces */
    while(cTmp == ' ' || cTmp == '~' && cTmp != 0)
    {
        if (cTmp == ' ')
        {
            length += EgoFontChars[cTmp - ' '].xspacing;
        }
        else
        {
            length += EGOFONT_TABSIZE + 1;
        }

        cnt++;
        cTmp = szText[cnt];
    }


    while(cTmp != ' ' && cTmp != '~' && cTmp != 0)
    {
        length += EgoFontChars[cTmp - ' '].xspacing;
        cnt++;
        cTmp = szText[cnt];
    }

    return length;
}

/*
 * Name:
 *     egofontBeginText
 * Description:
 *     Sets all the OpenGL-States needed for drawin text
 * Replaces:
 *     BeginText
 */
static void egofontBeginText(void)
{
    glEnable( GL_TEXTURE_2D );	/* Enable texture mapping */
    glBindTexture( GL_TEXTURE_2D, TxNewFont[0]);
    glAlphaFunc(GL_GREATER,0);
    glEnable(GL_ALPHA_TEST);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    /* Set the color for the string */
    glColor3fv(EgoFontColor);
}


/*
 * Name:
 *     egofontEndText
 * Replaces:
 *     EndText
 */
static void egofontEndText(void)
{
    egofontSetColor(1.0, 1.0, 1.0);   /* Back to white */
    glDisable(GL_ALPHA_TEST);
}

/*
 * Name:
 *     egofontDrawOneChar
 * Description:
 *     This function draws a letter or number. It is assumed that the screen
 *     is set up in a way that x/y 0,0 is at the left top of the screen 
 * Input:
 *     egofonttype: Which char to draw
 *     x, y:	 Where to draw the char.
 * Output:
 *     xspacing for this char
 */
static int egofontDrawOneChar(int letter, int x, int y)
{
    int i;
    FONT_CHAR_T drawchar;

    /* make a copy */
    memcpy(&drawchar, &EgoFontChars[letter - ' '], sizeof(FONT_CHAR_T));

    /* move the charpos */
    for (i = 0; i < 8; i += 2)
    {
        drawchar.measure[i] += x;
        drawchar.measure[i + 1] += y;
    }

    glBegin(GL_QUADS);
        glTexCoord2fv(&drawchar.uv[0]);
        glVertex2fv(&drawchar.measure[0]);

        glTexCoord2fv(&drawchar.uv[2]);
        glVertex2fv(&drawchar.measure[2]);

        glTexCoord2fv(&drawchar.uv[4]);
        glVertex2fv(&drawchar.measure[4]);

        glTexCoord2fv(&drawchar.uv[6]);
        glVertex2fv(&drawchar.measure[6]);
    glEnd();

    return drawchar.xspacing;
}

/* ========================================================================== */
/* ============================= PUBLIC FUNCTION(S) ========================= */
/* ========================================================================== */

/*
 * Name:
 *     egofontLoad
 * Description:
 *     This function loads the egofont bitmap and sets up the coordinates
 *     of each egofont on that bitmap...  Bitmap must have 16x6 egofonts
 * Input:
 *      szBitmap:   Name of the bitmapfile of the egofont
 *      sizeadjust: Adjustment of original size in bitmap. The original size
 *                  is multiplied by this factor
 * Output:
 *	    Success yes/no
 */
char egofontLoad(char *szBitmap)
{
    static int egofontxlinelen[] = { 19, 17, 15, 17, 18, 10 }; /* Always fixed */
    static int EgoFontXSpacing[] = {
        16, 8, 12, 14, 14, 20, 18, 7, 9, 9, 11, 14, 8, 8, 8, 8, 14, 12, 14,
        14, 15, 14, 14, 14, 14, 14, 8, 8, 14, 14, 14, 14, 17, 17, 16, 17,
        16, 15, 14, 17, 17, 8, 14, 18, 15, 19, 17, 17, 15, 18, 17,
        15, 16, 17, 17, 21, 17, 17, 16, 10, 8, 9, 14, 12, 7, 14, 14, 14,
        14, 14, 10, 14, 14, 8, 8, 15, 8, 20, 14, 14, 14, 14, 11, 13, 10, 14,
        14, 20, 15, 14, 13, 10, 7, 10, 14, 16
    };

    int cnt, x;
    int line;    
    float runu, runv;


    TxNewFont[0] = sdlgltexLoadSingleA(szBitmap, 0);

    if (TxNewFont[0] == 0)
    {
        return 0;
    }

    cnt = 0;
    runu = 0.0;
    runv = 0.0;

    /* egofontoffset = scry - EGOFONT_HEIGHT; */

    /* Now generate the texture offsets for all chars */
    for (line = 0; line < 6; line++)
    {
        for (x = 0; x < egofontxlinelen[line]; x++)
        {
             /* counterclockwise */
             /* first measure */
             EgoFontChars[cnt].measure[0] = 0;
             EgoFontChars[cnt].measure[1] = (EGOFONT_HEIGHT - 2);
             EgoFontChars[cnt].measure[2] = 0; /* size in pixels */
             EgoFontChars[cnt].measure[3] = 0;
             EgoFontChars[cnt].measure[4] = EgoFontXSpacing[cnt] - 1;
             EgoFontChars[cnt].measure[5] = 0;
    	     EgoFontChars[cnt].measure[6] = EgoFontXSpacing[cnt] - 1;
             EgoFontChars[cnt].measure[7] = (EGOFONT_HEIGHT - 2);

             EgoFontChars[cnt].uv[2] = runu + 0.004;
             EgoFontChars[cnt].uv[3] = runv + 0.004;;
    	     EgoFontChars[cnt].uv[0] = runu + 0.004;
             EgoFontChars[cnt].uv[1] = runv + 0.105660;
             EgoFontChars[cnt].uv[6] = runu + ((float)(EgoFontXSpacing[cnt] - 1) / 256.0) - 0.001;
             EgoFontChars[cnt].uv[7] = runv + 0.105660;
             EgoFontChars[cnt].uv[4] = runu + ((float)(EgoFontXSpacing[cnt] - 1) / 256.0) - 0.001;
             EgoFontChars[cnt].uv[5] = runv + 0.004;

             runu += ((float)(EgoFontXSpacing[cnt] + 1)/ 256.0);
             EgoFontChars[cnt].xspacing = ceil((float)EgoFontXSpacing[cnt]) + 1;
             cnt++;
        }

        runu = 0.0;		        /* U Back to begin of line  */
        runv += 0.1171875;      /* V on next line           */
    }

    // Space between lines
    FontYSpacing = (EGOFONT_HEIGHT + EGOFONT_ADD);

    return 1;    
}

/*
 * Name:
 *     egofontRelease
 * Description:
 *     Releases the texture of the egofont loaded by "egofontLoad"
 * Input:
 *     None
 */
void egofontRelease(void)
{
    glDeleteTextures(1, &TxNewFont[0]);
}

/*
 * Name:
 *     egofontDrawChar
 * Description:
 *     This function draws a letter or number.
 * Input:
 *     letter: Which letter to draw
 *     x, y:   Where to draw the char.
 * Replaces:
 *     draw_one_egofont
 */
void egofontDrawChar(int letter, int x, int y)
{
    egofontBeginText();
    egofontDrawOneChar(letter, x, y);
    egofontEndText();
}


/*
 * Name:
 *     egofontWrappedString
 * Description:
 *     This function spits a line of null terminated text onto the backbuffer,
 *     wrapping over the right side and returning the new y value
 * Input:
 *     szText: Text to draw
 *     x, y:   Where to start on screen
 *     maxx:   Maximum width (in pixels) for text output.
 * Replaces:
 *     draw_wrap_string
 */
int egofontWrappedString(char *szText, int x, int y, int maxx)
{
    int tab;
    int sttx = x;
    unsigned char cTmp = szText[0];
    int newy = y + FontYSpacing;
    unsigned char newword = 1;
    int cnt = 1;

    
    egofontBeginText();

    maxx = maxx+sttx;

    while (cTmp != 0)
    {
        /* Check each new word for wrapping */
        if (newword)
        {
            int endx = x + egofontLengthOfWord(szText + cnt - 1);

            newword = 0;

            if (endx > maxx)
            {
                /* Wrap the end and cut off spaces and tabs	*/
                x = sttx + FontYSpacing;
                y += FontYSpacing;
                newy += FontYSpacing;
                while (cTmp == ' ' || cTmp == '~')
                {
                    cTmp = szText[cnt];
                    cnt++;
                }
            }
        }
        else
        {
            if (cTmp == '~')
            {
                /* Use squiggle for tab */
                tab = x % EGOFONT_TABSIZE;
                    if (tab == 0)
                        tab = EGOFONT_TABSIZE;
                x += tab;
            }
            else
            {
                /* Normal letter  */
                x += egofontDrawOneChar(cTmp, x, y);
            }

            cTmp = szText[cnt];

            if (cTmp == '~' || cTmp == ' ')
            {
                newword = 1;
            }

            cnt++;
        }
    }

    egofontEndText();
    /* Set color back to white */
    return newy;
}


/*
 * Name:
 *     egofontDrawString
 * Description:
 *     This function spits a line of null terminated text onto the backbuffer
 * Input:
 *      szText: Text to print out.
 *      x,y:	Where to print out the text
 * Output:
 *	New y-position. Changed value form input if one or more newline
 *	characters are included into the string.
 * Replaces:
 *     draw_string
 */
int egofontDrawString(char *szText, int x, int y)
{
    unsigned char cTmp;
    int startx, tab;		/* Save for carriage newline */
    int cnt = 0;
    

    if (! szText)
    {
        return y;               /* Save from drawing empty strings      */
    }

    egofontBeginText();

    cTmp   = szText[0];
    startx = x;                 /* Aligned where we started */

    while(cTmp != 0)
    {
        // Convert ASCII to our own little egofont
        switch (cTmp)
        {
            case '\t':
            case '~':
                { /* Use squiggle for tab */
            	    tab = x % EGOFONT_TABSIZE;
                    if (tab == 0)
                        tab = EGOFONT_TABSIZE;
            	    x += tab;
                }
            	break;

            case '\n': /* Newline */
                y += FontYSpacing;      /* Down a line 		 */
                x = startx;		/* back to start of line */
             	break;

            default:
                {
                    // Normal letter
                    x += egofontDrawOneChar(cTmp, x, y);
                }
        }

        cnt++;
        cTmp = szText[cnt];
    }

    egofontEndText();
    
    return y;
}

/*
 * Name:
 *     egofontDrawStringToRect
 * Description:
 *     Draws the given text into the given rectangle, adjusting it
 *     to the rectangle depending on the given flags.
 * Input:
 *     szText: Text to draw
 *     rect:   Rect to draw the text into
 *     flags:  How to adjust relative to the gien rectangle
 */
void egofontDrawStringToRect(char *szText, EGOFONT_RECT_T *rect, int flags)
{
    EGOFONT_RECT_T textsize;

    
    if (! szText)
    {
        return;
    }
    
    egofontStringSize(szText, &textsize);
    textsize.x = rect -> x;
    textsize.y = rect -> y;

    if (flags & EGOFONT_TEXT_HCENTER)
    {
        textsize.x += (rect -> w - textsize.w) / 2;
    }
    else if (flags & EGOFONT_TEXT_RIGHTADJUST)
    {
        textsize.x += (rect -> w - textsize.w);
    }

    if (flags & EGOFONT_TEXT_VCENTER )
    {
        textsize.y += (rect -> h - textsize.h) / 2;
    }

    egofontDrawString(szText, textsize.x, textsize.y);
}

/*
 * Name:
 *     egofontSetColor
 * Description:
 *     Sets the color for the following text output.
 * Input:
 *     red, green, blue: Color to set
 */
void egofontSetColor(float red, float green, float blue)
{
     EgoFontColor[0] = red;
     EgoFontColor[1] = green;
     EgoFontColor[2] = blue;
}



/*
 * Name:
 *     egofontStringSize
 * Description:
 *     Returns the size of the given string in pixels. The left upper edge
 *     is always at (0, 0)
 * Input:
 *     text: Text to get the size from
 *     rect: Rect struct for return of text size
 */
void egofontStringSize(char *text, EGOFONT_RECT_T *rect)
{
    char  letter;
    int   tab;
    int   width;

    
    if (! text)
    {
        return;
    }

    width = 0;
    rect -> w = 0;
    rect -> h = EGOFONT_HEIGHT;

    while (*text != 0)
    {
        letter = *text;

        switch(letter)
        {
            case '\t':
            case  '~':
                { /* Use squiggle for tab */
                    tab = (width % EGOFONT_TABSIZE);
                    if (tab == 0)
                    {
                        tab = EGOFONT_TABSIZE;
                    }
                    width += tab;
                    break;
                }

            case '\n': /* Newline */
                rect -> h += FontYSpacing;      /* Down a line           */
                if (width > rect -> w)
                {
                    rect -> w = width;	        /* New horizonal size 	 */
                }
                width = 0;			/* Back to start of line */
             	break;

            default:
                {
                    // Normal letter
                    width += EgoFontChars[letter - ' '].xspacing;
                }
        }

        text++;
    }

    if (width > rect -> w)
    {
        rect -> w = width;	        /* New horizonal size 	 */
    }
}

/*
 * Name:
 *     egofontStringToMsgRect
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
 *     The actually set color is used.
 * Input:
 *     rect *:   Pointer on rect to draw the string within 
 *     string:   String to print
 */
void egofontStringToMsgRect(EGOFONT_RECT_T *rect, char *string)
{
    EGOFONT_RECT_T sizerect;
    EGOFONT_RECT_T argrect;
    char *pnextbreak, *pprevbreak;
    char prevsavechar, nextsavechar = 0;


    if (! string)
    {
        return;	/* Just for the case ... */
    }

    /* First try if the whole string fits into the rectangles width */
    egofontStringSize(string, &sizerect);

    if (sizerect.w <= rect -> w)
    {
        /* Print the string */
        egofontDrawString(string, rect -> x , rect -> y);
    }
    else
    {
        /* Drawing position if more then one line   */
        memcpy(&argrect, rect, sizeof(EGOFONT_RECT_T));

        pprevbreak = string;	/* Run pointer into the string 		    */

        /* We have to scan word by word */

        do
        {
            pnextbreak = strpbrk(pprevbreak, " \t\r\n-");

            if (pnextbreak)
            {
                /* The end of string is not reached yet */
                pnextbreak++;  	      	    /* Include the breaking character */
                nextsavechar = *pnextbreak; /* Save the break char.	       */
                *pnextbreak   = 0;	    /* Create a string out of it.     */
            }

            /* Now get the size, of the string */
            egofontStringSize(string, &sizerect);

            if (sizerect.w > rect -> w)
            {
                /* One word to much, use previous break. */
                prevsavechar = *pprevbreak;
                *pprevbreak  = 0;	/* Create the string */

                /* Print the string... */
                egofontDrawString(string, argrect.x , argrect.y);

                /* Move string to new start */
                string  = pprevbreak;
                *string = prevsavechar;   	/* Get back the prev char */

                /* Move to next line... */
                argrect.y += sizerect.h;
            }
            else
            {
            	/* Check for end of string */
                if (! pnextbreak)
                {
                    egofontDrawString(string, argrect.x, argrect.y);
                    return;
                }

                pprevbreak  = pnextbreak;    /* Move one word further.   */
            }

            if (pnextbreak)
            {
            	*pnextbreak = nextsavechar;  /* bring the next char back */
            }
    	}
        while (*string != 0);
    }
}
