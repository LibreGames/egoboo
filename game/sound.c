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
/// @brief Egoboo Sound System
/// @details Implements music and sound effects using SDL_mixer.

#include "sound.h"

#include "Log.h"
#include "camera.h"

#include "egoboo_utility.h"
#include "egoboo.h"

#include "particle.inl"
#include "char.inl"
#include "egoboo_math.inl"
#include "egoboo_types.inl"

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#define SOUND_BUFFER_SIZE 128

struct sSoundInfo
{
  Game_t   * gs;
  float      intensity;
  vect3      pos;
  Mix_Chunk *loadedwave;
  int        loops;
  OBJ_REF    whichobject;
  int        soundnumber;
};

typedef struct sSoundInfo SoundInfo_t;

SoundInfo_t * SoundInfo_new(SoundInfo_t * info)
{
  if(NULL == info) return info;

  memset(info, 0, sizeof(SoundInfo_t));

  info->loops = -1;

  return info;
};
bool_t SoundInfo_delete(SoundInfo_t * info)
{
  if(NULL == info) return bfalse;

  memset(info, 0, sizeof(SoundInfo_t));

  return btrue;
};

//------------------------------------------------------------------------------
struct sSoundBuffer
{
  int         size;
  SoundInfo_t buffer[SOUND_BUFFER_SIZE];
};

typedef struct sSoundBuffer SoundBuffer_t;

SoundBuffer_t * SoundBuffer_new(SoundBuffer_t * sb)
{
  if(NULL == sb) return sb;
  sb->size = 0;
  return sb;
}

bool_t SoundBuffer_delete(SoundBuffer_t * sb)
{
  int i;
  if(NULL == sb) return bfalse;

  for(i=0; i<SOUND_BUFFER_SIZE; i++)
  {
    SoundInfo_delete( sb->buffer + i);
  };
  sb->size = 0;

  return btrue;
};

bool_t SoundBuffer_store( SoundBuffer_t * sb, SoundInfo_t * si)
{
  if(NULL == sb || NULL == si || sb->size >= SOUND_BUFFER_SIZE) return bfalse;

  sb->buffer[sb->size] = *si;
  sb->size++;

  return btrue;
}

SoundInfo_t * SoundBuffer_retreive( SoundBuffer_t * sb)
{
  SoundInfo_t si_tmp;

  if(NULL == sb || sb->size <= 0) return NULL;

  sb->size--;

  si_tmp = sb->buffer[0];
  sb->buffer[0] = sb->buffer[sb->size];
  sb->buffer[sb->size] = si_tmp;

  return sb->buffer + sb->size;
};

static SoundBuffer_t _snd_buffer;

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void EgobooChannelFinishedCallback(int channel)
{
  // grab a sound off of the buffer and try again

  SoundInfo_t * si;

  si = SoundBuffer_retreive(&_snd_buffer);
  if(NULL == si) return;

  channel = Mix_PlayChannel( channel, si->loadedwave, si->loops );

  if( INVALID_CHANNEL == channel )
  {
    // ARGH! playing failed even though we have been given an onen channel!
    // This sample must be faulty!

    printf( "EgobooChannelFinishedCallback() - cannot play sound - error = \"%s\".\n", Mix_GetError() );
  }
  else
  {
    //log_debug("snd_play_sound() - \n\tplaying delayed sound on channel %d\n", channel );
    snd_apply_mods( channel, si->intensity, si->pos, GCamera.trackpos, GCamera.turn_lr);
  };

};

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

SoundState_t _sndState = { bfalse };

static SoundState_t * SoundState_new(SoundState_t * ss, struct sConfigData * cd);
static bool_t SoundState_synchronize(SoundState_t * ss, struct sConfigData * cd);

//------------------------------------------------------------------------------
SoundState_t * snd_getState(ConfigData_t * cd)
{
  if( !EKEY_VALID(_sndState) )
  {
    snd_initialize(cd);
  }

  return &_sndState;
}

//------------------------------------------------------------------------------
bool_t snd_synchronize(struct sConfigData * cd)
{
  SoundState_t * snd = snd_getState(cd);

  if(NULL == snd) return bfalse;

  SoundState_synchronize(snd, cd);

  return btrue;
}

//------------------------------------------------------------------------------
//This function enables the use of SDL_Mixer functions, returns btrue if success
bool_t snd_initialize(ConfigData_t * cd)
{
  if(NULL == cd) return bfalse;

  SoundState_new(&_sndState, cd);

  if ( !_sndState.mixer_loaded )
  {
    log_info( "Initializing SDL_mixer audio services version %i.%i.%i... ", MIX_MAJOR_VERSION, MIX_MINOR_VERSION, MIX_PATCHLEVEL);

    // initialize the overflow buffer
    SoundBuffer_new( &_snd_buffer );

    // grab the defaults from the configuration data
    SoundState_synchronize( &_sndState, cd );

    if( !snd_reopen() )
    {
      log_message( "Failed!\n" );
      log_error( "Unable to initialize audio: %s\n", Mix_GetError() );
    }
    else
    {
      _sndState.mixer_loaded = btrue;
      log_message( "Succeeded!\n" );
    }

  }

  return _sndState.mixer_loaded;
}

//------------------------------------------------------------------------------
bool_t snd_quit()
{
  if(NULL == &_sndState) return bfalse;

  if( _sndState.mixer_loaded )
  {
    Mix_CloseAudio();
    _sndState.mixer_loaded = bfalse;

    // initialize the overflow buffer
    SoundBuffer_delete( &_snd_buffer );
  };

  return !_sndState.mixer_loaded;
}


//------------------------------------------------------------------------------
//SOUND-------------------------------------------------------------------------
//------------------------------------------------------------------------------

void snd_apply_mods( int channel, float intensity, vect3 snd_pos, vect3 ear_pos, Uint16 ear_turn_lr  )
{
  /// @details BB> This functions modifies an already playing 3d sound sound according position, orientation, etc.
  //      Modeled after physical parameters, but does not model doppler shift...
  float dist_xyz2, dist_xy2, volume;
  float vl, vr, dx, dy;
  int vol_left, vol_right;
  EGO_CONST float reverbdistance = 128 * 2.5;  // there will effectively be "no change"  in the sound volume for distances less than 2.5 tiles away

  if ( !_sndState.soundActive || INVALID_CHANNEL == channel ) return;

  dist_xy2 = ( ear_pos.x - snd_pos.x ) * ( ear_pos.x - snd_pos.x ) + ( ear_pos.y - snd_pos.y ) * ( ear_pos.y - snd_pos.y );
  dist_xyz2 = dist_xy2 + ( ear_pos.z - snd_pos.z ) * ( ear_pos.z - snd_pos.z );
  volume = intensity * ( CData.soundvolume / 100.0f ) * reverbdistance * reverbdistance / ( reverbdistance * reverbdistance + dist_xyz2 ); //adjust volume with ratio and sound volume

  // convert the camera turn to a vector
  turn_to_vec( ear_turn_lr, &dx, &dy );

  if ( dist_xy2 == 0.0f )
  {
    vl = vr = 0.5f;
  }
  else
  {
    // the the cross product of the direction with the camera direction
    // (proportional to the sine of the angle)
    float cp, ftmp;

    // normalize the square of the value
    cp = ( ear_pos.x - snd_pos.x ) * dy - ( ear_pos.y - snd_pos.y ) * dx;
    ftmp = cp * cp / dist_xy2;

    // determine the volume in the left and right speakers
    if ( cp > 0 )
    {
      vl = (1.0f - ftmp) * 0.5f;
      vr = (1.0f + ftmp) * 0.5f;
    }
    else
    {
      vl = (1.0f + ftmp) * 0.5f;
      vr = (1.0f - ftmp) * 0.5f;
    }
  }

  vol_left  = MIN(255, 255 * vl * volume);
  vol_right = MIN(255, 255 * vr * volume);

  Mix_SetPanning( channel, vol_left, vol_right );

};

//------------------------------------------------------------------------------
int snd_play_sound( Game_t * gs, float intensity, vect3 pos, Mix_Chunk *loadedwave, int loops, OBJ_REF whichobject, int soundnumber)
{
  /// @details ZF> This function plays a specified sound
  // (Or returns -1 (INVALID_CHANNEL) if it failed to play the sound)
  Sint8 channel;
  Cap_t * pcap;

  if( !_sndState.soundActive ) return INVALID_CHANNEL;

  // the sound is too quiet hear
  if( 255.0f * intensity * ( CData.soundvolume / 100.0f ) < 1.0f )
    return INVALID_CHANNEL;

  pcap = ObjList_getPCap(gs, whichobject);

  if ( NULL == loadedwave  )
  {
    if( NULL == pcap )
    {
      log_warning( "Sound file not correctly loaded (Not found?) - Object %d is trying to play sound%i.wav\n", whichobject, soundnumber );
    }
    else
    {
      log_warning( "Sound file not correctly loaded (Not found?) - Object \"%s\" is trying to play sound%i.wav\n", ObjList_getPCap(gs, whichobject)->classname, soundnumber );
    };

    return INVALID_CHANNEL;
  }

  // clear any other error
  Mix_GetError();
  channel = Mix_PlayChannel( -1, loadedwave, loops );

  if( INVALID_CHANNEL == channel )
  {
    // store the snd_play_sound() info until a channel becomes free
    SoundInfo_t si_tmp;
    bool_t      stored;

    // initialize the sound info
    SoundInfo_new( &si_tmp );
    si_tmp.gs          = gs;
    si_tmp.intensity   = intensity;
    si_tmp.pos         = pos;
    si_tmp.loadedwave  = loadedwave;
    si_tmp.loops       = loops;
    si_tmp.whichobject = whichobject;
    si_tmp.soundnumber = soundnumber;

    stored = SoundBuffer_store( &_snd_buffer, &si_tmp );
    SoundInfo_delete( &si_tmp );

    if( stored )
    {
      // log_warning( "All sound channels are currently in use. Sound being stored to play later\n" );
    }
    else if( NULL == pcap )
    {
      log_warning( "All sound channels are currently in use. No buffers available. Global sound %d NOT playing\n", REF_TO_INT(whichobject) );
    }
    else
    {
      log_warning( "All sound channels are currently in use. No buffers available. Sound is NOT playing - Object \"%s\" is trying to play sound%i.wav\n", pcap->classname, soundnumber );
    };
  }
  else
  {
    snd_apply_mods( channel, intensity, pos, GCamera.trackpos, GCamera.turn_lr);
  };

  return channel;
}

//--------------------------------------------------------------------------------------------
int snd_play_global_sound( Game_t * gs, float intensity, vect3 pos, Sint8 sound )
{
  int channel = INVALID_CHANNEL;

  //This function plays a sound effect for a particle
  if ( INVALID_SOUND == sound ) return channel;
  if( !_sndState.soundActive ) return channel;

  //Play global (coins for example)
  if( sound < GSOUND_COUNT)
  {
    channel = snd_play_sound( gs, intensity, pos, _sndState.mc_list[sound], 0, INVALID_OBJ, sound );
  };

  return channel;
}

//--------------------------------------------------------------------------------------------
int snd_play_particle_sound( Game_t * gs, float intensity, PRT_REF particle, Sint8 sound )
{
  int channel = INVALID_CHANNEL;
  OBJ_REF iobj;

  //This function plays a sound effect for a particle
  if ( INVALID_SOUND == sound ) return channel;
  if( !_sndState.soundActive ) return channel;

  // Play particle sound sound
  iobj = gs->PrtList[particle].model;
  if( INVALID_OBJ == iobj )
  {
    //this is a global sound
    channel = snd_play_global_sound( gs, intensity, gs->PrtList[particle].ori.pos, sound);
  }
  else if ( VALID_OBJ(gs->ObjList, iobj) && sound < MAXWAVE)
  {
    channel = snd_play_sound( gs, intensity, gs->PrtList[particle].ori.pos, gs->ObjList[iobj].wavelist[sound], 0, iobj, sound );
  }

  return channel;
}


//------------------------------------------------------------------------------
void snd_stop_sound( int whichchannel )
{
  /// @details ZF> This function is used for stopping a looped sound, but can be used to stop
  // a particular sound too. If whichchannel is -1, all playing channels will fade out.

  if( _sndState.mixer_loaded )
  {
    Mix_FadeOutChannel( whichchannel, 400 ); //400 ms is nice
  }
}

//------------------------------------------------------------------------------
//Music Stuff-------------------------------------------------------------------
//------------------------------------------------------------------------------

bool_t snd_unload_music()
{
  int cnt;

  if(NULL == &_sndState) return bfalse;
  if(!_sndState.music_loaded) return btrue;

  for(cnt=0; cnt<MAXPLAYLISTLENGTH; cnt++)
  {
    if(NULL != _sndState.mus_list[cnt])
    {
      Mix_FreeMusic( _sndState.mus_list[cnt] );
      _sndState.mus_list[cnt] = NULL;
    }
  }

  _sndState.music_loaded = bfalse;

  return btrue;
}



//------------------------------------------------------------------------------
void snd_play_music( int songnumber, int fadetime, int loops )
{
  /// @details ZF> This functions plays a specified track loaded into memory
  if ( _sndState.musicActive && _sndState.music_loaded && _sndState.song_index != songnumber)
  {
    Mix_FadeOutMusic( fadetime );
    Mix_PlayMusic( _sndState.mus_list[songnumber], loops );
    _sndState.song_loops = loops;
    _sndState.song_index = songnumber;
  }
}

//------------------------------------------------------------------------------
void snd_stop_music(int fadetime)
{
  /// @details ZF> This function sets music track to pause

  Mix_FadeOutMusic(fadetime);
}


//------------------------------------------------------------------------------
bool_t SoundState_synchronize(SoundState_t * snd, ConfigData_t * cd)
{
  /// @details BB> update the current sound state from the config data

  if(NULL == snd || NULL == cd) return bfalse;

  snd->soundActive   = cd->allow_sound;
  snd->musicActive   = cd->allow_music;

  snd->music_volume  = cd->musicvolume;
  snd->sound_volume  = cd->soundvolume;

  snd->channel_count = cd->maxsoundchannel;
  snd->buffersize    = cd->buffersize;

  return btrue;
}

//------------------------------------------------------------------------------
bool_t snd_reopen()
{
  /// @details BB> an attempt to automatically update the SDL_mixer state. This could probably be optimized
  //      since not everything requires restarting the mixer.

  bool_t paused;

  // ???assume that pausing and then unpausing the music will preserve the song location???
  paused = (-1 != _sndState.song_index) && Mix_PausedMusic();
  Mix_PauseMusic();

  if( _sndState.mixer_loaded )
  {
    Mix_CloseAudio();
    _sndState.mixer_loaded = bfalse;
  };

  // open the mixer with a "safe" number of channels
  _sndState.mixer_loaded = ( 0 == Mix_OpenAudio( _sndState.frequency, MIX_DEFAULT_FORMAT, 2, _sndState.buffersize ));

  // register our callback for handling channel overflows
  Mix_ChannelFinished( EgobooChannelFinishedCallback );

  if(_sndState.mixer_loaded)
  {
    // try to allocate the requested number of channels
    _sndState.channel_count = Mix_AllocateChannels( _sndState.channel_count );
    Mix_VolumeMusic( _sndState.music_volume );
  }

  if(_sndState.mixer_loaded && _sndState.musicActive)
  {
    if(paused)
    {
      // try to restart the song
      Mix_PlayMusic( _sndState.mus_list[_sndState.song_index], _sndState.song_loops );
    }
    else
    {
      Mix_ResumeMusic();
    }
  }

  return _sndState.mixer_loaded;
}


//------------------------------------------------------------------------------
bool_t SoundState_delete(SoundState_t * snd)
{
  if(NULL == snd) return bfalse;

  if( !EKEY_PVALID(snd) ) return btrue;

  EKEY_PINVALIDATE(snd);

  return btrue;
}

//------------------------------------------------------------------------------
SoundState_t * SoundState_new(SoundState_t * snd, ConfigData_t * cd)
{
  /// @details BB> do a raw initialization of the sound state

  //fprintf( stdout, "SoundState_new()\n");

  if(NULL == snd) return NULL;

  SoundState_delete(snd);

  memset(snd, 0, sizeof(SoundState_t));

  EKEY_PNEW(snd, SoundState_t);

  snd->song_index  = -1;
  snd->frequency   = MIX_DEFAULT_FREQUENCY;

  SoundState_synchronize(snd, cd);

  return snd;
}
