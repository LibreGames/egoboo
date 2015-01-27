/*******************************************************************************
*  EGOFONT.H                                                                   *
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

#ifndef _EGOFONT_H_
#define _EGOFONT_H_

/*******************************************************************************
* DEFINES                                                                     *
*******************************************************************************/

#define EGOFONT_MOUSECURSOR    127
#define EGOFONT_MOUSECURSORRTS  43

/* ------ Flags for drawing text into a rectangle ------- */
#define EGOFONT_TEXT_HCENTER     0x0001          /* Center horizontally in rect. */
#define EGOFONT_TEXT_VCENTER     0x0002          /* Center vertically in rect.   */
#define EGOFONT_TEXT_LEFTADJUST  0x0004          /* Start on left side of rect.  */
#define EGOFONT_TEXT_RIGHTADJUST 0x0008          /* Start on right side of rect. */
#define EGOFONT_TEXT_CENTER (EGOFONT_TEXT_HCENTER | EGOFONT_TEXT_VCENTER)

/******************************************************************************
* TYPEDEFS                                                                    *
******************************************************************************/

typedef struct
{
    int x, y,
        w, h;

} EGOFONT_RECT_T;

/******************************************************************************
* CODE                                                                        *
******************************************************************************/

char egofontLoad(char *szBitmap);
void egofontRelease(void);
void egofontSetColor(float red, float green, float blue);
void egofontDrawChar(int letter, int x, int y);
int  egofontWrappedString(char *szText, int x, int y, int maxx);
int  egofontDrawString(char *szText, int x, int y);
void egofontStringSize(char *text, EGOFONT_RECT_T *rect);
void egofontStringToMsgRect(EGOFONT_RECT_T *rect, char *string);

#endif /* _EGOFONT_H_ */
