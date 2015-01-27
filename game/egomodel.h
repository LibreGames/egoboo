/*******************************************************************************
*  EGOMODEL.H                                                                  *
*    - Load and display for Quake 2 Models                                     *
*                                                                              *
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
* INCLUDES                                                                     *
*******************************************************************************/

#include "sdlgl3d.h"        // SDLGL3D_OBJECT

/*******************************************************************************
* TYPEDEFS                                                                     *
*******************************************************************************/

typedef struct
{
    int x, y, w, h;
    
} EGOMODEL_RECT;

/*******************************************************************************
* CODE                                                                         *
*******************************************************************************/

int  egomodelLoad(float bbox[2][3]);
void egomodelFreeAll(void);
// Actions
int  egomodelDraw(int mdl_no, int skin_no, int frame_no);
void egomodelDrawObject(SDLGL3D_OBJECT *pobj);
char egomodelSetAnimation(int obj_no, short int action);

void egomodelDrawIcon(int mdl_no, int skin_no, int *crect /* x1, y1, x2, y2 */);

#endif /* _EGOMODEL_H_ */
