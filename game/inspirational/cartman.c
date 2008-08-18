#include "cartman.h"

#include "../Ui.h"
#include "../Log.h"
#include "../mesh.inl"

#include "../egoboo.h"

#include <stdio.h>      // For printf and such
#include "../SDL_Pixel.h"
#include <SDL_image.h>

#include "../egoboo_types.inl"
#include "../graphic.inl"
#include "../input.inl"

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
  const Uint32 rmask = 0x000000ff;
  const Uint32 gmask = 0x0000ff00;
  const Uint32 bmask = 0x00ff0000;
  const Uint32 amask = 0xff000000;
#else
  const Uint32 rmask = 0xff000000;
  const Uint32 gmask = 0x00ff0000;
  const Uint32 bmask = 0x0000ff00;
  const Uint32 amask = 0x000000ff;
#endif


typedef enum e_cart_win_type_bits
{
  WINNOTHING = 0,             // Window display mode
  WINTILE    = 1 << 0,      //
  WINVERTEX  = 1 << 1,      //
  WINSIDE    = 1 << 2,      //
  WINFX      = 1 << 3        //
};

#define MAXSELECT 2560      // Max points that can be selected

#define NEARLOW         0.0 //16.0    // For autoweld
#define NEARHI        128.0 //112.0    //

#define TINYX 4 //8        // Plan tiles
#define TINYY 4 //8        //
#define SMALLX 31      // Small tiles
#define SMALLY 31      //
#define BIGX 63        // Big tiles
#define BIGY 63        //

#define CHAINEND 0xffffffff    // End of vertex chain
#define VERTEXUNUSED 0         // Check meshvrta to see if used

int   numwritten = 0;
int   numattempt = 0;

struct cart_light_list_t
{
  int   adding;
  int   count;
  int   x[MAXLIGHT];
  int   y[MAXLIGHT];
  Uint8 level[MAXLIGHT];
  int   radius[MAXLIGHT];
};
typedef struct cart_light_list_t cart_light_list;

cart_light_list clight_lst = {0, 0};

int   ambi = 22;
int   ambicut = 1;
int   direct = 16;

int cart_pos_x = 0;
int cart_pos_y = 0;

char    loadname[80];    // Text
int    SCRX = DEFAULT_SCREEN_W;    // Screen size
int    SCRY = DEFAULT_SCREEN_H;    //
int    OUTX = DEFAULT_SCREEN_W;    // Output size <= Screen size
int    OUTY = DEFAULT_SCREEN_H;    //


int    brushsize = 3;    // Size of raise/lower terrain brush
int    brushamount = 50;  // Amount of raise/lower

struct cart_image_list_t
{
  GLtexture cursor;       // Cursor image
  GLtexture point[16];    // Vertex image
  GLtexture pointon[16];  // Vertex image ( Selected )
  GLtexture ref;          // Meshfx images
  GLtexture drawref;      //
  GLtexture anim;         //
  GLtexture water;        //
  GLtexture wall;         //
  GLtexture impass;       //
  GLtexture damage;       //
  GLtexture slippy;       //
};
typedef struct cart_image_list_t cart_image_list;
cart_image_list cimg_lst;

struct cart_bmp_list_t
{
  SDL_Surface * hitemap;    // Heightmap image
  GLtexture     temp;    // A temporary bitmap
  GLtexture     dbuff;    // The double buffer bitmap
  GLtexture     smalltile[MAXTILE];  // Tiles
  GLtexture     bigtile[MAXTILE];  //
  GLtexture     tinysmalltile[MAXTILE];  // Plan tiles
  GLtexture     tinybigtile[MAXTILE];  //
  GLtexture     fanoff;    //
};
typedef struct cart_bmp_list_t cart_bmp_list;
cart_bmp_list cbmp_lst = {NULL};

int numsmalltile = 0;  //
int numbigtile = 0;    //

struct cart_points_t
{
  int     count;
  Uint32  data[MAXPOINTS];
};
typedef struct cart_points_t cart_points;

cart_points cpoint_lst = {0};

struct cart_select_list_t
{
  int     count;
  Uint32  data[MAXPOINTS];
};
typedef struct cart_select_list_t cart_select_list;
cart_select_list cselect_lst = {0};


float    debugx = -1;    // Blargh
float    debugy = -1;    //

#define MAXWIN 8                 // Number of windows
struct cart_window_info_t
{
  GLtexture tx;     // Window images
  bool_t    on;       // Draw it?
  int       borderx; // Window border size
  int       bordery; //
  SDL_Rect  rect;    // Window size
  Uint16    mode;     // Window display mode
};
typedef struct cart_window_info_t cart_window_info;

cart_window_info cwindow_info[MAXWIN];

struct cart_mouse_info_t
{
  int                data;    // More mouse data
  cart_window_info * w;

  int      x;       //
  int      y;       //
  Uint16   mode;    // Window mode
  Uint32   onfan;   // Fan mouse is on
  Uint16   tile;    // Tile
  Uint16   presser; // Random add for tiles
  Uint8    type;    // Fan type
  int      rect;    // Rectangle drawing
  int      rectx;   //
  int      recty;   //
  Uint8    fx;      //
};
typedef struct cart_mouse_info_t cart_mouse_info;

cart_mouse_info cmouse_info =
{
  -1,             // data
  -1,             // x
  -1,             // y
  0,              // mode
  0,              // onfan
  0,              // tile
  0,              // presser
  0,              // type
  0,              // rect
  0,              // rectx
  0,              // recty
  MPDFX_NOREFLECT // fx
};

int    colordepth = 8;    // 256 colors
int    keydelay = 0;    //


struct cart_fan_lines_t
{
  Uint32 numline;  // Number of lines to draw
  Uint8  linestart[MAXMESHLINE];
  Uint8  lineend[MAXMESHLINE];
};
typedef struct cart_fan_lines_t cart_fan_lines;
cart_fan_lines cfan_lines[MAXMESHTYPE];

struct cart_vert_extra_t
{
  Uint32 free_count;    // Number of free vertices
  Uint32 index;        // Current vertex check for new

  Uint32 next[MAXTOTALMESHVERTICES]; // Next vertex in fan
};
typedef struct cart_vert_extra_t cart_vert_extra;

struct cart_mesh_t
{
  MeshInfo_t * mi;
  MeshMem_t   * mm;
  cart_vert_extra xvrt;

  float  edgez;      //
  Uint32 seed;
};
typedef struct cart_mesh_t cart_mesh;
cart_mesh c_mesh = {NULL, NULL, {0, 0} };

//------------------------------------------------------------------------------
GLfloat * make_color(Uint8 r, Uint8 g, Uint8 b)
{
  static GLfloat color[4];

  color[0] = (r << 3) / 256.0f;
  color[1] = (g << 3) / 256.0f;
  color[2] = (b << 3) / 256.0f;
  color[3] = 1.0f;

  return color;
}

//------------------------------------------------------------------------------
void set_window_viewport( cart_window_info * w )
{
  if( NULL == w )
  {
    glViewport(0, 0, gfxState.scrx, gfxState.scry);
  }
  else
  {
    glViewport(w->rect.x, w->rect.y, w->rect.w, w->rect.h);
  };
};


//------------------------------------------------------------------------------
void draw_rect(cart_window_info * w, GLfloat color[], float xl, float yt, float xr, float yb )
{
  set_window_viewport( w );

  GLtexture_Bind(NULL, &gfxState);

  if(NULL != color)
  {
    glColor3fv( color );
  }

  glBegin(GL_QUADS);
  {
    glVertex2f( xl, yb );
    glVertex2f( xr, yb );
    glVertex2f( xr, yt );
    glVertex2f( xl, yt );
  }
  glEnd();
};


void draw_line(cart_window_info * w, GLfloat color[], float x1, float y1, float x2, float y2 )
{
  set_window_viewport( w );

  if( NULL != color )
  {
    glColor3fv( color );
  };

  glBegin(GL_LINES);
  {
    glVertex2f(x1, y1);
    glVertex2f(x2, y2);
  }
  glEnd();
}


//------------------------------------------------------------------------------
void clear_to_color( cart_window_info * w, GLfloat color[] )
{
  SDL_Rect dst;

  if( NULL == w )
  {
    dst.x = dst.y = 0;
    dst.w = gfxState.scrx;
    dst.h = gfxState.scry;
  }
  else
  {
    dst = w->rect;
  }

  draw_rect( w, color, dst.x, dst.y, dst.x + dst.w, dst.y + dst.h );

}



//------------------------------------------------------------------------------
void draw_blit_sprite(cart_window_info * w, GLtexture * sprite, int x, int y)
{
  FRect_t dst;

  set_window_viewport( w );

  dst.left   = x;
  dst.right  = x + sprite->txW;
  dst.top    = y;
  dst.bottom = y + sprite->txH;

  draw_texture_box(sprite, NULL, &dst);
};

//------------------------------------------------------------------------------
void add_light(cart_light_list * plst, int x, int y, int radius, int level)
{
  if(NULL == plst) return;

  if(plst->count >= MAXLIGHT)  clight_lst.count = MAXLIGHT-1;

  plst->x[plst->count] = x;
  plst->y[plst->count] = y;
  plst->radius[plst->count] = radius;
  plst->level[plst->count] = level;
  plst->count++;
}

//------------------------------------------------------------------------------
void alter_light(cart_light_list * plst, int x, int y)
{
  int radius, level;

  if(NULL == plst) return;

  plst->count--;
  if(plst->count < 0)  plst->count = 0;

  radius = abs(plst->x[plst->count] - x);
  level = abs(plst->y[plst->count] - y);
  if(radius > MAXRADIUS)  radius = MAXRADIUS;
  if(radius < MINRADIUS)  radius = MINRADIUS;
  plst->radius[plst->count] = radius;
  if(level > MAXHEIGHT) level = MAXHEIGHT;
  if(level < MINLEVEL) level = MINLEVEL;
  plst->level[plst->count] = level;

  plst->count++;
}

//------------------------------------------------------------------------------
void draw_circle( cart_window_info * w, float x, float y, float radius, GLfloat color[] )
{
  GLUquadricObj * qobj = gluNewQuadric();

  glMatrixMode( GL_MODELVIEW );
  glPushMatrix();

    if(NULL != color)
    {
      glColor4fv( color );
    };

    gluQuadricDrawStyle(qobj, GLU_FILL); /* flat shaded */
    gluQuadricNormals(qobj, GLU_FLAT);

    glTranslatef(x,y,0);

    gluDisk(qobj, 0, 1.0, 20, 4);

  glPopMatrix();
};

//------------------------------------------------------------------------------
void draw_light(cart_window_info * w, cart_light_list * plst, int number)
{
  int xdraw, ydraw, radius;
  int icolor;

  if( NULL == plst ) return;

  xdraw = (plst->x[number]/FOURNUM) - cart_pos_x + (w->rect.w>>1) - SMALLX;
  ydraw = (plst->y[number]/FOURNUM) - cart_pos_y + (w->rect.h>>1) - SMALLY;
  radius = abs(plst->radius[number])/FOURNUM;
  icolor = plst->level[number]>>3;

  draw_circle( w, xdraw, ydraw, radius, make_color(icolor, icolor, icolor) );
}

//------------------------------------------------------------------------------
int dist_from_border(cart_mesh * cmsh, int x, int y)
{
  if(x > cmsh->mi->edge_x / 2.0f) x = cmsh->mi->edge_x-x-1;
  if(x < 0) x = 0;

  if(y > cmsh->mi->edge_y / 2.0f) y = cmsh->mi->edge_y-y-1;
  if(y < 0) y = 0;

  return ( (x < y) ? x : y );
}

//------------------------------------------------------------------------------
int dist_from_edge(cart_mesh * cmsh, int x, int y)
{
  if(x > (cmsh->mi->tiles_x>>1)) x = cmsh->mi->tiles_x-x-1;
  if(y > (cmsh->mi->tiles_y>>1)) y = cmsh->mi->tiles_y-y-1;

  return ( (x < y) ? x : y );
}

//------------------------------------------------------------------------------
bool_t fan_is_floor(cart_mesh * cmsh, float x, float y)
{
  Uint32 fan;
  vect3 pos = {x,y,0};

  fan = mesh_get_fan( cmsh->mi, cmsh->mm, pos );
  if( INVALID_FAN != fan )
  {
    return mesh_has_no_bits(cmsh->mm->tilelst, fan, MPDFX_WALL | MPDFX_IMPASS);
  }

  return bfalse;
}

//------------------------------------------------------------------------------
void set_barrier_height(cart_mesh * cmsh, cart_mouse_info * m, int x, int y)
{
  Uint32 type, fan, vert;
  int cnt, noedges;
  float bestprox, prox, tprox, scale;
  vect3 pos = {x,y,0};

  fan = mesh_get_fan( cmsh->mi, cmsh->mm, pos );
  if(fan != -1)
  {
    if( mesh_has_some_bits( cmsh->mm->tilelst, fan, MPDFX_WALL ) )
    {
      type = cmsh->mm->tilelst[fan].type;
      noedges = btrue;
      vert = cmsh->mm->tilelst[fan].vrt_start;
      cnt = 0;
      while(cnt < pmesh->TileDict[type].vrt_count)
      {
        bestprox = 2*(NEARHI-NEARLOW)/3.0;
        if(fan_is_floor(cmsh, x+1, y))
        {
          prox = NEARHI-pmesh->TileDict[type].tx[cnt].u;
          if(prox < bestprox) bestprox = prox;
          noedges = bfalse;
        }
        if(fan_is_floor(cmsh, x, y+1))
        {
          prox = NEARHI-pmesh->TileDict[type].tx[cnt].v;
          if(prox < bestprox) bestprox = prox;
          noedges = bfalse;
        }
        if(fan_is_floor(cmsh, x-1, y))
        {
          prox = pmesh->TileDict[type].tx[cnt].u-NEARLOW;
          if(prox < bestprox) bestprox = prox;
          noedges = bfalse;
        }
        if(fan_is_floor(cmsh, x, y-1))
        {
          prox = pmesh->TileDict[type].tx[cnt].v-NEARLOW;
          if(prox < bestprox) bestprox = prox;
          noedges = bfalse;
        }
        if(noedges)
        {
          // Surrounded by walls on all 4 sides, but it may be a corner piece
          if(fan_is_floor(cmsh, x+1, y+1))
          {
            prox = NEARHI-pmesh->TileDict[type].tx[cnt].u;
            tprox = NEARHI-pmesh->TileDict[type].tx[cnt].v;
            if(tprox > prox) prox = tprox;
            if(prox < bestprox) bestprox = prox;
          }
          if(fan_is_floor(cmsh, x+1, y-1))
          {
            prox = NEARHI-pmesh->TileDict[type].tx[cnt].u;
            tprox = pmesh->TileDict[type].tx[cnt].v-NEARLOW;
            if(tprox > prox) prox = tprox;
            if(prox < bestprox) bestprox = prox;
          }
          if(fan_is_floor(cmsh, x-1, y+1))
          {
            prox = pmesh->TileDict[type].tx[cnt].u-NEARLOW;
            tprox = NEARHI-pmesh->TileDict[type].tx[cnt].v;
            if(tprox > prox) prox = tprox;
            if(prox < bestprox) bestprox = prox;
          }
          if(fan_is_floor(cmsh, x-1, y-1))
          {
            prox = pmesh->TileDict[type].tx[cnt].u-NEARLOW;
            tprox = pmesh->TileDict[type].tx[cnt].v-NEARLOW;
            if(tprox > prox) prox = tprox;
            if(prox < bestprox) bestprox = prox;
          }
        }
        scale = cwindow_info[m->data].rect.h-(m->y/FOURNUM);
        bestprox = bestprox*scale*BARRIERHEIGHT/cwindow_info[m->data].rect.h;
        if(bestprox > cmsh->edgez) bestprox = cmsh->edgez;
        if(bestprox < 0) bestprox = 0;
        cmsh->mm->vrt_z[vert] = bestprox;
        vert = cmsh->xvrt.next[vert];
        cnt++;
      }
    }
  }
}

//------------------------------------------------------------------------------
void fix_walls(cart_mesh * cmsh, cart_mouse_info * m)
{
  int x, y;

  y = 0;
  while(y < cmsh->mi->tiles_y)
  {
    x = 0;
    while(x < cmsh->mi->tiles_x)
    {
      set_barrier_height(cmsh, m, x, y);
      x++;
    }
    y++;
  }
}

//------------------------------------------------------------------------------
void impass_edges(cart_mesh * cmsh, int amount)
{
  int x, y;
  Uint32 fan;

  y = 0;
  while(y < cmsh->mi->tiles_y)
  {
    x = 0;
    while(x < cmsh->mi->tiles_x)
    {
      if(dist_from_edge(cmsh, x, y) < amount)
      {
        fan = mesh_convert_fan(cmsh->mi, x, y);
        cmsh->mm->tilelst[fan].fx |= MPDFX_IMPASS;
      }
      x++;
    }
    y++;
  }
}

//------------------------------------------------------------------------------
Uint8 get_twist(int x, int y)
{
  Uint8 twist;


  // x and y should be from -7 to 8
  if(x < -7) x = -7;
  if(x > 8) x = 8;
  if(y < -7) y = -7;
  if(y > 8) y = 8;
  // Now between 0 and 15
  x = x+7;
  y = y+7;
  twist = (y<<4)+x;
  return twist;
}

//------------------------------------------------------------------------------
Uint8 get_fan_twist(cart_mesh * cmsh, Uint32 fan)
{
  int zx, zy, vt0, vt1, vt2, vt3;
  Uint8 twist;

  vt0 = cmsh->mm->tilelst[fan].vrt_start;
  vt1 = cmsh->xvrt.next[vt0];
  vt2 = cmsh->xvrt.next[vt1];
  vt3 = cmsh->xvrt.next[vt2];
  zx = (cmsh->mm->vrt_z[vt0]+cmsh->mm->vrt_z[vt3]-cmsh->mm->vrt_z[vt1]-cmsh->mm->vrt_z[vt2])/SLOPE;
  zy = (cmsh->mm->vrt_z[vt2]+cmsh->mm->vrt_z[vt3]-cmsh->mm->vrt_z[vt0]-cmsh->mm->vrt_z[vt1])/SLOPE;
  twist = get_twist(zx, zy);


  return twist;
}

//------------------------------------------------------------------------------
//void make_graypal(void)
//{
//  int cnt, level;
//
//
//  cnt = 0;
//  while(cnt < 253)
//  {
//    level = cnt+2; if(level > 255) level = 255;
//    badpal[cnt].r = level>>2;
//    level = cnt+1; if(level > 255) level = 255;
//    badpal[cnt].g = level>>2;
//    badpal[cnt].b = cnt>>2;
//    cnt++;
//  }
//  badpal[253].r = 63;  badpal[253].g = 0;  badpal[253].b = 0;
//  badpal[254].r = 0;  badpal[254].g = 0;  badpal[254].b = 63;
//  badpal[255].r = 63;  badpal[255].g = 0;  badpal[255].b = 63;
//  return;
//}

//------------------------------------------------------------------------------
void make_hitemap(cart_mesh * cmsh)
{
  int x, y, pixx, pixy, level, fan;

  if(NULL != cbmp_lst.hitemap) SDL_FreeSurface(cbmp_lst.hitemap);
  cbmp_lst.hitemap = SDL_CreateRGBSurface(SDL_SWSURFACE, cmsh->mi->tiles_x<<2, cmsh->mi->tiles_y<<2, 32, rmask, gmask, bmask, amask);
  if(NULL == cbmp_lst.hitemap) return;

  y = 16;
  pixy = 0;
  while(pixy < (cmsh->mi->tiles_y<<2))
  {
    x = 16;
    pixx = 0;
    while(pixx < (cmsh->mi->tiles_x<<2))
    {
      vect3 pos = {x,y,0};

      level=(mesh_get_level(cmsh->mm, mesh_get_fan(cmsh->mi, cmsh->mm, pos), x, y, bfalse, NULL)*255/cmsh->edgez);  // level is 0 to 255
      if(level > 252) level = 252;
      fan = mesh_convert_fan(cmsh->mi, pixx>>2, pixy>>2);

      if( mesh_has_all_bits(cmsh->mm->tilelst, fan, MPDFX_WALL | MPDFX_IMPASS) ) level = 255;   // Both
      else if( mesh_has_all_bits(cmsh->mm->tilelst, fan, MPDFX_WALL)   ) level = 253;         // Wall
      else if( mesh_has_all_bits(cmsh->mm->tilelst, fan, MPDFX_IMPASS) ) level = 254;         // Impass

      SDL_PutPixel(cbmp_lst.hitemap, pixx, pixy, level);

      x+=32;
      pixx++;
    }
    y+=32;
    pixy++;
  }
  return;
}

//------------------------------------------------------------------------------
GLtexture * tiny_tile_at(cart_mesh * cmsh, int x, int y)
{
  GLtexture * retval = NULL;
  Uint16 tile, basetile;
  Uint8 type, fx;
  Uint32 fan;

  if(x < 0 || x >= cmsh->mi->tiles_x || y < 0 || y >= cmsh->mi->tiles_y)
  {
    return retval;
  }

  fan = mesh_convert_fan(cmsh->mi, x, y);
  if(INVALID_FAN == cmsh->mm->tilelst[fan].tile)
  {
    return NULL;
  }

  tile = cmsh->mm->tilelst[fan].tile;
  type = cmsh->mm->tilelst[fan].type;
  fx = cmsh->mm->tilelst[fan].fx;

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


  if(type >= (MAXMESHTYPE>>1))
  {
    retval = cbmp_lst.tinybigtile + tile;
  }
  else
  {
    retval = cbmp_lst.tinysmalltile + tile;
  }

  return retval;
}

//------------------------------------------------------------------------------
void make_planmap(cart_mesh * cmsh)
{
  int x, y, putx, puty;
  SDL_Surface* bmp_temp;


  bmp_temp = SDL_CreateRGBSurface(SDL_SWSURFACE, 64, 64, 32, rmask, gmask, bmask, amask);
  if(!bmp_temp)  return;

  if(NULL != cbmp_lst.hitemap) SDL_FreeSurface(cbmp_lst.hitemap);
  cbmp_lst.hitemap = SDL_CreateRGBSurface(SDL_SWSURFACE, cmsh->mi->tiles_x*TINYX, cmsh->mi->tiles_y*TINYY, 32, rmask, gmask, bmask, amask);
  if(!cbmp_lst.hitemap) return;


  do_clear();
  puty = 0;
  y = 0;
  while(y < cmsh->mi->tiles_y)
  {
    putx = 0;
    x = 0;
    while(x < cmsh->mi->tiles_x)
    {
      FRect_t dst = {putx, puty, putx + TINYX, puty + TINYY};
      GLtexture * tmp = tiny_tile_at(cmsh, x, y);
      draw_texture_box(tmp, NULL, &dst);

      putx+=TINYX;
      x++;
    }
    puty+=TINYY;
    y++;
  }


  SDL_BlitSurface(cbmp_lst.hitemap, &cbmp_lst.hitemap->clip_rect, bmp_temp, &bmp_temp->clip_rect);

  SDL_FreeSurface(cbmp_lst.hitemap);

  cbmp_lst.hitemap = bmp_temp;
}

//------------------------------------------------------------------------------
void draw_cursor_in_window(cart_mouse_info * m, cart_window_info * w)
{
  int x, y;

  if(m->data!=-1)
  {
    if(w->on && w != m->w)
    {
      if((cwindow_info[m->data].mode&WINSIDE) == (w->mode&WINSIDE))
      {
        x = ui_getMouseX()-cwindow_info[m->data].rect.x-cwindow_info[m->data].borderx;
        y = ui_getMouseY()-cwindow_info[m->data].rect.y-cwindow_info[m->data].bordery;
        draw_blit_sprite(w, cimg_lst.pointon + 10, x-4, y-4);
      }
    }
  }
  return;
}

//------------------------------------------------------------------------------
int get_vertex(cart_mesh * cmsh, int x, int y, int num)
{
  // ZZ> This function gets a vertex number or -1

  int vert, cnt;
  Uint32 fan;

  vert = -1;
  if(x>=0 && y>=0 && x<cmsh->mi->tiles_x && y<cmsh->mi->tiles_y)
  {
    fan = mesh_convert_fan(cmsh->mi, x, y);
    if(pmesh->TileDict[cmsh->mm->tilelst[fan].type].vrt_count>num)
    {
      vert = cmsh->mm->tilelst[fan].vrt_start;
      cnt = 0;
      while(cnt < num)
      {
        vert = cmsh->xvrt.next[vert];
        if(vert==-1)
        {
          log_error("BAD GET_VERTEX NUMBER(2nd), %d at %d, %d...\n" "%d VERTICES ALLOWED...\n\n", num, x, y , pmesh->TileDict[cmsh->mm->tilelst[fan].type].vrt_count);
        }
        cnt++;
      }
    }
  }
  return vert;
}

//------------------------------------------------------------------------------
int nearest_vertex(cart_mesh * cmsh, int x, int y, float nearx, float neary)
{
  // ZZ> This function gets a vertex number or -1

  int vert, bestvert, cnt;
  Uint32 fan;
  int num;
  float prox, proxx, proxy, bestprox;


  bestvert = -1;
  if(x>=0 && y>=0 && x<cmsh->mi->tiles_x && y<cmsh->mi->tiles_y)
  {
    fan = mesh_convert_fan(cmsh->mi, x, y);
    num = pmesh->TileDict[cmsh->mm->tilelst[fan].type].vrt_count;
    vert = cmsh->mm->tilelst[fan].vrt_start;
    vert = cmsh->xvrt.next[vert];
    vert = cmsh->xvrt.next[vert];
    vert = cmsh->xvrt.next[vert];
    vert = cmsh->xvrt.next[vert];
    bestprox = 9000;
    cnt = 4;
    while(cnt < num)
    {
      proxx = pmesh->TileDict[cmsh->mm->tilelst[fan].type].tx[cnt].u-nearx;
      proxy = pmesh->TileDict[cmsh->mm->tilelst[fan].type].tx[cnt].v-neary;
      if(proxx < 0) proxx=-proxx;
      if(proxy < 0) proxy=-proxy;
      prox = proxx+proxy;
      if(prox < bestprox)
      {
        bestvert = vert;
        bestprox = prox;
      }
      vert = cmsh->xvrt.next[vert];
      cnt++;
    }
  }
  return bestvert;
}

//------------------------------------------------------------------------------
void weld_select(MeshMem_t * mm, cart_select_list * csel)
{
  // ZZ> This function welds the highlighted vertices

  int cnt, x, y, z, ar, ag, ab;
  Uint32 vert;

  if(csel->count > 1)
  {
    x =  y = z = 0;
    ar = ag = ab = 0;
    cnt = 0;
    while(cnt < csel->count)
    {
      vert = csel->data[cnt];
      x+= mm->vrt_x[vert];
      y+= mm->vrt_y[vert];
      z+= mm->vrt_z[vert];
      ar+= mm->vrt_ar_fp8[vert];
      ag+= mm->vrt_ag_fp8[vert];
      ab+= mm->vrt_ab_fp8[vert];
      cnt++;
    }
    x+=cnt>>1;  y+=cnt>>1;
    x /= csel->count;
    y /= csel->count;
    z /= csel->count;
    ar /= csel->count;
    ag /= csel->count;
    ab /= csel->count;
    cnt = 0;
    while(cnt < csel->count)
    {
      vert = csel->data[cnt];
       mm->vrt_x[vert]=x;
       mm->vrt_y[vert]=y;
       mm->vrt_z[vert]=z;
       mm->vrt_ar_fp8[vert]=ar;
       mm->vrt_ag_fp8[vert]=ag;
       mm->vrt_ab_fp8[vert]=ab;
      cnt++;
    }
  }
  return;
}

//------------------------------------------------------------------------------
void add_select(cart_select_list * csel, int vert)
{
  // ZZ> This function highlights a vertex

  int cnt, found;

  if(csel->count < MAXSELECT && vert >= 0)
  {
    found = bfalse;
    cnt = 0;
    while(cnt < csel->count && !found)
    {
      if(csel->data[cnt]==vert)
      {
        found=btrue;
      }
      cnt++;
    }
    if(!found)
    {
      csel->data[csel->count] = vert;
      csel->count++;
    }
  }

  return;
}

//------------------------------------------------------------------------------
void clear_select(cart_select_list * csel)
{
  // ZZ> This function unselects all vertices

  csel->count = 0;
  return;
}

//------------------------------------------------------------------------------
int vert_selected(cart_select_list * csel, int vert)
{
  // ZZ> This function returns btrue if the vertex has been highlighted by user

  int cnt;

  cnt = 0;
  while(cnt < csel->count)
  {
    if(vert==csel->data[cnt])
    {
      return btrue;
    }
    cnt++;
  }

  return bfalse;
}

//------------------------------------------------------------------------------
void remove_select(cart_select_list * csel, int vert)
{
  // ZZ> This function makes sure the vertex is not highlighted

  int cnt, stillgoing;

  cnt = 0;
  stillgoing = btrue;
  while(cnt < csel->count && stillgoing)
  {
    if(vert==csel->data[cnt])
    {
      stillgoing = bfalse;
    }
    cnt++;
  }
  if(stillgoing == bfalse)
  {
    while(cnt < csel->count)
    {
      csel->data[cnt-1] = csel->data[cnt];
      cnt++;
    }
    csel->count--;
  }



  return;
}

//------------------------------------------------------------------------------
void fan_onscreen(cart_mesh * cmsh, Uint32 fan)
{
  // ZZ> This function flags a fan's points as being "onscreen"

  int cnt;
  Uint32 vert;

  vert = cmsh->mm->tilelst[fan].vrt_start;
  cnt = 0;
  while(cnt < pmesh->TileDict[cmsh->mm->tilelst[fan].type].vrt_count)
  {
    cpoint_lst.data[cpoint_lst.count] = vert;  cpoint_lst.count++;
    vert = cmsh->xvrt.next[vert];
    cnt++;
  }
  return;
}

//------------------------------------------------------------------------------
void make_onscreen( cart_mesh * cmsh )
{
  int x, y, cntx, cnty, numx, numy, mapx, mapy, mapxstt, mapystt;
  Uint32 fan;


  cpoint_lst.count = 0;
  mapxstt = (cart_pos_x-(ONSIZE>>1))/31;
  mapystt = (cart_pos_y-(ONSIZE>>1))/31;
  numx = (ONSIZE/SMALLX)+3;
  numy = (ONSIZE/SMALLY)+3;
  x = -cart_pos_x+(ONSIZE>>1)-SMALLX;
  y = -cart_pos_y+(ONSIZE>>1)-SMALLY;


  mapy = mapystt;
  cnty = 0;
  while(cnty < numy)
  {
    if(mapy>=0 && mapy<cmsh->mi->tiles_y)
    {
      mapx = mapxstt;
      cntx = 0;
      while(cntx < numx)
      {
        if(mapx>=0 && mapx<cmsh->mi->tiles_x)
        {
          fan = mesh_convert_fan(cmsh->mi, mapx, mapy);
          fan_onscreen(cmsh->mm, fan);
        }
        mapx++;
        cntx++;
      }
    }
    mapy++;
    cnty++;
  }

  return;
}

//------------------------------------------------------------------------------
void draw_top_fan(cart_mesh * cmsh, cart_window_info * w, int fan, int x, int y)
{
  // ZZ> This function draws the line drawing preview of the tile type...
  //     A wireframe tile from a vertex connection window

  Uint32 faketoreal[MAXMESHVERTICES];
  int fantype;
  int cnt, stt, end, vert;
  GLfloat * color;
  int size;

  set_window_viewport( w );

  fantype = cmsh->mm->tilelst[fan].type;
  color = make_color(16, 16, 31);
  if(fantype>=MAXMESHTYPE/2)
  {
    color = make_color(31, 16, 16);
  }

  vert = cmsh->mm->tilelst[fan].vrt_start;
  cnt = 0;
  while(cnt < pmesh->TileDict[fantype].vrt_count)
  {
    faketoreal[cnt] = vert;
    vert = cmsh->xvrt.next[vert];
    cnt++;
  }


  glBegin(GL_LINES);
  glColor3fv(color);
  cnt = 0;
  while(cnt < cfan_lines[fantype].numline)
  {
    stt = faketoreal[cfan_lines[fantype].linestart[cnt]];
    end = faketoreal[cfan_lines[fantype].lineend[cnt]];

    glVertex2f(cmsh->mm->vrt_x[stt]+x, cmsh->mm->vrt_y[stt]+y);
    glVertex2f(cmsh->mm->vrt_x[end]+x, cmsh->mm->vrt_y[end]+y);

    cnt++;
  }
  glEnd();



  cnt = 0;
  while(cnt < pmesh->TileDict[fantype].vrt_count)
  {
    vert = faketoreal[cnt];
    size = (cmsh->mm->vrt_z[vert] * 16.0f)/(cmsh->edgez+1);
    if(cmsh->mm->vrt_z[vert] >= 0)
    {
      if(vert_selected(&cselect_lst, vert))
      {
        draw_blit_sprite(NULL, cimg_lst.pointon + size,
          cmsh->mm->vrt_x[vert]+x-(cimg_lst.pointon[size].txW>>1),
          cmsh->mm->vrt_y[vert]+y-(cimg_lst.pointon[size].txH>>1));
      }
      else
      {
        draw_blit_sprite(NULL, cimg_lst.point + size,
          cmsh->mm->vrt_x[vert]+x-(cimg_lst.point[size].txW>>1),
          cmsh->mm->vrt_y[vert]+y-(cimg_lst.point[size].txH>>1));
      }
    }
    cnt++;
  }


  return;
}

//------------------------------------------------------------------------------
void draw_side_fan(cart_mesh * cmsh, cart_window_info * w, int fan, int x, int y)
{
  // ZZ> This function draws the line drawing preview of the tile type...
  //     A wireframe tile from a vertex connection window ( Side view )

  Uint32 faketoreal[MAXMESHVERTICES];
  int fantype;
  int cnt, stt, end, vert;
  GLfloat * color;
  int size;

  set_window_viewport( w );

  fantype = cmsh->mm->tilelst[fan].type;
  color = make_color(16, 16, 31);
  if(fantype>=MAXMESHTYPE/2)
  {
    color = make_color(31, 16, 16);
  }

  vert = cmsh->mm->tilelst[fan].vrt_start;
  cnt = 0;
  while(cnt < pmesh->TileDict[fantype].vrt_count)
  {
    faketoreal[cnt] = vert;
    vert = cmsh->xvrt.next[vert];
    cnt++;
  }


  glBegin(GL_LINES);
  glColor3fv( color );
  cnt = 0;
  while(cnt < cfan_lines[fantype].numline)
  {
    stt = faketoreal[cfan_lines[fantype].linestart[cnt]];
    end = faketoreal[cfan_lines[fantype].lineend[cnt]];
    if(cmsh->mm->vrt_z[stt] >= 0 && cmsh->mm->vrt_z[end] >= 0)
    {
      glVertex2f( cmsh->mm->vrt_x[stt]+x, -(cmsh->mm->vrt_z[stt] / 16.0f)+y );
      glVertex2f( cmsh->mm->vrt_x[end]+x, -(cmsh->mm->vrt_z[end] / 16.0f)+y );
    }
    cnt++;
  }
  glEnd();


  size = 5;
  cnt = 0;
  while(cnt < pmesh->TileDict[fantype].vrt_count)
  {
    vert = faketoreal[cnt];
    if(cmsh->mm->vrt_z[vert] >= 0)
    {
      if(vert_selected(&cselect_lst, vert))
      {
        draw_blit_sprite(w, cimg_lst.pointon + size,
          cmsh->mm->vrt_x[vert]+x-(cimg_lst.pointon[size].txW>>1),
          -(cmsh->mm->vrt_z[vert] / 16.0f)+y-(cimg_lst.pointon[size].txH>>1));
      }
      else
      {
        draw_blit_sprite(w, cimg_lst.point + size,
          cmsh->mm->vrt_x[vert]+x-(cimg_lst.point[size].txW>>1),
          -(cmsh->mm->vrt_z[vert] / 16.0f)+y-(cimg_lst.point[size].txH>>1));
      }
    }
    cnt++;
  }


  return;
}

//------------------------------------------------------------------------------
void draw_schematic(MeshMem_t * mm, cart_mouse_info * m, cart_window_info * w, int fantype, int x, int y)
{
  // ZZ> This function draws the line drawing preview of the tile type...
  //     The wireframe on the left side of the screen.

  int cnt, stt, end;
  GLfloat * color;

  color = make_color(16, 16, 31);
  if(m->type>=MAXMESHTYPE/2)  color = make_color(31, 16, 16);

  glBegin(GL_LINES);
  glColor4fv( color );
  cnt = 0;
  while(cnt < cfan_lines[fantype].numline)
  {
    stt = cfan_lines[fantype].linestart[cnt];
    end = cfan_lines[fantype].lineend[cnt];

    glVertex2f( pmesh->TileDict[fantype].tx[stt].u+x, pmesh->TileDict[fantype].tx[stt].v+y );
    glVertex2f( pmesh->TileDict[fantype].tx[end].u+x, pmesh->TileDict[fantype].tx[end].v+y );

    cnt++;
  }
  glEnd();


}

//------------------------------------------------------------------------------
void add_line(cart_fan_lines * clines, int fantype, int start, int end)
{
  // ZZ> This function adds a line to the vertex schematic

  int cnt;

  // Make sure line isn't already in list
  cnt = 0;
  while(cnt < clines[fantype].numline)
  {
    if((clines[fantype].linestart[cnt]==start &&
      clines[fantype].lineend[cnt]==end) ||
      (clines[fantype].lineend[cnt]==start &&
      clines[fantype].linestart[cnt]==end))
    {
      return;
    }
    cnt++;
  }


  // Add it in
  clines[fantype].linestart[cnt]=start;
  clines[fantype].lineend[cnt]=end;
  clines[fantype].numline++;

  return;
}


//------------------------------------------------------------------------------
void free_vertices(cart_mesh * cmsh)
{
  // ZZ> This function sets all vertices to unused

  int cnt;

  cnt = 0;
  while(cnt < MAXTOTALMESHVERTICES)
  {
    cmsh->mm->vrt_ar_fp8[cnt] = VERTEXUNUSED;
    cnt++;
  }
  cmsh->xvrt.index = 0;
  cmsh->xvrt.free_count = MAXTOTALMESHVERTICES;
  return;
}

//------------------------------------------------------------------------------
int get_free_vertex(cart_mesh * cmsh)
{
  // ZZ> This function returns btrue if it can find an unused vertex, and it
  // will set cmsh->xvrt.index to that vertex index.  bfalse otherwise.

  int cnt;

  if(cmsh->xvrt.free_count!=0)
  {
    cnt = 0;
    while(cnt < MAXTOTALMESHVERTICES && cmsh->mm->vrt_ar_fp8[cmsh->xvrt.index]!=VERTEXUNUSED)
    {
      cmsh->xvrt.index++;
      if(cmsh->xvrt.index == MAXTOTALMESHVERTICES)
      {
        cmsh->xvrt.index = 0;
      }
      cnt++;
    }
    if(cmsh->mm->vrt_ar_fp8[cmsh->xvrt.index]==VERTEXUNUSED)
    {
      cmsh->mm->vrt_ar_fp8[cmsh->xvrt.index]=60;
      return btrue;
    }
  }
  return bfalse;
}

//------------------------------------------------------------------------------
void remove_fan(cart_mesh * cmsh, int fan)
{
  // ZZ> This function removes a fan's vertices from usage and sets the fan
  //     to not be drawn

  int cnt, vert;
  Uint32 numvert;


  numvert = pmesh->TileDict[cmsh->mm->tilelst[fan].type].vrt_count;
  vert = cmsh->mm->tilelst[fan].vrt_start;
  cnt = 0;
  while(cnt < numvert)
  {
    cmsh->mm->vrt_ar_fp8[vert] = VERTEXUNUSED;
    cmsh->xvrt.free_count++;
    vert = cmsh->xvrt.next[vert];
    cnt++;
  }
  cmsh->mm->tilelst[fan].type = 0;
  cmsh->mm->tilelst[fan].fx = MPDFX_NOREFLECT;
}

//------------------------------------------------------------------------------
int add_fan(cart_mesh *cmsh, int fan, int x, int y)
{
  // ZZ> This function allocates the vertices needed for a fan

  int cnt;
  int numvert;
  Uint32 vertex;
  Uint32 vertexlist[17];


  numvert = pmesh->TileDict[cmsh->mm->tilelst[fan].type].vrt_count;
  if(cmsh->xvrt.free_count >= numvert)
  {
    cmsh->mm->tilelst[fan].fx = MPDFX_NOREFLECT;
    cnt = 0;
    while(cnt < numvert)
    {
      if(get_free_vertex(cmsh->mm)==bfalse)
      {
        // Reset to unused
        numvert = cnt;
        cnt = 0;
        while(cnt < numvert)
        {
          cmsh->mm->vrt_ar_fp8[cnt] = 60;
          cnt++;
        }
        return bfalse;
      }
      vertexlist[cnt] = cmsh->xvrt.index;
      cnt++;
    }
    vertexlist[cnt] = CHAINEND;


    cnt = 0;
    while(cnt < numvert)
    {
      vertex = vertexlist[cnt];
      cmsh->mm->vrt_x[vertex] = x + (pmesh->TileDict[cmsh->mm->tilelst[fan].type].tx[cnt].u / 4.0f);
      cmsh->mm->vrt_y[vertex] = y + (pmesh->TileDict[cmsh->mm->tilelst[fan].type].tx[cnt].v / 4.0f);
      cmsh->mm->vrt_z[vertex] = 0;
      cmsh->xvrt.next[vertex] = vertexlist[cnt+1];
      cnt++;
    }
    cmsh->mm->tilelst[fan].vrt_start = vertexlist[0];
    cmsh->xvrt.free_count-=numvert;
    return btrue;
  }
  return bfalse;
}

//------------------------------------------------------------------------------
void num_free_vertex(cart_mesh * cmsh)
{
  // ZZ> This function counts the unused vertices and sets cmsh->xvrt.free_count

  int cnt, num;

  num = 0;
  cnt = 0;
  while(cnt < MAXTOTALMESHVERTICES)
  {
    if(cmsh->mm->vrt_ar_fp8[cnt]==VERTEXUNUSED)
    {
      num++;
    }
    cnt++;
  }
  cmsh->xvrt.free_count=num;
}

//------------------------------------------------------------------------------
GLtexture * tile_at(cart_mesh * cmsh, int x, int y)
{
  GLtexture * retval = NULL;
  Uint16 tile, basetile;
  Uint8 type, fx;
  Uint32 fan;

  if(x < 0 || x >= cmsh->mi->tiles_x || y < 0 || y >= cmsh->mi->tiles_y)
  {
    return retval;
  }

  fan = mesh_convert_fan(cmsh->mi, x, y);
  if(INVALID_FAN == cmsh->mm->tilelst[fan].tile)
  {
    return NULL;
  }

  tile = cmsh->mm->tilelst[fan].tile;
  type = cmsh->mm->tilelst[fan].type;
  fx = cmsh->mm->tilelst[fan].fx;

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

  if(type >= (MAXMESHTYPE>>1))
  {
    retval = cbmp_lst.bigtile + tile;
  }
  else
  {
    retval = cbmp_lst.smalltile + tile;
  }

  return retval;
}

//------------------------------------------------------------------------------
int fan_at(cart_mesh * cmsh, int x, int y)
{
  int fan;

  if(x < 0 || x >= cmsh->mi->tiles_x || y < 0 || y >= cmsh->mi->tiles_y)
  {
    return INVALID_FAN;
  }

  fan = mesh_convert_fan(cmsh->mi, x, y);
  return fan;
}

//------------------------------------------------------------------------------
void weld_0(cart_mesh * cmsh, int x, int y)
{
  cart_select_list tlst = {0};

  clear_select(&tlst);
  add_select(&tlst, get_vertex(cmsh, x, y, 0));
  add_select(&tlst, get_vertex(cmsh, x-1, y, 1));
  add_select(&tlst, get_vertex(cmsh, x, y-1, 3));
  add_select(&tlst, get_vertex(cmsh, x-1, y-1, 2));
  weld_select(cmsh->mm, &tlst);
  clear_select(&tlst);
  return;
}

//------------------------------------------------------------------------------
void weld_1(cart_mesh * cmsh, int x, int y)
{
  cart_select_list tlst = {0};

  clear_select(&tlst);
  add_select(&tlst, get_vertex(cmsh, x, y, 1));
  add_select(&tlst, get_vertex(cmsh, x+1, y, 0));
  add_select(&tlst, get_vertex(cmsh, x, y-1, 2));
  add_select(&tlst, get_vertex(cmsh, x+1, y-1, 3));
  weld_select(cmsh->mm, &tlst);
  clear_select(&tlst);

  return;
}

//------------------------------------------------------------------------------
void weld_2(cart_mesh * cmsh, int x, int y)
{
  cart_select_list tlst = {0};

  clear_select(&tlst);
  add_select(&tlst, get_vertex(cmsh, x, y, 2));
  add_select(&tlst, get_vertex(cmsh, x+1, y, 3));
  add_select(&tlst, get_vertex(cmsh, x, y+1, 1));
  add_select(&tlst, get_vertex(cmsh, x+1, y+1, 0));
  weld_select(cmsh->mm, &tlst);
  clear_select(&tlst);

  return;
}

//------------------------------------------------------------------------------
void weld_3(cart_mesh * cmsh, int x, int y)
{
  cart_select_list tlst = {0};

  clear_select(&tlst);
  add_select(&tlst, get_vertex(cmsh, x, y, 3));
  add_select(&tlst, get_vertex(cmsh, x-1, y, 2));
  add_select(&tlst, get_vertex(cmsh, x, y+1, 0));
  add_select(&tlst, get_vertex(cmsh, x-1, y+1, 1));
  weld_select(cmsh->mm, &tlst);
  clear_select(&tlst);

}

//------------------------------------------------------------------------------
void weld_cnt(cart_mesh * cmsh, int x, int y, int cnt, Uint32 fan)
{
  cart_select_list tlst = {0};

  if(pmesh->TileDict[cmsh->mm->tilelst[fan].type].tx[cnt].u < NEARLOW+1 ||
     pmesh->TileDict[cmsh->mm->tilelst[fan].type].tx[cnt].v < NEARLOW+1 ||
     pmesh->TileDict[cmsh->mm->tilelst[fan].type].tx[cnt].u > NEARHI-1 ||
     pmesh->TileDict[cmsh->mm->tilelst[fan].type].tx[cnt].v > NEARHI-1)
  {
    clear_select(&tlst);
    add_select(&tlst, get_vertex(cmsh, x, y, cnt));

    if(pmesh->TileDict[cmsh->mm->tilelst[fan].type].tx[cnt].u < NEARLOW+1)
      add_select(&tlst, nearest_vertex(cmsh, x-1, y, NEARHI, pmesh->TileDict[cmsh->mm->tilelst[fan].type].tx[cnt].v));

    if(pmesh->TileDict[cmsh->mm->tilelst[fan].type].tx[cnt].v < NEARLOW+1)
      add_select(&tlst, nearest_vertex(cmsh, x, y-1, pmesh->TileDict[cmsh->mm->tilelst[fan].type].tx[cnt].u, NEARHI));

    if(pmesh->TileDict[cmsh->mm->tilelst[fan].type].tx[cnt].u > NEARHI-1)
      add_select(&tlst, nearest_vertex(cmsh, x+1, y, NEARLOW, pmesh->TileDict[cmsh->mm->tilelst[fan].type].tx[cnt].v));

    if(pmesh->TileDict[cmsh->mm->tilelst[fan].type].tx[cnt].v > NEARHI-1)
      add_select(&tlst, nearest_vertex(cmsh, x, y+1, pmesh->TileDict[cmsh->mm->tilelst[fan].type].tx[cnt].u, NEARLOW));

    weld_select(cmsh->mm, &tlst);
    clear_select(&tlst);
  }

}

//------------------------------------------------------------------------------
void fix_corners(cart_mesh * cmsh, int x, int y)
{
  Uint32 fan;


  fan = mesh_convert_fan(cmsh, x, y);
  weld_0(cmsh, x, y);
  weld_1(cmsh, x, y);
  weld_2(cmsh, x, y);
  weld_3(cmsh, x, y);
}

//------------------------------------------------------------------------------
void fix_vertices(cart_mesh * cmsh, int x, int y)
{
  Uint32 fan;
  int cnt;

  fix_corners(cmsh, x, y);
  fan = mesh_convert_fan(cmsh->mi, x, y);
  cnt = 4;
  while(cnt < pmesh->TileDict[cmsh->mm->tilelst[fan].type].vrt_count)
  {
    weld_cnt(cmsh, x, y, cnt, fan);
    cnt++;
  }
}

//------------------------------------------------------------------------------
void fix_mesh(cart_mesh * cmsh)
{
  // ZZ> This function corrects corners across entire mesh

  int x, y;

  y = 0;
  while(y < cmsh->mi->tiles_y)
  {
    x = 0;
    while(x < cmsh->mi->tiles_x)
    {
      //      fix_corners(cmsh, x, y);
      fix_vertices(cmsh, x, y);
      x++;
    }
    y++;
  }
  return;
}


//------------------------------------------------------------------------------
char tile_is_different(cart_mesh * cmsh, int x, int y, Uint16 tileset,
                       Uint16 tileand)
{
  // ZZ> bfalse if of same set, btrue if different

  Uint32 fan;
  if(x < 0 || x >= cmsh->mi->tiles_x || y < 0 || y >= cmsh->mi->tiles_y)
  {
    return bfalse;
  }

  fan = mesh_convert_fan(cmsh->mi, x, y);
  if(tileand == 192)
  {
    if(cmsh->mm->tilelst[fan].tile >= 48) return bfalse;
  }

  if((cmsh->mm->tilelst[fan].tile&tileand) == tileset)
  {
    return bfalse;
  }
  return btrue;
}

//------------------------------------------------------------------------------
Uint16 trim_code(cart_mesh * cmsh, int x, int y, Uint16 tileset)
{
  // ZZ> This function returns the standard tile set value thing...  For
  //     Trimming tops of walls and floors

  Uint16 code;

  if(tile_is_different(cmsh, x, y-1, tileset, 240))
  {
    // Top
    code = 0;
    if(tile_is_different(cmsh, x-1, y, tileset, 240))
    {
      // Left
      code = 8;
    }
    if(tile_is_different(cmsh, x+1, y, tileset, 240))
    {
      // Right
      code = 9;
    }
    return code;
  }
  if(tile_is_different(cmsh, x, y+1, tileset, 240))
  {
    // Bottom
    code = 1;
    if(tile_is_different(cmsh, x-1, y, tileset, 240))
    {
      // Left
      code = 10;
    }
    if(tile_is_different(cmsh, x+1, y, tileset, 240))
    {
      // Right
      code = 11;
    }
    return code;
  }
  if(tile_is_different(cmsh, x-1, y, tileset, 240))
  {
    // Left
    code = 2;
    return code;
  }
  if(tile_is_different(cmsh, x+1, y, tileset, 240))
  {
    // Right
    code = 3;
    return code;
  }



  if(tile_is_different(cmsh, x+1, y+1, tileset, 240))
  {
    // Bottom Right
    code = 4;
    return code;
  }
  if(tile_is_different(cmsh, x-1, y+1, tileset, 240))
  {
    // Bottom Left
    code = 5;
    return code;
  }
  if(tile_is_different(cmsh, x+1, y-1, tileset, 240))
  {
    // Top Right
    code = 6;
    return code;
  }
  if(tile_is_different(cmsh, x-1, y-1, tileset, 240))
  {
    // Top Left
    code = 7;
    return code;
  }


  code = 255;
  return code;
}

//------------------------------------------------------------------------------
Uint16 wall_code(cart_mesh * cmsh, int x, int y, Uint16 tileset)
{
  // ZZ> This function returns the standard tile set value thing...  For
  //     Trimming tops of walls and floors

  Uint16 code;

  if(tile_is_different(cmsh, x, y-1, tileset, 192))
  {
    // Top
    code = (rand()&2) + 20;
    if(tile_is_different(cmsh, x-1, y, tileset, 192))
    {
      // Left
      code = 48;
    }
    if(tile_is_different(cmsh, x+1, y, tileset, 192))
    {
      // Right
      code = 50;
    }
    return code;
  }

  if(tile_is_different(cmsh, x, y+1, tileset, 192))
  {
    // Bottom
    code = (rand()&2);
    if(tile_is_different(cmsh, x-1, y, tileset, 192))
    {
      // Left
      code = 52;
    }
    if(tile_is_different(cmsh, x+1, y, tileset, 192))
    {
      // Right
      code = 54;
    }
    return code;
  }

  if(tile_is_different(cmsh, x-1, y, tileset, 192))
  {
    // Left
    code = (rand()&2) + 16;
    return code;
  }
  if(tile_is_different(cmsh, x+1, y, tileset, 192))
  {
    // Right
    code = (rand()&2) + 4;
    return code;
  }


  if(tile_is_different(cmsh, x+1, y+1, tileset, 192))
  {
    // Bottom Right
    code = 32;
    return code;
  }

  if(tile_is_different(cmsh, x-1, y+1, tileset, 192))
  {
    // Bottom Left
    code = 34;
    return code;
  }

  if(tile_is_different(cmsh, x+1, y-1, tileset, 192))
  {
    // Top Right
    code = 36;
    return code;
  }

  if(tile_is_different(cmsh, x-1, y-1, tileset, 192))
  {
    // Top Left
    code = 38;
    return code;
  }

  code = 255;
  return code;
}

//------------------------------------------------------------------------------
void trim_mesh_tile(cart_mesh * cmsh, Uint16 tileset, Uint16 tileand)
{
  // ZZ> This function trims walls and floors and tops automagically

  Uint32 fan;
  int x, y, code;

  tileset = tileset&tileand;
  y = 0;
  while(y < cmsh->mi->tiles_y)
  {
    x = 0;
    while(x < cmsh->mi->tiles_x)
    {
      fan = mesh_convert_fan(cmsh->mi, x, y);
      if((cmsh->mm->tilelst[fan].tile&tileand) == tileset)
      {
        if(tileand == 192)
        {
          code = wall_code(cmsh->mm, x, y, tileset);
        }
        else
        {
          code = trim_code(cmsh->mm, x, y, tileset);
        }
        if(code != 255)
        {
          cmsh->mm->tilelst[fan].tile = tileset + code;
        }
      }
      x++;
    }
    y++;
  }
  return;
}

//------------------------------------------------------------------------------
void fx_mesh_tile(cart_mesh * cmsh, Uint16 tileset, Uint16 tileand, Uint8 fx)
{
  // ZZ> This function sets the fx for a group of tiles

  Uint32 fan;
  int x, y;

  tileset = tileset&tileand;
  y = 0;
  while(y < cmsh->mi->tiles_y)
  {
    x = 0;
    while(x < cmsh->mi->tiles_x)
    {
      fan = mesh_convert_fan(cmsh->mi, x, y);
      if((cmsh->mm->tilelst[fan].tile&tileand) == tileset)
      {
        cmsh->mm->tilelst[fan].fx = fx;
      }
      x++;
    }
    y++;
  }
  return;
}

//------------------------------------------------------------------------------
void set_mesh_tile(cart_mesh * cmsh, cart_mouse_info * m, Uint16 tiletoset)
{
  // ZZ> This function sets one tile type to another

  Uint32 fan;
  int x, y;

  y = 0;
  while(y < cmsh->mi->tiles_y)
  {
    x = 0;
    while(x < cmsh->mi->tiles_x)
    {
      fan = mesh_convert_fan(cmsh->mi, x, y);
      if(cmsh->mm->tilelst[fan].tile == tiletoset)
      {
        switch(m->presser)
        {
        case 0:
          cmsh->mm->tilelst[fan].tile=m->tile;
          break;
        case 1:
          cmsh->mm->tilelst[fan].tile=(m->tile&0xfffe)+IRAND(&(cmsh->seed), 1);
          break;
        case 2:
          cmsh->mm->tilelst[fan].tile=(m->tile&0xfffc)+IRAND(&(cmsh->seed), 2);
          break;
        case 3:
          cmsh->mm->tilelst[fan].tile=(m->tile&0xfff0)+(rand()&6);
          break;
        }
      }
      x++;
    }
    y++;
  }
  return;
}

//------------------------------------------------------------------------------
void setup_mesh(cart_mesh * cmsh)
{
  // ZZ> This function makes the mesh

  int x, y, fan, tile;

  free_vertices(cmsh->mm);
  printf("Mesh file not found, so creating a new one...\n");
  printf("Number of tiles in X direction ( 32-512 ):  ");
  scanf("%d", &cmsh->mi->tiles_x);
  printf("Number of tiles in Y direction ( 32-512 ):  ");
  scanf("%d", &cmsh->mi->tiles_y);
  cmsh->mi->edge_x = (cmsh->mi->tiles_x*SMALLX)-1;
  cmsh->mi->edge_y = (cmsh->mi->tiles_y*SMALLY)-1;
  cmsh->edgez = 180<<4;


  fan = 0;
  y = 0;
  tile = 0;
  while(y < cmsh->mi->tiles_y)
  {
    x = 0;
    while(x < cmsh->mi->tiles_x)
    {
      cmsh->mm->tilelst[fan].type = 2+0;
      cmsh->mm->tilelst[fan].tile = (((x&1)+(y&1))&1)+62;
      if(add_fan(cmsh->mm, fan, x*31, y*31)==bfalse)
      {
        log_error("NOT ENOUGH VERTICES!!!\n\n");
      }
      fan++;
      x++;
    }
    y++;
  }

  mesh_make_fanstart(cmsh->mi);
  fix_mesh(cmsh);
}

//------------------------------------------------------------------------------
void rip_small_tiles(SDL_Surface * bmpload)
{
  SDL_Surface *bmp_small, *bmp_tiny;
  int x, y;

  bmp_small = SDL_CreateRGBSurface(SDL_SWSURFACE, SMALLX, SMALLY, 32, rmask, gmask, bmask, amask);
  bmp_tiny  = SDL_CreateRGBSurface(SDL_SWSURFACE, TINYX, TINYY, 32, rmask, gmask, bmask, amask);


  y = 0;
  while(y < 256)
  {
    x = 0;
    while(x < 256)
    {
      SDL_Rect src = {x, y, TINYX, TINYY};

      SDL_BlitSurface(bmpload, &src, bmp_small, &bmp_small->clip_rect);
      GLtexture_Convert( GL_TEXTURE_2D, cbmp_lst.smalltile + numsmalltile, bmp_small, INVALID_KEY);

      SDL_BlitSurface(bmpload, &src, bmp_tiny, &bmp_tiny->clip_rect);
      GLtexture_Convert( GL_TEXTURE_2D, cbmp_lst.tinysmalltile + numsmalltile, bmp_tiny, INVALID_KEY);

      numsmalltile++;
      x+=32;
    }
    y+=32;
  }

  SDL_FreeSurface(bmp_small);
  SDL_FreeSurface(bmp_tiny);

  return;
}

//------------------------------------------------------------------------------
void rip_big_tiles(SDL_Surface* bmpload)
{
  SDL_Surface *bmp_small, *bmp_tiny;
  int x, y;

  bmp_small = SDL_CreateRGBSurface(SDL_SWSURFACE, SMALLX, SMALLY, 32, rmask, gmask, bmask, amask);
  bmp_tiny  = SDL_CreateRGBSurface(SDL_SWSURFACE, TINYX, TINYY, 32, rmask, gmask, bmask, amask);

  y = 0;
  while(y < 256)
  {
    x = 0;
    while(x < 256)
    {
      SDL_Rect src = {x, y, x + BIGX, y + BIGY};

      SDL_BlitSurface(bmpload, &src, bmp_small, &bmp_small->clip_rect);
      GLtexture_Convert(GL_TEXTURE_2D, cbmp_lst.bigtile + numbigtile, bmp_small, INVALID_KEY);

      SDL_BlitSurface(bmpload, &src, bmp_tiny, &bmp_tiny->clip_rect);
      GLtexture_Convert(GL_TEXTURE_2D, cbmp_lst.tinybigtile + numbigtile, bmp_tiny, INVALID_KEY);

      numbigtile++;
      x+=32;
    }
    y+=32;
  }
  SDL_FreeSurface(bmp_small);
  SDL_FreeSurface(bmp_tiny);

  return;
}

//------------------------------------------------------------------------------
void rip_tiles(SDL_Surface * bmpload)
{
  rip_small_tiles(bmpload);
  rip_big_tiles(bmpload);
}

//------------------------------------------------------------------------------
void cart_load_basic_textures(char *modname)
{
  // ZZ> This function loads the standard textures for a module

  char newloadname[256];
  SDL_Surface * surface;

  snprintf(newloadname, sizeof(newloadname), "%s" SLASH_STRING "gamedat" SLASH_STRING "tile0.bmp", modname);
  surface = IMG_Load(newloadname);
  rip_tiles(surface);

  snprintf(newloadname, sizeof(newloadname), "%s" SLASH_STRING "gamedat" SLASH_STRING "tile1.bmp", modname);
  surface = IMG_Load(newloadname);
  rip_tiles(surface);

  snprintf(newloadname, sizeof(newloadname), "%s" SLASH_STRING "gamedat" SLASH_STRING "tile2.bmp", modname);
  surface = IMG_Load(newloadname);
  rip_tiles(surface);

  snprintf(newloadname, sizeof(newloadname), "%s" SLASH_STRING "gamedat" SLASH_STRING "tile3.bmp", modname);
  surface = IMG_Load(newloadname);
  rip_tiles(surface);

  //cbmp_lst.fanoff = SDL_CreateRGBSurface(SDL_SWSURFACE, SMALLX, SMALLY, 32, rmask, gmask, bmask, amask);
  //draw_blit_sprite(cbmp_lst.fanoff, cimg_lst.pointon[10], (SMALLX-(*cimg_lst.pointon[10]).txW)/2, (SMALLY-(*cimg_lst.pointon[10]).txH)/2);
  return;
}


//------------------------------------------------------------------------------
//void show_name(char *newloadname)
//{
//  textout(screen, font, newloadname, 0, OUTY-16, make_color(31, 31, 31));
//  return;
//}

//------------------------------------------------------------------------------
int count_vertices(cart_mesh * cmsh)
{
  int fan, x, y, cnt, num, totalvert;
  Uint32 vert;

  totalvert = 0;
  y = 0;
  while(y < cmsh->mi->tiles_y)
  {
    x = 0;
    while(x < cmsh->mi->tiles_x)
    {
      fan = mesh_convert_fan(cmsh->mi, x, y);
      num = pmesh->TileDict[cmsh->mm->tilelst[fan].type].vrt_count;
      vert = cmsh->mm->tilelst[fan].vrt_start;
      cnt = 0;
      while(cnt < num)
      {
        totalvert++;
        vert = cmsh->xvrt.next[vert];
        cnt++;
      }
      x++;
    }
    y++;
  }
  return totalvert;
}

//------------------------------------------------------------------------------
void save_mesh(cart_mesh * cmsh, char *modname)
{
#define SAVE numwritten+=fwrite(&itmp, 4, 1, filewrite); numattempt++
#define SAVEF numwritten+=fwrite(&ftmp, 4, 1, filewrite); numattempt++
  FILE* filewrite;
  char newloadname[256];
  int itmp;
  float ftmp;
  int fan, x, y, cnt, num;
  Uint32 vert;
  Uint8 ctmp;


  numwritten = 0;
  numattempt = 0;
  make_newloadname(modname, SLASH_STRING "gamedat" SLASH_STRING "plan.bmp", newloadname);
  make_planmap(cmsh);
  if(cbmp_lst.hitemap)
  {
    SDL_SaveBMP(cbmp_lst.hitemap, newloadname);
  }


  make_newloadname(modname, SLASH_STRING "gamedat" SLASH_STRING "level.bmp", newloadname);
  make_hitemap(cmsh);
  if(cbmp_lst.hitemap)
  {
    SDL_SaveBMP(cbmp_lst.hitemap, newloadname);
  }
  mesh_make_twist();


  make_newloadname(modname, SLASH_STRING "gamedat" SLASH_STRING "level.mpd", newloadname);
  debug_message(1, newloadname);

  filewrite = fopen(newloadname, "wb");
  if(filewrite)
  {
    itmp=MAPID;  SAVE;
    //    This didn't work for some reason...
    //    itmp=MAXTOTALMESHVERTICES-cmsh->xvrt.free_count;  SAVE;
    itmp = count_vertices(cmsh);  SAVE;
    itmp=cmsh->mi->tiles_x;  SAVE;
    itmp=cmsh->mi->tiles_y;  SAVE;

    // Write tile data
    y = 0;
    while(y < cmsh->mi->tiles_y)
    {
      x = 0;
      while(x < cmsh->mi->tiles_x)
      {
        fan = mesh_convert_fan(cmsh->mi, x, y);
        itmp = (cmsh->mm->tilelst[fan].type<<24)+(cmsh->mm->tilelst[fan].fx<<16)+cmsh->mm->tilelst[fan].tile;  SAVE;
        x++;
      }
      y++;
    }

    // Write twist data
    y = 0;
    while(y < cmsh->mi->tiles_y)
    {
      x = 0;
      while(x < cmsh->mi->tiles_x)
      {
        fan = mesh_convert_fan(cmsh->mi, x, y);
        ctmp = cmsh->mm->tilelst[fan].twist;  numwritten+=fwrite(&ctmp, 1, 1, filewrite);
        numattempt++;
        x++;
      }
      y++;
    }

    // Write x vertices
    y = 0;
    while(y < cmsh->mi->tiles_y)
    {
      x = 0;
      while(x < cmsh->mi->tiles_x)
      {
        fan = mesh_convert_fan(cmsh->mi, x, y);
        num = pmesh->TileDict[cmsh->mm->tilelst[fan].type].vrt_count;
        vert = cmsh->mm->tilelst[fan].vrt_start;
        cnt = 0;
        while(cnt < num)
        {
          ftmp = cmsh->mm->vrt_x[vert]*FIXNUM;  SAVEF;
          vert = cmsh->xvrt.next[vert];
          cnt++;
        }
        x++;
      }
      y++;
    }

    // Write y vertices
    y = 0;
    while(y < cmsh->mi->tiles_y)
    {
      x = 0;
      while(x < cmsh->mi->tiles_x)
      {
        fan = mesh_convert_fan(cmsh->mi, x, y);
        num = pmesh->TileDict[cmsh->mm->tilelst[fan].type].vrt_count;
        vert = cmsh->mm->tilelst[fan].vrt_start;
        cnt = 0;
        while(cnt < num)
        {
          ftmp = cmsh->mm->vrt_y[vert]*FIXNUM;  SAVEF;
          vert = cmsh->xvrt.next[vert];
          cnt++;
        }
        x++;
      }
      y++;
    }

    // Write z vertices
    y = 0;
    while(y < cmsh->mi->tiles_y)
    {
      x = 0;
      while(x < cmsh->mi->tiles_x)
      {
        fan = mesh_convert_fan(cmsh->mi, x, y);
        num = pmesh->TileDict[cmsh->mm->tilelst[fan].type].vrt_count;
        vert = cmsh->mm->tilelst[fan].vrt_start;
        cnt = 0;
        while(cnt < num)
        {
          ftmp = cmsh->mm->vrt_z[vert]*FIXNUM;  SAVEF;
          vert = cmsh->xvrt.next[vert];
          cnt++;
        }
        x++;
      }
      y++;
    }


    // Write a vertices
    y = 0;
    while(y < cmsh->mi->tiles_y)
    {
      x = 0;
      while(x < cmsh->mi->tiles_x)
      {
        fan = mesh_convert_fan(cmsh->mi, x, y);
        num = pmesh->TileDict[cmsh->mm->tilelst[fan].type].vrt_count;
        vert = cmsh->mm->tilelst[fan].vrt_start;
        cnt = 0;
        while(cnt < num)
        {
          ctmp = cmsh->mm->vrt_ar_fp8[vert];  numwritten+=fwrite(&ctmp, 1, 1, filewrite);
          numattempt++;
          vert = cmsh->xvrt.next[vert];
          cnt++;
        }
        x++;
      }
      y++;
    }
  }

  return;
}


//------------------------------------------------------------------------------
void move_select(cart_mesh * cmsh, cart_select_list * csel, int x, int y, int z)
{
  int vert, cnt, newx, newy, newz;


  cnt = 0;
  while(cnt < csel->count)
  {
    vert = csel->data[cnt];
    newx = cmsh->mm->vrt_x[vert]+x;
    newy = cmsh->mm->vrt_y[vert]+y;
    newz = cmsh->mm->vrt_z[vert]+z;
    if(newx<0)  x=0-cmsh->mm->vrt_x[vert];
    if(newx>cmsh->mi->edge_x) x=cmsh->mi->edge_x-cmsh->mm->vrt_x[vert];
    if(newy<0)  y=0-cmsh->mm->vrt_y[vert];
    if(newy>cmsh->mi->edge_y) y=cmsh->mi->edge_y-cmsh->mm->vrt_y[vert];
    if(newz<0)  z=0-cmsh->mm->vrt_z[vert];
    if(newz>cmsh->edgez) z=cmsh->edgez-cmsh->mm->vrt_z[vert];
    cnt++;
  }



  cnt = 0;
  while(cnt < csel->count)
  {
    vert = csel->data[cnt];
    newx = cmsh->mm->vrt_x[vert]+x;
    newy = cmsh->mm->vrt_y[vert]+y;
    newz = cmsh->mm->vrt_z[vert]+z;


    if(newx<0)  newx=0;
    if(newx>cmsh->mi->edge_x)  newx=cmsh->mi->edge_x;
    if(newy<0)  newy=0;
    if(newy>cmsh->mi->edge_y)  newy=cmsh->mi->edge_y;
    if(newz<0)  newz=0;
    if(newz>cmsh->edgez)  newz=cmsh->edgez;


    cmsh->mm->vrt_x[vert]=newx;
    cmsh->mm->vrt_y[vert]=newy;
    cmsh->mm->vrt_z[vert]=newz;
    cnt++;
  }
  return;
}

//------------------------------------------------------------------------------
void set_select_no_bound_z(MeshMem_t * mm, cart_select_list * csel, int z)
{
  int vert, cnt;


  cnt = 0;
  while(cnt < csel->count)
  {
    vert = csel->data[cnt];
    mm->vrt_z[vert]=z;
    cnt++;
  }
  return;
}

//------------------------------------------------------------------------------
void move_mesh_z(cart_mesh * cmsh, int z, Uint16 tiletype, Uint16 tileand)
{
  int vert, cnt, newz, x, y, totalvert;
  Uint32 fan;


  tiletype = tiletype & tileand;
  y = 0;
  while(y < cmsh->mi->tiles_y)
  {
    x = 0;
    while(x < cmsh->mi->tiles_x)
    {
      fan = mesh_convert_fan(cmsh->mi, x, y);
      if((cmsh->mm->tilelst[fan].tile&tileand) == tiletype)
      {
        vert = cmsh->mm->tilelst[fan].vrt_start;
        totalvert = pmesh->TileDict[cmsh->mm->tilelst[fan].type].vrt_count;
        cnt = 0;
        while(cnt < totalvert)
        {
          newz = cmsh->mm->vrt_z[vert]+z;
          if(newz<0)  newz=0;
          if(newz>cmsh->edgez) newz=cmsh->edgez;
          cmsh->mm->vrt_z[vert] = newz;
          vert = cmsh->xvrt.next[vert];
          cnt++;
        }
      }
      x++;
    }
    y++;
  }
  return;
}

//------------------------------------------------------------------------------
void move_vert(cart_mesh * cmsh, int vert, int x, int y, int z)
{
  int newx, newy, newz;


  newx = cmsh->mm->vrt_x[vert]+x;
  newy = cmsh->mm->vrt_y[vert]+y;
  newz = cmsh->mm->vrt_z[vert]+z;


  if(newx<0)  newx=0;
  if(newx>cmsh->mi->edge_x)  newx=cmsh->mi->edge_x;
  if(newy<0)  newy=0;
  if(newy>cmsh->mi->edge_y)  newy=cmsh->mi->edge_y;
  if(newz<0)  newz=0;
  if(newz>cmsh->edgez)  newz=cmsh->edgez;


  cmsh->mm->vrt_x[vert]=newx;
  cmsh->mm->vrt_y[vert]=newy;
  cmsh->mm->vrt_z[vert]=newz;


  return;
}

//------------------------------------------------------------------------------
void raise_mesh(cart_mesh * cmsh, int x, int y, int amount, int size)
{
  int disx, disy, dis, cnt, newamount;
  Uint32 vert;


  cnt = 0;
  while(cnt < cpoint_lst.count)
  {
    vert = cpoint_lst.data[cnt];
    disx = cmsh->mm->vrt_x[vert]-(x/FOURNUM);
    disy = cmsh->mm->vrt_y[vert]-(y/FOURNUM);
    dis = sqrt(disx*disx+disy*disy);


    newamount = abs(amount)-((dis<<1)>>size);
    if(newamount < 0) newamount = 0;
    if(amount < 0)  newamount = -newamount;
    move_vert(cmsh, vert, 0, 0, newamount);

    cnt++;
  }


  return;
}

//------------------------------------------------------------------------------
void cart_load_module(cart_mesh * cmsh, char *modname)
{
  char newloadname[256];


  make_newloadname(".." SLASH_STRING "modules" SLASH_STRING "", modname, newloadname);
  //  show_name(newloadname);
  cart_load_basic_textures(newloadname);
  if(!mesh_load(cmsh, newloadname))
  {
    setup_mesh(cmsh);
  }
  clight_lst.count = 0;
  clight_lst.adding = 0;
  return;
}

//------------------------------------------------------------------------------
void render_tile_window(cart_window_info * w, cart_mesh * cmsh )
{
  GLtexture *bmptile;
  int x, y, xstt, ystt, cntx, cnty, numx, numy, mapx, mapy, mapxstt, mapystt;
  int cnt;


  mapxstt = (cart_pos_x-(w->rect.w>>1))/31;
  mapystt = (cart_pos_y-(w->rect.h>>1))/31;
  numx = (w->rect.w/SMALLX)+3;
  numy = (w->rect.h/SMALLY)+3;
  xstt = -((cart_pos_x-(w->rect.w>>1))%31)-(SMALLX);
  ystt = -((cart_pos_y-(w->rect.h>>1))%31)-(SMALLY);


  y = ystt;
  mapy = mapystt;
  cnty = 0;
  set_window_viewport( w );
  while(cnty < numy)
  {
    x = xstt;
    mapx = mapxstt;
    cntx = 0;
    while(cntx < numx)
    {
      FRect_t dst = {x, y, x + SMALLX, y + SMALLY};

      bmptile = tile_at(cmsh, mapx, mapy);

      draw_texture_box(bmptile, NULL, &dst);

      mapx++;
      cntx++;
      x+=31;
    }
    mapy++;
    cnty++;
    y+=31;
  }


  cnt = 0;
  while(cnt < clight_lst.count)
  {
    draw_light(w, &clight_lst, cnt);
    cnt++;
  }


  return;
}

//------------------------------------------------------------------------------
void render_fx_window(cart_window_info * w, cart_mesh * cmsh)
{
  GLtexture * bmptile;
  int x, y, xstt, ystt, cntx, cnty, numx, numy, mapx, mapy, mapxstt, mapystt;
  Uint32 fan;


  mapxstt = (cart_pos_x-(w->rect.w>>1))/31;
  mapystt = (cart_pos_y-(w->rect.h>>1))/31;
  numx = (w->rect.w/SMALLX)+3;
  numy = (w->rect.h/SMALLY)+3;
  xstt = -((cart_pos_x-(w->rect.w>>1))%31)-(SMALLX);
  ystt = -((cart_pos_y-(w->rect.h>>1))%31)-(SMALLY);

  set_window_viewport( w );

  y = ystt;
  mapy = mapystt;
  cnty = 0;
  while(cnty < numy)
  {
    x = xstt;
    mapx = mapxstt;
    cntx = 0;
    while(cntx < numx)
    {
      FRect_t dst = {x, y, SMALLX-1, SMALLY-1};
      bmptile = tile_at(cmsh, mapx, mapy);
      draw_texture_box(bmptile, NULL, &dst);

      fan = fan_at(cmsh, mapx, mapy);
      if(fan!=-1)
      {
        if( mesh_has_no_bits( cmsh->mm->tilelst, fan, MPDFX_NOREFLECT ) )
          draw_blit_sprite(w, &cimg_lst.ref, x, y);
        if( mesh_has_some_bits( cmsh->mm->tilelst, fan, MPDFX_SHINY ) )
          draw_blit_sprite(w, &cimg_lst.drawref, x+16, y);
        if( mesh_has_some_bits( cmsh->mm->tilelst, fan, MPDFX_ANIM ) )
          draw_blit_sprite(w, &cimg_lst.anim, x, y+16);
        if( mesh_has_some_bits( cmsh->mm->tilelst, fan, MPDFX_WALL ) )
          draw_blit_sprite(w, &cimg_lst.wall, x+15, y+15);
        if( mesh_has_some_bits( cmsh->mm->tilelst, fan, MPDFX_IMPASS ) )
          draw_blit_sprite(w, &cimg_lst.impass, x+15+8, y+15);
        if( mesh_has_some_bits( cmsh->mm->tilelst, fan, MPDFX_DAMAGE ) )
          draw_blit_sprite(w, &cimg_lst.damage, x+15, y+15+8);
        if( mesh_has_some_bits( cmsh->mm->tilelst, fan, MPDFX_SLIPPY ) )
          draw_blit_sprite(w, &cimg_lst.slippy, x+15+8, y+15+8);
        if( mesh_has_some_bits( cmsh->mm->tilelst, fan, MPDFX_WATER ) )
          draw_blit_sprite(w, &cimg_lst.water, x, y);
      }
      mapx++;
      cntx++;
      x+=31;
    }
    mapy++;
    cnty++;
    y+=31;
  }


  return;
}



//------------------------------------------------------------------------------
void render_vertex_window(cart_window_info * w, cart_mouse_info * m, cart_mesh * cmsh)
{
  int x, y, cntx, cnty, numx, numy, mapx, mapy, mapxstt, mapystt;
  Uint32 fan;


  //  cpoint_lst.count = 0;
  mapxstt = (cart_pos_x-(w->rect.w>>1))/31;
  mapystt = (cart_pos_y-(w->rect.h>>1))/31;
  numx = (w->rect.w/SMALLX)+3;
  numy = (w->rect.h/SMALLY)+3;
  x = -cart_pos_x+(w->rect.h>>1)-SMALLX;
  y = -cart_pos_y+(w->rect.h>>1)-SMALLY;


  mapy = mapystt;
  cnty = 0;
  while(cnty < numy)
  {
    if(mapy>=0 && mapy<cmsh->mi->tiles_y)
    {
      mapx = mapxstt;
      cntx = 0;
      while(cntx < numx)
      {
        if(mapx>=0 && mapx<cmsh->mi->tiles_x)
        {
          fan = mesh_convert_fan(cmsh->mi, mapx, mapy);
          draw_top_fan(cmsh->mm, w, fan, x, y);
        }
        mapx++;
        cntx++;
      }
    }
    mapy++;
    cnty++;
  }


  if(m->rect && m->mode==WINVERTEX)
  {
    draw_rect(w, make_color(16 + (ups_loops&15), 16 + (ups_loops&15), 0),
      (m->rectx/FOURNUM)+x, (m->recty/FOURNUM)+y,
      (m->x/FOURNUM)+x, (m->y/FOURNUM)+y );
  }


  if((SDLKEYDOWN(SDLK_p) || ((mous.latch.b&2) && cselect_lst.count == 0)) && m->mode==WINVERTEX)
  {
    raise_mesh(cmsh->mm, m->x, m->y, brushamount, brushsize);
  }



  return;
}



//------------------------------------------------------------------------------
void render_side_window(cart_window_info * w, cart_mouse_info * m, cart_mesh * cmsh)
{
  int x, y, cntx, cnty, numx, numy, mapx, mapy, mapxstt, mapystt;
  Uint32 fan;

  set_window_viewport( w );

  mapxstt = (cart_pos_x-(w->rect.w>>1))/31;
  mapystt = (cart_pos_y-(w->rect.h>>1))/31;
  numx = (w->rect.w/SMALLX)+3;
  numy = (w->rect.h/SMALLY)+3;
  x = -cart_pos_x+(w->rect.w>>1)-SMALLX;
  y = w->rect.h-10;


  mapy = mapystt;
  cnty = 0;
  while(cnty < numy)
  {
    if(mapy>=0 && mapy<cmsh->mi->tiles_y)
    {
      mapx = mapxstt;
      cntx = 0;
      while(cntx < numx)
      {
        if(mapx>=0 && mapx<cmsh->mi->tiles_x)
        {
          fan = mesh_convert_fan(cmsh->mi, mapx, mapy);
          draw_side_fan(cmsh->mm, w, fan, x, y);
        }
        mapx++;
        cntx++;
      }
    }
    mapy++;
    cnty++;
  }

  if(m->rect && m->mode==WINSIDE)
  {
    draw_rect(w, make_color(16 + (ups_loops&15), 16 + (ups_loops&15), 0),
      (m->rectx/FOURNUM)+x, (m->y/FOURNUM), (m->x/FOURNUM)+x, (m->recty/FOURNUM) );
  }


  return;
}

//------------------------------------------------------------------------------
void render_window(cart_window_info * w, cart_mesh * cmsh, cart_mouse_info * m)
{

  make_onscreen(cmsh);
  if(w->on)
  {
    if(w->mode&WINTILE)
    {
      render_tile_window(w, cmsh);
    }
    else
    {
      // Untiled bitmaps clear
      clear_to_color(w, make_color(0, 0, 0));
    }

    if(w->mode&WINFX)
    {
      render_fx_window(w, cmsh);
    }

    if(w->mode&WINVERTEX)
    {
      render_vertex_window(w, m, cmsh);
    }

    if(w->mode&WINSIDE)
    {
      render_side_window(w, m, cmsh);
    }

    draw_cursor_in_window(m, w);
  }

  return;
}

//------------------------------------------------------------------------------
void load_window(cart_window_info * w, char *loadname, int x, int y, int bx, int by,
                 int sx, int sy, Uint16 mode)
{
  GLtexture_Load(GL_TEXTURE_2D, &(w->tx), loadname, -1);
  w->borderx = bx;
  w->bordery = by;
  w->rect.x = x;
  w->rect.y = y;
  w->rect.w = sx;
  w->rect.h = sy;
  w->on = btrue;
  w->mode = mode;
  return;
}

//------------------------------------------------------------------------------
void render_all_windows(cart_mesh * cmsh, cart_mouse_info * m)
{
  int cnt;
  cnt = 0;
  while(cnt < MAXWIN)
  {
    render_window(cwindow_info + cnt, cmsh, m);
    cnt++;
  }
  return;
}

//------------------------------------------------------------------------------
void load_all_windows(void)
{
  int cnt;
  cnt = 0;
  while(cnt < MAXWIN)
  {
    cwindow_info[cnt].on = bfalse;
    cnt++;
  }

  load_window(cwindow_info + 0, "window.pcx", 180, 16,  7, 9, 200, 200, WINVERTEX);
  load_window(cwindow_info + 1, "window.pcx", 410, 16,  7, 9, 200, 200, WINTILE  );
  load_window(cwindow_info + 2, "window.pcx", 180, 248, 7, 9, 200, 200, WINSIDE  );
  load_window(cwindow_info + 3, "window.pcx", 410, 248, 7, 9, 200, 200, WINFX    );
}

//------------------------------------------------------------------------------
void draw_window(cart_window_info * w)
{
  FRect_t dst;

  if(!w->on) return;

  dst.left   = w->rect.x;
  dst.top    = w->rect.y;
  dst.right  = w->tx.txH + dst.left;
  dst.bottom = w->tx.txH + dst.top;

  draw_texture_box( &(w->tx), NULL, &dst);
}


//------------------------------------------------------------------------------
void draw_all_windows(void)
{
  int cnt;
  cnt = 0;
  while(cnt < MAXWIN)
  {
    draw_window(cwindow_info + cnt);
    cnt++;
  }
  return;
}

//------------------------------------------------------------------------------
void bound_camera(cart_mesh * cmsh)
{
  if(cart_pos_x < 0)
  {
    cart_pos_x = 0;
  }
  if(cart_pos_y < 0)
  {
    cart_pos_y = 0;
  }
  if(cart_pos_x > cmsh->mi->tiles_x*SMALLX)
  {
    cart_pos_x = cmsh->mi->tiles_x*SMALLX;
  }
  if(cart_pos_y > cmsh->mi->tiles_y*SMALLY)
  {
    cart_pos_y = cmsh->mi->tiles_y*SMALLY;
  }
  return;
}

//------------------------------------------------------------------------------
void rect_select(MeshMem_t * mm, cart_select_list * csel, cart_mouse_info * m)
{
  // ZZ> This function checks the rectangular selection

  int cnt;
  Uint32 vert;
  int tlx, tly, brx, bry;
  int y;

  if(m->mode == WINVERTEX)
  {
    tlx = m->rectx/FOURNUM;
    brx = m->x/FOURNUM;
    tly = m->recty/FOURNUM;
    bry = m->y/FOURNUM;


    if(tlx>brx)  { cnt = tlx;  tlx=brx;  brx=cnt; }
    if(tly>bry)  { cnt = tly;  tly=bry;  bry=cnt; }


    cnt = 0;
    while(cnt < cpoint_lst.count && csel->count<MAXSELECT)
    {
      vert = cpoint_lst.data[cnt];
      if(mm->vrt_x[vert]>=tlx &&
        mm->vrt_x[vert]<=brx &&
        mm->vrt_y[vert]>=tly &&
        mm->vrt_y[vert]<=bry)
      {
        add_select(csel, vert);
      }
      cnt++;
    }
  }
  if(m->mode == WINSIDE)
  {
    tlx = m->rectx/FOURNUM;
    brx = m->x/FOURNUM;
    tly = m->recty/FOURNUM;
    bry = m->y/FOURNUM;

    y = 190;//((*(cwindow_info[m->data].tx)).txH-10);


    if(tlx>brx)  { cnt = tlx;  tlx=brx;  brx=cnt; }
    if(tly>bry)  { cnt = tly;  tly=bry;  bry=cnt; }


    cnt = 0;
    while(cnt < cpoint_lst.count && csel->count<MAXSELECT)
    {
      vert = cpoint_lst.data[cnt];
      if(mm->vrt_x[vert]>=tlx &&
        mm->vrt_x[vert]<=brx &&
        -(mm->vrt_z[vert] / 16.0f)+y>=tly &&
        -(mm->vrt_z[vert] / 16.0f)+y<=bry)
      {
        add_select(csel, vert);
      }
      cnt++;
    }
  }
}

//------------------------------------------------------------------------------
void rect_unselect(MeshMem_t * mm, cart_select_list * csel, cart_mouse_info * m)
{
  // ZZ> This function checks the rectangular selection, and removes any fans
  //     in the selection area

  int cnt;
  Uint32 vert;
  int tlx, tly, brx, bry;
  int y;

  if(m->mode == WINVERTEX)
  {
    tlx = m->rectx/FOURNUM;
    brx = m->x/FOURNUM;
    tly = m->recty/FOURNUM;
    bry = m->y/FOURNUM;


    if(tlx>brx)  { cnt = tlx;  tlx=brx;  brx=cnt; }
    if(tly>bry)  { cnt = tly;  tly=bry;  bry=cnt; }


    cnt = 0;
    while(cnt < cpoint_lst.count && csel->count<MAXSELECT)
    {
      vert = cpoint_lst.data[cnt];
      if(mm->vrt_x[vert]>=tlx &&
        mm->vrt_x[vert]<=brx &&
        mm->vrt_y[vert]>=tly &&
        mm->vrt_y[vert]<=bry)
      {
        remove_select(csel, vert);
      }
      cnt++;
    }
  }
  if(m->mode == WINSIDE)
  {
    tlx = m->rectx/FOURNUM;
    brx = m->x/FOURNUM;
    tly = m->recty/FOURNUM;
    bry = m->y/FOURNUM;

    y = 190;//((*(cwindow_info[m->data].tx)).txH-10);


    if(tlx>brx)  { cnt = tlx;  tlx=brx;  brx=cnt; }
    if(tly>bry)  { cnt = tly;  tly=bry;  bry=cnt; }


    cnt = 0;
    while(cnt < cpoint_lst.count && csel->count<MAXSELECT)
    {
      vert = cpoint_lst.data[cnt];
      if(mm->vrt_x[vert]>=tlx &&
        mm->vrt_x[vert]<=brx &&
        -(mm->vrt_z[vert] / 16.0f)+y>=tly &&
        -(mm->vrt_z[vert] / 16.0f)+y<=bry)
      {
        remove_select(csel, vert);
      }
      cnt++;
    }
  }
}

//------------------------------------------------------------------------------
int set_vrta(cart_mesh * cmsh, Uint32 vert)
{
  int newa, x, y, z, brx, bry, brz, deltaz, dist, cnt;
  int newlevel, distance, disx, disy;
  vect3 pos;


  // To make life easier
  x = cmsh->mm->vrt_x[vert]*FOURNUM;
  y = cmsh->mm->vrt_y[vert]*FOURNUM;
  z = cmsh->mm->vrt_z[vert];

  pos.x = x;
  pos.y = y;
  pos.z = 0;


  // Directional light
  brx = x+64;
  bry = y+64;
  brz = mesh_get_level( cmsh->mm, mesh_get_fan(cmsh->mm, cmsh->mi, pos), brx, y, bfalse, NULL) +
        mesh_get_level( cmsh->mm, mesh_get_fan(cmsh->mm, cmsh->mi, pos), x, bry, bfalse, NULL) +
        mesh_get_level( cmsh->mm, mesh_get_fan(cmsh->mm, cmsh->mi, pos), x+46, y+46, bfalse, NULL);
  if(z < -128) z = -128;
  if(brz < -128) brz = -128;
  deltaz = z+z+z-brz;
  newa = (deltaz*direct>>8);


  // Point lights !!!BAD!!!
  newlevel = 0;
  cnt = 0;
  while(cnt < clight_lst.count)
  {
    disx = x-clight_lst.x[cnt];
    disy = y-clight_lst.y[cnt];
    distance = sqrt(disx*disx + disy*disy);
    if(distance < clight_lst.radius[cnt])
    {
      newlevel += ((clight_lst.level[cnt]*(clight_lst.radius[cnt]-distance))/clight_lst.radius[cnt]);
    }
    cnt++;
  }
  newa += newlevel;



  // Bounds
  if(newa < -ambicut) newa = -ambicut;
  newa+=ambi;
  if(newa <= 0) newa = 1;
  if(newa > 255) newa = 255;
  cmsh->mm->vrt_ar_fp8[vert]=newa;



  // Edge fade
  dist = dist_from_border(cmsh, cmsh->mm->vrt_x[vert], cmsh->mm->vrt_y[vert]);
  if(dist <= FADEBORDER)
  {
    newa = newa*dist/FADEBORDER;
    if(newa==VERTEXUNUSED)  newa=1;
    cmsh->mm->vrt_ar_fp8[vert]=newa;
  }


  return 0;
}

//------------------------------------------------------------------------------
void calc_vrta(cart_mesh * cmsh)
{
  int x, y, fan, num, cnt;
  Uint32 vert;


  y = 0;
  while(y < cmsh->mi->tiles_y)
  {
    x = 0;
    while(x < cmsh->mi->tiles_x)
    {
      fan = mesh_convert_fan(cmsh->mi, x, y);
      vert = cmsh->mm->tilelst[fan].vrt_start;
      num = pmesh->TileDict[cmsh->mm->tilelst[fan].type].vrt_count;
      cnt = 0;
      while(cnt < num)
      {
        set_vrta(cmsh, vert);
        vert = cmsh->xvrt.next[vert];
        cnt++;
      }
      x++;
    }
    y++;
  }
  return;
}

//------------------------------------------------------------------------------
void level_vrtz(cart_mesh * cmsh)
{
  int x, y, fan, num, cnt;
  Uint32 vert;


  y = 0;
  while(y < cmsh->mi->tiles_y)
  {
    x = 0;
    while(x < cmsh->mi->tiles_x)
    {
      fan = mesh_convert_fan(cmsh->mi, x, y);
      vert = cmsh->mm->tilelst[fan].vrt_start;
      num = pmesh->TileDict[cmsh->mm->tilelst[fan].type].vrt_count;
      cnt = 0;
      while(cnt < num)
      {
        cmsh->mm->vrt_z[vert] = 0;
        vert = cmsh->xvrt.next[vert];
        cnt++;
      }
      x++;
    }
    y++;
  }
  return;
}

//------------------------------------------------------------------------------
void jitter_select(cart_mesh * cmsh, cart_select_list * csel)
{
  int cnt;
  Uint32 vert;

  cnt = 0;
  while(cnt < csel->count)
  {
    vert = csel->data[cnt];
    move_vert(cmsh, vert, IRAND(&(cmsh->seed), 2)-1, IRAND(&(cmsh->seed), 2)-1, 0);
    cnt++;
  }
  return;
}

//------------------------------------------------------------------------------
void jitter_mesh(cart_mesh * cmsh)
{
  cart_select_list tsel;

  int x, y, fan, num, cnt;
  Uint32 vert;


  y = 0;
  while(y < cmsh->mi->tiles_y)
  {
    x = 0;
    while(x < cmsh->mi->tiles_x)
    {
      fan = mesh_convert_fan(cmsh->mi, x, y);
      vert = cmsh->mm->tilelst[fan].vrt_start;
      num = pmesh->TileDict[cmsh->mm->tilelst[fan].type].vrt_count;
      cnt = 0;
      while(cnt < num)
      {
        clear_select(&tsel);
        add_select(&tsel, vert);

        //        srand(cmsh->mm->vrt_x[vert]+cmsh->mm->vrt_y[vert]+dunframe);
        move_select(cmsh, &tsel, IRAND(&(cmsh->seed), 3)-3, IRAND(&(cmsh->seed), 3)-3, IRAND(&(cmsh->seed), 6)-32);
        vert = cmsh->xvrt.next[vert];
        cnt++;
      }
      x++;
    }
    y++;
  }
  clear_select(&tsel);
  return;
}

//------------------------------------------------------------------------------
void flatten_mesh(cart_mesh * cmsh, cart_mouse_info * m)
{
  int x, y, fan, num, cnt;
  Uint32 vert;
  int height;


  height = (780 - m->y) * 4;
  if(height < 0)  height = 0;
  if(height > cmsh->edgez) height = cmsh->edgez;
  y = 0;
  while(y < cmsh->mi->tiles_y)
  {
    x = 0;
    while(x < cmsh->mi->tiles_x)
    {
      fan = mesh_convert_fan(cmsh->mi, x, y);
      vert = cmsh->mm->tilelst[fan].vrt_start;
      num = pmesh->TileDict[cmsh->mm->tilelst[fan].type].vrt_count;
      cnt = 0;
      while(cnt < num)
      {
        if(cmsh->mm->vrt_z[vert] > height - 50)
          if(cmsh->mm->vrt_z[vert] < height + 50)
            cmsh->mm->vrt_z[vert] = height;
        vert = cmsh->xvrt.next[vert];
        cnt++;
      }
      x++;
    }
    y++;
  }
  clear_select(&cselect_lst);
  return;
}


//------------------------------------------------------------------------------
void clear_mesh(cart_mesh * cmsh, cart_mouse_info * m)
{
  int x, y;
  Uint32 fan;


  if(INVALID_FAN != m->tile)
  {
    y = 0;
    while(y < cmsh->mi->tiles_y)
    {
      x = 0;
      while(x < cmsh->mi->tiles_x)
      {
        fan = mesh_convert_fan(cmsh->mi, x, y);
        remove_fan(cmsh->mm, fan);
        switch(m->presser)
        {
        case 0:
          cmsh->mm->tilelst[fan].tile=m->tile;
          break;
        case 1:
          cmsh->mm->tilelst[fan].tile = (m->tile&0xfffe) + IRAND(&(cmsh->seed), 1);
          break;
        case 2:
          if(m->type >= 32)
            cmsh->mm->tilelst[fan].tile = (m->tile&0xfff8)+(rand()&6);
          else
            cmsh->mm->tilelst[fan].tile = (m->tile&0xfffc)+IRAND(&(cmsh->seed), 2);
          break;
        case 3:
          cmsh->mm->tilelst[fan].tile=(m->tile&0xfff0)+(rand()&6);
          break;
        }
        cmsh->mm->tilelst[fan].type=m->type;
        if(m->type<=1) cmsh->mm->tilelst[fan].type = IRAND(&(cmsh->seed), 1);
        if(m->type == 32 || m->type == 33)
          cmsh->mm->tilelst[fan].type = 32 + IRAND(&(cmsh->seed), 1);
        add_fan(cmsh->mm, fan, x*31, y*31);
        x++;
      }
      y++;
    }
  }
  return;
}

//------------------------------------------------------------------------------
void three_e_mesh(cart_mesh * cmsh, cart_mouse_info * m)
{
  // ZZ> Replace all 3F tiles with 3E tiles...

  int x, y;
  Uint32 fan;

  if(INVALID_FAN != m->tile)
  {
    y = 0;
    while(y < cmsh->mi->tiles_y)
    {
      x = 0;
      while(x < cmsh->mi->tiles_x)
      {
        fan = mesh_convert_fan(cmsh->mi, x, y);
        if(cmsh->mm->tilelst[fan].tile==0x3F)  cmsh->mm->tilelst[fan].tile=0x3E;
        x++;
      }
      y++;
    }
  }
  return;
}

//------------------------------------------------------------------------------
void toggle_fx(cart_mouse_info * m, int fxmask)
{
  m->fx ^= fxmask;
}

//------------------------------------------------------------------------------
void ease_up_mesh(cart_mesh * cmsh, int zadd)
{
  // ZZ> This function lifts the entire mesh

  int x, y, cnt;
  Uint32 fan, vert;

  y = 0;
  while(y < cmsh->mi->tiles_y)
  {
    x = 0;
    while(x < cmsh->mi->tiles_x)
    {
      fan = mesh_convert_fan(cmsh->mi, x, y);
      vert = cmsh->mm->tilelst[fan].vrt_start;
      cnt = 0;
      while(cnt < pmesh->TileDict[cmsh->mm->tilelst[fan].type].vrt_count)
      {
        move_vert(cmsh, vert, 0, 0, zadd);
        vert = cmsh->xvrt.next[vert];
        cnt++;
      }
      x++;
    }
    y++;
  }
}

//------------------------------------------------------------------------------
void select_connected(cart_mesh * cmsh, cart_select_list * csel)
{
  int vert, cnt, tnc, x, y, totalvert;
  Uint32 fan;
  Uint8 found, selectfan;


  y = 0;
  while(y < cmsh->mi->tiles_y)
  {
    x = 0;
    while(x < cmsh->mi->tiles_x)
    {
      fan = mesh_convert_fan(cmsh->mi, x, y);
      selectfan = bfalse;
      totalvert = pmesh->TileDict[cmsh->mm->tilelst[fan].type].vrt_count;
      cnt = 0;
      vert = cmsh->mm->tilelst[fan].vrt_start;
      while(cnt < totalvert)
      {

        found = bfalse;
        tnc = 0;
        while(tnc < csel->count && !found)
        {
          if(csel->data[tnc]==vert)
          {
            found=btrue;
          }
          tnc++;
        }
        if(found) selectfan = btrue;
        vert = cmsh->xvrt.next[vert];
        cnt++;
      }
      if(selectfan)
      {
        cnt = 0;
        vert = cmsh->mm->tilelst[fan].vrt_start;
        while(cnt < totalvert)
        {
          add_select(csel, vert);
          vert = cmsh->xvrt.next[vert];
          cnt++;
        }
      }
      x++;
    }
    y++;
  }
}


//------------------------------------------------------------------------------
void check_keys(cart_mesh * cmsh, cart_mouse_info * m, char *modname)
{
  char newloadname[256];

  if(keydelay <= 0)
  {
    // Hurt
    if(SDLKEYDOWN(SDLK_h))
    {
      toggle_fx(m, MPDFX_DAMAGE);
      keydelay=KEYDELAY;
    }
    // Impassable
    if(SDLKEYDOWN(SDLK_i))
    {
      toggle_fx(m, MPDFX_IMPASS);
      keydelay=KEYDELAY;
    }
    // Barrier
    if(SDLKEYDOWN(SDLK_b))
    {
      toggle_fx(m, MPDFX_WALL);
      keydelay=KEYDELAY;
    }
    // Overlay
    if(SDLKEYDOWN(SDLK_o))
    {
      toggle_fx(m, MPDFX_WATER);
      keydelay=KEYDELAY;
    }
    // Reflective
    if(SDLKEYDOWN(SDLK_r))
    {
      toggle_fx(m, MPDFX_NOREFLECT);
      keydelay=KEYDELAY;
    }
    // Draw reflections
    if(SDLKEYDOWN(SDLK_d))
    {
      toggle_fx(m, MPDFX_SHINY);
      keydelay=KEYDELAY;
    }
    // Animated
    if(SDLKEYDOWN(SDLK_a))
    {
      toggle_fx(m, MPDFX_ANIM);
      keydelay=KEYDELAY;
    }
    // Slippy
    if(SDLKEYDOWN(SDLK_s))
    {
      toggle_fx(m, MPDFX_SLIPPY);
      keydelay=KEYDELAY;
    }
    if(SDLKEYDOWN(SDLK_g))
    {
      fix_mesh(cmsh);
      keydelay=KEYDELAY;
    }
    if(SDLKEYDOWN(SDLK_z))
    {
      set_mesh_tile(cmsh, m, cmsh->mm->tilelst[m->onfan].tile);
      keydelay=KEYDELAY;
    }
    if(SDLKEYDOWN(SDLK_LSHIFT))
    {
      if(cmsh->mm->tilelst[m->onfan].type >= (MAXMESHTYPE>>1))
      {
        fx_mesh_tile(cmsh, cmsh->mm->tilelst[m->onfan].tile, 192, m->fx);
      }
      else
      {
        fx_mesh_tile(cmsh, cmsh->mm->tilelst[m->onfan].tile, 240, m->fx);
      }
      keydelay=KEYDELAY;
    }
    if(SDLKEYDOWN(SDLK_x))
    {
      if(cmsh->mm->tilelst[m->onfan].type >= (MAXMESHTYPE>>1))
      {
        trim_mesh_tile(cmsh, cmsh->mm->tilelst[m->onfan].tile, 192);
      }
      else
      {
        trim_mesh_tile(cmsh, cmsh->mm->tilelst[m->onfan].tile, 240);
      }
      keydelay=KEYDELAY;
    }
    if(SDLKEYDOWN(SDLK_e))
    {
      ease_up_mesh(cmsh, 10);
    }
    if(SDLKEYDOWN(SDLK_c))
    {
      clear_mesh(cmsh, m);
      keydelay=KEYDELAY;
    }
    if(SDLKEYDOWN(SDLK_LEFTBRACKET) || SDLKEYDOWN(SDLK_RIGHTBRACKET))
    {
      select_connected(cmsh, &cselect_lst);
    }
    if(SDLKEYDOWN(SDLK_8))
    {
      three_e_mesh(cmsh, m);
      keydelay=KEYDELAY;
    }
    if(SDLKEYDOWN(SDLK_j))
    {
      if(cselect_lst.count == 0) { jitter_mesh(&cselect_lst); }
      else { jitter_select(cmsh, &cselect_lst); }
      keydelay=KEYDELAY;
    }
    if(SDLKEYDOWN(SDLK_l))
    {
      level_vrtz(cmsh);
    }
    if(SDLKEYDOWN(SDLK_w))
    {
      impass_edges(cmsh, 2);
      calc_vrta(cmsh);
      make_newloadname(".." SLASH_STRING "modules" SLASH_STRING "", modname, newloadname);
      save_mesh(cmsh, newloadname);
      keydelay=KEYDELAY;
    }
    if(SDLKEYDOWN(SDLK_SPACE))
    {
      weld_select(cmsh->mm, &cselect_lst);
      keydelay=KEYDELAY;
    }
    if(SDLKEYDOWN(SDLK_INSERT))
    {
      m->type = (m->type-1) % MAXMESHTYPE;
      while(cfan_lines[m->type].numline==0)
      {
        m->type = (m->type-1) % MAXMESHTYPE;
      }
      keydelay=KEYDELAY;
    }
    if(SDLKEYDOWN(SDLK_BACKSPACE))
    {
      m->type = (m->type+1) % MAXMESHTYPE;
      while(cfan_lines[m->type].numline==0)
      {
        m->type = (m->type+1) % MAXMESHTYPE;
      }
      keydelay=KEYDELAY;
    }
    if(SDLKEYDOWN(SDLK_KP_PLUS))
    {
      m->tile=(m->tile+1)&255;
      keydelay=KEYDELAY;
    }
    if(SDLKEYDOWN(SDLK_KP_MINUS))
    {
      m->tile=(m->tile-1)&255;
      keydelay=KEYDELAY;
    }
    if(SDLKEYDOWN(SDLK_UP) || SDLKEYDOWN(SDLK_LEFT) || SDLKEYDOWN(SDLK_DOWN) || SDLKEYDOWN(SDLK_RIGHT))
    {
      if(SDLKEYDOWN(SDLK_UP))
      {
        cart_pos_y-=CAMRATE;
      }
      if(SDLKEYDOWN(SDLK_LEFT))
      {
        cart_pos_x-=CAMRATE;
      }
      if(SDLKEYDOWN(SDLK_DOWN))
      {
        cart_pos_y+=CAMRATE;
      }
      if(SDLKEYDOWN(SDLK_RIGHT))
      {
        cart_pos_x+=CAMRATE;
      }
    }
    if(SDLKEYDOWN(SDLK_END))
    {
      brushsize = 0;
    }
    if(SDLKEYDOWN(SDLK_PAGEDOWN))
    {
      brushsize = 1;
    }
    if(SDLKEYDOWN(SDLK_HOME))
    {
      brushsize = 2;
    }
    if(SDLKEYDOWN(SDLK_PAGEUP))
    {
      brushsize = 3;
    }
    if(SDLKEYDOWN(SDLK_1))
    {
      m->presser = 0;
    }
    if(SDLKEYDOWN(SDLK_2))
    {
      m->presser = 1;
    }
    if(SDLKEYDOWN(SDLK_3))
    {
      m->presser = 2;
    }
    if(SDLKEYDOWN(SDLK_4))
    {
      m->presser = 3;
    }
  }


  return;
}

//------------------------------------------------------------------------------
//void setup_screen(void)
//{
//  set_color_depth(colordepth);
//  set_gfx_mode(GFX_AUTODETECT, SCRX, SCRY, SCRX, SCRY);
//  set_palette(goodpal);
//  clear(screen);
//  cbmp_lst.dbuff = SDL_CreateRGBSurface(SDL_SWSURFACE, OUTX, OUTY, 32, rmask, gmask, bmask, amask);
//  clear(cbmp_lst.dbuff);
//  text_mode(-1);
//
//
//  return;
//}

//------------------------------------------------------------------------------
//void create_imgcursor(void)
//{
//  int x, y;
//  Uint8 col, loc;
//
//  col = make_color(31, 31, 31);      // White color
//  loc = make_color(3, 3, 3);        // Gray color
//  bmp_temp = SDL_CreateRGBSurface(SDL_SWSURFACE, 8, 8, 32, rmask, gmask, bmask, amask);
//
//  // Simple triangle
//  draw_line(NULL, loc, 0, 0, 0, 7, );
//  y = 1;
//  while(y < 8)
//  {
//    _putpixel(bmp_temp, 0, y, loc);
//    x = 1;
//    while(x < 8)
//    {
//      if(x < 8-y) _putpixel(bmp_temp, x, y, col);
//      else _putpixel(bmp_temp, x, y, 0);
//      x++;
//    }
//    y++;
//  }
//  cimg_lst.cursor = get_rle_sprite(bmp_temp);
//  GLtexture_Release(&bmp_temp);
//
//
//  return;
//}

//------------------------------------------------------------------------------
void load_img(void)
{
  int cnt;
  SDL_Surface *bmp_other, *bmp_temp;


  bmp_temp = IMG_Load("point.pcx");
  cnt = 0;
  while(cnt < 16)
  {
    bmp_other = SDL_CreateRGBSurface(SDL_SWSURFACE, (cnt>>1)+4, ((cnt+1)>>1)+4, 32, rmask, gmask, bmask, amask);
    SDL_BlitSurface(bmp_temp, &bmp_temp->clip_rect, bmp_other, &bmp_other->clip_rect);

    GLtexture_Convert( GL_TEXTURE_2D, cimg_lst.point + cnt, bmp_other, INVALID_KEY);

    SDL_FreeSurface( bmp_other );

    cnt++;
  }
  SDL_FreeSurface(bmp_temp);

  bmp_temp = IMG_Load("pointon.pcx");
  cnt = 0;
  while(cnt < 16)
  {
    bmp_other = SDL_CreateRGBSurface(SDL_SWSURFACE, (cnt>>1)+4, ((cnt+1)>>1)+4, 32, rmask, gmask, bmask, amask);
    SDL_BlitSurface(bmp_temp, &bmp_temp->clip_rect, bmp_other, &bmp_other->clip_rect);

    GLtexture_Convert( GL_TEXTURE_2D, cimg_lst.pointon + cnt , bmp_other, INVALID_KEY);

    SDL_FreeSurface( bmp_other );

    cnt++;
  }
  SDL_FreeSurface( bmp_temp );

  GLtexture_Load( GL_TEXTURE_2D, &cimg_lst.ref,     "ref.pcx",     INVALID_KEY);
  GLtexture_Load( GL_TEXTURE_2D, &cimg_lst.drawref, "drawref.pcx", INVALID_KEY);
  GLtexture_Load( GL_TEXTURE_2D, &cimg_lst.anim,    "anim.pcx",    INVALID_KEY);
  GLtexture_Load( GL_TEXTURE_2D, &cimg_lst.water,   "Water.pcx",   INVALID_KEY);
  GLtexture_Load( GL_TEXTURE_2D, &cimg_lst.wall,    "slit.pcx",    INVALID_KEY);
  GLtexture_Load( GL_TEXTURE_2D, &cimg_lst.impass,  "impass.pcx",  INVALID_KEY);
  GLtexture_Load( GL_TEXTURE_2D, &cimg_lst.damage,  "damage.pcx",  INVALID_KEY);
  GLtexture_Load( GL_TEXTURE_2D, &cimg_lst.slippy,  "slippy.pcx",  INVALID_KEY);

  return;
}

//------------------------------------------------------------------------------
void draw_lotsa_stuff(cart_mouse_info * m, cart_mesh *cmsh)
{
  int x, y, cnt, todo, tile, add;


  // Tell which tile we're in
  x = debugx * 128;
  y = debugy * 128;
  draw_string( ui_getBMFont(),  0, 226, make_color(31, 31, 31), "X = %6.2f (%d)", debugx, x);
  draw_string( ui_getBMFont(),  0, 234, make_color(31, 31, 31), "Y = %6.2f (%d)", debugy, y);


  // Tell user what keys are important
  draw_string( ui_getBMFont(),  0, OUTY-120, make_color(31, 31, 31), "O = Overlay (Water)");
  draw_string( ui_getBMFont(),  0, OUTY-112, make_color(31, 31, 31), "R = Reflective");
  draw_string( ui_getBMFont(),  0, OUTY-104, make_color(31, 31, 31), "D = Draw Reflection");
  draw_string( ui_getBMFont(),  0, OUTY- 96, make_color(31, 31, 31), "A = Animated");
  draw_string( ui_getBMFont(),  0, OUTY- 88, make_color(31, 31, 31), "B = Barrier (Slit)");
  draw_string( ui_getBMFont(),  0, OUTY- 80, make_color(31, 31, 31), "I = Impassable (Wall)");
  draw_string( ui_getBMFont(),  0, OUTY- 72, make_color(31, 31, 31), "H = Hurt");
  draw_string( ui_getBMFont(),  0, OUTY- 64, make_color(31, 31, 31), "S = Slippy");


  // Vertices left
  draw_string( ui_getBMFont(),  0, OUTY-56, make_color(31, 31, 31), "Vertices %d", cmsh->xvrt.free_count);


  // Misc data
  draw_string( ui_getBMFont(),  0, OUTY-40, make_color(31, 31, 31), "Ambient   %d", ambi);
  draw_string( ui_getBMFont(),  0, OUTY-32, make_color(31, 31, 31), "Ambicut   %d", ambicut);
  draw_string( ui_getBMFont(),  0, OUTY-24, make_color(31, 31, 31), "Direct    %d", direct);
  draw_string( ui_getBMFont(),  0, OUTY-16, make_color(31, 31, 31), "Brush amount %d", brushamount);
  draw_string( ui_getBMFont(),  0, OUTY-8,  make_color(31, 31, 31), "Brush size   %d", brushsize);


  // Cursor
  draw_blit_sprite(NULL, &cimg_lst.cursor, ui_getMouseX(), ui_getMouseY());


  // Tile picks
  if(m->tile<=MAXTILE)
  {
    switch(m->presser)
    {
    case 0:
      todo = 1;
      tile = m->tile;
      add = 1;
      break;
    case 1:
      todo = 2;
      tile = m->tile&0xfffe;
      add = 1;
      break;
    case 2:
      todo = 4;
      tile = m->tile&0xfffc;
      add = 1;
      break;
    case 3:
      todo = 4;
      tile = m->tile&0xfff0;
      add = 2;
      break;
    }

    x = 0;
    cnt = 0;
    while(cnt < todo)
    {

      if(m->type&32)
      {
        FRect_t dst = {x, 0, x + SMALLX, SMALLY};
        draw_texture_box(cbmp_lst.bigtile + tile, NULL, &dst);
      }
      else
      {
        FRect_t dst = {x, 0, x + SMALLX, SMALLY};
        draw_texture_box(cbmp_lst.smalltile + tile, NULL, &dst);
      }

      x+=SMALLX;
      tile+=add;
      cnt++;
    }
    draw_string( ui_getBMFont(),  0, 32, make_color(31, 31, 31), "Tile 0x%02x", m->tile);
    draw_string( ui_getBMFont(),  0, 40, make_color(31, 31, 31), "Eats %d verts", pmesh->TileDict[m->type].vrt_count);
    if(m->type>=MAXMESHTYPE/2)
    {
      draw_string( ui_getBMFont(),  0, 56, make_color(31, 16, 16), "63x63 Tile");
    }
    else
    {
      draw_string( ui_getBMFont(),  0, 56, make_color(16, 16, 31), "31x31 Tile");
    }

    draw_schematic(cmsh->mm, m, NULL, m->type, 0, 64);
  }

  // FX selection
  if( HAS_NO_BITS( m->fx, MPDFX_NOREFLECT ) )
    draw_blit_sprite(NULL, &cimg_lst.ref, 0, 200);

  if( HAS_SOME_BITS( m->fx, MPDFX_SHINY ) )
    draw_blit_sprite(NULL, &cimg_lst.drawref, 16, 200);

  if( HAS_SOME_BITS( m->fx, MPDFX_ANIM ) )
    draw_blit_sprite(NULL, &cimg_lst.anim, 0, 216);

  if( HAS_SOME_BITS( m->fx, MPDFX_WALL ) )
    draw_blit_sprite(NULL, &cimg_lst.wall, 15, 215);

  if( HAS_SOME_BITS( m->fx, MPDFX_IMPASS ) )
    draw_blit_sprite(NULL, &cimg_lst.impass, 15+8, 215);

  if( HAS_SOME_BITS( m->fx, MPDFX_DAMAGE ) )
    draw_blit_sprite(NULL, &cimg_lst.damage, 15, 215+8);

  if( HAS_SOME_BITS( m->fx, MPDFX_SLIPPY ) )
    draw_blit_sprite(NULL, &cimg_lst.slippy, 15+8, 215+8);

  if( HAS_SOME_BITS( m->fx, MPDFX_WATER ) )
    draw_blit_sprite(NULL, &cimg_lst.water, 0, 200);


  if(numattempt > 0)
  {
    draw_string( ui_getBMFont(),  0, 0, make_color(31, 31, 31), "numwritten %d/%d", numwritten, numattempt);
  }


  // Write double buffer to screen
  SDL_GL_SwapBuffers();


  return;
}


//------------------------------------------------------------------------------
void draw_slider(int tlx, int tly, int brx, int bry, int* pvalue,
                 int minvalue, int maxvalue)
{
  int cnt;
  int value;


  // Pick a new value
  value = *pvalue;
  if(ui_getMouseX() >= tlx && ui_getMouseX() <= brx && ui_getMouseY() >= tly && ui_getMouseY() <= bry && mous.latch.b)
  {
    value = (((ui_getMouseY() - tly)*(maxvalue-minvalue))/(bry - tly)) + minvalue;
  }
  if(value < minvalue) value = minvalue;
  if(value > maxvalue) value = maxvalue;
  *pvalue = value;


  // Draw it
  if(maxvalue != 0)
  {
    cnt = ((value-minvalue)*20/(maxvalue-minvalue))+11;

    draw_line(NULL, make_color(cnt, cnt, cnt), tlx, (((value-minvalue)*(bry-tly)/(maxvalue-minvalue)))+tly, brx, (((value-minvalue)*(bry-tly)/(maxvalue-minvalue)))+tly);
  }

  draw_rect(NULL, make_color(31, 31, 31), tlx, tly, brx, bry );
}

//------------------------------------------------------------------------------
void cart_draw_main(void)
{
  do_clear();

  draw_all_windows();
  draw_slider( 0, 250, 19, 350, &ambi,          0, 200);
  draw_slider(20, 250, 39, 350, &ambicut,       0, ambi);
  draw_slider(40, 250, 59, 350, &direct,        0, 100);
  draw_slider(60, 250, 79, 350, &brushamount, -50,  50);
  draw_lotsa_stuff(&cmouse_info, &c_mesh);

  return;
}

//------------------------------------------------------------------------------
void show_info(void)
{
  log_info("%s - Version %01d.%02d\n", CARTMAN_NAME, CARTMAN_VERSION/100, CARTMAN_VERSION%100);
  return;
}

//------------------------------------------------------------------------------
int cartman(char * modulename)
{
  show_info();            // Text title

  load_all_windows();          // Load windows
  load_img();            // Load other images
  TileDictionary_load();          // Get fan data
  cart_load_module(&c_mesh, modulename);        // Load the module
  render_all_windows(&c_mesh, &cmouse_info);          // Create basic windows

  while(!SDLKEYDOWN(SDLK_ESCAPE) && !SDLKEYDOWN(SDLK_F1))      // Main loop
  {
    render_all_windows(&c_mesh, &cmouse_info);
    cart_draw_main();
  }


  show_info();            // Ending statistics

  return 0;
}

//------------------------------------------------------------------------------


////------------------------------------------------------------------------------
//void unbound_mouse()
//{
//  mstlx = 0;
//  mstly = 0;
//  msbrx = OUTX-1;
//  msbry = OUTY-1;
//  return;
//}

////------------------------------------------------------------------------------
//void bound_mouse(cart_mouse_info * m)
//{
//  if(m->data != -1)
//  {
//    mstlx = cwindow_info[m->data].rect.x+cwindow_info[m->data].borderx;
//    mstly = cwindow_info[m->data].rect.y+cwindow_info[m->data].bordery;
//    msbrx = mstlx+cwindow_info[m->data].rect.w-1;
//    msbry = mstly+cwindow_info[m->data].rect.h-1;
//  }
//  return;
//}

////------------------------------------------------------------------------------
//int mesh_get_level(cart_mesh * cmsh, int x, int y)
//{
//  Uint32 fan;
//  int z0, z1, z2, z3;         // Height of each fan corner
//  int zleft, zright,zdone;    // Weighted height of each side
//
//
//  if((y>>7) >= cmsh->mi->tiles_y || (x>>7) >= cmsh->mi->tiles_x)
//    return 0;
//  fan = mesh_convert_fan(cmsh->mi, x>>7, y>>7);
//  x = x&127;
//  y = y&127;
//  z0 = cmsh->mm->vrt__start[cmsh->mm->tilelst[fan]+0].vrtz;
//  z1 = cmsh->mm->vrt__start[cmsh->mm->tilelst[fan]+1].vrtz;
//  z2 = cmsh->mm->vrt__start[cmsh->mm->tilelst[fan]+2].vrtz;
//  z3 = cmsh->mm->vrt__start[cmsh->mm->tilelst[fan]+3].vrtz;
//
//  zleft = (z0*(128-y)+z3*y)>>7;
//  zright = (z1*(128-y)+z2*y)>>7;
//  zdone = (zleft*(128-x)+zright*x)>>7;
//  return (zdone);
//}

////------------------------------------------------------------------------------
//void mesh_make_twist(cart_mesh * cmsh)
//{
//  Uint32 fan, numfan;
//
//  numfan = cmsh->mi->tiles_x*cmsh->mi->tiles_y;
//  fan = 0;
//  while(fan < numfan)
//  {
//    cmsh->mm->tilelst[fan].twist = get_fan_twist(cmsh->mm, fan);
//    fan++;
//  }
//
//
//  return;
//}

//------------------------------------------------------------------------------
//int mesh_load(cart_mesh * cmsh, char *modname)
//{
//  FILE* fileread;
//  char newloadname[256];
//  int itmp, num, cnt;
//  float ftmp;
//  Uint32 fan;
//  Uint32 numvert, numfan;
//  Uint32 vert;
//  int x, y;
//
//
//  make_newloadname(modname, SLASH_STRING "gamedat" SLASH_STRING "level.mpd", newloadname);
//  fileread = fopen(newloadname, "rb");
//  if(fileread)
//  {
//    free_vertices(cmsh->mm);
//    fread(&itmp, 4, 1, fileread);  if(itmp != MAPID) return bfalse;
//    fread(&itmp, 4, 1, fileread);  numvert = itmp;
//    fread(&itmp, 4, 1, fileread);  cmsh->mi->tiles_x = itmp;
//    fread(&itmp, 4, 1, fileread);  cmsh->mi->tiles_y = itmp;
//    numfan = cmsh->mi->tiles_x*cmsh->mi->tiles_y;
//    cmsh->mi->edge_x = (cmsh->mi->tiles_x*SMALLX)-1;
//    cmsh->mi->edge_y = (cmsh->mi->tiles_y*SMALLY)-1;
//    cmsh->edgez = 180<<4;
//    cmsh->xvrt.free_count = MAXTOTALMESHVERTICES-numvert;
//
//
//    // Load fan data
//    fan = 0;
//    while(fan < numfan)
//    {
//      fread(&itmp, 4, 1, fileread);
//      cmsh->mm->tilelst[fan].type = itmp>>24;
//      cmsh->mm->tilelst[fan].fx = itmp>>16;
//      cmsh->mm->tilelst[fan].tile = itmp;
//      fan++;
//    }
//    // Load normal data
//    fan = 0;
//    while(fan < numfan)
//    {
//      fread(&itmp, 1, 1, fileread);
//      cmsh->mm->tilelst[fan].twist = itmp;
//      fan++;
//    }
//
//
//
//
//
//
//    // Load vertex x data
//    cnt = 0;
//    while(cnt < numvert)
//    {
//      fread(&ftmp, 4, 1, fileread);
//      cmsh->mm->vrt_x[cnt] = ftmp/FIXNUM;
//      cnt++;
//    }
//    // Load vertex y data
//    cnt = 0;
//    while(cnt < numvert)
//    {
//      fread(&ftmp, 4, 1, fileread);
//      cmsh->mm->vrt_y[cnt] = ftmp/FIXNUM;
//      cnt++;
//    }
//    // Load vertex z data
//    cnt = 0;
//    while(cnt < numvert)
//    {
//      fread(&ftmp, 4, 1, fileread);
//      cmsh->mm->vrt_z[cnt] = ftmp/FIXNUM;
////      cmsh->mm->vrt_z[cnt] = 0;
//      cnt++;
//    }
//    // Load vertex a data
//    cnt = 0;
//    while(cnt < numvert)
//    {
////      fread(&itmp, 1, 1, fileread);
//      cmsh->mm->vrt_a[cnt] = 60;  // !!!BAD!!!
//      cnt++;
//    }
//
//
//    mesh_make_fanstart();
//    vert = 0;
//    y = 0;
//    while(y < cmsh->mi->tiles_y)
//    {
//      x = 0;
//      while(x < cmsh->mi->tiles_x)
//      {
//        fan = mesh_convert_fan(cmsh->mi, x, y);
//        num = pmesh->TileDict[cmsh->mm->tilelst[fan].type].vrt_count;
//        cmsh->mm->tilelst[fan].vrt_start = vert;
//        cnt = 0;
//        while(cnt < num)
//        {
//          cmsh->xvrt.next[vert] = vert+1;
//          vert++;
//          cnt++;
//        }
//        cmsh->xvrt.next[vert-1] = CHAINEND;
//        x++;
//      }
//      y++;
//    }
//    return btrue;
//  }
//  return bfalse;
//}

//void move_camera(cart_mouse_info * m)
//{
//  if(((mous.latch.b&4) || SDLKEYDOWN(SDLK_m)) && m->data!=-1)
//  {
//    cart_pos_x+=mcx;
//    cart_pos_y+=mcy;
//    bound_camera(cmsh);
//    ui_getMouseX()=msxold;
//    ui_getMouseY()=msyold;
//  }
//  return;
//}
//------------------------------------------------------------------------------
//void mouse_side(cart_mouse_info * m, int cnt)
//{
//  m->x = ui_getMouseX() - cwindow_info[cnt].rect.x-cwindow_info[cnt].borderx+cart_pos_x-69;
//  m->y = ui_getMouseY() - cwindow_info[cnt].rect.y-cwindow_info[cnt].bordery;
//  m->x = m->x*FOURNUM;
//  m->y = m->y*FOURNUM;
//  if(SDLKEYDOWN(SDLK_f))
//  {
//    flatten_mesh(cmsh, m);
//  }
//  if(mous.latch.b&1)
//  {
//    if(m->rect==bfalse)
//    {
//      m->rect=btrue;
//      m->rectx=m->x;
//      m->recty=m->y;
//    }
//  }
//  else
//  {
//    if(m->rect==btrue)
//    {
//      if(cselect_lst.count!=0 && !SDLKEYDOWN(SDLK_LALT) &&  !SDLKEYDOWN(SDLK_RALT) &&
//                         !SDLKEYDOWN(SDLK_LCTRL) && !SDLKEYDOWN(SDLK_RCTRL))
//      {
//        clear_select(&cselect_lst);
//      }
//      if(SDLKEYDOWN(SDLK_ALT) || SDLKEYDOWN(SDLK_ALTGR))
//      {
//        rect_unselect(cmsh, &cselect_lst, m);
//      }
//      else
//      {
//        rect_select(cmsh, &cselect_lst, m);
//      }
//      m->rect = bfalse;
//    }
//  }
//  if(mous.latch.b&2)
//  {
//    move_select(cmsh, &cselect_lst, mcx, 0, -(mcy<<4));
//    bound_mouse();
//  }
//
//  if(SDLKEYDOWN(SDLK_y))
//  {
//    move_select(cmsh, &cselect_lst, 0, 0, -(mcy<<4));
//    bound_mouse();
//  }
//  if(SDLKEYDOWN(SDLK_5))
//  {
//    set_select_no_bound_z(&cmesh, &cselect_lst, -8000<<2);
//  }
//  if(SDLKEYDOWN(SDLK_6))
//  {
//    set_select_no_bound_z(&cmesh, &cselect_lst, -127<<2);
//  }
//  if(SDLKEYDOWN(SDLK_7))
//  {
//    set_select_no_bound_z(&cmesh, &cselect_lst, 127<<2);
//  }
//  if(SDLKEYDOWN(SDLK_u))
//  {
//    if(m->type >= (MAXMESHTYPE>>1))
//    {
//      move_mesh_z(cmsh, -(mcy<<4), m->tile, 192);
//    }
//    else
//    {
//      move_mesh_z(cmsh, -(mcy<<4), m->tile, 240);
//    }
//    bound_mouse();
//  }
//  if(SDLKEYDOWN(SDLK_n))
//  {
//    if(SDLKEYDOWN(SDLK_RSHIFT))
//    {
//      // Move the first 16 up and down
//      move_mesh_z(cmsh, -(mcy<<4), 0, 240);
//    }
//    else
//    {
//      // Move the entire mesh up and down
//      move_mesh_z(cmsh, -(mcy<<4), 0, 0);
//    }
//    bound_mouse();
//  }
//  if(SDLKEYDOWN(SDLK_q))
//  {
//    fix_walls(cmsh, m);
//  }
//
//
//  return;
//}

//------------------------------------------------------------------------------
//void mouse_tile(cart_mesh * cmsh, cart_mouse_info * mi, int cnt)
//{
//  int x, y, keyt, vert, keyv;
//  float tl, tr, bl, br;
//
//
//  cmsh->mi->x = ui_getMouseX() - cwindow_info[cnt].rect.x-cwindow_info[cnt].borderx+cart_pos_x-69;
//  cmsh->mi->y = ui_getMouseY() - cwindow_info[cnt].rect.y-cwindow_info[cnt].bordery+cart_pos_y-69;
//  if(cmsh->mi->x < 0 ||
//     cmsh->mi->x >= SMALLX*cmsh->mi->tiles_x ||
//     cmsh->mi->y < 0 ||
//     cmsh->mi->y >= SMALLY*cmsh->mi->tiles_y)
//  {
//    cmsh->mi->x = cmsh->mi->x*FOURNUM;
//    cmsh->mi->y = cmsh->mi->y*FOURNUM;
//    if(mous.latch.b&2)
//    {
//       cmsh->mi->type = 0+0;
//       cmsh->mi->tile = 0xffff;
//    }
//  }
//  else
//  {
//    cmsh->mi->x = cmsh->mi->x*FOURNUM;
//    cmsh->mi->y = cmsh->mi->y*FOURNUM;
//    if(cmsh->mi->x >= (cmsh->mi->tiles_x<<7))  cmsh->mi->x = (cmsh->mi->tiles_x<<7)-1;
//    if(cmsh->mi->y >= (cmsh->mi->tiles_y<<7))  cmsh->mi->y = (cmsh->mi->tiles_y<<7)-1;
//    debugx = cmsh->mi->x/128.0;
//    debugy = cmsh->mi->y/128.0;
//    x = cmsh->mi->x>>7;
//    y = cmsh->mi->y>>7;
//    cmsh->mi->onfan = mesh_convert_fan(cmsh->mi, x, y);
//
//
//    if(!SDLKEYDOWN(SDLK_k))
//    {
//      clight_lst.adding = bfalse;
//    }
//    if(SDLKEYDOWN(SDLK_k)&&clight_lst.adding==bfalse)
//    {
//      add_light(&clight_lst, cmsh->mi->x, cmsh->mi->y, MINRADIUS, MAXHEIGHT);
//      clight_lst.adding = btrue;
//    }
//    if(clight_lst.adding)
//    {
//      alter_light(&clight_lst, cmsh->mi->x, cmsh->mi->y);
//    }
//    if(mous.latch.b&1)
//    {
//      keyt = SDLKEYDOWN(SDLK_t);
//      keyv = SDLKEYDOWN(SDLK_v);
//      if(!keyt)
//      {
//        if(!keyv)
//        {
//          // Save corner heights
//          vert = cmsh->mm->tilelst[cmsh->mi->onfan].vrt_start;
//          tl = cmsh->mm->vrt_z[vert];
//          vert = cmsh->xvrt.next[vert];
//          tr = cmsh->mm->vrt_z[vert];
//          vert = cmsh->xvrt.next[vert];
//          br = cmsh->mm->vrt_z[vert];
//          vert = cmsh->xvrt.next[vert];
//          bl = cmsh->mm->vrt_z[vert];
//        }
//        remove_fan(cmsh->mm, cmsh->mi->onfan);
//      }
//      switch(cmsh->mi->presser)
//      {
//        case 0:
//          cmsh->mm->tilelst[cmsh->mi->onfan].tile=cmsh->mi->tile;
//          break;
//        case 1:
//          cmsh->mm->tilelst[cmsh->mi->onfan].tile=(cmsh->mi->tile&0xfffe)+IRAND(&(cmsh->seed), 1);
//          break;
//        case 2:
//          cmsh->mm->tilelst[cmsh->mi->onfan].tile=(cmsh->mi->tile&0xfffc)+IRAND(&(cmsh->seed), 2);
//          break;
//        case 3:
//          cmsh->mm->tilelst[cmsh->mi->onfan].tile=(cmsh->mi->tile&0xfff0)+(rand()&6);
//          break;
//      }
//      if(!keyt)
//      {
//        cmsh->mm->tilelst[cmsh->mi->onfan].type=cmsh->mi->type;
//        add_fan(cmsh->mm, cmsh->mi->onfan, (cmsh->mi->x>>7)*31, (cmsh->mi->y>>7)*31);
//        cmsh->mm->tilelst[cmsh->mi->onfan].fx=cmsh->mi->fx;
//        if(!keyv)
//        {
//          // Return corner heights
//          vert = cmsh->mm->tilelst[cmsh->mi->onfan].vrt_start;
//          cmsh->mm->vrt_z[vert] = tl;
//          vert = cmsh->xvrt.next[vert];
//          cmsh->mm->vrt_z[vert] = tr;
//          vert = cmsh->xvrt.next[vert];
//          cmsh->mm->vrt_z[vert] = br;
//          vert = cmsh->xvrt.next[vert];
//          cmsh->mm->vrt_z[vert] = bl;
//        }
//      }
//    }
//    if(mous.latch.b&2)
//    {
//      cmsh->mi->type = cmsh->mm->tilelst[cmsh->mi->onfan].type;
//      cmsh->mi->tile = cmsh->mm->tilelst[cmsh->mi->onfan].tile;
//    }
//  }
//
//
//  return;
//}

//------------------------------------------------------------------------------
//void mouse_fx(cart_mesh * cmsh, cart_mouse_info * m, int cnt)
//{
//  int x, y;
//
//
//  m->x = ui_getMouseX() - cwindow_info[cnt].rect.x-cwindow_info[cnt].borderx+cart_pos_x-69;
//  m->y = ui_getMouseY() - cwindow_info[cnt].rect.y-cwindow_info[cnt].bordery+cart_pos_y-69;
//  if(m->x < 0 ||
//     m->x >= SMALLX*cmsh->mi->tiles_x ||
//     m->y < 0 ||
//     m->y >= SMALLY*cmsh->mi->tiles_y)
//  {
//  }
//  else
//  {
//    m->x = m->x*FOURNUM;
//    m->y = m->y*FOURNUM;
//    if(m->x >= (cmsh->mi->tiles_x<<7))  m->x = (cmsh->mi->tiles_x<<7)-1;
//    if(m->y >= (cmsh->mi->tiles_y<<7))  m->y = (cmsh->mi->tiles_y<<7)-1;
//    debugx = m->x/128.0;
//    debugy = m->y/128.0;
//    x = m->x>>7;
//    y = m->y>>7;
//    m->onfan = mesh_convert_fan(cmsh->mi, x, y);
//
//
//    if(mous.latch.b&1)
//    {
//      cmsh->mm->tilelst[m->onfan].fx = m->fx;
//    }
//    if(mous.latch.b&2)
//    {
//      m->fx = cmsh->mm->tilelst[m->onfan].fx;
//    }
//  }
//
//
//  return;
//}

//------------------------------------------------------------------------------
//void mouse_vertex(cart_mouse_info * m, int cnt)
//{
//  m->x = ui_getMouseX() - cwindow_info[cnt].rect.x-cwindow_info[cnt].borderx+cart_pos_x-69;
//  m->y = ui_getMouseY() - cwindow_info[cnt].rect.y-cwindow_info[cnt].bordery+cart_pos_y-69;
//  m->x = m->x*FOURNUM;
//  m->y = m->y*FOURNUM;
//  if(SDLKEYDOWN(SDLK_f))
//  {
////    fix_corners(cmsh, m->x>>7, m->y>>7);
//    fix_vertices(cmsh, m->x>>7, m->y>>7);
//  }
//  if(SDLKEYDOWN(SDLK_5))
//  {
//    set_select_no_bound_z(&cmesh, &cselect_lst, -8000<<2);
//  }
//  if(SDLKEYDOWN(SDLK_6))
//  {
//    set_select_no_bound_z(&cmesh, &cselect_lst, -127<<2);
//  }
//  if(SDLKEYDOWN(SDLK_7))
//  {
//    set_select_no_bound_z(&cmesh, &cselect_lst, 127<<2);
//  }
//  if(mous.latch.b&1)
//  {
//    if(m->rect==bfalse)
//    {
//      m->rect=btrue;
//      m->rectx=m->x;
//      m->recty=m->y;
//    }
//  }
//  else
//  {
//    if(m->rect==btrue)
//    {
//      if(cselect_lst.count!=0 && !SDLKEYDOWN(SDLK_ALT) && !SDLKEYDOWN(SDLK_ALTGR) &&
//                         !SDLKEYDOWN(SDLK_LCTRL) && !SDLKEYDOWN(SDLK_RCTRL))
//      {
//        clear_select(csel);
//      }
//      if(SDLKEYDOWN(SDLK_ALT) || SDLKEYDOWN(SDLK_ALTGR))
//      {
//        rect_unselect(cmsh, &cselect_lst, m);
//      }
//      else
//      {
//        rect_select(cmsh, &cselect_lst, m);
//      }
//      m->rect = bfalse;
//    }
//  }
//  if(mous.latch.b&2)
//  {
//    move_select(cmsh, &cselect_lst, mcx, mcy, 0);
//    bound_mouse();
//  }
//
//
//  return;
//}

//------------------------------------------------------------------------------
//void check_mouse(cart_mouse_info * m)
//{
//  int x, y, cnt;
//
//
//  debugx = -1;
//  debugy = -1;
//
//  unbound_mouse();
//  move_camera(m);
//  m->data = -1;
//  m->x = -1;
//  m->y = -1;
//  m->mode = 0;
//  cnt = 0;
//  while(cnt < MAXWIN)
//  {
//    if(cwindow_info[cnt].on)
//    {
//      if(ui_getMouseX() >= cwindow_info[cnt].rect.x+cwindow_info[cnt].borderx &&
//         ui_getMouseX() <  cwindow_info[cnt].rect.x+cwindow_info[cnt].borderx+cwindow_info[cnt].rect.w &&
//         ui_getMouseY() >= cwindow_info[cnt].rect.y+cwindow_info[cnt].bordery &&
//         ui_getMouseY() <  cwindow_info[cnt].rect.y+cwindow_info[cnt].bordery+cwindow_info[cnt].rect.h)
//      {
//        m->data = cnt;
//        m->mode = cwindow_info[cnt].mode;
//        if(m->mode==WINTILE)
//        {
//          mouse_tile(m, cnt);
//        }
//        if(m->mode==WINVERTEX)
//        {
//          mouse_vertex(m, cnt);
//        }
//        if(m->mode==WINSIDE)
//        {
//          mouse_side(m, cnt);
//        }
//        if(m->mode==WINFX)
//        {
//          mouse_fx(cmsh, m, cnt);
//        }
//      }
//    }
//    cnt++;
//  }
//  return;
//}