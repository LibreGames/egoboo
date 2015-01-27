/*******************************************************************************
*  SDLGLMD2.H                                                                  *
*	- Load and display for Quake 2 Models      		                           *
*									                                           *
*   Copyright © 2000, by Mustata Bogdan (LoneRunner)                           *
*   Adjusted for use with SDLGL Paul Mueller <bitnapper>                       *
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

#ifndef _SDLGL_SDLGLMD2_H_
#define _SDLGL_SDLGLMD2_H_

/*******************************************************************************
* TYPEDEFS                                                                     *
*******************************************************************************/

typedef struct {

    int curFrame;
    int curAnim;
    int nextFrame;
    int animStatus;
    int animSpeed;
    int animSleep;
    int interp;
    int nextiFrame;
    int lastFrame;

} SDLGLMD2_ANIM;            /* Definition of an animation and it's model to use */

/*******************************************************************************
* CODE                                                                         *
*******************************************************************************/

int  sdlglmd2LoadModel(const char *filename);
int  sdlglmd2DrawModel(int modelno, int frameno, int skinno);
void sdlglmd2DrawModelAnimated(int modelno, int skinno, SDLGLMD2_ANIM *anim, int tickspassed);
void sdlglmd2FreeModels(void);
void sdlglmd2LoadSkin(const char *filename, int modelno, int skinno);
void sdlglmd2LoadIcon(const char *filename, int modelno, int iconno);
int  sdlglmd2GetIconTexNo(int modelno, int iconno);

#endif /* _SDLGL_SDLGLMD2_H_ */
