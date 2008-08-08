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

struct sNet;
struct sConfigData;
struct sStatus;
struct sChr;
struct SoundState_t;
struct sGraphics;
struct script_global_values_t;
struct sGameStack;

struct sClient;
struct sServer;

enum e_Action;
enum e_Team;
enum e_Experience;


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// This is for random naming
#define CHOPPERMODEL                    32          //
#define MAXCHOP                         (OBJLST_COUNT*CHOPPERMODEL)
#define CHOPSIZE                        8
#define CHOPDATACHUNK                   (MAXCHOP*CHOPSIZE)

struct sChopData
{
  Uint16   count;                  // The number of name parts
  Uint32   write;                  // The data pointer
  char     text[CHOPDATACHUNK];    // The name parts
  Uint16   start[MAXCHOP];         // The first character of each part
};
typedef struct sChopData ChopData_t;

//--------------------------------------------------------------------------------------------
// Display messages
struct s_message_element
{
  Sint16    time;                                //
  char      textdisplay[MESSAGESIZE];            // The displayed text

};
typedef struct s_message_element MESSAGE_ELEMENT;

struct sMessageData
{
  // Message files
  Uint16  total;                                         // The number of messages
  Uint32  totalindex;                                    // Where to put letter

  Uint32  index[MAXTOTALMESSAGE];                        // Where it is
  char    text[MESSAGEBUFFERSIZE];                       // The text buffer
};
typedef struct sMessageData MessageData_t;

struct sMessageQueue
{
  int             count;

  Uint16          start;
  MESSAGE_ELEMENT list[MAXMESSAGE];
  float           timechange;
};
typedef struct sMessageQueue MessageQueue_t;

//--------------------------------------------------------------------------------------------
#define MAXENDTEXT 1024

struct sGame
{
  egoboo_key_t ekey;

  ProcState_t  proc;

  bool_t Single_frame;       // Is the game in single frame mode?
  bool_t Do_frame;           // Tell the game to process one frame

  // in-game menu stuff
  MenuProc_t igm;

  // random stuff
  Uint32 randie_index;       // The current index of the random number table

  // module parameters
  MOD_INFO    mod;
  ModState_t    modstate;
  ModSummary_t modtxt;

  // game-specific module stuff
  Mesh_t        Mesh;

  // water and lighting info
  WATER_INFO    water;

  // physics info
  PhysicsData_t phys;

  // game loop variables
  double dFrame, dUpdate;

  // links to important data
  struct sNet    * ns;
  struct sConfigData  * cd;
  struct sClient     * cl;
  struct sServer     * sv;

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
  Passage_t PassList[PASSLST_COUNT];

  // shop data
  Uint16 ShopList_count;
  Shop_t   ShopList[PASSLST_COUNT];

  // team data
  TeamList_t TeamList;

  // local variables
  float  PlaList_level;
  int    PlaList_count;                                 // Number of players
  PlaList_t PlaList;

  // messages
  MessageData_t MsgList;

  // script values
  ScriptInfo_t ScriptList;

  // dynamic lighting info
  int          DLightList_distancetobeat;      // The number to beat
  int          DLightList_count;               // Number of dynamic lights
  DLightList_t DLightList;

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

  ChopData_t chop;

};
typedef struct sGame Game_t;

Game_t * Game_create(struct sNet * net,  struct sClient * cl, struct sServer * sv);
bool_t   Game_destroy(Game_t ** gs );
bool_t   Game_renew(Game_t * gs);

INLINE ScriptInfo_t * Game_getScriptInfo(Game_t * gs) { if(NULL ==gs) return NULL; return &(gs->ScriptList); }
INLINE ProcState_t  * Game_getProcedure(Game_t * gs)  { if(NULL ==gs) return NULL; return &(gs->proc); }
INLINE MenuProc_t   * Game_getMenuProc(Game_t * gs)   { if(NULL ==gs) return NULL; return &(gs->igm); }
INLINE Mesh_t       * Game_getMesh(Game_t * gs)       { if(NULL ==gs) return NULL; return &(gs->Mesh); }

retval_t Game_registerNetwork( Game_t * gs, struct sNet    * net, bool_t destroy );
retval_t Game_registerClient ( Game_t * gs, struct sClient * cl,  bool_t destroy  );
retval_t Game_registerServer ( Game_t * gs, struct sServer * sv,  bool_t destroy  );


bool_t CapList_new( Game_t * gs );
bool_t CapList_delete( Game_t * gs );
bool_t CapList_renew( Game_t * gs );

bool_t EveList_new( Game_t * gs );
bool_t EveList_delete( Game_t * gs );
bool_t EveList_renew( Game_t * gs );

bool_t MadList_new( Game_t * gs );
bool_t MadList_delete( Game_t * gs );
bool_t MadList_renew( Game_t * gs );

bool_t PipList_new( Game_t * gs );
bool_t PipList_delete( Game_t * gs );
bool_t PipList_renew( Game_t * gs );

bool_t ChrList_new( Game_t * gs );
bool_t ChrList_delete( Game_t * gs );
bool_t ChrList_renew( Game_t * gs );

bool_t EncList_new( Game_t * gs );
bool_t EncList_delete( Game_t * gs );
bool_t EncList_renew( Game_t * gs );

bool_t PrtList_new( Game_t * gs );
bool_t PrtList_delete( Game_t * gs );
bool_t PrtList_renew( Game_t * gs );

bool_t PassList_new( Game_t * gs );
bool_t PassList_delete( Game_t * gs );
bool_t PassList_renew( Game_t * gs );

bool_t ShopList_new( Game_t * gs );
bool_t ShopList_delete( Game_t * gs );
bool_t ShopList_renew( Game_t * gs );

bool_t TeamList_new( Game_t * gs );
bool_t TeamList_delete( Game_t * gs );
bool_t TeamList_renew( Game_t * gs );

bool_t PlaList_new( Game_t * gs );
bool_t PlaList_delete( Game_t * gs );
bool_t PlaList_renew( Game_t * gs );



//--------------------------------------------------------------------------------------------
struct sSearchInfo
{
  bool_t  initialize;
  CHR_REF besttarget;                                      // For find_target
  Uint16  bestangle;                                       //
  Uint16  useangle;                                        //
  int     bestdistance;
  CHR_REF nearest;
  float   distance;
};
typedef struct sSearchInfo SearchInfo_t;

SearchInfo_t * SearchInfo_new(SearchInfo_t * psearch);

//--------------------------------------------------------------------------------------------
//Weather and water gfx
struct s_weather_info
{
  bool_t    overwater; // EQ( bfalse );       // Only spawn over water?
  int       timereset; // EQ( 10 );          // Rate at which weather particles spawn
  float     time; // EQ( 0 );                // 0 is no weather
  PLA_REF   player;
};
typedef struct s_weather_info WEATHER_INFO;

extern WEATHER_INFO GWeather;

//--------------------------------------------------------------------------------------------

void   make_newloadname( char *modname, char *appendname, char *newloadname );
void   export_one_character( Game_t * gs, CHR_REF character, Uint16 owner, int number );
void   export_all_local_players( Game_t * gs );
int    MessageQueue_get_free(MessageQueue_t * mq);
bool_t display_message( Game_t * gs, int message, CHR_REF chr_ref );

void load_action_names( char* loadname );
void read_setup( char* filename );

void make_lightdirectionlookup();
void make_enviro( void );
void draw_chr_info();
bool_t do_screenshot();
void move_water( float dUpdate );

CHR_REF search_best_leader( Game_t * gs, TEAM_REF team, CHR_REF exclude );
void call_for_help( Game_t * gs, CHR_REF character );
void give_experience( Game_t * gs, CHR_REF character, int amount, enum e_Experience xptype );
void give_team_experience( Game_t * gs, TEAM_REF team, int amount, enum e_Experience xptype );
void setup_alliances( Game_t * gs, char *modname );
void check_respawn( Game_t * gs );
void update_timers( Game_t * gs );
void reset_teams( Game_t * gs );
void reset_messages( Game_t * gs );
void reset_timers( Game_t * gs );

void set_default_config_data(struct sConfigData * pcon);

int load_all_messages( Game_t * gs, const char *szObjectpath, const char *szObjectname );

void attach_particle_to_character( Game_t * gs, PRT_REF particle, CHR_REF chr_ref, Uint16 vertoffset );

bool_t prt_search_block( Game_t * gs, SearchInfo_t * psearch, int block_x, int block_y, PRT_REF iprt, Uint16 facing,
                         bool_t request_friends, bool_t allow_anyone, TEAM_REF team,
                         CHR_REF donttarget, CHR_REF oldtarget );

bool_t prt_search_wide( Game_t * gs, SearchInfo_t * psearch, PRT_REF iprt, Uint16 facing,
                        Uint16 targetangle, bool_t request_friends, bool_t allow_anyone,
                        TEAM_REF team, CHR_REF donttarget, CHR_REF oldtarget );

bool_t chr_search_distant( Game_t * gs, SearchInfo_t * psearch, CHR_REF character, int maxdist, bool_t ask_enemies, bool_t ask_dead );

bool_t chr_search_block_nearest( Game_t * gs, SearchInfo_t * psearch, int block_x, int block_y, CHR_REF character, bool_t ask_items,
                               bool_t ask_friends, bool_t ask_enemies, bool_t ask_dead, bool_t seeinvisible, IDSZ idsz );

bool_t chr_search_wide_nearest( Game_t * gs, SearchInfo_t * psearch, CHR_REF character, bool_t ask_items,
                                bool_t ask_friends, bool_t ask_enemies, bool_t ask_dead, IDSZ idsz );

bool_t chr_search_wide( Game_t * gs, SearchInfo_t * psearch, CHR_REF character, bool_t ask_items,
                        bool_t ask_friends, bool_t ask_enemies, bool_t ask_dead, IDSZ idsz, bool_t excludeid );

bool_t chr_search_block( Game_t * gs, SearchInfo_t * psearch, int block_x, int block_y, CHR_REF character, bool_t ask_items,
                         bool_t ask_friends, bool_t ask_enemies, bool_t ask_dead, bool_t seeinvisible, IDSZ idsz,
                         bool_t excludeid );

bool_t chr_search_nearby( Game_t * gs, SearchInfo_t * psearch, CHR_REF character, bool_t ask_items,
                          bool_t ask_friends, bool_t ask_enemies, bool_t ask_dead, IDSZ ask_idsz );

void release_all_models(Game_t * gs);
void init_all_models(Game_t * gs);

void setup_characters( Game_t * gs, char *modname );
void ChrList_resynch(Game_t * gs);


struct sGameStack * Get_GameStack();

void set_alerts( Game_t * gs, CHR_REF character, float dUpdate );

void   print_status( Game_t * gs, Uint16 statindex );
bool_t add_status( Game_t * gs, CHR_REF character );
bool_t remove_stat( Game_t * gs, struct sChr * pchr );
void   sort_statlist( Game_t * gs );


bool_t reset_characters( Game_t * gs );

bool_t chr_is_player( Game_t * gs, CHR_REF character);


bool_t count_players(Game_t * gs);
void clear_message_queue(MessageQueue_t * q);
void clear_messages( MessageData_t * md);

void load_global_icons(Game_t * gs);

void recalc_character_bumpers( Game_t * gs );

char * naming_generate( Game_t * gs, Obj_t * pobj );
void naming_read( struct sGame * gs, const char * szModpath, const char * szObjectname, Obj_t * pobj);
void naming_prime( struct sGame * gs );

bool_t decode_escape_sequence( Game_t * gs, char * buffer, size_t buffer_size, const char * message, CHR_REF chr_ref );