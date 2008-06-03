#pragma once

#include "input.h"
#include "module.h"

#include "object.h"
#include "char.h"
#include "particle.h"
#include "mad.h"
#include "enchant.h"
#include "passage.h"
#include "menu.h"
#include "script.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct CNet_t;
struct ConfigData_t;
struct Status_t;
struct Chr_t;
struct SoundState_t;
struct GraphicState_t;
struct script_global_values_t;
struct GSStack_t;

struct CClient_t; 
struct CServer_t;

enum Action_e;
enum Team_e;
enum Experience_e;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

typedef struct CGame_t
{
  bool_t initialized;

  ProcState proc;

  bool_t Single_frame;       // Is the game in single frame mode?
  bool_t Do_frame;           // Tell the game to process one frame

  // in-game menu stuff
  MenuProc igm;

  // random stuff
  Uint32 randie_index;       // The current index of the random number table

  // module parameters
  MOD_INFO    mod;
  ModState    modstate;
  MOD_SUMMARY modtxt;

  // mgame-specific module stuff
  MESH_INFO   mesh;
  MeshMem     Mesh_Mem;

  WATER_INFO water;

  // game loop variables
  double dFrame, dUpdate;

  // links to important data
  struct CNet_t    * ns;
  struct ConfigData_t  * cd;
  struct CClient_t     * cl;
  struct CServer_t     * sv;

  // profiles
  Cap CapList[MAXCAP];
  Eve EveList[MAXEVE];
  Mad MadList[MAXMODEL];

  int PipList_count;
  Pip PipList[MAXPRTPIP];

  // character data
  int     ChrFreeList_count;
  CHR_REF ChrFreeList[MAXCHR];
  Chr     ChrList[MAXCHR];

  // enchant data
  int     EncFreeList_count;
  ENC_REF EncFreeList[MAXENCHANT];
  Enc     EncList[MAXENCHANT];

  // particle data
  int     PrtFreeList_count;
  PRT_REF PrtFreeList[MAXPRT];
  Prt     PrtList[MAXPRT];

  // passage data
  Uint32  PassList_count;
  Passage PassList[MAXPASS];

  // shop data
  Uint16 ShopList_count;
  Shop   ShopList[MAXPASS];

  // team data
  TeamInfo TeamList[TEAM_COUNT];

  // local variables
  int    PlaList_count;                                 // Number of players
  Player PlaList[MAXPLAYER];


  // script values
  ScriptInfo ScriptList;

  // local texture info
  GLtexture TxTexture[MAXTEXTURE];      // All normal textures

  int       TxIcon_count;               // Number of icons
  GLtexture TxIcon[MAXTEXTURE+1];       // All icon textures

  GLtexture TxMap;

  int       nullicon;
  int       keybicon;
  int       mousicon;
  int       joyaicon;
  int       joybicon;
  int       bookicon;                      // The first book icon

  Uint16    skintoicon[MAXTEXTURE];        // Skin to icon

  bool_t somepladead;           // Someone has died.
  bool_t allpladead;            // Has everyone died?

  Sint32 stt_clock;             // GetTickCount at start
  Sint32 lst_clock;             // The last total of ticks so far
  Sint32 wld_clock;             // The sync clock
  Uint32 wld_frame;             // The number of frames that should have been drawn

  Sint32 all_clock;             // The total number of ticks so far
  Uint32 all_frame;             // The total number of frames drawn so far

  float  pit_clock;             // For pit kills
  bool_t pitskill;              // Do they kill?


} CGame;

CGame * CGame_create(struct CNet_t * net,  struct CClient_t * cl, struct CServer_t * sv);
bool_t  CGame_destroy(CGame ** gs );
bool_t  CGame_renew(CGame * gs);

INLINE ScriptInfo * CGame_getScriptInfo(CGame * gs) { if(NULL==gs) return NULL; return &(gs->ScriptList); }
INLINE ProcState *  CGame_getProcedure(CGame * gs)  { if(NULL==gs) return NULL; return &(gs->proc); }
INLINE MenuProc  *  CGame_getMenuProc(CGame * gs)   { if(NULL==gs) return NULL; return &(gs->igm); }


bool_t CapList_new( CGame * gs );
bool_t CapList_delete( CGame * gs );
bool_t CapList_renew( CGame * gs );

bool_t EveList_new( CGame * gs );
bool_t EveList_delete( CGame * gs );
bool_t EveList_renew( CGame * gs );

bool_t MadList_new( CGame * gs );
bool_t MadList_delete( CGame * gs );
bool_t MadList_renew( CGame * gs );

bool_t PipList_new( CGame * gs );
bool_t PipList_delete( CGame * gs );
bool_t PipList_renew( CGame * gs );

bool_t ChrList_new( CGame * gs );
bool_t ChrList_delete( CGame * gs );
bool_t ChrList_renew( CGame * gs );

bool_t EncList_new( CGame * gs );
bool_t EncList_delete( CGame * gs );
bool_t EncList_renew( CGame * gs );

bool_t PrtList_new( CGame * gs );
bool_t PrtList_delete( CGame * gs );
bool_t PrtList_renew( CGame * gs );

bool_t PassList_new( CGame * gs );
bool_t PassList_delete( CGame * gs );
bool_t PassList_renew( CGame * gs );

bool_t ShopList_new( CGame * gs );
bool_t ShopList_delete( CGame * gs );
bool_t ShopList_renew( CGame * gs );

bool_t TeamList_new( CGame * gs );
bool_t TeamList_delete( CGame * gs );
bool_t TeamList_renew( CGame * gs );

bool_t PlaList_new( CGame * gs );
bool_t PlaList_delete( CGame * gs );
bool_t PlaList_renew( CGame * gs );



//--------------------------------------------------------------------------------------------


typedef struct SearchInfo_t
{
  bool_t  initialize;
  CHR_REF besttarget;                                      // For find_target
  Uint16  bestangle;                                       //
  Uint16  useangle;                                        //
  int     bestdistance;
  CHR_REF nearest;
  float   distance;
} SearchInfo;

SearchInfo * SearchInfo_new(SearchInfo * psearch);

//--------------------------------------------------------------------------------------------

void make_newloadname( char *modname, char *appendname, char *newloadname );
void export_one_character( CGame * gs, CHR_REF character, Uint16 owner, int number );
void export_all_local_players( CGame * gs );
int get_free_message(void);
void display_message( CGame * gs, int message, CHR_REF character );

void load_action_names( char* loadname );
void read_setup( char* filename );

void make_lightdirectionlookup();
void make_enviro( void );
void draw_chr_info();
bool_t do_screenshot();
void move_water( float dUpdate );

CHR_REF search_best_leader( CGame * gs, enum Team_e team, CHR_REF exclude );
void call_for_help( CGame * gs, CHR_REF character );
void give_experience( CGame * gs, CHR_REF character, int amount, enum Experience_e xptype );
void give_team_experience( CGame * gs, enum Team_e team, int amount, enum Experience_e xptype );
void setup_alliances( CGame * gs, char *modname );
void check_respawn( CGame * gs );
void update_timers( CGame * gs );
void reset_teams( CGame * gs );
void reset_messages( CGame * gs );
void reset_timers( CGame * gs );

void set_default_config_data(struct ConfigData_t * pcon);

void load_all_messages( CGame * gs, char *szObjectpath, char *szObjectname, Uint16 object );

void attach_particle_to_character( CGame * gs, PRT_REF particle, CHR_REF chr_ref, Uint16 vertoffset );

bool_t prt_search_block( CGame * gs, SearchInfo * psearch, int block_x, int block_y, PRT_REF iprt, Uint16 facing,
                         bool_t request_friends, bool_t allow_anyone, enum Team_e team,
                         Uint16 donttarget, Uint16 oldtarget );

bool_t prt_search_wide( CGame * gs, SearchInfo * psearch, PRT_REF iprt, Uint16 facing,
                        Uint16 targetangle, bool_t request_friends, bool_t allow_anyone,
                        enum Team_e team, Uint16 donttarget, Uint16 oldtarget );

bool_t chr_search_distant( CGame * gs, SearchInfo * psearch, CHR_REF character, int maxdist, bool_t ask_enemies, bool_t ask_dead );

bool_t chr_search_block_nearest( CGame * gs, SearchInfo * psearch, int block_x, int block_y, CHR_REF character, bool_t ask_items,
                               bool_t ask_friends, bool_t ask_enemies, bool_t ask_dead, bool_t seeinvisible, IDSZ idsz );

bool_t chr_search_wide_nearest( CGame * gs, SearchInfo * psearch, CHR_REF character, bool_t ask_items,
                                bool_t ask_friends, bool_t ask_enemies, bool_t ask_dead, IDSZ idsz );

bool_t chr_search_wide( CGame * gs, SearchInfo * psearch, CHR_REF character, bool_t ask_items,
                        bool_t ask_friends, bool_t ask_enemies, bool_t ask_dead, IDSZ idsz, bool_t excludeid );

bool_t chr_search_block( CGame * gs, SearchInfo * psearch, int block_x, int block_y, CHR_REF character, bool_t ask_items,
                         bool_t ask_friends, bool_t ask_enemies, bool_t ask_dead, bool_t seeinvisible, IDSZ idsz,
                         bool_t excludeid );

bool_t chr_search_nearby( CGame * gs, SearchInfo * psearch, CHR_REF character, bool_t ask_items,
                          bool_t ask_friends, bool_t ask_enemies, bool_t ask_dead, IDSZ ask_idsz );

void release_all_models(CGame * gs);
void init_all_models(CGame * gs);

void setup_characters( CGame * gs, char *modname );
void despawn_characters(CGame * gs);


struct GSStack_t * Get_GSStack();

void set_alerts( CGame * gs, CHR_REF character, float dUpdate );

void   print_status( CGame * gs, Uint16 statindex );
bool_t add_status( CGame * gs, CHR_REF character );
bool_t remove_stat( CGame * gs, struct Chr_t * pchr );
void   sort_statlist( CGame * gs );


bool_t reset_characters( CGame * gs );

bool_t chr_is_player( CGame * gs, CHR_REF character);


bool_t count_players(CGame * gs);