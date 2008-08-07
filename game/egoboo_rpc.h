#pragma once

#include "char.h"
#include "enchant.h"
#include "particle.h"

// this is the header file for the RPC ("remote procedure call") functions that are necessary for the
// client/server networking

// the spawning is the most important thing that must be handled through RPCs
void snd_prt_spawn( PRT_SPAWN_INFO si );
void rec_prt_spawn( PRT_SPAWN_INFO si );

void snd_chr_spawn( CHR_SPAWN_INFO si );
void rec_chr_spawn( CHR_SPAWN_INFO si );


void snd_enc_spawn( ENC_SPAWN_INFO si );
void rec_enc_spawn( ENC_SPAWN_INFO si );


// Because of the way that egoboo is set up there are several modifications to these objects that
// get applied after they are spawned

void snd_prt_mod_XX( PRT_REF prt, int val );
void rec_prt_mod_XX( PRT_REF prt, int val );

void snd_chr_mod_XX( CHR_REF prt, int val );
void rec_chr_mod_XX( CHR_REF prt, int val );