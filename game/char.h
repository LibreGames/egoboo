#pragma once

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

/// @file char.h
/// @note You will routinely include "char.h" only in headers (*.h) files where you need to declare an
///       object of team_t or ego_chr. In *.inl files or *.c/*.cpp files you will routinely include "char.inl", instead.

#include "character_defs.h"

#include "egoboo_object.h"

#include "file_formats/cap_file.h"
#include "graphic_mad.h"

#include "sound.h"
#include "script.h"
#include "md2.h"
#include "graphic.h"
#include "physics.h"
#include "bsp.h"

#include "egoboo.h"

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct ego_mad;
struct ego_eve;
struct ego_pip;
struct ego_pro;
struct ego_billboard_data;
struct ego_ai_state;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// no bounds checking
#define IS_FLYING_CHR_RAW(ICHR)   ( (ChrList.lst[ICHR].is_flying_jump || ChrList.lst[ICHR].is_flying_platform) )
#define IS_PACKED_CHR_RAW(ICHR)   ( ChrList.lst[ICHR].pack.is_packed )
#define IS_ATTACHED_CHR_RAW(ICHR) ( (DEFINED_CHR(ChrList.lst[ICHR].attachedto) || ChrList.lst[ICHR].pack.is_packed) )

#define IS_INVICTUS_PCHR_RAW(PCHR) ( ( VALID_PLA( (PCHR)->is_which_player ) ? PlaStack.lst[(PCHR)->is_which_player].wizard_mode : bfalse ) || (PCHR)->invictus )
#define IS_FLYING_PCHR_RAW(PCHR)   ( ((PCHR)->is_flying_jump || (PCHR)->is_flying_platform) )
#define IS_PACKED_PCHR_RAW(PCHR)   ( (PCHR)->pack.is_packed )
#define IS_ATTACHED_PCHR_RAW(PCHR) ( (DEFINED_CHR((PCHR)->attachedto) || (PCHR)->pack.is_packed) )

// bounds checking
#define IS_FLYING_CHR(ICHR)   ( !DEFINED_CHR(ICHR) ? bfalse : IS_FLYING_CHR_RAW(ICHR)   )
#define IS_PACKED_CHR(ICHR)   ( !DEFINED_CHR(ICHR) ? bfalse : IS_PACKED_CHR_RAW(ICHR)   )
#define IS_ATTACHED_CHR(ICHR) ( !DEFINED_CHR(ICHR) ? bfalse : IS_ATTACHED_CHR_RAW(ICHR) )

#define IS_INVICTUS_PCHR(PCHR) ( !DEFINED_PCHR(PCHR) ? bfalse : IS_INVICTUS_PCHR_RAW(PCHR) )
#define IS_FLYING_PCHR(PCHR)   ( !DEFINED_PCHR(PCHR) ? bfalse : IS_FLYING_PCHR_RAW(PCHR)   )
#define IS_PACKED_PCHR(PCHR)   ( !DEFINED_PCHR(PCHR) ? bfalse : IS_PACKED_PCHR_RAW(PCHR)   )
#define IS_ATTACHED_PCHR(PCHR) ( !DEFINED_PCHR(PCHR) ? bfalse : IS_ATTACHED_PCHR_RAW(PCHR) )

//--------------------------------------------------------------------------------------------

/// The description of a single team
struct team_t
{
    bool_t   hatesteam[TEAM_MAX];    ///< Don't damage allies...
    Uint16   morale;                 ///< Number of characters on team
    CHR_REF  leader;                 ///< The leader of the team
    CHR_REF  sissy;                  ///< Whoever called for help last
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

struct ego_cap_data : public s_cap_data
{
    static ego_cap_data * init( ego_cap_data * ptr ) {  s_cap_data * rv = cap_data_init( ptr ); /* plus whatever else we need to add */ return NULL == rv ? NULL : ptr; }
};

//--------------------------------------------------------------------------------------------
struct ego_cap : public ego_cap_data
{
    static ego_cap * init( ego_cap * ptr ) {  ego_cap_data * rv = ego_cap_data::init( ptr ); /* plus whatever else we need to add */ return NULL == rv ? NULL : ptr; }
};

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// The latch used by the characters/ai's in the game
struct latch_game_t
{
    bool_t     raw_valid; ///< does this latch have any valid data in it?
    latch_2d_t raw;       ///< the raw control settings

    bool_t     trans_valid; ///< does this latch have any valid data in it?
    latch_3d_t trans;       ///< the translated control values, relative to the camera
};

void latch_game_init( latch_game_t * platch );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------

/// Everything that is necessary to compute the character's interaction with the environment
struct chr_environment_t
{
    // floor stuff
    float   grid_level;           ///< Height the current grid
    float   grid_lerp;
    Uint8   grid_twist;

    float   walk_level;           ///< Height of tile or platform (or water if the character has waterwalk)
    float   walk_lerp;
    fvec3_t walk_nrm;             ///< the normal to whatever the character is standing on

    float   fly_level;            ///< Height of tile, platform, or water, whichever is highest.
    float   fly_lerp;

    // stuff related to the surface we are standing on (if any)
    bool_t  grounded;            ///< standing on something?
    fvec3_t ground_vel;          ///< the velocity of the current "ground"
    fvec3_t ground_diff;         ///< the relative velocity between the character and the current "ground"
    float   ground_fric;         ///< the current friction between the character and the ground

    // friction stuff
    bool_t is_slipping;
    bool_t is_slippy,    is_watery;
    float  air_friction, ice_friction;
    float  fluid_friction_hrz, fluid_friction_vrt;
    float  traction, friction_hrz;

    // misc states
    bool_t   inwater;
    fvec3_t  chr_vel, chr_acc;
    fvec3_t  acc;

    fvec3_t  legs_vel;
};

//--------------------------------------------------------------------------------------------
struct pack_t
{
    bool_t         is_packed;    ///< Is it in the inventory?
    bool_t         was_packed;   ///< Temporary thing...
    CHR_REF        next;        ///< Link to the next item
    int            count;       ///< How many
};

bool_t pack_add_item( pack_t * ppack, CHR_REF item );
bool_t pack_remove_item( pack_t * ppack, CHR_REF iparent, CHR_REF iitem );

#define PACK_BEGIN_LOOP(IT,INIT) IT = INIT; while( MAX_CHR != IT ) { CHR_REF IT##_internal = ChrList.lst[IT].pack.next;
#define PACK_END_LOOP(IT) IT = IT##_internal; }

//--------------------------------------------------------------------------------------------

/// the data used to define the spawning of a character
struct chr_spawn_data_t
{
    fvec3_t     pos;
    PRO_REF     profile;
    TEAM_REF    team;
    Uint8       skin;
    FACING_T    facing;
    STRING      name;
    CHR_REF     override;
};

//--------------------------------------------------------------------------------------------

/// The definition of the character object
/// This "inherits" for ego_object
struct ego_chr_data
{
    // character state
    ego_ai_state     ai;              ///< ai data
    latch_game_t   latch;           ///< the latch data

    // character stats
    STRING         name;            ///< My name
    Uint8          gender;          ///< Gender

    Uint8          lifecolor;       ///< Bar color
    SFP8_T         life;            ///< Basic character stats
    SFP8_T         lifemax;         ///< 8.8 fixed point
    UFP8_T         life_heal;       ///< 8.8 fixed point
    SFP8_T         life_return;     ///< Regeneration/poison - 8.8 fixed point

    Uint8          manacolor;       ///< Bar color
    SFP8_T         mana;            ///< Mana stuff
    SFP8_T         manamax;         ///< 8.8 fixed point
    SFP8_T         manaflow;        ///< 8.8 fixed point
    SFP8_T         manareturn;      ///< 8.8 fixed point

    SFP8_T         strength;        ///< Strength     - 8.8 fixed point
    SFP8_T         wisdom;          ///< Wisdom       - 8.8 fixed point
    SFP8_T         intelligence;    ///< Intelligence - 8.8 fixed point
    SFP8_T         dexterity;       ///< Dexterity    - 8.8 fixed point

    Uint32         experience;                    ///< Experience
    Uint8          experiencelevel;               ///< Experience Level

    pack_t         pack;             ///< what the character is holding

    Sint16         money;            ///< Money
    Uint8          ammomax;          ///< Ammo stuff
    Uint16         ammo;
    CHR_REF        holdingwhich[SLOT_COUNT]; ///< !=MAX_CHR if character is holding something
    CHR_REF        inventory[INVEN_COUNT];   ///< !=MAX_CHR if character is storing something

    // team stuff
    TEAM_REF       team;            ///< Character's team
    TEAM_REF       baseteam;        ///< Character's starting team

    // enchant data
    ENC_REF        firstenchant;                  ///< Linked list for enchants
    ENC_REF        undoenchant;                   ///< Last enchantment spawned

    float          fat;                           ///< Character's size
    float          fat_stt;                       ///< Character's initial size
    float          fat_goto;                      ///< Character's size goto
    Sint16         fat_goto_time;                 ///< Time left in size change

    // jump stuff
    float          jump_power;                    ///< Jump power
    Uint8          jump_time;                     ///< Delay until next jump
    Uint8          jump_number;                   ///< Number of jumps remaining
    Uint8          jump_number_reset;             ///< Number of jumps total, 255=Flying
    bool_t         jump_ready;                    ///< For standing on a platform character

    // attachments
    CHR_REF        attachedto;                    ///< !=MAX_CHR if character is a held weapon
    slot_t         inwhich_slot;                  ///< SLOT_LEFT or SLOT_RIGHT

    // platform stuff
    bool_t         platform;                      ///< Can it be stood on
    bool_t         canuseplatforms;               ///< Can use platforms?
    float          holdingweight;                 ///< For weighted buttons
    float          targetplatform_overlap;        ///< What is the height of the target platform?
    CHR_REF        targetplatform_ref;            ///< Am I trying to attach to a platform?
    CHR_REF        onwhichplatform_ref;           ///< Am I on a platform?
    Uint32         onwhichplatform_update;        ///< When was the last platform attachment made?
    float          onwhichplatform_weight;        ///< How much weight did I put on the platform?

    // combat stuff
    Uint8          damagetargettype;              ///< Type of damage for AI DamageTarget
    Uint8          reaffirmdamagetype;            ///< For relighting torches
    Uint8          damagemodifier[DAMAGE_COUNT];  ///< Resistances and inversion
    Uint8          defense;                       ///< Base defense rating
    SFP8_T         damageboost;                   ///< Add to swipe damage
    SFP8_T         damagethreshold;               ///< Damage below this number is ignored

    // sound stuff
    Sint8          sound_index[SOUND_COUNT];       ///< a map for soundX.wav to sound types
    int            loopedsound_channel;           ///< Which sound channel it is looping on, -1 is none.

    // missile handling
    Uint8          missiletreatment;              ///< For deflection, etc.
    Uint8          missilecost;                   ///< Mana cost for each one
    CHR_REF        missilehandler;                ///< Who pays the bill for each one...

    // "variable" properties
    bool_t         is_hidden;
    bool_t         alive;                         ///< Is it alive?
    bool_t         waskilled;                     ///< Fix for network
    PLA_REF        is_which_player;               ///< btrue = player
    bool_t         islocalplayer;                 ///< btrue = local player
    bool_t         invictus;                      ///< Totally invincible?
    bool_t         iskursed;                      ///< Can't be dropped?
    bool_t         nameknown;                     ///< Is the name known?
    bool_t         ammoknown;                     ///< Is the ammo known?
    bool_t         hitready;                      ///< Was it just dropped?
    bool_t         isequipped;                    ///< For boots and rings and stuff

    // "constant" properties
    bool_t         isitem;                        ///< Is it grabbable?
    bool_t         cangrabmoney;                  ///< Picks up coins?
    bool_t         openstuff;                     ///< Can it open chests/doors?
    bool_t         stickybutt;                    ///< Rests on floor
    bool_t         isshopitem;                    ///< Spawned in a shop?
    bool_t         ismount;                       ///< Can you ride it?
    bool_t         canbecrushed;                  ///< Crush in a door?
    bool_t         canchannel;                    ///< Can it convert life to mana?
    Sint16         manacost;                      ///< Mana cost to use

    // misc timers
    Sint16         grogtime;                      ///< Grog timer
    Sint16         dazetime;                      ///< Daze timer
    Sint16         boretime;                      ///< Boredom timer
    Uint8          carefultime;                   ///< "You hurt me!" timer
    Uint16         reloadtime;                    ///< Time before another shot
    Uint8          damagetime;                    ///< Invincibility timer

    // graphics info
    Uint8          flashand;        ///< 1,3,7,15,31 = Flash, 255 = Don't
    bool_t         transferblend;   ///< Give transparency to weapons?
    bool_t         draw_icon;       ///< Show the icon?
    Uint8          sparkle;         ///< Sparkle color or 0 for off
    bool_t         StatusList_on;   ///< Display stats?
    SFP8_T         uoffvel;         ///< Moving texture speed
    SFP8_T         voffvel;
    float          shadow_size;      ///< Current size of shadow
    float          shadow_size_save; ///< Without size modifiers
    float          shadow_size_stt;  ///< Initial shadow size
    BBOARD_REF     ibillboard;       ///< The attached billboard

    // model info
    bool_t         is_overlay;                    ///< Is this an overlay? Track aitarget...
    Uint16         skin;                          ///< Character's skin
    PRO_REF        profile_ref;                      ///< Character's profile
    PRO_REF        basemodel_ref;                     ///< The true form
    Uint8          alpha_base;
    Uint8          light_base;
    chr_instance_t inst;                          ///< the render data

    // Skills
    int           darkvision_level;
    int           see_kurse_level;
    int           see_invisible_level;
    IDSZ_node_t      skills[MAX_IDSZ_MAP_SIZE];

    /// collision info

    /// @note - to make it easier for things to "hit" one another (like a damage particle from
    ///        a torch hitting a grub bug), Aaron sometimes made the bumper size much different
    ///        than the shape of the actual object.
    ///        The old bumper data that is read from the data.txt file will be kept in
    ///        the struct "bump". A new bumper that actually matches the size of the object will
    ///        be kept in the struct "collision"
    ego_bumper     bump_stt;
    ego_bumper     bump;
    ego_bumper     bump_save;

    ego_bumper     bump_1;       ///< the loosest collision volume that mimics the current bump
    ego_oct_bb       chr_cv;       ///< the collision volume determined by the model's points.
    ego_oct_bb       chr_min_cv;   ///< the smallest collision volume.
    ego_oct_bb       chr_max_cv;   ///< the largest collision volume. For character, particle, and platform collisions.

    Uint8        stoppedby;                     ///< Collision mask

    // character location data
    fvec3_t        pos_stt;                       ///< Starting position
    fvec3_t        pos;                           ///< Character's position
    fvec3_t        vel;                           ///< Character's velocity
    orientation_t  ori;                           ///< Character's orientation

    fvec3_t        pos_old;                       ///< Character's last position
    fvec3_t        vel_old;                       ///< Character's last velocity
    orientation_t  ori_old;                       ///< Character's last orientation

    Uint32         onwhichgrid;                   ///< Where the char is
    Uint32         onwhichblock;                  ///< The character's collision block
    CHR_REF        bumplist_next;                 ///< Next character on fanblock

    // movement properties
    bool_t         waterwalk;                     ///< Always above watersurfacelevel?
    TURN_MODE      turnmode;                      ///< Turning mode

    BIT_FIELD      movement_bits;                 ///< What movement modes are allowed?
    float          anim_speed_sneak;              ///< Movement rate of the sneak animation
    float          anim_speed_walk;               ///< Walking if above this speed
    float          anim_speed_run;                ///< Running if above this speed
    float          maxaccel;                      ///< The current maxaccel_reset
    float          maxaccel_reset;                ///< The actual maximum acceleration

    bool_t         is_flying_platform;             ///< The object is flying by manipulating the fly_height variable
    bool_t         can_fly_jump;                   ///< The object can fly by jumping (jump_number_reset == JUMP_NUMBER_INFINITE)
    bool_t         is_flying_jump;                 ///< The object can_fly_jump and has jumped
    float          fly_height;                     ///< Height to stabilize at

    // data for doing the physics in bump_all_objects()
    phys_data_t       phys;
    chr_environment_t enviro;
    ego_BSP_leaf          bsp_leaf;

    float             targetmount_overlap;
    CHR_REF           targetmount_ref;
    int               dismount_timer;                ///< a timer BB added in to make mounts and dismounts not so unpredictable
    CHR_REF           dismount_object;               ///< the object that you were dismounting from

    bool_t         safe_valid;                    ///< is the last "safe" position valid?
    fvec3_t        safe_pos;                      ///< the last "safe" position
    Uint32         safe_time;                     ///< the last "safe" time
    Uint32         safe_grid;                     ///< the last "safe" grid

    breadcrumb_list_t crumbs;                     ///< a list of previous valid positions that the object has passed through

    ego_chr_data();
    ~ego_chr_data();
};

struct ego_chr : public ego_chr_data
{
    ego_object obj_base;
    chr_spawn_data_t  spawn_data;

    ego_chr();
    ~ego_chr();

    static ego_chr * ctor( ego_chr * );
    static ego_chr * dtor( ego_chr * );

    // global chr configuration functions
    static ego_chr * run_object( ego_chr * pchr );
    static ego_chr * run_object_construct( ego_chr * pprt, int max_iterations );
    static ego_chr * run_object_initialize( ego_chr * pprt, int max_iterations );
    static ego_chr * run_object_activate( ego_chr * pprt, int max_iterations );
    static ego_chr * run_object_deinitialize( ego_chr * pprt, int max_iterations );
    static ego_chr * run_object_deconstruct( ego_chr * pprt, int max_iterations );

    static bool_t    request_terminate( const CHR_REF by_reference ichr );

    // matrix related functions
    static egoboo_rv  update_matrix( ego_chr * pchr, bool_t update_size );
    static ego_chr *  update_hide( ego_chr * pchr );
    static egoboo_rv  update_collision_size( ego_chr * pchr, bool_t update_matrix );
    static bool_t     matrix_valid( ego_chr * pchr );
    static bool_t     get_matrix_cache( ego_chr * pchr, matrix_cache_t * mc_tmp );

    // generic accessors
    static void set_enviro_grid_level( ego_chr * pchr, float level );
    static void set_redshift( ego_chr * pchr, int rs );
    static void set_grnshift( ego_chr * pchr, int gs );
    static void set_blushift( ego_chr * pchr, int bs );
    static void set_sheen( ego_chr * pchr, int sheen );
    static void set_alpha( ego_chr * pchr, int alpha );
    static void set_light( ego_chr * pchr, int light );
    static void set_fly_height( ego_chr * pchr, float height );
    static void set_jump_number_reset( ego_chr * pchr, int number );
    static fvec3_t get_pos( ego_chr * pchr );
    static bool_t  set_pos( ego_chr * pchr, fvec3_base_t pos );
    static float * get_pos_v( ego_chr * pchr );
    static bool_t set_maxaccel( ego_chr * pchr, float new_val );
    static int     get_price( const CHR_REF by_reference ichr );
    static CHR_REF get_lowest_attachment( const CHR_REF by_reference ichr, bool_t non_item );
    static const char * get_name( const CHR_REF by_reference ichr, Uint32 bits );
    static const char * get_dir_name( const CHR_REF by_reference ichr );
    static int get_skill( ego_chr *pchr, IDSZ whichskill );
    static const char * get_gender_possessive( const CHR_REF by_reference ichr, char buffer[], size_t buffer_len );
    static const char * get_gender_name( const CHR_REF by_reference ichr, char buffer[], size_t buffer_len );

    // these accessor functions are too complex to be inlined
    static MAD_REF        get_imad( const CHR_REF by_reference ichr );
    static struct ego_mad * get_pmad( const CHR_REF by_reference ichr );
    static TX_REF         get_icon_ref( const CHR_REF by_reference item );

    // animation stuff
    static Uint32    get_framefx( ego_chr * pchr );
    static void      set_frame( const CHR_REF by_reference character, int action, int frame, int lip );
    static egoboo_rv set_action( ego_chr * pchr, int action, bool_t action_ready, bool_t override_action );
    static egoboo_rv set_anim( ego_chr * pchr, int action, int frame, bool_t action_ready, bool_t override_action );
    static egoboo_rv start_anim( ego_chr * pchr, int action, bool_t action_ready, bool_t override_action );
    static egoboo_rv increment_action( ego_chr * pchr );
    static egoboo_rv increment_frame( ego_chr * pchr );
    static egoboo_rv play_action( ego_chr * pchr, int action, bool_t action_ready );

    // functions related to stored positions
    static bool_t  update_breadcrumb_raw( ego_chr * pchr );
    static bool_t  update_breadcrumb( ego_chr * pchr, bool_t force );
    static breadcrumb_t * get_last_breadcrumb( ego_chr * pchr );

    static bool_t  update_safe_raw( ego_chr * pchr );
    static bool_t  update_safe( ego_chr * pchr, bool_t force );
    static bool_t  get_safe( ego_chr * pchr, fvec3_base_t pos );

    // INLINE functions

    static INLINE PRO_REF  get_ipro( const CHR_REF by_reference ichr );
    static INLINE CAP_REF  get_icap( const CHR_REF by_reference ichr );
    static INLINE EVE_REF  get_ieve( const CHR_REF by_reference ichr );
    static INLINE PIP_REF  get_ipip( const CHR_REF by_reference ichr, int ipip );
    static INLINE TEAM_REF get_iteam( const CHR_REF by_reference ichr );
    static INLINE TEAM_REF get_iteam_base( const CHR_REF by_reference ichr );

    static INLINE ego_pro * get_ppro( const CHR_REF by_reference ichr );
    static INLINE ego_cap * get_pcap( const CHR_REF by_reference ichr );
    static INLINE ego_eve * get_peve( const CHR_REF by_reference ichr );
    static INLINE ego_pip * get_ppip( const CHR_REF by_reference ichr, int ipip );

    static INLINE Mix_Chunk      * get_chunk_ptr( ego_chr * pchr, int index );
    static INLINE Mix_Chunk      * get_chunk( const CHR_REF by_reference ichr, int index );
    static INLINE team_t         * get_pteam( const CHR_REF by_reference ichr );
    static INLINE team_t         * get_pteam_base( const CHR_REF by_reference ichr );
    static INLINE ego_ai_state     * get_pai( const CHR_REF by_reference ichr );
    static INLINE chr_instance_t * get_pinstance( const CHR_REF by_reference ichr );

    static INLINE IDSZ       get_idsz( const CHR_REF by_reference ichr, int type );
    static INLINE latch_2d_t convert_latch_2d( const ego_chr * pchr, const latch_2d_t by_reference src );

    static INLINE void update_size( ego_chr * pchr );
    static INLINE void init_size( ego_chr * pchr, ego_cap * pcap );
    static INLINE void set_size( ego_chr * pchr, float size );
    static INLINE void set_width( ego_chr * pchr, float width );
    static INLINE void set_shadow( ego_chr * pchr, float width );
    static INLINE void set_height( ego_chr * pchr, float height );
    static INLINE void set_fat( ego_chr * pchr, float fat );

    static INLINE bool_t has_idsz( const CHR_REF by_reference ichr, IDSZ idsz );
    static INLINE bool_t is_type_idsz( const CHR_REF by_reference ichr, IDSZ idsz );
    static INLINE bool_t has_vulnie( const CHR_REF by_reference item, const PRO_REF by_reference weapon_profile );

    static INLINE bool_t get_MatUp( ego_chr *pchr, fvec3_t   * pvec );
    static INLINE bool_t get_MatRight( ego_chr *pchr, fvec3_t   * pvec );
    static INLINE bool_t get_MatForward( ego_chr *pchr, fvec3_t   * pvec );
    static INLINE bool_t get_MatTranslate( ego_chr *pchr, fvec3_t   * pvec );
};

//--------------------------------------------------------------------------------------------
// list definitions
//--------------------------------------------------------------------------------------------

DECLARE_STACK_EXTERN( team_t, TeamStack, TEAM_MAX );

#define VALID_TEAM_RANGE( ITEAM ) ( ((ITEAM) >= 0) && ((ITEAM) < TEAM_MAX) )

DECLARE_STACK_EXTERN( ego_cap,  CapStack,  MAX_PROFILE );

#define VALID_CAP_RANGE( ICAP ) ( ((ICAP) >= 0) && ((ICAP) < MAX_CAP) )
#define LOADED_CAP( ICAP )       ( VALID_CAP_RANGE( ICAP ) && CapStack.lst[ICAP].loaded )

// counters for debugging wall collisions
extern int chr_stoppedby_tests;
extern int chr_pressure_tests;

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// Function prototypes

void character_system_begin();
void character_system_end();

void drop_money( const CHR_REF by_reference character, int money );
void call_for_help( const CHR_REF by_reference character );
void give_experience( const CHR_REF by_reference character, int amount, xp_type xptype, bool_t override_invictus );
void give_team_experience( const TEAM_REF by_reference team, int amount, Uint8 xptype );
int  damage_character( const CHR_REF by_reference character, FACING_T direction,
                       IPair damage, Uint8 damagetype, TEAM_REF team,
                       CHR_REF attacker, BIT_FIELD effects, bool_t ignore_invictus );
void kill_character( const CHR_REF by_reference character, const CHR_REF by_reference killer, bool_t ignore_invictus );
bool_t heal_character( const CHR_REF by_reference character, const CHR_REF by_reference healer, int amount, bool_t ignore_invictus );
void spawn_poof( const CHR_REF by_reference character, const PRO_REF by_reference profile );
void spawn_defense_ping( ego_chr *pchr, const CHR_REF by_reference attacker );

void reset_character_alpha( const CHR_REF by_reference character );
void reset_character_accel( const CHR_REF by_reference character );
bool_t detach_character_from_mount( const CHR_REF by_reference character, Uint8 ignorekurse, Uint8 doshop );

bool_t remove_item_from_pack( const CHR_REF by_reference pack_holder, const CHR_REF by_reference item_ref );

void flash_character_height( const CHR_REF by_reference character, Uint8 valuelow, Sint16 low, Uint8 valuehigh, Sint16 high );
void flash_character( const CHR_REF by_reference character, Uint8 value );

void free_one_character_in_game( const CHR_REF by_reference character );
//void make_one_weapon_matrix( const CHR_REF by_reference iweap, const CHR_REF by_reference iholder, bool_t do_phys  );
void make_all_character_matrices( bool_t do_physics );
void free_inventory_in_game( const CHR_REF by_reference character );

void keep_weapons_with_holders();
void make_one_character_matrix( const CHR_REF by_reference cnt );

void update_all_characters( void );
void move_all_characters( void );
void cleanup_all_characters( void );

void increment_all_character_update_counters( void );

void do_level_up( const CHR_REF by_reference character );
bool_t setup_xp_table( const CHR_REF by_reference character );

void free_all_chraracters();

BIT_FIELD chr_hit_wall( ego_chr * pchr, float test_pos[], float nrm[], float * pressure );
bool_t chr_test_wall( ego_chr * pchr, float test_pos[] );

int chr_count_free();

CHR_REF spawn_one_character( fvec3_t   pos, const PRO_REF by_reference profile, const TEAM_REF by_reference team, Uint8 skin, FACING_T facing, const char *name, const CHR_REF by_reference override );
void    respawn_character( const CHR_REF by_reference character );
int     change_armor( const CHR_REF by_reference character, int skin );
void    change_character( const CHR_REF by_reference cnt, const PRO_REF by_reference profile, Uint8 skin, Uint8 leavewhich );
void    change_character_full( const CHR_REF by_reference ichr, const PRO_REF by_reference profile, Uint8 skin, Uint8 leavewhich );
bool_t  cost_mana( const CHR_REF by_reference character, int amount, const CHR_REF by_reference killer );
void    switch_team( const CHR_REF by_reference character, const TEAM_REF by_reference team );
void    issue_clean( const CHR_REF by_reference character );
int     restock_ammo( const CHR_REF by_reference character, IDSZ idsz );
void    attach_character_to_mount( const CHR_REF by_reference character, const CHR_REF by_reference mount, grip_offset_t grip_off );
bool_t  chr_inventory_add_item( const CHR_REF by_reference item, const CHR_REF by_reference character );
CHR_REF chr_inventory_remove_item( const CHR_REF by_reference character, grip_offset_t grip_off, bool_t ignorekurse );
void    drop_all_idsz( const CHR_REF by_reference character, IDSZ idsz_min, IDSZ idsz_max );
bool_t  drop_all_items( const CHR_REF by_reference character );
bool_t  character_grab_stuff( const CHR_REF by_reference chara, grip_offset_t grip, bool_t people );

egoboo_rv  export_one_character_quest_vfs( const char *szSaveName, const CHR_REF by_reference character );
bool_t     export_one_character_name_vfs( const char *szSaveName, const CHR_REF by_reference character );
bool_t     export_one_character_profile_vfs( const char *szSaveName, const CHR_REF by_reference character );
bool_t     export_one_character_skin_vfs( const char *szSaveName, const CHR_REF by_reference character );
CAP_REF    load_one_character_profile_vfs( const char *szLoadName, int slot_override, bool_t required );

void character_swipe( const CHR_REF by_reference cnt, slot_t slot );

bool_t is_invictus_direction( FACING_T direction, const CHR_REF by_reference character, Uint16 effects );

void   init_slot_idsz();

struct ego_billboard_data * chr_make_text_billboard( const CHR_REF by_reference ichr, const char * txt, SDL_Color text_color, GLXvector4f tint, int lifetime_secs, BIT_FIELD opt_bits );

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
struct ego_chr_bundle
{
    CHR_REF   chr_ref;
    ego_chr * chr_ptr;

    PRO_REF   pro_ref;
    ego_pro * pro_ptr;

    CAP_REF   cap_ref;
    ego_cap * cap_ptr;

    static ego_chr_bundle * ctor( ego_chr_bundle * pbundle );
    static ego_chr_bundle * validate( ego_chr_bundle * pbundle );
    static ego_chr_bundle * set( ego_chr_bundle * pbundle, ego_chr * pchr );
};

//--------------------------------------------------------------------------------------------
// helper functions

void character_system_begin();
void character_system_end();

void   init_all_cap();
void   release_all_cap();
bool_t release_one_cap( const CAP_REF by_reference icap );

void reset_teams();

bool_t apply_reflection_matrix( chr_instance_t * pinst, float grid_level );

// generic helper functions
bool_t  chr_teleport( const CHR_REF by_reference ichr, float x, float y, float z, FACING_T facing_z );
CHR_REF chr_has_inventory_idsz( const CHR_REF by_reference ichr, IDSZ idsz, bool_t equipped, CHR_REF * pack_last );
CHR_REF chr_holding_idsz( const CHR_REF by_reference ichr, IDSZ idsz );
CHR_REF chr_has_item_idsz( const CHR_REF by_reference ichr, IDSZ idsz, bool_t equipped, CHR_REF * pack_last );
bool_t  chr_can_see_object( const CHR_REF by_reference ichr, const CHR_REF by_reference iobj );
bool_t  chr_can_mount( const CHR_REF by_reference ichr_a, const CHR_REF by_reference ichr_b );
bool_t  chr_is_attacking( ego_chr *pchr );
bool_t  chr_calc_environment( ego_chr * pchr );
void    chr_instance_get_tint( chr_instance_t * pinst, GLfloat * tint, Uint32 bits );
int     convert_grip_to_local_points( ego_chr * pholder, Uint16 grip_verts[], fvec4_t   dst_point[] );
int     convert_grip_to_global_points( const CHR_REF by_reference iholder, Uint16 grip_verts[], fvec4_t   dst_point[] );

const char * describe_value( float value, float maxval, int * rank_ptr );
const char * describe_damage( float value, float maxval, int * rank_ptr );
const char * describe_wounds( float max, float current );

// physics function
void   character_physics_initialize();
void   character_physics_finalize_all( float dt );
bool_t character_physics_get_mass_pair( ego_chr * pchr_a, ego_chr * pchr_b, float * wta, float * wtb );

bool_t chr_is_over_water( ego_chr *pchr );

float calc_dismount_lerp( const ego_chr * pchr_a, const ego_chr * pchr_b );

bool_t chr_copy_enviro( ego_chr * chr_psrc, ego_chr * chr_pdst );

#define _char_h
