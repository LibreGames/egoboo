/******************************************************************************
*    SCRIPT.H                                                                 *
*        Header file for the SCRIPT.C                                         *
*                                                                             *
*    Initial version                                                          *
*        October 2010 / bitnapper                                             *
******************************************************************************/

#ifndef _SCRIPT_H_
#define _SCRIPT_H_

/******************************************************************************
* DEFINES                                                                     *
******************************************************************************/

#define SC_MAXNAMELEN	 31	

/* ----- Argument-Formats ----- */
#define SC_INVALID	    0x00   	
#define SC_CHAR	        0x01    
#define SC_SMALLINT	    0x02   
#define SC_STRING	    0x03   
#define SC_VARARG       0x04 

/* ----- Codenumbers ------- */
#define SC_CODESUB	    ((char)0x01)
#define SC_CODECONST	((char)0x02)
#define SC_CODEVARARGS  ((char)0xCA)
#define SC_CODEBREAK    ((char)0xCB)
#define SC_CODESELECT	((char)0xCC)
#define SC_CODECASE	    ((char)0xCD)
#define SC_CODEJUMP     ((char)0xF2)
#define SC_CODEEND	    ((char)0xF1)
#define SC_CODERETURN   ((char)0xF0)
#define SC_CODECALL     ((char)0xEF)
#define SC_CODEIF	    ((char)0xEE)
#define SC_CODEAND      ((char)0xF9)
#define SC_CODEOR	    ((char)0xF8)
#define SC_CODETHEN     ((char)0xEE)
/* ------------- Comparision codes ------- */
#define SC_COMPAREEQUAL     ((char)0xFF)
#define SC_COMPARENOTEQUAL  ((char)0xFE)
#define SC_COMPARELESS      ((char)0xFD)
#define SC_COMPARELESSEQUAL ((char)0xFC)
#define SC_COMPAREGREATER   ((char)0xFB)
#define SC_COMPAREGREATEREQUAL  ((char)0xFA)
#define SC_COMPAREAND       ((char)0xF9)
#define SC_COMPAREOR        ((char)0xF8)

/* ---- Subcodes -- - */
#define SC_VALUE	 ((char)0xE2)
#define SC_VALSHORT	 ((char)0xCC)
#define SC_VALINT	 ((char)0xCB)
#define SC_VALSTRING ((char)0xCA)

/******************************************************************************
* TYPEDEFS                                                                    *
******************************************************************************/

typedef struct {
    char len;			    /* Length of keyword/operator to check  */
    char keyword[8];		/* keyword to check			            */
    char code;		        /* Code to generate, if any		        */
} SC_TOKEN;

typedef struct {
    char len;				        /* Length of procname to check	        */
    char name[SC_MAXNAMELEN + 1];	/* Procname to check		            */
    char code;			            /* Code to generate, if any	            */
    char sub_code;                  /* != 0: Second code to generate        */
    char argcount;			        /* Number of arguments to encode        */
    char args[12];		            /* Array with kind of args to encode    */
} SC_PROC;

typedef struct {
    char type;                           /* if 0, its the end of the array    */
    char value;                          /* The value of the constant	     */
    char name[SC_MAXNAMELEN + 1];
} SC_CONSTANT;

typedef struct {
    char code;
    char name[SC_MAXNAMELEN + 4];       /* Name of variable 	       	     */
    int  firstconst;			        /* Number of first constant in the   */
                                        /* constant list that are allowed    */
                                        /* to replace this variable/argument */
    int  constcount;			        /* Number of allowed constants 	     */
} SC_VARIABLE;

/******************************************************************************
* DATA	                                                                      *
******************************************************************************/

/*
// extern ScriptFixProc StandardProc[];
extern SC_TOKEN    Conditions[];
extern SC_TOKEN    CatConditions[];
extern SC_TOKEN    Keywords[];
extern SC_CONSTANT StdConstant[];
extern SC_VARIABLE StdVariable[];
extern SC_PROC     StdScriptProc[];
extern SC_PROC     StdScriptFunc[];
*/


/******************************************************************************
* CODE	                                                                      *
******************************************************************************/

int  encodeMain(char *filename);	// ScriptEncode_Main(char *filename);

#endif /* _SCRIPT_H_  */
