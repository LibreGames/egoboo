//********************************************************************************************
//* Egoboo - graphic_mad.c
//*
//* Character model drawing code.
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
#include "Mad.h"
#include "id_md2.h"
#include "Log.h"
#include "camera.h"

#include "egoboo.h"

#include <assert.h>

#include "char.inl"
#include "Md2.inl"
#include "input.inl"
#include "graphic.inl"

float sinlut[MAXLIGHTROTATION];
float coslut[MAXLIGHTROTATION];

float spek_global_lighting( int rotation, int normal, vect3 lite );
float spek_local_lighting( int rotation, int normal );
float spek_calc_local_lighting( Uint16 turn, vect3 nrm );
void make_speklut();
void make_spektable( vect3 lite );
void make_lighttospek( void );

void   chr_draw_BBox(CHR_REF ichr);

//---------------------------------------------------------------------------------------------
// md2_blend_vertices
// Blends the vertices and normals between 2 frames of a md2 model for animation.
//
// NOTE: Only meant to be called from draw_textured_md2, which does the necessary
// checks to make sure that the inputs are valid.  So this function itself assumes
// that they are valid.  User beware!
//
void md2_blend_vertices(CChr * pchr, Sint32 vrtmin, Sint32 vrtmax)
{
  MD2_Model * pmd2;
  const MD2_Frame *pfrom, *pto;

  Sint32 numVertices, i;
  vect2  off;
  float lerp;

  OBJ_REF iobj;
  CObj  * pobj;

  CMad  * pmad;

  CGame * gs = gfxState.gs;

  if( !EKEY_PVALID(pchr) ) return;

  iobj = pchr->model = VALIDATE_OBJ(gs->ObjList, pchr->model);
  if( INVALID_OBJ == iobj) return;
  pobj = gs->ObjList + iobj;

  pmad = ObjList_getPMad(gs, iobj);
  if(NULL == pmad) return;

  pmd2 = pmad->md2_ptr;
  if(NULL == pmd2) return;

  // handle "default" argument
  if(vrtmin<0)
  {
    vrtmin = 0;
  };

  // handle "default" argument
  if(vrtmax<0)
  {
    vrtmax = md2_get_numVertices(pmd2);
  }

  // check pto see if the blend is already cached
  if( (pchr->anim.last == pchr->vdata.frame0) &&
      (pchr->anim.next == pchr->vdata.frame1) &&
      (vrtmin         >= pchr->vdata.vrtmin) &&
      (vrtmax         <= pchr->vdata.vrtmax) &&
      (pchr->anim.flip  == pchr->vdata.lerp))
      return;


  pfrom  = md2_get_Frame(pmd2, pchr->anim.last);
  pto    = md2_get_Frame(pmd2, pchr->anim.next);
  lerp  = pchr->anim.flip;

  off.u = FP8_TO_FLOAT(pchr->uoffset_fp8);
  off.v = FP8_TO_FLOAT(pchr->voffset_fp8);

  // do some error trapping
  if(NULL ==pfrom && NULL ==pto) return;
  else if(NULL ==pfrom) lerp = 1.0f;
  else if(NULL ==pto  ) lerp = 0.0f;
  else if(pfrom==pto  ) lerp = 0.0f;

  // do the interpolating
  numVertices = md2_get_numVertices(pmd2);

  if (lerp <= 0)
  {
    // copy the vertices in frame 'pfrom' over
    for (i=vrtmin; i<numVertices && i<=vrtmax; i++)
    {
      pchr->vdata.Vertices[i].x = pfrom->vertices[i].x;
      pchr->vdata.Vertices[i].y = pfrom->vertices[i].y;
      pchr->vdata.Vertices[i].z = pfrom->vertices[i].z;

      pchr->vdata.Vertices[i].x *= -1;

      pchr->vdata.Normals[i].x = kMd2Normals[pfrom->vertices[i].normal][0];
      pchr->vdata.Normals[i].y = kMd2Normals[pfrom->vertices[i].normal][1];
      pchr->vdata.Normals[i].z = kMd2Normals[pfrom->vertices[i].normal][2];

      pchr->vdata.Normals[i].x *= -1;

      if(pchr->enviro)
      {
        pchr->vdata.Texture[i].s  = off.u + CLIP((1-pchr->vdata.Normals[i].z)/2.0f,0,1);
        pchr->vdata.Texture[i].t  = off.v + atan2(pchr->vdata.Normals[i].y, pchr->vdata.Normals[i].x)*INV_TWO_PI - pchr->ori.turn_lr/(float)(1<<16);
        pchr->vdata.Texture[i].t -= atan2(pchr->ori.pos.y-GCamera.pos.y, pchr->ori.pos.x-GCamera.pos.x)*INV_TWO_PI;
      };
    }
  }
  else if (lerp >= 1.0f)
  {
    // copy the vertices in frame 'pto'
    for (i=vrtmin; i<numVertices && i<=vrtmax; i++)
    {
      pchr->vdata.Vertices[i].x = pto->vertices[i].x;
      pchr->vdata.Vertices[i].y = pto->vertices[i].y;
      pchr->vdata.Vertices[i].z = pto->vertices[i].z;

      pchr->vdata.Vertices[i].x  *= -1;

      pchr->vdata.Normals[i].x = kMd2Normals[pto->vertices[i].normal][0];
      pchr->vdata.Normals[i].y = kMd2Normals[pto->vertices[i].normal][1];
      pchr->vdata.Normals[i].z = kMd2Normals[pto->vertices[i].normal][2];

      pchr->vdata.Normals[i].x  *= -1;

      if(pchr->enviro)
      {
        pchr->vdata.Texture[i].s  = off.u + CLIP((1-pchr->vdata.Normals[i].z)/2.0f,0,1);
        pchr->vdata.Texture[i].t  = off.v + atan2(pchr->vdata.Normals[i].y, pchr->vdata.Normals[i].x )*INV_TWO_PI - pchr->ori.turn_lr/(float)(1<<16);
        pchr->vdata.Texture[i].t -= atan2(pchr->ori.pos.y-GCamera.pos.y, pchr->ori.pos.x-GCamera.pos.x )*INV_TWO_PI;
      };
    }
  }
  else
  {
    // mix the vertices
    for (i=vrtmin; i<numVertices && i<=vrtmax; i++)
    {
      pchr->vdata.Vertices[i].x = pfrom->vertices[i].x + (pto->vertices[i].x - pfrom->vertices[i].x) * lerp;
      pchr->vdata.Vertices[i].y = pfrom->vertices[i].y + (pto->vertices[i].y - pfrom->vertices[i].y) * lerp;
      pchr->vdata.Vertices[i].z = pfrom->vertices[i].z + (pto->vertices[i].z - pfrom->vertices[i].z) * lerp;

      pchr->vdata.Vertices[i].x  *= -1;

      pchr->vdata.Normals[i].x = kMd2Normals[pfrom->vertices[i].normal][0] + (kMd2Normals[pto->vertices[i].normal][0] - kMd2Normals[pfrom->vertices[i].normal][0]) * lerp;
      pchr->vdata.Normals[i].y = kMd2Normals[pfrom->vertices[i].normal][1] + (kMd2Normals[pto->vertices[i].normal][1] - kMd2Normals[pfrom->vertices[i].normal][1]) * lerp;
      pchr->vdata.Normals[i].z = kMd2Normals[pfrom->vertices[i].normal][2] + (kMd2Normals[pto->vertices[i].normal][2] - kMd2Normals[pfrom->vertices[i].normal][2]) * lerp;

      pchr->vdata.Normals[i].x  *= -1;

      if(pchr->enviro)
      {
        pchr->vdata.Texture[i].s  = off.u + CLIP((1-pchr->vdata.Normals[i].z)/2.0f,0,1);
        pchr->vdata.Texture[i].t  = off.v + atan2(pchr->vdata.Normals[i].y, pchr->vdata.Normals[i].x)*INV_TWO_PI - pchr->ori.turn_lr/(float)(1<<16);
        pchr->vdata.Texture[i].t -= atan2(pchr->ori.pos.y-GCamera.pos.y, pchr->ori.pos.x-GCamera.pos.x)*INV_TWO_PI;
      };
    }
  }


  // cache the blend parameters
  pchr->vdata.frame0 = pchr->anim.last;
  pchr->vdata.frame1 = pchr->anim.next;
  pchr->vdata.vrtmin = vrtmin;
  pchr->vdata.vrtmax = vrtmax;
  pchr->vdata.lerp   = pchr->anim.flip;
  pchr->vdata.needs_lighting = btrue;

  // invalidate the cached bumper data
  pchr->bmpdata.cv.lod = -1;
}

//---------------------------------------------------------------------------------------------
void md2_blend_lighting(CChr * pchr)
{
  CGame * gs = gfxState.gs;

  Uint16  sheen_fp8, spekularity_fp8;
  Uint16 lightnew_r, lightnew_g, lightnew_b;
  Uint16 lightold_r, lightold_g, lightold_b;
  Uint16 lightrotationr, lightrotationg, lightrotationb;
  Uint16 ambilevelr_fp8, ambilevelg_fp8, ambilevelb_fp8;
  Uint16  speklevelr_fp8, speklevelg_fp8, speklevelb_fp8;
  Uint8  r_sft, g_sft, b_sft;
  Uint16 trans;
  vect2  offset;

  Uint32 i, numVertices;
  VData_Blended * vd;

  OBJ_REF         iobj;
  CObj          * pobj;

  CMad          * pmad;
  MD2_Model     * pmd2;

  CCap          * pcap;

  if( !EKEY_PVALID(pchr) ) return;
  vd = &(pchr->vdata);

  // only calculate the lighting if it is "needed"
  // TODO: this is only sensitive to changes in the character orientation and
  //       vertex blending.  it needs to be sensitive to global lighting changes, too.
  if(!vd->needs_lighting) return;

  iobj = pchr->model = VALIDATE_OBJ(gs->ObjList, pchr->model);
  if( INVALID_OBJ == iobj) return;
  pobj = gs->ObjList + iobj;

  pmad = ObjList_getPMad(gs, iobj);
  if(NULL == pmad) return;

  pmd2 = pmad->md2_ptr;
  if(NULL == pmd2) return;

  pcap = ObjList_getPCap(gs, iobj);
  if(NULL == pcap) return;

  sheen_fp8       = pchr->sheen_fp8;
  spekularity_fp8 = FLOAT_TO_FP8( ( float )sheen_fp8 / ( float )MAXSPEKLEVEL );

  lightrotationr = pchr->ori.turn_lr + pchr->tlight.turn_lr.r;
  lightrotationg = pchr->ori.turn_lr + pchr->tlight.turn_lr.g;
  lightrotationb = pchr->ori.turn_lr + pchr->tlight.turn_lr.b;

  ambilevelr_fp8 = pchr->tlight.ambi_fp8.r;
  ambilevelg_fp8 = pchr->tlight.ambi_fp8.g;
  ambilevelb_fp8 = pchr->tlight.ambi_fp8.b;

  speklevelr_fp8 = pchr->tlight.spek_fp8.r;
  speklevelg_fp8 = pchr->tlight.spek_fp8.g;
  speklevelb_fp8 = pchr->tlight.spek_fp8.b;

  offset.u = textureoffset[ FP8_TO_INT( pchr->uoffset_fp8 )];
  offset.v = textureoffset[ FP8_TO_INT( pchr->voffset_fp8 )];

  r_sft = pchr->redshift;
  g_sft = pchr->grnshift;
  b_sft = pchr->blushift;

  trans = pcap->alpha_fp8;

  numVertices = md2_get_numVertices(pmd2);
  if(gfxState.shading != GL_FLAT)
  {

    // mix the vertices
    for (i=0; i<numVertices; i++)
    {
      float ftmp;

      lightnew_r = 0;
      lightnew_g = 0;
      lightnew_b = 0;

      ftmp = DotProduct( vd->Normals[i], GLight.spekdir );
      if ( ftmp > 0.0f )
      {
        lightnew_r += ftmp * ftmp * spekularity_fp8 * GLight.spekcol.r;
        lightnew_g += ftmp * ftmp * spekularity_fp8 * GLight.spekcol.g;
        lightnew_b += ftmp * ftmp * spekularity_fp8 * GLight.spekcol.b;
      }

      lightnew_r += speklevelr_fp8 * spek_calc_local_lighting(lightrotationr, vd->Normals[i]);
      lightnew_g += speklevelg_fp8 * spek_calc_local_lighting(lightrotationg, vd->Normals[i]);
      lightnew_b += speklevelb_fp8 * spek_calc_local_lighting(lightrotationb, vd->Normals[i]);

      lightnew_r = lighttospek[sheen_fp8][lightnew_r] + ambilevelr_fp8 + GLight.ambicol.r * 255;
      lightnew_g = lighttospek[sheen_fp8][lightnew_g] + ambilevelg_fp8 + GLight.ambicol.g * 255;
      lightnew_b = lighttospek[sheen_fp8][lightnew_b] + ambilevelb_fp8 + GLight.ambicol.b * 255;

      lightold_r = vd->Colors[i].r;
      lightold_g = vd->Colors[i].g;
      lightold_b = vd->Colors[i].b;

      vd->Colors[i].r = MIN( 255, 0.9 * lightold_r + 0.1 * lightnew_r );
      vd->Colors[i].g = MIN( 255, 0.9 * lightold_g + 0.1 * lightnew_g );
      vd->Colors[i].b = MIN( 255, 0.9 * lightold_b + 0.1 * lightnew_b );
      vd->Colors[i].a = 1.0f;

      vd->Colors[i].r /= (float)(1 << r_sft) * FP8_TO_FLOAT(trans);
      vd->Colors[i].g /= (float)(1 << g_sft) * FP8_TO_FLOAT(trans);
      vd->Colors[i].b /= (float)(1 << b_sft) * FP8_TO_FLOAT(trans);
    }
  }
  else
  {
    for (i=0; i<numVertices; i++)
    {
      vd->Colors[i].r  = FP8_TO_FLOAT(ambilevelr_fp8);
      vd->Colors[i].g  = FP8_TO_FLOAT(ambilevelg_fp8);
      vd->Colors[i].b  = FP8_TO_FLOAT(ambilevelb_fp8);
      vd->Colors[i].a  = 1.0;

      vd->Colors[i].r /= (float)(1 << r_sft) * FP8_TO_FLOAT(trans);
      vd->Colors[i].g /= (float)(1 << g_sft) * FP8_TO_FLOAT(trans);
      vd->Colors[i].b /= (float)(1 << b_sft) * FP8_TO_FLOAT(trans);
    };

  };

  vd->needs_lighting = bfalse;
}



/* md2_blend_vertices
* Blends the vertices and normals between 2 frames of a md2 model for animation.
*
* NOTE: Only meant to be called from draw_textured_md2, which does the necessary
* checks to make sure that the inputs are valid.  So this function itself assumes
* that they are valid.  User beware!
*/

//--------------------------------------------------------------------------------------------
// Draws a JF::MD2_Model in the new format
// using OpenGL commands in the MD2 for acceleration
void draw_textured_md2_opengl(CHR_REF ichr)
{
  const MD2_GLCommand * cmd;
  Uint32 cmd_count;
  vect2  off;

  CGame * gs   = gfxState.gs;
  CChr  * pchr = ChrList_getPChr(gs, ichr);
  CMad  * pmad = ChrList_getPMad(gs, ichr);

  VData_Blended * vd   = &(pchr->vdata);
  MD2_Model     * pmd2 = pmad->md2_ptr;

  md2_blend_vertices(pchr, -1, -1);
  md2_blend_lighting(pchr);
  chr_calculate_bumpers(gs, pchr, 0);

  off.u = FP8_TO_FLOAT(pchr->uoffset_fp8);
  off.v = FP8_TO_FLOAT(pchr->voffset_fp8);

  if(gfxState.shading != GL_FLAT)
  {
    glEnableClientState(GL_NORMAL_ARRAY);
    glNormalPointer(GL_FLOAT, 0, vd->Normals[0].v);
  }

  glEnableClientState(GL_VERTEX_ARRAY);
  glVertexPointer(3, GL_FLOAT, 0, vd->Vertices[0].v);

  glEnableClientState(GL_COLOR_ARRAY);
  glColorPointer (4, GL_FLOAT, 0, vd->Colors[0].v);


    cmd  = md2_get_Commands(pmd2);
    for (/*nothing */; NULL !=cmd; cmd = cmd->next)
    {
      Uint32 i;
      glBegin(cmd->gl_mode);

      cmd_count = cmd->command_count;
      for(i=0; i<cmd_count; i++)
      {
        glTexCoord2f( cmd->data[i].s + off.u, cmd->data[i].t + off.v );
        glArrayElement( cmd->data[i].index );
      }
      glEnd();
    }


  glDisableClientState(GL_VERTEX_ARRAY);
  glDisableClientState(GL_COLOR_ARRAY);
  if(gfxState.shading != GL_FLAT) glDisableClientState(GL_NORMAL_ARRAY);


#if defined(DEBUG_CHR_NORMALS) && defined(_DEBUG)
  if ( CData.DevMode )
  {
    int cnt;
    int verts = pmad->transvertices;

      glBegin( GL_LINES );
      glLineWidth( 2.0f );
      glColor4f( 1, 1, 1, 1 );
      for ( cnt = 0; cnt < verts; cnt++ )
      {
        glVertex3fv( vd->Vertices[cnt].v );
        glVertex3f( vd->Vertices[cnt].x + vd->Normals[cnt].x*10, vd->Vertices[cnt].y + vd->Normals[cnt].y*10, vd->Vertices[cnt].z + vd->Normals[cnt].z*10 );
      };
      glEnd();
  }
#endif

}

//--------------------------------------------------------------------------------------------
// Draws a JF::MD2_Model in the new format
// using OpenGL commands in the MD2 for acceleration
void draw_enviromapped_md2_opengl(CHR_REF ichr)
{
  const MD2_GLCommand * cmd;
  Uint32 cmd_count;

  CGame * gs    = gfxState.gs;
  CChr  * pchr = ChrList_getPChr(gs, ichr);
  CMad  * pmad = ChrList_getPMad(gs, ichr);

  VData_Blended * vd   = &(pchr->vdata);
  MD2_Model     * pmd2 = pmad->md2_ptr;

  md2_blend_vertices(pchr, -1, -1);
  md2_blend_lighting(pchr);
  chr_calculate_bumpers(gs, pchr, 0);

  glEnableClientState(GL_NORMAL_ARRAY);
  glNormalPointer( GL_FLOAT, 0, vd->Normals[0].v );

  glEnableClientState(GL_VERTEX_ARRAY);
  glVertexPointer(3, GL_FLOAT, 0, vd->Vertices[0].v);

  glEnableClientState(GL_COLOR_ARRAY);
  glColorPointer (4, GL_FLOAT, 0, vd->Colors[0].v);

  glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  glTexCoordPointer(2, GL_FLOAT, 0, vd->Texture[0]._v);


    cmd = md2_get_Commands(pmd2);
    for (/*nothing */; NULL !=cmd; cmd = cmd->next)
    {
      Uint32 i;
      glBegin(cmd->gl_mode);

      cmd_count = cmd->command_count;
      for(i=0; i<cmd_count; i++)
      {
        glArrayElement( cmd->data[i].index );
      }
      glEnd();
    }


  glDisableClientState(GL_VERTEX_ARRAY);
  glDisableClientState(GL_COLOR_ARRAY);
  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
  glDisableClientState(GL_NORMAL_ARRAY);

#if defined(DEBUG_CHR_NORMALS) && defined(_DEBUG)
  if ( CData.DevMode )
  {
    int cnt;
    int verts = pmad->transvertices;


      glBegin( GL_LINES );
      glLineWidth( 2.0f );
      glColor4f( 1, 1, 1, 1 );
      for ( cnt = 0; cnt < verts; cnt++ )
      {
        glVertex3fv( vd->Vertices[cnt].v );
        glVertex3f( vd->Vertices[cnt].x + vd->Normals[cnt].x*10, vd->Vertices[cnt].y + vd->Normals[cnt].y*10, vd->Vertices[cnt].z + vd->Normals[cnt].z*10 );
      };
      glEnd();


  }
#endif
}

//--------------------------------------------------------------------------------------------
void calc_lighting_data( CGame * gs, CChr * pchr )
{
  LData * pldata = &(pchr->ldata);

  Uint16 sheen_fp8       = pchr->sheen_fp8;
  Uint16 spekularity_fp8 = FLOAT_TO_FP8(( float ) sheen_fp8 / ( float ) MAXSPEKLEVEL );
  Uint16 texture         = pchr->skin_ref + gs->ObjList[pchr->model].skinstart;

  Uint8 r_sft = pchr->redshift;
  Uint8 g_sft = pchr->grnshift;
  Uint8 b_sft = pchr->blushift;

  float ftmp;

  ftmp = (( float )( MAXSPEKLEVEL - sheen_fp8 ) / ( float ) MAXSPEKLEVEL ) * ( FP8_TO_FLOAT( pchr->alpha_fp8 ) );
  pldata->diffuse.r = ftmp * FP8_TO_FLOAT( pchr->alpha_fp8 ) / ( float )( 1 << r_sft );
  pldata->diffuse.g = ftmp * FP8_TO_FLOAT( pchr->alpha_fp8 ) / ( float )( 1 << g_sft );
  pldata->diffuse.b = ftmp * FP8_TO_FLOAT( pchr->alpha_fp8 ) / ( float )( 1 << b_sft );


  ftmp = (( float ) sheen_fp8 / ( float ) MAXSPEKLEVEL ) * ( FP8_TO_FLOAT( pchr->alpha_fp8 ) );
  pldata->specular.r = ftmp / ( float )( 1 << r_sft );
  pldata->specular.g = ftmp / ( float )( 1 << g_sft );
  pldata->specular.b = ftmp / ( float )( 1 << b_sft );

  pldata->shininess[0] = sheen_fp8 + 2;

  if ( 255 != pchr->light_fp8 )
  {
    ftmp = FP8_TO_FLOAT( pchr->light_fp8 );
    pldata->emission.r = ftmp / ( float )( 1 << r_sft );
    pldata->emission.g = ftmp / ( float )( 1 << g_sft );
    pldata->emission.b = ftmp / ( float )( 1 << b_sft );
  }
}

//--------------------------------------------------------------------------------------------
void render_mad_lit( CHR_REF ichr )
{
  // ZZ> This function draws an environment mapped model

  CChr * pchr;
  Uint16 texture;
  GLfloat mat_none[4] = {0,0,0,0};

  CGame * gs = gfxState.gs;

  if( !ACTIVE_CHR(gs->ChrList, ichr) ) return;
  pchr = gs->ChrList + ichr;

  calc_lighting_data(gs, pchr);

  glMaterialfv( GL_FRONT_AND_BACK, GL_SPECULAR,  pchr->ldata.specular.v );
  glMaterialfv( GL_FRONT_AND_BACK, GL_SHININESS, pchr->ldata.shininess );
  glMaterialfv( GL_FRONT_AND_BACK, GL_AMBIENT,   mat_none );
  glMaterialfv( GL_FRONT_AND_BACK, GL_DIFFUSE,   mat_none );
  glMaterialfv( GL_FRONT_AND_BACK, GL_EMISSION,  pchr->ldata.emission.v );

  ATTRIB_PUSH( "render_mad_lit", GL_ENABLE_BIT | GL_TRANSFORM_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_CURRENT_BIT );
  {
    glMatrixMode( GL_MODELVIEW );
    glPushMatrix();
    pchr->matrix.CNV( 3, 2 ) += RAISE;
    glMultMatrixf( pchr->matrix.v );
    pchr->matrix.CNV( 3, 2 ) -= RAISE;

    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LEQUAL );

    if (SDLKEYDOWN(SDLK_F7))
    {
      GLTexture_Bind(NULL, &gfxState);
    }
    else
    {
      texture = pchr->skin_ref + gs->ObjList[pchr->model].skinstart;
      GLTexture_Bind( gs->TxTexture + texture, &gfxState );
    }

    draw_textured_md2_opengl(ichr);

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
  }
  ATTRIB_POP( "render_mad_lit" );

#if defined(DEBUG_BBOX) && defined(_DEBUG)
  if(CData.DevMode)
  {
    chr_draw_BBox(ichr);
  }
#endif

}

//--------------------------------------------------------------------------------------------
void render_texmad(CHR_REF ichr, Uint8 trans)
{
  CChr * pchr;
  Uint16 texture;
  CGame * gs = gfxState.gs;

  if(!ACTIVE_CHR(gs->ChrList, ichr)) return;
  pchr = gs->ChrList + ichr;

  texture = pchr->skin_ref + gs->ObjList[pchr->model].skinstart;

  md2_blend_vertices(pchr, -1, -1);
  md2_blend_lighting(pchr);

  ATTRIB_PUSH( "render_texmad", GL_ENABLE_BIT | GL_TRANSFORM_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_CURRENT_BIT );
  {
    glMatrixMode( GL_MODELVIEW );
    glPushMatrix();
    pchr->matrix.CNV( 3, 2 ) += RAISE;
    glMultMatrixf( pchr->matrix.v );
    pchr->matrix.CNV( 3, 2 ) -= RAISE;

    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LEQUAL );

    // Choose texture and matrix
    if (SDLKEYDOWN(SDLK_F7))
    {
      GLTexture_Bind(NULL, &gfxState);
    }
    else
    {
      GLTexture_Bind( gs->TxTexture + texture, &gfxState );
    }

    draw_textured_md2_opengl(ichr);

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
  }
  ATTRIB_POP( "render_texmad" );

#if defined(DEBUG_BBOX) && defined(_DEBUG)
  if(CData.DevMode)
  {
    chr_draw_BBox(ichr);
  }
#endif
}


//--------------------------------------------------------------------------------------------
void render_enviromad(CHR_REF ichr, Uint8 trans)
{
  Uint16 texture;
  CGame * gs = gfxState.gs;

  if(!ACTIVE_CHR(gs->ChrList, ichr)) return;

  texture = gs->ChrList[ichr].skin_ref + gs->ObjList[gs->ChrList[ichr].model].skinstart;

  ATTRIB_PUSH( "render_enviromad", GL_TRANSFORM_BIT | GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_CURRENT_BIT | GL_TEXTURE_BIT );
  {
    glMatrixMode( GL_MODELVIEW );
    glPushMatrix();
    gs->ChrList[ichr].matrix.CNV( 3, 2 ) += RAISE;
    glMultMatrixf( gs->ChrList[ichr].matrix.v );
    gs->ChrList[ichr].matrix.CNV( 3, 2 ) -= RAISE;

    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LEQUAL );

    // Choose texture and matrix
    if ( SDLKEYDOWN(SDLK_F7) )
    {
      GLTexture_Bind(NULL, &gfxState);
    }
    else
    {
      GLTexture_Bind( gs->TxTexture + texture, &gfxState );
    }

    glEnable( GL_TEXTURE_GEN_S );     // Enable Texture Coord Generation For S (NEW)
    glEnable( GL_TEXTURE_GEN_T );     // Enable Texture Coord Generation For T (NEW)

    draw_enviromapped_md2_opengl(ichr);

    glDisable( GL_TEXTURE_GEN_S );     // Enable Texture Coord Generation For S (NEW)
    glDisable( GL_TEXTURE_GEN_T );     // Enable Texture Coord Generation For T (NEW)


    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
  }
  ATTRIB_POP( "render_enviromad" );
}

//--------------------------------------------------------------------------------------------
void render_mad( CHR_REF ichr, Uint8 trans )
{
  // ZZ> This function picks the actual function to use

  CGame * gs   = gfxState.gs;
  CChr  * pchr = ChrList_getPChr(gs, ichr);
  CCap  * pcap = ChrList_getPCap(gs, ichr);
  Sint8   hide = pcap->hidestate;

  // 100% transparent items are not drawn
  if(trans == 0) return;

  // items that are in packs are not drawn
  if(INVALID_CHR != pchr->inwhichpack) return;

  // hidden items are not drawn
  if(hide != NOHIDE && hide == pchr->aistate.state) return; 

  // dispatch the drawing to the correct routine
  if ( pchr->enviro )
    render_enviromad( ichr, trans );
  else
    render_texmad( ichr, trans );

}

//--------------------------------------------------------------------------------------------
void render_refmad( CHR_REF ichr, Uint16 trans_fp8 )
{
  // ZZ> This function draws characters reflected in the floor

  CGame * gs   = gfxState.gs;
  CChr  * pchr = ChrList_getPChr(gs, ichr);
  CCap  * pcap = ChrList_getPCap(gs, ichr);

  int alphatmp_fp8;
  float level = pchr->level;
  float zpos = ( pchr->matrix ).CNV( 3, 2 ) - level;

  Uint16 lastframe = pchr->anim.last;
  Uint8 sheensave;
  bool_t fog_save;

  if ( !pcap->prop.reflect ) return;

  alphatmp_fp8 = trans_fp8 - zpos * 0.5f;
  if ( alphatmp_fp8 <= 0 )   return;
  if ( alphatmp_fp8 > 255 ) alphatmp_fp8 = 255;

  sheensave = pchr->sheen_fp8;
  pchr->redshift += 1;
  pchr->grnshift += 1;
  pchr->blushift += 1;
  pchr->sheen_fp8 >>= 1;
  ( pchr->matrix ).CNV( 0, 2 ) = - ( pchr->matrix ).CNV( 0, 2 );
  ( pchr->matrix ).CNV( 1, 2 ) = - ( pchr->matrix ).CNV( 1, 2 );
  ( pchr->matrix ).CNV( 2, 2 ) = - ( pchr->matrix ).CNV( 2, 2 );
  ( pchr->matrix ).CNV( 3, 2 ) = - ( pchr->matrix ).CNV( 3, 2 ) + level + level;
  fog_save = GFog.on;
  GFog.on  = bfalse;

  render_mad( ichr, alphatmp_fp8 );

  GFog.on = fog_save;
  ( pchr->matrix ).CNV( 0, 2 ) = - ( pchr->matrix ).CNV( 0, 2 );
  ( pchr->matrix ).CNV( 1, 2 ) = - ( pchr->matrix ).CNV( 1, 2 );
  ( pchr->matrix ).CNV( 2, 2 ) = - ( pchr->matrix ).CNV( 2, 2 );
  ( pchr->matrix ).CNV( 3, 2 ) = - ( pchr->matrix ).CNV( 3, 2 ) + level + level;
  pchr->sheen_fp8 = sheensave;
  pchr->redshift -= 1;
  pchr->grnshift -= 1;
  pchr->blushift -= 1;

}

//---------------------------------------------------------------------------------------------
float spek_global_lighting( int rotation, int normal, vect3 lite )
{
  // ZZ> This function helps make_spektable()

  float fTmp, flite;
  vect3 nrm;
  float sinrot, cosrot;

  nrm.x = -kMd2Normals[normal][0];
  nrm.y = kMd2Normals[normal][1];
  nrm.z = kMd2Normals[normal][2];

  sinrot = sinlut[rotation];
  cosrot = coslut[rotation];
  fTmp   = cosrot * nrm.x + sinrot * nrm.y;
  nrm.y = cosrot * nrm.y - sinrot * nrm.x;
  nrm.x = fTmp;

  fTmp = DotProduct( nrm, lite );
  flite = 0.0f;
  if ( fTmp > 0 ) flite = fTmp * fTmp;

  return flite;
}

//---------------------------------------------------------------------------------------------
float spek_local_lighting( int rotation, int normal )
{
  // ZZ> This function helps make_spektable()


  float fTmp, fLite;
  vect3 nrm;
  float sinrot, cosrot;

  nrm.x = -kMd2Normals[normal][0];
  nrm.y =  kMd2Normals[normal][1];

  sinrot = sinlut[rotation];
  cosrot = coslut[rotation];

  fTmp = cosrot * nrm.x + sinrot * nrm.y;

  fLite = 0.0f;
  if ( fTmp > 0.0f )  fLite = fTmp * fTmp;

  return fLite;
}

//---------------------------------------------------------------------------------------------
float spek_calc_local_lighting( Uint16 turn, vect3 nrm )
{
  // ZZ> This function helps make_spektable
  float fTmp1, fTmp2, fLite;
  Uint16 turn_sin;
  float sinrot, cosrot;

  turn_sin = turn >> 2;

  sinrot = turntosin[turn_sin];
  cosrot = turntocos[turn_sin];

  fTmp1 = cosrot * nrm.x + sinrot * nrm.y;
  fTmp2 = nrm.x * nrm.x + nrm.y * nrm.y;


  if(fTmp2 == 0.0f)
  {
    fLite = 1.0f;
  }
  else if ( fTmp1 > 0.0f )
  {
    fLite = fTmp1 * fTmp1 / fTmp2;
  }
  else
  {
    fLite = 0.0f;
  }

  return fLite;
}


//---------------------------------------------------------------------------------------------
void make_speklut()
{
  // ZZ > Build a lookup table for sin/cos

  int cnt;

  for ( cnt = 0; cnt < MAXLIGHTROTATION; cnt++ )
  {
    sinlut[cnt] = sin( TWO_PI * cnt / MAXLIGHTROTATION );
    coslut[cnt] = cos( TWO_PI * cnt / MAXLIGHTROTATION );
  }
};

//---------------------------------------------------------------------------------------------
void make_spektable( vect3 lite )
{
  // ZZ> This function makes a light table to fake directional lighting

  int cnt, tnc;
  float flight;
  vect3 loc_lite = lite;

  flight = loc_lite.x * loc_lite.x + loc_lite.y * loc_lite.y + loc_lite.z * loc_lite.z;
  if ( flight > 0 )
  {
    flight = sqrt( flight );
    loc_lite.x /= flight;
    loc_lite.y /= flight;
    loc_lite.z /= flight;
    for ( cnt = 0; cnt < MD2LIGHTINDICES - 1; cnt++ )  // Spikey mace
    {
      for ( tnc = 0; tnc < MAXLIGHTROTATION; tnc++ )
      {
        spek_global[tnc][cnt] = spek_global_lighting( tnc, cnt, loc_lite );
        spek_local[tnc][cnt]  = spek_local_lighting( tnc, cnt );
      }
    }
  }
  else
  {
    for ( cnt = 0; cnt < MD2LIGHTINDICES - 1; cnt++ )  // Spikey mace
    {
      for ( tnc = 0; tnc < MAXLIGHTROTATION; tnc++ )
      {
        spek_global[tnc][cnt] = 0;
        spek_local[tnc][cnt]  = spek_local_lighting( tnc, cnt );
      }
    }
  }

  // Fill in index number 162 for the spike mace
  for ( tnc = 0; tnc < MAXLIGHTROTATION; tnc++ )
  {
    spek_global[tnc][MD2LIGHTINDICES-1] = 0;
    spek_local[tnc][cnt]                = 0;
  }
}

//---------------------------------------------------------------------------------------------
void make_lighttospek( void )
{
  // ZZ> This function makes a light table to fake directional lighting

  int cnt, tnc;
  //  Uint8 spek;
  //  float fTmp, fPow;


  // New routine
  for ( cnt = 0; cnt < MAXSPEKLEVEL; cnt++ )
  {
    for ( tnc = 0; tnc < 256; tnc++ )
    {
      lighttospek[cnt][tnc] = FLOAT_TO_FP8( pow( FP8_TO_FLOAT( tnc ), 1.0 + cnt / 2.0f ) );
      lighttospek[cnt][tnc] = MIN( 255, lighttospek[cnt][tnc] );
    }
  }
}


//--------------------------------------------------------------------------------------------
void chr_draw_BBox(CHR_REF ichr)
{
  CGame * gs  = gfxState.gs;

  CChr * pchr;
  BData * bd;

  if( !ACTIVE_CHR(gs->ChrList, ichr) ) return;
  pchr = gs->ChrList + ichr;
  bd = &(pchr->bmpdata);

  // make sure that SOME bumper is calculated for this object
  chr_calculate_bumpers(gs, pchr, 0);

  if(bd->cv.lod >= 3)
  {
    int i;

    for(i=0; i<8; i++)
    {
      CVolume_draw( bd->cv_tree->leaf + i, btrue, bfalse );
    }
  }
  else
  {
    CVolume_draw( &(bd->cv), bd->calc_size > 0, bd->calc_size_big > 0 );
  }

};



