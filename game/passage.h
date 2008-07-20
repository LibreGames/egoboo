#pragma once

#include "egoboo_types.h"

struct CGame_t;

#define PASSLST_COUNT             256                     // Maximum number of passages ( mul 32 )
#define NOOWNER             INVALID_CHR

//Passages
typedef struct Passage_t
{
  egoboo_key ekey;

  IRect   area;            // Passage positions
  int     music;           // Music track appointed to the specific passage
  Uint32  mask;
  bool_t  open;            // Is the passage open?
  CHR_REF owner;           // Who controls the passage?
} Passage;

Passage * Passage_new(Passage *ppass);
bool_t    Passage_delete(Passage *ppass);
Passage * Passage_renew(Passage *ppass);

typedef struct Shop_t
{
  egoboo_key ekey;
  PASS_REF passage;  // The passage number
  CHR_REF  owner;    // Who gets the gold?
} Shop;

Shop * Shop_new(Shop *pshop);
bool_t Shop_delete(Shop *pshop);
Shop * Shop_renew(Shop *pshop);

// Passage control functions
bool_t open_passage( struct CGame_t * gs,  PASS_REF passage );
void check_passage_music(struct CGame_t * gs );
void flash_passage( struct CGame_t * gs,  PASS_REF passage, Uint8 color );
CHR_REF who_is_blocking_passage( struct CGame_t * gs,  PASS_REF passage );
CHR_REF who_is_blocking_passage_ID( struct CGame_t * gs,  PASS_REF passage, IDSZ idsz );
bool_t close_passage( struct CGame_t * gs,  Uint32 passage );
void clear_passages(struct CGame_t * gs );
Uint32 ShopList_add( struct CGame_t * gs,  CHR_REF owner, PASS_REF passage );
Uint32 PassList_add( struct CGame_t * gs,  int tlx, int tly, int brx, int bry, bool_t open, Uint32 mask );

bool_t break_passage( struct CGame_t * gs, PASS_REF passage, Uint16 starttile, Uint16 frames, Uint16 become, Uint32 meshfxor, Sint32 *pix, Sint32 *piy );

bool_t search_tile_in_passage( struct CGame_t * gs, PASS_REF passage, Uint32 tiletype, Sint32 tmpx, Sint32 tmpy, Sint32 * pix, Sint32 * piy );
void PassList_load( struct CGame_t * gs, char *modname );


bool_t passage_check_any( struct CGame_t * gs, CHR_REF ichr, PASS_REF pass, Uint16 * powner );
bool_t passage_check_all( struct CGame_t * gs, CHR_REF ichr, PASS_REF pass, Uint16 * powner );
bool_t passage_check( struct CGame_t * gs, CHR_REF ichr, PASS_REF pass, Uint16 * powner );

