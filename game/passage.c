//********************************************************************************************
//* Egoboo - passage.c
//*
//* Manages passages and doors and whatnot.  Things that impede your progress!
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

#include "passage.h"
#include "script.h"

#include "egoboo_utility.h"
#include "egoboo.h"

#include "char.inl"
#include "mesh.inl"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t open_passage( CGame * gs, Uint32 passage )
{
  // ZZ> This function makes a passage passable

  int fan_x, fan_y;
  bool_t useful = bfalse, btmp;

  if ( passage >= gs->PassList_count ) return bfalse;

  useful = !gs->PassList[passage].open;

  for ( fan_y = gs->PassList[passage].area.top; fan_y <= gs->PassList[passage].area.bottom; fan_y++ )
  {
    for ( fan_x = gs->PassList[passage].area.left; fan_x <= gs->PassList[passage].area.right; fan_x++ )
    {
      btmp = mesh_fan_clear_bits( gs, fan_x, fan_y, MPDFX_IMPASS | MPDFX_WALL | gs->PassList[passage].mask );
      useful = useful || btmp;
    }
  }

  gs->PassList[passage].open = btrue;

  return useful;
}

//--------------------------------------------------------------------------------------------
bool_t break_passage( CGame * gs, Uint32 passage, Uint16 starttile, Uint16 frames, Uint16 become, Uint32 meshfxor, Sint32 *pix, Sint32 *piy )
{
  // ZZ> This function breaks the tiles of a passage if there is a character standing
  //     on 'em...  Turns the tiles into damage terrain if it reaches last frame.

  int fan_x, fan_y;
  Uint16 tile, endtile;
  bool_t useful = bfalse;
  CHR_REF character;
  Chr * pchr;

  ScriptInfo * slist = CGame_getScriptInfo(gs); 

  if ( passage >= gs->PassList_count ) return useful;

  endtile = starttile + frames - 1;
  for ( character = 0; character < MAXCHR; character++ )
  {
    if ( !VALID_CHR( gs->ChrList,  character ) || chr_in_pack( gs->ChrList, MAXCHR, character ) ) continue;
    pchr = gs->ChrList + character;

    if ( pchr->weight > 20 && pchr->flyheight == 0 && pchr->pos.z < ( pchr->level + 20 ) && !chr_attached( gs->ChrList, MAXCHR, character ) )
    {
      if ( passage_check( gs, character, passage, NULL ) )
      {
        fan_x = MESH_FLOAT_TO_FAN( pchr->pos.x );
        fan_y = MESH_FLOAT_TO_FAN( pchr->pos.y );

        // The character is in the passage, so might need to break...
        tile =  mesh_get_tile( gs, fan_x, fan_y );
        if ( tile >= starttile && tile < endtile )
        {
          // Remember where the hit occured...
          if(NULL != pix) *pix = pchr->pos.x;
          if(NULL != piy) *piy = pchr->pos.y;
          useful = btrue;

          // Change the tile
          tile = mesh_bump_tile( gs, fan_x, fan_y );
          if ( tile == endtile )
          {
            mesh_fan_add_bits( gs, fan_x, fan_y, meshfxor );
            mesh_set_tile( gs, fan_x, fan_y, become );
          }
        }
      }
    }
  }

  return useful;
}

//--------------------------------------------------------------------------------------------
void flash_passage( CGame * gs, Uint32 passage, Uint8 color )
{
  // ZZ> This function makes a passage flash white

  int fan_x, fan_y;

  if ( passage >= gs->PassList_count ) return;

  for ( fan_y = gs->PassList[passage].area.top; fan_y <= gs->PassList[passage].area.bottom; fan_y++ )
  {
    for ( fan_x = gs->PassList[passage].area.left; fan_x <= gs->PassList[passage].area.right; fan_x++ )
    {
      mesh_set_colora( gs, fan_x, fan_y, color );
    }
  }

}

//--------------------------------------------------------------------------------------------
bool_t search_tile_in_passage( CGame * gs, Uint32 passage, Uint32 tiletype, Sint32 tmpx, Sint32 tmpy, Sint32 * pix, Sint32 * piy )
{
  // ZZ> This function finds the next tile in the passage, slist->tmpx and slist->tmpy
  //     must be set first, and are set on a find...  Returns btrue or bfalse
  //     depending on if it finds one or not

  int fan_x, fan_y;

  ScriptInfo * slist = CGame_getScriptInfo(gs); 

  if ( passage >= gs->PassList_count ) return bfalse;

  // Do the first row
  fan_x = MESH_INT_TO_FAN( tmpx );
  fan_y = MESH_INT_TO_FAN( tmpy );

  if ( fan_x < gs->PassList[passage].area.left )  fan_x = gs->PassList[passage].area.left;
  if ( fan_y < gs->PassList[passage].area.top  )  fan_y = gs->PassList[passage].area.top;

  for ( /*nothing*/; fan_y <= gs->PassList[passage].area.bottom; fan_y++ )
  {
    for ( /*nothing*/; fan_x <= gs->PassList[passage].area.right; fan_x++ )
    {
      if ( tiletype == mesh_get_tile( gs, fan_x, fan_y ) )
      {
        if(NULL != pix) *pix = MESH_FAN_TO_INT( fan_x ) + ( 1 << 6 );
        if(NULL != piy) *piy = MESH_FAN_TO_INT( fan_y ) + ( 1 << 6 );
        return btrue;
      }
    }
  }

  return bfalse;
}

//--------------------------------------------------------------------------------------------
Uint16 who_is_blocking_passage( CGame * gs, Uint32 passage )
{
  // ZZ> This function returns MAXCHR if there is no character in the passage,
  //     otherwise the index of the first character found is returned...
  //     Finds living ones, then items and corpses

  CHR_REF character, foundother;

  if ( passage >= gs->PassList_count ) return MAXCHR;

  // Look at each character
  foundother = MAXCHR;
  for ( character = 0; character < MAXCHR; character++ )
  {
    if ( !VALID_CHR( gs->ChrList,  character ) || chr_in_pack( gs->ChrList, MAXCHR, character ) ) continue;

    if ( passage_check_any( gs, character, passage, NULL ) )
    {
      if ( gs->ChrList[character].alive && !gs->ChrList[character].isitem )
      {
        // Found a live one
        return character;
      }
      else
      {
        // Found something else
        foundother = character;
      }
    }
  }

  // No characters found
  return foundother;
}

//--------------------------------------------------------------------------------------------
void check_passage_music(CGame * gs)
{
  //This function checks all passages if there is a player in it, if it is, it plays a specified
  //song set in by the AI script functions

  CHR_REF character;
  Uint16 passage;

  for ( passage = 0; passage < gs->PassList_count; passage++ )
  {
    //Only check passages that have music assigned to them
    if ( INVALID_SOUND == gs->PassList[passage].music ) continue;

    // Look at each character
    for ( character = 0; character < MAXCHR; character++ )
    {
      if ( !VALID_CHR( gs->ChrList,  character ) || chr_in_pack( gs->ChrList, MAXCHR, character ) || !chr_is_player(gs, character) ) continue;

      if ( passage_check_any( gs, character, passage, NULL ) )
      {
        snd_play_music( gs->PassList[passage].music, 0, -1 );   //start music track
      }
    }
  }

}

//--------------------------------------------------------------------------------------------
Uint16 who_is_blocking_passage_ID( CGame * gs, Uint32 passage, IDSZ idsz )
{
  // ZZ> This function returns MAXCHR if there is no character in the passage who
  //     have an item with the given ID.  Otherwise, the index of the first character
  //     found is returned...  Only finds living characters...

  CHR_REF character;
  Uint16  sTmp;

  // Look at each character
  for ( character = 0; character < MAXCHR; character++ )
  {
    if ( !VALID_CHR( gs->ChrList, character ) || chr_in_pack( gs->ChrList, MAXCHR, character ) ) continue;

    if ( !gs->ChrList[character].isitem && gs->ChrList[character].alive )
    {
      if ( passage_check_any( gs, character, passage, NULL ) )
      {
        // Found a live one...  Does it have a matching item?

        // Check the pack
        sTmp  = chr_get_nextinpack( gs->ChrList, MAXCHR, character );
        while ( VALID_CHR( gs->ChrList,  sTmp ) )
        {
          if ( CAP_INHERIT_IDSZ( gs,  gs->ChrList[sTmp].model, idsz ) )
          {
            // It has the item...
            return character;
          }
          sTmp  = chr_get_nextinpack( gs->ChrList, MAXCHR, sTmp );
        }

        for ( _slot = SLOT_BEGIN; _slot < SLOT_COUNT; _slot = ( SLOT )( _slot + 1 ) )
        {
          sTmp = chr_get_holdingwhich( gs->ChrList, MAXCHR, character, _slot );
          if ( VALID_CHR( gs->ChrList,  sTmp ) && CAP_INHERIT_IDSZ( gs,  gs->ChrList[sTmp].model, idsz ) )
          {
            // It has the item...
            return character;
          }
        };
      }
    }
  }

  // No characters found
  return MAXCHR;
}

//--------------------------------------------------------------------------------------------
bool_t close_passage( CGame * gs, Uint32 passage )
{
  // ZZ> This function makes a passage impassable, and returns btrue if it isn't blocked

  int fan_x, fan_y, cnt;
  CHR_REF character;
  Uint16 numcrushed;
  Uint16 crushedcharacters[MAXCHR];
  bool_t useful = bfalse, btmp;
  Passage * ppass;

  if ( passage >= gs->PassList_count )
    return bfalse;

  ppass = gs->PassList + passage;

  if ( ppass->area.left > ppass->area.right || ppass->area.top > ppass->area.bottom )
    return bfalse;

  useful = ppass->open;

  if ( HAS_SOME_BITS( ppass->mask, MPDFX_IMPASS | MPDFX_WALL ) )
  {
    numcrushed = 0;
    for ( character = 0; character < MAXCHR; character++ )
    {
      if ( !VALID_CHR( gs->ChrList,  character ) ) continue;

      if ( passage_check( gs, character, passage, NULL ) )
      {
        if ( !gs->ChrList[character].canbecrushed )
        {
          // door cannot close
          return bfalse;
        }
        else
        {
          crushedcharacters[numcrushed] = character;
          numcrushed++;
        }
      }
    }


    // Crush any unfortunate characters
    for ( cnt = 0; cnt < numcrushed; cnt++ )
    {
      character = crushedcharacters[cnt];
      gs->ChrList[character].aistate.alert |= ALERT_CRUSHED;
    }

    useful = useful || ( numcrushed != 0 );
  }

  // Close it off
  for ( fan_y = ppass->area.top; fan_y <= ppass->area.bottom; fan_y++ )
  {
    for ( fan_x = ppass->area.left; fan_x <= ppass->area.right; fan_x++ )
    {
      btmp = mesh_fan_add_bits( gs, fan_x, fan_y, ppass->mask );
      useful = useful || btmp;
    }
  }

  ppass->open = bfalse;

  return useful;
}

//--------------------------------------------------------------------------------------------
void clear_passages(CGame * gs)
{
  // ZZ> This function clears the passage list ( for doors )

  gs->PassList_count = 0;
  gs->ShopList_count = 0;
}

//--------------------------------------------------------------------------------------------
bool_t passage_check_all( CGame * gs, CHR_REF ichr, Uint16 pass, Uint16 * powner )
{
  // BB > character ichr is completely inside passage pass

  float x_min, x_max;
  float y_min, y_max;
  bool_t retval = bfalse;

  if ( !VALID_CHR( gs->ChrList,  ichr ) || pass >= gs->PassList_count ) return retval;

  x_min = gs->ChrList[ichr].bmpdata.cv.x_min;
  x_max = gs->ChrList[ichr].bmpdata.cv.x_max;

  y_min = gs->ChrList[ichr].bmpdata.cv.x_min;
  y_max = gs->ChrList[ichr].bmpdata.cv.x_max;

  retval = ( x_min > MESH_FAN_TO_INT( gs->PassList[pass].area.left ) && x_max < MESH_FAN_TO_INT( gs->PassList[pass].area.right + 1 ) ) &&
           ( y_min > MESH_FAN_TO_INT( gs->PassList[pass].area.top ) && y_max < MESH_FAN_TO_INT( gs->PassList[pass].area.bottom + 1 ) );

  if ( retval )
  {
    signal_target( gs, gs->PassList[pass].owner, MESSAGE_ENTERPASSAGE, ichr );

    if ( NULL != powner )
    {
      *powner = gs->PassList[pass].owner;
    }
  };

  return retval;
};

//--------------------------------------------------------------------------------------------
bool_t passage_check_any( CGame * gs, CHR_REF ichr, Uint16 pass, Uint16 * powner )
{
  // BB > character ichr is partially inside passage pass

  float x_min, x_max;
  float y_min, y_max;

  if ( !VALID_CHR( gs->ChrList,  ichr ) || pass >= gs->PassList_count ) return bfalse;

  x_min = gs->ChrList[ichr].bmpdata.cv.x_min;
  x_max = gs->ChrList[ichr].bmpdata.cv.x_max;

  y_min = gs->ChrList[ichr].bmpdata.cv.y_min;
  y_max = gs->ChrList[ichr].bmpdata.cv.y_max;

  if ( x_max < MESH_FAN_TO_INT( gs->PassList[pass].area.left ) || x_min > MESH_FAN_TO_INT( gs->PassList[pass].area.right + 1 ) ) return bfalse;
  if ( y_max < MESH_FAN_TO_INT( gs->PassList[pass].area.top ) || y_min > MESH_FAN_TO_INT( gs->PassList[pass].area.bottom + 1 ) ) return bfalse;

  signal_target( gs, gs->PassList[pass].owner, MESSAGE_ENTERPASSAGE, ichr );

  if ( NULL != powner )
  {
    *powner = gs->PassList[pass].owner;
  }

  return btrue;
};

//--------------------------------------------------------------------------------------------
bool_t passage_check( CGame * gs, CHR_REF ichr, Uint16 pass, Uint16 * powner )
{
  // BB > character ichr's center is inside passage pass

  bool_t retval = bfalse;

  if ( !VALID_CHR( gs->ChrList,  ichr ) || pass >= gs->PassList_count ) return retval;

  retval = ( gs->ChrList[ichr].pos.x > MESH_FAN_TO_INT( gs->PassList[pass].area.left ) && gs->ChrList[ichr].pos.x < MESH_FAN_TO_INT( gs->PassList[pass].area.right + 1 ) ) &&
           ( gs->ChrList[ichr].pos.y > MESH_FAN_TO_INT( gs->PassList[pass].area.top ) && gs->ChrList[ichr].pos.y < MESH_FAN_TO_INT( gs->PassList[pass].area.bottom + 1 ) );

  if ( retval )
  {
    signal_target( gs, gs->PassList[pass].owner, MESSAGE_ENTERPASSAGE, ichr );

    if ( NULL != powner )
    {
      *powner = gs->PassList[pass].owner;
    }
  };

  return retval;
};
//--------------------------------------------------------------------------------------------
Uint32 ShopList_add( CGame * gs, Uint16 owner, Uint32 passage )
{
  // ZZ> This function creates a shop passage

  Uint32 shop_passage = MAXPASS;

  if ( passage < gs->PassList_count && gs->ShopList_count < MAXPASS )
  {
    shop_passage = gs->ShopList_count;

    // The passage exists...
    gs->ShopList[shop_passage].passage = passage;
    gs->PassList[passage].owner        = owner;
    gs->ShopList[shop_passage].owner   = owner;  // Assume the owner is alive
    gs->ShopList_count++;
  }

  return shop_passage;
}

//--------------------------------------------------------------------------------------------
Uint32 PassList_add( CGame * gs, int tlx, int tly, int brx, int bry, bool_t open, Uint32 mask )
{
  // ZZ> This function creates a passage area

  Uint32 passage = MAXPASS;

  // clip the passage borders
  tlx = mesh_clip_fan_x( &(gs->mesh), tlx );
  tly = mesh_clip_fan_x( &(gs->mesh), tly );

  brx = mesh_clip_fan_x( &(gs->mesh), brx );
  bry = mesh_clip_fan_x( &(gs->mesh), bry );

  if ( gs->PassList_count < MAXPASS )
  {
    passage = gs->PassList_count;
    gs->PassList_count++;

    //gs->PassList[passage].area.left   = MIN( tlx, brx );
    //gs->PassList[passage].area.top    = MIN( tly, bry );
    //gs->PassList[passage].area.right  = MAX( tlx, brx );
    //gs->PassList[passage].area.bottom = MAX( tly, bry );

    // allow for "inverted" passages
    gs->PassList[passage].area.left   = tlx;
    gs->PassList[passage].area.top    = tly;
    gs->PassList[passage].area.right  = brx;
    gs->PassList[passage].area.bottom = bry;

    gs->PassList[passage].mask        = mask;
    gs->PassList[passage].music       = INVALID_SOUND;     //Set no song as default

    gs->PassList[passage].open = open;
  }

  return passage;
}

//--------------------------------------------------------------------------------------------
void PassList_load( CGame * gs, char *modname )
{
  // ZZ> This function reads the passage file

  STRING newloadname;
  int tlx, tly, brx, bry;
  bool_t open;
  Uint32 mask;
  FILE *fileread;

  // Reset all of the old passages
  clear_passages( gs );

  // Load the file
  snprintf( newloadname, sizeof( newloadname ), "%s%s" SLASH_STRING "%s", modname, CData.gamedat_dir, CData.passage_file );
  fileread = fs_fileOpen( PRI_NONE, NULL, newloadname, "r" );
  if ( NULL == fileread ) return;

  while ( fgoto_colon_yesno( fileread ) )
  {
    fscanf( fileread, "%d%d%d%d", &tlx, &tly, &brx, &bry );

    open = fget_bool( fileread );

    // set basic wall flags
    mask = MPDFX_IMPASS | MPDFX_WALL;

    // "Shoot Through"
    if ( fget_bool( fileread ) )
      mask = MPDFX_IMPASS;

    // "Slippy Close"
    if ( fget_bool( fileread ) )
      mask = MPDFX_SLIPPY;

    PassList_add( gs, tlx, tly, brx, bry, open, mask );
  }
  fs_fileClose( fileread );


};


//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Passage * Passage_new(Passage *ppass) 
{ 
  //fprintf( stdout, "Passage_new()\n");

  if(NULL==ppass) return ppass; 

  memset(ppass, 0, sizeof(Passage)); 

  ppass->music = INVALID_SOUND;
  ppass->open  = btrue;
  ppass->owner = MAXCHR;
  
  return ppass; 
};

//--------------------------------------------------------------------------------------------
bool_t Passage_delete(Passage *ppass) 
{ 
  if(NULL==ppass) return bfalse; 

  ppass->music = INVALID_SOUND;
  ppass->open  = btrue;
  ppass->owner = MAXCHR;
  
  return btrue; 
};

//--------------------------------------------------------------------------------------------
Passage * Passage_renew(Passage *ppass) 
{ 
  Passage_delete(ppass);
  return Passage_new(ppass);
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Shop * Shop_new(Shop *pshop) 
{ 
  //fprintf( stdout, "Shop_new()\n");

  if(NULL==pshop) return pshop; 

  pshop->passage = MAXPASS; 
  pshop->owner   = MAXCHR; 

  return pshop; 
};

//--------------------------------------------------------------------------------------------
bool_t Shop_delete(Shop *pshop) 
{ 
  if(NULL==pshop) return bfalse; 

  pshop->passage = MAXPASS; 
  pshop->owner   = MAXCHR; 

  return btrue; 
};

//--------------------------------------------------------------------------------------------
Shop * Shop_renew(Shop *pshop) 
{ 
  Shop_delete(pshop);
  return Shop_new(pshop);
};