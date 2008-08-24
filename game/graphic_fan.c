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
/// @brief Drawing routines for MPD files (world mesh)
/// @details Render game tiles.
/// @todo Implement OpenGL fog effects.


#include "ogl_include.h"
#include "Log.h"
#include "Frustum.h"

#include "egoboo.h"

#include <assert.h>

#include "particle.inl"
#include "mesh.inl"
#include "graphic.inl"
#include "egoboo_math.inl"
#include "egoboo_types.inl"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

RENDERLIST renderlist;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
void render_fan_ref( Uint32 fan, char tex_loaded, float level )
{
  /// @details ZZ@> This function draws a mesh fan

  Game_t * gs = Graphics_requireGame(&gfxState);
  Mesh_t * pmesh = Game_getMesh(gs);
  Graphics_Data_t * gfx = gfxState.pGfx;

  GLVertex v[MAXMESHVERTICES];
  Uint16 commands;
  Uint16 vertices;
  Uint16 basetile;
  Uint16 texture;
  Uint16 cnt, tnc, entry, vertex;
  Uint32 badvertex;
  vect2 off;
  int light_flat_r, light_flat_g, light_flat_b;

  // vertex is a value from 0-15, for the meshcommandref/u/v variables
  // badvertex is a value that references the actual vertex number

  Uint16 tile = pmesh->Mem.tilelst[fan].tile;               // Tile
  Uint8  fx   = pmesh->Mem.tilelst[fan].fx;                 // Fx bits
  Uint16 type = pmesh->Mem.tilelst[fan].type;               // Command type ( index to points in fan )

  if ( INVALID_TILE == tile )
    return;

  // Animate the tiles
  if ( HAS_SOME_BITS( fx, MPDFX_ANIM ) )
  {
    if ( type >= ( MAXMESHTYPE >> 1 ) )
    {
      // Big tiles
      basetile = tile & gs->Tile_Anim.bigbaseand;// Animation set
      tile += gs->Tile_Anim.frameadd << 1;         // Animated tile
      tile = ( tile & gs->Tile_Anim.bigframeand ) + basetile;
    }
    else
    {
      // Small tiles
      basetile = tile & gs->Tile_Anim.baseand;// Animation set
      tile += gs->Tile_Anim.frameadd;         // Animated tile
      tile = ( tile & gs->Tile_Anim.frameand ) + basetile;
    }
  }

  off.u = gTileTxBox[tile].off.u;          // Texture offsets
  off.v = gTileTxBox[tile].off.v;          //

  texture = ( tile >> 6 ) + TX_TILE_0;   // 64 tiles in each 256x256 texture
  vertices = pmesh->TileDict[type].vrt_count;   // Number of vertices
  commands = pmesh->TileDict[type].cmd_count;   // Number of commands

  // Original points
  badvertex = pmesh->Mem.tilelst[fan].vrt_start;          // Get big reference value

  if ( texture != tex_loaded ) return;

  light_flat_r = 0;
  light_flat_g = 0;
  light_flat_b = 0;
  for ( cnt = 0; cnt < vertices; cnt++ )
  {
    float ftmp;
    light_flat_r += pmesh->Mem.vrt_lr_fp8[badvertex] + pmesh->Mem.vrt_ar_fp8[badvertex];
    light_flat_g += pmesh->Mem.vrt_lg_fp8[badvertex] + pmesh->Mem.vrt_ag_fp8[badvertex];
    light_flat_b += pmesh->Mem.vrt_lb_fp8[badvertex] + pmesh->Mem.vrt_ab_fp8[badvertex];

    ftmp = pmesh->Mem.vrt_z[badvertex] - level;
    v[cnt].pos.x = pmesh->Mem.vrt_x[badvertex];
    v[cnt].pos.y = pmesh->Mem.vrt_y[badvertex];
    v[cnt].pos.z = level - ftmp;

    v[cnt].col.r = FP8_TO_FLOAT( pmesh->Mem.vrt_lr_fp8[badvertex] + pmesh->Mem.vrt_ar_fp8[badvertex] );
    v[cnt].col.g = FP8_TO_FLOAT( pmesh->Mem.vrt_lg_fp8[badvertex] + pmesh->Mem.vrt_ag_fp8[badvertex] );
    v[cnt].col.b = FP8_TO_FLOAT( pmesh->Mem.vrt_lb_fp8[badvertex] + pmesh->Mem.vrt_ab_fp8[badvertex] );

    badvertex++;
  }

  light_flat_r /= vertices;
  light_flat_g /= vertices;
  light_flat_b /= vertices;


  // Change texture if need be
  if ( pmesh->Info.last_texture != texture )
  {
    GLtexture_Bind( gfx->TxTexture + texture, &gfxState );
    pmesh->Info.last_texture = texture;
  }

  ATTRIB_PUSH( "render_fan", GL_CURRENT_BIT );
  {
    // Render each command
    if ( gfxState.shading == GL_FLAT )
    {
      // use the average lighting
      glColor4f( FP8_TO_FLOAT( light_flat_r ), FP8_TO_FLOAT( light_flat_g ), FP8_TO_FLOAT( light_flat_b ), 1 );
      entry = 0;
      for ( cnt = 0; cnt < commands; cnt++ )
      {
        glBegin( GL_TRIANGLE_FAN );
        for ( tnc = 0; tnc < pmesh->TileDict[type].cmd_size[cnt]; tnc++ )
        {
          vertex = pmesh->TileDict[type].vrt[entry];

          glTexCoord2f( pmesh->TileDict[type].tx[vertex].u + off.u, pmesh->TileDict[type].tx[vertex].v + off.v );
          glVertex3fv( v[vertex].pos.v );

          entry++;
        }
        glEnd();
      }
    }
    else
    {
      // use per-vertex lighting
      entry = 0;
      for ( cnt = 0; cnt < commands; cnt++ )
      {
        glBegin( GL_TRIANGLE_FAN );
        for ( tnc = 0; tnc < pmesh->TileDict[type].cmd_size[cnt]; tnc++ )
        {
          vertex = pmesh->TileDict[type].vrt[entry];

          glTexCoord2f( pmesh->TileDict[type].tx[vertex].u + off.u, pmesh->TileDict[type].tx[vertex].v + off.v );
          glColor3fv( v[vertex].col.v );
          glVertex3fv( v[vertex].pos.v );

          entry++;
        }
        glEnd();
      }
    }
  }
  ATTRIB_POP( "render_fan" );
}


//--------------------------------------------------------------------------------------------
void render_fan( Uint32 fan, char tex_loaded )
{
  /// @details ZZ@> This function draws a mesh fan

  Game_t * gs = Graphics_requireGame(&gfxState);
  Mesh_t * pmesh = Game_getMesh(gs);
  Graphics_Data_t * gfx = gfxState.pGfx;

  GLVertex v[MAXMESHVERTICES];
  Uint16 commands;
  Uint16 vertices;
  Uint16 basetile;
  Uint16 texture;
  Uint16 cnt, tnc, entry, vertex;
  Uint32 badvertex;
  vect2 off;
  GLvector light_flat;
  vect3 nrm, pos;

  // vertex is a value from 0-15, for the meshcommandref/u/v variables
  // badvertex is a value that references the actual vertex number

  Uint16 tile = pmesh->Mem.tilelst[fan].tile;               // Tile
  Uint8  fx   = pmesh->Mem.tilelst[fan].fx;                 // Fx bits
  Uint16 type = pmesh->Mem.tilelst[fan].type;               // Command type ( index to points in fan )

  if ( INVALID_TILE == tile ) return;

  mesh_calc_normal_fan( pmesh, &(gs->phys), fan, &nrm, &pos );


  // Animate the tiles
  if ( HAS_SOME_BITS( fx, MPDFX_ANIM ) )
  {
    if ( type >= ( MAXMESHTYPE >> 1 ) )
    {
      // Big tiles
      basetile = tile & gs->Tile_Anim.bigbaseand;// Animation set
      tile += gs->Tile_Anim.frameadd << 1;         // Animated tile
      tile = ( tile & gs->Tile_Anim.bigframeand ) + basetile;
    }
    else
    {
      // Small tiles
      basetile = tile & gs->Tile_Anim.baseand;// Animation set
      tile += gs->Tile_Anim.frameadd;         // Animated tile
      tile = ( tile & gs->Tile_Anim.frameand ) + basetile;
    }
  }

  off.u = gTileTxBox[tile].off.u;          // Texture offsets
  off.v = gTileTxBox[tile].off.v;          //

  texture = ( tile >> 6 ) + TX_TILE_0;   // (1 << 6) == 64 tiles in each 256x256 texture
  vertices = pmesh->TileDict[type].vrt_count;   // Number of vertices
  commands = pmesh->TileDict[type].cmd_count;   // Number of commands

  // Original points
  badvertex = pmesh->Mem.tilelst[fan].vrt_start;   // Get big reference value

  if ( texture != tex_loaded ) return;

  light_flat.r =
  light_flat.g =
  light_flat.b =
  light_flat.a = 0.0f;
  for ( cnt = 0; cnt < vertices; cnt++ )
  {
    v[cnt].pos.x = pmesh->Mem.vrt_x[badvertex];
    v[cnt].pos.y = pmesh->Mem.vrt_y[badvertex];
    v[cnt].pos.z = pmesh->Mem.vrt_z[badvertex];

    v[cnt].col.r = FP8_TO_FLOAT( pmesh->Mem.vrt_lr_fp8[badvertex] + pmesh->Mem.vrt_ar_fp8[badvertex] );
    v[cnt].col.g = FP8_TO_FLOAT( pmesh->Mem.vrt_lg_fp8[badvertex] + pmesh->Mem.vrt_ag_fp8[badvertex] );
    v[cnt].col.b = FP8_TO_FLOAT( pmesh->Mem.vrt_lb_fp8[badvertex] + pmesh->Mem.vrt_ab_fp8[badvertex] );
    v[cnt].col.a = 1.0f;

#if defined(DEBUG_MESHFX) && defined(_DEBUG)
    if(CData.DevMode)
    {

      if ( HAS_SOME_BITS( fx, MPDFX_WALL ) )
      {
        v[cnt].col.r /= 5.0f;
        v[cnt].col.r += 0.8f;
      }

      if ( HAS_SOME_BITS( fx, MPDFX_IMPASS ) )
      {
        v[cnt].col.g /= 5.0f;
        v[cnt].col.g += 0.8f;
      }

      if ( HAS_SOME_BITS( fx, MPDFX_SLIPPY ) )
      {
        v[cnt].col.b /= 5.0f;
        v[cnt].col.b += 0.8f;
      }
    }

#endif

    light_flat.r += v[cnt].col.r;
    light_flat.g += v[cnt].col.g;
    light_flat.b += v[cnt].col.b;
    light_flat.a += v[cnt].col.a;

#if defined(DEBUG_MESH_NORMALS) && defined(_DEBUG)
    if(CData.DevMode)
    {
      vect3 * pv3 = (vect3 *) &(v[cnt].pos.v);
      mesh_calc_normal_pos( gs, fan, *pv3, &(v[cnt].nrm) );
    }
#endif

    badvertex++;
  }

  if ( vertices > 1 )
  {
    light_flat.r /= vertices;
    light_flat.g /= vertices;
    light_flat.b /= vertices;
    light_flat.a /= vertices;
  };

  // Change texture if need be
  if ( pmesh->Info.last_texture != texture )
  {
    GLtexture_Bind( gfx->TxTexture + texture, &gfxState );
    pmesh->Info.last_texture = texture;
  }

  ATTRIB_PUSH( "render_fan", GL_CURRENT_BIT );
  {
    // Render each command
    if ( gfxState.shading == GL_FLAT )
    {
      // use the average lighting
      glColor4fv( light_flat.v );
      entry = 0;
      for ( cnt = 0; cnt < commands; cnt++ )
      {
        glBegin( GL_TRIANGLE_FAN );
        for ( tnc = 0; tnc < pmesh->TileDict[type].cmd_size[cnt]; tnc++ )
        {
          vertex = pmesh->TileDict[type].vrt[entry];

          glTexCoord2f( pmesh->TileDict[type].tx[vertex].u + off.u, pmesh->TileDict[type].tx[vertex].v + off.v );
          glVertex3fv( v[vertex].pos.v );

          entry++;
        }
        glEnd();
      }
    }
    else
    {
      // use per-vertex lighting
      entry = 0;
      for ( cnt = 0; cnt < commands; cnt++ )
      {
        glBegin( GL_TRIANGLE_FAN );
        for ( tnc = 0; tnc < pmesh->TileDict[type].cmd_size[cnt]; tnc++ )
        {
          vertex = pmesh->TileDict[type].vrt[entry];

          glColor4fv( v[vertex].col.v );
          glTexCoord2f( pmesh->TileDict[type].tx[vertex].u + off.u, pmesh->TileDict[type].tx[vertex].v + off.v );
          glVertex3fv( v[vertex].pos.v );

          entry++;
        }
        glEnd();
      }
    }


#if defined(DEBUG_MESH_NORMALS) && defined(_DEBUG)
    if ( CData.DevMode )
    {
      glBegin( GL_LINES );
      {
        glLineWidth( 1.5f );
        glColor4f( 1, 1, 1, 1 );
        for ( cnt = 0; cnt < 4; cnt++ )
        {
          glVertex3fv( v[cnt].pos.v );
          glVertex3f( v[cnt].pos.x + v[cnt].nrm.x*64, v[cnt].pos.y + v[cnt].nrm.y*64, v[cnt].pos.z + v[cnt].nrm.z*64 );
        }
      }
      glEnd();

      glBegin( GL_LINES );
      glLineWidth( 3.0f );
      glColor4f( 1, 1, 1, 1 );
      glVertex3fv( pos.v );
      glVertex3f( pos.x + nrm.x*128, pos.y + nrm.y*128, pos.z + nrm.z*128 );
      glEnd();
    }
#endif

  }
  ATTRIB_POP( "render_fan" );


}

// float z;
// Uint8 red, grn, blu;
/*  if(gfx->Fog.on)
{
// The full fog value
gfx->Fog.spec = 0xff000000 | (gfx->Fog.red<<16) | (gfx->Fog.grn<<8) | (gfx->Fog.blu);
for (cnt = 0; cnt < vertices; cnt++)
{
v[cnt].pos.x = (float) pmesh->Mem.vrt_x[badvertex];
v[cnt].pos.y = (float) pmesh->Mem.vrt_y[badvertex];
v[cnt].pos.z = (float) pmesh->Mem.vrt_z[badvertex];
z = v[cnt].pos.z;


// Figure out the fog coloring
if(z < gfx->Fog.top)
{
if(z < gfx->Fog.bottom)
{
v[cnt].dcSpecular = gfx->Fog.spec;  // Full fog
}
else
{
z = 1.0 - ((z - gfx->Fog.bottom)/gfx->Fog.distance);  // 0.0 to 1.0... Amount of fog to keep
red = (gfx->Fog.red * z);
grn = (gfx->Fog.grn * z);
blu = (gfx->Fog.blu * z);
ambi = 0xff000000 | (red<<16) | (grn<<8) | (blu);
v[cnt].dcSpecular = ambi;
}
}
else
{
v[cnt].dcSpecular = 0;  // No fog
}

ambi = (DWORD) pmesh->Mem.vrt_l_fp8[badvertex];
ambi = (ambi<<8)|ambi;
ambi = (ambi<<8)|ambi;
//                v[cnt].dcColor = ambi;
//                v[cnt].dwReserved = 0;
badvertex++;
}
}
*/

//--------------------------------------------------------------------------------------------
void render_water_fan( Uint32 fan, Uint8 layer, Uint8 mode )
{
  /// @details ZZ@> This function draws a water fan

  Game_t * gs = Graphics_requireGame(&gfxState);
  Mesh_t * pmesh = Game_getMesh(gs);
  Graphics_Data_t * gfx = gfxState.pGfx;

  GLVertex v[MAXMESHVERTICES];
  Uint16 type;
  Uint16 commands;
  Uint16 vertices;
  Uint16 texture, frame;
  Uint16 cnt, tnc, entry, vertex;
  Uint32 badvertex;
  vect2  off;

  // vertex is a value from 0-15, for the meshcommandref/u/v variables
  // badvertex is a value that references the actual vertex number

  // To make life easier
  type   = 0;                              // Command type ( index to points in fan )
  off.u  = gfx->Water.layer[layer].u;         // Texture offsets
  off.v  = gfx->Water.layer[layer].v;         //
  frame  = gfx->Water.layer[layer].frame;     // Frame

  texture = layer + TX_WATER_TOP;        // Water starts at texture 5
  vertices = pmesh->TileDict[type].vrt_count;   // Number of vertices
  commands = pmesh->TileDict[type].cmd_count;   // Number of commands


  // figure the ambient light
  badvertex = pmesh->Mem.tilelst[fan].vrt_start;          // Get big reference value
  for ( cnt = 0; cnt < vertices; cnt++ )
  {
    v[cnt].pos.x = pmesh->Mem.vrt_x[badvertex];
    v[cnt].pos.y = pmesh->Mem.vrt_y[badvertex];
    v[cnt].pos.z = gfx->Water.layer[layer].zadd[frame][mode][cnt] + gfx->Water.layer[layer].z;

    if ( !gfx->Water.light )
    {
      v[cnt].col.r = FP8_TO_FLOAT( pmesh->Mem.vrt_lr_fp8[badvertex] );
      v[cnt].col.g = FP8_TO_FLOAT( pmesh->Mem.vrt_lg_fp8[badvertex] );
      v[cnt].col.b = FP8_TO_FLOAT( pmesh->Mem.vrt_lb_fp8[badvertex] );
      v[cnt].col.a = FP8_TO_FLOAT( gfx->Water.layer[layer].alpha_fp8 );
    }
    else
    {
      v[cnt].col.r = pmesh->Mem.vrt_lr_fp8[badvertex] * FP8_TO_FLOAT(gfx->Water.layer[layer].alpha_fp8);//FP8_TO_FLOAT( FP8_MUL( pmesh->Mem.vrt_lr_fp8[badvertex], gfx->Water.layer[layer].alpha_fp8 ) );
      v[cnt].col.g = pmesh->Mem.vrt_lg_fp8[badvertex] * FP8_TO_FLOAT(gfx->Water.layer[layer].alpha_fp8);//FP8_TO_FLOAT( FP8_MUL( pmesh->Mem.vrt_lg_fp8[badvertex], gfx->Water.layer[layer].alpha_fp8 ) );
      v[cnt].col.b = pmesh->Mem.vrt_lb_fp8[badvertex] * FP8_TO_FLOAT(gfx->Water.layer[layer].alpha_fp8);//FP8_TO_FLOAT( FP8_MUL( pmesh->Mem.vrt_lb_fp8[badvertex], gfx->Water.layer[layer].alpha_fp8 ) );
      v[cnt].col.a = 1.0f;
    }

    badvertex++;
  };

  // Render each command
  v[0].tx.s = 1 + off.u;
  v[0].tx.t = 0 + off.v;
  v[1].tx.s = 1 + off.u;
  v[1].tx.t = 1 + off.v;
  v[2].tx.s = 0 + off.u;
  v[2].tx.t = 1 + off.v;
  v[3].tx.s = 0 + off.u;
  v[3].tx.t = 0 + off.v;

  ATTRIB_PUSH( "render_water_fan", GL_TEXTURE_BIT | GL_CURRENT_BIT );
  {
    GLtexture_Bind( gfx->TxTexture + texture, &gfxState );

    entry = 0;
    for ( cnt = 0; cnt < commands; cnt++ )
    {
      glBegin( GL_TRIANGLE_FAN );
      for ( tnc = 0; tnc < pmesh->TileDict[type].cmd_size[cnt]; tnc++ )
      {
        vertex = pmesh->TileDict[type].vrt[entry];
        glColor4fv( v[vertex].col.v );
        glTexCoord2fv( v[vertex].tx._v );
        glVertex3fv( v[vertex].pos.v );

        entry++;
      }
      glEnd();
    }
  }
  ATTRIB_POP( "render_water_fan" );
}


//--------------------------------------------------------------------------------------------
void render_water_fan_lit( Uint32 fan, Uint8 layer, Uint8 mode )
{
  /// @details ZZ@> This function draws a water fan

  Game_t * gs = Graphics_requireGame(&gfxState);
  Mesh_t * pmesh = Game_getMesh(gs);
  Graphics_Data_t * gfx = gfxState.pGfx;

  GLVertex v[MAXMESHVERTICES];
  Uint16 type;
  Uint16 commands;
  Uint16 vertices;
  Uint16 texture, frame;
  Uint16 cnt, tnc, entry, vertex;
  Uint32 badvertex;
  vect2  off;

  // Uint8 red, grn, blu;
  // float z;
  //Uint32 ambi, spek;
  // DWORD gfx->Fog.spec;

  // vertex is a value from 0-15, for the meshcommandref/u/v variables
  // badvertex is a value that references the actual vertex number

  // To make life easier
  type  = 0;                                  // Command type ( index to points in fan )
  off.u  = gfx->Water.layer[layer].u;         // Texture offsets
  off.v  = gfx->Water.layer[layer].v;         //
  frame = gfx->Water.layer[layer].frame;      // Frame

  texture  = layer + TX_WATER_TOP;              // Water starts at texture TX_WATER_TOP == 5
  vertices = pmesh->TileDict[type].vrt_count;   // Number of vertices
  commands = pmesh->TileDict[type].cmd_count;   // Number of commands


  badvertex = pmesh->Mem.tilelst[fan].vrt_start;          // Get big reference value
  for ( cnt = 0; cnt < vertices; cnt++ )
  {
    v[cnt].pos.x = pmesh->Mem.vrt_x[badvertex];
    v[cnt].pos.y = pmesh->Mem.vrt_y[badvertex];
    v[cnt].pos.z = gfx->Water.layer[layer].zadd[frame][mode][cnt] + gfx->Water.layer[layer].z;

    v[cnt].col.r = v[cnt].col.g = v[cnt].col.b = 1.0f;
    v[cnt].col.a = FP8_TO_FLOAT( gfx->Water.layer[layer].alpha_fp8 );

    badvertex++;
  };

  // Render each command
  v[0].tx.s = 1 + off.u;
  v[0].tx.t = 0 + off.v;
  v[1].tx.s = 1 + off.u;
  v[1].tx.t = 1 + off.v;
  v[2].tx.s = 0 + off.u;
  v[2].tx.t = 1 + off.v;
  v[3].tx.s = 0 + off.u;
  v[3].tx.t = 0 + off.v;

  ATTRIB_PUSH( "render_water_fan_lit", GL_TEXTURE_BIT | GL_CURRENT_BIT );
  {
    // Change texture if need be
    if ( pmesh->Info.last_texture != texture )
    {
      GLtexture_Bind( gfx->TxTexture + texture, &gfxState );
      pmesh->Info.last_texture = texture;
    }

    entry = 0;
    for ( cnt = 0; cnt < commands; cnt++ )
    {
      glBegin( GL_TRIANGLE_FAN );
      for ( tnc = 0; tnc < pmesh->TileDict[type].cmd_size[cnt]; tnc++ )
      {
        vertex = pmesh->TileDict[type].vrt[entry];
        glColor4fv( v[vertex].col.v );
        glTexCoord2fv( v[vertex].tx._v );
        glVertex3fv( v[vertex].pos.v );

        entry++;
      }
      glEnd();
    }
  }
  ATTRIB_POP( "render_water_fan_lit" );
}

//--------------------------------------------------------------------------------------------
bool_t make_renderlist(RENDERLIST * prlst)
{
  Uint32 fan;
  Uint32 tile_count;
  bool_t inview;
  static Uint32 next_wldframe = 0;

  Game_t * gs = Graphics_requireGame(&gfxState);

  Mesh_t     * pmesh = Game_getMesh(gs);
  MeshInfo_t * mi    = &(pmesh->Info);
  MeshMem_t  * mm    = &(pmesh->Mem);

  if(NULL == prlst) return bfalse;

  // make a delay
  if(gs->wld_frame < next_wldframe) return btrue;
  next_wldframe = gs->wld_frame + 7;

  prlst->num_totl = 0;
  prlst->num_shine = 0;
  prlst->num_reflc = 0;
  prlst->num_norm = 0;
  prlst->num_watr = 0;

  tile_count = mi->tiles_x * mi->tiles_y;
  for ( fan = 0; fan < tile_count; fan++ )
  {
    inview = Frustum_BBoxInFrustum( &gFrustum, mm->tilelst[fan].bbox.mins.v, mm->tilelst[fan].bbox.maxs.v );

    mm->tilelst[fan].inrenderlist = bfalse;

    if ( inview && prlst->num_totl < MAXMESHRENDER )
    {
      bool_t is_shine, is_noref, is_norml, is_water;

      mm->tilelst[fan].inrenderlist = btrue;

      is_shine = mesh_has_all_bits( mm->tilelst, fan, MPDFX_SHINY );
      is_noref = mesh_has_all_bits( mm->tilelst, fan, MPDFX_NOREFLECT );
      is_norml = !is_shine;
      is_water = mesh_has_some_bits( mm->tilelst, fan, MPDFX_WATER );

      // Put each tile in basic list
      prlst->totl[prlst->num_totl] = fan;
      prlst->num_totl++;
      mesh_fan_add_renderlist( mm->tilelst, fan );

      // Put each tile
      if ( !is_noref )
      {
        prlst->reflc[prlst->num_reflc] = fan;
        prlst->num_reflc++;
      }

      if ( is_shine )
      {
        prlst->shine[prlst->num_shine] = fan;
        prlst->num_shine++;
      }

      if ( is_norml )
      {
        prlst->norm[prlst->num_norm] = fan;
        prlst->num_norm++;
      }

      if ( is_water )
      {
        // precalculate the "mode" variable so that we don't waste time rendering the waves
        int tx, ty;

        ty = fan / mi->tiles_x;
        tx = fan % mi->tiles_x;

        prlst->watr_mode[prlst->num_watr] = ((ty & 1) << 1) + (tx & 1);
        prlst->watr[prlst->num_watr]      = fan;

        prlst->num_watr++;
      }
    };
  };

  return btrue;
}

//--------------------------------------------------------------------------------------------
void set_fan_dyna_light( int fanx, int fany, PRT_REF particle )
{
  /// @details ZZ@> This function is a little helper, lighting the selected fan
  ///     with the chosen particle

  vect3 dif, nrm;
  Uint32 fan;
  int   vertex, lastvertex;
  float flight, dist2;
  float light_r, light_g, light_b;
  float light_r0, light_g0, light_b0;

  Game_t * gs = Graphics_requireGame(&gfxState);
  PPrt_t   prtlst = gs->PrtList;

  Mesh_t     * pmesh = Game_getMesh(gs);
  MeshInfo_t * mi    = &(pmesh->Info);
  MeshMem_t  * mm    = &(pmesh->Mem);

  if ( fanx >= 0 && fanx < mi->tiles_x && fany >= 0 && fany < mi->tiles_y )
  {
    fan = mesh_convert_fan( mi, fanx, fany );
    vertex = mm->tilelst[fan].vrt_start;
    lastvertex = vertex + pmesh->TileDict[mm->tilelst[fan].type].vrt_count;
    while ( vertex < lastvertex )
    {
      light_r0 = light_r = mm->vrt_lr_fp8[vertex];
      light_g0 = light_g = mm->vrt_lg_fp8[vertex];
      light_b0 = light_b = mm->vrt_lb_fp8[vertex];

      dif.x = prtlst[particle].ori.pos.x - mm->vrt_x[vertex];
      dif.y = prtlst[particle].ori.pos.y - mm->vrt_y[vertex];
      dif.z = prtlst[particle].ori.pos.z - mm->vrt_z[vertex];

      dist2 = DotProduct( dif, dif );

      flight = prtlst[particle].dyna.level;
      flight *= 127 * prtlst[particle].dyna.falloff / ( 127 * gs->PrtList[particle].dyna.falloff + dist2 );

      if ( dist2 > 0.0f )
      {
        float ftmp, dist = sqrt( dist2 );
        vect3 pos = VECT3(mm->vrt_x[vertex], mm->vrt_y[vertex], mm->vrt_z[vertex]);

        dif.x /= dist;
        dif.y /= dist;
        dif.z /= dist;

        mesh_calc_normal( pmesh, &(gs->phys), pos, &nrm );

        ftmp = DotProduct( dif, nrm );
        if ( ftmp > 0 )
        {
          light_r += 255 * flight * ftmp * ftmp;
          light_g += 255 * flight * ftmp * ftmp;
          light_b += 255 * flight * ftmp * ftmp;
        };
      }
      else
      {
        light_r += 255 * flight;
        light_g += 255 * flight;
        light_b += 255 * flight;
      }

      mm->vrt_lr_fp8[vertex] = 0.9 * light_r0 + 0.1 * light_r;
      mm->vrt_lg_fp8[vertex] = 0.9 * light_g0 + 0.1 * light_g;
      mm->vrt_lb_fp8[vertex] = 0.9 * light_b0 + 0.1 * light_b;

      //if ( mi->exploremode )
      //{
      //  if ( mm->vrt_lr_fp8[vertex] > light_r0 ) mm->vrt_ar_fp8[vertex] = 0.9 * mm->vrt_ar_fp8[vertex] + 0.1 * mm->vrt_lr_fp8[vertex];
      //  if ( mm->vrt_lg_fp8[vertex] > light_g0 ) mm->vrt_ag_fp8[vertex] = 0.9 * mm->vrt_ag_fp8[vertex] + 0.1 * mm->vrt_lg_fp8[vertex];
      //  if ( mm->vrt_lb_fp8[vertex] > light_b0 ) mm->vrt_ab_fp8[vertex] = 0.9 * mm->vrt_ab_fp8[vertex] + 0.1 * mm->vrt_lb_fp8[vertex];
      //};

      vertex++;
    }
  }
}

//--------------------------------------------------------------------------------------------
void do_dyna_light(Game_t * gs)
{
  /// @details ZZ@> This function does dynamic lighting of visible fans

  PRT_REF prt_cnt;
  int cnt, lastvertex, vertex, fan, entry, fanx, fany, addx, addy;
  float light_r, light_g, light_b;
  float dist2;
  vect3 dif, nrm;

  Mesh_t * pmesh   = NULL;
  MeshMem_t *  mm  = NULL;

  PPrt_t  prtlst       = NULL;
  size_t  prtlst_size  = PRTLST_COUNT;
  PDLight_t dynalst      = NULL;

  Graphics_Data_t * gfx = gfxState.pGfx;

  float dist_factor;

  if(NULL == gs) gs = Graphics_requireGame( &gfxState );

  pmesh   = Game_getMesh(gs);
  mm      = &(pmesh->Mem);
  prtlst  = gs->PrtList;
  dynalst = gfx->DLightList;

  // Don't need to do every frame
  if ( 0 != ( gfxState.fps_loops & 7 ) ) return;

  // factor to make sure that the light falls to 1/8 when the distance
  // from the dynalight == falloff_distance
  dist_factor = 1.0f  / sqrt( (float)8 - 1 );

  // Do each floor tile
  if ( gfx->exploremode )
  {
    // Set base light level in explore mode...

    for ( prt_cnt = 0; prt_cnt < prtlst_size; prt_cnt++ )
    {
      if ( !ACTIVE_PRT( prtlst, prt_cnt ) || !prtlst[prt_cnt].dyna.on ) continue;

      fanx = MESH_FLOAT_TO_FAN( prtlst[prt_cnt].ori.pos.x );
      fany = MESH_FLOAT_TO_FAN( prtlst[prt_cnt].ori.pos.y );

      for ( addy = -DYNAFANS; addy <= DYNAFANS; addy++ )
      {
        for ( addx = -DYNAFANS; addx <= DYNAFANS; addx++ )
        {
          set_fan_dyna_light( fanx + addx, fany + addy, prt_cnt );
        }
      }
    }
  }
  else
  {
    float ftmp;
    if ( gfxState.shading != GL_FLAT )
    {
      // Add to base light level in normal mode
      entry = 0;
      while ( entry < renderlist.num_totl )
      {
        fan = renderlist.totl[entry];

        vertex = mm->tilelst[fan].vrt_start;
        lastvertex = vertex + pmesh->TileDict[mm->tilelst[fan].type].vrt_count;
        while ( vertex < lastvertex )
        {
          vect3 pos = VECT3(mm->vrt_x[vertex], mm->vrt_y[vertex], mm->vrt_z[vertex]);

          mesh_calc_normal( pmesh, &(gs->phys), pos, &nrm );

          // do global lighting
          if(gfx->Light.on)
          {
            light_r = gfx->Light.ambicol.r * 255;
            light_g = gfx->Light.ambicol.g * 255;
            light_b = gfx->Light.ambicol.b * 255;

            ftmp = DotProduct( nrm, gfx->Light.spekdir );
            if ( ftmp > 0 )
            {
              float m, k1, k2;
              m = 0.5f;
              k1 = 1.0f - m;
              k2 = m/(1- m);
              ftmp = k1 * (ftmp + k2);

              light_r += gfx->Light.spekcol.r * 255 * ftmp * ftmp;
              light_g += gfx->Light.spekcol.g * 255 * ftmp * ftmp;
              light_b += gfx->Light.spekcol.b * 255 * ftmp * ftmp;
            };

            if ( light_r > 255 ) light_r = 255;
            if ( light_r <   0 ) light_r = 0;
            mm->vrt_ar_fp8[vertex] = 0.9 * mm->vrt_ar_fp8[vertex] + 0.1 * light_r;

            if ( light_g > 255 ) light_g = 255;
            if ( light_g <   0 ) light_g = 0;
            mm->vrt_ag_fp8[vertex] = 0.9 * mm->vrt_ag_fp8[vertex]  + 0.1 * light_g;

            if ( light_b > 255 ) light_b = 255;
            if ( light_b <   0 ) light_b = 0;
            mm->vrt_ab_fp8[vertex] = 0.9 * mm->vrt_ab_fp8[vertex]  + 0.1 * light_b;
          }

          // Do light particles
          light_r = 0;
          light_g = 0;
          light_b = 0;
          for ( cnt = 0; cnt < gfx->DLightList_count; cnt++ )
          {
            float flight;

            dif.x = dynalst[cnt].pos.x - mm->vrt_x[vertex];
            dif.y = dynalst[cnt].pos.y - mm->vrt_y[vertex];
            dif.z = dynalst[cnt].pos.z - mm->vrt_z[vertex];

            ftmp = DotProduct( dif, nrm );
            if ( ftmp > 0 )
            {
              float dist_temp = dist_factor * dynalst[cnt].falloff;

              dist2 = DotProduct( dif, dif );

              flight = dynalst[cnt].level;
              flight *= dist_temp*dist_temp / ( dist_temp*dist_temp + dist2 );

              if(flight * 255 * gfx->DLightList_count > 1)
              {
                if ( dist2 > 0.0f )
                {
                  // little kludge to soften the normal-dependent lighting
                  // the dot ptoduct factor will vary between 1 for full lighting
                  // down to m*m for "shadowed" lighting
                  float m, k1, k2;
                  m = 0.5f;
                  k1 = 1.0f - m;
                  k2 = m/(1- m);

                  ftmp /= sqrt( dist2 );
                  ftmp = k1 * (ftmp + k2);

                  light_r += 255 * flight * ftmp * ftmp;
                  light_g += 255 * flight * ftmp * ftmp;
                  light_b += 255 * flight * ftmp * ftmp;
                }
                else
                {
                  light_r += 255 * flight;
                  light_g += 255 * flight;
                  light_b += 255 * flight;
                }
              }
            }
          }

          if ( light_r > 255 ) light_r = 255;
          if ( light_r <   0 ) light_r = 0;
          mm->vrt_lr_fp8[vertex] = 0.9 * mm->vrt_lr_fp8[vertex] + 0.1 * light_r;

          if ( light_g > 255 ) light_g = 255;
          if ( light_g <   0 ) light_g = 0;
          mm->vrt_lg_fp8[vertex] = 0.9 * mm->vrt_lg_fp8[vertex]  + 0.1 * light_g;

          if ( light_b > 255 ) light_b = 255;
          if ( light_b <   0 ) light_b = 0;
          mm->vrt_lb_fp8[vertex] = 0.9 * mm->vrt_lb_fp8[vertex]  + 0.1 * light_b;

          //if ( gfx->exploremode )
          //{
          //  if ( light_r > mm->vrt_ar_fp8[vertex] ) mm->vrt_ar_fp8[vertex] = 0.9 * mm->vrt_ar_fp8[vertex] + 0.1 * light_r;
          //  if ( light_g > mm->vrt_ag_fp8[vertex] ) mm->vrt_ag_fp8[vertex] = 0.9 * mm->vrt_ag_fp8[vertex] + 0.1 * light_g;
          //  if ( light_b > mm->vrt_ab_fp8[vertex] ) mm->vrt_ab_fp8[vertex] = 0.9 * mm->vrt_ab_fp8[vertex] + 0.1 * light_b;
          //}

          vertex++;
        }
        entry++;
      }
    }
    else
    {
      entry = 0;
      while ( entry < renderlist.num_totl )
      {
        fan = renderlist.totl[entry];
        vertex = mm->tilelst[fan].vrt_start;
        lastvertex = vertex + pmesh->TileDict[mm->tilelst[fan].type].vrt_count;
        while ( vertex < lastvertex )
        {
          // Do light particles
          mm->vrt_lr_fp8[vertex] = mm->vrt_ar_fp8[vertex];
          mm->vrt_lg_fp8[vertex] = mm->vrt_ag_fp8[vertex];
          mm->vrt_lb_fp8[vertex] = mm->vrt_ab_fp8[vertex];

          vertex++;
        }
        entry++;
      }
    }
  }
};


//--------------------------------------------------------------------------------------------
//void make_renderlist()
//{
//  /// @details ZZ@> This function figures out which mesh fans to draw
//
//  int cnt, fan, fanx, fany;
//  int row, run, numrow;
//  int xlist[4], ylist[4];
//  int leftnum, leftlist[4];
//  int rightnum, rightlist[4];
//  int fanrowstart[128], fanrowrun[128];
//  int x, stepx, divx, basex;
//  int from, to;
//
//
//  // Clear old render lists
//  for (cnt = 0; cnt < renderlist.num_totl; cnt++)
//  {
//    fan = renderlist.totl[cnt];
//    mesh_fan_remove_renderlist(fan);
//  }
//  renderlist.num_totl = 0;
//  renderlist.num_shine = 0;
//  renderlist.num_reflc = 0;
//  renderlist.num_norm = 0;
//  renderlist.num_watr = 0;
//
//  // It works better this way...
//  cornery[cornerlistlowtohighy[3]] += 256;
//
//  // Make life simpler
//  xlist[0] = cornerx[cornerlistlowtohighy[0]];
//  xlist[1] = cornerx[cornerlistlowtohighy[1]];
//  xlist[2] = cornerx[cornerlistlowtohighy[2]];
//  xlist[3] = cornerx[cornerlistlowtohighy[3]];
//  ylist[0] = cornery[cornerlistlowtohighy[0]];
//  ylist[1] = cornery[cornerlistlowtohighy[1]];
//  ylist[2] = cornery[cornerlistlowtohighy[2]];
//  ylist[3] = cornery[cornerlistlowtohighy[3]];
//
//  // Find the center line
//  divx = ylist[3] - ylist[0]; if (divx < 1) return;
//  stepx = xlist[3] - xlist[0];
//  basex = xlist[0];
//
//
//  // Find the points in each edge
//  leftlist[0] = 0;  leftnum = 1;
//  rightlist[0] = 0;  rightnum = 1;
//  if (xlist[1] < (stepx*(ylist[1] - ylist[0]) / divx) + basex)
//  {
//    leftlist[leftnum] = 1;  leftnum++;
//    cornerx[1] -= 512;
//  }
//  else
//  {
//    rightlist[rightnum] = 1;  rightnum++;
//    cornerx[1] += 512;
//  }
//  if (xlist[2] < (stepx*(ylist[2] - ylist[0]) / divx) + basex)
//  {
//    leftlist[leftnum] = 2;  leftnum++;
//    cornerx[2] -= 512;
//  }
//  else
//  {
//    rightlist[rightnum] = 2;  rightnum++;
//    cornerx[2] += 512;
//  }
//  leftlist[leftnum] = 3;  leftnum++;
//  rightlist[rightnum] = 3;  rightnum++;
//
//
//  // Make the left edge ( rowstart )
//  fany = MESH_INT_TO_FAN(ylist[0]);
//  row = 0;
//  cnt = 1;
//  while (cnt < leftnum)
//  {
//    from = leftlist[cnt-1];  to = leftlist[cnt];
//    x = xlist[from];
//    divx = ylist[to] - ylist[from];
//    stepx = 0;
//    if (divx > 0)
//    {
//      stepx = MESH_FAN_TO_INT(xlist[to] - xlist[from]) / divx;
//    }
//    x -= 256;
//
//    run = MESH_INT_TO_FAN(ylist[to]);
//    while (fany < run)
//    {
//      if (fany >= 0 && fany < pmesh->Info.tiles_y)
//      {
//        fanx = MESH_INT_TO_FAN(x);
//        if (fanx < 0)  fanx = 0;
//        if (fanx >= pmesh->Info.tiles_x)  fanx = pmesh->Info.tiles_x - 1;
//        fanrowstart[row] = fanx;
//        row++;
//      }
//      x += stepx;
//      fany++;
//    }
//    cnt++;
//  }
//  numrow = row;
//
//
//  // Make the right edge ( rowrun )
//  fany = MESH_INT_TO_FAN(ylist[0]);
//  row = 0;
//  cnt = 1;
//  while (cnt < rightnum)
//  {
//    from = rightlist[cnt-1];  to = rightlist[cnt];
//    x = xlist[from];
//    //x+=128;
//    divx = ylist[to] - ylist[from];
//    stepx = 0;
//    if (divx > 0)
//    {
//      stepx = MESH_FAN_TO_INT(xlist[to] - xlist[from]) / divx;
//    }
//
//    run = MESH_INT_TO_FAN(ylist[to]);
//    while (fany < run)
//    {
//      if (fany >= 0 && fany < pmesh->Info.tiles_y)
//      {
//        fanx = MESH_INT_TO_FAN(x);
//        if (fanx < 0)  fanx = 0;
//        if (fanx >= pmesh->Info.tiles_x - 1)  fanx = pmesh->Info.tiles_x - 1;//-2
//        fanrowrun[row] = ABS(fanx - fanrowstart[row]) + 1;
//        row++;
//      }
//      x += stepx;
//      fany++;
//    }
//    cnt++;
//  }
//
//  if (numrow != row)
//  {
//    log_error("ROW error (%i, %i)\n", numrow, row);
//  }
//
//  // Fill 'em up again
//  fany = MESH_INT_TO_FAN(ylist[0]);
//  if (fany < 0) fany = 0;
//  if (fany >= pmesh->Info.tiles_y) fany = pmesh->Info.tiles_y - 1;
//  row = 0;
//  while (row < numrow)
//  {
//    cnt = mesh_convert_fan(fanrowstart[row], fany);
//    run = fanrowrun[row];
//    fanx = 0;
//    while (fanx < run)
//    {
//      if (renderlist.num_totl<MAXMESHRENDER)
//      {
//        bool_t is_shine, is_noref, is_norml, is_water;
//
//        is_shine = mesh_has_some_bits(cnt, MPDFX_SHINY);
//        is_noref = mesh_has_no_bits(cnt, MPDFX_NOREFLECT);
//        is_norml = !is_shine;
//        is_water = mesh_has_some_bits(cnt, MPDFX_WATER);
//
//        // Put each tile in basic list
//        renderlist.totl[renderlist.num_totl] = cnt;
//        renderlist.num_totl++;
//        mesh_fan_add_renderlist(cnt);
//
//        // Put each tile in one other list, for shadows and relections
//        if (!is_noref)
//        {
//          renderlist.reflc[renderlist.num_reflc] = cnt;
//          renderlist.num_reflc++;
//        }
//
//        if (is_shine)
//        {
//          renderlist.shine[renderlist.num_shine] = cnt;
//          renderlist.num_shine++;
//        }
//
//        if (is_norml)
//        {
//          renderlist.norm[renderlist.num_norm] = cnt;
//          renderlist.num_norm++;
//        }
//
//        if (is_water)
//        {
//          renderlist.watr[renderlist.num_watr] = cnt;
//          renderlist.num_watr++;
//        }
//      };
//
//      cnt++;
//      fanx++;
//    }
//    row++;
//    fany++;
//  }
//}
