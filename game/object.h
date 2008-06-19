#pragma once

#include "egoboo_types.h"
#include "egoboo_math.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

#define OBJLST_COUNT 1024

struct CGame_t;
struct CChr_t;
struct CPrt_t;

struct Mix_Chunk;

#define MAXSECTION                      4           // T-wi-n-k...  Most of 4 sections
#define MAXWAVE         16                            // Up to 16 waves per model


typedef enum prtpip_t
{
  PRTPIP_COIN_001 = 0,                  // Coins are the first particles loaded
  PRTPIP_COIN_005,                      //
  PRTPIP_COIN_025,                      //
  PRTPIP_COIN_100,                      //
  PRTPIP_WEATHER_1,                     // Weather particles
  PRTPIP_WEATHER_2,                     // Weather particle finish
  PRTPIP_SPLASH,                        // Water effects are next
  PRTPIP_RIPPLE,                        //
  PRTPIP_DEFEND,                        // Defend particle
  PRTPIP_PEROBJECT_COUNT                //
} PRTPIP;

//--------------------------------------------------------------------------------------------
typedef struct CProfile_t
{
  bool_t          initialized;
  bool_t          used;                          // is it loaded?

  // debug info
  STRING          name;                          // Model name

  // message stuff
  Uint16          msg_start;                      // The first message

  // skin stuff
  Uint16          skins;                         // Number of skins
  Uint16          skinstart;                     // Starting skin of model

  // sound stuff
  struct Mix_Chunk *     wavelist[MAXWAVE];             //sounds in a object

  // naming stuff
  Uint16          sectionsize[MAXSECTION];       // Number of choices, 0
  Uint16          sectionstart[MAXSECTION];      //

  EVE_REF         eve;
  CAP_REF         cap;
  MAD_REF         mad;
  AI_REF          ai;                              // AI for this model
  PIP_REF         prtpip[PRTPIP_PEROBJECT_COUNT];  // Local particles

} CProfile;

typedef struct CProfile_t CObj;

#ifdef __cplusplus
  typedef TList<CProfile_t, OBJLST_COUNT> ObjList_t;
  typedef TPList<CProfile_t, OBJLST_COUNT> PObj;
#else
  typedef CObj ObjList_t[OBJLST_COUNT];
  typedef CObj * PObj;
#endif

CProfile * CProfile_new(CProfile *);
bool_t     CProfile_delete(CProfile *);
CProfile * CProfile_renew(CProfile *);


bool_t  ObjList_new   ( struct CGame_t * gs );
bool_t  ObjList_delete( struct CGame_t * gs );
bool_t  ObjList_renew ( struct CGame_t * gs );
OBJ_REF ObjList_get_free(  struct CGame_t * gs, OBJ_REF request );

       CObj   * ObjList_getPObj(struct CGame_t * gs, OBJ_REF iobj);
struct CEve_t * ObjList_getPEve(struct CGame_t * gs, OBJ_REF iobj);
struct CCap_t * ObjList_getPCap(struct CGame_t * gs, OBJ_REF iobj);
struct CMad_t * ObjList_getPMad(struct CGame_t * gs, OBJ_REF iobj);
struct CPip_t * ObjList_getPPip(struct CGame_t * gs, OBJ_REF iobj, int i);

EVE_REF ObjList_getREve(struct CGame_t * gs, OBJ_REF obj);
CAP_REF ObjList_getRCap(struct CGame_t * gs, OBJ_REF obj);
MAD_REF ObjList_getRMad(struct CGame_t * gs, OBJ_REF obj);
MAD_REF ObjList_getRAi (struct CGame_t * gs, OBJ_REF obj);
PIP_REF ObjList_getRPip(struct CGame_t * gs, OBJ_REF iobj, int i);

#define VALID_OBJ_RANGE(XX) (((XX)>=0) && ((XX)<OBJLST_COUNT))
#define VALID_OBJ(LST, XX)    ( VALID_OBJ_RANGE(XX) && LST[XX].used )
#define VALIDATE_OBJ(LST, XX) ( VALID_OBJ(LST, XX) ? (XX) : (INVALID_OBJ) )


//--------------------------------------------------------------------------------------------
typedef struct collision_volume_t
{
  int   lod;
  float x_min, x_max;
  float y_min, y_max;
  float z_min, z_max;
  float xy_min, xy_max;
  float yx_min, yx_max;
} CVolume;

CVolume CVolume_merge(CVolume * pv1, CVolume * pv2);
CVolume CVolume_intersect(CVolume * pv1, CVolume * pv2);
bool_t  CVolume_draw( CVolume * cv, bool_t draw_square, bool_t draw_diamond );


//--------------------------------------------------------------------------------------------
typedef struct CVolume_Tree_t { CVolume leaf[8]; } CVolume_Tree;

//--------------------------------------------------------------------------------------------
typedef struct bump_data_t
{
  bool_t valid;       // is this data valid?

  Uint8  shadow;      // Size of shadow
  Uint8  size;        // Size of bumpers
  Uint8  sizebig;     // For octagonal bumpers
  Uint8  height;      // Distance from head to toe

  bool_t calc_is_platform;
  bool_t calc_is_mount;

  float  calc_size;
  float  calc_size_big;
  float  calc_height;

  CVolume        cv;
  CVolume_Tree * cv_tree;
  vect3   mids_hi, mids_lo;
} BData;

INLINE BData * BData_new(BData * b);
INLINE bool_t  BData_delete(BData * b);
INLINE BData * BData_renew(BData * b);

//--------------------------------------------------------------------------------------------
typedef enum Team_e
{
  TEAM_EVIL            = 'E' -'A',                      // E
  TEAM_GOOD            = 'G' -'A',                      // G
  TEAM_NULL            = 'N' -'A',                      // N
  TEAM_ZIPPY           = 'Z' -'A',
  TEAM_DAMAGE,                                          // For damage tiles
  TEAM_COUNT                                              // Teams A-Z, +1 more for damage tiles
} TEAM;

typedef struct CTeam_t
{
  bool_t  hatesteam[TEAM_COUNT];  // Don't damage allies...
  Uint16  morale;                 // Number of characters on team
  CHR_REF leader;                 // The leader of the team
  CHR_REF sissy;                  // Whoever called for help last
} CTeam;

#ifdef __cplusplus
  typedef TList<CTeam_t, TEAM_COUNT> TeamList_t;
  typedef TPList<CTeam_t, TEAM_COUNT> PTeam;
#else
  typedef CTeam TeamList_t[TEAM_COUNT];
  typedef CTeam * PTeam;
#endif

CTeam * CTeam_new(CTeam *pteam);
bool_t  CTeam_delete(CTeam *pteam);
CTeam * CTeam_renew(CTeam *pteam);

#define VALID_TEAM_RANGE(XX) ( ((XX)>=0) && ((XX)<TEAM_COUNT) )

INLINE CHR_REF team_get_sissy( struct CGame_t * gs, TEAM_REF iteam );
INLINE CHR_REF team_get_leader( struct CGame_t * gs, TEAM_REF iteam );

//--------------------------------------------------------------------------------------------
typedef struct vertex_data_blended_t
{
  Uint32  frame0;
  Uint32  frame1;
  Uint32  vrtmin;
  Uint32  vrtmax;
  float   lerp;
  bool_t  needs_lighting;

  // Storage for blended vertices
  vect3 *Vertices;
  vect3 *Normals;
  vect4 *Colors;
  vect2 *Texture;
  float *Ambient;      // Lighting hack ( Ooze )
} VData_Blended;

INLINE VData_Blended * VData_Blended_new();
INLINE void VData_Blended_delete(VData_Blended * v);

INLINE void VData_Blended_construct(VData_Blended * v);
INLINE void VData_Blended_destruct(VData_Blended * v);
INLINE void VData_Blended_Allocate(VData_Blended * v, size_t verts);
INLINE void VData_Blended_Deallocate(VData_Blended * v);

//--------------------------------------------------------------------------------------------
typedef enum damage_e
{
  DAMAGE_SLASH   = 0,                          //
  DAMAGE_CRUSH,                                //
  DAMAGE_POKE,                                 //
  DAMAGE_HOLY,                                 // (Most invert Holy damage )
  DAMAGE_EVIL,                                 //
  DAMAGE_FIRE,                                 //
  DAMAGE_ICE,                                  //
  DAMAGE_ZAP,                                  //
  MAXDAMAGETYPE,                              // Damage types
  DAMAGE_NULL     = 255,                       //
} DAMAGE;

#define DAMAGE_SHIFT         3                       // 000000xx Resistance ( 1 is common )
#define DAMAGE_INVERT        4                       // 00000x00 Makes damage heal
#define DAMAGE_CHARGE        8                       // 0000x000 Converts damage to mana
#define DAMAGE_MANA         16                       // 000x0000 Makes damage deal to mana

//--------------------------------------------------------------------------------------------
typedef struct tile_damage_t
{
  PIP_REF  parttype;
  short  partand;
  Sint8  sound;
  int    amount; //  EQ( 256 );                           // Amount of damage
  DAMAGE type; //  EQ( DAMAGE_FIRE );                      // Type of damage
} TILE_DAMAGE;

extern TILE_DAMAGE GTile_Dam;

#define DELAY_TILESOUND 16
#define TILEREAFFIRMAND  3


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------


void setup_particles( struct CGame_t * gs );

void spawn_bump_particles( struct CGame_t * gs, CHR_REF character, PRT_REF particle );

void   disaffirm_attached_particles( struct CGame_t * gs, CHR_REF character );
Uint16 number_of_attached_particles( struct CGame_t * gs, CHR_REF character );
void   reaffirm_attached_particles( struct CGame_t * gs, CHR_REF character );

void switch_team( struct CGame_t * gs, CHR_REF character, TEAM_REF team );
int  restock_ammo( struct CGame_t * gs, CHR_REF character, IDSZ idsz );
void issue_clean( struct CGame_t * gs, CHR_REF character );

int load_one_object( struct CGame_t * gs, int skin_count, const char * szObjectpath, char* szObjectname );


void obj_clear_pips( struct CGame_t * gs );
