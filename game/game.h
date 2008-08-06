#pragma once

#include "input.h"
#include "module.h"

#include "char.h"
#include "particle.h"
#include "enchant.h"
#include "passage.h"
#include "mesh.h"
#include "Menu.h"
#include "script.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct CNet_t;
struct ConfigData_t;
struct Status_t;
struct CChr_t;
struct SoundState_t;
struct CGraphics_t;
struct script_global_values_t;
struct GSStack_t;

struct CClient_t;
struct CServer_t;

enum Action_e;
enum Team_e;
enum Experience_e;


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// This is for random naming
#define CHOPPERMODEL                    32          //
#define MAXCHOP                         (OBJLST_COUNT*CHOPPERMODEL)
#define CHOPSIZE                        8
#define CHOPDATACHUNK                   (MAXCHOP*CHOPSIZE)

struct ChopData_t
{
  Uint16   count;                  // The number of name parts
  Uint32   write;                  // The data pointer
  char     text[CHOPDATACHUNK];    // The name parts
  Uint16   start[MAXCHOP];         // The first character of each part
};
typedef struct ChopData_t ChopData;

//--------------------------------------------------------------------------------------------
// Display messages
struct message_element_t
{
  Sint16    time;                                //
  char      textdisplay[MESSAGESIZE];            // The displayed text

};
typedef struct message_element_t MESSAGE_ELEMENT;

struct MessageData_t
{
  // Message files
  Uint16  total;                                         // The number of messages
  Uint32  totalindex;                                    // Where to put letter

  Uint32  index[MAXTOTALMESSAGE];                        // Where it is
  char    text[MESSAGEBUFFERSIZE];                       // The text buffer
};
typedef struct MessageData_t MessageData;

struct MessageQueue_t
{
  int             count;

  Uint16          start;
  MESSAGE_ELEMENT list[MAXMESSAGE];
  float           timechange;
};
typedef struct MessageQueue_t MessageQueue;

//--------------------------------------------------------------------------------------------
#define MAXENDTEXT 1024

struct CGame_t
{
  egoboo_key ekey;

  ProcState  proc;

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

  // water and lighting info
  WATER_INFO water;

  // physics info
  CPhysicsData phys;

  // game loop variables
  double dFrame, dUpdate;

  // links to important data
  struct CNet_t    * ns;
  struct ConfigData_t  * cd;
  struct CClient_t     * cl;
  struct CServer_t     * sv;

  int      ObjFreeList_count;
  OBJ_REF  ObjFreeList[CHRLST_COUNT];
  ObjList_t ObjList;

  // profiles
  CapList_t CapList;
  EveList_t EveList;
  MadList_t MadList;

  int PipList_count;
  PipList_t PipList;

  // character data
  int       ChrFreeList_count;
  CHR_REF   ChrFreeList[CHRLST_COUNT];
  ChrList_t ChrList;

  // enchant data
  int       EncFreeList_count;
  ENC_REF   EncFreeList[ENCLST_COUNT];
  EncList_t EncList;

  // particle data
  int       PrtFreeList_count;
  PRT_REF   PrtFreeList[PRTLST_COUNT];
  PrtList_t PrtList;

  // passage data
  Uint32  PassList_count;
  Passage PassList[PASSLST_COUNT];

  // shop data
  Uint16 ShopList_count;
  Shop   ShopList[PASSLST_COUNT];

  // team data
  TeamList_t TeamList;

  // local variables
  float  PlaList_level;
  int    PlaList_count;                                 // Number of players
  PlaList_t PlaList;

  // messages
  MessageData MsgList;

  // script values
  ScriptInfo ScriptList;

  // local texture info
  GLtexture TxTexture[MAXTEXTURE];      // All normal textures

  int       TxIcon_count;               // Number of icons
  GLtexture TxIcon[MAXICONTX];       // All icon textures

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

  char endtext[MAXENDTEXT];     // The end-module text

  ChopData chop;

};
typedef struct CGame_t CGame;

CGame * CGame_create(struct CNet_t * net,  struct CClient_t * cl, struct CServer_t * sv);
bool_t  CGame_destroy(CGame ** gs );
bool_t  CGame_renew(CGame * gs);

INLINE ScriptInfo * CGame_getScriptInfo(CGame * gs) { if(NULL ==gs) return NULL; return &(gs->ScriptList); }
INLINE ProcState *  CGame_getProcedure(CGame * gs)  { if(NULL ==gs) return NULL; return &(gs->proc); }
INLINE MenuProc  *  CGame_getMenuProc(CGame * gs)   { if(NULL ==gs) return NULL; return &(gs->igm); }

retval_t CGame_registerNetwork( CGame * gs, struct CNet_t    * net, bool_t destroy );
retval_t CGame_registerClient ( CGame * gs, struct CClient_t * cl,  bool_t destroy  );
retval_t CGame_registerServer ( CGame * gs, struct CServer_t * sv,  bool_t destroy  );


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
struct SearchInfo_t
{
  bool_t  initialize;
  CHR_REF besttarget;                                      // For find_target
  Uint16  bestangle;                                       //
  Uint16  useangle;                                        //
  int     bestdistance;
  CHR_REF nearest;
  float   distance;
};
typedef struct SearchInfo_t SearchInfo;

SearchInfo * SearchInfo_new(SearchInfo * psearch);

//--------------------------------------------------------------------------------------------
//Weather and water gfx
struct weather_info_t
{
  bool_t    overwater; // EQ( bfalse );       // Only spawn over water?
  int       timereset; // EQ( 10 );          // Rate at which weather particles spawn
  float     time; // EQ( 0 );                // 0 is no weather
  PLA_REF   player;
};
typedef struct weather_info_t WEATHER_INFO;

extern WEATHER_INFO GWeather;

//--------------------------------------------------------------------------------------------

void   make_newloadname( char *modname, char *appendname, char *newloadname );
void   export_one_character( CGame * gs, CHR_REF character, Uint16 owner, int number );
void   export_all_local_players( CGame * gs );
int    MessageQueue_get_free(MessageQueue * mq);
bool_t display_message( CGame * gs, int message, CHR_REF chr_ref );

void load_action_names( char* loadname );
void read_setup( char* filename );

void make_lightdirectionlookup();
void make_enviro( void );
void draw_chr_info();
bool_t do_screenshot();
void move_water( float dUpdate );

CHR_REF search_best_leader( CGame * gs, TEAM_REF team, CHR_REF exclude );
void call_for_help( CGame * gs, CHR_REF character );
void give_experience( CGame * gs, CHR_REF character, int amount, enum Experience_e xptype );
void give_team_experience( CGame * gs, TEAM_REF team, int amount, enum Experience_e xptype );
void setup_alliances( CGame * gs, char *modname );
void check_respawn( CGame * gs );
void update_timers( CGame * gs );
void reset_teams( CGame * gs );
void reset_messages( CGame * gs );
void reset_timers( CGame * gs );

void set_default_config_data(struct ConfigData_t * pcon);

int load_all_messages( CGame * gs, const char *szObjectpath, const char *szObjectname );

void attach_particle_to_character( CGame * gs, PRT_REF particle, CHR_REF chr_ref, Uint16 vertoffset );

bool_t prt_search_block( CGame * gs, SearchInfo * psearch, int block_x, int block_y, PRT_REF iprt, Uint16 facing,
                         bool_t request_friends, bool_t allow_anyone, TEAM_REF team,
                         CHR_REF donttarget, CHR_REF oldtarget );

bool_t prt_search_wide( CGame * gs, SearchInfo * psearch, PRT_REF iprt, Uint16 facing,
                        Uint16 targetangle, bool_t request_friends, bool_t allow_anyone,
                        TEAM_REF team, CHR_REF donttarget, CHR_REF oldtarget );

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
void ChrList_resynch(CGame * gs);


struct GSStack_t * Get_GSStack();

void set_alerts( CGame * gs, CHR_REF character, float dUpdate );

void   print_status( CGame * gs, Uint16 statindex );
bool_t add_status( CGame * gs, CHR_REF character );
bool_t remove_stat( CGame * gs, struct CChr_t * pchr );
void   sort_statlist( CGame * gs );


bool_t reset_characters( CGame * gs );

bool_t chr_is_player( CGame * gs, CHR_REF character);


bool_t count_players(CGame * gs);
void clear_message_queue(MessageQueue * q);
void clear_messages( MessageData * md);

void load_global_icons(CGame * gs);

void recalc_character_bumpers( CGame * gs );

char * naming_generate( CGame * gs, CObj * pobj );
void naming_read( struct CGame_t * gs, const char * szModpath, const char * szObjectname, CObj * pobj);
void naming_prime( struct CGame_t * gs );

bool_t decode_escape_sequence( CGame * gs, char * buffer, size_t buffer_size, const char * message, CHR_REF chr_ref );