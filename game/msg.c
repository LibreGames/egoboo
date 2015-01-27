/*******************************************************************************
*  MSG.C                                                                       *
*    - EGOBOO-Game                                                             *
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
* INCLUDES                                                                     *
*******************************************************************************/

#include <memory.h>
#include <stdio.h>
#include <string.h>


#include "egofile.h"        // Directories for files
#include "sdlglcfg.h"       // Load the text file as raw
#include "misc.h"

// Own header
#include "msg.h"

/*******************************************************************************
* DEFINES                                                                      *
*******************************************************************************/

#define MSG_MAX          200    // Game messages to send around
#define MSG_STR_MAX     1200    // Message strings for display
#define MSG_STRLEN_MAX    90    // Maximum lenght of a message string  

/*******************************************************************************
* TYPEDEFS                                                                     *
*******************************************************************************/

typedef struct
{
    int act_idx;                    // For loading dynamic string
    int first_global_tip;           // Number of first global tip message
    int num_global_tips;            // Number of global game tips
    int first_local_tip;            // Number of first local  tip message
    int num_local_tips;             // Number of local game tips (random)    
    char msg[MSG_STR_MAX + 2][MSG_STRLEN_MAX]; // All possible messages
    
} MSG_STRLIST_T;


/*******************************************************************************
* DATA                                                                         *
*******************************************************************************/

// Messages for the game: First: Global Tips, Second: 
static MSG_T MsgList[MSG_MAX + 2];
static MSG_STRLIST_T MsgStrList = 
{
    0, 0, 0, 0, 0,
    { "Don't die...\n" }
};

/*******************************************************************************
* CODE                                                                         *
*******************************************************************************/

/*
 * Name:
 *     msgExpand
 * Description:
 *     Puts the message into the message list for collection by the 'receiver'  
 *     The number of the message is filled into the given info and then the 
 *     message is added at the end of the message queue.
 *     Clears the message list, if the number of the message is < 0
 * Input:
 *     msg *:     Pointer on message descriptor with info for expanding string
 *     psrc *:    String to expand 
 *     pdest *:   Where to return the expanded string
 *     dest_size: Maximum length of destination, including trailing 0
 */
static void msgExpand(MSG_T *msg, char *psrc, char *pdest, int dest_size)
{
    // @todo: Flesh this out / Port it
    // int  cnt;
    // char szTmp[256];

    /*
    chr_t      * pchr, *ptarget, *powner;
    ai_state_t * pai;

    pchr    = !INGAME_CHR( ichr ) ? NULL : ChrList_get_ptr( ichr );
    pai     = ( NULL == pchr )    ? NULL : &( pchr->ai );

    ptarget = (( NULL == pai ) || !INGAME_CHR( pai->target ) ) ? pchr : ChrList_get_ptr( pai->target );
    powner  = (( NULL == pai ) || !INGAME_CHR( pai->owner ) ) ? pchr : ChrList_get_ptr( pai->owner );

    cnt = 0;
    while ( CSTR_END != *src && src < src_end && dst < dst_end )
    {
        if ( '%' == *src )
        {
            char * ebuffer, * ebuffer_end;

            // go to the escape character
            src++;

            // set up the buffer to hold the escape data
            ebuffer     = szTmp;
            ebuffer_end = szTmp + SDL_arraysize( szTmp ) - 1;

            // make the excape buffer an empty string
            *ebuffer = CSTR_END;

            switch ( *src )
            {
                case '%' : // the % symbol
                    {
                        snprintf( szTmp, SDL_arraysize( szTmp ), "%%" );
                    }
                    break;

                case 'n' : // Name
                    {
                        chr_get_name( ichr, CHRNAME_ARTICLE, szTmp, SDL_arraysize( szTmp ) );
                    }
                    break;

                case 'c':  // Class name
                    {
                        if ( NULL != pchr )
                        {
                            ebuffer     = chr_get_pcap( ichr )->classname;
                            ebuffer_end = ebuffer + SDL_arraysize( chr_get_pcap( ichr )->classname );
                        }
                    }
                    break;

                case 't':  // Target name
                    {
                        if ( NULL != pai )
                        {
                            chr_get_name( pai->target, CHRNAME_ARTICLE, szTmp, SDL_arraysize( szTmp ) );
                        }
                    }
                    break;

                case 'o':  // Owner name
                    {
                        if ( NULL != pai )
                        {
                            chr_get_name( pai->owner, CHRNAME_ARTICLE, szTmp, SDL_arraysize( szTmp ) );
                        }
                    }
                    break;

                case 's':  // Target class name
                    {
                        if ( NULL != ptarget )
                        {
                            ebuffer     = chr_get_pcap( pai->target )->classname;
                            ebuffer_end = ebuffer + SDL_arraysize( chr_get_pcap( pai->target )->classname );
                        }
                    }
                    break;

                case '0':
                case '1':
                case '2':
                case '3': // Target's skin name
                    {
                        if ( NULL != ptarget )
                        {
                            ebuffer = chr_get_pcap( pai->target )->skin_info.name[( *src )-'0'];
                            ebuffer_end = ebuffer + SDL_arraysize( chr_get_pcap( pai->target )->skin_info.name[( *src )-'0'] );
                        }
                    }
                    break;

                case 'a':  // Character's ammo
                    {
                        if ( NULL != pchr )
                        {
                            if ( pchr->ammoknown )
                            {
                                snprintf( szTmp, SDL_arraysize( szTmp ), "%d", pchr->ammo );
                            }
                            else
                            {
                                snprintf( szTmp, SDL_arraysize( szTmp ), "?" );
                            }
                        }
                    }
                    break;

                case 'k':  // Kurse state
                    {
                        if ( NULL != pchr )
                        {
                            if ( pchr->iskursed )
                            {
                                snprintf( szTmp, SDL_arraysize( szTmp ), "kursed" );
                            }
                            else
                            {
                                snprintf( szTmp, SDL_arraysize( szTmp ), "unkursed" );
                            }
                        }
                    }
                    break;

                case 'p':  // Character's possessive
                    {
                        if ( NULL != pchr )
                        {
                            if ( pchr->gender == GENDER_FEMALE )
                            {
                                snprintf( szTmp, SDL_arraysize( szTmp ), "her" );
                            }
                            else if ( pchr->gender == GENDER_MALE )
                            {
                                snprintf( szTmp, SDL_arraysize( szTmp ), "his" );
                            }
                            else
                            {
                                snprintf( szTmp, SDL_arraysize( szTmp ), "its" );
                            }
                        }
                    }
                    break;

                case 'm':  // Character's gender
                    {
                        if ( NULL != pchr )
                        {
                            if ( pchr->gender == GENDER_FEMALE )
                            {
                                snprintf( szTmp, SDL_arraysize( szTmp ), "female " );
                            }
                            else if ( pchr->gender == GENDER_MALE )
                            {
                                snprintf( szTmp, SDL_arraysize( szTmp ), "male " );
                            }
                            else
                            {
                                snprintf( szTmp, SDL_arraysize( szTmp ), " " );
                            }
                        }
                    }
                    break;

                case 'g':  // Target's possessive
                    {
                        if ( NULL != ptarget )
                        {
                            if ( ptarget->gender == GENDER_FEMALE )
                            {
                                snprintf( szTmp, SDL_arraysize( szTmp ), "her" );
                            }
                            else if ( ptarget->gender == GENDER_MALE )
                            {
                                snprintf( szTmp, SDL_arraysize( szTmp ), "his" );
                            }
                            else
                            {
                                snprintf( szTmp, SDL_arraysize( szTmp ), "its" );
                            }
                        }
                    }
                    break;

                case '#':  // New line (enter)
                    {
                        snprintf( szTmp, SDL_arraysize( szTmp ), "\n" );
                    }
                    break;

                case 'd':  // tmpdistance value
                    {
                        if ( NULL != pstate )
                        {
                            snprintf( szTmp, SDL_arraysize( szTmp ), "%d", pstate->distance );
                        }
                    }
                    break;

                case 'x':  // tmpx value
                    {
                        if ( NULL != pstate )
                        {
                            snprintf( szTmp, SDL_arraysize( szTmp ), "%d", pstate->x );
                        }
                    }
                    break;

                case 'y':  // tmpy value
                    {
                        if ( NULL != pstate )
                        {
                            snprintf( szTmp, SDL_arraysize( szTmp ), "%d", pstate->y );
                        }
                    }
                    break;

                case 'D':  // tmpdistance value
                    {
                        if ( NULL != pstate )
                        {
                            snprintf( szTmp, SDL_arraysize( szTmp ), "%2d", pstate->distance );
                        }
                    }
                    break;

                case 'X':  // tmpx value
                    {
                        if ( NULL != pstate )
                        {
                            snprintf( szTmp, SDL_arraysize( szTmp ), "%2d", pstate->x );
                        }
                    }
                    break;

                case 'Y':  // tmpy value
                    {
                        if ( NULL != pstate )
                        {
                            snprintf( szTmp, SDL_arraysize( szTmp ), "%2d", pstate->y );
                        }
                    }
                    break;

                default:
                    snprintf( szTmp, SDL_arraysize( szTmp ), "%%%c???", ( *src ) );
                    break;
            }

            if ( CSTR_END == *ebuffer )
            {
                ebuffer     = szTmp;
                ebuffer_end = szTmp + SDL_arraysize( szTmp );
                snprintf( szTmp, SDL_arraysize( szTmp ), "%%%c???", ( *src ) );
            }

            // make the line capitalized if necessary
            if ( 0 == cnt && NULL != ebuffer )  *ebuffer = char_toupper(( unsigned )( *ebuffer ) );

            // Copy the generated text
            while ( CSTR_END != *ebuffer && ebuffer < ebuffer_end && dst < dst_end )
            {
                *dst++ = *ebuffer++;
            }
            *dst = CSTR_END;
        }
        else
        {
            // Copy the letter
            *dst = *src;
            dst++;
        }

        src++;
        cnt++;
    }

    // make sure the destination string is terminated
    if ( dst < dst_end )
    {
        *dst = CSTR_END;
    }
    *dst_end = CSTR_END;
    */
}

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
 *     sender:    Sent by this character 
 *     receiver:  This character has to react on this message
 *     why:       Why the message is sent (AI: ALERTIF_
 *     log_str *: Message for log 
 */
void msgSend(int sender, int receiver, int why, char *log_str)
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
    MSG_BORED,
    MSG_TOOMUCHBAGGAGE,
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

/*
 * Name:
 *     msgLoad
 * Description:
 *     Load message(s) of given type into the 
 * Input:
 *     which: Which kind of message to load 
 * Output:
 *     Number of first message loaded 
 */
void msgLoad(int which)
{
    switch(which)
    {
        case MSG_LOAD_RESET:
            // Reset the message-buffer
            break;
        case MSG_TIP_GLOBAL:
            // Load global tips from file
            break;
        case MSG_TIP_LOCAL:
            // Load local tips from file, starts overwrite 
            break;
        case MSG_SCRIPT_LIST:   
            // Load messages belonging to a script
            break;
    }
}

/*
 * Name:
 *     msgGetText
 * Description:
 *     Get a message of given type 
 * Input:
 *     which: Which kind of message to get
 * Output:
 *     Pointer on string
 */
char *msgGetText(int which)
{
    const char *retval = "Don't die...\n";    
    int msg_no;
    
    
    // @todo: Return local hint, if available, else return global hint
    switch(which)
    {
        case MSG_TIP_GLOBAL:
            // Get a global tips string, random
            msg_no = miscRandVal(MsgStrList.num_global_tips);
            msg_no += MsgStrList.first_global_tip;
            break;
            
        case MSG_TIP_LOCAL:
            // Get a local tips string, random
            msg_no = miscRandVal(MsgStrList.num_local_tips);
            msg_no += MsgStrList.first_local_tip;
            break;
        default:
            return "Don't die...\n";
    }
    
    return MsgStrList.msg[msg_no];
}

/*
 * Name:
 *     msgGetTextModule
 * Description:
 *      Get the message with given number 'translated', if needed
 * Input:
 *     which: Which kind of messate to get
 * Output:
 *     Number of first message loaded 
 */
int msgGetTextScript(MSG_T *pmsg, int msg_no, char *str_buf, int buf_len) 
{
    char *src_str;
    
    
    str_buf[0] = 0;     // Create an empty string
    
    src_str = MsgStrList.msg[msg_no];
    // msgExpand(pmsg, src_str, str_buf, buf_len);
    
    strncpy(str_buf, src_str, buf_len - 1);
    str_buf[buf_len - 1] = 0;
    
    return str_buf[0];
}

