/*******************************************************************************
*  SPELL.H                                                                     *
*    - EGOBOO-Game                                                             *     
*                                                                              *
*    - [...]                                                                   *
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

#ifndef _SPELL_H_
#define _SPELL_H_

/*******************************************************************************
* INCLUDES                                                                     *
*******************************************************************************/


/*******************************************************************************
* TYPEDEFS                                                                     *
*******************************************************************************/

// Description of values that can be set by a spell and are reset if spell expires
typedef struct
{
    char which;     
    char sub_code;      // Which damage modifier    <CHAR_VAL_DMGRESIST
                        // Missile treatment as 'char' ( NORMAL, DEFLECT, REFLECT )
    char inv_type;      // Type of inversion, if damage
    short int value;
    
} SPELL_SET_T;

// Description of values than can be modified by a spell and removed if it expires
// These values are cumulative
typedef struct
{
    char which;
    float value;
    
} SPELL_ADD_T;

/*******************************************************************************
* CODE                                                                         *
*******************************************************************************/

void spellInit(void);
int  spellLoad(void);

#endif  /* #define _SPELL_H_ */