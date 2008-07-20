#pragma once

#include "object.h"

#include "egoboo_math.h"
#include "egoboo.h"

struct CGame_t;

#define MADLST_COUNT                        OBJLST_COUNT   // Max number of models
//#define MD2START                        0x32504449  // MD2 files start with these four bytes
#define MD2MAXLOADSIZE                  (512*1024)   // Don't load any models bigger than 512k
#define EQUALLIGHTINDEX                 162          // I added an extra index to do the spikey mace...
#define MD2LIGHTINDICES                 163          // MD2's store vertices as x,y,z,normal

#define MAXLIGHTROTATION                256         // Number of premade light maps
#define MAXSPEKLEVEL                    16          // Number of premade specularities


extern float           spek_global[MAXLIGHTROTATION][MD2LIGHTINDICES];
extern float           spek_local[MAXLIGHTROTATION][MD2LIGHTINDICES];
extern float           indextoenvirox[MD2LIGHTINDICES];                    // Environment map
extern float           lighttoenviroy[256];                                // Environment map
extern Uint32          lighttospek[MAXSPEKLEVEL][256];                     //

// This stuff is for actions
typedef enum Action_e
{
  ACTION_DA = 0,        // :DA Dance ( Standing still )
  ACTION_DB,            // :DB Dance ( Bored )
  ACTION_DC,            // :DC Dance ( Bored )
  ACTION_DD,            // :DD Dance ( Bored )
  ACTION_UA,            // :UA Unarmed
  ACTION_UB,            // :UB Unarmed
  ACTION_UC,            // :UC Unarmed
  ACTION_UD,            // :UD Unarmed
  ACTION_TA,            // :TA Thrust
  ACTION_TB,            // :TB Thrust
  ACTION_TC,            // :TC Thrust
  ACTION_TD,            // :TD Thrust
  ACTION_CA,            // :CA Crush
  ACTION_CB,            // :CB Crush
  ACTION_CC,            // :CC Crush
  ACTION_CD,            // :CD Crush
  ACTION_SA,            // :SA Slash
  ACTION_SB,            // :SB Slash
  ACTION_SC,            // :SC Slash
  ACTION_SD,            // :SD Slash
  ACTION_BA,            // :BA Bash
  ACTION_BB,            // :BB Bash
  ACTION_BC,            // :BC Bash
  ACTION_BD,            // :BD Bash
  ACTION_LA,            // :LA Longbow
  ACTION_LB,            // :LB Longbow
  ACTION_LC,            // :LC Longbow
  ACTION_LD,            // :LD Longbow
  ACTION_XA,            // :XA Crossbow
  ACTION_XB,            // :XB Crossbow
  ACTION_XC,            // :XC Crossbow
  ACTION_XD,            // :XD Crossbow
  ACTION_FA,            // :FA Flinged
  ACTION_FB,            // :FB Flinged
  ACTION_FC,            // :FC Flinged
  ACTION_FD,            // :FD Flinged
  ACTION_PA,            // :PA Parry
  ACTION_PB,            // :PB Parry
  ACTION_PC,            // :PC Parry
  ACTION_PD,            // :PD Parry
  ACTION_EA,            // :EA Evade
  ACTION_EB,            // :EB Evade
  ACTION_RA,            // :RA Roll
  ACTION_ZA,            // :ZA Zap Magic
  ACTION_ZB,            // :ZB Zap Magic
  ACTION_ZC,            // :ZC Zap Magic
  ACTION_ZD,            // :ZD Zap Magic
  ACTION_WA,            // :WA Sneak
  ACTION_WB,            // :WB Walk
  ACTION_WC,            // :WC Run
  ACTION_WD,            // :WD Push
  ACTION_JA,            // :JA Jump
  ACTION_JB,            // :JB Jump ( Falling ) ( Drop left )
  ACTION_JC,            // :JC Jump ( Falling ) ( Drop right )
  ACTION_HA,            // :HA Hit ( Taking damage )
  ACTION_HB,            // :HB Hit ( Taking damage )
  ACTION_HC,            // :HC Hit ( Taking damage )
  ACTION_HD,            // :HD Hit ( Taking damage )
  ACTION_KA,            // :KA Killed
  ACTION_KB,            // :KB Killed
  ACTION_KC,            // :KC Killed
  ACTION_KD,            // :KD Killed
  ACTION_MA,            // :MA Drop Item Left
  ACTION_MB,            // :MB Drop Item Right
  ACTION_MC,            // :MC Cheer
  ACTION_MD,            // :MD Show Off
  ACTION_ME,            // :ME Grab Item Left
  ACTION_MF,            // :MF Grab Item Right
  ACTION_MG,            // :MG Open Chest
  ACTION_MH,            // :MH Sit ( Not implemented )
  ACTION_MI,            // :MI Ride
  ACTION_MJ,            // :MJ Activated ( For items )
  ACTION_MK,            // :MK Snoozing
  ACTION_ML,            // :ML Unlock
  ACTION_MM,            // :MM Held Left
  ACTION_MN,            // :MN Held Right
  MAXACTION,            //                      // Number of action types

  ACTION_ST = ACTION_DA,

  ACTION_INVALID        = 0xffff                // Action not valid for this character
} ACTION;

extern char cActionName[MAXACTION][2];                  // Two letter name code

typedef enum mad_effects_bits_e
{
  MADFX_INVICTUS       = 1 <<  0,                    // I  Invincible
  MADFX_ACTLEFT        = 1 <<  1,                    // AL Activate left item
  MADFX_ACTRIGHT       = 1 <<  2,                    // AR Activate right item
  MADFX_GRABLEFT       = 1 <<  3,                    // GL GO Grab left/Grab only item
  MADFX_GRABRIGHT      = 1 <<  4,                    // GR Grab right item
  MADFX_DROPLEFT       = 1 <<  5,                    // DL DO Drop left/Drop only item
  MADFX_DROPRIGHT      = 1 <<  6,                    // DR Drop right item
  MADFX_STOP           = 1 <<  7,                    // S  Stop movement
  MADFX_FOOTFALL       = 1 <<  8,                    // F  Footfall sound
  MADFX_CHARLEFT       = 1 <<  9,                    // CL Grab left/Grab only character
  MADFX_CHARRIGHT      = 1 << 10,                    // CR Grab right character
  MADFX_POOF           = 1 << 11                     // P  Poof
} MADFX_BITS;

typedef enum lip_transition_e
{
  LIPT_DA = 0,                                  // For smooth transitions 'tween
  LIPT_WA,                                      //   walking rates
  LIPT_WB,                                      //
  LIPT_WC,                                      //
  LIPT_COUNT
} LIPT;


//------------------------------------
//Model stuff
//------------------------------------

#define MAXFRAMESPERANIM 16

typedef struct CMad_t
{
  egoboo_key      ekey;
  bool_t          Loaded;

  // debugging
  STRING          name;

  MD2_Model *     md2_ptr;                          // Md2 model pointer
  Uint16          vertices;                      // Number of vertices
  Uint16          transvertices;                 // Number to transform
  Uint8  *        framelip;                      // 0-15, How far into action is each frame
  Uint16 *        framefx;                       // Invincibility, Spawning
  Uint16          frameliptowalkframe[LIPT_COUNT][MAXFRAMESPERANIM];    // For walk animations
  bool_t          actionvalid[MAXACTION];        // bfalse if not valid
  Uint16          actionstart[MAXACTION];        // First frame of anim
  Uint16          actionend[MAXACTION];          // One past last frame

  int             bbox_frames;
  BBOX_ARY *      bbox_arrays;
} CMad;

#ifdef __cplusplus
  typedef TList<CMad_t, MADLST_COUNT> MadList_t;
  typedef TPList<CMad_t, MADLST_COUNT> PMad;
#else
  typedef CMad MadList_t[MADLST_COUNT];
  typedef CMad * PMad;
#endif

CMad *  Mad_new(CMad * pmad);
bool_t  Mad_delete(CMad *pmad);
CMad *  Mad_renew(CMad * pmad);

void   MadList_free_one( struct CGame_t * gs, Uint16 imdl );

#define VALID_MAD_RANGE(XX) ( ((XX)>=0) && ((XX)<MADLST_COUNT) )
#define VALID_MAD(LST, XX)   ( VALID_MAD_RANGE(XX) && EKEY_VALID(LST[XX]) )
#define VALIDATE_MAD(LST,XX) ( VALID_MAD(LST, XX) ? (XX) : (INVALID_MAD) )
#define LOADED_MAD(LST, XX)  ( VALID_MAD(LST, XX) && LST[XX].Loaded )

ACTION action_number(char * szName);
Uint16 action_frame();
void   action_copy_correct( struct CGame_t * gs, MAD_REF object, ACTION actiona, ACTION actionb );
void   get_walk_frame( struct CGame_t * gs, MAD_REF object, LIPT lip_trans, ACTION action );
Uint16 get_framefx( char * szName );
void   make_framelip( struct CGame_t * gs, MAD_REF object, ACTION action );
void   get_actions( struct CGame_t * gs, MAD_REF object );
void   make_mad_equally_lit( struct CGame_t * gs, MAD_REF model );

bool_t mad_generate_bbox_tree(int max_level, CMad * pmad);


MAD_REF MadList_load_one( struct CGame_t * gs, const char * szModpath, const char * szObjectname, MAD_REF irequest );

bool_t mad_display_bbox_tree(int level, matrix_4x4 matrix, CMad * pmad, int frame1, int frame2);
void load_copy_file( struct CGame_t * gs, const char * szModpath, const char * szObjectname, MAD_REF object );

void ObjList_log_used( struct CGame_t * gs, char *savename );
