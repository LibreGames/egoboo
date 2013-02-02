/*******************************************************************************
*  EGOMODEL.H                                                                  *
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

#ifndef _EGOMODEL_H_
#define _EGOMODEL_H_

/*******************************************************************************
* TYPEDEFS                                                                     *
*******************************************************************************/

typedef struct
{
    int curFrame;
    int curAnim;
    int nextFrame;
    int animStatus;
    int animSpeed;
    int animSleep;
    int interp;
    int nextiFrame;
    int lastFrame;

} EGOMODEL_ANIM;            /* Definition of an animation and it's model to use */

typedef struct
{
    int x, y, w, h;
    
} EGOMODEL_RECT;

/*******************************************************************************
* CODE                                                                         *
*******************************************************************************/

int  egomodelLoadModel(const char *fdir, int mdl_no);
void egomodelFreeAll(void);
/* @todo: Include these into function 'LoadModel'
    void egomodelLoadSkin(const char *filename, int modelno, int skin_no);
    void egomodelLoadIcon(const char *filename, int modelno, int iconno);
*/
int  egomodelDraw(int mdl_no, int frameno, int skin_no);
void egomodelDrawAnimated(int mdl_no, int skin_no, EGOMODEL_ANIM *anim, float sec_passed);
void egomodelDrawIcon(int mdl_no, int skin_no, int *crect /* x1, y1, x2, y2 */);

int  egomodelGetIconTexNo(int modelno, int iconno);

#endif /* _EGOMODEL_H_ */
