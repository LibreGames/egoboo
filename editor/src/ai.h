/*******************************************************************************
*  TEMPLATE.H                                                                  *
*    - EGOBOO-Editor                                                           *     
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

#ifndef _TEMPLATE_H_
#define _TEMPLATE_H_

/*******************************************************************************
* INCLUDES								                                   *
*******************************************************************************/

/*******************************************************************************
* TYPEDEFS								                                       *
*******************************************************************************/

typedef struct
{
    // Init basic data
    int index;
    int owner;    // AI is owned by this character (self)
    int alert;
    int state;
    int content;
    int passage;
    int target;    
    int child;
    int target_old;
    
    int bumplast;
    int hitlast;

    int order_counter;
    int order_value;
    int lastitemused;
    int timer;          // > 0: Timeout in ticks 
    char timer_set;     // Timer is set OR send a message
    int poof_time;      // Time before poofing
    // Script stuff
    int script_no;  // Number of script for this one
    
} AI_STATE_T;       // State of an AI-Controlled character 

typedef struct 
{
    int     x;
    int     y;
    int     turn;
    int     distance;
    int     argument;
    int     operationsum;
    
} SCRIPT_STATE_T;       // Actual arguments 'tmpx', 'tmpy'... in scripts 


/*******************************************************************************
* CODE 								                                       *
*******************************************************************************/

void aiStateInit(const int char_no, char rank);
void aiMain(void);

#endif  /* #define _TEMPLATE_H_ */