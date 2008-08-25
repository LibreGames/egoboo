#pragma once

//********************************************************************************************
//*
//*    This file is part of Egoboo.
//*
//*    Egoboo is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Egoboo is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

///
/// @file
/// @brief Global Type Definitions.
/// @details Defines basic types that are used throughout the game code.


#include "egoboo_config.h"

#include <SDL_endian.h>
#include <SDL_types.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#ifdef __cplusplus
  typedef bool bool_t;

  enum e_bool
  {
    btrue  = true,
    bfalse = false
  };
#else
  enum e_bool
  {
    btrue  = ( 1 == 1 ),
    bfalse = ( !btrue )
  };
  typedef enum e_bool bool_t;
#endif

enum e_retval
{
  rv_error    = -1,
  rv_fail     = bfalse,
  rv_succeed  = btrue,
  rv_waiting  = 2
};
typedef enum e_retval retval_t;

typedef char STRING[256];

#if defined(__cplusplus)
#    define EGOBOO_NEW( TYPE ) new TYPE
#    define EGOBOO_NEW_ARY( TYPE, COUNT ) new TYPE [ COUNT ]
#    define EGOBOO_DELETE(PTR) if(NULL != PTR) { delete PTR; PTR = NULL; }
#    define EGOBOO_DELETE_ARY(PTR) if(NULL != PTR) { delete [] PTR; PTR = NULL; }
#else
//#    if defined(_DEBUG)
#        define EGOBOO_NEW( TYPE ) (TYPE *)calloc(1, sizeof(TYPE))
#        define EGOBOO_NEW_ARY( TYPE, COUNT ) (TYPE *)calloc(COUNT, sizeof(TYPE))
#        define EGOBOO_DELETE(PTR) if(NULL != PTR) { free(PTR); PTR = NULL; }
#        define EGOBOO_DELETE_ARY(PTR) if(NULL != PTR) { free(PTR); PTR = NULL; }
//#    else
//#        define EGOBOO_NEW( TYPE ) (TYPE *)malloc( sizeof(TYPE) )
//#        define EGOBOO_NEW_ARY( TYPE, COUNT ) (TYPE *)malloc(COUNT * sizeof(TYPE))
//#    endif
#endif

enum e_color
{
  COLR_WHITE = 0,
  COLR_RED,
  COLR_YELLOW,
  COLR_GREEN,
  COLR_BLUE,
  COLR_PURPLE
};
typedef enum e_color COLR;


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct s_egoboo_key
{
  Uint32 v1, v2;
  bool_t dynamic;
  Uint32 data_type;
  void * pdata;
};
typedef struct s_egoboo_key egoboo_key_t;

#define EKEY_NEW(XX,YY) egoboo_key_new( &(XX.ekey), ekey_##YY, &(XX) )
#define EKEY_PNEW(XX,YY) egoboo_key_new( &(XX->ekey), ekey_##YY, XX )

#define EKEY_INVALIDATE(XX) egoboo_key_invalidate( &(XX.ekey) )
#define EKEY_PINVALIDATE(XX) egoboo_key_invalidate( &(XX->ekey) )

#define EKEY_VALID(XX) egoboo_key_valid(&(XX.ekey))
#define EKEY_PVALID(XX) ((NULL != (XX)) && egoboo_key_valid(&(XX->ekey)) )


enum e_ekey_list
{
  ekey_HashNode_t,
  ekey_Cap_t,
  ekey_Chr_t,
  ekey_Client_t,
  ekey_Enc_t,
  ekey_Eve_t,
  ekey_Game_t,
  ekey_Mad_t,
  ekey_Net_t,
  ekey_Pip_t,
  ekey_Prt_t,
  ekey_Player_t,
  ekey_Profile_t,
  ekey_Server_t,
  ekey_CHR_SPAWN_INFO,
  ekey_ENC_SPAWN_INFO,
  ekey_PRT_SPAWN_INFO,
  ekey_CListIn_Client_t,
  ekey_CListOut_Info_t,
  ekey_NetHost_t,
  ekey_PhysicsData_t,
  ekey_GameStack_t,
  ekey_Gui_t,
  ekey_ProcState_t,
  ekey_MachineState_t,
  ekey_ModState_t,
  ekey_nfile_ReceiveQueue_t,
  ekey_nfile_ReceiveState_t,
  ekey_nfile_SendState_t,
  ekey_nfile_SendQueue_t,
  ekey_NFileState_t,
  ekey_SoundState_t,
  ekey_MenuProc_t,
  ekey_ChrEnviro_t,
  ekey_Graphics_t,
  ekey_CHR_SETUP_INFO,
  ekey_Properties_t,
  ekey_Team_t,
  ekey_MeshMem_t,
  ekey_MOD_INFO,
  ekey_ModSummary_t,
  ekey_PacketRequest_t,
  ekey_NetThread_t,
  ekey_Passage_t,
  ekey_Shop_t,
  ekey_Status_t,
  ekey_CHR_SPAWN_QUEUE,
  ekey_AI_STATE,
  ekey_List_t,
  ekey_BUMPLIST,
  ekey_BSP_node_t,
  ekey_BSP_leaf_t,
  ekey_BSP_tree_t,
  ekey_ChrHeap_t,
  ekey_EncHeap_t,
  ekey_PrtHeap_t,
  ekey_GLtexture,
  ekey_BMFont_t,
  ekey_Graphics_Data_t,
  ekey_MeshInfo_t
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct s_rect_sint32
{
  Sint32 left;
  Sint32 right;
  Sint32 top;
  Sint32 bottom;
};
typedef struct s_rect_sint32 IRect_t;

struct s_rect_float
{
  float left;
  float right;
  float top;
  float bottom;
};
typedef struct s_rect_float FRect_t;


//--------------------------------------------------------------------------------------------
typedef Uint32 IDSZ;

#ifndef MAKE_IDSZ
#define MAKE_IDSZ(idsz) ((IDSZ)((((idsz)[0]-'A') << 15) | (((idsz)[1]-'A') << 10) | (((idsz)[2]-'A') << 5) | (((idsz)[3]-'A') << 0)))
#endif

#define IDSZ_NONE            MAKE_IDSZ("NONE")       // [NONE]

//--------------------------------------------------------------------------------------------
struct s_pair
{
  Sint32 ibase;
  Uint32 irand;
};
typedef struct s_pair PAIR;
//--------------------------------------------------------------------------------------------
struct s_range
{
  float ffrom, fto;
};
typedef struct s_range RANGE;

#ifdef __cplusplus

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define OBJLST_COUNT              1024
#define EVELST_COUNT              OBJLST_COUNT     ///<  One enchant type per model
#define ENCLST_COUNT          128                  ///<  Number of enchantments
#define CAPLST_COUNT              OBJLST_COUNT
#define CHRLST_COUNT              350              ///<  Max number of characters
#define MADLST_COUNT              OBJLST_COUNT     ///<  Max number of models
#define PIPLST_COUNT           1024                ///<  Particle templates
#define PRTLST_COUNT              512              ///<  Max number of particles

struct sCap;
struct sChr;
struct sPip;
struct sPrt;
struct sEve;
struct sEnc;
struct sTeam;
struct sPlayer;
struct sMad;
struct sProfile;

template <typename _ty, unsigned _sz> struct TPList;

/// @brief  Templated Typed List
/// @detail An abstraction of the existing type of list used in Egoboo. With the templated
///  "pointer type" template <typename _ty, unsigned _sz> struct TPList, it creates a system
///  where someone cannot accidentally access a character profile when they are looking for
///  a model...
template <typename _ty, unsigned _sz>
struct TList
{
  struct Handle
  {
    typedef size_t htype;

    size_t val;
    explicit Handle(size_t v = _sz) { val = v; };

    bool operator == (htype v) { return val == v; };
    bool operator != (htype v) { return val != v; };
    bool operator >= (htype v) { return val >= v; };
    bool operator <= (htype v) { return val <= v; };
    bool operator >  (htype v) { return val > v; };
    bool operator <  (htype v) { return val < v; };

    bool operator == (Handle v) { return val == v.val; };
    bool operator != (Handle v) { return val != v.val; };
    bool operator >= (Handle v) { return val >= v.val; };
    bool operator <= (Handle v) { return val <= v.val; };
    bool operator >  (Handle v) { return val >  v.val; };
    bool operator <  (Handle v) { return val <  v.val; };

    Handle & operator = (htype  v) { val = v; return *this; };
    Handle & operator = (Handle v) { val = v.val; return *this; };

    Handle & operator ++ () { ++val; return *this; }
    Handle & operator -- () { --val; return *this; }

    Handle & operator ++ (int) { val++; return *this; }
    Handle & operator -- (int) { val--; return *this; }
  };

  static Handle INVALID;

  size_t sz;
  _ty  * data;

  TList() { sz = _sz; data = EGOBOO_NEW_ARY(_ty, sz); }
  ~TList() { free(data); }

  _ty & operator [] (Handle i) { return data[i.val]; }
  _ty * operator +  (Handle i) { return data + i.val; }

  operator TPList<_ty, _sz> () { TPList<_ty, _sz> tmp; tmp.plist = (TPList<_ty, _sz>::myplist)this; return tmp; }
};

/// @brief Templated Typed List "Pointer"
/// @detail This is actually an abstraction for the type of integer references
///  that have always been in use in Egoboo. However, this allows for strict type
///  checking in C++, to make sure we are getting what we want.
template <typename _ty, unsigned _sz>
struct TPList
{
  typedef typename TList<_ty, _sz>::Handle myref;
  typedef typename TList<_ty, _sz>      mylist;
  typedef typename TList<_ty, _sz>    * myplist;

  myplist plist;

  _ty & operator [] (typename myref i) { return (*plist)[i]; };
  _ty * operator +  (typename myref i) { return (*plist) + i; };

  operator typename myplist ()  { return plist; };
  operator typename mylist & () { return *plist; };
};

//-----------------------------------------

#define CAP_REF TList<sCap, CAPLST_COUNT>::Handle
#define CHR_REF TList<sChr, CHRLST_COUNT>::Handle

#define PIP_REF TList<sPip, PIPLST_COUNT>::Handle
#define PRT_REF TList<sPrt, PRTLST_COUNT>::Handle

#define EVE_REF TList<sEve, EVELST_COUNT>::Handle
#define ENC_REF TList<sEnc, ENCLST_COUNT>::Handle

#define TEAM_REF TList<sTeam, TEAM_COUNT>::Handle
#define PLA_REF  TList<sPlayer, PLALST_COUNT>::Handle

#define MAD_REF TList<sMad, MADLST_COUNT>::Handle

#define OBJ_REF TList<sProfile, OBJLST_COUNT>::Handle

typedef Uint16 AI_REF;
typedef Uint16 PASS_REF;
typedef Uint16 SHOP_REF;

#define INVALID_CAP TList<sCap, CAPLST_COUNT>::INVALID
#define INVALID_CHR TList<sChr, CHRLST_COUNT>::INVALID

#define INVALID_PIP TList<sPip, PIPLST_COUNT>::INVALID
#define INVALID_PRT TList<sPrt, PRTLST_COUNT>::INVALID

#define INVALID_EVE TList<sEve, EVELST_COUNT>::INVALID
#define INVALID_ENC TList<sEnc, ENCLST_COUNT>::INVALID

#define INVALID_TEAM TList<sTeam, TEAM_COUNT>::INVALID
#define INVALID_PLA  TList<sPlayer, PLALST_COUNT>::INVALID

#define INVALID_MAD TList<sMad, MADLST_COUNT>::INVALID

#define INVALID_OBJ TList<sProfile, OBJLST_COUNT>::INVALID

#define INVALID_AI  AILST_COUNT

#define REF_TO_INT(XX) XX.val

#else

//typedef struct CList_t
//{
//  egoboo_key_t ekey;
//  size_t     count;
//  size_t     size;
//  void     * data;
//} CList;



typedef Uint16 CAP_REF;
typedef Uint16 CHR_REF;

typedef Uint16 PIP_REF;
typedef Uint16 PRT_REF;

typedef Uint16 EVE_REF;
typedef Uint16 ENC_REF;

typedef Uint16 TEAM_REF;
typedef Uint16 PLA_REF;

typedef Uint16 MAD_REF;
typedef Uint16 AI_REF;
typedef Uint16 OBJ_REF;

typedef Uint16 PASS_REF;
typedef Uint16 SHOP_REF;


#define INVALID_CAP CAPLST_COUNT
#define INVALID_CHR CHRLST_COUNT

#define INVALID_PIP PIPLST_COUNT
#define INVALID_PRT PRTLST_COUNT

#define INVALID_EVE EVELST_COUNT
#define INVALID_ENC ENCLST_COUNT

#define INVALID_TEAM TEAM_COUNT
#define INVALID_PLA PLALST_COUNT

#define INVALID_MAD MADLST_COUNT
#define INVALID_AI  AILST_COUNT
#define INVALID_OBJ OBJLST_COUNT
#define INVALID_PASS PASSLST_COUNT

#define CAP_REF(XX) (CAP_REF)(XX)
#define CHR_REF(XX) (CHR_REF)(XX)

#define PIP_REF(XX) (PIP_REF)(XX)
#define PRT_REF(XX) (PRT_REF)(XX)

#define EVE_REF(XX) (EVE_REF)(XX)
#define ENC_REF(XX) (ENC_REF)(XX)

#define TEAM_REF(XX) (TEAM_REF)(XX)
#define PLA_REF(XX) (PLA_REF)(XX)

#define MAD_REF(XX) (MAD_REF)(XX)
#define AI_REF(XX) (AI_REF)(XX)
#define OBJ_REF(XX) (OBJ_REF)(XX)

#define PASS_REF(XX) (PASS_REF)(XX)
#define SHOP_REF(XX) (SHOP_REF)(XX)

#define REF_TO_INT(XX) XX
#endif



//--------------------------------------------------------------------------------------------
union u_float_int_convert
{
  float f;
  Uint32 i;
};

typedef union u_float_int_convert FCONVERT;

//--------------------------------------------------------------------------------------------
enum e_ProcessStates
{
  PROC_Begin,
  PROC_Entering,
  PROC_Running,
  PROC_Leaving,
  PROC_Finish
};

typedef enum e_ProcessStates ProcessStates;

//--------------------------------------------------------------------------------------------
struct sClockState;

struct sProcState
{
  egoboo_key_t ekey;

  // process variables
  ProcessStates State;              ///< what are we doing now?
  bool_t        Active;             ///< Keep looping or quit?
  bool_t        Paused;             ///< Is it paused?
  bool_t        KillMe;             ///< someone requested that we terminate!
  bool_t        Terminated;         ///< We are completely done.

  // each process has its own clock
  struct sClockState * clk;

  int           returnValue;
};
typedef struct sProcState ProcState_t;

ProcState_t * ProcState_new(ProcState_t * ps);
bool_t        ProcState_delete(ProcState_t * ps);
ProcState_t * ProcState_renew(ProcState_t * ps);
bool_t        ProcState_init(ProcState_t * ps);




enum e_respawn_mode
{
  RESPAWN_NONE = 0,
  RESPAWN_NORMAL,
  RESPAWN_ANYTIME
};
typedef enum e_respawn_mode RESPAWN_MODE;

typedef int (SDLCALL *SDL_Callback_Ptr)(void *);


// a hash type for "efficiently" storing data
struct sHashNode
{
  egoboo_key_t ekey;
  struct sHashNode * next;
  void * data;
};
typedef struct sHashNode HashNode_t;


struct sHashList
{
  int         allocated;
  int      *  subcount;
  HashNode_t ** sublist;
};
typedef struct sHashList HashList_t;




struct sBSP_node
{
  egoboo_key_t ekey;

  struct sBSP_node * next;
  int                 data_type;
  void              * data;
};
typedef struct sBSP_node BSP_node_t;



struct sBSP_leaf
{
  egoboo_key_t ekey;

  struct sBSP_leaf  * parent;
  size_t               child_count;
  struct sBSP_leaf ** children;
  BSP_node_t           * nodes;
};
typedef struct sBSP_leaf BSP_leaf_t;




struct sBSP_tree
{
  egoboo_key_t ekey;

  int dimensions;
  int depth;

  size_t       leaf_count;
  BSP_leaf_t * leaf_list;

  BSP_leaf_t * root;
};
typedef struct sBSP_tree BSP_tree_t;



#define PROFILE_PROTOTYPE(XX) struct sClockState; extern struct sClockState * clkstate_##XX; extern double clkcount_##XX; extern double clktime_##XX;

PROFILE_PROTOTYPE(ekey)
