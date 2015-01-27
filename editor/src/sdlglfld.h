/*******************************************************************************
*  SDLGLFLD.H                                                                  *
*      - Different basic functions for handling of SDLGL_FIELD-fields          *
*                                                                              *
*  SDLGL - Library                                                             *
*      (c)2004 Paul Mueller <pmtech@swissonline.ch>                            *
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

#ifndef _SDLGL_FLD_H_
#define _SDLGL_FLD_H_

/*******************************************************************************
* INCLUDES                                								       *
*******************************************************************************/

#include "sdlgl.h"

/*******************************************************************************
* TYPEDEFS                                 								       *
*******************************************************************************/

typedef struct {

    int usertype;   /* Can be used by user to find box in a list        */
    int eltop;      /* Number of top element                            */
    int elvisi;     /* Number of elements visible in slider box         */
    int numelement; /* Number of elements in slider box                 */
    int helvisi;    /* Number of horizonal elements for h. slider boxes */
                    /* if > 0, it's a horizontal sliderbox              */
    int elchosen;   /* For highlight info: From top of box              */
    int actel;      /* Number of actual element (effective)             */
    /* ---- The following info is used to adjust the slider button ---- */
    SDLGL_FIELD *fields;    /* Pointer on first field of slider box     */

} SDLGLFLD_SLIDERBOX;

/*******************************************************************************
* CODE                                   								       *
*******************************************************************************/

    /* ======== Input handling functions ========= */
int  sdlglfldHandle(SDLGL_EVENT *event);
void sdlglfldValueFromClick(SDLGL_EVENT *event, int maxval, int *value);
void sdlglfldChangeValue(int dir, int maxval, int *value);
void sdlglfldSlider(SDLGL_EVENT *event, int maxval, int *value, SDLGL_RECT *button);
void sdlglfldSliderBox(SDLGL_EVENT *event, SDLGLFLD_SLIDERBOX *sb);

#endif
