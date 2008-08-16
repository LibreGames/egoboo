//********************************************************************************************
//* Egoboo - Menu.c
//*
//* Implements the main menu tree, using the code in Ui.
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

//********************************************************************************************
// New menu code
//********************************************************************************************

#include "Menu.h"

#include "Ui.h"
#include "Log.h"
#include "Server.h"
#include "Client.h"
#include "NetFile.h"
#include "Clock.h"
#include "sound.h"

#include "egoboo.h"

#include <assert.h>

#include "input.inl"
#include "graphic.inl"
#include "Network.inl"
#include "egoboo_types.inl"


int              loadplayer_count = 0;
LOAD_PLAYER_INFO loadplayer[MAXLOADPLAYER];

static retval_t   MenuProc_ensure_server(MenuProc_t * ms, struct sGame * gs);
static retval_t   MenuProc_ensure_client(MenuProc_t * ms, struct sGame * gs);
static retval_t   MenuProc_ensure_network(MenuProc_t * ms, struct sGame * gs);

static retval_t   MenuProc_start_server(MenuProc_t * ms, struct sGame * gs);
static retval_t   MenuProc_start_client(MenuProc_t * ms, struct sGame * gs);
static retval_t   MenuProc_start_network(MenuProc_t * ms, struct sGame * gs);
static retval_t   MenuProc_start_netfile(MenuProc_t * ms, struct sGame * gs);

static int mnu_doMain( MenuProc_t * mproc, float deltaTime );
static int mnu_doSinglePlayer( MenuProc_t * mproc, float deltaTime );
static int mnu_doChooseModule( MenuProc_t * mproc, float deltaTime );
static int mnu_doChoosePlayer( MenuProc_t * mproc, float deltaTime );
static int mnu_doOptions( MenuProc_t * mproc, float deltaTime );
static int mnu_doAudioOptions( MenuProc_t * mproc, float deltaTime );
static int mnu_doVideoOptions( MenuProc_t * mproc, float deltaTime );
static int mnu_doLaunchGame( MenuProc_t * mproc, float deltaTime );
static int mnu_doNotImplemented( MenuProc_t * mproc, float deltaTime );
//static int mnu_doModel(MenuProc_t * mproc, float deltaTime);
static int mnu_doNetwork(MenuProc_t * mproc, float deltaTime);
static int mnu_doHostGame(MenuProc_t * mproc, float deltaTime);
static int mnu_doUnhostGame(MenuProc_t * mproc, float deltaTime);
static int mnu_doJoinGame(MenuProc_t * mproc, float deltaTime);

static int mnu_doIngameQuit( MenuProc_t * mproc, float deltaTime );
static int mnu_doIngameInventory( MenuProc_t * mproc, float deltaTime );
static int mnu_doIngameEndGame( MenuProc_t * mproc, float deltaTime );

static int mnu_handleKeyboard(  MenuProc_t * mproc  );

static retval_t mnu_upload_game_info(Game_t * gs, MenuProc_t * ms);
static retval_t mnu_ensure_game(MenuProc_t * mproc, struct sGame ** optional);

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
enum e_mnu_states
{
  MM_Begin,
  MM_Entering,
  MM_Running,
  MM_Leaving,
  MM_Finish,
};

typedef enum e_mnu_states MenuProcs;



//--------------------------------------------------------------------------------------------

struct s_options_data
{
  // Audio
  STRING sz_maxsoundchannel;
  STRING sz_buffersize;
  STRING sz_soundvolume;
  STRING sz_musicvolume;

  bool_t  allow_sound;      //Allow playing of sound?
  int     soundvolume;     //Volume of sounds played
  int     maxsoundchannel; //Max number of sounds playing at the same time
  int     buffersize;      //Buffer size set in setup.txt

  bool_t  allow_music;     // Allow music and loops?
  int     musicvolume;    //The sound volume of music

  // VIDEO
  STRING sz_maxmessage;
  STRING sz_scrd;
  STRING sz_scrz;

  bool_t  zreflect;              // Reflection z buffering?
  int     maxtotalmeshvertices;  // of vertices
  bool_t  fullscreen;            // Start in CData.fullscreen?
  int     scrd;                   // Screen bit depth
  int     scrx;                 // Screen X size
  int     scry;                 // Screen Y size
  int     scrz;                  // Screen z-buffer depth ( 8 unsupported )
  int     maxmessage;    //
  bool_t  messageon;           // Messages?
  int     wraptolerance;      // Status_t bar
  bool_t  staton;             // Draw the status bars?
  bool_t  render_overlay;     //Draw overlay?
  bool_t  render_background;  // Do we render the water as a background?
  GLenum  perspective;        // Perspective correct textures?
  bool_t  dither;             // Dithering?
  Uint8   reffadeor;                  // 255 = Don't fade reflections
  GLenum  shading;                    // Gourad shading?
  bool_t  antialiasing;               // Antialiasing?
  bool_t  refon;                      // Reflections?
  bool_t  shaon;                      // Shadows?
  int     texturefilter;              // Texture filtering?
  bool_t  wateron;                    // Water overlays?
  bool_t  shasprite;                  // Shadow sprites?
  bool_t  phongon;                    // Do phong overlay?
  bool_t  twolayerwateron;            // Two layer water?
  bool_t  overlayvalid;               // Allow large overlay?
  bool_t  backgroundvalid;            // Allow large background?
  bool_t  fogallowed;                 //
  int     particletype;               // Particle Effects image
  Uint8   autoturncamera;             // Type of camera control...
  bool_t  vsync;                      // Wait for vertical sync?
  bool_t  gfxacceleration;            // Force OpenGL GFX acceleration?
};

typedef struct s_options_data OPTIONS_DATA;

OPTIONS_DATA OData;

// TEMPORARY!
#define NET_DONE_SENDING_FILES 10009
#define NET_NUM_FILES_TO_SEND  10010
static STRING mnu_filternamebuffer    = NULL_STRING;
static STRING mnu_display_mode_buffer = NULL_STRING;
static int    mnu_display_mode_index = 0;

#define MAXWIDGET 100
static int       mnu_widgetCount;
static ui_Widget_t mnu_widgetList[MAXWIDGET];

static int mnu_selectedPlayerCount = 0;
static int mnu_selectedInput[PLALST_COUNT] = {0};
static PLA_REF mnu_selectedPlayer[PLALST_COUNT];

/* Variables for the model viewer in mnu_doChoosePlayer */
static float  mnu_modelAngle = 0;
static Uint32 mnu_modelIndex = 0;

/* Copyright text variables.  Change these to change how the copyright text appears */
const char mnu_copyrightText[] = "Welcome to Egoboo!\nhttp://egoboo.sourceforge.net\nVersion 2.7.x";
static int mnu_copyrightLeft = 0;
static int mnu_copyrightTop  = 0;

/* Options info text variables.  Change these to change how the options text appears */
const char mnu_optionsText[] = "Change your audio, input and video\nsettings here.";
static int mnu_optionsTextLeft = 0;
static int mnu_optionsTextTop  = 0;

/* Button labels.  Defined here for consistency's sake, rather than leaving them as constants */
static const char *mnu_mainMenuButtons[] =
{
  "New Game",
  "Load Game",
  "Network",
  "Options",
  "Quit",
  ""
};

static const char *mnu_singlePlayerButtons[] =
{
  "New Player_t",
  "Load Saved Player_t",
  "Back",
  ""
};

const char *netMenuButtons[] =
{
  "Host Game",
  "Join Game",
  "Back",
  ""
};

static const char *mnu_multiPlayerButtons[] =
{
  "Start Game",
  "Back",
  ""
};

static const char *mnu_optionsButtons[] =
{
  "Audio Options",
  "Input Controls",
  "Video Settings",
  "Back",
  ""
};

static const char *mnu_audioOptionsText[] =
{
  "N/A",    //Enable sound
  "N/A",    //Sound volume
  "N/A",    //Enable music
  "N/A",    //Music volume
  "N/A",    //Sound channels
  "N/A",    //Sound buffer
  "Save Settings",
  ""
};

static const char * mnu_videoOptionsText[] =
{
  "N/A",  // Antialaising
  "N/A",  // Particle Effects
  "N/A",  // Fast & ugly
  "N/A",  // Fullscreen
  "N/A",  // Reflections
  "N/A",  // Texture filtering
  "N/A",  // Shadows
  "N/A",  // Z bit
  "N/A",  // Force V-Synch
  "N/A",  // 3D effects
  "N/A",  // Multi water layer
  "N/A",  // Max messages
  "N/A",  // Screen resolution
  "Save Settings",
  ""
};

/* Button position for the "easy" menus, like the main one */
static int mnu_buttonLeft = 0;
static int mnu_buttonTop = 0;

static bool_t mnu_startNewPlayer = bfalse;

/* The font used for drawing text.  It's smaller than the button font */
static TTFont_t *mnu_Font = NULL;

//--------------------------------------------------------------------------------------------
static bool_t mnu_removeSelectedPlayerInput( PLA_REF player, Uint32 input );

//--------------------------------------------------------------------------------------------
static void init_options_data()
{
  // Audio
  OData.sz_maxsoundchannel[0] = EOS;
  OData.sz_buffersize[0]      = EOS;
  OData.sz_soundvolume[0]     = EOS;
  OData.sz_musicvolume[0]     = EOS;

  OData.allow_sound = CData.allow_sound;
  OData.soundvolume = CData.soundvolume;
  OData.maxsoundchannel = CData.maxsoundchannel;
  OData.buffersize = CData.buffersize;

  OData.allow_music = CData.allow_music;
  OData.musicvolume = CData.musicvolume;

  // VIDEO
  OData.sz_maxmessage[0] = EOS;
  OData.sz_scrd[0]       = EOS;
  OData.sz_scrz[0]       = EOS;

  OData.zreflect = CData.zreflect;
  OData.maxtotalmeshvertices = CData.maxtotalmeshvertices;
  OData.fullscreen = CData.fullscreen;
  OData.scrd = CData.scrd;
  OData.scrx = CData.scrx;
  OData.scry = CData.scry;
  OData.scrz = CData.scrz;
  OData.maxmessage = CData.maxmessage;
  OData.messageon = CData.messageon;
  OData.wraptolerance = CData.wraptolerance;
  OData.staton = CData.staton;
  OData.render_overlay = CData.render_overlay;
  OData.render_background = CData.render_background;
  OData.perspective = CData.perspective;
  OData.dither = CData.dither;
  OData.reffadeor = CData.reffadeor;
  OData.shading = CData.shading;
  OData.antialiasing = CData.antialiasing;
  OData.refon = CData.refon;
  OData.shaon = CData.shaon;
  OData.texturefilter = CData.texturefilter;
  OData.wateron = CData.wateron;
  OData.shasprite = CData.shasprite;
  OData.phongon = CData.phongon;
  OData.twolayerwateron = CData.twolayerwateron;
  OData.overlayvalid = CData.overlayvalid;
  OData.backgroundvalid = CData.backgroundvalid;
  OData.fogallowed     = CData.fogallowed;
  OData.particletype   = CData.particletype;
  OData.autoturncamera = CData.autoturncamera;
  OData.vsync          = CData.vsync;
};

//--------------------------------------------------------------------------------------------
static void update_options_data()
{
  // Audio
  CData.allow_sound = OData.allow_sound;
  CData.soundvolume = OData.soundvolume;
  CData.maxsoundchannel = OData.maxsoundchannel;
  CData.buffersize = OData.buffersize;

  CData.allow_music = OData.allow_music;
  CData.musicvolume = OData.musicvolume;

  // VIDEO
  CData.zreflect = OData.zreflect;
  //CData.maxtotalmeshvertices = OData.maxtotalmeshvertices;
  CData.fullscreen = OData.fullscreen;
  CData.scrd = OData.scrd;
  CData.scrx = OData.scrx;
  CData.scry = OData.scry;
  CData.scrz = OData.scrz;
  CData.maxmessage = OData.maxmessage;
  CData.messageon = OData.messageon;
  CData.wraptolerance = OData.wraptolerance;
  CData.staton = OData.staton;
  CData.render_overlay = OData.render_overlay;
  CData.perspective = OData.perspective;
  CData.dither = OData.dither;
  CData.reffadeor = OData.reffadeor;
  CData.shading = OData.shading;
  CData.antialiasing = OData.antialiasing;
  CData.refon = OData.refon;
  CData.shaon = OData.shaon;
  CData.texturefilter = OData.texturefilter;
  CData.wateron = OData.wateron;
  CData.shasprite = OData.shasprite;
  CData.phongon = OData.phongon;
  CData.twolayerwateron = OData.twolayerwateron;
  CData.overlayvalid = OData.overlayvalid;
  CData.backgroundvalid = OData.backgroundvalid;
  CData.fogallowed = OData.fogallowed;
  CData.autoturncamera = OData.autoturncamera;
  CData.vsync          = OData.vsync;
};


//--------------------------------------------------------------------------------------------
bool_t mnu_slideButton( ui_Widget_t * pw2, ui_Widget_t * pw1, float dx, float dy )
{
  if ( NULL == pw2 || NULL == pw1 ) return bfalse;
  if ( !ui_copyWidget( pw2, pw1 ) ) return bfalse;

  pw2->x += dx;
  pw2->y += dy;

  return btrue;
}

//--------------------------------------------------------------------------------------------
// "Slidy" buttons used in some of the menus.  They're shiny.
struct sSlidyButtonState
{
  float lerp;
  int   top;
  int   left;

  int         button_count;
  ui_Widget_t * button_list;
};

struct sSlidyButtonState SlidyButtonState_t;

//--------------------------------------------------------------------------------------------
int mnu_initWidgetsList( ui_Widget_t wlist[], int wmax, const char * text[] )
{
  int i, cnt;
  cnt = 0;
  for ( i = 0; i < wmax && EOS != text[i][0]; i++, cnt++ )
  {
    ui_initWidget( wlist + i, i, mnu_Font, text[i], NULL, mnu_buttonLeft, mnu_buttonTop + ( i * 35 ), 200, 30 );
  };

  return cnt;
};

//--------------------------------------------------------------------------------------------
static void initSlidyButtons( float lerp, ui_Widget_t * buttons, int count )
{
  SlidyButtonState_t.lerp         = lerp;
  SlidyButtonState_t.button_list  = buttons;
  SlidyButtonState_t.button_count = count;
}

//--------------------------------------------------------------------------------------------
static void mnu_updateSlidyButtons( float deltaTime )
{
  SlidyButtonState_t.lerp += ( deltaTime * 1.5f );
}

//--------------------------------------------------------------------------------------------
static void mnu_drawSlidyButtons()
{
  int i;
  ui_Widget_t wtmp;

  for ( i = 0; i < SlidyButtonState_t.button_count;  i++ )
  {
    float dx;

    dx = - ( 360 - i * 35 )  * SlidyButtonState_t.lerp;

    mnu_slideButton( &wtmp, &SlidyButtonState_t.button_list[i], dx, 0 );
    ui_doButton( &wtmp );
  }
}

//--------------------------------------------------------------------------------------------
// initMenus()
// Loads resources for the menus, and figures out where things should
// be positioned.  If we ever allow changing resolution on the fly, this
// function will have to be updated/called more than once.
//

int initMenus()
{
  int i;

  snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.uifont_ttf );
  mnu_Font = fnt_loadFont( CStringTmp1, CData.uifont_points_small );
  if ( NULL == mnu_Font )
  {
    log_error( "Could not load the menu font!\n" );
    return 0;
  }

  // Figure out where to draw the buttons
  mnu_buttonLeft = 40;
  mnu_buttonTop = gfxState.surface->h - 20;
  for ( i = 0; mnu_mainMenuButtons[i][0] != 0; i++ )
  {
    mnu_buttonTop -= 35;
  }

  // Figure out where to draw the copyright text
  mnu_copyrightLeft = 0;
  mnu_copyrightLeft = 0;
  fnt_getTextBoxSize( mnu_Font, mnu_copyrightText, 20, &mnu_copyrightLeft, &mnu_copyrightTop );
  // Draw the copyright text to the right of the buttons
  mnu_copyrightLeft = 280;
  // And relative to the bottom of the screen
  mnu_copyrightTop = gfxState.surface->h - mnu_copyrightTop - 20;

  // Figure out where to draw the options text
  mnu_optionsTextLeft = 0;
  mnu_optionsTextLeft = 0;
  fnt_getTextBoxSize( mnu_Font, mnu_optionsText, 20, &mnu_optionsTextLeft, &mnu_optionsTextTop );
  // Draw the copyright text to the right of the buttons
  mnu_optionsTextLeft = 280;
  // And relative to the bottom of the screen
  mnu_optionsTextTop = gfxState.surface->h - mnu_optionsTextTop - 20;

  return 1;
}

//--------------------------------------------------------------------------------------------
int mnu_doMain( MenuProc_t * mproc, float deltaTime )
{
  static MenuProcs menuState = MM_Begin;
  static GLtexture background;
  static ui_Widget_t wBackground, wCopyright;
  static float lerp;
  static int menuChoice = 0;

  int result = 0;

  switch ( menuState )
  {
    case MM_Begin:
      printf("mnu_doMain()\n");

      GLtexture_new( &background );

      // set up menu variables
      snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.mnu_dir, CData.menu_main_bitmap );
      GLtexture_Load( GL_TEXTURE_2D, &background, CStringTmp1, INVALID_TEXTURE );

      mnu_widgetCount = mnu_initWidgetsList( mnu_widgetList, MAXWIDGET, mnu_mainMenuButtons );
      initSlidyButtons( 1.0f, mnu_widgetList, mnu_widgetCount );

      ui_initWidget( &wBackground, UI_Invalid, ui_getTTFont(), NULL, &background, ( gfxState.surface->w - background.imgW ), 0, 0, 0 );
      ui_initWidget( &wCopyright,  UI_Invalid, ui_getTTFont(), mnu_copyrightText, NULL, mnu_copyrightLeft, mnu_copyrightTop, 0, 0 );

      menuChoice = 0;
      menuState = MM_Entering;
      break;

    case MM_Entering:
      // do buttons sliding in animation, and background fading in
      // background

      glColor4f( 1, 1, 1, 1 - SlidyButtonState_t.lerp );
      ui_drawImage( &wBackground );

      // "Copyright" text
      ui_drawTextBox( &wCopyright, 20 );

      mnu_drawSlidyButtons();
      mnu_updateSlidyButtons( -deltaTime );

      // Let lerp wind down relative to the time elapsed
      if ( SlidyButtonState_t.lerp <= 0.0f )
      {
        menuState = MM_Running;
      }

      break;

    case MM_Running:
      // Do normal run
      // Background
      glColor4f( 1, 1, 1, 1 );
      ui_drawImage( &wBackground );

      // "Copyright" text
      ui_drawTextBox( &wCopyright, 20 );

      // Buttons
      if ( BUTTON_UP == ui_doButton( mnu_widgetList + 0 ) )
      {
        // begin single player stuff
        menuChoice = 1;
      }

      if ( BUTTON_UP == ui_doButton( mnu_widgetList + 1 ) )
      {
        // begin multi player stuff
        menuChoice = 2;
      }

      if ( BUTTON_UP == ui_doButton( mnu_widgetList + 2 ) )
      {
        // go to options menu
        menuChoice = 3;
      }

      if ( BUTTON_UP == ui_doButton( mnu_widgetList + 3 ) )
      {
        // quit game
        menuChoice = 4;
      }

      if ( BUTTON_UP == ui_doButton( mnu_widgetList + 4 ) )
      {
        // quit game
        menuChoice = -1;
      }

      if ( menuChoice != 0 )
      {
        menuState = MM_Leaving;
        mnu_widgetCount = mnu_initWidgetsList( mnu_widgetList, MAXWIDGET, mnu_mainMenuButtons );
        initSlidyButtons( 0.0f, mnu_widgetList, mnu_widgetCount );
      }
      break;

    case MM_Leaving:
      // Do buttons sliding out and background fading
      // Do the same stuff as in MM_Entering, but backwards
      glColor4f( 1, 1, 1, 1 - SlidyButtonState_t.lerp );
      ui_drawImage( &wBackground );

      // "Copyright" text
      ui_drawTextBox( &wCopyright, 20 );

      // Buttons
      mnu_drawSlidyButtons();
      mnu_updateSlidyButtons( deltaTime );
      if ( SlidyButtonState_t.lerp >= 1.0f )
      {
        menuState = MM_Finish;
      }
      break;

    case MM_Finish:
      // Free the background texture; don't need to hold onto it
      menuState = MM_Begin; // Make sure this all resets next time mnu_doMain is called

      // Set the next menu to load
      result = menuChoice;
      ui_Reset();

      GLtexture_delete( &background );
      break;

  };

  return result;
}

//--------------------------------------------------------------------------------------------
int mnu_doSinglePlayer( MenuProc_t * mproc, float deltaTime )
{
  static MenuProcs menuState = MM_Begin;
  static GLtexture background;
  static ui_Widget_t wBackground, wCopyright;
  static int menuChoice;
  int result = 0;

  switch ( menuState )
  {
    case MM_Begin:
      printf("mnu_doSinglePlayer()\n");

      GLtexture_new( &background );

      // Load resources for this menu
      snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.mnu_dir, CData.menu_advent_bitmap );
      GLtexture_Load( GL_TEXTURE_2D, &background, CStringTmp1, INVALID_KEY );

      mnu_widgetCount = mnu_initWidgetsList( mnu_widgetList, MAXWIDGET, mnu_singlePlayerButtons );
      initSlidyButtons( 1.0f, mnu_widgetList, mnu_widgetCount );

      ui_initWidget( &wBackground, UI_Invalid, ui_getTTFont(), NULL, &background, ( gfxState.surface->w - background.imgW ), 0, 0, 0 );
      ui_initWidget( &wCopyright,  UI_Invalid, ui_getTTFont(), mnu_copyrightText, NULL, mnu_copyrightLeft, mnu_copyrightTop, 0, 0 );

      menuChoice = 0;
      menuState = MM_Entering;
      break;

    case MM_Entering:
      glColor4f( 1, 1, 1, 1 - SlidyButtonState_t.lerp );

      // Draw the background image
      ui_drawImage( &wBackground );

      // "Copyright" text
      ui_drawTextBox( &wCopyright, 20 );

      mnu_drawSlidyButtons();
      mnu_updateSlidyButtons( -deltaTime );

      if ( SlidyButtonState_t.lerp <= 0.0f )
        menuState = MM_Running;

      break;

    case MM_Running:

      // Draw the background image
      ui_drawImage( &wBackground );

      // "Copyright" text
      ui_drawTextBox(  &wCopyright, 20  );

      // Buttons
      if ( BUTTON_UP == ui_doButton( mnu_widgetList + 0 ) )
      {
        menuChoice = 1;
      }

      if ( BUTTON_UP == ui_doButton( mnu_widgetList + 1 ) )
      {
        menuChoice = 2;
      }

      if ( BUTTON_UP == ui_doButton( mnu_widgetList + 2 ) )
      {
        menuChoice = 3;
      }

      if ( menuChoice != 0 )
      {
        menuState = MM_Leaving;
        mnu_widgetCount = mnu_initWidgetsList( mnu_widgetList, MAXWIDGET, mnu_singlePlayerButtons );
        initSlidyButtons( 0.0f, mnu_widgetList, mnu_widgetCount );
      }

      break;

    case MM_Leaving:
      // Do buttons sliding out and background fading
      // Do the same stuff as in MM_Entering, but backwards
      glColor4f( 1, 1, 1, 1 - SlidyButtonState_t.lerp );
      ui_drawImage( &wBackground );

      // "Copyright" text
      ui_drawTextBox( &wCopyright, 20 );

      mnu_drawSlidyButtons();
      mnu_updateSlidyButtons( deltaTime );

      if ( SlidyButtonState_t.lerp >= 1.0f )
      {
        menuState = MM_Finish;
      }
      break;

    case MM_Finish:

      // Set the next menu to load
      result = menuChoice;

      // And make sure that if we come back to this menu, it resets
      // properly
      menuState = MM_Begin;
      ui_Reset();

      // delete the background texture as we exit
      GLtexture_delete( &background );

      break;
  }

  return result;
}

//--------------------------------------------------------------------------------------------
// TODO: I totally fudged the layout of this menu by adding an offset for when
// the game isn't in 640x480.  Needs to be fixed.
int mnu_doChooseModule( MenuProc_t * mproc, float deltaTime )
{
  static MenuProcs menuState = MM_Begin;
  static int menuChoice = 0;

  static int startIndex;
  static GLtexture background;
  static ui_Widget_t wBackground, wCopyright, wtmp;
  static char description[1024];
  static int local_selectedModule = -1;

  static int moduleMenuOffsetX;
  static int moduleMenuOffsetY;

  int result = 0;

  char * pchar, * pchar_end;
  size_t i;
  int    j, x, y;
  char txtBuffer[128];

  Server_t * sv;

  //// make sure a Client_t state is defined
  //MenuProc_ensure_client(mproc, NULL);

  // make sure a Server_t state is defined
  MenuProc_ensure_server(mproc, NULL);
  sv = mproc->sv;

  switch ( menuState )
  {
    case MM_Begin:
      printf("mnu_doChooseModule()\n");

      GLtexture_new( &background );

      // Reload all avalible modules (Hidden ones may pop up after the player has completed one)
      sv->loc_mod_count = mnu_load_mod_data(mproc, sv->loc_mod, MAXMODULE);

      // Load font & background
      snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.mnu_dir, CData.menu_sleepy_bitmap );
      GLtexture_Load( GL_TEXTURE_2D, &background, CStringTmp1, INVALID_KEY );

      ui_initWidget( &wBackground, UI_Invalid, ui_getTTFont(), NULL, &background, ( gfxState.surface->w - background.imgW ), 0, 0, 0 );
      ui_initWidget( &wCopyright,  UI_Invalid, ui_getTTFont(), mnu_copyrightText, NULL, mnu_copyrightLeft, mnu_copyrightTop, 0, 0 );

      startIndex = 0;
      mnu_prime_modules(mproc);
      sv->loc_modtxt.val = -2;

      description[0] = EOS;

      // Find the modules that we want to allow loading for.  If mnu_startNewPlayer
      // is true, we want ones that don't allow imports (e.g. starter modules).
      // Otherwise, we want modules that allow imports
      if ( mnu_selectedPlayerCount > 0 )
      {
        for ( i = 0; i < sv->loc_mod_count; i++ )
        {
          if ( sv->loc_mod[i].importamount >= mnu_selectedPlayerCount )
          {
            mproc->validModules[mproc->validModules_count] = i;
            mproc->validModules_count++;
          }
        }
      }
      else
      {
        // Starter modules
        for ( i = 0; i < sv->loc_mod_count; i++ )
        {
          if ( sv->loc_mod[i].importamount == 0 && sv->loc_mod[i].maxplayers == 1 )
          {
            mproc->validModules[mproc->validModules_count] = i;
            mproc->validModules_count++;
          };
        }
      };

      // Figure out at what offset we want to draw the module menu.
      moduleMenuOffsetX = ( gfxState.surface->w - DEFAULT_SCREEN_W ) / 2;
      moduleMenuOffsetY = ( gfxState.surface->h - DEFAULT_SCREEN_H ) / 2;

      // navigation buttons
      ui_initWidget( mnu_widgetList + 0, 0, mnu_Font, "<-", NULL, moduleMenuOffsetX + 20, moduleMenuOffsetY + 74, 30, 30 );
      ui_initWidget( mnu_widgetList + 1, 1, mnu_Font, "->", NULL, moduleMenuOffsetX + 590, moduleMenuOffsetY + 74, 30, 30 );
      ui_initWidget( mnu_widgetList + 2, 2, mnu_Font, "Select Module", NULL, moduleMenuOffsetX + 327, moduleMenuOffsetY + 173, 200, 30 );
      ui_initWidget( mnu_widgetList + 3, 3, mnu_Font, "Back", NULL, moduleMenuOffsetX + 327, moduleMenuOffsetY + 208, 200, 30 );

      // Module "windows"
      ui_initWidget( mnu_widgetList + 4, 4, mnu_Font, NULL, NULL, moduleMenuOffsetX, moduleMenuOffsetY, 138, 138 );
      ui_initWidget( mnu_widgetList + 5, 5, mnu_Font, NULL, NULL, moduleMenuOffsetX, moduleMenuOffsetY, 138, 138 );
      ui_initWidget( mnu_widgetList + 6, 6, mnu_Font, NULL, NULL, moduleMenuOffsetX, moduleMenuOffsetY, 138, 138 );

      // Module description
      ui_initWidget( mnu_widgetList + 7, UI_Invalid, mnu_Font, description, NULL, moduleMenuOffsetX + 21, moduleMenuOffsetY + 173, 291, 230 );

      x = ( gfxState.surface->w / 2 ) - ( background.imgW / 2 );
      y = gfxState.surface->h - background.imgH;

      menuChoice = 0;
      menuState = MM_Entering;
      break;

    case MM_Entering:
      menuState = MM_Running;
      break;

    case MM_Running:
      // Draw the background
      glColor4f( 1, 1, 1, 1 );
      ui_drawImage( &wBackground );

      // Draw the arrows to pick modules
      if ( BUTTON_UP == ui_doButton( mnu_widgetList + 0 ) )
      {
        startIndex--;
        if ( startIndex < 0 ) startIndex += mproc->validModules_count;
      }

      if ( BUTTON_UP == ui_doButton( mnu_widgetList + 1 ) )
      {
        startIndex++;
        if ( startIndex >= mproc->validModules_count ) startIndex -= mproc->validModules_count;
      }

      // Draw buttons for the modules that can be selected
      x = 93;
      y = 20;
      for ( i = 0; i < 3; i++ )
      {
        int imod;

        if(mproc->validModules_count > 0)
        {
          j = ( i + startIndex ) % mproc->validModules_count;
          imod = mproc->validModules[j];

          mnu_widgetList[4+i].img = mproc->TxTitleImage + sv->loc_mod[imod].tx_title_idx;
        }
        else
        {
          j = -1;
        }

        mnu_slideButton( &wtmp, mnu_widgetList + 4+i, x, y );

        if ( BUTTON_UP == ui_doImageButton( &wtmp ) )
        {
          mproc->selectedModule = j;
        }

        x += 138 + 20; // Width of the button, and the spacing between buttons
      }

      // Draw the module description as a button and a text box
      ui_drawButton( mnu_widgetList + 7);
      ui_drawTextBox( mnu_widgetList + 7, 20 );

      // And draw the next & back buttons
      if ( BUTTON_UP == ui_doButton( mnu_widgetList + 2 ) )
      {
        // go to the next menu with this module selected
        menuState = MM_Leaving;
      }

      if ( BUTTON_UP == ui_doButton( mnu_widgetList + 3 ) )
      {
        // Signal mnu_Run to go back to the previous menu
        mproc->selectedModule = -1;
        menuState = MM_Leaving;
      }

      // Draw the text description of the selected module
      if ( mproc->selectedModule != local_selectedModule )
      {
        MOD_INFO * mi;

        local_selectedModule = mproc->selectedModule;
        mi = sv->loc_mod + mproc->validModules[local_selectedModule];

        // initialize the pointers to the description
        pchar = description;
        pchar_end = pchar + sizeof(description) - 1;

        y = 173 + 5;
        x = 21 + 5;

        // module name
        pchar += snprintf(pchar, pchar_end - pchar, "%s\n", mi->longname);

        // module rank
        pchar += snprintf( pchar, pchar_end - pchar, "Difficulty: %s\n", mi->rank );

        // number of players
        if ( mi->maxplayers > 1 )
        {
          if ( mi->minplayers == mi->maxplayers )
          {
            pchar += snprintf( pchar, pchar_end - pchar, "%d Players\n", mi->minplayers );
          }
          else
          {
            pchar += snprintf( pchar, pchar_end - pchar, "%d - %d Players\n", mi->minplayers, mi->maxplayers );
          }
        }
        else
        {
          if ( mi->importamount == 0 )
          {
            pchar += snprintf( pchar, pchar_end - pchar, "Starter Module\n" );
          }
          else
          {
            pchar += snprintf( pchar, pchar_end - pchar, "Special Module\n" );
          }
        }

        // And finally, the summary
        snprintf( txtBuffer, sizeof( txtBuffer ), "%s" SLASH_STRING "%s" SLASH_STRING "%s" SLASH_STRING "%s", CData.modules_dir, mi->loadname, CData.gamedat_dir, CData.mnu_file );
        if ( mproc->validModules[local_selectedModule] != sv->loc_modtxt.val )
        {
          if ( module_read_summary( txtBuffer, &(sv->loc_modtxt) ) ) 
          {
            sv->loc_modtxt.val = mproc->validModules[local_selectedModule];
          };
        };

        for ( i = 0; i < SUMMARYLINES; i++ )
        {
          if(EOS != sv->loc_modtxt.summary[i][0])
          {
            pchar += snprintf( pchar, pchar_end - pchar, "%s\n", sv->loc_modtxt.summary[i] );
          };
        }

        *pchar = EOS;

        str_decode( description, sizeof(description), description );
      }

      break;

    case MM_Leaving:
      menuState = MM_Finish;
      break;

    case MM_Finish:
      menuState = MM_Begin;

      if ( mproc->selectedModule == -1 )
      {
        // -1 == quit
        result = -1;
      }
      else
      {
        mproc->selectedModule = mproc->validModules[mproc->selectedModule];

        // Save all the module info
        memcpy( &(sv->mod), sv->loc_mod + mproc->selectedModule, sizeof(MOD_INFO));
        sv->selectedModule = mproc->selectedModule;

        //set up the ModState_t
        ModState_renew( &(sv->modstate), sv->loc_mod + mproc->selectedModule, (Uint32)(~0));

        // 1 == start the game
        result = 1;
      }
      ui_Reset();

      // delete the background texture as we exit
      GLtexture_delete( &background );

      break;

  }

  return result;
}

//--------------------------------------------------------------------------------------------
static bool_t mnu_checkSelectedPlayer( PLA_REF player )
{
  int i;
  if ( player < 0 || player > loadplayer_count ) return bfalse;

  for ( i = 0; i < PLALST_COUNT && i < mnu_selectedPlayerCount; i++ )
  {
    if ( mnu_selectedPlayer[i] == player ) return btrue;
  }

  return bfalse;
};

//--------------------------------------------------------------------------------------------
static PLA_REF mnu_getSelectedPlayer( PLA_REF player )
{
  PLA_REF ipla;
  if ( player < 0 || player > loadplayer_count ) return INVALID_PLA;

  for ( ipla = 0; ipla < PLALST_COUNT && ipla < mnu_selectedPlayerCount; ipla++ )
  {
    if ( mnu_selectedPlayer[ REF_TO_INT(ipla) ] == player ) return ipla;
  }

  return INVALID_PLA;
};

//--------------------------------------------------------------------------------------------
static bool_t mnu_addSelectedPlayer( PLA_REF player )
{
  if ( player < 0 || player > loadplayer_count || mnu_selectedPlayerCount >= PLALST_COUNT ) return bfalse;
  if ( mnu_checkSelectedPlayer( player ) ) return bfalse;

  mnu_selectedPlayer[mnu_selectedPlayerCount] = REF_TO_INT(player);
  mnu_selectedInput[mnu_selectedPlayerCount]  = INBITS_NONE;
  mnu_selectedPlayerCount++;

  return btrue;
};

//--------------------------------------------------------------------------------------------
static bool_t mnu_removeSelectedPlayer( PLA_REF player )
{
  int i;
  bool_t found = bfalse;

  if ( player < 0 || player > loadplayer_count || mnu_selectedPlayerCount <= 0 ) return bfalse;

  if ( mnu_selectedPlayerCount == 1 )
  {
    if ( mnu_selectedPlayer[0] == player )
    {
      mnu_selectedPlayerCount = 0;
    };
  }
  else
  {
    for ( i = 0; i < PLALST_COUNT && i < mnu_selectedPlayerCount; i++ )
    {
      if ( mnu_selectedPlayer[i] == player )
      {
        found = btrue;
        break;
      }
    }

    if ( found )
    {
      i++;
      for ( /* nothing */; i < PLALST_COUNT && i < mnu_selectedPlayerCount; i++ )
      {
        mnu_selectedPlayer[i-1] = mnu_selectedPlayer[i];
      }

      mnu_selectedPlayerCount--;
    }
  };

  return found;
};

//--------------------------------------------------------------------------------------------
static bool_t mnu_addSelectedPlayerInput( PLA_REF player, Uint32 input )
{
  int i;
  bool_t retval = bfalse;

  for ( i = 0; i < mnu_selectedPlayerCount; i++ )
  {
    if ( mnu_selectedPlayer[i] == player )
    {
      retval = btrue;
      break;
    }
  }

  if ( retval )
  {
    for ( i = 0; i < mnu_selectedPlayerCount; i++ )
    {
      if ( mnu_selectedPlayer[i] != player )
      {
        // steal this input away from all other players
        mnu_selectedInput[i] &= ~input;
        if ( INBITS_NONE == mnu_selectedInput[i] )
          mnu_removeSelectedPlayerInput( mnu_selectedPlayer[i], input );
      }
    }

    for ( i = 0; i < mnu_selectedPlayerCount; i++ )
    {
      if ( mnu_selectedPlayer[i] == player )
      {
        // actually add in the input
        mnu_selectedInput[i] |= input;
      }
    }
  }

  return retval;
};

//--------------------------------------------------------------------------------------------
static bool_t mnu_removeSelectedPlayerInput( PLA_REF player, Uint32 input )
{
  int i;

  for ( i = 0; i < PLALST_COUNT && i < mnu_selectedPlayerCount; i++ )
  {
    if ( mnu_selectedPlayer[i] == player )
    {
      mnu_selectedInput[i] &= ~input;
      if ( INBITS_NONE == mnu_selectedInput[i] )
        mnu_removeSelectedPlayer( player );

      return btrue;
    }
  }

  return bfalse;
};

//--------------------------------------------------------------------------------------------
void import_selected_players()
{
  // ZZ > Build the import directory

  char srcDir[64], destDir[64];
  int i;
  PLA_REF iplayer;

  // clear out the old directory
  empty_import_directory();

  //make sure the directory exists
  fs_createDirectory( CData.import_dir );

  for ( localplayer_count = 0; localplayer_count < mnu_selectedPlayerCount; localplayer_count++ )
  {
    iplayer = mnu_selectedPlayer[localplayer_count];
    localplayer_control[localplayer_count] = mnu_selectedInput[localplayer_count];
    localplayer_slot[localplayer_count]    = MAXIMPORTPERCHAR * localplayer_count;

    // Copy the character to the import directory
    snprintf( srcDir,  sizeof( srcDir ),  "%s" SLASH_STRING "%s", CData.players_dir, loadplayer[REF_TO_INT(iplayer)].dir );
    snprintf( destDir, sizeof( destDir ), "%s" SLASH_STRING "temp%04d.obj", CData.import_dir, localplayer_slot[localplayer_count] );
    fs_copyDirectory( srcDir, destDir );

    // Copy all of the character's items to the import directory
    for ( i = 0; i < 8; i++ )
    {
      snprintf( srcDir, sizeof( srcDir ), "%s" SLASH_STRING "%s" SLASH_STRING "%d.obj", CData.players_dir, loadplayer[REF_TO_INT(iplayer)].dir, i );
      snprintf( destDir, sizeof( destDir ), "%s" SLASH_STRING "temp%04d.obj", CData.import_dir, localplayer_slot[localplayer_count] + i + 1 );

      fs_copyDirectory( srcDir, destDir );
    }
  };
};

//--------------------------------------------------------------------------------------------
int mnu_doChoosePlayer( MenuProc_t * mproc, float deltaTime )
{
  static MenuProcs menuState = MM_Begin;
  static int menuChoice = 0;

  static GLtexture background;
  static ui_Widget_t wBackground, wCopyright, wtmp;
  static bool_t bquit = bfalse;
  int result = 0;
  static int numVertical, numHorizontal;
  int i, j, k, m, x, y;
  PLA_REF player;
  static GLtexture TxInput[4];
  static Uint32 BitsInput[4];
  static bool_t created_game = bfalse;

  Game_t   * gs = Graphics_getGame(&gfxState);
  Client_t * cl = mproc->cl;
  Server_t * sv = mproc->sv;

  switch ( menuState )
  {
    case MM_Begin:
      printf("mnu_doChoosePlayer()\n");

      GLtexture_new( &background );

      for(i=0; i<4; i++)
      {
        GLtexture_new(TxInput + i);
      };

      if( !EKEY_PVALID(gs) )
      {
        mnu_ensure_game(mproc, &gs);
        gs->proc.Active = bfalse;
        created_game = btrue;
      }

      mnu_selectedPlayerCount = 0;
      mnu_selectedPlayer[0] = 0;

      snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.mnu_dir, CData.menu_sleepy_bitmap );
      GLtexture_Load( GL_TEXTURE_2D, &background, CStringTmp1, INVALID_KEY );

      snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.keybicon_bitmap );
      GLtexture_Load( GL_TEXTURE_2D, TxInput + 0, CStringTmp1, INVALID_KEY );
      BitsInput[0] = INBITS_KEYB;

      snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.mousicon_bitmap );
      GLtexture_Load( GL_TEXTURE_2D, TxInput + 1, CStringTmp1, INVALID_KEY );
      BitsInput[1] = INBITS_MOUS;

      snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.joyaicon_bitmap );
      GLtexture_Load( GL_TEXTURE_2D, TxInput + 2, CStringTmp1, INVALID_KEY );
      BitsInput[2] = INBITS_JOYA;

      snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.joybicon_bitmap );
      GLtexture_Load( GL_TEXTURE_2D, TxInput + 3, CStringTmp1, INVALID_KEY );
      BitsInput[3] = INBITS_JOYB;

      ui_initWidget( &wBackground, UI_Invalid, ui_getTTFont(), NULL, &background, ( gfxState.surface->w - background.imgW ), 0, 0, 0 );
      ui_initWidget( &wCopyright,  UI_Invalid, ui_getTTFont(), mnu_copyrightText, NULL, mnu_copyrightLeft, mnu_copyrightTop, 0, 0 );

      // load information for all the players that could be imported
      check_player_import( gs );

      // set the configuration
      ui_initWidget( mnu_widgetList + 0, 0, mnu_Font, "Select Module", NULL, 40, gfxState.scry - 35*3, 200, 30 );
      ui_initWidget( mnu_widgetList + 1, 1, mnu_Font, "Back", NULL, 40, gfxState.scry - 35*2, 200, 30 );

      ui_initWidget(mnu_widgetList + 0, 0, mnu_Font, "Select Module", NULL, 40, 350, 200, 30);
      ui_initWidget(mnu_widgetList + 1, 1, mnu_Font, "Back", NULL, 40, 385, 200, 30);

      // initialize the selction buttons
      numVertical   = ( gfxState.scry - 35 * 4 - 20 ) / 47;
      numHorizontal = ( gfxState.scrx - 20 * 2 ) / 364;
      x = 20;
      m = 2;
      for ( i = 0; i < numHorizontal; i++ )
      {
        y = 20;
        for ( j = 0; j < numVertical; j++ )
        {
          ui_initWidget( mnu_widgetList + m, m, mnu_Font, NULL, NULL, x, y, 175, 42 );
          ui_widgetAddMask( mnu_widgetList + m, UI_BITS_CLICKED );
          m++;

          for ( k = 0; k < 4; k++, m++ )
          {
            ui_initWidget( mnu_widgetList + m, m, mnu_Font, NULL, TxInput + k, x + 175 + k*42, y, 42, 42 );
            ui_widgetAddMask( mnu_widgetList + m, UI_BITS_CLICKED );
          };

          y += 47;
        }
      x += 180;
      };


      menuChoice = 0;
      menuState = MM_Entering;
      break;

    case MM_Entering:
      menuState = MM_Running;
      break;

    case MM_Running:
      // Figure out how many players we can show without scrolling

      // Draw the background
      ui_drawImage( &wBackground );

      // Draw the player selection buttons
      // I'm being tricky, and drawing two buttons right next to each other
      // for each player: one with the icon, and another with the name.  I'm
      // given those buttons the same ID, so they'll act like the same button.

      player = 0;
      m = 2;
      for ( i = 0; i < numHorizontal && player < loadplayer_count; i++ )
      {
        for ( j = 0; j < numVertical && player < loadplayer_count; j++, player++ )
        {
          PLA_REF splayer;

          mnu_widgetList[m].img  = gs->TxIcon + REF_TO_INT(player);
          mnu_widgetList[m].text = loadplayer[REF_TO_INT(player)].name;

          splayer = mnu_getSelectedPlayer( player );

          if ( INVALID_PLA != splayer )
          {
            mnu_widgetList[m].state |= UI_BITS_CLICKED;
          }
          else
          {
            mnu_widgetList[m].state &= ~UI_BITS_CLICKED;
          }

          if ( BUTTON_DOWN == ui_doImageButtonWithText( mnu_widgetList + m ) )
          {
            if ( 0 != ( mnu_widgetList[m].state & UI_BITS_CLICKED ) && !mnu_checkSelectedPlayer( player ) )
            {
              // button has become clicked
              //mnu_addSelectedPlayer(player);
            }
            else if ( 0 == ( mnu_widgetList[m].state & UI_BITS_CLICKED ) && mnu_checkSelectedPlayer( player ) )
            {
              // button has become unclicked
              mnu_removeSelectedPlayer( player );
            };
          };
          m++;

          splayer = mnu_getSelectedPlayer( player );

          for ( k = 0; k < 4; k++, m++ )
          {
            // make the button states reflect the chosen input devices
            if ( INVALID_PLA == splayer || 0 == ( mnu_selectedInput[ REF_TO_INT(splayer) ] & BitsInput[k] ) )
            {
              mnu_widgetList[m].state &= ~UI_BITS_CLICKED;
            }
            else if ( 0 != ( mnu_selectedInput[REF_TO_INT(splayer)] & BitsInput[k] ) )
            {
              mnu_widgetList[m].state |= UI_BITS_CLICKED;
            }

            if ( BUTTON_DOWN == ui_doImageButton( mnu_widgetList + m ) )
            {
              if ( 0 != ( mnu_widgetList[m].state & UI_BITS_CLICKED ) )
              {
                // button has become clicked
                if ( INVALID_PLA == splayer ) mnu_addSelectedPlayer( player );
                mnu_addSelectedPlayerInput( player, BitsInput[k] );
              }
              else if ( INVALID_PLA != splayer && 0 == ( mnu_widgetList[m].state & UI_BITS_CLICKED ) )
              {
                // button has become unclicked
                mnu_removeSelectedPlayerInput( player, BitsInput[k] );
              };
            };
          };
        }
      };

      // Buttons for going ahead
      if ( BUTTON_UP == ui_doButton( mnu_widgetList + 0 ) )
      {
        menuState = MM_Leaving;
      }

      if ( BUTTON_UP == ui_doButton( mnu_widgetList + 1 ) )
      {
        bquit = btrue;
        mnu_selectedPlayerCount = 0;
        menuState = MM_Leaving;
      }

      break;

    case MM_Leaving:
      menuState = MM_Finish;
      break;

    case MM_Finish:
      menuState = MM_Begin;

      if ( mnu_selectedPlayerCount > 0 )
      {
        MenuProc_ensure_server(mproc, gs);
        if(NULL != mproc->sv) { sv = gs->sv = mproc->sv; }

        mnu_load_mod_data(mproc, sv->loc_mod, MAXMODULE);           // Reload all avalilable modules
        import_selected_players();

        mnu_upload_game_info(gs, mproc);

        result = 1;
      }
      else
      {
        if(created_game) gs->proc.KillMe = btrue;
        result = bquit ? -1 : 1;
      }
      ui_Reset();

      // delete the textures as we exit
      GLtexture_delete( &background );

      for(i=0; i<4; i++)
      {
        GLtexture_delete(TxInput + i);
      };

      break;

  }

  return result;
}


//--------------------------------------------------------------------------------------------
int mnu_doOptions( MenuProc_t * mproc, float deltaTime )
{
  static MenuProcs menuState = MM_Begin;
  static GLtexture background;
  static ui_Widget_t wBackground, wCopyright;
  static float lerp;
  static int menuChoice = 0;

  int result = 0;

  switch ( menuState )
  {
    case MM_Begin:
      printf("mnu_doOptions()\n");

      GLtexture_new( &background );;

      // set up menu variables
      init_options_data();

      snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.mnu_dir, CData.menu_gnome_bitmap );
      GLtexture_Load( GL_TEXTURE_2D, &background, CStringTmp1, INVALID_KEY );

      ui_initWidget( &wBackground, UI_Invalid, ui_getTTFont(), NULL, &background, ( gfxState.surface->w - background.imgW ), 0, 0, 0 );
      ui_initWidget( &wCopyright,  UI_Invalid, ui_getTTFont(), mnu_optionsText, NULL, mnu_optionsTextLeft, mnu_optionsTextTop, 0, 0 );

      mnu_widgetCount = mnu_initWidgetsList( mnu_widgetList, MAXWIDGET, mnu_optionsButtons );
      initSlidyButtons( 1.0f, mnu_widgetList, mnu_widgetCount );

      menuChoice = 0;
      menuState = MM_Entering;
      break;

    case MM_Entering:
      // do buttons sliding in animation, and background fading in
      // background

      //Draw the background
      glColor4f( 1, 1, 1, 1 - SlidyButtonState_t.lerp );
      ui_drawImage( &wBackground );

      // "Copyright" text
      ui_drawTextBox( &wCopyright, 20 );

      mnu_drawSlidyButtons();
      mnu_updateSlidyButtons( -deltaTime );

      // Let lerp wind down relative to the time elapsed
      if ( SlidyButtonState_t.lerp <= 0.0f )
      {
        menuState = MM_Running;
      }

      break;

    case MM_Running:
      // Do normal run
      // Background
      glColor4f( 1, 1, 1, 1 );
      ui_drawImage( &wBackground );

      // "Options" text
      ui_drawTextBox( &wCopyright, 20 );

      // Buttons
      if ( BUTTON_UP == ui_doButton( mnu_widgetList + 0 ) )
      {
        //audio options
        menuChoice = 1;
      }

      if ( BUTTON_UP == ui_doButton( mnu_widgetList + 1 ) )
      {
        //input options
        menuChoice = 2;
      }

      if ( BUTTON_UP == ui_doButton( mnu_widgetList + 2 ) )
      {
        //video options
        menuChoice = 3;
      }

      if ( BUTTON_UP == ui_doButton( mnu_widgetList + 3 ) )
      {
        //back to main menu
        menuChoice = 4;
      }

      if ( menuChoice != 0 )
      {
        menuState = MM_Leaving;
        mnu_widgetCount = mnu_initWidgetsList( mnu_widgetList, MAXWIDGET, mnu_optionsButtons );
        initSlidyButtons( 0.0f, mnu_widgetList, mnu_widgetCount );
      }
      break;

    case MM_Leaving:
      // Do buttons sliding out and background fading
      // Do the same stuff as in MM_Entering, but backwards
      glColor4f( 1, 1, 1, 1 - SlidyButtonState_t.lerp );
      ui_drawImage( &wBackground );

      // "Options" text
      ui_drawTextBox( &wCopyright, 20 );

      // Buttons
      mnu_drawSlidyButtons();
      mnu_updateSlidyButtons( deltaTime );
      if ( SlidyButtonState_t.lerp >= 1.0f )
      {
        menuState = MM_Finish;
      }
      break;

    case MM_Finish:
      menuState = MM_Begin; // Make sure this all resets next time mnu_doMain is called

      // Export some of the menu options
      //update_options_data();

      // Set the next menu to load
      result = menuChoice;
      ui_Reset();

      // delete the background texture as we exit
      GLtexture_delete( &background );

      break;

  }
  return result;
}

//--------------------------------------------------------------------------------------------
int mnu_doAudioOptions( MenuProc_t * mproc, float deltaTime )
{
  static MenuProcs menuState = MM_Begin;
  static GLtexture background;
  static ui_Widget_t wBackground, wCopyright;
  static float lerp;
  static int menuChoice = 0;

  int result = 0;

  switch ( menuState )
  {
    case MM_Begin:
      printf("mnu_doAudioOptions()\n");

      GLtexture_new( &background );;

      // set up menu variables
      snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.mnu_dir, CData.menu_gnome_bitmap );
      GLtexture_Load( GL_TEXTURE_2D, &background, CStringTmp1, INVALID_KEY );

      ui_initWidget( &wBackground, UI_Invalid, ui_getTTFont(), NULL, &background, ( gfxState.surface->w - background.imgW ), 0, 0, 0 );
      ui_initWidget( &wCopyright,  UI_Invalid, ui_getTTFont(), mnu_copyrightText, NULL, mnu_copyrightLeft, mnu_copyrightTop, 0, 0 );

      ui_initWidget( mnu_widgetList + 0, 0, mnu_Font, mnu_audioOptionsText[0], NULL, mnu_buttonLeft + 150, gfxState.surface->h - 270, 100, 30 );
      ui_initWidget( mnu_widgetList + 1, 1, mnu_Font, mnu_audioOptionsText[1], NULL, mnu_buttonLeft + 150, gfxState.surface->h - 235, 100, 30 );
      ui_initWidget( mnu_widgetList + 2, 2, mnu_Font, mnu_audioOptionsText[2], NULL, mnu_buttonLeft + 150, gfxState.surface->h - 165, 100, 30 );
      ui_initWidget( mnu_widgetList + 3, 3, mnu_Font, mnu_audioOptionsText[3], NULL, mnu_buttonLeft + 150, gfxState.surface->h - 130, 100, 30 );
      ui_initWidget( mnu_widgetList + 4, 4, mnu_Font, mnu_audioOptionsText[4], NULL, mnu_buttonLeft + 450, gfxState.surface->h - 200, 100, 30 );
      ui_initWidget( mnu_widgetList + 5, 5, mnu_Font, mnu_audioOptionsText[5], NULL, mnu_buttonLeft + 450, gfxState.surface->h - 165, 100, 30 );
      ui_initWidget( mnu_widgetList + 6, 6, mnu_Font, mnu_audioOptionsText[6], NULL, mnu_buttonLeft, gfxState.surface->h - 60, 200, 30 );

      menuChoice = 0;
      menuState = MM_Entering;
      break;

    case MM_Entering:
      // do buttons sliding in animation, and background fading in
      // background
      glColor4f( 1, 1, 1, 1 - SlidyButtonState_t.lerp );

      //Draw the background
      ui_drawImage( &wBackground );

      //Load the current settings
      if ( OData.allow_sound ) mnu_widgetList[0].text = "On";
      else mnu_widgetList[0].text = "Off";

      snprintf( OData.sz_soundvolume, sizeof( OData.sz_soundvolume ), "%i", OData.soundvolume );
      mnu_widgetList[1].text = OData.sz_soundvolume;

      if ( OData.allow_music ) mnu_widgetList[2].text = "On";
      else mnu_widgetList[2].text = "Off";

      snprintf( OData.sz_musicvolume, sizeof( OData.sz_musicvolume ), "%i", OData.musicvolume );
      mnu_widgetList[3].text = OData.sz_musicvolume;

      snprintf( OData.sz_maxsoundchannel, sizeof( OData.sz_maxsoundchannel ), "%i", OData.maxsoundchannel );
      mnu_widgetList[4].text = OData.sz_maxsoundchannel;

      snprintf( OData.sz_buffersize, sizeof( OData.sz_buffersize ), "%i", OData.buffersize );
      mnu_widgetList[5].text = OData.sz_buffersize;

      //Fall trough
      menuState = MM_Running;
      break;

    case MM_Running:
      // Do normal run
      // Background
      glColor4f( 1, 1, 1, 1 );
      ui_drawImage( &wBackground );

      // TODO : need to make this interactive
      fnt_drawTextBox( mnu_Font, "Sound:", mnu_buttonLeft, gfxState.surface->h - 270, 0, 0, 20 );
      // Buttons
      if ( BUTTON_UP == ui_doButton( mnu_widgetList + 0 ) )
      {
        if ( OData.allow_sound )
        {
          OData.allow_sound = bfalse;
          mnu_widgetList[0].text = "Off";
        }
        else
        {
          OData.allow_sound = btrue;
          mnu_widgetList[0].text = "On";
        }
      }

      // TODO : need to make this interactive
      fnt_drawTextBox( mnu_Font, "Sound Volume:", mnu_buttonLeft, gfxState.surface->h - 235, 0, 0, 20 );
      if ( BUTTON_UP == ui_doButton( mnu_widgetList + 1 ) )
      {
        OData.soundvolume += 5;
        OData.soundvolume %= 100;

        snprintf( OData.sz_soundvolume, sizeof( OData.sz_soundvolume ), "%i", OData.soundvolume );
        mnu_widgetList[1].text = OData.sz_soundvolume;
      }

      // TODO : need to make this interactive
      fnt_drawTextBox( mnu_Font, "Music:", mnu_buttonLeft, gfxState.surface->h - 165, 0, 0, 20 );
      if ( BUTTON_UP == ui_doButton( mnu_widgetList + 2 ) )
      {
        if ( OData.allow_music )
        {
          OData.allow_music = bfalse;
          mnu_widgetList[2].text = "Off";
        }
        else
        {
          OData.allow_music = btrue;
          mnu_widgetList[2].text = "On";
        }
      }

      // TODO : need to make this interactive
      fnt_drawTextBox( mnu_Font, "Music Volume:", mnu_buttonLeft, gfxState.surface->h - 130, 0, 0, 20 );
      if ( BUTTON_UP == ui_doButton( mnu_widgetList + 3 ) )
      {
        OData.musicvolume += 5;
        OData.musicvolume %= 100;

        snprintf( OData.sz_musicvolume, sizeof( OData.sz_musicvolume ), "%i", OData.musicvolume );
        mnu_widgetList[3].text = OData.sz_musicvolume;
      }

      fnt_drawTextBox( mnu_Font, "Sound Channels:", mnu_buttonLeft + 300, gfxState.surface->h - 200, 0, 0, 20 );
      if ( BUTTON_UP == ui_doButton( mnu_widgetList + 4 ) )
      {
        switch ( OData.maxsoundchannel )
        {
          case 128:
            OData.maxsoundchannel = 8;
            break;

          case 8:
            OData.maxsoundchannel = 16;
            break;

          case 16:
            OData.maxsoundchannel = 32;
            break;

          case 32:
            OData.maxsoundchannel = 64;
            break;

          case 64:
            OData.maxsoundchannel = 128;
            break;

          default:
            OData.maxsoundchannel = 64;
            break;

        }

        snprintf( OData.sz_maxsoundchannel, sizeof( OData.sz_maxsoundchannel ), "%i", OData.maxsoundchannel );
        mnu_widgetList[4].text = OData.sz_maxsoundchannel;
      }

      fnt_drawTextBox( mnu_Font, "Buffer Size:", mnu_buttonLeft + 300, gfxState.surface->h - 165, 0, 0, 20 );
      if ( BUTTON_UP == ui_doButton( mnu_widgetList + 5 ) )
      {
        switch ( OData.buffersize )
        {
          case 8192:
            OData.buffersize = 512;
            break;

          case 512:
            OData.buffersize = 1024;
            break;

          case 1024:
            OData.buffersize = 2048;
            break;

          case 2048:
            OData.buffersize = 4096;
            break;

          case 4096:
            OData.buffersize = 8192;
            break;

          default:
            OData.buffersize = 2048;
            break;

        }
        snprintf( OData.sz_buffersize, sizeof( OData.sz_buffersize ), "%i", OData.buffersize );
        mnu_widgetList[5].text = OData.sz_buffersize;
      }

      if ( BUTTON_UP == ui_doButton( mnu_widgetList + 6 ) )
      {
        //save settings and go back
        mnu_saveSettings();
        if ( OData.allow_music )
        {
          snd_play_music( 0, 0, -1 );
        }
        else if ( OData.allow_sound )
        {
          Mix_PauseMusic();
        }
        else
        {
          snd_quit();
        }

        //If the number of sound channels changed, allocate them properly
        if( OData.maxsoundchannel != CData.maxsoundchannel )
        {
          Mix_AllocateChannels( OData.maxsoundchannel );
        }

        //Update options data, user may have changed settings last time she accessed the video menu
        update_options_data();

        menuState = MM_Leaving;
      }

      break;

    case MM_Leaving:
      // Do buttons sliding out and background fading
      // Do the same stuff as in MM_Entering, but backwards
      glColor4f( 1, 1, 1, 1 - SlidyButtonState_t.lerp );
      ui_drawImage( &wBackground );

      //Fall trough
      menuState = MM_Finish;
      break;

    case MM_Finish:
      menuState = MM_Begin; // Make sure this all resets next time mnu_doMain is called

      // update the audio using the info from this menu
      update_options_data();
      snd_synchronize(&CData);
      snd_reopen();

      // Set the next menu to load
      result = 1;
      ui_Reset();

      // delete the background texture as we exit
      GLtexture_delete( &background );

      break;

  }
  return result;
}

//--------------------------------------------------------------------------------------------
int mnu_doVideoOptions( MenuProc_t * mproc, float deltaTime )
{
  static MenuProcs menuState = MM_Begin;
  static GLtexture background;
  static ui_Widget_t wBackground, wCopyright;
  static float lerp;
  static int menuChoice = 0;
  static STRING button_txt;
  int result = 0;

  static STRING mnu_text[13];

  int dmode_min,  dmode, imode_min = 0, cnt;

  switch ( menuState )
  {
    case MM_Begin:
      printf("mnu_doVideoOptions()\n");

      GLtexture_new( &background );;

      // set up menu variables
      snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.mnu_dir, CData.menu_gnome_bitmap );
      GLtexture_Load( GL_TEXTURE_2D, &background, CStringTmp1, INVALID_KEY );

      ui_initWidget( &wBackground, UI_Invalid, ui_getTTFont(), NULL, &background, ( gfxState.surface->w - background.imgW ), 0, 0, 0 );
      ui_initWidget( &wCopyright,  UI_Invalid, ui_getTTFont(), mnu_copyrightText, NULL, mnu_copyrightLeft, mnu_copyrightTop, 0, 0 );

      ui_initWidget( mnu_widgetList + 0,  0, mnu_Font, mnu_text[ 0], NULL, mnu_buttonLeft + 150, gfxState.surface->h - 215, 100, 30 );
      ui_initWidget( mnu_widgetList + 1,  1, mnu_Font, mnu_text[ 1], NULL, mnu_buttonLeft + 150, gfxState.surface->h - 180, 100, 30 );
      ui_initWidget( mnu_widgetList + 2,  2, mnu_Font, mnu_text[ 2], NULL, mnu_buttonLeft + 150, gfxState.surface->h - 145, 100, 30 );
      ui_initWidget( mnu_widgetList + 3,  3, mnu_Font, mnu_text[ 3], NULL, mnu_buttonLeft + 150, gfxState.surface->h - 110, 100, 30 );
      ui_initWidget( mnu_widgetList + 4,  4, mnu_Font, mnu_text[ 4], NULL, mnu_buttonLeft + 150, gfxState.surface->h - 250, 100, 30 );
      ui_initWidget( mnu_widgetList + 5,  5, mnu_Font, mnu_text[ 5], NULL, mnu_buttonLeft + 150, gfxState.surface->h - 285, 130, 30 );
      ui_initWidget( mnu_widgetList + 6,  6, mnu_Font, mnu_text[ 6], NULL, mnu_buttonLeft + 150, gfxState.surface->h - 320, 100, 30 );
      ui_initWidget( mnu_widgetList + 7,  7, mnu_Font, mnu_text[ 7], NULL, mnu_buttonLeft + 450, gfxState.surface->h - 320, 100, 30 );
      ui_initWidget( mnu_widgetList + 8,  8, mnu_Font, mnu_text[ 8], NULL, mnu_buttonLeft + 450, gfxState.surface->h - 285, 100, 30 );
      ui_initWidget( mnu_widgetList + 9,  9, mnu_Font, mnu_text[ 9], NULL, mnu_buttonLeft + 450, gfxState.surface->h - 250, 100, 30 );
      ui_initWidget( mnu_widgetList + 10, 10, mnu_Font, mnu_text[10], NULL, mnu_buttonLeft + 450, gfxState.surface->h - 215, 100, 30 );
      ui_initWidget( mnu_widgetList + 11, 11, mnu_Font, mnu_text[11], NULL, mnu_buttonLeft + 450, gfxState.surface->h - 145,  75, 30 );
      ui_initWidget( mnu_widgetList + 12, 12, mnu_Font, mnu_text[12], NULL, mnu_buttonLeft + 450, gfxState.surface->h - 110, 125, 30 );
      ui_initWidget( mnu_widgetList + 13, 13, mnu_Font, mnu_text[13], NULL, mnu_buttonLeft +   0, gfxState.surface->h -  60, 200, 30 );


      //scan the video_mode_list to find the mode closest to our own
      dmode_min = MAX( ABS( OData.scrx - gfxState.video_mode_list[0]->w ), ABS( OData.scry - gfxState.video_mode_list[0]->h ) );
      imode_min = 0;
      for ( cnt = 1; NULL != gfxState.video_mode_list[cnt]; ++cnt )
      {
        dmode = MAX( ABS( OData.scrx - gfxState.video_mode_list[cnt]->w ), ABS( OData.scry - gfxState.video_mode_list[cnt]->h ) );

        if ( dmode < dmode_min )
        {
          dmode_min = dmode;
          imode_min = cnt;
        };
      };

      mnu_display_mode_index = imode_min;
      if ( dmode_min != 0 )
      {
        OData.scrx = gfxState.video_mode_list[imode_min]->w;
        OData.scry = gfxState.video_mode_list[imode_min]->h;
      };

      menuChoice = 0;
      menuState = MM_Entering;
      break;

    case MM_Entering:
      // do buttons sliding in animation, and background fading in
      // background
      glColor4f( 1, 1, 1, 1 - SlidyButtonState_t.lerp );

      //Draw the background
      ui_drawImage( &wBackground );

      //Load all the current video settings
      if ( OData.antialiasing ) mnu_widgetList[0].text = "On";
      else mnu_widgetList[0].text = "Off";

      switch ( OData.particletype )
      {
        case PART_NORMAL:
          mnu_widgetList[1].text = "Normal";
          break;

        case PART_SMOOTH:
          mnu_widgetList[1].text = "Smooth";
          break;

        case PART_FAST:
          mnu_widgetList[1].text = "Fast";
          break;

        default:
          OData.particletype = PART_NORMAL;
          mnu_widgetList[1].text = "Normal";
          break;
      }

      if ( OData.texturefilter == TX_UNFILTERED )
      {
        snprintf( mnu_widgetList[5].text, sizeof( STRING ), "Unfiltered" );
      }
      else if ( OData.texturefilter == TX_LINEAR )
      {
        snprintf( mnu_widgetList[5].text, sizeof( STRING ), "Linear" );
      }
      else if ( OData.texturefilter == TX_MIPMAP )
      {
        snprintf( mnu_widgetList[5].text, sizeof( STRING ), "Mipmap" );
      }
      else if ( OData.texturefilter == TX_BILINEAR )
      {
        snprintf( mnu_widgetList[5].text, sizeof( STRING ), "Bilinear" );
      }
      else if ( OData.texturefilter == TX_TRILINEAR_1 )
      {
        snprintf( mnu_widgetList[5].text, sizeof( STRING ), "Tlinear 1" );
      }
      else if ( OData.texturefilter == TX_TRILINEAR_2 )
      {
        snprintf( mnu_widgetList[5].text, sizeof( STRING ), "Tlinear 2" );
      }
      else if ( OData.texturefilter >= TX_ANISOTROPIC )
      {
        snprintf( mnu_widgetList[5].text, sizeof( STRING ), "%s%d", "AIso ", gfxState.userAnisotropy );
      }

      if ( OData.autoturncamera == btrue ) mnu_widgetList[2].text = "On";
      else if ( OData.autoturncamera == bfalse ) mnu_widgetList[2].text = "Off";
      else if ( OData.autoturncamera == 255 ) mnu_widgetList[2].text = "Quick";

      if ( OData.fullscreen ) mnu_widgetList[3].text = "True";
      else mnu_widgetList[3].text = "False";

      // Reflections
      if ( OData.refon )
      {
        mnu_widgetList[4].text = "Low";
        if ( OData.reffadeor == 0 )
        {
          mnu_widgetList[4].text = "Medium";
          if ( OData.zreflect ) mnu_widgetList[4].text = "High";
        }
      }
      else mnu_widgetList[4].text = "Off";


      // Max messages
      snprintf( OData.sz_maxmessage, sizeof( OData.sz_maxmessage ), "%i", OData.maxmessage );
      if ( OData.maxmessage > MAXMESSAGE || OData.maxmessage < 0 ) OData.maxmessage = MAXMESSAGE - 1;
      if ( OData.maxmessage == 0 ) snprintf( OData.sz_maxmessage, sizeof( OData.sz_maxmessage ), "None" );         //Set to default
      mnu_widgetList[11].text = OData.sz_maxmessage;

      // Shadows
      if ( OData.shaon )
      {
        mnu_widgetList[6].text = "Normal";
        if ( !OData.shasprite ) mnu_widgetList[6].text = "Best";
      }
      else mnu_widgetList[6].text = "Off";

      // Z depth
      if ( OData.scrz != 32 && OData.scrz != 16 && OData.scrz != 24 )
      {
        OData.scrz = 16;       //Set to default
      }
      snprintf( OData.sz_scrz, sizeof( OData.sz_scrz ), "%i", OData.scrz );      //Convert the integer to a char we can use
      mnu_widgetList[7].text = OData.sz_scrz;

      // Vsync
      if ( OData.vsync ) mnu_widgetList[8].text = "Yes";
      else mnu_widgetList[8].text = "No";

      // 3D Effects
      if ( OData.dither )
      {
        mnu_widgetList[9].text = "Okay";
        if ( OData.overlayvalid && OData.backgroundvalid )
        {
          mnu_widgetList[9].text = "Good";
          if ( GL_NICEST == OData.perspective ) mnu_widgetList[9].text = "Superb";
        }
        else              //Set to defaults
        {
          OData.perspective     = GL_FASTEST;
          OData.backgroundvalid = bfalse;
          OData.overlayvalid    = bfalse;
        }
      }
      else               //Set to defaults
      {
        OData.perspective      = GL_FASTEST;
        OData.backgroundvalid  = bfalse;
        OData.overlayvalid     = bfalse;
        OData.dither           = bfalse;
        mnu_widgetList[9].text = "Off";
      }

      if ( OData.twolayerwateron ) mnu_widgetList[10].text = "On";
      else mnu_widgetList[10].text = "Off";


      if ( NULL == gfxState.video_mode_list[mnu_display_mode_index] )
      {
        mnu_display_mode_index = 0;
        OData.scrx = gfxState.video_mode_list[mnu_display_mode_index]->w;
        OData.scry = gfxState.video_mode_list[mnu_display_mode_index]->h;
      };

      snprintf( mnu_display_mode_buffer, sizeof( mnu_display_mode_buffer ), "%dx%d", OData.scrx, OData.scry );
      mnu_widgetList[12].text = mnu_display_mode_buffer;

      menuState = MM_Running;
      break;

    case MM_Running:
      // Do normal run
      // Background
      glColor4f( 1, 1, 1, 1 );
      ui_drawImage( &wBackground );

      //Antialiasing Button
      fnt_drawTextBox( mnu_Font, "Antialiasing:", mnu_buttonLeft, gfxState.surface->h - 215, 0, 0, 20 );
      if ( BUTTON_UP == ui_doButton( mnu_widgetList + 0 ) )
      {
        if ( OData.antialiasing )
        {
          mnu_widgetList[0].text = "Off";
          OData.antialiasing = bfalse;
        }
        else
        {
          mnu_widgetList[0].text = "On";
          OData.antialiasing = btrue;
        }
      }

      //Color depth
      fnt_drawTextBox( mnu_Font, "Particle Effects:", mnu_buttonLeft, gfxState.surface->h - 180, 0, 0, 20 );
      if ( BUTTON_UP == ui_doButton( mnu_widgetList + 1 ) )
      {

        switch ( OData.particletype )
        {

          case PART_FAST:
            mnu_widgetList[1].text = "Normal";
            OData.particletype = PART_NORMAL;
            break;

          case PART_NORMAL:
            mnu_widgetList[1].text = "Smooth";
            OData.particletype = PART_SMOOTH;
            break;

          case PART_SMOOTH:
            mnu_widgetList[1].text = "Fast";
            OData.particletype = PART_FAST;
            break;

          default:
            mnu_widgetList[1].text = "Normal";
            OData.particletype = PART_NORMAL;
            break;
        }
      }

      //Autoturn camera
      fnt_drawTextBox( mnu_Font, "Autoturn Camera:", mnu_buttonLeft, gfxState.surface->h - 145, 0, 0, 20 );
      if ( BUTTON_UP == ui_doButton( mnu_widgetList + 2 ) )
      {
        if ( OData.autoturncamera == 255 )
        {
          mnu_widgetList[2].text = "Off";
          OData.autoturncamera = bfalse;
        }
        else if ( !OData.autoturncamera )
        {
          mnu_widgetList[2].text = "On";
          OData.autoturncamera = btrue;
        }
        else
        {
          mnu_widgetList[2].text = "Quick";
          OData.autoturncamera = 255;
        }
      }

      //Fullscreen
      fnt_drawTextBox( mnu_Font, "Fullscreen:", mnu_buttonLeft, gfxState.surface->h - 110, 0, 0, 20 );
      if ( BUTTON_UP == ui_doButton( mnu_widgetList + 3 ) )
      {
        if ( OData.fullscreen )
        {
          mnu_widgetList[3].text = "False";
          OData.fullscreen = bfalse;
        }
        else
        {
          mnu_widgetList[3].text = "True";
          OData.fullscreen = btrue;
        }
      }

      //Reflection
      fnt_drawTextBox( mnu_Font, "Reflections:", mnu_buttonLeft, gfxState.surface->h - 250, 0, 0, 20 );
      if ( BUTTON_UP == ui_doButton( mnu_widgetList + 4 ) )
      {
        if ( OData.refon && OData.reffadeor == 0 && OData.zreflect )
        {
          OData.refon = bfalse;
          OData.reffadeor = bfalse;
          OData.zreflect = bfalse;
          mnu_widgetList[4].text = "Off";
        }
        else
        {
          if ( OData.refon && OData.reffadeor == 255 && !OData.zreflect )
          {
            mnu_widgetList[4].text = "Medium";
            OData.reffadeor = 0;
            OData.zreflect = bfalse;
          }
          else
          {
            if ( OData.refon && OData.reffadeor == 0 && !OData.zreflect )
            {
              mnu_widgetList[4].text = "High";
              OData.zreflect = btrue;
            }
            else
            {
              OData.refon = btrue;
              OData.reffadeor = 255;    //Just in case so we dont get stuck at "Low"
              mnu_widgetList[4].text = "Low";
              OData.zreflect = bfalse;
            }
          }
        }
      }

      //Texture Filtering
      fnt_drawTextBox( mnu_Font, "Texture Filtering:", mnu_buttonLeft, gfxState.surface->h - 285, 0, 0, 20 );
      if ( BUTTON_UP == ui_doButton( mnu_widgetList + 5 ) )
      {
        if ( OData.texturefilter == TX_UNFILTERED )
        {
          OData.texturefilter++;
          snprintf( mnu_widgetList[5].text, sizeof( STRING ), "Linear");
        }
        else if ( OData.texturefilter == TX_LINEAR )
        {
          OData.texturefilter++;
          snprintf( mnu_widgetList[5].text, sizeof( STRING ), "Mipmap");
        }
        else if ( OData.texturefilter == TX_MIPMAP )
        {
          OData.texturefilter++;
          snprintf( mnu_widgetList[5].text, sizeof( STRING ), "Bilinear");
        }
        else if ( OData.texturefilter == TX_BILINEAR )
        {
          OData.texturefilter++;
          snprintf( mnu_widgetList[5].text, sizeof( STRING ), "Tlinear 1");
        }
        else if ( OData.texturefilter == TX_TRILINEAR_1 )
        {
          OData.texturefilter++;
          snprintf( mnu_widgetList[5].text, sizeof( STRING ), "Tlinear 2");
        }
        else if ( OData.texturefilter >= TX_TRILINEAR_2 && OData.texturefilter < TX_ANISOTROPIC + gfxState.log2Anisotropy - 1 )
        {
          OData.texturefilter++;
          gfxState.userAnisotropy = 1 << ( OData.texturefilter - TX_ANISOTROPIC + 1 );
          snprintf( mnu_widgetList[5].text, sizeof( STRING ), "%s%d", "AIso ", gfxState.userAnisotropy );
        }
        else
        {
          OData.texturefilter = TX_UNFILTERED;
          snprintf( mnu_widgetList[5].text, sizeof( STRING ), "Unfiltered");
        }
      }

      //Shadows
      fnt_drawTextBox( mnu_Font, "Shadows:", mnu_buttonLeft, gfxState.surface->h - 320, 0, 0, 20 );
      if ( BUTTON_UP == ui_doButton( mnu_widgetList + 6 ) )
      {
        if ( OData.shaon && !OData.shasprite )
        {
          OData.shaon = bfalse;
          OData.shasprite = bfalse;        //Just in case
          mnu_widgetList[6].text = "Off";
        }
        else
        {
          if ( OData.shaon && OData.shasprite )
          {
            mnu_widgetList[6].text = "Best";
            OData.shasprite = bfalse;
          }
          else
          {
            OData.shaon = btrue;
            OData.shasprite = btrue;
            mnu_widgetList[6].text = "Normal";
          }
        }
      }

      //Z bit
      fnt_drawTextBox( mnu_Font, "Z Bit:", mnu_buttonLeft + 300, gfxState.surface->h - 320, 0, 0, 20 );
      if ( BUTTON_UP == ui_doButton( mnu_widgetList + 7 ) )
      {
        switch ( OData.scrz )
        {
          case 32:
            mnu_widgetList[7].text = "16";
            OData.scrz = 16;
            break;

          case 16:
            mnu_widgetList[7].text = "24";
            OData.scrz = 24;
            break;

          case 24:
            mnu_widgetList[7].text = "32";
            OData.scrz = 32;
            break;

          default:
            mnu_widgetList[7].text = "16";
            OData.scrz = 16;
            break;

        }
      }

      ////Fog
      //fnt_drawTextBox( mnu_Font, "Fog Effects:", mnu_buttonLeft + 300, gfxState.surface->h - 285, 0, 0, 20 );
      //if ( BUTTON_UP == ui_doButton( mnu_widgetList + 8 ) )
      //{
      //  if ( OData.fogallowed )
      //  {
      //    OData.fogallowed = bfalse;
      //    mnu_widgetList[8].text = "Disable";
      //  }
      //  else
      //  {
      //    OData.fogallowed = btrue;
      //    mnu_widgetList[8].text = "Enable";
      //  }
      //}

      //V-Synch
      fnt_drawTextBox( mnu_Font, "Wait for Vsync:", mnu_buttonLeft + 300, gfxState.surface->h - 285, 0, 0, 20 );
      if ( BUTTON_UP == ui_doButton( mnu_widgetList + 8 ) )
      {
        if ( OData.fogallowed )
        {
          OData.vsync = bfalse;
          mnu_widgetList[8].text = "Off";
        }
        else
        {
          OData.vsync = btrue;
          mnu_widgetList[8].text = "On";
        }
      }

      //Perspective correction, overlay/background effects and phong mapping
      fnt_drawTextBox( mnu_Font, "3D Effects:", mnu_buttonLeft + 300, gfxState.surface->h - 250, 0, 0, 20 );
      if ( BUTTON_UP == ui_doButton( mnu_widgetList + 9 ) )
      {
        if ( OData.dither && ( GL_NICEST == OData.perspective ) && OData.overlayvalid && OData.backgroundvalid )
        {
          OData.dither           = bfalse;
          OData.perspective      = GL_FASTEST;
          OData.overlayvalid     = bfalse;
          OData.backgroundvalid  = bfalse;
          mnu_widgetList[9].text = "Off";
        }
        else
        {
          if ( !OData.dither )
          {
            mnu_widgetList[9].text = "Okay";
            OData.dither = btrue;
          }
          else
          {
            if (( GL_FASTEST == OData.perspective ) && OData.overlayvalid && OData.backgroundvalid )
            {
              mnu_widgetList[9].text = "Superb";
              OData.perspective = GL_NICEST;
            }
            else
            {
              OData.overlayvalid = btrue;
              OData.backgroundvalid = btrue;
              mnu_widgetList[9].text = "Good";
            }
          }
        }
      }

      //Water Quality
      fnt_drawTextBox( mnu_Font, "Pretty Water:", mnu_buttonLeft + 300, gfxState.surface->h - 215, 0, 0, 20 );
      if ( BUTTON_UP == ui_doButton( mnu_widgetList + 10 ) )
      {
        if ( OData.twolayerwateron )
        {
          mnu_widgetList[10].text = "Off";
          OData.twolayerwateron = bfalse;
        }
        else
        {
          mnu_widgetList[10].text = "On";
          OData.twolayerwateron = btrue;
        }
      }

      //Text messages
      fnt_drawTextBox( mnu_Font, "Max  Messages:", mnu_buttonLeft + 300, gfxState.surface->h - 145, 0, 0, 20 );
      if ( BUTTON_UP == ui_doButton( mnu_widgetList + 11 ) )
      {
        if ( OData.maxmessage != MAXMESSAGE )
        {
          OData.maxmessage++;
          OData.messageon = btrue;
          snprintf( OData.sz_maxmessage, sizeof( OData.sz_maxmessage ), "%i", OData.maxmessage );     //Convert integer to a char we can use
        }
        else
        {
          OData.maxmessage = 0;
          OData.messageon = bfalse;
          snprintf( OData.sz_maxmessage, sizeof( OData.sz_maxmessage ), "None" );
        }
        mnu_widgetList[11].text = OData.sz_maxmessage;
      }

      //Screen Resolution
      fnt_drawTextBox( mnu_Font, "Resolution:", mnu_buttonLeft + 300, gfxState.surface->h - 110, 0, 0, 20 );
      if ( BUTTON_UP == ui_doButton( mnu_widgetList + 12 ) )
      {

        if ( NULL == gfxState.video_mode_list[mnu_display_mode_index] )
        {
          mnu_display_mode_index = 0;
        }
        else
        {
          mnu_display_mode_index++;
          if ( NULL == gfxState.video_mode_list[mnu_display_mode_index] )
          {
            mnu_display_mode_index = 0;
          }
        };

        OData.scrx = gfxState.video_mode_list[mnu_display_mode_index]->w;
        OData.scry = gfxState.video_mode_list[mnu_display_mode_index]->h;
        snprintf( mnu_display_mode_buffer, sizeof( mnu_display_mode_buffer ), "%dx%d", OData.scrx, OData.scry );
        mnu_widgetList[12].text = mnu_display_mode_buffer;
      };

      //Save settings button
      if ( BUTTON_UP == ui_doButton( mnu_widgetList + 13 ) )
      {
        menuChoice = 1;
        mnu_saveSettings();
      }

      if ( menuChoice != 0 )
      {
        menuState = MM_Leaving;
        mnu_widgetCount = mnu_initWidgetsList( mnu_widgetList, MAXWIDGET, mnu_videoOptionsText );
        initSlidyButtons( 0.0f, mnu_widgetList, mnu_widgetCount );
      }
      break;

    case MM_Leaving:
      // Do buttons sliding out and background fading
      // Do the same stuff as in MM_Entering, but backwards
      glColor4f( 1, 1, 1, 1 - SlidyButtonState_t.lerp );
      ui_drawImage( &wBackground );

      // "Options" text
      fnt_drawTextBox( mnu_Font, mnu_optionsText, mnu_optionsTextLeft, mnu_optionsTextTop, 0, 0, 20 );

      //Fall trough
      menuState = MM_Finish;
      break;

    case MM_Finish:
      menuState = MM_Begin; // Make sure this all resets next time mnu_doMain is called

      //Update options data, user may have changed settings last time she accessed the video menu
      update_options_data();

      // Force the program to modify the graphics
      // to get this to work properly, you need to reload all textures!
      {
        // buffer graphics update so we can restore the old state if there is an error
        Graphics_t *pgfx, gfx_new = gfxState;
        Graphics_synch(&gfx_new, &CData);

        // ??? do we have to free this surface ???
        gfxState.surface = NULL;
        pgfx = sdl_set_mode(&gfxState, &gfx_new, btrue);

        if(NULL == pgfx)
        {
          log_error("Cannot set the graphics mode - \n\t\"%s\"\n", SDL_GetError());
          exit(-1);
        }
        else if( pgfx != &gfxState)
        {
          gfxState = gfx_new;
        }

        // !!!! reload the textures here !!!!
      };

      // reset the auto-formatting for the menus
      initMenus();

      //Check which particle image to load
      if      ( CData.particletype == PART_NORMAL ) strncpy( CData.particle_bitmap, "particle_normal.png" , sizeof( STRING ) );
      else if ( CData.particletype == PART_SMOOTH ) strncpy( CData.particle_bitmap, "particle_smooth.png" , sizeof( STRING ) );
      else if ( CData.particletype == PART_FAST  )  strncpy( CData.particle_bitmap, "particle_fast.png" , sizeof( STRING ) );

      // Set the next menu to load
      result = menuChoice;
      ui_Reset();

      // delete the background texture as we exit
      GLtexture_delete( &background );

      break;
  }
  return result;
}


//--------------------------------------------------------------------------------------------
int mnu_doLaunchGame( MenuProc_t * mproc, float deltaTime )
{
  static MenuProcs menuState = MM_Begin;
  static int menuChoice = 0;

  static TTFont_t *font;
  int x, y, i, result = 0;
  static Game_t * gs = NULL;
  

  switch ( menuState )
  {
    case MM_Begin:
      printf("mnu_doLaunchGame()\n");

      font = ui_getTTFont();

      if(!Graphics_hasGame(&gfxState))
      {
        mnu_ensure_game(mproc, &gs);
      }
      else
      {
        // make sure the game has the correct information
        // destroy any extra info
        gs = Graphics_getGame(&gfxState);
        assert( rv_succeed == mnu_upload_game_info(gs, mproc) );
      }

      if(NULL != mproc->cl && -1 != mproc->cl->selectedModule)
      {
        memcpy( &(gs->mod), &(mproc->cl->req_mod), sizeof(MOD_INFO));
        memcpy( &(gs->modstate), &(mproc->cl->req_modstate), sizeof(MOD_INFO));
      }
      else if(NULL != mproc->sv && -1 != mproc->sv->selectedModule)
      {
        memcpy( &(gs->mod), &(mproc->sv->mod), sizeof(MOD_INFO));
        memcpy( &(gs->modstate), &(mproc->sv->modstate), sizeof(MOD_INFO));
      }
      else
      {
        log_error("mnu_doLaunchGame() - \n\tinvalid module configuration");
      }

      menuChoice = 0;
      menuState = MM_Entering;
      break;

    case MM_Entering:
      menuState = MM_Running;

      ui_initWidget( mnu_widgetList + 0, UI_Invalid, mnu_Font, NULL, NULL, 30, 30, gfxState.scrx - 60, gfxState.scry - 60 );

      menuState = MM_Running;
      break;


    case MM_Running:

      ui_drawButton( mnu_widgetList + 0 );

      x = 35;
      y = 35;
      glColor4f( 1, 1, 1, 1 );

      fnt_drawTextFormatted( font, x, y, "Module selected: %s", gs->mod.longname );
      y += 35;

      if ( gs->modstate.import_valid )
      {
        for ( i = 0; i < mnu_selectedPlayerCount; i++ )
        {
          fnt_drawTextFormatted( font, x, y, "Player_t selected: %s", loadplayer[REF_TO_INT(mnu_selectedPlayer[i])].name );
          y += 35;
        };
      }
      else
      {
        fnt_drawText( font, x, y, "Starting a new player." );
      }

      // tell the game not to do any more pageflips so that this menu will persist
      request_pageflip_pause();
      menuState = MM_Leaving;
      break;

    case MM_Leaving:
      // Launch the game here.
      // Do not quit this loop until the game is fully loaded
      ProcState_init( &(gs->proc) );

      menuState = MM_Finish;
      break;

    case MM_Finish:
      result = 1;
      menuState = MM_Begin;
      ui_Reset();
      break;
  }

  return result;
}

//--------------------------------------------------------------------------------------------
int mnu_doNotImplemented( MenuProc_t * mproc, float deltaTime )
{
  static MenuProcs menuState = MM_Begin;
  static int menuChoice = 0;

  static char notImplementedMessage[] = "Not implemented yet!  Check back soon!";
  int result = 0;

  int x, y;
  int w, h;

  switch ( menuState )
  {
    case MM_Begin:
      printf("mnu_doNotImplemented()\n");

      fnt_getTextSize( ui_getTTFont(), notImplementedMessage, &w, &h );
      w += 50; // add some space on the sides

      x = ( gfxState.surface->w - w ) / 2;
      y = ( gfxState.surface->h - 34 ) / 2;

      ui_initWidget( mnu_widgetList + 0, UI_Invalid, mnu_Font, notImplementedMessage, NULL, x, y, w, 30 );

      menuChoice = 0;
      menuState = MM_Entering;
      break;

    case MM_Entering:

      menuState = MM_Running;
      break;


    case MM_Running:

      if ( BUTTON_UP == ui_doButton( mnu_widgetList + 0 ) )
      {
        menuState = MM_Leaving;
      }

      break;


    case MM_Leaving:
      menuState = MM_Finish;
      break;


    case MM_Finish:
      result = 1;

      menuState = MM_Begin;

      ui_Reset();
      break;
  }

  return result;
}






//--------------------------------------------------------------------------------------------
int mnu_doNetworkOff( MenuProc_t * mproc, float deltaTime )
{
  static MenuProcs menuState = MM_Begin;
  static int menuChoice = 0;

  static char networkOffMessage[] = "Network is not on.  Check your cables... ;)";
  int result = 0;

  int x, y;
  int w, h;

  switch ( menuState )
  {
    case MM_Begin:
      printf("mnu_doNetworkOff()\n");

      fnt_getTextSize( ui_getTTFont(), networkOffMessage, &w, &h );
      w += 50; // add some space on the sides

      x = ( gfxState.surface->w - w ) / 2;
      y = ( gfxState.surface->h - 34 ) / 2;

      ui_initWidget( mnu_widgetList + 0, UI_Invalid, mnu_Font, networkOffMessage, NULL, x, y, w, 30 );

      menuChoice = 0;
      menuState = MM_Entering;
      break;

    case MM_Entering:

      menuState = MM_Running;


    case MM_Running:

      if ( BUTTON_UP == ui_doButton( mnu_widgetList + 0 ) )
      {
        menuState = MM_Leaving;
      }

      break;


    case MM_Leaving:
      menuState = MM_Finish;


    case MM_Finish:
      result = 1;

      menuState = MM_Begin;

      ui_Reset();
      break;
  }

  return result;
}
//--------------------------------------------------------------------------------------------
int mnu_RunIngame( MenuProc_t * mproc )
{
  ProcState_t * proc;
  double frameDuration, frameTicks;
  float deltaTime;

  Game_t * gs = Graphics_requireGame(&gfxState);

  if(NULL == mproc || mproc->proc.Terminated) return -1;

  proc = &(mproc->proc);

  ClockState_frameStep( proc->clk );
  frameDuration = ClockState_getFrameDuration( proc->clk );
  frameTicks    = frameDuration * TICKS_PER_SEC;

  deltaTime = 0;
  if(!proc->Paused)
  {
    deltaTime = frameDuration;
    mproc->dUpdate += frameTicks / UPDATESKIP;
  }

  // make sure that the mproc has all of the required info
  MenuProc_resynch( mproc, gs );

  switch ( mproc->whichMenu )
  {
    case mnu_Inventory:
      mproc->MenuResult = mnu_doIngameInventory( mproc, deltaTime );
      if ( mproc->MenuResult != 0 )
      {
        if ( mproc->MenuResult == 1 ) proc->returnValue = -1;
      }
      break;

    case mnu_Quit:
      mproc->MenuResult = mnu_doIngameQuit( mproc, deltaTime );
      if ( mproc->MenuResult != 0 )
      {
        if ( mproc->MenuResult == -1 )
        {
          // menu finished
          mproc->whichMenu = mnu_EndGame;
        }
      }
      break;

    case mnu_EndGame:
      mproc->MenuResult = mnu_doIngameEndGame( mproc, deltaTime );
      if ( mproc->MenuResult != 0 )
      {
        if ( mproc->MenuResult == -1 )
        {
          // menu finished
          proc->returnValue = -1;
        }
      }
      break;

    default:
    case mnu_NotImplemented:
      mproc->MenuResult = mnu_doNotImplemented( mproc, deltaTime );
      if ( mproc->MenuResult != 0 )
      {
        proc->returnValue = -1;
      }
  }

  return proc->returnValue;
}


//--------------------------------------------------------------------------------------------
int mnu_Run( MenuProc_t * mproc )
{
  ProcState_t * proc;
  Game_t     * gs;
  double frameDuration, frameTicks;

  if(NULL == mproc || mproc->proc.Terminated) return -1;

  gs   = Graphics_getGame(&gfxState);
  proc = &(mproc->proc);

  ClockState_frameStep( proc->clk );
  frameDuration = ClockState_getFrameDuration( proc->clk );
  frameTicks    = frameDuration * TICKS_PER_SEC;

  if(!proc->Paused)
  {
    mproc->dUpdate += frameTicks / UPDATESKIP;
  }

  switch ( mproc->whichMenu )
  {
    case mnu_Main:
      mproc->MenuResult = mnu_doMain( mproc, frameDuration );
      if( mnu_handleKeyboard(mproc) < 0 ) mproc->MenuResult = -1;  // handle escape

      if ( mproc->MenuResult != 0 )
      {
        mproc->lastMenu = mnu_Main;
             if ( mproc->MenuResult == -1 ) mproc->proc.KillMe = btrue; // need to request a quit somehow
        else if ( mproc->MenuResult ==  1 )
        { MenuProc_ensure_server(mproc, gs); MenuProc_ensure_client(mproc, gs); mproc->whichMenu = mnu_ChooseModule; mnu_startNewPlayer = btrue; mnu_selectedPlayerCount = 0; }
        else if ( mproc->MenuResult ==  2 )
        { MenuProc_ensure_server(mproc, gs); MenuProc_ensure_client(mproc, gs); mproc->whichMenu = mnu_ChoosePlayer; mnu_startNewPlayer = bfalse; }
        else if ( mproc->MenuResult ==  3 ) mproc->whichMenu = mnu_Network;
        else if ( mproc->MenuResult ==  4 ) mproc->whichMenu = mnu_Options;
      }
      break;

    case mnu_SinglePlayer:
      mproc->MenuResult = mnu_doSinglePlayer( mproc, frameDuration );
      if( mnu_handleKeyboard(mproc) < 0 ) mproc->MenuResult = -1;  // handle escape

      if ( mproc->MenuResult != 0 )
      {
        mproc->lastMenu = mnu_SinglePlayer;
        if ( mproc->MenuResult == 1 )
        {
          mproc->whichMenu = mnu_ChooseModule;
          mnu_startNewPlayer = btrue;
        }
        else if ( mproc->MenuResult == 2 )
        {
          mproc->whichMenu = mnu_ChoosePlayer;
          mnu_startNewPlayer = bfalse;
        }
        else if ( mproc->MenuResult == 3 ) mproc->whichMenu = mnu_Main;
        else mproc->whichMenu = mnu_NewPlayer;
      }
      break;

    case mnu_Network:
      mproc->MenuResult = mnu_doNetwork( mproc, frameDuration);
      if( mnu_handleKeyboard(mproc) < 0 ) mproc->MenuResult = -1;  // handle escape
      if (mproc->MenuResult != 0)
      {
        mproc->lastMenu = mnu_Network;
        if (mproc->MenuResult == -1)     { mproc->lastMenu = mnu_Main; mproc->whichMenu = mnu_Main; }
        else if (mproc->MenuResult == 1) { mproc->lastMenu = mnu_Network; mproc->whichMenu = mnu_HostGame; }
        else if (mproc->MenuResult == 2) { mproc->lastMenu = mnu_Network; mproc->whichMenu = mnu_JoinGame; }
        else if (mproc->MenuResult == 3) { mproc->lastMenu = mnu_Main; mproc->whichMenu = mnu_NetworkOff; }
        else                  { mproc->lastMenu = mnu_Main; mproc->whichMenu = mnu_Main; }
      }
      break;

    case mnu_NetworkOff:
      mproc->MenuResult = mnu_doNetworkOff(mproc, frameDuration);
      if( mnu_handleKeyboard(mproc) < 0 ) mproc->MenuResult = -1;  // handle escape

      if ( mproc->MenuResult != 0 )
      {
        mproc->whichMenu = mproc->lastMenu;
      }
      break;

    case mnu_HostGame:
      mproc->lastMenu = mnu_Network;

      if(NULL!=mproc->sv && mproc->sv->ready)
      {
        mproc->whichMenu = mnu_UnhostGame;
      }
      else
      {
        mproc->MenuResult = mnu_doHostGame(mproc, frameDuration);
        if( mnu_handleKeyboard(mproc) < 0 ) mproc->MenuResult = -1;  // handle escape

        if (mproc->MenuResult != 0)
        {
          if (mproc->MenuResult == -1)
          {
            mproc->whichMenu = mproc->lastMenu;
          }
          else  if (mproc->MenuResult == 1)
          {
            mproc->whichMenu = mnu_UnhostGame;

            // make sure we have a valid game
            mnu_ensure_game(mproc, &gs);

            ModState_renew( (&gs->modstate), &(mproc->sv->mod), (Uint32)(~0));

            if(!gs->modstate.loaded)
            {
              log_info("SDL_main: Loading module %s... ", mproc->sv->mod.loadname);
              gs->modstate.loaded = module_load(gs, mproc->sv->mod.loadname);
              mproc->sv->ready    = gs->modstate.loaded;

              if(gs->modstate.loaded)
              {
                log_info("Succeeded!\n");
              }
              else
              {
                log_info("Failed!\n");
              }
            }

            // make sure the NetFile server is running
            MenuProc_start_netfile(mproc, gs);

            // make sure the server module is running
            MenuProc_start_server(mproc, gs);
          }
        }
      }
      break;

    case mnu_UnhostGame:
      mproc->lastMenu = mnu_Network;
      if(!mproc->sv->ready)
      {
        mproc->whichMenu = mnu_HostGame;
      }
      else
      {
        mproc->MenuResult = mnu_doUnhostGame(mproc, frameDuration);
        if( mnu_handleKeyboard(mproc) < 0 ) mproc->MenuResult = -1;  // handle escape

        if (mproc->MenuResult != 0)
        {
          mproc->whichMenu = mnu_HostGame;
          if (mproc->MenuResult == -1)
          {
            mproc->lastMenu  = mnu_Main;
            mproc->whichMenu = mnu_Network;
          }
          else if (mproc->MenuResult == 1)
          {
            mproc->whichMenu = mnu_HostGame;

            // make sure we have a valid game
            mnu_ensure_game(mproc, &gs);

            if(!gs->modstate.loaded)
            {
              module_quit(gs);
              gs->modstate.loaded = bfalse;
            }
            mproc->sv->ready = bfalse;

            sv_unhostGame(mproc->sv);

            CServer_shutDown(mproc->sv);
          }
        };
      };
      break;

    case mnu_JoinGame:
      mproc->MenuResult = mnu_doJoinGame(mproc, frameDuration);
      if( mnu_handleKeyboard(mproc) < 0 ) mproc->MenuResult = -1;  // handle escape

      if (mproc->MenuResult != 0)
      {
        if(mproc->MenuResult == -1)
        {
          // if we exit without making a selection, shut off the client
          mproc->lastMenu = mnu_Network;
          if(!mproc->cl->logged_on)
          {
            Client_shutDown(mproc->cl);
          }
          mproc->whichMenu = mproc->lastMenu;
        }
        if(mproc->MenuResult == 0)
        {
          // can not log on for some reason. do nothing
        }
        else if (mproc->MenuResult == 1)
        {
          mproc->lastMenu = mnu_Network;

          // make sure we have a valid game
          mnu_ensure_game(mproc, &gs);

          if(!gs->modstate.loaded)
          {
            memcpy(&(gs->modstate), &(mproc->cl->req_modstate), sizeof(ModState_t));

            log_info("SDL_main: Loading module %s... ", mproc->cl->req_mod.loadname);

            gs->modstate.loaded  = module_load(gs, mproc->cl->req_mod.loadname);
            mproc->cl->waiting      = gs->modstate.loaded;

            if(gs->modstate.loaded)
            {
              log_info("Succeeded!\n");
            }
            else
            {
              log_info("Failed!\n");
            }
          };

          gs->allpladead      = bfalse;

          Client_joinGame(mproc->cl, mproc->cl->req_host);

          mproc->whichMenu = mnu_ChoosePlayer;
        }

      }
      break;

    case mnu_ChooseModule:
      mproc->MenuResult = mnu_doChooseModule( mproc, frameDuration );
      if( mnu_handleKeyboard(mproc) < 0 ) mproc->MenuResult = -1;  // handle escape

           if ( mproc->MenuResult == -1 ) mproc->whichMenu = mproc->lastMenu;
      else if ( mproc->MenuResult ==  1 ) mproc->whichMenu = mnu_TestResults;
      break;

    case mnu_ChoosePlayer:
      mproc->MenuResult = mnu_doChoosePlayer( mproc, frameDuration );
      if( mnu_handleKeyboard(mproc) < 0 ) mproc->MenuResult = -1;  // handle escape

      if ( mproc->MenuResult == -1 )     mproc->whichMenu = mproc->lastMenu;
      else if ( mproc->MenuResult == 1 ) { mproc->whichMenu = mnu_ChooseModule; mnu_startNewPlayer = bfalse; }
      break;

    case mnu_Options:
      mproc->MenuResult = mnu_doOptions( mproc, frameDuration );
      if( mnu_handleKeyboard(mproc) < 0 ) mproc->MenuResult = -1;  // handle escape

      if ( mproc->MenuResult != 0 )
      {
        if ( mproc->MenuResult == -1 )   mproc->whichMenu = mproc->lastMenu;
        else if ( mproc->MenuResult == 1 ) mproc->whichMenu = mnu_AudioOptions;
        else if ( mproc->MenuResult == 2 ) mproc->whichMenu = mnu_InputOptions;
        else if ( mproc->MenuResult == 3 ) mproc->whichMenu = mnu_VideoOptions;
        else if ( mproc->MenuResult == 4 ) mproc->whichMenu = mnu_Main;
      }
      break;

    case mnu_AudioOptions:
      mproc->MenuResult = mnu_doAudioOptions( mproc, frameDuration );
      if( mnu_handleKeyboard(mproc) < 0 ) mproc->MenuResult = -1;  // handle escape

      if ( mproc->MenuResult != 0 )
      {
        mproc->whichMenu = mnu_Options;
      }
      break;

    case mnu_VideoOptions:
      mproc->MenuResult = mnu_doVideoOptions( mproc, frameDuration );
      if( mnu_handleKeyboard(mproc) < 0 ) mproc->MenuResult = -1;  // handle escape

      if ( mproc->MenuResult != 0 )
      {
        mproc->whichMenu = mnu_Options;
      }
      break;

    case mnu_TestResults:
      mproc->MenuResult = mnu_doLaunchGame( mproc, frameDuration );
      if( mnu_handleKeyboard(mproc) < 0 ) mproc->MenuResult = -1;  // handle escape

      if ( mproc->MenuResult != 0 )
      {
        mproc->whichMenu = mnu_Main;
        mproc->proc.returnValue = 1;
      }
      break;

    default:
    case mnu_NotImplemented:
      mproc->MenuResult = mnu_doNotImplemented( mproc, frameDuration );
      if( mnu_handleKeyboard(mproc) < 0 ) mproc->MenuResult = -1;  // handle escape

      if ( mproc->MenuResult != 0 )
      {
        mproc->whichMenu = mproc->lastMenu;
      }
  }


  return mproc->proc.returnValue;
}


//--------------------------------------------------------------------------------------------
void mnu_frameStep()
{

}

//--------------------------------------------------------------------------------------------
void mnu_saveSettings()
{
  //This function saves all current game settings to setup.txt
  FILE* setupfile;
  STRING write;
  char *TxtTmp;

  setupfile = fs_fileOpen( PRI_NONE, NULL, CData.setup_file, "w" );
  if ( setupfile )
  {
    /*GRAPHIC PART*/
    fputs( "{GRAPHIC}\n", setupfile );

    snprintf( write, sizeof( write ), "[MAX_NUMBER_VERTICES] : \"%i\"\n", OData.maxtotalmeshvertices / 1024 );
    fputs( write, setupfile );

    snprintf( write, sizeof( write ), "[COLOR_DEPTH] : \"%i\"\n", OData.scrd );
    fputs( write, setupfile );

    snprintf( write, sizeof( write ), "[Z_DEPTH] : \"%i\"\n", OData.scrz );
    fputs( write, setupfile );

    if ( OData.fullscreen ) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
    snprintf( write, sizeof( write ), "[FULLSCREEN] : \"%s\"\n", TxtTmp );
    fputs( write, setupfile );

    if ( OData.zreflect ) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
    snprintf( write, sizeof( write ), "[Z_REFLECTION] : \"%s\"\n", TxtTmp );
    fputs( write, setupfile );

    snprintf( write, sizeof( write ), "[SCREENSIZE_X] : \"%i\"\n", OData.scrx );
    fputs( write, setupfile );

    snprintf( write, sizeof( write ), "[SCREENSIZE_Y] : \"%i\"\n", OData.scry );
    fputs( write, setupfile );

    snprintf( write, sizeof( write ), "[MAX_TEXT_MESSAGE] : \"%i\"\n", OData.maxmessage );
    fputs( write, setupfile );

    if ( OData.staton ) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
    snprintf( write, sizeof( write ), "[STATUS_BAR] : \"%s\"\n", TxtTmp );
    fputs( write, setupfile );

    if ( GL_NICEST == OData.perspective ) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
    snprintf( write, sizeof( write ), "[PERSPECTIVE_CORRECT] : \"%s\"\n", TxtTmp );
    fputs( write, setupfile );

    if ( OData.texturefilter >= TX_ANISOTROPIC && gfxState.maxAnisotropy > 0.0f )
    {
      int anisotropy = MIN( gfxState.maxAnisotropy, gfxState.userAnisotropy );
      snprintf( mnu_filternamebuffer, sizeof( mnu_filternamebuffer ), "%d ANISOTROPIC %d", OData.texturefilter, anisotropy );
    }
    else if ( OData.texturefilter >= TX_ANISOTROPIC )
    {
      OData.texturefilter = TX_TRILINEAR_2;
      snprintf( mnu_filternamebuffer, sizeof( mnu_filternamebuffer ), "%d TRILINEAR 2", OData.texturefilter );
    }
    else
    {
      switch ( OData.texturefilter )
      {
        case TX_UNFILTERED:
          snprintf( mnu_filternamebuffer, sizeof( mnu_filternamebuffer ), "%d UNFILTERED", OData.texturefilter );
          break;

        case TX_MIPMAP:
          snprintf( mnu_filternamebuffer, sizeof( mnu_filternamebuffer ), "%d MIPMAP", OData.texturefilter );
          break;

        case TX_TRILINEAR_1:
          snprintf( mnu_filternamebuffer, sizeof( mnu_filternamebuffer ), "%d TRILINEAR 1", OData.texturefilter );
          break;

        case TX_TRILINEAR_2:
          snprintf( mnu_filternamebuffer, sizeof( mnu_filternamebuffer ), "%d TRILINEAR 2", OData.texturefilter );
          break;

        default:
        case TX_LINEAR:
          OData.texturefilter = TX_LINEAR;
          snprintf( mnu_filternamebuffer, sizeof( mnu_filternamebuffer ), "%d LINEAR", OData.texturefilter );
          break;

      }
    }

    snprintf( write, sizeof( write ), "[TEXTURE_FILTERING] : \"%s\"\n", mnu_filternamebuffer );
    fputs( write, setupfile );

    if ( OData.shading == GL_SMOOTH ) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
    snprintf( write, sizeof( write ), "[GOURAUD_SHADING] : \"%s\"\n", TxtTmp );
    fputs( write, setupfile );

    if ( OData.antialiasing ) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
    snprintf( write, sizeof( write ), "[ANTIALIASING] : \"%s\"\n", TxtTmp );
    fputs( write, setupfile );

    if ( OData.dither ) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
    snprintf( write, sizeof( write ), "[DITHERING] : \"%s\"\n", TxtTmp );
    fputs( write, setupfile );

    if ( OData.refon ) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
    snprintf( write, sizeof( write ), "[REFLECTION] : \"%s\"\n", TxtTmp );
    fputs( write, setupfile );

    if ( OData.shaon ) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
    snprintf( write, sizeof( write ), "[SHADOWS] : \"%s\"\n", TxtTmp );
    fputs( write, setupfile );

    if ( OData.shasprite ) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
    snprintf( write, sizeof( write ), "[SHADOW_AS_SPRITE] : \"%s\"\n", TxtTmp );
    fputs( write, setupfile );

    if ( OData.phongon ) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
    snprintf( write, sizeof( write ), "[PHONG] : \"%s\"\n", TxtTmp );
    fputs( write, setupfile );

    if ( OData.fogallowed ) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
    snprintf( write, sizeof( write ), "[FOG] : \"%s\"\n", TxtTmp );
    fputs( write, setupfile );

    if ( OData.reffadeor == 0 ) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
    snprintf( write, sizeof( write ), "[FLOOR_REFLECTION_FADEOUT] : \"%s\"\n", TxtTmp );
    fputs( write, setupfile );

    if ( OData.twolayerwateron ) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
    snprintf( write, sizeof( write ), "[MULTI_LAYER_WATER] : \"%s\"\n", TxtTmp );
    fputs( write, setupfile );

    if ( OData.overlayvalid ) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
    snprintf( write, sizeof( write ), "[OVERLAY] : \"%s\"\n", TxtTmp );
    fputs( write, setupfile );

    if ( OData.backgroundvalid ) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
    snprintf( write, sizeof( write ), "[BACKGROUND] : \"%s\"\n", TxtTmp );
    fputs( write, setupfile );

    if ( OData.particletype == PART_SMOOTH ) TxtTmp = "SMOOTH";
    else if ( OData.particletype == PART_NORMAL ) TxtTmp = "NORMAL";
    else if ( OData.particletype == PART_FAST ) TxtTmp = "FAST";
    snprintf( write, sizeof( write ), "[PARTICLE_EFFECTS] : \"%s\"\n", TxtTmp );
    fputs( write, setupfile );

    if ( OData.vsync ) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
    snprintf( write, sizeof( write ), "[VERTICAL_SYNC] : \"%s\"\n", TxtTmp );
    fputs( write, setupfile );

    if ( OData.gfxacceleration ) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
    snprintf( write, sizeof( write ), "[FORCE_GFX_ACCEL] : \"%s\"\n", TxtTmp );
    fputs( write, setupfile );


    /*SOUND PART*/
    snprintf( write, sizeof( write ), "\n{SOUND}\n" );
    fputs( write, setupfile );

    if ( OData.allow_music ) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
    snprintf( write, sizeof( write ), "[MUSIC] : \"%s\"\n", TxtTmp );
    fputs( write, setupfile );

    snprintf( write, sizeof( write ), "[MUSIC_VOLUME] : \"%i\"\n", OData.musicvolume );
    fputs( write, setupfile );

    if ( OData.allow_sound ) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
    snprintf( write, sizeof( write ), "[SOUND] : \"%s\"\n", TxtTmp );
    fputs( write, setupfile );

    snprintf( write, sizeof( write ), "[SOUND_VOLUME] : \"%i\"\n", OData.soundvolume );
    fputs( write, setupfile );

    snprintf( write, sizeof( write ), "[OUTPUT_BUFFER_SIZE] : \"%i\"\n", OData.buffersize );
    fputs( write, setupfile );

    snprintf( write, sizeof( write ), "[MAX_SOUND_CHANNEL] : \"%i\"\n", OData.maxsoundchannel );
    fputs( write, setupfile );

    /*Camera_t PART*/
    snprintf( write, sizeof( write ), "\n{CONTROL}\n" );
    fputs( write, setupfile );

    switch ( OData.autoturncamera )
    {
      case 255: TxtTmp = "GOOD";
        break;

      case btrue: TxtTmp = "TRUE";
        break;

      case bfalse: TxtTmp = "FALSE";
        break;

    }
    snprintf( write, sizeof( write ), "[AUTOTURN_CAMERA] : \"%s\"\n", TxtTmp );
    fputs( write, setupfile );

    /*NETWORK PART*/
    snprintf( write, sizeof( write ), "\n{NETWORK}\n" );
    fputs( write, setupfile );

    if ( CData.request_network ) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
    snprintf( write, sizeof( write ), "[NETWORK_ON] : \"%s\"\n", TxtTmp );
    fputs( write, setupfile );

    TxtTmp = CData.net_hosts[0];
    snprintf( write, sizeof( write ), "[HOST_NAME] : \"%s\"\n", TxtTmp );
    fputs( write, setupfile );

    TxtTmp = CData.net_messagename;
    snprintf( write, sizeof( write ), "[MULTIPLAYER_NAME] : \"%s\"\n", TxtTmp );
    fputs( write, setupfile );

    snprintf( write, sizeof( write ), "[LAG_TOLERANCE] : \"%i\"\n", CData.lag );
    fputs( write, setupfile );

    /*DEBUG PART*/
    snprintf( write, sizeof( write ), "\n{DEBUG}\n" );
    fputs( write, setupfile );

    if ( CData.fpson ) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
    snprintf( write, sizeof( write ), "[DISPLAY_FPS] : \"%s\"\n", TxtTmp );
    fputs( write, setupfile );

    if ( CData.HideMouse ) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
    snprintf( write, sizeof( write ), "[HIDE_MOUSE] : \"%s\"\n", TxtTmp );
    fputs( write, setupfile );

    if ( SDL_GRAB_ON == CData.GrabMouse ) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
    snprintf( write, sizeof( write ), "[GRAB_MOUSE] : \"%s\"\n", TxtTmp );
    fputs( write, setupfile );

    if ( CData.DevMode ) TxtTmp = "TRUE"; else TxtTmp = "FALSE";
    snprintf( write, sizeof( write ), "[DEVELOPER_MODE] : \"%s\"\n", TxtTmp );
    fputs( write, setupfile );

    //Close it up
    fs_fileClose( setupfile );
  }
  else
  {
    log_warning( "Cannot open setup.txt to write new settings into.\n" );
  }
}

//--------------------------------------------------------------------------------------------
//int mnu_doModel(float deltaTime)
//{
//  // Advance the model's animation
//  m_modelLerp += (float)deltaTime / kAnimateSpeed;
//  if (m_modelLerp >= 1.0f)
//  {
//   m_modelFrame = m_modelNextFrame;
//   m_modelNextFrame++;
//
//   // Roll back to the first frame if we run out of them
//   if (m_modelNextFrame >= m_model->numFrames())
//   {
//    m_modelNextFrame = 0;
//   }
//
//   m_modelLerp -= 1.0f;
//  }
//
//  float width, height;
//  width  = (float)gfxState.scrx;
//  height = (float)gfxState.scry;
//
//    glPushAttrib(GL_ENABLE_BIT|GL_DEPTH_BUFFER_BIT|GL_TRANSFORM_BIT|GL_VIEWPORT_BIT|GL_COLOR_BUFFER_BIT);
//  glMatrixMode(GL_PROJECTION);                              /* GL_TRANSFORM_BIT */
//  glLoadIdentity();                                         /* GL_TRANSFORM_BIT */
//  gluPerspective(60.0, width/height, 1, 2048);              /* GL_TRANSFORM_BIT */
//
//  glMatrixMode(GL_MODELVIEW);                               /* GL_TRANSFORM_BIT */
//  glLoadIdentity();                                         /* GL_TRANSFORM_BIT */
//  glTranslatef(20, -20, -75);                               /* GL_TRANSFORM_BIT */
//  glRotatef(-90, 1, 0, 0);                                  /* GL_TRANSFORM_BIT */
//  glRotatef(mnu_modelAngle, 0, 0, 1);                         /* GL_TRANSFORM_BIT */
//
//  glEnable(GL_TEXTURE_2D);
//  glEnable(GL_DEPTH_TEST);
//
//    GLtexture_Bind( &m_modelTxr, &gfxState );
//  m_model->drawBlendedFrames(m_modelFrame, m_modelNextFrame, m_modelLerp);
// }
//};


void mnu_enterMenuMode()
{
  // make the mouse behave like a cursor

  SDL_WM_GrabInput( SDL_GRAB_OFF );
  SDL_ShowCursor( SDL_DISABLE );
  mous.game = bfalse;
};

void mnu_exitMenuMode()
{
  // restore the default mouse mode

  SDL_WM_GrabInput( CData.GrabMouse );
  SDL_ShowCursor( CData.HideMouse ? SDL_DISABLE : SDL_ENABLE );
  mous.game = btrue;
};

//--------------------------------------------------------------------------------------------
int mnu_doNetwork(MenuProc_t * mproc, float deltaTime)
{
  static int menuState = MM_Begin;
  static GLtexture background;
  static ui_Widget_t wBackground, wCopyright;
  static float lerp;
  static int menuChoice = 0;
  int result = 0;

  if(!net_Started()) return 3;

  switch (menuState)
  {
  case MM_Begin:
    printf("mnu_doNetwork()\n");

    GLtexture_new( &background );;

    // set up menu variables
    snprintf(CStringTmp1, sizeof(CStringTmp1), "%s" SLASH_STRING "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.mnu_dir, CData.menu_main_bitmap);
    GLtexture_Load(GL_TEXTURE_2D, &background, CStringTmp1, INVALID_TEXTURE);
    ui_initWidget( &wBackground, UI_Invalid, ui_getTTFont(), NULL, &background, ( gfxState.surface->w - background.imgW ), 0, 0, 0 );

    mnu_widgetCount = mnu_initWidgetsList(mnu_widgetList, MAXWIDGET, netMenuButtons );

    initSlidyButtons(1.0f, mnu_widgetList, mnu_widgetCount);

    menuChoice = 0;
    menuState = MM_Entering;
    break;

  case MM_Entering:
    // do buttons sliding in animation, and background fading in
    // background
    glColor4f(1, 1, 1, 1 - SlidyButtonState_t.lerp);
    ui_initWidget( &wBackground, UI_Invalid, ui_getTTFont(), NULL, &background, ( gfxState.surface->w - background.imgW ), 0, 0, 0 );

    // "Copyright" text
    fnt_drawTextBox(mnu_Font, mnu_copyrightText, mnu_copyrightLeft, mnu_copyrightTop, 0, 0, 20);

    mnu_drawSlidyButtons();
    mnu_updateSlidyButtons(-deltaTime);

    // Let lerp wind down relative to the time elapsed
    if (SlidyButtonState_t.lerp <= 0.0f)
    {
      menuState = MM_Running;
    }

    break;

  case MM_Running:
    // Do normal run
    // Background
    glColor4f(1, 1, 1, 1);
    ui_drawImage( &wBackground );

    // "Copyright" text
    fnt_drawTextBox(mnu_Font, mnu_copyrightText, mnu_copyrightLeft, mnu_copyrightTop, 0, 0, 20);

    // Buttons
    if (BUTTON_UP == ui_doButton(mnu_widgetList + 0))
    {
      // begin single player stuff
      menuChoice = 1;
    }

    if (BUTTON_UP == ui_doButton(mnu_widgetList + 1))
    {
      // begin multi player stuff
      menuChoice = 2;
    }

    if (BUTTON_UP == ui_doButton(mnu_widgetList + 2))
    {
      // begin networked stuff
      menuChoice = -1;
    }

    if (menuChoice != 0)
    {
      menuState = MM_Leaving;
      initSlidyButtons(0.0f, mnu_widgetList, mnu_widgetCount);
    }
    break;

  case MM_Leaving:
    // Do buttons sliding out and background fading
    // Do the same stuff as in MM_Entering, but backwards
    glColor4f(1, 1, 1, 1 - SlidyButtonState_t.lerp);
    ui_drawImage( &wBackground );

    // "Copyright" text
    fnt_drawTextBox(mnu_Font, mnu_copyrightText, mnu_copyrightLeft, mnu_copyrightTop, 0, 0, 20);

    // Buttons
    mnu_drawSlidyButtons();
    mnu_updateSlidyButtons(deltaTime);
    if (SlidyButtonState_t.lerp >= 1.0f)
    {
      menuState = MM_Finish;
    }
    break;

  case MM_Finish:
    menuState = MM_Begin; // Make sure this all resets next time doMainMenu is called

    // Set the next menu to load
    result = menuChoice;

    ui_Reset();

    // delete the background texture as we exit
    GLtexture_delete( &background );

    break;
  };

  return result;
}

//--------------------------------------------------------------------------------------------
int mnu_doHostGame(MenuProc_t * mproc, float deltaTime)
{
  static int menuState = MM_Begin;
  static int menuChoice = 0;

  static ui_Widget_t wBackground, wCopyright, wtmp;
  static int startIndex;
  static GLtexture background;

  static int moduleMenuOffsetX;
  static int moduleMenuOffsetY;

  int result = 0;
  int i, j, x, y;
  char txtBuffer[128];

  Server_t * sv;

  if(!net_Started()) return -1;

  // make sure a Server_t state is defined
  if(rv_succeed != MenuProc_ensure_server(mproc, NULL)) return -1;
  sv = mproc->sv;

  switch (menuState)
  {
  case MM_Begin:
    printf("mnu_doHostGame()\n");

    GLtexture_new( &background );;

    //Reload all avalible modules (Hidden ones may pop up after the player has completed one)
    mnu_prime_titleimage(mproc);
    sv->loc_mod_count = mnu_load_mod_data(mproc, sv->loc_mod, MAXMODULE);

    // Load font & background
    snprintf(CStringTmp1, sizeof(CStringTmp1), "%s" SLASH_STRING "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.mnu_dir, CData.menu_sleepy_bitmap);
    GLtexture_Load(GL_TEXTURE_2D, &background, CStringTmp1, INVALID_KEY);

    ui_initWidget( &wBackground, UI_Invalid, ui_getTTFont(), NULL, &background, ( gfxState.surface->w - background.imgW ), 0, 0, 0 );
    ui_initWidget( &wCopyright,  UI_Invalid, ui_getTTFont(), mnu_copyrightText, NULL, mnu_copyrightLeft, mnu_copyrightTop, 0, 0 );

    startIndex = 0;
    mnu_prime_modules(mproc);

    // Find the module's that we want to allow hosting for.
    // Only modules taht allow importing AND have modmaxplayers > 1
    for (i = 0; i<sv->loc_mod_count; i++)
    {
      if (sv->loc_mod[i].importamount > 0 && sv->loc_mod[i].maxplayers > 1)
      {
        mproc->validModules[mproc->validModules_count] = i;
        mproc->validModules_count++;
      }
    }

    // Figure out at what offset we want to draw the module menu.
    moduleMenuOffsetX = (gfxState.surface->w - DEFAULT_SCREEN_W) / 2;
    moduleMenuOffsetY = (gfxState.surface->h - DEFAULT_SCREEN_H) / 2;

    // navigation buttons
    ui_initWidget( mnu_widgetList + 0, 0, mnu_Font, "<-",          NULL, moduleMenuOffsetX +  20, moduleMenuOffsetY +  74, 30, 30 );
    ui_initWidget( mnu_widgetList + 1, 1, mnu_Font, "->",          NULL, moduleMenuOffsetX + 590, moduleMenuOffsetY +  74, 30, 30 );
    ui_initWidget( mnu_widgetList + 2, 2, mnu_Font, "Host Module", NULL, moduleMenuOffsetX + 327, moduleMenuOffsetY + 173, 200, 30 );
    ui_initWidget( mnu_widgetList + 3, 3, mnu_Font, "Back",        NULL, moduleMenuOffsetX + 327, moduleMenuOffsetY + 208, 200, 30 );

    // Module "windows"
    ui_initWidget( mnu_widgetList + 4, 4, mnu_Font, NULL, NULL, moduleMenuOffsetX, moduleMenuOffsetY, 138, 138 );
    ui_initWidget( mnu_widgetList + 5, 5, mnu_Font, NULL, NULL, moduleMenuOffsetX, moduleMenuOffsetY, 138, 138 );
    ui_initWidget( mnu_widgetList + 6, 6, mnu_Font, NULL, NULL, moduleMenuOffsetX, moduleMenuOffsetY, 138, 138 );

    // Module description
    ui_initWidget( mnu_widgetList + 7, UI_Invalid, mnu_Font, NULL, NULL, moduleMenuOffsetX + 21, moduleMenuOffsetY + 173, 291, 230 );

    x = ( gfxState.surface->w / 2 ) - ( background.imgW / 2 );
    y = gfxState.surface->h - background.imgH;

    menuChoice = 0;
    menuState = MM_Entering;
    break;

  case MM_Entering:
    menuState = MM_Running;
    break;

  case MM_Running:
    // Draw the background
    glColor4f(1, 1, 1, 1);

    x = (gfxState.surface->w / 2) - (background.imgW / 2);
    y = gfxState.surface->h - background.imgH;
    ui_drawImage( &wBackground );

    // Draw the arrows to pick modules
    if ( BUTTON_UP == ui_doButton( mnu_widgetList + 0 ) )
    {
      startIndex--;
      if ( startIndex < 0 ) startIndex += mproc->validModules_count;
    }

    if ( BUTTON_UP == ui_doButton( mnu_widgetList + 1 ) )
    {
      startIndex++;
      if ( startIndex >= mproc->validModules_count ) startIndex -= mproc->validModules_count;
    }

    // Clamp startIndex to 0
    startIndex = MAX(0, startIndex);

      // Draw buttons for the modules that can be selected
      x = 93;
      y = 20;
      for ( i = 0; i < 3; i++ )
      {
        int imod;

        j = ( i + startIndex ) % mproc->validModules_count;
        imod = mproc->validModules[j];
        mnu_widgetList[4+i].img = mproc->TxTitleImage + sv->loc_mod[imod].tx_title_idx;

        mnu_slideButton( &wtmp, mnu_widgetList + 4+i, x, y );
        if ( BUTTON_UP == ui_doImageButton( &wtmp ) )
        {
          mproc->selectedModule = j;
        }

        x += 138 + 20; // Width of the button, and the spacing between buttons
      }

      // Draw an unused button as the backdrop for the text for now
      ui_drawButton( mnu_widgetList + 7 );

      // And draw the next & back buttons
      if ( BUTTON_UP == ui_doButton( mnu_widgetList + 2 ) )
      {
        // go to the next menu with this module selected
        menuState = MM_Leaving;
      }

      if ( BUTTON_UP == ui_doButton( mnu_widgetList + 3 ) )
      {
        // Signal mnu_Run to go back to the previous menu
        mproc->selectedModule = -1;
        menuState = MM_Leaving;
      }

      // Draw the text description of the selected module
      if ( mproc->selectedModule > -1 )
      {
        MOD_INFO * mi = sv->loc_mod + mproc->validModules[mproc->selectedModule];

        y = 173 + 5;
        x = 21 + 5;
        glColor4f( 1, 1, 1, 1 );
        fnt_drawText( mnu_Font, moduleMenuOffsetX + x, moduleMenuOffsetY + y,
                      mi->longname );
        y += 20;

        snprintf( txtBuffer, sizeof( txtBuffer ), "Difficulty: %s", mi->rank );
        fnt_drawText( mnu_Font, moduleMenuOffsetX + x, moduleMenuOffsetY + y, txtBuffer );
        y += 20;

        if ( mi->maxplayers > 1 )
        {
          if ( mi->minplayers == mi->maxplayers )
          {
            snprintf( txtBuffer, sizeof( txtBuffer ), "%d Players", mi->minplayers );
          }
          else
          {
            snprintf( txtBuffer, sizeof( txtBuffer ), "%d - %d Players", mi->minplayers, mi->maxplayers );
          }
        }
        else
        {
          if ( mi->importamount == 0 )
          {
            snprintf( txtBuffer, sizeof( txtBuffer ), "Starter Module" );
          }
          else
          {
            snprintf( txtBuffer, sizeof( txtBuffer ), "Special Module" );
          }
        }
        fnt_drawText( mnu_Font, moduleMenuOffsetX + x, moduleMenuOffsetY + y, txtBuffer );
        y += 20;

        // And finally, the summary
        snprintf( txtBuffer, sizeof( txtBuffer ), "%s" SLASH_STRING "%s" SLASH_STRING "%s" SLASH_STRING "%s", CData.modules_dir, mi->loadname, CData.gamedat_dir, CData.mnu_file );
        if ( mproc->validModules[mproc->selectedModule] != sv->loc_modtxt.val )
        {
          if ( module_read_summary( txtBuffer, &(sv->loc_modtxt) ) ) sv->loc_modtxt.val = mproc->validModules[mproc->selectedModule];
        };

        for ( i = 0;i < SUMMARYLINES;i++ )
        {
          fnt_drawText( mnu_Font, moduleMenuOffsetX + x, moduleMenuOffsetY + y, sv->loc_modtxt.summary[i] );
          y += 20;
        }
      }

    break;

  case MM_Leaving:
    menuState = MM_Finish;
    break;

  case MM_Finish:
    menuState = MM_Begin;

    // figure out what to do
    if (-1 == mproc->selectedModule)
    {
      result = -1;
    }
    else
    {
      // Save the module info of the picked module
      sv->selectedModule = mproc->selectedModule;
      memcpy(&(sv->mod), sv->loc_mod + mproc->validModules[sv->selectedModule], sizeof(MOD_INFO));

      result = 1;
    }

    ui_Reset();

    // delete the background texture as we exit
    GLtexture_delete( &background );

    break;

  }

  return result;
}

//--------------------------------------------------------------------------------------------
int mnu_doUnhostGame(MenuProc_t * mproc, float deltaTime)
{
  static int menuState = MM_Begin;
  static int menuChoice = 0;

  static ui_Widget_t wBackground, wCopyright, wtmp;
  static int startIndex;
  static GLtexture background;

  static int moduleMenuOffsetX;
  static int moduleMenuOffsetY;

  int result = 0;
  int i, x, y;
  char txtBuffer[128];

  Server_t * sv;

  if(!net_Started()) return -1;

  // make sure a Server_t state is defined
  if(NULL == mproc->sv) return -1;
  sv = mproc->sv;

  switch (menuState)
  {
  case MM_Begin:
    printf("mnu_doUnhostGame()\n");

    GLtexture_new( &background );;

    //Reload all avalible modules (Hidden ones may pop up after the player has completed one)
    mnu_prime_titleimage(mproc);
    sv->loc_mod_count = mnu_load_mod_data(mproc, sv->loc_mod, MAXMODULE);

    // find the module that matches the one that is being hosted
    mproc->validModules_count = 0;
    mproc->selectedModule = -1;
    sv->selectedModule = -1;

    for (i = 0;i < sv->loc_mod_count; i++)
    {
      mproc->validModules[mproc->validModules_count] = i;
      mproc->validModules_count++;

      if( 0 == strcmp(sv->loc_mod[i].loadname, sv->mod.loadname) )
      {
        mproc->selectedModule = i;
      }
    };

    // Load font & background
    snprintf(CStringTmp1, sizeof(CStringTmp1), "%s" SLASH_STRING "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.mnu_dir, CData.menu_sleepy_bitmap);
    GLtexture_Load(GL_TEXTURE_2D, &background, CStringTmp1, INVALID_KEY);
    ui_initWidget( &wBackground, UI_Invalid, ui_getTTFont(), NULL, &background, ( gfxState.surface->w - background.imgW ), 0, 0, 0 );

    // Figure out at what offset we want to draw the module menu.
    moduleMenuOffsetX = (gfxState.surface->w - DEFAULT_SCREEN_W) / 2;
    moduleMenuOffsetY = (gfxState.surface->h - DEFAULT_SCREEN_H) / 2;

    // navigation buttons
    ui_initWidget( mnu_widgetList + 2, 2, mnu_Font, "Unhost Module", NULL, moduleMenuOffsetX + 327, moduleMenuOffsetY + 173, 200, 30 );
    ui_initWidget( mnu_widgetList + 3, 3, mnu_Font, "Back",        NULL, moduleMenuOffsetX + 327, moduleMenuOffsetY + 208, 200, 30 );

    // Module "windows"
    ui_initWidget( mnu_widgetList + 5, 5, mnu_Font, NULL, NULL, moduleMenuOffsetX, moduleMenuOffsetY, 138, 138 );

    // Module description
    ui_initWidget( mnu_widgetList + 7, UI_Invalid, mnu_Font, NULL, NULL, moduleMenuOffsetX + 21, moduleMenuOffsetY + 173, 291, 230 );

    menuChoice = 0;
    menuState = MM_Entering;
    break;

  case MM_Entering:
    menuState = MM_Running;
    break;

  case MM_Running:
    // Draw the background
    glColor4f(1, 1, 1, 1);
    x = (gfxState.surface->w / 2) - (background.imgW / 2);
    y = gfxState.surface->h - background.imgH;
    ui_drawImage( &wBackground );


    // Draw buttons for the modules that can be selected
    startIndex = mproc->selectedModule;
    x = 93;
    y = 20;

    x += 138 + 20; // Width of the button, and the spacing between buttons

    {
      int imod = mproc->validModules[mproc->selectedModule];
      mnu_widgetList[5].img = mproc->TxTitleImage + sv->loc_mod[imod].tx_title_idx;
      mnu_slideButton( &wtmp, mnu_widgetList + 5, x, y );
      ui_doImageButton( &wtmp );
    }

    // Draw an unused button as the backdrop for the text for now
    ui_drawButton( mnu_widgetList + 7 );

    // And draw the next & back buttons
    if (ui_doButton(mnu_widgetList + 2))
    {
      // go to the next menu with this module selected
      menuState = MM_Leaving;
    }

    if (ui_doButton( mnu_widgetList + 3))
    {
      // Signal doMenu to go back to the previous menu
      mproc->selectedModule = -1;
      menuState = MM_Leaving;
    }

    // Draw the text description of the selected module
    //if (mproc->selectedModule > -1)
    {
      y = 173 + 5;
      x = 21 + 5;
      glColor4f(1, 1, 1, 1);
      fnt_drawText(mnu_Font, moduleMenuOffsetX + x, moduleMenuOffsetY + y, sv->mod.longname);
      y += 20;

      snprintf(txtBuffer, sizeof(txtBuffer), "Difficulty: %s", sv->mod.rank);
      fnt_drawText(mnu_Font, moduleMenuOffsetX + x, moduleMenuOffsetY + y, txtBuffer);
      y += 20;

      if (sv->mod.maxplayers > 1)
      {
        if (sv->mod.minplayers == sv->mod.maxplayers)
        {
          snprintf(txtBuffer, sizeof(txtBuffer), "%d Players", sv->mod.minplayers);
        }
        else
        {
          snprintf(txtBuffer, sizeof(txtBuffer), "%d - %d Players", sv->mod.minplayers, sv->mod.maxplayers);
        }
      }
      else
      {
        snprintf(txtBuffer, sizeof(txtBuffer), "Starter Module");
      }
      fnt_drawText(mnu_Font, moduleMenuOffsetX + x, moduleMenuOffsetY + y, txtBuffer);
      y += 20;

      // And finally, the summary
      snprintf(txtBuffer, sizeof(txtBuffer), "%s" SLASH_STRING "%s" SLASH_STRING "%s" SLASH_STRING "%s", CData.modules_dir, sv->mod.loadname, CData.gamedat_dir, CData.mnu_file);
      module_read_summary(txtBuffer, &(sv->loc_modtxt));
      for (i = 0;i < SUMMARYLINES;i++)
      {
        fnt_drawText(mnu_Font, moduleMenuOffsetX + x, moduleMenuOffsetY + y, sv->loc_modtxt.summary[i]);
        y += 20;
      }
    }

    break;

  case MM_Leaving:
    menuState = MM_Finish;
    break;

  case MM_Finish:
    if(mproc->selectedModule == -1)
    {
      result = -1;
    }
    else
    {
      sv->selectedModule = -1;
      result = 1;
    };

    menuState = MM_Begin;

    ui_Reset();

    // delete the background texture as we exit
    GLtexture_delete( &background );

    break;

  }

  return result;
}

//--------------------------------------------------------------------------------------------
int mnu_doJoinGame(MenuProc_t * mproc, float deltaTime)
{
  static int menuState = MM_Begin;
  static int menuChoice = 0;

  static ui_Widget_t wBackground, wCopyright, wtmp;
  static int startIndex;
  static GLtexture background;

  static int moduleMenuOffsetX;
  static int moduleMenuOffsetY;

  static bool_t all_module_images_loaded = bfalse;

  static bool_t all_module_info_loaded = bfalse;

  retval_t wait_return;

  SYS_PACKET egopkt;

  Game_t   * gs;
  Client_t * cl;

  int result = 0;
  int i, j, x, y;
  char txtBuffer[128];

  if( !net_Started() ) return -1;

  gs = Graphics_requireGame(&gfxState);

  // make sure a Server_t state is defined
  if( rv_succeed != MenuProc_ensure_client(mproc, Graphics_requireGame(&gfxState)) )
  {
    return -1;
  }
  cl = mproc->cl;

  switch (menuState)
  {
  case MM_Begin:

    printf("mnu_doJoinGame()\n");

    GLtexture_new( &background );;

    // start up the file transfer and client components at this point

    // make sure that the NetFile server is running
    MenuProc_start_netfile(mproc, gs);

    // make sure that the client module is running
    MenuProc_start_client(mproc, gs);

    // Load font & background
    snprintf(CStringTmp1, sizeof(CStringTmp1), "%s" SLASH_STRING "%s" SLASH_STRING "%s", CData.basicdat_dir, CData.mnu_dir, CData.menu_sleepy_bitmap);
    GLtexture_Load(GL_TEXTURE_2D, &background, CStringTmp1, INVALID_KEY);

    ui_initWidget( &wBackground, UI_Invalid, ui_getTTFont(), NULL, &background, ( gfxState.surface->w - background.imgW ), 0, 0, 0 );
    ui_initWidget( &wCopyright,  UI_Invalid, ui_getTTFont(), mnu_copyrightText, NULL, mnu_copyrightLeft, mnu_copyrightTop, 0, 0 );

    // initialize the module selection data
    startIndex = 0;
    mnu_prime_modules(mproc);
    mnu_prime_titleimage(mproc);

    // Grab the module info from all the servers
    cl_begin_request_module(cl);
    cl_request_module_images(cl);
    cl_request_module_info(cl);

    // Figure out at what offset we want to draw the module menu.
    moduleMenuOffsetX = (gfxState.surface->w - DEFAULT_SCREEN_W) / 2;
    moduleMenuOffsetY = (gfxState.surface->h - DEFAULT_SCREEN_H) / 2;

    // navigation buttons
    ui_initWidget( mnu_widgetList + 0, 0, mnu_Font, "<-", NULL, moduleMenuOffsetX + 20, moduleMenuOffsetY + 74, 30, 30 );
    ui_initWidget( mnu_widgetList + 1, 1, mnu_Font, "->", NULL, moduleMenuOffsetX + 590, moduleMenuOffsetY + 74, 30, 30 );
    ui_initWidget( mnu_widgetList + 2, 2, mnu_Font, "Join Module", NULL, moduleMenuOffsetX + 327, moduleMenuOffsetY + 173, 200, 30 );
    ui_initWidget( mnu_widgetList + 3, 3, mnu_Font, "Back", NULL, moduleMenuOffsetX + 327, moduleMenuOffsetY + 208, 200, 30 );

    // Module "windows"
    ui_initWidget( mnu_widgetList + 4, 4, mnu_Font, NULL, NULL, moduleMenuOffsetX, moduleMenuOffsetY, 138, 138 );
    ui_initWidget( mnu_widgetList + 5, 5, mnu_Font, NULL, NULL, moduleMenuOffsetX, moduleMenuOffsetY, 138, 138 );
    ui_initWidget( mnu_widgetList + 6, 6, mnu_Font, NULL, NULL, moduleMenuOffsetX, moduleMenuOffsetY, 138, 138 );

    // Module description
    ui_initWidget( mnu_widgetList + 7, UI_Invalid, mnu_Font, NULL, NULL, moduleMenuOffsetX + 21, moduleMenuOffsetY + 173, 291, 230 );

    x = ( gfxState.surface->w / 2 ) - ( background.imgW / 2 );
    y = gfxState.surface->h - background.imgH;

    mproc->selectedModule = -1;

    menuChoice = 0;
    menuState = MM_Entering;
    break;

  case MM_Entering:
    menuState = MM_Running;
    break;

  case MM_Running:

    // keep trying to load the module images as long as the client is still deciding
    {
      cl_request_module_images(cl);
      mnu_load_cl_images(mproc);
    }

    // do the asynchronous module info download
    {
      cl_request_module_info(cl);
      cl_load_module_info(cl);

      mnu_prime_modules(mproc);

      for(i=0; i<cl->rem_mod_count; i++)
      {
        if(cl->rem_mod[i].is_hosted)
        {
          mproc->validModules[mproc->validModules_count] = i;
          mproc->validModules_count++;
        }
      }
    }

    // Draw the background
    glColor4f(1, 1, 1, 1);
    x = (gfxState.surface->w / 2) - (background.imgW / 2);
    y = gfxState.surface->h - background.imgH;
    ui_drawImage( &wBackground );

    // Draw the arrows to pick modules
    if ( BUTTON_UP == ui_doButton( mnu_widgetList + 0 ) )
    {
      startIndex--;
      if ( startIndex < 0 ) startIndex += mproc->validModules_count;
    }

    if ( BUTTON_UP == ui_doButton( mnu_widgetList + 1 ) )
    {
      startIndex++;
      if ( startIndex >= mproc->validModules_count ) startIndex -= mproc->validModules_count;
    }


    // Draw buttons for the modules that can be selected

    if(mproc->validModules_count > 0)
    {
      int imin, imax;

      mnu_widgetList[4].img = NULL;
      mnu_widgetList[5].img = NULL;
      mnu_widgetList[6].img = NULL;

      if(mproc->validModules_count == 1)
      {
        imin = 1;
        imax = imin + 1;
      }
      else if(mproc->validModules_count == 2)
      {
        imin = 1;
        imax = imin + 2;
      }
      else
      {
        imin = 0;
        imax = 3;
      }

      y = 20;
      for ( i = imin; i < imax; i++ )
      {
        int imod;

        x = 93 + i*(138 + 20);

        j = ( i + startIndex ) % mproc->validModules_count;
        imod = mproc->validModules[j];

        if(MAXMODULE != cl->rem_mod[imod].tx_title_idx)
        {
          mnu_widgetList[4+i].img = mproc->TxTitleImage + cl->rem_mod[imod].tx_title_idx;
        }

        mnu_slideButton( &wtmp, mnu_widgetList + 4+i, x, y );
        if ( BUTTON_UP == ui_doImageButton( &wtmp ) )
        {
          mproc->selectedModule = j;
        }
      }
    }

    // Draw an unused button as the backdrop for the text for now
    ui_drawButton( mnu_widgetList + 7 );

    // And draw the next & back buttons
    if (ui_doButton(mnu_widgetList + 2))
    {
      // go to the next menu with this module selected
      mproc->selectedModule = mproc->validModules[mproc->selectedModule];
      menuState = MM_Leaving;
    }

    if (ui_doButton(mnu_widgetList + 3))
    {
      // Signal doMenu to go back to the previous menu
      mproc->selectedModule = -1;
      menuState = MM_Leaving;
    }

    // Draw the text description of the selected module
    if (mproc->selectedModule > -1)
    {
      MOD_INFO * mi = cl->rem_mod + mproc->validModules[mproc->selectedModule];
        y = 173 + 5;
        x = 21 + 5;
        glColor4f( 1, 1, 1, 1 );
        fnt_drawText( mnu_Font, moduleMenuOffsetX + x, moduleMenuOffsetY + y,
                      mi->longname );
        y += 20;

        snprintf( txtBuffer, sizeof( txtBuffer ), "Difficulty: %s", mi->rank );
        fnt_drawText( mnu_Font, moduleMenuOffsetX + x, moduleMenuOffsetY + y, txtBuffer );
        y += 20;

        if ( mi->maxplayers > 1 )
        {
          if ( mi->minplayers == mi->maxplayers )
          {
            snprintf( txtBuffer, sizeof( txtBuffer ), "%d Players", mi->minplayers );
          }
          else
          {
            snprintf( txtBuffer, sizeof( txtBuffer ), "%d - %d Players", mi->minplayers, mi->maxplayers );
          }
        }
        else
        {
          if ( mi->importamount == 0 )
          {
            snprintf( txtBuffer, sizeof( txtBuffer ), "Starter Module" );
          }
          else
          {
            snprintf( txtBuffer, sizeof( txtBuffer ), "Special Module" );
          }
        }
        fnt_drawText( mnu_Font, moduleMenuOffsetX + x, moduleMenuOffsetY + y, txtBuffer );
        y += 20;

        // And finally, the summary
        snprintf( txtBuffer, sizeof( txtBuffer ), "%s" SLASH_STRING "%s" SLASH_STRING "%s" SLASH_STRING "%s", CData.modules_dir, mi->loadname, CData.gamedat_dir, CData.mnu_file );
        if ( mproc->validModules[mproc->selectedModule] != cl->rem_modtxt.val )
        {
          if ( module_read_summary( txtBuffer, &(cl->rem_modtxt) ) ) cl->rem_modtxt.val = mproc->validModules[mproc->selectedModule];
        };

        for ( i = 0;i < SUMMARYLINES;i++ )
        {
          fnt_drawText( mnu_Font, moduleMenuOffsetX + x, moduleMenuOffsetY + y, cl->rem_modtxt.summary[i] );
          y += 20;
        }
    }

    break;

  case MM_Leaving:
    menuState = MM_Finish;
    break;

  case MM_Finish:
    // release all of the network connections
    cl_end_request_module(cl);

    menuState = MM_Begin;

    if (-1 == mproc->selectedModule)
    {
      result = -1;  // Back
    }
    else
    {
      // Save the module info of the picked module
      cl->selectedModule = mproc->selectedModule;
      memcpy(&cl->req_mod, &cl->rem_mod[cl->selectedModule], sizeof(MOD_INFO));
      ModState_new(&(cl->req_modstate), &(cl->req_mod), cl->req_seed);

      if( CClient_connect(cl, cl->rem_mod[cl->selectedModule].host) )
      {
        net_startNewSysPacket(&egopkt);
        sys_packet_addUint16(&egopkt, TO_HOST_REQUEST_MODULE);
        sys_packet_addString(&egopkt, cl->req_mod.loadname);
        Client_sendPacketToHost(cl, &egopkt);

        // wait for up to 2 minutes to transfer needed files
        // this probably needs to be longer
        wait_return = net_waitForPacket(cl_getHost()->asynch, cl->gamePeer, 2*60000, NET_DONE_SENDING_FILES, NULL);
        CClient_disconnect(cl);
        if(rv_fail == wait_return || rv_error == wait_return) assert(bfalse);

        result = 1;
      }
      else
      {
        // cannot connect to the host
        result = 0;
      }
    }

    ui_Reset();

    // delete the background texture as we exit
    GLtexture_delete( &background );

    break;

  }

  return result;
}

//--------------------------------------------------------------------------------------------
int mnu_handleKeyboard( MenuProc_t * mproc )
{
  int retval = 0;
  bool_t control, alt, shift, mod;

  control = (SDLKEYDOWN(SDLK_RCTRL) || SDLKEYDOWN(SDLK_LCTRL));
  alt     = (SDLKEYDOWN(SDLK_RALT) || SDLKEYDOWN(SDLK_LALT));
  shift   = (SDLKEYDOWN(SDLK_RSHIFT) || SDLKEYDOWN(SDLK_LSHIFT));
  mod     = control || alt || shift;

  // Check for quitters
  if ( SDLKEYDOWN( SDLK_ESCAPE ) && !mod && !mproc->proc.Paused  )
  {
    retval = -1;
  }

  return retval;
};

//--------------------------------------------------------------------------------------------
int mnu_doIngameQuit( MenuProc_t * mproc, float deltaTime )
{
  static MenuProcs menuState = MM_Begin;
  static float lerp;
  static int menuChoice = 0;
  static char * buttons[] = { "Quit", "Continue", "" };

  int result = 0;

  Game_t * gs = Graphics_requireGame(&gfxState);

  switch ( menuState )
  {
    case MM_Begin:
      printf("mnu_doIngameQuit()\n");

      // set up menu variables
      mnu_widgetCount = mnu_initWidgetsList( mnu_widgetList, MAXWIDGET, buttons );
      initSlidyButtons( 1.0f, mnu_widgetList, mnu_widgetCount );

      mous.game = bfalse;

      menuChoice = 0;
      menuState = MM_Entering;
      break;

    case MM_Entering:
      // do buttons sliding in animation, and background fading in
      // background

      mnu_drawSlidyButtons();
      mnu_updateSlidyButtons( -deltaTime );

      // Let lerp wind down relative to the time elapsed
      if ( SlidyButtonState_t.lerp <= 0.0f )
      {
        menuState = MM_Running;
      }

      break;

    case MM_Running:
      // Do normal run
      // Background

      // Buttons
      if ( BUTTON_UP == ui_doButton( mnu_widgetList + 0 ) )
      {
        // "quit"
        menuChoice = -1;
      }

      if ( BUTTON_UP == ui_doButton( mnu_widgetList + 1 ) )
      {
        // "continue"
        menuChoice = 1;
      }

      if ( menuChoice != 0 )
      {
        menuState = MM_Leaving;
        mnu_widgetCount = mnu_initWidgetsList( mnu_widgetList, MAXWIDGET, buttons );
        initSlidyButtons( 0.0f, mnu_widgetList, mnu_widgetCount );
      }
      break;

    case MM_Leaving:
      // Do buttons sliding out and background fading
      // Do the same stuff as in MM_Entering, but backwards

      // Buttons
      mnu_drawSlidyButtons();
      mnu_updateSlidyButtons( deltaTime );
      if ( SlidyButtonState_t.lerp >= 1.0f )
      {
        menuState = MM_Finish;
      }
      break;

    case MM_Finish:
      menuState = MM_Begin; // Make sure this all resets next time mnu_doMain is called

      // Set the next menu to load
      //result = menuChoice;
      result = 1;
      mproc->whichMenu = mnu_EndGame;

      ui_Reset();
      break;

  };

  return result;
}


//--------------------------------------------------------------------------------------------
int mnu_doIngameInventory( MenuProc_t * mproc, float deltaTime )
{
  static MenuProcs menuState = MM_Begin;
  static float lerp;
  static int menuChoice = 0;
  int i,j,k;
  Uint16 itex;
  OBJ_REF iobj;
  CHR_REF ichr, iitem;
  int result = 0, offsetX, offsetY;
  static Uint32 starttime;
  int iw, ih;
  int total_inpack = 0;

  Game_t   * gs = Graphics_requireGame(&gfxState);
  Client_t * cl = mproc->cl;

  if( !EKEY_PVALID(gs) ) return -1;

  iw = iconrect.right  - iconrect.left;
  ih = iconrect.bottom - iconrect.top;

  switch ( menuState )
  {

    case MM_Begin:

      printf("mnu_doIngameInventory()\n");

      mous.game = bfalse;

      // set up menu variables
      for(i=0, k=0; i<MAXSTAT; i++)
      {
        offsetX = cl->StatList[i].pos.x - 5*iw;
        offsetY = cl->StatList[i].pos.y;

        ichr    = cl->StatList[i].chr_ref;
        if(!ACTIVE_CHR(gs->ChrList, ichr)) continue;

        iitem = ichr;
        for(j=0; j<gs->ChrList[ichr].numinpack; j++)
        {
          iitem = gs->ChrList[iitem].nextinpack;
          if(!ACTIVE_CHR(gs->ChrList, iitem)) break;

          iobj = gs->ChrList[iitem].model;
          if(!VALID_OBJ(gs->ObjList, iobj)) break;

          itex = gs->skintoicon[gs->ChrList[iitem].skin_ref + gs->ObjList[iobj].skinstart];
          ui_initWidget( mnu_widgetList + k, k, mnu_Font, NULL, gs->TxIcon + itex, offsetX + iw*j, offsetY, iw, ih );

          k++;
          total_inpack++;
        }
        j++;
      }
      mnu_widgetCount = k;

      initSlidyButtons( 1.0f, mnu_widgetList, mnu_widgetCount );

      starttime = SDL_GetTicks();
      menuChoice = 0;

      if(0==total_inpack)
      {
        menuState = MM_Leaving;
      }
      else
      {
        menuState = MM_Entering;
      }
      break;

    case MM_Entering:
      // do buttons sliding in animation, and background fading in
      // background

      mnu_drawSlidyButtons();
      mnu_updateSlidyButtons( -deltaTime );

      // Let lerp wind down relative to the time elapsed
      if ( SlidyButtonState_t.lerp <= 0.0f )
      {
        menuState = MM_Running;
      }

      break;

    case MM_Running:
      // Do normal run
      // Background

      for(i=0; i<mnu_widgetCount; i++)
      {
        ui_drawImage( mnu_widgetList + i );
      }

      if(SDL_GetTicks() - starttime > 5000)
      {
        initSlidyButtons( 0.0f, mnu_widgetList, mnu_widgetCount );
        menuState = MM_Leaving;
      }

      break;

    case MM_Leaving:
      // Do buttons sliding out and background fading
      // Do the same stuff as in MM_Entering, but backwards

      // Buttons
      mnu_drawSlidyButtons();
      mnu_updateSlidyButtons( deltaTime );
      if ( SlidyButtonState_t.lerp >= 1.0f )
      {
        menuState = MM_Finish;
      }
      break;

    case MM_Finish:
      // Free the background texture; don't need to hold onto it

      menuState = MM_Begin; // Make sure this all resets next time mnu_doMain is called

      // Set the next menu to load
      result = -1;


      mous.game = btrue;

      ui_Reset();
      break;

  };

  return result;
}

//--------------------------------------------------------------------------------------------
int mnu_doIngameEndGame( MenuProc_t * mproc, float deltaTime )
{
  static MenuProcs menuState = MM_Begin;
  static int menuChoice = 0;

  static char notImplementedMessage[] = "Not implemented yet!  Check back soon!";
  int result = 0;

  Game_t * gs = Graphics_requireGame(&gfxState);

  int x, y;
  int w, h;

  switch ( menuState )
  {
    case MM_Begin:
      printf("mnu_doIngameEndGame()\n");

      fnt_getTextSize( ui_getTTFont(), gs->endtext , &w, &h );
      w += 50; // add some space on the sides

      x = ( gfxState.surface->w - w ) / 2;
      y = ( gfxState.surface->h - 34 ) / 2;

      ui_initWidget( mnu_widgetList + 0, UI_Invalid, mnu_Font, gs->endtext, NULL, x, y, w, 30 );

      menuChoice = 0;
      menuState = MM_Entering;
      break;

    case MM_Entering:

      menuState = MM_Running;
      break;


    case MM_Running:

      if ( BUTTON_UP == ui_doButton( mnu_widgetList + 0 ) )
      {
        menuState = MM_Leaving;
      }

      break;


    case MM_Leaving:
      menuState = MM_Finish;
      break;


    case MM_Finish:

      menuState = MM_Begin;

      result = mproc->MenuResult = -1;
      if(NULL != gs && -1 == result)
      {
        gs->proc.KillMe = btrue;
      }

      mous.game = btrue;

      ui_Reset();
      break;
  }

  return result;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

MenuProc_t * MenuProc_new(MenuProc_t *mproc)
{
  //fprintf( stdout, "MenuProc_new()\n");

  int i;

  if(NULL == mproc) return mproc;

  MenuProc_delete( mproc );

  memset(mproc, 0, sizeof(MenuProc_t));

  EKEY_PNEW( mproc, MenuProc_t );

  ProcState_new( &(mproc->proc) );

  mproc->selectedModule     = -1;
  mproc->validModules_count = 0;
  memset(mproc->validModules, 0, sizeof(int) * MAXMODULE);

  for(i=0; i<MAXMODULE; i++)
  {
    GLtexture_new( mproc->TxTitleImage + i );
  };


  return mproc;
}

//--------------------------------------------------------------------------------------------
bool_t MenuProc_delete(MenuProc_t * ms)
{
  int i;
  bool_t retval = bfalse;

  if(NULL == ms) return bfalse;
  if(!EKEY_PVALID(ms)) return btrue;

  EKEY_PINVALIDATE(ms);

  retval = ProcState_delete(&(ms->proc));

  for(i=0; i<MAXMODULE; i++)
  {
    GLtexture_delete( ms->TxTitleImage + i );
  };

  return retval;
}

//--------------------------------------------------------------------------------------------
MenuProc_t * MenuProc_renew(MenuProc_t *ms)
{
  MenuProc_delete(ms);
  return MenuProc_new(ms);
}

//--------------------------------------------------------------------------------------------
bool_t MenuProc_init(MenuProc_t * ms)
{
  if(NULL == ms) return bfalse;

  if(!EKEY_PVALID(ms))
  {
    MenuProc_new(ms);
  }

  ProcState_init( &(ms->proc) );

  ms->lastMenu  = mnu_Main;
  ms->whichMenu = mnu_Main;

  return btrue;
}

//--------------------------------------------------------------------------------------------
retval_t MenuProc_ensure_server(MenuProc_t * ms, Game_t * gs)
{
  if( !EKEY_PVALID(ms) ) return rv_fail;

  if( !EKEY_PVALID(ms->sv) )
  {
    ms->sv = CServer_create(gs);
  }

  return (NULL != ms->sv) ? rv_succeed : rv_error;
};

//--------------------------------------------------------------------------------------------
retval_t   MenuProc_ensure_client(MenuProc_t * ms, Game_t * gs)
{
  if( !EKEY_PVALID(ms) ) return rv_fail;

  if( !EKEY_PVALID(ms->cl) )
  {
    ms->cl = Client_create(gs);
  }

  return  (NULL != ms->cl) ? rv_succeed : rv_error;
};

//--------------------------------------------------------------------------------------------
retval_t   MenuProc_ensure_network(MenuProc_t * ms, Game_t * gs)
{
  if( !EKEY_PVALID(ms) ) return rv_fail;

  if( !EKEY_PVALID(ms->net) )
  {
    ms->net = CNet_create(gs);
  }

  return  (NULL != ms->net) ? rv_succeed : rv_error;
};


//--------------------------------------------------------------------------------------------
retval_t   MenuProc_start_server(MenuProc_t * ms, Game_t * gs)
{
  retval_t ret;

  // ensure that there is a server
  ret = MenuProc_ensure_server(ms, gs);
  if(rv_succeed != ret) return ret;

  // start up the server
  ret = CServer_startUp(ms->sv);

  return ret;
}

//--------------------------------------------------------------------------------------------
retval_t   MenuProc_start_client(MenuProc_t * ms, Game_t * gs)
{
  retval_t ret;

  // ensure that there is a server
  ret = MenuProc_ensure_client(ms, gs);
  if(rv_succeed != ret) return ret;

  // start up the server
  ret = Client_startUp(ms->cl);

  return ret;
}

//--------------------------------------------------------------------------------------------
retval_t   MenuProc_start_network(MenuProc_t * ms, Game_t * gs)
{
  assert( bfalse );
  return rv_fail;
}

//--------------------------------------------------------------------------------------------
retval_t   MenuProc_start_netfile(MenuProc_t * ms, Game_t * gs)
{
  retval_t ret;

  // ensure that there is a network
  ret = MenuProc_ensure_network(ms, gs);
  if(rv_succeed != ret) return ret;

  // start up NetFile
  ret = NFileState_startUp( ms->net->nfs );

  return ret;
};

//--------------------------------------------------------------------------------------------
bool_t MenuProc_resynch(MenuProc_t * ms, Game_t * gs)
{
  if( !EKEY_PVALID(ms) || !EKEY_PVALID(gs) ) return bfalse;

  ms->cl  = gs->cl;
  ms->sv  = gs->sv;
  ms->net = gs->ns;

  return btrue;
};

//--------------------------------------------------------------------------------------------
bool_t MenuProc_init_ingame(MenuProc_t * ms)
{
  if(NULL == ms) return bfalse;

  if(!EKEY_PVALID(ms))
  {
    MenuProc_new(ms);
  }

  ProcState_init( &(ms->proc) );

  ms->lastMenu  = mnu_Quit;
  ms->whichMenu = mnu_Quit;

  return btrue;
}

//--------------------------------------------------------------------------------------------
Uint32 mnu_load_titleimage(MenuProc_t * mproc, Uint32 titleimage, char *szLoadName)
{
  // ZZ> This function loads a title in the specified image slot, forcing it into
  //     system memory.  Returns btrue if it worked
  Uint32 retval = MAXMODULE;

  if(INVALID_TEXTURE != GLtexture_Load(GL_TEXTURE_2D,  mproc->TxTitleImage + titleimage, szLoadName, INVALID_KEY))
  {
    retval = titleimage;
  }

  return retval;
}

//--------------------------------------------------------------------------------------------
size_t mnu_load_mod_data(MenuProc_t * mproc, MOD_INFO * mi_ary, size_t mi_len)
{
  // ZZ> This function loads the title image for each module.  Modules without a
  //     title are marked as invalid

  char searchname[15];
  STRING loadname;
  const char *FileName;
  FILE* filesave;
  size_t modcount;

  FS_FIND_INFO fs_finfo;

  fs_find_info_new( &fs_finfo );

  // Convert searchname
  strcpy( searchname, "modules" SLASH_STRING "*.mod" );

  // Log a directory list
  filesave = fs_fileOpen( PRI_NONE, NULL, CData.modules_file, "w" );
  if ( NULL != filesave  )
  {
    fprintf( filesave, "This file logs all of the modules found\n" );
    fprintf( filesave, "** Denotes an invalid module (Or locked)\n\n" );
  }
  else
  {
    log_warning( "Could not write to %s\n", CData.modules_file );
  }

  // Search for .mod directories
  FileName = fs_findFirstFile( &fs_finfo, CData.modules_dir, NULL, "*.mod" );
  modcount = 0;
  while ( FileName && modcount < mi_len )
  {
    strncpy( mi_ary[modcount].loadname, FileName, sizeof( mi_ary[modcount].loadname ) );
    snprintf( loadname, sizeof( loadname ), "%s" SLASH_STRING "%s" SLASH_STRING "%s" SLASH_STRING "%s", CData.modules_dir, FileName, CData.gamedat_dir, CData.mnu_file );
    if ( module_read_data( mi_ary + modcount, loadname ) )
    {
      snprintf( CStringTmp1, sizeof( CStringTmp1 ), "%s" SLASH_STRING "%s" SLASH_STRING "%s" SLASH_STRING "%s", CData.modules_dir, FileName, CData.gamedat_dir, CData.title_bitmap );
      mi_ary[modcount].tx_title_idx = mnu_load_titleimage( mproc, modcount, CStringTmp1 );
      if ( MAXMODULE != mi_ary[modcount].tx_title_idx )
      {
        fprintf( filesave, "%02d.  %s\n", modcount, mi_ary[modcount].longname );
        modcount++;
      }
      else
      {
        fprintf( filesave, "**.  %s\n", FileName );
      }
    }
    else
    {
      fprintf( filesave, "**.  %s\n", FileName );
    }
    FileName = fs_findNextFile(&fs_finfo);
  }
  fs_findClose(&fs_finfo);
  if ( NULL != filesave  ) fs_fileClose( filesave );

  return modcount;
}

//--------------------------------------------------------------------------------------------
bool_t mnu_load_cl_images(MenuProc_t *mproc)
{
  // BB> This function loads the title image(s) for the modules that the client
  //     is browsing

  int cnt;
  bool_t all_loaded = btrue;
  Client_t * cl = mproc->cl;

  all_loaded = btrue;
  for(cnt=0; cnt<MAXMODULE; cnt++)
  {
    // check to see if the module has been loaded
    if(MAXMODULE==cl->rem_mod[cnt].tx_title_idx && cl->rem_mod[cnt].is_hosted)
    {
      // see if the the file has appeared locally
      snprintf(CStringTmp1, sizeof(CStringTmp1), "%s" SLASH_STRING "%s" SLASH_STRING "%s" SLASH_STRING "%s", CData.modules_dir, cl->rem_mod[cnt].loadname, CData.gamedat_dir, CData.title_bitmap);
      cl->rem_mod[cnt].tx_title_idx = mnu_load_titleimage(mproc, cnt, CStringTmp1);

      if(MAXMODULE==cl->rem_mod[cnt].tx_title_idx)
      {
        all_loaded = bfalse;
        break;
      }
    }
  }

  return all_loaded;
}

//---------------------------------------------------------------------------------------------
void mnu_free_all_titleimages(MenuProc_t * mproc)
{
  // ZZ> This function clears out all of the title images

  int cnt;
  for ( cnt = 0; cnt < MAXMODULE; cnt++ )
  {
    GLtexture_Release( mproc->TxTitleImage + cnt );
  }
}


//---------------------------------------------------------------------------------------------
void mnu_prime_modules(MenuProc_t * mproc)
{
  mproc->selectedModule     = -1;
  mproc->validModules_count = 0;
  memset(mproc->validModules, 0, sizeof(int) * MAXMODULE);

  if(NULL != mproc->cl)
  {
    mproc->cl->selectedModule = -1;
  }

  if(NULL != mproc->sv)
  {
    mproc->sv->selectedModule = -1;
  }
}

//---------------------------------------------------------------------------------------------
retval_t mnu_ensure_game(MenuProc_t * mproc, Game_t ** optional)
{
  bool_t  is_valid = bfalse;
  Game_t * ptmp     = NULL;

  // make sure that we have a Game_t somewhere
  if(NULL != optional) ptmp = *optional;
  if(NULL == ptmp    ) ptmp = Graphics_getGame(&gfxState);
  if(NULL == ptmp    )
  {
    // create a new game
    Net_t    * net = NULL;
    Client_t * cl  = NULL;
    Server_t * sv  = NULL;

    if( EKEY_PVALID(mproc) )
    {
      net = mproc->net;
      cl  = mproc->cl;
      sv  = mproc->sv;
    }

    ptmp = Game_create(net, cl, sv);
  }

  // make sure that Game_t is known
  is_valid = EKEY_PVALID(ptmp);
  if( is_valid )
  {
    if(NULL != optional    ) *optional   = ptmp;
    Graphics_ensureGame(&gfxState, ptmp);
  }

  return is_valid ? rv_succeed : rv_error;
}

//---------------------------------------------------------------------------------------------
retval_t mnu_upload_game_info(Game_t * gs, MenuProc_t * ms)
{
  // BB > register the various networking modules with the given game

  if( !EKEY_PVALID(gs) || !EKEY_PVALID(ms)) return rv_fail;

  Game_registerNetwork( gs, ms->net, btrue );
  Game_registerClient ( gs, ms->cl,  btrue );
  Game_registerServer ( gs, ms->sv,  btrue );

  return rv_succeed;
};