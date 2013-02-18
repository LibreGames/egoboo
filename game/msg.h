/*******************************************************************************
*  MSG.H                                                                       *
*    - EGOBOO-Game                                                             *
*                                                                              *
*    - Send/Receive messages in game                                           *
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

#ifndef _MSG_H_
#define _MSG_H_

/*******************************************************************************
* INCLUDES								                                   *
*******************************************************************************/

/*******************************************************************************
* ENUMS                               								         *
*******************************************************************************/

// @note: The messaging system replaces the 'ALERTIF-Flags'. Thus the AI don't 
//        has to poll for alerts, it just reacts if it receives a message
//        by the function 'aiTranslateMsg()' which is sent by 'getMsg()' in the
//        game-loop. Thus, they can be sent to a network-player, too
typedef enum
{
    MSG_RESET = -1,  // Reset the internal message buffer
    MSG_NONE = 0,
    MSG_ATLASTWAYPOINT,
    MSG_ATTACKED,
    MSG_ATWAYPOINT,
    MSG_BLOCKED,
    MSG_BORED,
    MSG_BUMPED,
    MSG_CALLEDFORHELP,
    MSG_CANTUSEITEM,
    MSG_CHANGED,
    MSG_CLEANEDUP,
    MSG_CONFUSED,
    MSG_CRUSHED,
    MSG_DISAFFIRMED,
    MSG_DROPPED,
    MSG_GRABBED,
    MSG_HEALED,
    MSG_HITGROUND,
    MSG_HITVULNERABLE,
    MSG_INWATER,
    MSG_KILLED,
    MSG_LEADERKILLED,
    MSG_LEVELUP,
    MSG_NOTDROPPED,
    MSG_NOTPUTAWAY,
    MSG_ORDERED,
    MSG_REAFFIRMED,
    MSG_SCOREDAHIT,
    MSG_SPAWNED,
    MSG_TAKENOUT,
    MSG_TARGETKILLED,
    MSG_THROWN,
    MSG_TOOMUCHBAGGAGE,
    MSG_USED,

    // add in some aliases
    MSG_PUTAWAY     = MSG_ATLASTWAYPOINT,
    MSG_NOTTAKENOUT = MSG_NOTPUTAWAY,
    MSG_TOOBIG,
    MSG_ITEMFOUND,
    MSG_HURT,       // Message to attacker that the target is hurt (for scripts)
    // Other messages
    MSG_GAME_OBJNOTFOUND = 1000
    
} E_MSG_WHY;

/*******************************************************************************
* DEFINES                               								    *
*******************************************************************************/

// Special receivers
#define MSG_REC_GAME    -2  // This message is for the game
#define MSG_REC_LOG     -3  // This message is for the log
#define MSG_REC_ERROR   -4  // This is an error message

// Type of game-tips
#define MSG_LOAD_RESET  0  // Reset the message buffer to 
#define MSG_TIP_GLOBAL  1
#define MSG_TIP_LOCAL   2
#define MSG_SCRIPT_LIST 3   // Is the list of messages for a script

/*******************************************************************************
* TYPEDEFS                                                                     *
*******************************************************************************/

typedef struct
{    
    int  id;        // <> 0: Message is valid in list: 0: End of List        
    int  why;       // Why this message was sent for:
                    // translation to 'human readable form'
                    // reaction of AI         
    int sender;     // Sender of message  'msgSend()'                  
    int receiver;   // receiver of message 'msgGet()'
    // @todo: Additional info, if needed  for 'expanded' script Messages
    // @todo: Info from AI-State for these variables    
    // @todo: Load in-game messages to ==> msgModuleLoad() / msgModuleGet()
    char *expand_str;   // For ingame messages which have to be expanded
    
} MSG_T; 

/*******************************************************************************
* CODE 								                                       *
*******************************************************************************/

void msgSend(int sender, int receiver, int why, char *log_str);
// @todo: msgLog(char *log_str);
char msgGet(int receiver, MSG_T *pmsg, int remove);
int  msgGetNext(int prev_no, MSG_T *pmsg);
char msgToString(MSG_T *pmsg, char *str_buf, int buf_len);
void msgLoad(int which);        // Global and local tips, script messages
int  msgGetText(int which, int msg_no, char *str_buf, int buf_len);
int  msgGetTextScript(MSG_T *pmsg, int msg_no, char *str_buf, int buf_len); 

#endif  /* #define _MSG_H_ */