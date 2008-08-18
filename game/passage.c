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
#include "sound.h"
#include "file_common.h"

#include "egoboo_utility.h"
#include "egoboo.h"

#include "char.inl"
#include "mesh.inl"
#include "egoboo_types.inl"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
bool_t passage_open( Game_t * gs, PASS_REF passage )
{
  // ZZ> This function makes a passage passable

  int fan_x, fan_y;
  bool_t useful = bfalse, btmp;

  Mesh_t * pmesh = Game_getMesh(gs);

  if ( passage >= gs->PassList_count ) return bfalse;
  useful = !gs->PassList[passage].open;

  for ( fan_y = gs->PassList[passage].area.top; fan_y <= gs->PassList[passage].area.bottom; fan_y++ )
  {
    for ( fan_x = gs->PassList[passage].area.left; fan_x <= gs->PassList[passage].area.right; fan_x++ )
    {
      btmp = mesh_fan_clear_bits( pmesh, fan_x, fan_y, MPDFX_IMPASS | MPDFX_WALL | gs->PassList[passage].mask );
      useful = useful || btmp;
    }
  }

  gs->PassList[passage].open = btrue;

  return useful;
}

//--------------------------------------------------------------------------------------------
bool_t passage_break_tiles( Game_t * gs, PASS_REF passage, Uint16 starttile, Uint16 frames, Uint16 become, Uint32 meshfxor, Sint32 *pix, Sint32 *piy )
{
  // ZZ> This function breaks the tiles of a passage if there is a character standing
  //     on 'em...  Turns the tiles into damage terrain if it reaches last frame.

  int fan_x, fan_y;
  Uint16 tile, endtile;
  bool_t useful = bfalse;
  CHR_REF character;
  Chr_t * pchr;

  ScriptInfo_t * slist = Game_getScriptInfo(gs);
  Mesh_t       * pmesh = Game_getMesh(gs);

  if ( passage >= gs->PassList_count ) return useful;

  endtile = starttile + frames - 1;
  for ( character = 0; character < CHRLST_COUNT; character++ )
  {
    if ( !ACTIVE_CHR( gs->ChrList,  character ) || chr_in_pack( gs->ChrList, CHRLST_COUNT, character ) ) continue;
    pchr = gs->ChrList + character;

    if ( pchr->weight > 20 && pchr->flyheight == 0 && pchr->ori.pos.z < ( pchr->level + 20 ) && !chr_attached( gs->ChrList, CHRLST_COUNT, character ) )
    {
      if ( passage_check( gs, character, passage, NULL ) )
      {
        fan_x = MESH_FLOAT_TO_FAN( pchr->ori.pos.x );
        fan_y = MESH_FLOAT_TO_FAN( pchr->ori.pos.y );

        // The character is in the passage, so might need to break...
        tile =  mesh_get_tile( pmesh, fan_x, fan_y );
        if ( tile >= starttile && tile < endtile )
        {
          // Remember where the hit occured...
          if(NULL != pix) *pix = pchr->ori.pos.x;
          if(NULL != piy) *piy = pchr->ori.pos.y;
          useful = btrue;

          // Change the tile
          tile = mesh_bump_tile( pmesh, fan_x, fan_y );
          if ( tile == endtile )
          {
            mesh_fan_add_bits( pmesh, fan_x, fan_y, meshfxor );
            mesh_set_tile( pmesh, fan_x, fan_y, become );
          }
        }
      }
    }
  }

  return useful;
}

//--------------------------------------------------------------------------------------------
void passage_flash( Game_t * gs, PASS_REF passage, Uint8 color )
{
  // ZZ> This function makes a passage flash white

  int fan_x, fan_y;

  Mesh_t * pmesh = Game_getMesh(gs);

  if ( passage >= gs->PassList_count ) return;

  for ( fan_y = gs->PassList[passage].area.top; fan_y <= gs->PassList[passage].area.bottom; fan_y++ )
  {
    for ( fan_x = gs->PassList[passage].area.left; fan_x <= gs->PassList[passage].area.right; fan_x++ )
    {
      mesh_set_colora( pmesh, fan_x, fan_y, color );
    }
  }

}

//--------------------------------------------------------------------------------------------
bool_t passage_search_tile( Game_t * gs, PASS_REF passage, Uint32 tiletype, Sint32 tmpx, Sint32 tmpy, Sint32 * pix, Sint32 * piy )
{
  // ZZ> This function finds the next tile in the passage, slist->tmpx and slist->tmpy
  //     must be set first, and are set on a find...  Returns btrue or bfalse
  //     depending on if it finds one or not

  int fan_x, fan_y;

  ScriptInfo_t * slist = Game_getScriptInfo(gs);
  Mesh_t       * pmesh = Game_getMesh(gs);

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
      if ( tiletype == mesh_get_tile( pmesh, fan_x, fan_y ) )
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
CHR_REF passage_search_blocking( Game_t * gs, PASS_REF passage )
{
  // ZZ> This function returns CHRLST_COUNT if there is no character in the passage,
  //     otherwise the index of the first character found is returned...
  //     Finds living ones, then items and corpses

  CHR_REF character, foundother;

  if ( passage >= gs->PassList_count ) return INVALID_CHR;

  // Look at each character
  foundother = INVALID_CHR;
  for ( character = 0; character < CHRLST_COUNT; character++ )
  {
    if ( !ACTIVE_CHR( gs->ChrList,  character ) || chr_in_pack( gs->ChrList, CHRLST_COUNT, character ) ) continue;

    if ( passage_check_any( gs, character, passage, NULL ) )
    {
      if ( gs->ChrList[character].alive && !gs->ChrList[character].prop.isitem )
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
void passage_check_music(Game_t * gs)
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
    for ( character = 0; character < CHRLST_COUNT; character++ )
    {
      if ( !ACTIVE_CHR( gs->ChrList,  character ) || chr_in_pack( gs->ChrList, CHRLST_COUNT, character ) || !chr_is_player(gs, character) ) continue;

      if ( passage_check_any( gs, character, passage, NULL ) )
      {
        snd_play_music( gs->PassList[passage].music, 0, -1 );   //start music track
      }
    }
  }

}

//--------------------------------------------------------------------------------------------
CHR_REF passage_search_blocking_ID( Game_t * gs, PASS_REF passage, IDSZ idsz )
{
  // ZZ> This function returns CHRLST_COUNT if there is no character in the passage who
  //     have an item with the given ID.  Otherwise, the index of the first character
  //     found is returned...  Only finds living characters...

  CHR_REF character;
  CHR_REF  sTmp;

  // Look at each character
  for ( character = 0; character < CHRLST_COUNT; character++ )
  {
    if ( !ACTIVE_CHR( gs->ChrList, character ) || chr_in_pack( gs->ChrList, CHRLST_COUNT, character ) ) continue;

    if ( !gs->ChrList[character].prop.isitem && gs->ChrList[character].alive )
    {
      if ( passage_check_any( gs, character, passage, NULL ) )
      {
        // Found a live one...  Does it have a matching item?

        // Check the pack
        sTmp  = chr_get_nextinpack( gs->ChrList, CHRLST_COUNT, character );
        while ( ACTIVE_CHR( gs->ChrList,  sTmp ) )
        {
          if ( CAP_INHERIT_IDSZ( gs, ChrList_getRCap(gs, sTmp), idsz ) )
          {
            // It has the item...
            return character;
          }
          sTmp  = chr_get_nextinpack( gs->ChrList, CHRLST_COUNT, sTmp );
        }

        for ( _slot = SLOT_BEGIN; _slot < SLOT_COUNT; _slot = ( SLOT )( _slot + 1 ) )
        {
          sTmp = chr_get_holdingwhich( gs->ChrList, CHRLST_COUNT, character, _slot );
          if ( ACTIVE_CHR( gs->ChrList,  sTmp ) && CAP_INHERIT_IDSZ( gs,  ChrList_getRCap(gs, sTmp), idsz ) )
          {
            // It has the item...
            return character;
          }
        };
      }
    }
  }

  // No characters found
  return INVALID_CHR;
}

//--------------------------------------------------------------------------------------------
bool_t passage_close( Game_t * gs, Uint32 passage )
{
  // ZZ> This function makes a passage impassable, and returns btrue if it isn't blocked

  int fan_x, fan_y, cnt;
  CHR_REF character;
  Uint16 numcrushed;
  CHR_REF crushedcharacters[CHRLST_COUNT];
  bool_t useful = bfalse, btmp;
  Passage_t * ppass;

  Mesh_t * pmesh = Game_getMesh(gs);

  if ( passage >= gs->PassList_count )
    return bfalse;

  ppass = gs->PassList + passage;

  if ( ppass->area.left > ppass->area.right || ppass->area.top > ppass->area.bottom )
    return bfalse;

  useful = ppass->open;

  if ( HAS_SOME_BITS( ppass->mask, MPDFX_IMPASS | MPDFX_WALL ) )
  {
    numcrushed = 0;
    for ( character = 0; character < CHRLST_COUNT; character++ )
    {
      if ( !ACTIVE_CHR( gs->ChrList,  character ) ) continue;

      if ( passage_check( gs, character, passage, NULL ) )
      {
        if ( !gs->ChrList[character].prop.canbecrushed )
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
      btmp = mesh_fan_add_bits( pmesh, fan_x, fan_y, ppass->mask );
      useful = useful || btmp;
    }
  }

  ppass->open = bfalse;

  return useful;
}

//--------------------------------------------------------------------------------------------
void clear_all_passages(Game_t * gs)
{
  // ZZ> This function clears the passage list ( for doors )

  gs->PassList_count = 0;
  gs->ShopList_count = 0;
}

//--------------------------------------------------------------------------------------------
bool_t passage_check_all( Game_t * gs, CHR_REF ichr, Uint16 pass, CHR_REF * powner )
{
  // BB > character ichr is completely inside passage pass

  float x_min, x_max;
  float y_min, y_max;
  bool_t retval = bfalse;
  CHR_REF     iowner;
  Chr_t     * pchr;
  CVolume_t * pcv;
  IRect_t   * parea;
  
  if ( !ACTIVE_CHR( gs->ChrList,  ichr ) || pass >= gs->PassList_count ) return retval;

  iowner = gs->PassList[pass].owner;
  if ( INVALID_CHR == iowner )
  {
    if ( NULL != powner )
    {
      *powner = INVALID_CHR;
    }
    return bfalse;
  };

  pchr = gs->ChrList + ichr;
  pcv  = &(pchr->bmpdata.cv);
  parea = &(gs->PassList[pass].area);

  // check whether the character is in the passage now
  x_min = pcv->x_min;
  x_max = pcv->x_max;

  y_min = pcv->y_min;
  y_max = pcv->y_max;

  retval = ( x_min > MESH_FAN_TO_INT( parea->left ) && x_max < MESH_FAN_TO_INT( parea->right + 1 ) ) &&
           ( y_min > MESH_FAN_TO_INT( parea->top )  && y_max < MESH_FAN_TO_INT( parea->bottom + 1 ) );

  if ( retval )
  {
    // check whether it was in the passage before
    x_min = pcv->x_min + (pchr->ori_old.pos.x - pchr->ori.pos.x) ;
    x_max = pcv->x_max + (pchr->ori_old.pos.x - pchr->ori.pos.x) ;

    y_min = pcv->y_min + (pchr->ori_old.pos.y - pchr->ori.pos.y);
    y_max = pcv->y_max + (pchr->ori_old.pos.y - pchr->ori.pos.y);

    retval = ( x_min > MESH_FAN_TO_INT( parea->left ) && x_max < MESH_FAN_TO_INT( parea->right + 1 ) ) &&
             ( y_min > MESH_FAN_TO_INT( parea->top )  && y_max < MESH_FAN_TO_INT( parea->bottom + 1 ) );

    if( retval )
    {
      signal_target( gs, 0, iowner, SIGNAL_ENTERPASSAGE, REF_TO_INT(ichr) );

      if ( NULL != powner )
      {
        *powner = iowner;
      }
    }
  };

  return retval;
};

//--------------------------------------------------------------------------------------------
bool_t passage_check_any( Game_t * gs, CHR_REF ichr, Uint16 pass, CHR_REF * powner )
{
  // BB > character ichr is partially inside passage pass

  float x_min, x_max;
  float y_min, y_max;
  CHR_REF     iowner;
  Chr_t     * pchr;
  CVolume_t * pcv;
  IRect_t   * parea;

  if ( !ACTIVE_CHR( gs->ChrList,  ichr ) || pass >= gs->PassList_count ) return bfalse;

  iowner = gs->PassList[pass].owner;
  if ( INVALID_CHR == iowner )
  {
    if ( NULL != powner )
    {
      *powner = INVALID_CHR;
    }
    return bfalse;
  };

  pchr = gs->ChrList + ichr;
  pcv  = &(pchr->bmpdata.cv);
  parea = &(gs->PassList[pass].area);

  // check whether it is intersecting the passage now
  x_min = pcv->x_min;
  x_max = pcv->x_max;

  y_min = pcv->y_min;
  y_max = pcv->y_max;

  if ( x_max < MESH_FAN_TO_INT( parea->left ) || x_min > MESH_FAN_TO_INT( parea->right + 1 ) ) return bfalse;
  if ( y_max < MESH_FAN_TO_INT( parea->top )  || y_min > MESH_FAN_TO_INT( parea->bottom + 1 ) ) return bfalse;

  // check whether it was intersecting the passage before
  x_min = pcv->x_min + (pchr->ori_old.pos.x - pchr->ori.pos.x) ;
  x_max = pcv->x_max + (pchr->ori_old.pos.x - pchr->ori.pos.x) ;

  y_min = pcv->y_min + (pchr->ori_old.pos.y - pchr->ori.pos.y);
  y_max = pcv->y_max + (pchr->ori_old.pos.y - pchr->ori.pos.y);

  if ( x_max < MESH_FAN_TO_INT( parea->left ) || x_min > MESH_FAN_TO_INT( parea->right + 1 ) ) return bfalse;
  if ( y_max < MESH_FAN_TO_INT( parea->top )  || y_min > MESH_FAN_TO_INT( parea->bottom + 1 ) ) return bfalse;

  signal_target( gs, 0, iowner, SIGNAL_ENTERPASSAGE, REF_TO_INT(ichr) );

  if ( NULL != powner )
  {
    *powner = iowner;
  }

  return btrue;
};

//--------------------------------------------------------------------------------------------
bool_t passage_check( Game_t * gs, CHR_REF ichr, Uint16 pass, CHR_REF * powner )
{
  // BB > character ichr's center is inside passage pass

  bool_t retval = bfalse;
  Orientation_t * pori;
  IRect_t * parea;
  CHR_REF   iowner;
  Chr_t   * pchr;

  if ( !ACTIVE_CHR( gs->ChrList,  ichr ) || pass >= gs->PassList_count ) return bfalse;

  iowner = gs->PassList[pass].owner;
  if ( INVALID_CHR == iowner )
  {
    if ( NULL != powner )
    {
      *powner = INVALID_CHR;
    }
    return bfalse;
  };

  pchr = gs->ChrList + ichr;

  pori = &(pchr->ori);
  parea = &(gs->PassList[pass].area);

  retval = ( pori->pos.x > MESH_FAN_TO_INT( parea->left ) && pori->pos.x < MESH_FAN_TO_INT( parea->right + 1 ) ) &&
           ( pori->pos.y > MESH_FAN_TO_INT( parea->top ) && pori->pos.y < MESH_FAN_TO_INT( parea->bottom + 1 ) );

  if ( retval )
  {
    pori = &(pchr->ori_old);

    retval = ( pori->pos.x > MESH_FAN_TO_INT( parea->left ) && pori->pos.x < MESH_FAN_TO_INT( parea->right + 1 ) ) &&
            ( pori->pos.y > MESH_FAN_TO_INT( parea->top ) && pori->pos.y < MESH_FAN_TO_INT( parea->bottom + 1 ) );

    if ( retval )
    {
      signal_target( gs, 0, iowner, SIGNAL_ENTERPASSAGE, REF_TO_INT(ichr) );

      if ( NULL != powner )
      {
        *powner = iowner;
      }
    }
  };

  return retval;
};
//--------------------------------------------------------------------------------------------
Uint32 ShopList_add( Game_t * gs, CHR_REF owner, PASS_REF passage )
{
  // ZZ> This function creates a shop passage

  Uint32 shop_passage = INVALID_PASS;

  if ( passage < gs->PassList_count && gs->ShopList_count < PASSLST_COUNT )
  {
    shop_passage = gs->ShopList_count;
    gs->ShopList_count++;

    // The passage exists...
    gs->PassList[passage].owner        = owner;
    gs->ShopList[shop_passage].passage = passage;
    gs->ShopList[shop_passage].owner   = owner;  // Assume the owner is alive
  }

  return shop_passage;
}

//--------------------------------------------------------------------------------------------
Uint32 PassList_add( Game_t * gs, int tlx, int tly, int brx, int bry, bool_t open, Uint32 mask )
{
  // ZZ> This function creates a passage area

  Uint32   passage = PASSLST_COUNT;
  Mesh_t * pmesh = Game_getMesh(gs);

  // clip the passage borders
  tlx = mesh_clip_fan_x( &(pmesh->Info), tlx );
  tly = mesh_clip_fan_x( &(pmesh->Info), tly );

  brx = mesh_clip_fan_x( &(pmesh->Info), brx );
  bry = mesh_clip_fan_x( &(pmesh->Info), bry );

  if ( gs->PassList_count < PASSLST_COUNT )
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
void PassList_load( Game_t * gs, char *modname )
{
  // ZZ> This function reads the passage file

  STRING newloadname;
  int tlx, tly, brx, bry;
  bool_t open;
  Uint32 mask;
  FILE *fileread;

  // Reset all of the old passages
  clear_all_passages( gs );

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
Passage_t * Passage_new(Passage_t *ppass)
{
  //fprintf( stdout, "Passage_new()\n");

  if(NULL ==ppass) return ppass;

  Passage_delete( ppass );

  memset(ppass, 0, sizeof(Passage_t));

  EKEY_PNEW( ppass, Passage_t );

  ppass->music = INVALID_SOUND;
  ppass->open  = btrue;
  ppass->owner = INVALID_CHR;

  return ppass;
};

//--------------------------------------------------------------------------------------------
bool_t Passage_delete(Passage_t *ppass)
{
  if(NULL ==ppass) return bfalse;
  if(!EKEY_PVALID( ppass )) return btrue;

  EKEY_PINVALIDATE(ppass);

  ppass->music = INVALID_SOUND;
  ppass->open  = btrue;
  ppass->owner = INVALID_CHR;

  return btrue;
};

//--------------------------------------------------------------------------------------------
Passage_t * Passage_renew(Passage_t *ppass)
{
  Passage_delete(ppass);
  return Passage_new(ppass);
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
Shop_t * Shop_new(Shop_t *pshop)
{
  //fprintf( stdout, "Shop_new()\n");

  if(NULL ==pshop) return pshop;

  Shop_delete( pshop );

  memset( pshop, 0, sizeof(Shop_t) );

  EKEY_PNEW( pshop, Shop_t );

  pshop->passage = INVALID_PASS;
  pshop->owner   = INVALID_CHR;

  return pshop;
};

//--------------------------------------------------------------------------------------------
bool_t Shop_delete(Shop_t *pshop)
{
  if(NULL ==pshop) return bfalse;
  if(!EKEY_PVALID( pshop )) return btrue;

  EKEY_PINVALIDATE(pshop);

  pshop->passage = INVALID_PASS;
  pshop->owner   = INVALID_CHR;

  return btrue;
};

//--------------------------------------------------------------------------------------------
Shop_t * Shop_renew(Shop_t *pshop)
{
  Shop_delete(pshop);
  return Shop_new(pshop);
};