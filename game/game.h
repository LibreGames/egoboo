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

struct NetState_t;
struct ConfigData_t;
struct Status_t;
struct Chr_t;
struct SoundState_t;
struct GraphicState_t;
struct script_global_values_t;
struct GSStack_t;

enum Action_e;
enum Team_e;
enum Experience_e;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

typedef struct GameState_t
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
  struct NetState_t    * ns;
  struct ConfigData_t  * cd;

  // aliases to the NetState pointers
  struct ClientState_t * al_cs;
  struct ServerState_t * al_ss;


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


} GameState;

GameState * GameState_create();
bool_t GameState_destroy(GameState ** gs );
bool_t GameState_renew(GameState * gs);

INLINE ScriptInfo * GameState_getScriptInfo(GameState * gs) { if(NULL==gs) return NULL; return &(gs->ScriptList); }
INLINE ProcState * GameState_getProcedure(GameState * gs) { if(NULL==gs) return NULL; return &(gs->proc); }
INLINE MenuProc  * GameState_getMenuProc(GameState * gs) { if(NULL==gs) return NULL; return &(gs->igm); }


bool_t CapList_new( GameState * gs );
bool_t CapList_delete( GameState * gs );
bool_t CapList_renew( GameState * gs );

bool_t EveList_new( GameState * gs );
bool_t EveList_delete( GameState * gs );
bool_t EveList_renew( GameState * gs );

bool_t MadList_new( GameState * gs );
bool_t MadList_delete( GameState * gs );
bool_t MadList_renew( GameState * gs );

bool_t PipList_new( GameState * gs );
bool_t PipList_delete( GameState * gs );
bool_t PipList_renew( GameState * gs );

bool_t ChrList_new( GameState * gs );
bool_t ChrList_delete( GameState * gs );
bool_t ChrList_renew( GameState * gs );

bool_t EncList_new( GameState * gs );
bool_t EncList_delete( GameState * gs );
bool_t EncList_renew( GameState * gs );

bool_t PrtList_new( GameState * gs );
bool_t PrtList_delete( GameState * gs );
bool_t PrtList_renew( GameState * gs );

bool_t PassList_new( GameState * gs );
bool_t PassList_delete( GameState * gs );
bool_t PassList_renew( GameState * gs );

bool_t ShopList_new( GameState * gs );
bool_t ShopList_delete( GameState * gs );
bool_t ShopList_renew( GameState * gs );

bool_t TeamList_new( GameState * gs );
bool_t TeamList_delete( GameState * gs );
bool_t TeamList_renew( GameState * gs );

bool_t PlaList_new( GameState * gs );
bool_t PlaList_delete( GameState * gs );
bool_t PlaList_renew( GameState * gs );



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
void export_one_character( GameState * gs, CHR_REF character, Uint16 owner, int number );
void export_all_local_players( GameState * gs );
int get_free_message(void);
void display_message( GameState * gs, int message, CHR_REF character );

void load_action_names( char* loadname );
void read_setup( char* filename );

void make_lightdirectionlookup();
void make_enviro( void );
void draw_chr_info();
bool_t do_screenshot();
void move_water( float dUpdate );

CHR_REF search_best_leader( GameState * gs, enum Team_e team, CHR_REF exclude );
void call_for_help( GameState * gs, CHR_REF character );
void give_experience( GameState * gs, CHR_REF character, int amount, enum Experience_e xptype );
void give_team_experience( GameState * gs, enum Team_e team, int amount, enum Experience_e xptype );
void setup_alliances( GameState * gs, char *modname );
void check_respawn( GameState * gs );
void update_timers( GameState * gs );
void reset_teams( GameState * gs );
void reset_messages( GameState * gs );
void reset_timers( GameState * gs );

void set_default_config_data(struct ConfigData_t * pcon);

void load_all_messages( GameState * gs, char *szObjectpath, char *szObjectname, Uint16 object );

void attach_particle_to_character( GameState * gs, PRT_REF particle, CHR_REF chr_ref, Uint16 vertoffset );

bool_t prt_search_block( GameState * gs, SearchInfo * psearch, int block_x, int block_y, PRT_REF iprt, Uint16 facing,
                         bool_t request_friends, bool_t allow_anyone, enum Team_e team,
                         Uint16 donttarget, Uint16 oldtarget );

bool_t prt_search_wide( GameState * gs, SearchInfo * psearch, PRT_REF iprt, Uint16 facing,
                        Uint16 targetangle, bool_t request_friends, bool_t allow_anyone,
                        enum Team_e team, Uint16 donttarget, Uint16 oldtarget );

bool_t chr_search_distant( GameState * gs, SearchInfo * psearch, CHR_REF character, int maxdist, bool_t ask_enemies, bool_t ask_dead );

bool_t chr_search_block_nearest( GameState * gs, SearchInfo * psearch, int block_x, int block_y, CHR_REF character, bool_t ask_items,
                               bool_t ask_friends, bool_t ask_enemies, bool_t ask_dead, bool_t seeinvisible, IDSZ idsz );

bool_t chr_search_wide_nearest( GameState * gs, SearchInfo * psearch, CHR_REF character, bool_t ask_items,
                                bool_t ask_friends, bool_t ask_enemies, bool_t ask_dead, IDSZ idsz );

bool_t chr_search_wide( GameState * gs, SearchInfo * psearch, CHR_REF character, bool_t ask_items,
                        bool_t ask_friends, bool_t ask_enemies, bool_t ask_dead, IDSZ idsz, bool_t excludeid );

bool_t chr_search_block( GameState * gs, SearchInfo * psearch, int block_x, int block_y, CHR_REF character, bool_t ask_items,
                         bool_t ask_friends, bool_t ask_enemies, bool_t ask_dead, bool_t seeinvisible, IDSZ idsz,
                         bool_t excludeid );

bool_t chr_search_nearby( GameState * gs, SearchInfo * psearch, CHR_REF character, bool_t ask_items,
                          bool_t ask_friends, bool_t ask_enemies, bool_t ask_dead, IDSZ ask_idsz );

void release_all_models(GameState * gs);
void init_all_models(GameState * gs);

void setup_characters( GameState * gs, char *modname );
void despawn_characters(GameState * gs);


struct GSStack_t * Get_GSStack();

void set_alerts( GameState * gs, CHR_REF character, float dUpdate );

void   print_status( GameState * gs, Uint16 statindex );
bool_t add_status( GameState * gs, CHR_REF character );
bool_t remove_stat( GameState * gs, struct Chr_t * pchr );
void   sort_statlist( GameState * gs );


bool_t reset_characters( GameState * gs );

bool_t chr_is_player( GameState * gs, CHR_REF character);


bool_t count_players(GameState * gs);