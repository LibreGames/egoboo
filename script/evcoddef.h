/******************************************************************************
*    EVCODDEF.H                                                               *
*        The event definitions needed for coding and decoding of JADBASIC     *
*	 for the events used in JAD.					                          *	 	
*        Initial code: September-08-2001 / PM				                  *
******************************************************************************/

#ifndef  _EVCODDEF_H_
#define _EVCODDEF_H_

/******************************************************************************
* DEFINES		      						                                  *
******************************************************************************/

/* Different event definitions */
#define EVENT_ITEMHAND   ((char)0xFF)
#define EVENT_ITEMBOTTOM ((char)0xFE)

/* The different possible types that can be used in the events */
#define EVENT_COMPAREOR     ((char)0xF8)
#define EVENT_WHOLEWALL	    ((char)0xF7)
#define EVENT_ITEMVALUE     ((char)0xF6)
#define EVENT_ITEMNO	    ((char)0xF5)
// #define 0xF4 not used
#define EVENT_MONSTER       ((char)0xF3)
#define EVENT_THISITEMNO    ((char)0xF2)
#define EVENT_PARTY	        ((char)0xF1)
#define EVENT_GLOBALFLAG    ((char)0xF0)
#define EVENT_LEVELFLAG     ((char)0xEF)
// #define EVENT_CONDITION  0xEE  // SC_CODEIF SC_CODETHEN
#define EVENT_ENDCONDITION  ((char)0xEE) 
#define EVENT_PARTYDIRECTION ((char)0xED)
// #define EVENT_THROWNITEM 0xEC
#define EVENT_LEVEL	        ((char)0xEB)
#define EVENT_DOOR	        ((char)0xEA)
#define EVENT_WALLPART      ((char)0xE9)
#define EVENT_WHOLEPARTY    ((char)0xE8)
#define EVENT_USEDITEM      ((char)0xE7)
#define EVENT_PLAYERPOS     ((char)0xE6) // #define 0xE6 ==> Own definition, has to be checked
#define EVENT_PLAYER	    ((char)0xE5)
#define EVENT_CHOICE	    ((char)0xE4)
#define EVENT_ITEMCLASS     ((char)0xE3) // #define 0xE3 ==> Own definition, has to be checked 
#define EVENT_VALUE	        ((char)0xE2)
#define EVENT_ITEMTYPE	    ((char)0xE1)
#define EVENT_TRIGGERFLAGS  ((char)0xE0)
#define EVENT_SPELL	        ((char)0xDF)
#define EVENT_ITEM          ((char)0xDE)
#define EVENT_MEMBERRACE    ((char)0xDD)	
#define EVENT_MEMBERCLASS   ((char)0xDC)
#define EVENT_RANDOMVALUE   ((char)0xDB)
#define EVENT_PARTYVISIBLE  ((char)0xDA)
// #define 0xD9
#define EVENT_TEXTCHOICE    0xD8
#define EVENT_TRIGGERITEM   0xD7
#define EVENT_WINDOWTEXT    0xD6
#define EVENT_DISPLAYTEXT   0xD5
#define EVENT_WINDOWUPDATE  0xD4
#define EVENT_OUTPICS	    0xD3
#define EVENT_COMPAREVALUE  0xD2
#define EVENT_DANGER        0xD1   /* EVENT_SLEEPIMPOSSIBLE */
#define EVENT_UNIDSTRING    0xD0
#define EVENT_ITEMIDSTRING  0xCF
#define EVENT_MEMBERPICNO   0xCE

/******************************************************************************
* DATA 			      						                                  *
******************************************************************************/

#endif /* _EVCODDEF_H_ */
