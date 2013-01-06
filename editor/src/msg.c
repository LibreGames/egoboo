/*******************************************************************************
*  MSG.C                                                                       *
*    - EGOBOO-Editor                                                           *     
*                                                                              *
*    - Sending and handling messages for editor and play                       *
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

#include <memory.h>


// Own header
#include "msg.h"

/*******************************************************************************
* DEFINES								                                   *
*******************************************************************************/

#define MSG_MAX 200

/*******************************************************************************
* DATA									                                   *
*******************************************************************************/

static MSG_T MsgList[MSG_MAX + 2];

/*******************************************************************************
* CODE									                                   *
*******************************************************************************/

/* ========================================================================== */
/* ============================= PUBLIC FUNCTION(S) ========================= */
/* ========================================================================== */

/*
 * Name:
 *     msgSend
 * Description:
 *     Puts the message into the message list for collection by the 'receiver'  
 *     The numbe rof the message is filled into the given info and then the 
 *     message is added at the end of the message queue.
 *     Clears the message list, if the number of the message is < 0
 * Input:
 *     sender:   Sent by this character 
 *     receiver: This character has to react on this message
 *     why:      Why the message is sent (AI: ALERTIF_
 */
void msgSend(int sender, int receiver, int why)
{
    int msgno;
    
    
    if(MSG_RESET == why)
    {
        // Reset the message buffer
        memset(MsgList, 0, sizeof(MsgList)); /* Clear msg list   */
        MsgList[0].id = 0;                  // no message
        MsgList[1].id = 0;                  // no message
    }
    else
    {
        /* First look for empty message slot */
        for (msgno = 1; msgno < MSG_MAX; msgno++)
        {
            if(MsgList[msgno].id <= 0)
            {
                // End of list or deleted
                MsgList[msgno].id       = msgno;
                MsgList[msgno].why      = why;
                MsgList[msgno].sender   = sender;
                MsgList[msgno].receiver = receiver;
                // Message is sent
                return;
            }
        }
    }
}

/*
 * Name:
 *     msgGet
 * Description:
 *     Returns a copy of the message in the message buffer, if any to the receiver *     
 * Input:
 *     receiver: Number of character to get message for  
 *     pmsg *:   Pointer on buffer to fill with message info
 *     remove:   Remove message from message-stack, yes/no
 * Output:
 *     Message available yes/no
 */
char msgGet(int receiver, MSG_T *pmsg, int remove)
{
    int msgno;

    
    for (msgno = 1; msgno < MSG_MAX; msgno++)
    {
        if(receiver == MsgList[msgno].receiver)
        {
            // Copy message to caller
            memcpy(pmsg, &MsgList[msgno], sizeof(MSG_T));
            
            if(remove)
            {
                memset(&MsgList[msgno], 0, sizeof(MSG_T));
                // Mark it as deleted
                MsgList[msgno].id = -1;
            }
            // We returned a message
            return 1;
        }
    }
    /* Clear the buffer given in the argument */
    memset(pmsg, 0, sizeof(MSG_T));
    
    return 0;
}

/*
 * Name:
 *     msgGetNext
 * Description:
 *     Gets the next message from the message stack, if there's any left
 *     if 'prev_no' == 0, then the first message   
 * Input:
 *     prev_no: Number of last message returned by this function
 *     pmsg *:  Where to return the message descriptor
 * Output:
 *     > valid message number, Message buffer is filled  
 *     String has been generated yes/no
 */
int msgGetNext(int prev_no, MSG_T *pmsg)
{
    int msg_no;
    
    
    if(prev_no == 0)
    {
        // Start with first message
        prev_no = 1;
    }
    
    // return next valid message
    for (msg_no = prev_no; msg_no < MSG_MAX; msg_no++)
    {
        if(MsgList[msg_no].id == 0)
        {
            // end of list
            return 0;
        }
        
        if(MsgList[msg_no].id > 0)
        {
            // Copy message to caller
            memcpy(pmsg, &MsgList[msg_no], sizeof(MSG_T));
            // Remove it
            memset(&MsgList[msg_no], 0, sizeof(MSG_T));
            // Mark it as deleted
            MsgList[msg_no].id = -1;
            // Found one
            return 1;
        }
    }

    return 0;    
}

/*
 * Name:
 *     msgToString
 * Description:
 *     'Translates' the message into 'human readable form'
 * Input:
 *     pmsg *:    Pointer on message ti translate to string
 *     str_buf *: Where to return the string
 *     buf_len:   Maximum size of 'str_buf'
 * Output:
 *     String has been generated yes/no
 */
char msgToString(MSG_T *pmsg, char *str_buf, int buf_len)
{

    // @todo: Write 'translation code'
    switch(pmsg -> why)
    {
        // AI-Messages
        case MSG_LEVELUP:
            // "%s gained a level!!!", DisplayMsg_printf... chr_get_name
            // chr_get_name( GET_REF_PCHR( pchr ), CHRNAME_ARTICLE | CHRNAME_DEFINITE | CHRNAME_CAPITAL, NULL, 0 )
            // sound_play_chunk_full( g_wavelist[GSND_LEVELUP] );
            break;
        // Other messages
        case MSG_CANTUSEITEM:
            // "%s can't use this item...", chr_get_name // DisplayMsg_printf
            break;
    }

    // Dead code to keep compiler quiet
    str_buf[0] = 0;
    // Dead code to keep compiler quiet
    if(buf_len > 0)
    {
    }
/*
typedef enum
{
    
    MSG_SPAWNED,
    MSG_HITVULNERABLE,
    MSG_ATWAYPOINT,
    MSG_ATLASTWAYPOINT,
    MSG_ATTACKED,
    MSG_BUMPED,
    MSG_ORDERED,
    MSG_CALLEDFORHELP,
    MSG_KILLED,
    MSG_TARGETKILLED,
    MSG_DROPPED,
    MSG_GRABBED,
    MSG_REAFFIRMED,
    MSG_LEADERKILLED,
    MSG_USED,
    MSG_CLEANEDUP,
    MSG_SCOREDAHIT,
    MSG_HEALED,
    MSG_DISAFFIRMED,
    MSG_CHANGED,
    MSG_INWATER,
    MSG_BORED1,
    MSG_TOOMUCHBAGGAGE,
            MSG_LEVELUP,
    MSG_CONFUSED,
    MSG_HITGROUND,
    MSG_NOTDROPPED,
    MSG_BLOCKED,
    MSG_THROWN,
    MSG_CRUSHED,
    MSG_NOTPUTAWAY,
    MSG_TAKENOUT,
            MSG_CANTUSEITEM,

    // add in some aliases
    MSG_PUTAWAY     = MSG_ATLASTWAYPOINT,
    MSG_NOTTAKENOUT = MSG_NOTPUTAWAY

} E_MSG_WHY;
    */
    return 0;
}