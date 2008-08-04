/* Egoboo - egobootypedef.h
 * Defines some basic types that are used throughout the game code.
 */

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

#pragma once

#include "egoboo_config.h"

#include <SDL_endian.h>
#include <SDL_types.h>

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#ifdef __cplusplus
  typedef bool bool_t;

  enum bool_e
  {
    btrue  = true,
    bfalse = false
  };
#else
  enum bool_e
  {
    btrue  = ( 1 == 1 ),
    bfalse = ( !btrue )
  };
  typedef enum bool_e bool_t;
#endif

enum retval_e
{
  rv_error    = -1,
  rv_fail     = bfalse,
  rv_succeed  = btrue,
  rv_waiting  = 2
};
typedef enum retval_e retval_t;

typedef char STRING[256];

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct egoboo_key_t
{
  Uint32 v1, v2;
  bool_t dynamic;
  Uint32 data_type;
  void * pdata;
};
typedef struct egoboo_key_t egoboo_key;

INLINE egoboo_key * egoboo_key_create(Uint32 itype, void * pdata);
INLINE bool_t       egoboo_key_destroy(egoboo_key ** pkey);
INLINE egoboo_key * egoboo_key_new(egoboo_key * pkey, Uint32 itype, void * pdata);
INLINE bool_t       egoboo_key_validate(egoboo_key * pkey);
INLINE bool_t       egoboo_key_invalidate(egoboo_key * pkey);
INLINE bool_t       egoboo_key_valid(egoboo_key * pkey);
INLINE void *       egoboo_key_get_data(egoboo_key * pkey, Uint32 type);

#define EKEY_NEW(XX,YY) egoboo_key_new( &(XX.ekey), ekey_##YY, &(XX) )
#define EKEY_PNEW(XX,YY) egoboo_key_new( &(XX->ekey), ekey_##YY, XX )

#define EKEY_INVALIDATE(XX) egoboo_key_invalidate( &(XX.ekey) )
#define EKEY_PINVALIDATE(XX) egoboo_key_invalidate( &(XX->ekey) )

#define EKEY_VALID(XX) egoboo_key_valid(&(XX.ekey))
#define EKEY_PVALID(XX) ((NULL != (XX)) && egoboo_key_valid(&(XX->ekey)) )


enum
{
  ekey_HashNode,
  ekey_CCap,
  ekey_CChr,
  ekey_CClient,
  ekey_CEnc,
  ekey_CEve,
  ekey_CGame,
  ekey_CMad,
  ekey_CNet,
  ekey_CPip,
  ekey_CPrt,
  ekey_CPlayer,
  ekey_CProfile,
  ekey_CServer,
  ekey_chr_spawn_info,
  ekey_enc_spawn_info,
  ekey_prt_spawn_info,
  ekey_CListIn_Client,
  ekey_CListOut_Info,
  ekey_NetHost,
  ekey_CPhysicsData,
  ekey_GSStack,
  ekey_CGui,
  ekey_ProcState,
  ekey_MachineState,
  ekey_ModState,
  ekey_nfile_ReceiveQueue,
  ekey_nfile_ReceiveState,
  ekey_nfile_SendState,
  ekey_nfile_SendQueue,
  ekey_NFileState,
  ekey_SoundState,
  ekey_MenuProc,
  ekey_CChrEnviro,
  ekey_CGraphics,
  ekey_chr_setup_info,
  ekey_CProperties,
  ekey_CTeam,
  ekey_MeshMem,
  ekey_MOD_INFO,
  ekey_ModSummary,
  ekey_NetRequest,
  ekey_NetThread,
  ekey_Passage,
  ekey_Shop,
  ekey_Status,
  ekey_chr_spawn_queue,
  ekey_AI_STATE,
  ekey_CList,
  ekey_BUMPLIST,
  ekey_BSP_node,
  ekey_BSP_leaf,
  ekey_BSP_tree
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct rect_sint32_t
{
  Sint32 left;
  Sint32 right;
  Sint32 top;
  Sint32 bottom;
};
typedef struct rect_sint32_t IRect;

struct rect_float_t
{
  float left;
  float right;
  float top;
  float bottom;
};
typedef struct rect_float_t FRect;


//--------------------------------------------------------------------------------------------
typedef Uint32 IDSZ;

#ifndef MAKE_IDSZ
#define MAKE_IDSZ(idsz) ((IDSZ)((((idsz)[0]-'A') << 15) | (((idsz)[1]-'A') << 10) | (((idsz)[2]-'A') << 5) | (((idsz)[3]-'A') << 0)))
#endif


//--------------------------------------------------------------------------------------------
struct pair_t
{
  Sint32 ibase;
  Uint32 irand;
};
typedef struct pair_t PAIR;
//--------------------------------------------------------------------------------------------
struct range_t
{
  float ffrom, fto;
};
typedef struct range_t RANGE;

#ifdef __cplusplus

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
#define OBJLST_COUNT              1024
#define EVELST_COUNT              OBJLST_COUNT  // One enchant type per model
#define ENCLST_COUNT          128         // Number of enchantments
#define CAPLST_COUNT              OBJLST_COUNT
#define CHRLST_COUNT              350         // Max number of characters
#define MADLST_COUNT              OBJLST_COUNT   // Max number of models
#define PIPLST_COUNT           1024                    // Particle templates
#define PRTLST_COUNT              512         // Max number of particles

struct CCap_t;
struct CChr_t;
struct CPip_t;
struct CPrt_t;
struct CEve_t;
struct CEnc_t;
struct CTeam_t;
struct CPlayer_t;
struct CMad_t;
struct CProfile_t;

template <typename _ty, unsigned _sz> struct TPList;

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

  TList() { sz = _sz; data = calloc(sz, sizeof(_ty)); }
  ~TList() { free(data); }

  _ty & operator [] (Handle i) { return data[i.val]; }
  _ty * operator +  (Handle i) { return data + i.val; }

  operator TPList<_ty, _sz> () { TPList<_ty, _sz> tmp; tmp.plist = (TPList<_ty, _sz>::myplist)this; return tmp; }
};

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

#define CAP_REF TList<CCap_t, CAPLST_COUNT>::Handle
#define CHR_REF TList<CChr_t, CHRLST_COUNT>::Handle

#define PIP_REF TList<CPip_t, PIPLST_COUNT>::Handle
#define PRT_REF TList<CPrt_t, PRTLST_COUNT>::Handle

#define EVE_REF TList<CEve_t, EVELST_COUNT>::Handle
#define ENC_REF TList<CEnc_t, ENCLST_COUNT>::Handle

#define TEAM_REF TList<CTeam_t, TEAM_COUNT>::Handle
#define PLA_REF  TList<CPlayer_t, PLALST_COUNT>::Handle

#define MAD_REF TList<CMad_t, MADLST_COUNT>::Handle

#define OBJ_REF TList<CProfile_t, OBJLST_COUNT>::Handle

typedef Uint16 AI_REF;
typedef Uint16 PASS_REF;
typedef Uint16 SHOP_REF;

#define INVALID_CAP TList<CCap_t, CAPLST_COUNT>::INVALID
#define INVALID_CHR TList<CChr_t, CHRLST_COUNT>::INVALID

#define INVALID_PIP TList<CPip_t, PIPLST_COUNT>::INVALID
#define INVALID_PRT TList<CPrt_t, PRTLST_COUNT>::INVALID

#define INVALID_EVE TList<CEve_t, EVELST_COUNT>::INVALID
#define INVALID_ENC TList<CEnc_t, ENCLST_COUNT>::INVALID

#define INVALID_TEAM TList<CTeam_t, TEAM_COUNT>::INVALID
#define INVALID_PLA  TList<CPlayer_t, PLALST_COUNT>::INVALID

#define INVALID_MAD TList<CMad_t, MADLST_COUNT>::INVALID

#define INVALID_OBJ TList<CProfile_t, OBJLST_COUNT>::INVALID

#define INVALID_AI  AILST_COUNT

#define REF_TO_INT(XX) XX.val

#else

//typedef struct CList_t
//{
//  egoboo_key ekey;
//  size_t     count;
//  size_t     size;
//  void     * data;
//} CList;
//
//INLINE CList * CList_new(CList * lst, size_t count, size_t size);
//INLINE bool_t  CList_delete(CList * lst);
//INLINE void  * CList_getData(CList * lst, int index);


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
union float_int_convert_u 
{ 
  float f; 
  Uint32 i; 
};

typedef union float_int_convert_u FCONVERT;

#if SDL_BYTEORDER != SDL_LIL_ENDIAN
    INLINE float SwapLE_float( float val );
#endif

//--------------------------------------------------------------------------------------------
enum ProcessStates_e
{
  PROC_Begin,
  PROC_Entering,
  PROC_Running,
  PROC_Leaving,
  PROC_Finish,
};

typedef enum ProcessStates_e ProcessStates;

//--------------------------------------------------------------------------------------------
struct ClockState_t;

struct ProcState_t
{
  egoboo_key ekey;

  // process variables
  ProcessStates State;              // what are we doing now?
  bool_t        Active;             // Keep looping or quit?
  bool_t        Paused;             // Is it paused?
  bool_t        KillMe;             // someone requested that we terminate!
  bool_t        Terminated;         // We are completely done.

  // each process has its own clock
  struct ClockState_t * clk;

  int           returnValue;
};
typedef struct ProcState_t ProcState;

ProcState * ProcState_new(ProcState * ps);
bool_t      ProcState_delete(ProcState * ps);
ProcState * ProcState_renew(ProcState * ps);
bool_t      ProcState_init(ProcState * ps);




enum respawn_mode_e
{
  RESPAWN_NONE = 0,
  RESPAWN_NORMAL,
  RESPAWN_ANYTIME
};
typedef enum respawn_mode_e RESPAWN_MODE;

typedef int (SDLCALL *SDL_Callback_Ptr)(void *);


// a hash type for "efficiently" storing data
struct HashNode_t
{
  egoboo_key ekey;
  struct HashNode_t * next;
  void * data;
};
typedef struct HashNode_t HashNode;

INLINE HashNode * HashNode_create(void * data);
INLINE bool_t          HashNode_destroy(HashNode **);
INLINE HashNode * HashNode_insert_after (HashNode lst[], HashNode * n);
INLINE HashNode * HashNode_insert_before(HashNode lst[], HashNode * n);
INLINE HashNode * HashNode_remove_after (HashNode lst[]);
INLINE HashNode * HashNode_remove       (HashNode lst[]);

struct HashList_t
{
  int         allocated;
  int      *  subcount;
  HashNode ** sublist;
};
typedef struct HashList_t HashList;

INLINE HashList * HashList_create(int size);
INLINE bool_t     HashList_destroy(HashList **);


struct BSP_node_t
{
  egoboo_key ekey;

  struct BSP_node_t * next;
  int                 data_type;
  void              * data;
};
typedef struct BSP_node_t BSP_node;

INLINE BSP_node * BSP_node_new( BSP_node * t, void * data, int type );
INLINE bool_t     BSP_node_delete( BSP_node * t );

struct BSP_leaf_t
{
  egoboo_key ekey;

  struct BSP_leaf_t  * parent;
  size_t               child_count;
  struct BSP_leaf_t ** children;
  BSP_node           * nodes;
};
typedef struct BSP_leaf_t BSP_leaf;

INLINE BSP_leaf * BSP_leaf_new( BSP_leaf * L, int size );
INLINE bool_t     BSP_leaf_delete( BSP_leaf * L );
INLINE bool_t     BSP_leaf_insert( BSP_leaf * L, BSP_node * n );


struct BSP_tree_t
{
  egoboo_key ekey;

  int dimensions;
  int depth;

  int        leaf_count;
  BSP_leaf * leaf_list;

  BSP_leaf * root;
};
typedef struct BSP_tree_t BSP_tree;

INLINE BSP_tree * BSP_tree_new( BSP_tree * t, Sint32 dim, Sint32 depth);
INLINE bool_t     BSP_tree_delete( BSP_tree * t );

INLINE Sint32 BSP_tree_count_nodes(Sint32 dim, Sint32 depth);
