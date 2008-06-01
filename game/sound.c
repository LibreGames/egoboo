//********************************************************************************************
//* Egoboo - sound.c
//*
//* Implements music and sound effects using SDL_mixer.
//*
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

#include "sound.h"
#include "Log.h"
#include "camera.h"

#include "egoboo_utility.h"
#include "egoboo.h"

#include "particle.inl"
#include "char.inl"

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

SoundState sndState = { bfalse };


//------------------------------------------------------------------------------
//This function enables the use of SDL_Mixer functions, returns btrue if success
bool_t snd_initialize(SoundState * ss, ConfigData * cd)
{
  if(NULL == ss || NULL == cd) return bfalse;

  SoundState_new(ss, cd);

  if ( !ss->mixer_loaded )
  {
    log_info( "Initializing SDL_mixer audio services version %i.%i.%i... ", MIX_MAJOR_VERSION, MIX_MINOR_VERSION, MIX_PATCHLEVEL);

    sound_state_synchronize( ss, cd );

    if( !snd_reopen(ss) )
    {
      log_message( "Failed!\n" );
      log_error( "Unable to initialize audio: %s\n", Mix_GetError() );
    }
    else 
    {
      ss->mixer_loaded = btrue;
      log_message( "Succeeded!\n" );
    }

  }
  
  return ss->mixer_loaded;
}

//------------------------------------------------------------------------------
bool_t snd_quit(SoundState * ss)
{
  if(NULL == ss) return bfalse;

  if( ss->mixer_loaded )
  { 
    Mix_CloseAudio();
    ss->mixer_loaded = bfalse;
  };

  return !ss->mixer_loaded;
}


//------------------------------------------------------------------------------
//SOUND-------------------------------------------------------------------------
//------------------------------------------------------------------------------

void snd_apply_mods( int channel, float intensity, vect3 snd_pos, vect3 ear_pos, Uint16 ear_turn_lr  )
{
  // BB > This functions modifies an already playing 3d sound sound according position, orientation, etc.
  //      Modeled after physical parameters, but does not model doppler shift...
  float dist_xyz2, dist_xy2, volume;
  float vl, vr, dx, dy;
  int vol_left, vol_right;
  const float reverbdistance = 128 * 2.5;

  if ( !sndState.soundActive || INVALID_CHANNEL == channel ) return;

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
int snd_play_sound( GameState * gs, float intensity, vect3 pos, Mix_Chunk *loadedwave, int loops, int whichobject, int soundnumber)
{
  // ZF> This function plays a specified sound
  // (Or returns -1 (INVALID_CHANNEL) if it failed to play the sound)
  int channel;

  if( !sndState.soundActive ) return INVALID_CHANNEL;

  if ( loadedwave == NULL )
  {
    log_warning( "Sound file not correctly loaded (Not found?) - Object \"%s\" is trying to play sound%i.wav\n", gs->CapList[gs->ChrList[whichobject].model].classname, soundnumber );
    return INVALID_CHANNEL;
  }

  channel = Mix_PlayChannel( -1, loadedwave, loops );

  if( INVALID_CHANNEL == channel )
  {
    if(whichobject < 0)
    {
      log_warning( "All sound channels are currently in use. Global sound %d NOT playing\n", -whichobject );
    }
    else
    {
      log_warning( "All sound channels are currently in use. Sound is NOT playing - Object \"%s\" is trying to play sound%i.wav\n", gs->CapList[gs->ChrList[whichobject].model].classname, soundnumber );
    };
  }
  else
  {
    snd_apply_mods( channel, intensity, pos, GCamera.trackpos, GCamera.turn_lr);
  };

  return channel;
}

//--------------------------------------------------------------------------------------------
int snd_play_particle_sound( GameState * gs, float intensity, PRT_REF particle, Sint8 sound )
{
  int channel = INVALID_CHANNEL;
  Uint16 imdl;

  //This function plays a sound effect for a particle
  if ( INVALID_SOUND == sound ) return channel;
  if( !sndState.soundActive ) return channel;

  //Play local sound or else global (coins for example)
  imdl = gs->PrtList[particle].model;
  if ( VALID_MDL(imdl) )
  {
    channel = snd_play_sound( gs, intensity, gs->PrtList[particle].pos, gs->CapList[imdl].wavelist[sound], 0, imdl, sound );
  }
  else
  {
    channel = snd_play_sound( gs, intensity, gs->PrtList[particle].pos, sndState.mc_list[sound], 0, -sound, sound );
  };

  return channel;
}


//------------------------------------------------------------------------------
void snd_stop_sound( int whichchannel )
{
  // ZF> This function is used for stopping a looped sound, but can be used to stop
  // a particular sound too. If whichchannel is -1, all playing channels will fade out.

  if( sndState.mixer_loaded )
  {
    Mix_FadeOutChannel( whichchannel, 400 ); //400 ms is nice
  }
}

//------------------------------------------------------------------------------
//Music Stuff-------------------------------------------------------------------
//------------------------------------------------------------------------------

bool_t snd_unload_music(SoundState * ss)
{
  int cnt;

  if(NULL == ss) return bfalse;
  if(!ss->music_loaded) return btrue;

  for(cnt=0; cnt<MAXPLAYLISTLENGTH; cnt++)
  {
    if(NULL != ss->mus_list[cnt])
    {
      Mix_FreeMusic( ss->mus_list[cnt] );
      ss->mus_list[cnt] = NULL;
    }
  }

  ss->music_loaded = bfalse;

  return btrue;
}



//------------------------------------------------------------------------------
void snd_play_music( int songnumber, int fadetime, int loops )
{
  // ZF> This functions plays a specified track loaded into memory
  if ( sndState.musicActive && sndState.music_loaded && sndState.song_index != songnumber)
  {
    Mix_FadeOutMusic( fadetime );
    Mix_PlayMusic( sndState.mus_list[songnumber], loops );
    sndState.song_loops = loops;
    sndState.song_index = songnumber;
  }
}

//------------------------------------------------------------------------------
void snd_stop_music(int fadetime)
{
  //ZF> This function sets music track to pause

  Mix_FadeOutMusic(fadetime);
}


//------------------------------------------------------------------------------
SoundState * SoundState_new(SoundState * ss, ConfigData * cd)
{
  // BB > do a raw initialization of the sound state

  fprintf( stdout, "SoundState_new()\n");

  if(NULL == ss) return NULL;

  if( ss->initialized ) return ss;
 
  memset(ss, 0, sizeof(SoundState));

  ss->song_index  = -1;
  ss->initialized = btrue;
  ss->frequency   = MIX_DEFAULT_FREQUENCY;

  sound_state_synchronize(ss, cd);

  return ss;
}

//------------------------------------------------------------------------------
bool_t sound_state_synchronize(SoundState * ss, ConfigData * cd)
{
  // BB > update the current sound state from the config data

  if(NULL == ss || NULL == cd) return bfalse;

  ss->soundActive   = cd->allow_sound;
  ss->musicActive   = cd->allow_music;

  ss->music_volume  = cd->musicvolume;
  ss->sound_volume  = cd->soundvolume;

  ss->channel_count = cd->maxsoundchannel;
  ss->buffersize    = cd->buffersize;

  return btrue;
}

//------------------------------------------------------------------------------
bool_t snd_reopen(SoundState * ss)
{
  // BB > an attempt to automatically update the SDL_mixer state. This could probably be optimized
  //      since not everything requires restarting the mixer.

  bool_t paused;

  if(NULL == ss) return bfalse;

  // ???assume that pausing and then unpausing the music will preserve the song location???
  paused = (-1 != ss->song_index) && Mix_PausedMusic();
  Mix_PauseMusic();

  if( ss->mixer_loaded )
  { 
    Mix_CloseAudio();
    ss->mixer_loaded = bfalse;
  };

  ss->mixer_loaded = ( 0 == Mix_OpenAudio( ss->frequency, MIX_DEFAULT_FORMAT, 2, ss->buffersize ));

  if(ss->mixer_loaded)
  {
    Mix_AllocateChannels( ss->channel_count );
    Mix_VolumeMusic( ss->music_volume );
  }

  if(ss->mixer_loaded && ss->musicActive)
  {
    if(paused)
    {
      // try to restart the song
      Mix_PlayMusic( ss->mus_list[ss->song_index], ss->song_loops );
    }
    else
    {
      Mix_ResumeMusic();
    }
  }

  return ss->mixer_loaded;
}
