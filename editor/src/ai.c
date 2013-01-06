/*******************************************************************************
*  AI.C                                                                        *
*    - EGOBOO-Editor                                                           *     
*                                                                              *
*    - AI: Main functions                                                      *
*      (c) 2013 Paul Mueller <muellerp61@bluewin.ch>                           *
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
* INCLUDES								                                   *
*******************************************************************************/


#include "msg.h"

// Own header
#include "ai.h"

/*******************************************************************************
* TYPEDEFS								                                   *
*******************************************************************************/

/*******************************************************************************
* DATA									                                   *
*******************************************************************************/

/*******************************************************************************
* CODE									                                       *
*******************************************************************************/

// @todo: aiHandleOneMessage(&msg_info);

/* ========================================================================== */
/* ============================= PUBLIC FUNCTION(S) ========================= */
/* ========================================================================== */

/*
 * Name:
 *     aiMain
 * Description:
 *     Runs trough all AI-Profiles and executes the code based on state and so on.
 *     Removes the messages from the message stack. 
 *     @important: Human players have to fetch their messages before this function 
 * Input: 
 *     None
 */
void aiMain(void)
{
    int msg_no;
    MSG_T msg_info;
    
    
    // Loop trough all messages
    msg_no = msgGetNext(0, &msg_info);
    
    while(msg_no > 0)
    {
        // @todo: Translate all messages for this ai
        // @todo: aiHandleOneMessage(&msg_info);
        
        // Get next message
        msg_no = msgGetNext(msg_no, &msg_info);
    }
}