//********************************************************************************************
//* Egoboo - graphic_prc.c
//*
//* Particle system drawing code.
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

#include "ogl_include.h"
#include "Log.h"
#include "camera.h"

#include "egoboo.h"

#include <assert.h>

#include "graphic.inl"
#include "particle.inl"
#include "char.inl"
#include "egoboo_math.inl"
#include "egoboo_math.inl"
#include "egoboo_types.inl"
#include "mesh.inl"

enum e_prt_ori
{
  ori_v,  // vertical
  ori_h,  // horizontal
  ori_p,  // ?? can't remember (arrow) ??
  ori_b   // billboard
};

typedef enum e_prt_ori PRT_ORI;

PRT_ORI particle_orientation[256] =
{
  ori_b, ori_b, ori_v, ori_v, ori_b, ori_b, ori_b, ori_b, ori_v, ori_b, ori_v, ori_b, ori_b, ori_v, ori_b, ori_b,
  ori_v, ori_v, ori_v, ori_v, ori_v, ori_v, ori_v, ori_v, ori_v, ori_v, ori_v, ori_v, ori_v, ori_v, ori_v, ori_v,
  ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b,
  ori_v, ori_v, ori_v, ori_v, ori_v, ori_v, ori_v, ori_v, ori_v, ori_v, ori_v, ori_v, ori_b, ori_b, ori_b, ori_b,
  ori_h, ori_h, ori_h, ori_h, ori_h, ori_h, ori_h, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b,
  ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_p, ori_p, ori_p, ori_p, ori_p, ori_p, ori_p, ori_p,
  ori_b, ori_b, ori_b, ori_v, ori_b, ori_b, ori_b, ori_b, ori_p, ori_p, ori_p, ori_p, ori_p, ori_p, ori_b, ori_b,
  ori_b, ori_b, ori_b, ori_p, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b,
  ori_b, ori_b, ori_b, ori_b, ori_v, ori_v, ori_v, ori_v, ori_v, ori_v, ori_v, ori_v, ori_h, ori_h, ori_v, ori_v,
  ori_v, ori_v, ori_v, ori_b, ori_v, ori_p, ori_p, ori_p, ori_b, ori_b, ori_p, ori_b, ori_b, ori_b, ori_b, ori_b,
  ori_b, ori_b, ori_b, ori_b, ori_b, ori_v, ori_b, ori_v, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_v, ori_v,
  ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b,
  ori_b, ori_b, ori_v, ori_b, ori_p, ori_p, ori_p, ori_b, ori_b, ori_p, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b,
  ori_p, ori_v, ori_v, ori_b, ori_b, ori_b, ori_p, ori_p, ori_p, ori_p, ori_p, ori_b, ori_b, ori_b, ori_b, ori_b,
  ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_b, ori_v, ori_v, ori_v, ori_v, ori_p, ori_p, ori_p, ori_p
};

//--------------------------------------------------------------------------------------------
bool_t calc_billboard( Game_t * gs, GLVertex vrtlst[], GLVertex * vert, float size, Uint16 image);

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
extern void Begin3DMode();

GLint prt_attrib_open;
GLint prt_attrib_close;


void get_vectors( PRT_REF prt, vect3 * vert, vect3 * horiz, float * dist )
{
  CHR_REF chr;
  PIP_REF pip;
  float cossize, sinsize;
  Uint16 rotate;
  vect3 vector_right, vector_up;
  vect3 vec1, vec2;
  vect3 vect_out;

  PRT_ORI ori;
  Uint32  image;

  Game_t * gs = Graphics_requireGame(&gfxState);

  PChr_t  chrlst = gs->ChrList;

  Prt_t * pprt = PrtList_getPPrt(gs, prt);
  //Pip_t * ppip = PrtList_getPPip(gs, prt);

  if ( NULL == pprt ) return;

  assert( NULL != vert  );
  assert( NULL != horiz );
  assert( NULL != dist  );

  chr = prt_get_attachedtochr( gs, prt );
  pip = pprt->pip;

  rotate = 0;
  image = FP8_TO_INT( pprt->image_fp8 + pprt->imagestt_fp8 );
  ori = particle_orientation[image];

  // if the velocity is zero, convert the projectile to a billboard
  if (( ori == ori_p ) && ABS( pprt->ori.vel.x ) + ABS( pprt->ori.vel.y ) + ABS( pprt->ori.vel.z ) < 0.1 )
  {
    ori = ori_b;
    rotate += 16384;
  }

  vec1.x = 0;
  vec1.y = 0;
  vec1.z = 1;

  vec2.x = GCamera.pos.x - pprt->ori.pos.x;
  vec2.y = GCamera.pos.y - pprt->ori.pos.y;
  vec2.z = GCamera.pos.z - pprt->ori.pos.z;

  vect_out.x = GCamera.mView.v[ 2];
  vect_out.y = GCamera.mView.v[ 6];
  vect_out.z = GCamera.mView.v[10];

  *dist = DotProduct(vect_out, vec2);

  switch ( ori )
  {
      // the particle is standing like a tree
    case ori_v:
      {
        vector_right = Normalize( CrossProduct( vec1, vec2 ) );
        vector_up    = vec1;

        rotate += pprt->rotate + 8192;
      };
      break;

      // the particle is lying like a coin
    case ori_h:
      {
        vector_right = Normalize( CrossProduct( vec1, vec2 ) );
        vector_up    = Normalize( CrossProduct( vec1, vector_right ) );

        rotate += pprt->rotate - 24576;
      };
      break;

      // the particle is flying through the air (projectile motion)
    case ori_p:
      {
        vect3 vec_vel, vec1, vec2, vec3;

        CHR_REF prt_target = prt_get_target( gs, prt );

        if ( ABS( pprt->ori.vel.x ) + ABS( pprt->ori.vel.y ) + ABS( pprt->ori.vel.z ) > 0.0f )
        {
          vec_vel.x = pprt->ori.vel.x;
          vec_vel.y = pprt->ori.vel.y;
          vec_vel.z = pprt->ori.vel.z;
        }
        else if ( ACTIVE_CHR( chrlst,  prt_target ) && ACTIVE_CHR( chrlst,  prt_target ) )
        {
          vec_vel.x = chrlst[prt_target].ori.pos.x - pprt->ori.pos.x;
          vec_vel.y = chrlst[prt_target].ori.pos.y - pprt->ori.pos.y;
          vec_vel.z = chrlst[prt_target].ori.pos.z - pprt->ori.pos.z;
        }
        else
        {
          vec_vel.x = 0;
          vec_vel.y = 0;
          vec_vel.z = 1;
        };

        vec2.x = GCamera.pos.x - pprt->ori.pos.x;
        vec2.y = GCamera.pos.y - pprt->ori.pos.y;
        vec2.z = GCamera.pos.z - pprt->ori.pos.z;

        vec1 = Normalize( vec_vel );
        vec3 = CrossProduct( vec1, Normalize( vec2 ) );

        vector_right = vec3;
        vector_up    = vec1;

        rotate += pprt->rotate - 8192;
      }
      break;

      // normal billboarded particle
    default:
    case ori_b:
      {
        // this is the simple billboard
        //vector_right.x = GCamera.mView.v[0];
        //vector_right.y = GCamera.mView.v[4];
        //vector_right.z = GCamera.mView.v[8];

        //vector_up.x = GCamera.mView.v[1];
        //vector_up.y = GCamera.mView.v[5];
        //vector_up.z = GCamera.mView.v[9];

        vector_right = Normalize( CrossProduct( vec1, vec2 ) );
        vector_up    = Normalize( CrossProduct( vector_right, vec2 ) );

        rotate += pprt->rotate - 24576;
      };

      break;
  };

  if ( ACTIVE_CHR( chrlst,  chr ) && gs->PipList[pip].rotatewithattached )
  {
    rotate += chrlst[chr].ori.turn_lr;
  }

  rotate >>= 2;
  sinsize = turntosin[rotate & TRIGTABLE_MASK];
  cossize = turntocos[rotate & TRIGTABLE_MASK];

  ( *horiz ).x = cossize * vector_right.x - sinsize * vector_up.x;
  ( *horiz ).y = cossize * vector_right.y - sinsize * vector_up.y;
  ( *horiz ).z = cossize * vector_right.z - sinsize * vector_up.z;

  ( *vert ).x  = sinsize * vector_right.x + cossize * vector_up.x;
  ( *vert ).y  = sinsize * vector_right.y + cossize * vector_up.y;
  ( *vert ).z  = sinsize * vector_right.z + cossize * vector_up.z;
};


//--------------------------------------------------------------------------------------------
void render_antialias_prt( Uint32 vrtcount, GLVertex * vrtlist )
{
  Game_t * gs = Graphics_requireGame(&gfxState);
  PPrt_t   prtlst = gs->PrtList;

  GLVertex vtlist[4];
  Uint16 cnt;
  PRT_REF prt;
  CHR_REF chr;
  PIP_REF pip;
  Uint16 image;
  float size;
  int i;

  ATTRIB_PUSH( "render_antialias_prt", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT );
  {
    glDepthMask( GL_FALSE );

    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LESS );

    glEnable( GL_ALPHA_TEST );
    glAlphaFunc( GL_GREATER, 0 );

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    // Render each particle that was on

    for ( cnt = 0; cnt < vrtcount; cnt++ )
    {
      // Get the index from the color slot
      prt = ( Uint16 ) vrtlist[cnt].color;
      chr = prt_get_attachedtochr( gs, prt );
      pip = prtlst[prt].pip;

      // Render solid ones twice...  For Antialias
      if ( prtlst[prt].type != PRTTYPE_SOLID ) continue;

      {
        GLvector color_component = {FP8_TO_FLOAT( prtlst[prt].lightr_fp8 ), FP8_TO_FLOAT( prtlst[prt].lightg_fp8 ), FP8_TO_FLOAT( prtlst[prt].lightb_fp8 ), FP8_TO_FLOAT( antialiastrans_fp8 ) };

        // Figure out the sprite's size based on distance
        size = FP8_TO_FLOAT( prtlst[prt].size_fp8 ) * 0.25f * 1.1f;  // [claforte] Fudge the value.

        // Fill in the rest of the data
        image = FP8_TO_FLOAT( prtlst[prt].image_fp8 + prtlst[prt].imagestt_fp8 );

        // Create the billboard vertices for this particle.
        calc_billboard(gs, vtlist, vrtlist + cnt, size, image);

        // Go on and draw it
        glBegin( GL_TRIANGLE_FAN );
        glColor4fv( color_component.v );  //[claforte] should use alpha_component instead of 0.5?
        for ( i = 0; i < 4; i++ )
        {
          glTexCoord2fv( vtlist[i].tx._v );
          glVertex3fv( vtlist[i].pos.v );
        }
        glEnd();

      }
    }
  }
  ATTRIB_POP( "render_antialias_prt" );
};

//--------------------------------------------------------------------------------------------
void render_solid_prt( Uint32 vrtcount, GLVertex * vrtlist )
{
  Game_t * gs = Graphics_requireGame(&gfxState);
  PPrt_t   prtlst = gs->PrtList;

  GLVertex vtlist[4];
  Uint16 cnt;
  PRT_REF prt;
  CHR_REF chr;
  PIP_REF pip;
  Uint16 image;
  float size;
  int i;

  ATTRIB_PUSH( "render_solid_prt", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT );
  {
    // Render each particle that was on
    glDepthMask( GL_TRUE );

    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LEQUAL );

    glDisable( GL_BLEND );
    glEnable( GL_ALPHA_TEST );
    glAlphaFunc( GL_GREATER, 0 );


    for ( cnt = 0; cnt < vrtcount; cnt++ )
    {
      // Get the index from the color slot
      prt = ( Uint16 ) vrtlist[cnt].color;
      chr = prt_get_attachedtochr( gs, prt );
      pip = prtlst[prt].pip;

      // Draw sprites this round
      if ( prtlst[prt].type != PRTTYPE_SOLID ) continue;

      {
        GLvector color_component = { FP8_TO_FLOAT( prtlst[prt].lightr_fp8 ), FP8_TO_FLOAT( prtlst[prt].lightg_fp8 ), FP8_TO_FLOAT( prtlst[prt].lightb_fp8 ), 1};

        // [claforte] Fudge the value.
        size = FP8_TO_FLOAT( prtlst[prt].size_fp8 ) * 0.25f;

        // Fill in the rest of the data
        image = FP8_TO_FLOAT( prtlst[prt].image_fp8 + prtlst[prt].imagestt_fp8 );

        // Create the billboard vertices for this particle.
        calc_billboard(gs, vtlist, vrtlist + cnt, size, image);

        glBegin( GL_TRIANGLE_FAN );
        glColor4fv( color_component.v );
        for ( i = 0; i < 4; i++ )
        {
          glTexCoord2fv( vtlist[i].tx._v );
          glVertex3fv( vtlist[i].pos.v );
        }
        glEnd();
      }
    }
  }
  glPopAttrib();
};
//--------------------------------------------------------------------------------------------
void render_transparent_prt( Uint32 vrtcount, GLVertex * vrtlist )
{
  Game_t * gs = Graphics_requireGame(&gfxState);
  PPrt_t   prtlst = gs->PrtList;

  GLVertex vtlist[4];
  Uint16 cnt;
  PRT_REF prt;
  CHR_REF chr;
  PIP_REF pip;
  Uint16 image;
  float size;
  int i;

  ATTRIB_PUSH( "render_transparent_prt", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT );
  {
    glDepthMask( GL_FALSE );

    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LEQUAL );

    glEnable( GL_ALPHA_TEST );
    glAlphaFunc( GL_GREATER, 0 );

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );


    for ( cnt = 0; cnt < vrtcount; cnt++ )
    {
      // Get the index from the color slot
      prt = ( Uint16 ) vrtlist[cnt].color;
      chr = prt_get_attachedtochr( gs, prt );
      pip = prtlst[prt].pip;

      // Draw transparent sprites this round
      if ( prtlst[prt].type != PRTTYPE_ALPHA ) continue;

      {
        GLvector color_component = {FP8_TO_FLOAT( prtlst[prt].lightr_fp8 ), FP8_TO_FLOAT( prtlst[prt].lightg_fp8 ), FP8_TO_FLOAT( prtlst[prt].lightb_fp8 ), FP8_TO_FLOAT( prtlst[prt].alpha_fp8 ) };

        // Figure out the sprite's size based on distance
        size = FP8_TO_FLOAT( prtlst[prt].size_fp8 ) * 0.5f;  // [claforte] Fudge the value.

        // Fill in the rest of the data
        image = FP8_TO_FLOAT( prtlst[prt].image_fp8 + prtlst[prt].imagestt_fp8 );

        // Create the billboard vertices for this particle.
        calc_billboard(gs, vtlist, vrtlist + cnt, size, image);

        // Go on and draw it
        glBegin( GL_TRIANGLE_FAN );
        glColor4fv( color_component.v );  //[claforte] should use alpha_component instead of 0.5?
        for ( i = 0; i < 4; i++ )
        {
          glTexCoord2fv( vtlist[i].tx._v  );
          glVertex3fv( vtlist[i].pos.v );
        }
        glEnd();
      }
    }
  }
  glPopAttrib();
};


//--------------------------------------------------------------------------------------------
bool_t calc_billboard(Game_t * gs, GLVertex vrtlst[], GLVertex * vert, float size, Uint16 image)
{
  Graphics_Data_t * gfx = gfxState.pGfx;

  if(NULL == vrtlst || NULL == vert) return bfalse;

  if(0==size)
  {
    vrtlst[0].pos.x = vert->pos.x;
    vrtlst[0].pos.y = vert->pos.y;
    vrtlst[0].pos.z = vert->pos.z;

    vrtlst[1].pos.x = vert->pos.x;
    vrtlst[1].pos.y = vert->pos.y;
    vrtlst[1].pos.z = vert->pos.z;

    vrtlst[2].pos.x = vert->pos.x;
    vrtlst[2].pos.y = vert->pos.y;
    vrtlst[2].pos.z = vert->pos.z;

    vrtlst[3].pos.x = vert->pos.x;
    vrtlst[3].pos.y = vert->pos.y;
    vrtlst[3].pos.z = vert->pos.z;
  }
  else
  {
    vrtlst[0].pos.x = vert->pos.x + (-vert->rt.x - vert->up.x ) * size;
    vrtlst[0].pos.y = vert->pos.y + (-vert->rt.y - vert->up.y ) * size;
    vrtlst[0].pos.z = vert->pos.z + (-vert->rt.z - vert->up.z ) * size;

    vrtlst[1].pos.x = vert->pos.x + ( vert->rt.x - vert->up.x ) * size;
    vrtlst[1].pos.y = vert->pos.y + ( vert->rt.y - vert->up.y ) * size;
    vrtlst[1].pos.z = vert->pos.z + ( vert->rt.z - vert->up.z ) * size;

    vrtlst[2].pos.x = vert->pos.x + ( vert->rt.x + vert->up.x ) * size;
    vrtlst[2].pos.y = vert->pos.y + ( vert->rt.y + vert->up.y ) * size;
    vrtlst[2].pos.z = vert->pos.z + ( vert->rt.z + vert->up.z ) * size;

    vrtlst[3].pos.x = vert->pos.x + (-vert->rt.x + vert->up.x ) * size;
    vrtlst[3].pos.y = vert->pos.y + (-vert->rt.y + vert->up.y ) * size;
    vrtlst[3].pos.z = vert->pos.z + (-vert->rt.z + vert->up.z ) * size;
  }

  vrtlst[0].tx.s = CALCULATE_PRT_U0( image );
  vrtlst[0].tx.t = CALCULATE_PRT_V0( image );

  vrtlst[1].tx.s = CALCULATE_PRT_U1( image );
  vrtlst[1].tx.t = CALCULATE_PRT_V0( image );

  vrtlst[2].tx.s = CALCULATE_PRT_U1( image );
  vrtlst[2].tx.t = CALCULATE_PRT_V1( image );

  vrtlst[3].tx.s = CALCULATE_PRT_U0( image );
  vrtlst[3].tx.t = CALCULATE_PRT_V1( image );

  return 0!=size;
}

//--------------------------------------------------------------------------------------------
void render_light_prt( Uint32 vrtcount, GLVertex * vrtlist )
{
  Game_t * gs = Graphics_requireGame(&gfxState);
  PPrt_t   prtlst = gs->PrtList;
  Prt_t  * pprt;

  GLVertex vtlist[4];
  GLvector color_component;
  Uint16 cnt;
  PRT_REF prt;
  CHR_REF chr;
  PIP_REF pip;
  Uint16 image;
  float size;
  int i;

  ATTRIB_PUSH( "render_light_prt", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT );
  {
    glDepthMask( GL_FALSE );

    glEnable( GL_ALPHA_TEST );
    glAlphaFunc( GL_GREATER, 0 );

    glEnable( GL_BLEND );
    glBlendFunc( GL_ONE, GL_ONE );


    for ( cnt = 0; cnt < vrtcount; cnt++ )
    {
      // Get the index from the color slot
      prt = ( Uint16 ) vrtlist[cnt].color;
      if( !ACTIVE_PRT(prtlst, prt) ) continue;
      pprt = prtlst + prt;

      chr = prt_get_attachedtochr( gs, prt );
      pip = pprt->pip;

      // Draw lights this round
      if ( pprt->type != PRTTYPE_LIGHT || 0 == pprt->alpha_fp8) continue;

      color_component.r =
      color_component.g =
      color_component.b = FP8_TO_FLOAT( pprt->alpha_fp8 );
      color_component.a = 1.0f;

      // [claforte] Fudge the value.
      size = FP8_TO_FLOAT( prtlst[prt].size_fp8 ) * 0.5f;

      // Fill in the rest of the data
      image = FP8_TO_FLOAT( pprt->image_fp8 + pprt->imagestt_fp8 );

      // Create the billboard vertices for this particle.
      calc_billboard(gs, vtlist, vrtlist + cnt, size, image);

      // Go on and draw it
      glBegin( GL_TRIANGLE_FAN );
      glColor4fv( color_component.v );
      for ( i = 0; i < 4; i++ )
      {
        glTexCoord2fv( vtlist[i].tx._v );
        glVertex3fv( vtlist[i].pos.v );
      }
      glEnd();

    }
  }
  glPopAttrib();
};


//--------------------------------------------------------------------------------------------
int cmp_particle_vertices( const void * pleft, const void * pright )
{
  // do a distance based comparison

  GLVertex *vleft = (GLVertex *)pleft, *vright = (GLVertex *)pright;

  if( NULL == vleft || NULL == vright ) return 0;

  if( vleft->col.r < vright->col.r )
  {
    return -1;
  }
  else if( vleft->col.r > vright->col.r )
  {
    return 1;
  }
  else
  {
    return 0;
  }

};

//--------------------------------------------------------------------------------------------
void sort_particles( GLVertex v[], int numparticle )
{
  // do the in-place quick sort

  if(NULL == v || 0 == numparticle) return;

  qsort(v, numparticle, sizeof(GLVertex), cmp_particle_vertices);
}

//--------------------------------------------------------------------------------------------
void render_particles()
{
  // ZZ> This function draws the sprites for particle systems

  Game_t *gs = Graphics_requireGame(&gfxState);
  Graphics_Data_t * gfx = gfxState.pGfx;
  PPrt_t   prtlst = gs->PrtList;

  GLVertex v[PRTLST_COUNT];
  Uint16 numparticle;
  PRT_REF prt_cnt;

  if ( MAXTEXTURE == particletexture || INVALID_TEXTURE == GLtexture_GetTextureID( gfx->TxTexture + particletexture ) ) return;

  // Original points
  numparticle = 0;
  for ( prt_cnt = 0; prt_cnt < PRTLST_COUNT; prt_cnt++ )
  {
    if ( !ACTIVE_PRT( prtlst, prt_cnt ) || /* !prtlst[prt_cnt].inview  || */ prtlst[prt_cnt].gopoof || prtlst[prt_cnt].size_fp8 == 0 ) continue;

    v[numparticle].pos.x = ( float ) prtlst[prt_cnt].ori.pos.x;
    v[numparticle].pos.y = ( float ) prtlst[prt_cnt].ori.pos.y;
    v[numparticle].pos.z = ( float ) prtlst[prt_cnt].ori.pos.z;

    // !!!!!PRE CALCULATE the billboard vectors so you only have to do it ONCE!!!!!!!
    get_vectors( prt_cnt, &v[numparticle].up, &v[numparticle].rt, &v[numparticle].col.r );

    // [claforte] Aaron did a horrible hack here. Fix that ASAP.
    v[numparticle].color = REF_TO_INT(prt_cnt);  // Store an index in the color slot...
    numparticle++;
  }

  if ( 0 == numparticle ) return;

  // sort particles by distance
  sort_particles( v, numparticle );

  ATTRIB_PUSH( "render_particles", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT | GL_POLYGON_BIT );
  {
    // Flat shade these babies
    glShadeModel( gfxState.shading );

    glDisable( GL_CULL_FACE );
    glDisable( GL_DITHER );

    // Choose texture
    GLtexture_Bind( gfx->TxTexture + particletexture, &gfxState );

    // DO ANTIALIAS SOLID SPRITES FIRST
    render_antialias_prt( numparticle, v );

    // DO SOLID SPRITES FIRST
    render_solid_prt( numparticle, v );

    // LIGHTS DONE LAST
    render_light_prt( numparticle, v );

    // DO TRANSPARENT SPRITES NEXT
    render_transparent_prt( numparticle, v );
  }
  glPopAttrib();

};

//--------------------------------------------------------------------------------------------
void render_antialias_prt_ref( Uint32 vrtcount, GLVertex * vrtlist )
{
  Game_t *gs = Graphics_requireGame(&gfxState);
  PPrt_t  prtlst = gs->PrtList;

  GLVertex vtlist[4];
  Uint16 cnt;
  PRT_REF prt;
  CHR_REF chr;
  PIP_REF pip;
  Uint16 image;
  float size;
  int i;

  ATTRIB_PUSH( "render_antialias_prt_ref", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT );
  {
    glDepthMask( GL_FALSE );

    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LESS );

    glEnable( GL_ALPHA_TEST );
    glAlphaFunc( GL_GREATER, 0 );

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    // Render each particle that was on
    for ( cnt = 0; cnt < vrtcount; cnt++ )
    {
      // Get the index from the color slot
      prt = ( Uint16 ) vrtlist[cnt].color;
      chr = prt_get_attachedtochr( gs, prt );
      pip = prtlst[prt].pip;

      // Render solid ones twice...  For Antialias
      if ( prtlst[prt].type != PRTTYPE_SOLID ) continue;

      {
        GLvector color_component = {FP8_TO_FLOAT( prtlst[prt].lightr_fp8 ), FP8_TO_FLOAT( prtlst[prt].lightg_fp8 ), FP8_TO_FLOAT( prtlst[prt].lightb_fp8 ), FP8_TO_FLOAT( antialiastrans_fp8 ) };

        // Figure out the sprite's size based on distance
        size = FP8_TO_FLOAT( prtlst[prt].size_fp8 ) * 0.25f * 1.1f;  // [claforte] Fudge the value.

        // Fill in the rest of the data
        image = FP8_TO_FLOAT( prtlst[prt].image_fp8 + prtlst[prt].imagestt_fp8 );

        // Create the billboard vertices for this particle.
        calc_billboard(gs, vtlist, vrtlist + cnt, size, image);

        // Go on and draw it
        glBegin( GL_TRIANGLE_FAN );
        glColor4fv( color_component.v );
        for ( i = 0; i < 4; i++ )
        {
          glTexCoord2fv( vtlist[i].tx._v );
          glVertex3fv( vtlist[i].pos.v );
        }
        glEnd();
      }
    }
  }
  glPopAttrib();

};

//--------------------------------------------------------------------------------------------
void render_solid_prt_ref( Uint32 vrtcount, GLVertex * vrtlist )
{
  Game_t * gs = Graphics_requireGame(&gfxState);
  PPrt_t   prtlst = gs->PrtList;

  GLVertex vtlist[4];
  Uint16 cnt;
  PRT_REF prt;
  CHR_REF chr;
  PIP_REF pip;
  Uint16 image;
  float size;
  int i;

  ATTRIB_PUSH( "render_solid_prt_ref", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT );
  {
    // Render each particle that was on
    glDepthMask( GL_TRUE );

    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LEQUAL );

    glDisable( GL_BLEND );
    glEnable( GL_ALPHA_TEST );
    glAlphaFunc( GL_GREATER, 0 );


    for ( cnt = 0; cnt < vrtcount; cnt++ )
    {
      // Get the index from the color slot
      prt = ( Uint16 ) vrtlist[cnt].color;
      chr = prt_get_attachedtochr( gs, prt );
      pip = prtlst[prt].pip;

      // Draw sprites this round
      if ( prtlst[prt].type != PRTTYPE_SOLID ) continue;

      {
        GLvector color_component = {FP8_TO_FLOAT( prtlst[prt].lightr_fp8 ), FP8_TO_FLOAT( prtlst[prt].lightg_fp8 ), FP8_TO_FLOAT( prtlst[prt].lightb_fp8 ), 1};

        // [claforte] Fudge the value.
        size = FP8_TO_FLOAT( prtlst[prt].size_fp8 ) * 0.25f;

        // Fill in the rest of the data
        image = FP8_TO_FLOAT( prtlst[prt].image_fp8 + prtlst[prt].imagestt_fp8 );

        // Create the billboard vertices for this particle.
        calc_billboard(gs, vtlist, vrtlist + cnt, size, image);

        glBegin( GL_TRIANGLE_FAN );
        glColor4fv( color_component.v );
        for ( i = 0; i < 4; i++ )
        {
          glTexCoord2fv( vtlist[i].tx._v );
          glVertex3fv( vtlist[i].pos.v );
        }
        glEnd();
      }
    }
  }
  glPopAttrib();
};

//--------------------------------------------------------------------------------------------
void render_transparent_prt_ref( Uint32 vrtcount, GLVertex * vrtlist )
{
  Game_t * gs = Graphics_requireGame(&gfxState);
  PPrt_t   prtlst = gs->PrtList;

  GLVertex vtlist[4];
  Uint16 cnt;
  PRT_REF prt;
  CHR_REF chr;
  PIP_REF pip;
  Uint16 image;
  float size;
  int i;

  ATTRIB_PUSH( "render_transparent_prt_ref", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT );
  {
    glDepthMask( GL_FALSE );

    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LESS );

    glEnable( GL_ALPHA_TEST );
    glAlphaFunc( GL_GREATER, 0 );

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    for ( cnt = 0; cnt < vrtcount; cnt++ )
    {
      // Get the index from the color slot
      prt = ( Uint16 ) vrtlist[cnt].color;
      chr = prt_get_attachedtochr( gs, prt );
      pip = prtlst[prt].pip;

      // Draw transparent sprites this round
      if ( prtlst[prt].type != PRTTYPE_ALPHA ) continue;

      {
        GLvector color_component = { FP8_TO_FLOAT( prtlst[prt].lightr_fp8 ), FP8_TO_FLOAT( prtlst[prt].lightg_fp8 ), FP8_TO_FLOAT( prtlst[prt].lightb_fp8 ), FP8_TO_FLOAT( prtlst[prt].alpha_fp8 ) };

        // Figure out the sprite's size based on distance
        size = FP8_TO_FLOAT( prtlst[prt].size_fp8 ) * 0.25f;  // [claforte] Fudge the value.

        // Fill in the rest of the data
        image = FP8_TO_FLOAT( prtlst[prt].image_fp8 + prtlst[prt].imagestt_fp8 );

        // Create the billboard vertices for this particle.
        calc_billboard(gs, vtlist, vrtlist + cnt, size, image);

        // Go on and draw it
        glBegin( GL_TRIANGLE_FAN );
        glColor4fv( color_component.v );  //[claforte] should use alpha_component instead of 0.5?
        for ( i = 0; i < 4; i++ )
        {
          glTexCoord2fv( vtlist[i].tx._v );
          glVertex3fv( vtlist[i].pos.v );
        }
        glEnd();
      }

    }
  }
  glPopAttrib();
};

//--------------------------------------------------------------------------------------------
void render_light_prt_ref( Uint32 vrtcount, GLVertex * vrtlist )
{
  Game_t * gs = Graphics_requireGame(&gfxState);
  PPrt_t   prtlst = gs->PrtList;

  GLVertex vtlist[4];
  Uint16 cnt;
  PRT_REF prt;
  CHR_REF chr;
  PIP_REF pip;
  Uint16 image;
  float size;
  int i;

  ATTRIB_PUSH( "render_light_prt_ref", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT | GL_CURRENT_BIT );
  {
    glDepthMask( GL_FALSE );

    glEnable( GL_ALPHA_TEST );
    glAlphaFunc( GL_GREATER, 0 );

    glEnable( GL_BLEND );
    glBlendFunc( GL_ONE, GL_ONE );


    for ( cnt = 0; cnt < vrtcount; cnt++ )
    {
      GLvector color_component;
      // Get the index from the color slot
      prt = ( Uint16 ) vrtlist[cnt].color;
      color_component.r = color_component.g = color_component.b = FP8_TO_FLOAT( prtlst[prt].alpha_fp8 );
      color_component.a = 1.0f;
      chr = prt_get_attachedtochr( gs, prt );
      pip = prtlst[prt].pip;

      // Draw lights this round
      if ( prtlst[prt].type != PRTTYPE_LIGHT ) continue;

      // [claforte] Fudge the value.
      size = FP8_TO_FLOAT( prtlst[prt].size_fp8 ) * 0.5f;

      // Fill in the rest of the data
      image = FP8_TO_FLOAT( prtlst[prt].image_fp8 + prtlst[prt].imagestt_fp8 );

      // Create the billboard vertices for this particle.
      calc_billboard(gs, vtlist, vrtlist + cnt, size, image);

      // Go on and draw it
      glBegin( GL_TRIANGLE_FAN );
      glColor4fv( color_component.v );
      for ( i = 0; i < 4; i++ )
      {
        glTexCoord2fv( vtlist[i].tx._v );
        glVertex3fv( vtlist[i].pos.v );
      }
      glEnd();
    }

  }
  glPopAttrib();
};

//--------------------------------------------------------------------------------------------
void render_particle_reflections()
{
  // ZZ> This function draws the sprites for particle systems

  Game_t * gs    = Graphics_requireGame(&gfxState);
  Graphics_Data_t * gfx = gfxState.pGfx;

  PPrt_t   prtlst = gs->PrtList;
  Mesh_t * pmesh = Game_getMesh(gs);

  GLVertex v[PRTLST_COUNT];
  Uint16 numparticle;
  PRT_REF prt_cnt;
  float level;

  if ( INVALID_TEXTURE == GLtexture_GetTextureID( gfx->TxTexture + particletexture ) )
    return;

  // Original points
  numparticle = 0;
  for ( prt_cnt = 0; prt_cnt < PRTLST_COUNT; prt_cnt++ )
  {
    if ( !ACTIVE_PRT( prtlst, prt_cnt ) || !prtlst[prt_cnt].inview || prtlst[prt_cnt].size_fp8 == 0 ) continue;

    if ( mesh_has_some_bits( pmesh->Mem.tilelst, prtlst[prt_cnt].onwhichfan, MPDFX_SHINY ) )
    {
      level = prtlst[prt_cnt].level;
      v[numparticle].pos.x = prtlst[prt_cnt].ori.pos.x;
      v[numparticle].pos.y = prtlst[prt_cnt].ori.pos.y;
      v[numparticle].pos.z = level + level - prtlst[prt_cnt].ori.pos.z;

      // !!!!!PRE CALCULATE the billboard vectors so you only have to do it ONCE!!!!!!!
      get_vectors( prt_cnt, &v[numparticle].up, &v[numparticle].rt, &v[numparticle].col.r );

      v[numparticle].color = REF_TO_INT(prt_cnt);  // Store an index in the color slot...
      numparticle++;
    }
  }

  if ( 0 == numparticle ) return;

  // sort the particles by distance
  sort_particles( v, numparticle );


  ATTRIB_PUSH( "render_particle_reflections", GL_ENABLE_BIT | GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_LIGHTING_BIT | GL_POLYGON_BIT );
  {
    // Flat shade these babies
    glShadeModel( gfxState.shading );

    // Choose texture and matrix
    GLtexture_Bind( gfx->TxTexture + particletexture, &gfxState );

    glDisable( GL_CULL_FACE );
    glDisable( GL_DITHER );

    // DO ANTIALIAS SOLID SPRITES FIRST
    ATTRIB_GUARD_OPEN( prt_attrib_open );
    render_antialias_prt_ref( numparticle, v );
    ATTRIB_GUARD_CLOSE( prt_attrib_open, prt_attrib_close );

    // DO SOLID SPRITES FIRST
    ATTRIB_GUARD_OPEN( prt_attrib_open );
    render_solid_prt_ref( numparticle, v );
    ATTRIB_GUARD_CLOSE( prt_attrib_open, prt_attrib_close );

    // DO TRANSPARENT SPRITES NEXT
    ATTRIB_GUARD_OPEN( prt_attrib_open );
    render_transparent_prt_ref( numparticle, v );
    ATTRIB_GUARD_CLOSE( prt_attrib_open, prt_attrib_close );

    // LIGHTS DONE LAST
    ATTRIB_GUARD_OPEN( prt_attrib_open );
    render_light_prt_ref( numparticle, v );
    ATTRIB_GUARD_CLOSE( prt_attrib_open, prt_attrib_close );
  }
  glPopAttrib();

}


