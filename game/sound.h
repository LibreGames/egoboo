#pragma once

#include "egoboo_math.h"
#include "egoboo_types.h"

#include <SDL_mixer.h>

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

struct CGame_t; 

//------------------------------------------------------------------------------

#define MAXSEQUENCE         256                     // Number of tracks in sequence
#define MAXWAVE         16                            // Up to 16 waves per model
#define VOLMIN          -4000                         // Minumum Volume level
#define VOLUMERATIO     7                             // Volume ratio

//Sound using SDL_Mixer

#define INVALID_SOUND   (-1)
#define INVALID_CHANNEL (-1)

#define FIX_SOUND(XX) ((((XX)<0) || ((XX)>=MAXWAVE)) ? INVALID_SOUND : (XX))

typedef enum global_sound_t
{
  GSOUND_COINGET = 0,              // 0 - Pick up coin
  GSOUND_DEFEND,                   // 1 - Defend clank
  GSOUND_WEATHER,                  // 2 - Weather Effect
  GSOUND_SPLASH,                   // 3 - Hit Water tile (Splash)
  GSOUND_COINFALL,                 // 4 - Coin falls on ground
  GSOUND_LEVELUP,				           // 5 - Level up sound
  GSOUND_COUNT = MAXWAVE
};

//------------------------------------------------------------------------------

struct ConfigData_t;

//------------------------------------------------------------------------------

//Music using SDL_Mixer
#define MAXPLAYLISTLENGTH 25      //Max number of different tracks loaded into memory

typedef struct sound_state_t
{
  bool_t      initialized;           // has SoundState_new() been run on this data?
  bool_t      mixer_loaded;          // Is the SDL_Mixer loaded?
  bool_t      music_loaded;          // Is the music loaded in memory?

  int         channel_count;         // actual number of sound channels
  int         buffersize;            // actual sound buffer size
  int         frequency;

  int         sound_volume;          // Volume of sounds played
  bool_t      soundActive;

  bool_t      musicActive;
  int         music_volume;          // The sound volume of music
  int         song_loops;
  int         song_index;            // index of the cullrently playing song

  Mix_Chunk * mc_list[MAXWAVE];            // All sounds loaded into memory
  Mix_Music * mus_list[MAXPLAYLISTLENGTH]; //This is a specific music file loaded into memory

} SoundState;

extern SoundState sndState;

//------------------------------------------------------------------------------

bool_t snd_initialize(struct ConfigData_t * cd);
bool_t snd_quit();
bool_t snd_synchronize(struct ConfigData_t * cd);
SoundState * snd_getState(struct ConfigData_t * cd);

bool_t snd_reopen();
bool_t snd_unload_music();
void snd_stop_music(int fadetime);
void snd_apply_mods( int channel, float intensity, vect3 snd_pos, vect3 ear_pos, Uint16 ear_turn_lr  );
int snd_play_sound( struct CGame_t * gs, float intensity, vect3 pos, Mix_Chunk *loadedwave, int loops, int whichobject, int soundnumber);
void snd_stop_sound( int whichchannel );
void snd_play_music( int songnumber, int fadetime, int loops );
int snd_play_particle_sound( struct CGame_t * gs, float intensity, PRT_REF particle, Sint8 sound );

