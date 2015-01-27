/* Egoboo - Font.h
 * True-type font drawing functionality.  Uses Freetype 2 & OpenGL
 * to do it's business.
 */

/*
    This file is part of Egoboo.

    Egoboo is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Egoboo is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef egoboo_Font_h
#define egoboo_Font_h

#define FNT_NUM_FONT_CHARACTERS 94
#define FNT_SMALL_FONT_SIZE 12
#define FNT_NORMAL_FONT_SIZE 16
#define FNT_LARGE_FONT_SIZE 20
#define FNT_MAX_FONTS 8

typedef struct Font Font;

extern Font  *fnt_loadFont(const char *fileName, int pointSize);
extern void  fnt_freeFont(Font *font);

extern void  fnt_drawText(Font *font, int x, int y, const char *text);
extern void  fnt_drawTextFormatted(Font *font, int x, int y, const char *format, ...);
extern void  fnt_drawTextBox(Font *font, const char *text, int x, int y, int width, int height, int spacing);

// Only works properly on a single line of text
extern void  fnt_getTextSize(Font *font, const char *text, int *width, int *height);
// Works for multiple-line strings, using the user-supplied spacing
extern void  fnt_getTextBoxSize(Font *font, const char *text, int spacing, int *width, int *height);
#endif // include guard
