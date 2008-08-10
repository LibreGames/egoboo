//********************************************************************************************
//* Egoboo - TTF_Font.c
//*
//* Code for implementing bitmapped and outline fonts.
//* Outline fonts are currently handled through SDL_ttf
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
#include "Font.h"

#include "ogl_texture.h"
#include "Log.h"

#include "egoboo_utility.h"
#include "egoboo.h"

#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_ttf.h>
#include <assert.h>



//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct sTTFont
{
  TTF_Font *ttfFont;

  GLuint  texture;
  GLfloat texCoords[4];
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static bool_t   fnt_initialized = bfalse;

struct s_fnt_registry
{
  int      count;
  TTFont_t * list[256];
};

typedef struct s_fnt_registry FNT_REGISTRY;

static FNT_REGISTRY fnt_registry = {0};

INLINE bool_t fnt_reg_init(FNT_REGISTRY * r)
{
  if(NULL == r) return bfalse;

  assert(0 == r->count);
  memset(r->list, 0, sizeof(FNT_REGISTRY));

  return btrue;
};

INLINE bool_t fnt_reg_add(FNT_REGISTRY * r, TTFont_t * f)
{
  int i;

  if(NULL == r || NULL == f || r->count >= 256) return bfalse;

  // make sure there are no duplicates
  for(i = 0; i< r->count; i++)
  {
    if(r->list[i] == f) return bfalse;
  }

  r->list[r->count] = f;
  r->count++;

  return btrue;
}

INLINE TTFont_t * fnt_reg_remove(FNT_REGISTRY * r, TTFont_t * f)
{
  bool_t   found;
  TTFont_t * retval = NULL;
  int      i;

  if(NULL == r || NULL == f || r->count <= 0) return retval;

  // make sure there are no duplicates
  found = bfalse;
  for(i = 0; i< r->count; i++)
  {
    if(r->list[i] == f) {found = btrue; break; }
  }

  if(found)
  {
    // remove the element
    r->list[i] = r->list[r->count - 1];
    r->list[r->count - 1] = NULL;
    r->count--;

    retval = f;
  };

  return retval;
}

INLINE bool_t fnt_reg_empty(FNT_REGISTRY * r)
{
  if(NULL == r || r->count <= 0) return btrue;
  return bfalse;
}

INLINE TTFont_t * fnt_reg_pop(FNT_REGISTRY * r)
{
  TTFont_t * retval = NULL;

  if(NULL == r || r->count <= 0) return retval;

  r->count--;
  retval = r->list[r->count];

  return retval;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

static int    powerOfTwo( int input );
static int    copySurfaceToTexture( SDL_Surface *surface, GLuint texture, GLfloat *texCoords );
static bool_t fnt_init();
static void   fnt_quit(void);

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// The next two functions are borrowed from the gl_font.c test program from SDL_ttf
static int powerOfTwo( int input )
{
  int value = 1;

  while ( value < input )
  {
    value <<= 1;
  }

  return value;
}

//--------------------------------------------------------------------------------------------
static int copySurfaceToTexture( SDL_Surface *surface, GLuint texture, GLfloat *texCoords )
{
  int w, h;
  SDL_Surface *image;
  SDL_Rect area;
  Uint32 saved_flags;
  Uint8 saved_alpha;

  // Use the surface width & height expanded to the next powers of two
  w = powerOfTwo( surface->w );
  h = powerOfTwo( surface->h );
  texCoords[0] = 0.0f;
  texCoords[1] = 0.0f;
  texCoords[2] = ( GLfloat ) surface->w / w;
  texCoords[3] = ( GLfloat ) surface->h / h;

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
  image = SDL_CreateRGBSurface( SDL_SWSURFACE, w, h, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000 );
#else
  image = SDL_CreateRGBSurface( SDL_SWSURFACE, w, h, 32, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff );
#endif

  if ( NULL == image  )
  {
    return 0;
  }

  // Save the alpha blending attributes
  saved_flags = surface->flags & ( SDL_SRCALPHA | SDL_RLEACCELOK );
  saved_alpha = surface->format->alpha;
  if (( saved_flags & SDL_SRCALPHA ) == SDL_SRCALPHA )
  {
    SDL_SetAlpha( surface, 0, 0 );
  }

  // Copy the surface into the texture image
  area.x = 0;
  area.y = 0;
  area.w = surface->w;
  area.h = surface->h;
  SDL_BlitSurface( surface, &area, image, &area );

  // Restore the blending attributes
  if (( saved_flags & SDL_SRCALPHA ) == SDL_SRCALPHA )
  {
    SDL_SetAlpha( surface, saved_flags, saved_alpha );
  }

  // Send the texture to OpenGL
  glBindTexture( GL_TEXTURE_2D, texture );
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
  glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
  glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image->pixels );

  // Don't need the extra image anymore
  SDL_FreeSurface( image );

  return 1;
}

//--------------------------------------------------------------------------------------------
static bool_t fnt_init()
{
  if(fnt_initialized) return btrue;

  if ( TTF_Init() == -1 )
  {
    log_warning( "fnt_loadFont() - \n\tCould not initialize SDL_TTF!\n" );
  }
  else
  {
    fnt_reg_init(&fnt_registry);

    atexit( fnt_quit );
    fnt_initialized = btrue;
  }


  return fnt_initialized;
};

//--------------------------------------------------------------------------------------------
static void fnt_quit(void)
{
  // BB > automatically unregister and delete all fonts that have been opened and TTF_Quit()

  TTFont_t * pfnt;
  bool_t close_fonts;

  close_fonts = fnt_initialized && TTF_WasInit();

  while( !fnt_reg_empty(&fnt_registry) )
  {
    pfnt = fnt_reg_pop(&fnt_registry);
    assert(NULL != pfnt);

    if(close_fonts && NULL !=pfnt->ttfFont)
    {
      TTF_CloseFont(pfnt->ttfFont);
    }

    FREE(pfnt);
  }

  if(TTF_WasInit())
  {
    TTF_Quit();
  }
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
TTFont_t* fnt_loadFont( const char *fileName, int pointSize )
{
  TTFont_t   *newFont;
  TTF_Font *ttfFont;

  // Make sure the TTF library was initialized
  fnt_init();

  // Try and open the font
  ttfFont = TTF_OpenFont( fileName, pointSize );
  if ( NULL == ttfFont )
  {
    // couldn't open it, for one reason or another
    return NULL;
  }

  // Everything looks good
  newFont = ( TTFont_t* ) calloc( 1, sizeof( TTFont_t ) );
  newFont->ttfFont = ttfFont;
  newFont->texture = 0;

  // register the font
  fnt_reg_add(&fnt_registry, newFont);

  return newFont;
}

//--------------------------------------------------------------------------------------------
bool_t fnt_freeFont( TTFont_t *font )
{
  TTFont_t * pfnt = NULL;

  if ( NULL == font ) return bfalse;

  // see if the font was registered
  pfnt = fnt_reg_remove(&fnt_registry, font);

  // only delete registered fonts.
  // if there is a valid pointer but it is not in the registry, it must be a dangling pointer
  if(NULL != pfnt)
  {
    if(NULL != pfnt->ttfFont) { TTF_CloseFont( pfnt->ttfFont); }
    glDeleteTextures( 1, &pfnt->texture );
    FREE( font );
  }


  return (NULL != pfnt);
}

//--------------------------------------------------------------------------------------------
void fnt_drawText( TTFont_t *font, int x, int y, const char *text )
{
  SDL_Surface *textSurf;
  SDL_Color color = { 0xFF, 0xFF, 0xFF, 0 };

  if ( NULL == font ||  !VALID_CSTR(text)  ) return;

  // Let TTF render the text
  textSurf = TTF_RenderText_Blended( font->ttfFont, text, color );

  // Does this font already have a texture?  If not, allocate it here
  if ( font->texture == 0 )
  {
    glGenTextures( 1, &font->texture );
  }

  // Copy the surface to the texture
  if ( copySurfaceToTexture( textSurf, font->texture, font->texCoords ) )
  {
    // And draw the darn thing
    glBegin( GL_TRIANGLE_STRIP );
    glTexCoord2f( font->texCoords[0], font->texCoords[1] );
    glVertex2i( x, y );
    glTexCoord2f( font->texCoords[2], font->texCoords[1] );
    glVertex2i( x + textSurf->w, y );
    glTexCoord2f( font->texCoords[0], font->texCoords[3] );
    glVertex2i( x, y + textSurf->h );
    glTexCoord2f( font->texCoords[2], font->texCoords[3] );
    glVertex2i( x + textSurf->w, y + textSurf->h );
    glEnd();
  }

  // Done with the surface
  SDL_FreeSurface( textSurf );

}

//--------------------------------------------------------------------------------------------
void fnt_getTextSize( TTFont_t *font, const char *text, int *pwidth, int *pheight )
{
  if ( font )
  {
    TTF_SizeText( font->ttfFont, text, pwidth, pheight );
  }
}

//--------------------------------------------------------------------------------------------
// fnt_drawTextBox
// Draws a text string into a box, splitting it into lines according to newlines in the string.
// NOTE: Doesn't pay attention to the width/height arguments yet.
//
//   font    - The font to draw with
//   text    - The text to draw
//   x       - The x position to start drawing at
//   y       - The y position to start drawing at
//   width   - Maximum width of the box (not implemented)
//   height  - Maximum height of the box (not implemented)
//   spacing - Amount of space to move down between lines. (usually close to your font size)

void fnt_drawTextBox( TTFont_t *font, const char *text, int x, int y,  int width, int height, int spacing )
{
  GLint matrix_mode;
  GLint viewport_save[4];
  size_t len;
  char *buffer, *line;

  if ( NULL == font ) return;

  // If text is empty, there's nothing to draw
  if (  !VALID_CSTR(text)  ) return;

  //if(0 != width && 0 != height)
  //{
  //  //grab the old viewoprt info
  //  glGetIntegerv (GL_VIEWPORT, viewport_save);
  //  glGetIntegerv(GL_MATRIX_MODE, &matrix_mode);

  //  // forcefully clip the active region to the given box

  //  glMatrixMode( GL_PROJECTION );                            /* GL_TRANSFORM_BIT */
  //  glPushMatrix();
  //  glLoadIdentity();
  //  glOrtho( x, x + width, y + height, y, -1, 1 );

  //  glViewport(x,gfxState.scry-y-height,width,height);
  //}

  // Split the passed in text into separate lines
  len = strlen( text );
  buffer = (char*)calloc( len + 1, sizeof(char) );
  strncpy( buffer, text, len );

  line = strtok( buffer, "\n" );
  while ( NULL != line  )
  {
    fnt_drawText( font, x, y, line );
    y += spacing;
    line = strtok( NULL, "\n" );
  }
  FREE( buffer );

  //if(0 != width && 0 != height)
  //{
  //  glMatrixMode( matrix_mode );                            /* GL_TRANSFORM_BIT */
  //  glPopMatrix();

  //  // restore the old viewport
  //  glViewport(viewport_save[0],viewport_save[1],viewport_save[2],viewport_save[3]);
  //}
}

//--------------------------------------------------------------------------------------------
void fnt_getTextBoxSize( TTFont_t *font, const char *text, int spacing, int *width, int *height )
{
  char *buffer, *line;
  size_t len;
  int tmp_w, tmp_h;

  if ( NULL == font ) return;
  if (  !VALID_CSTR(text)  ) return;

  // Split the passed in text into separate lines
  len = strlen( text );
  buffer = (char*)calloc( len + 1, sizeof(char) );
  strncpy( buffer, text, len );

  line = strtok( buffer, "\n" );
  *width = *height = 0;
  while ( NULL != line  )
  {
    TTF_SizeText( font->ttfFont, line, &tmp_w, &tmp_h );
    *width = ( *width > tmp_w ) ? *width : tmp_w;
    *height += spacing;

    line = strtok( NULL, "\n" );
  }
  FREE( buffer );
}

//--------------------------------------------------------------------------------------------
void fnt_drawTextFormatted( TTFont_t * fnt, int x, int y, const char *format, ... )
{
  va_list args;
  char buffer[256];

  va_start( args, format );
  vsnprintf( buffer, 256, format, args );
  va_end( args );

  fnt_drawText( fnt, x, y, buffer );
}


//--------------------------------------------------------------------------------------------
BMFont_t * BMFont_new( BMFont_t * pfnt )
{
  if( EKEY_PVALID(pfnt) )
  {
    BMFont_delete( pfnt );
  }

  memset(pfnt, 0, sizeof(BMFont_t));

  EKEY_PNEW(pfnt, BMFont_t);

  GLtexture_new( &(pfnt->tex) );

  return pfnt;
}

//--------------------------------------------------------------------------------------------
bool_t BMFont_delete( BMFont_t * pfnt )
{
  if( !EKEY_PVALID(pfnt) ) return btrue;

  GLtexture_delete( &(pfnt->tex) );

  EKEY_PINVALIDATE( pfnt );

  return btrue;
}

//--------------------------------------------------------------------------------------------
bool_t BMFont_load( BMFont_t * pfnt, int scr_height, char* szBitmap, char* szSpacing )
{
  // ZZ> This function loads the font bitmap and sets up the coordinates
  //     of each font on that bitmap...  Bitmap must have 16x6 fonts

  int cnt, y, xsize, ysize, xdiv, ydiv;
  int xstt, ystt;
  int xspacing, yspacing;
  Uint8 cTmp;
  FILE *fileread;

  if ( INVALID_TEXTURE == GLtexture_Load( GL_TEXTURE_2D, &(pfnt->tex), szBitmap, 0 ) ) return bfalse;

  // Clear out the conversion table
  for ( cnt = 0; cnt < 256; cnt++ )
    pfnt->ascii_table[cnt] = 0;

  // Get the size of the bitmap
  xsize = GLtexture_GetImageWidth( &(pfnt->tex) );
  ysize = GLtexture_GetImageHeight( &(pfnt->tex) );
  if ( xsize == 0 || ysize == 0 )
    log_warning( "Bad font size! (basicdat" SLASH_STRING "%s) - X size: %i , Y size: %i\n", szBitmap, xsize, ysize );


  // Figure out the general size of each font
  ydiv = ysize / NUMFONTY;
  xdiv = xsize / NUMFONTX;


  // Figure out where each font is and its spacing
  fileread = fs_fileOpen( PRI_NONE, NULL, szSpacing, "r" );
  if ( NULL == fileread  ) return bfalse;

  globalname = szSpacing;

  // Uniform font height is at the top
  yspacing = fget_next_int( fileread );
  pfnt->offset = scr_height - yspacing;

  // Mark all as unused
  for ( cnt = 0; cnt < 255; cnt++ )
    pfnt->ascii_table[cnt] = 255;


  cnt = 0;
  y = 0;
  xstt = 0;
  ystt = 0;
  while ( cnt < 255 && fgoto_colon_yesno( fileread ) )
  {
    cTmp = fgetc( fileread );
    xspacing = fget_int( fileread );
    if ( pfnt->ascii_table[cTmp] == 255 )
    {
      pfnt->ascii_table[cTmp] = cnt;
    }

    if ( xstt + xspacing + 1 >= xsize )
    {
      xstt = 0;
      ystt += yspacing;
    }

    pfnt->rect[cnt].x = xstt;
    pfnt->rect[cnt].w = xspacing;
    pfnt->rect[cnt].y = ystt;
    pfnt->rect[cnt].h = yspacing - 1;
    pfnt->spacing_x[cnt] = xspacing;

    xstt += xspacing + 1;

    cnt++;
  }
  fs_fileClose( fileread );


  // Space between lines
  pfnt->spacing_y = ( yspacing >> 1 ) + FONTADD;

  return btrue;
}


