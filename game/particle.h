#pragma once

#include "object.h"

struct CGame_t;

#define TURNSPD                         .01         // Cutoff for turning or same direction

#define SPAWN_NOCHARACTER        255                 // For particles that spawn characters...

#define PIPLST_COUNT           1024                    // Particle templates
#define PRTLST_COUNT              512         // Max number of particles

#define DYNAFANS  12
#define MAXFALLOFF 1400

enum particle_type
{
  PRTTYPE_LIGHT = 0,                         // Magic effect particle
  PRTTYPE_SOLID,                             // Sprite particle
  PRTTYPE_ALPHA,                             // Smoke particle
};
typedef enum particle_type PRTTYPE;

enum dyna_mode_e
{
  DYNA_OFF = 0,
  DYNA_ON,
  DYNA_LOCAL,
};
typedef enum dyna_mode_e DYNA_MODE;

#define MAXDYNA                         8           // Number of dynamic lights
#define MAXDYNADIST                     2700        // Leeway for offscreen lights


//Lightning effects
typedef struct dynalight_info_t
{
  int                     count;               // Number of dynamic lights
  int                     distancetobeat;      // The number to beat

} DYNALIGHT_INFO;

extern DYNALIGHT_INFO GDyna;

#define DYNALIGHT_MEMBERS                \
  float level;   /* Light level    */ \
  float falloff; /* Light falloff  */

struct dynalight_list_t
{
  DYNALIGHT_MEMBERS
  vect3  pos;        // Light position
  int distance;      // The distances
};
typedef struct dynalight_list_t DYNALIGHT_LIST;

extern DYNALIGHT_LIST GDynaLight[MAXDYNA];

struct dynalight_pip_t
{
  DYNALIGHT_MEMBERS

  bool_t    on;                  // Dynamic light?
  DYNA_MODE mode;                // Dynamic light on?
  float     leveladd;            // Dyna light changes
  float     falloffadd;          //
};
typedef struct dynalight_pip_t DYNALIGHT_PIP;

// Particle profiles
struct CPip_t
{
  egoboo_key      ekey;
  bool_t          Loaded;

  bool_t          force;                        // Force spawn?
  STRING          fname;
  STRING          comment;
  PRTTYPE         type;                         // Transparency mode
  Uint8           numframes;                    // Number of frames
  Uint16          imagebase;                    // Starting image
  PAIR            imageadd;                     // Frame rate
  Uint16          time;                         // Time until end
  PAIR            rotate;                       // Rotation
  Uint16          rotateadd;                    // Rotation
  Uint16          sizebase_fp8;                 // Size
  Sint16          sizeadd;                      // Size rate
  float           spdlimit;                     // Speed limit
  float           dampen;                       // Bounciness
  Sint8           bumpmoney;                    // Value of particle
  Uint8           bumpsize;                     // Bounding box size
  Uint8           bumpheight;                   // Bounding box height
  float           bumpstrength;
  bool_t          endwater;                     // End if underwater
  bool_t          endbump;                      // End if bumped
  bool_t          endground;                    // End if on ground
  bool_t          endwall;                      // End if hit a wall
  bool_t          endlastframe;                 // End on last frame
  PAIR            damage_fp8;                   // Damage
  DAMAGE          damagetype;                   // Damage type
  PAIR            facing;                       // Facing
  Uint16          facingadd;                    // Facing rotation
  PAIR            xyspacing;                    // Spacing
  PAIR            zspacing;                     // Altitude
  PAIR            xyvel;                    // Shot velocity
  PAIR            zvel;                     // Up velocity
  Uint16          contspawntime;                // Spawn timer
  Uint8           contspawnamount;              // Spawn amount
  Uint16          contspawnfacingadd;           // Spawn in circle
  PIP_REF         contspawnpip;                 // Spawn type ( local )
  Uint8           endspawnamount;               // Spawn amount
  Uint16          endspawnfacingadd;            // Spawn in circle
  PIP_REF         endspawnpip;                  // Spawn type ( local )
  Uint8           bumpspawnamount;              // Spawn amount
  PIP_REF         bumpspawnpip;                 // Spawn type ( global )
  DYNALIGHT_PIP   dyna;                         // Dynalight info
  Uint16          dazetime;                     // Daze
  Uint16          grogtime;                     // Drunkeness
  Sint8           soundspawn;                   // Beginning sound
  Sint8           soundend;                     // Ending sound
  Sint8           soundfloor;                   // Floor sound
  Sint8           soundwall;                    // Ricochet sound
  bool_t          friendlyfire;                 // Friendly fire
  bool_t          rotatetoface;                 // Arrows/Missiles
  bool_t          causepancake;                 // Cause pancake?
  bool_t          causeknockback;               // Cause knockback?
  bool_t          newtargetonspawn;             // Get new target?
  bool_t          homing;                       // Homing?
  Uint16          targetangle;                  // To find target
  float           homingaccel;                  // Acceleration rate
  float           homingfriction;               // Deceleration rate

  bool_t          targetcaster;                 // Target caster?
  bool_t          spawnenchant;                 // Spawn enchant?
  bool_t          needtarget;                   // Need a target?
  bool_t          onlydamagefriendly;           // Only friends?
  bool_t          hateonly;                     // Only enemies? !!BAD NOT DONE!!
  bool_t          startontarget;                // Start on target?
  int             zaimspd;                      // [ZSPD] For Z aiming
  Uint16          damfx;                        // Damage effects
  bool_t          allowpush;                    //Allow particle to push characters around
  bool_t          intdamagebonus;               //Add intelligence as damage bonus
  bool_t          wisdamagebonus;               //Add wisdom as damage bonus
  float           manadrain;                      //Reduce target mana by this amount
  float           lifedrain;                      //Reduce target mana by this amount
  bool_t          rotatewithattached;           // do attached particles rotate with the object?
};
typedef struct CPip_t CPip;

#ifdef __cplusplus
  typedef TList<CPip_t, PIPLST_COUNT> PipList_t;
  typedef TPList<CPip_t, PIPLST_COUNT> PPip;
#else
  typedef CPip PipList_t[PIPLST_COUNT];
  typedef CPip * PPip;
#endif

CPip * Pip_new(CPip * ppip);
bool_t Pip_delete(CPip * ppip);
CPip * Pip_renew(CPip * ppip);

#define VALID_PIP_RANGE(XX) (((XX)>=0) && ((XX)<PIPLST_COUNT))
#define VALID_PIP(LST, XX)    ( VALID_PIP_RANGE(XX) && EKEY_VALID(LST[XX]) )
#define VALIDATE_PIP(LST, XX) ( VALID_PIP(LST, XX) ? (XX) : (INVALID_PIP) )
#define LOADED_PIP(LST, XX)   ( VALID_PIP(LST, XX) && LST[XX].Loaded )


struct dynalight_prt_t
{
  DYNALIGHT_MEMBERS

  bool_t    on;                  // Dynamic light?
  DYNA_MODE mode;                // Dynamic light on?
};
typedef struct dynalight_prt_t DYNALIGHT_PRT;

struct prt_spawn_info_t
{
  egoboo_key ekey;

  struct CGame_t * gs;
  Uint32 seed;
  OBJ_REF iobj;
  PIP_REF ipip;
  PRT_REF iprt;

  float    intensity;
  vect3    pos;
  vect3    vel;
  Uint16   facing;
  CHR_REF  characterattach;
  Uint32   offset;
  TEAM_REF team;
  CHR_REF  characterorigin;
  Uint16   multispawn;
  CHR_REF oldtarget;
};
typedef struct prt_spawn_info_t prt_spawn_info;

prt_spawn_info * prt_spawn_info_new(prt_spawn_info * psi, struct CGame_t * gs);
bool_t           prt_spawn_info_delete(prt_spawn_info * psi);

struct CPrt_t
{
  egoboo_key      ekey;
  bool_t          reserved;         // Is it going to be used?
  bool_t          req_active;      // Are we going to auto-activate ASAP?
  bool_t          active;          // is it currently on?
  bool_t          gopoof;          // Is poof requested?
  bool_t          freeme;          // Is PrtList_free_one() requested?

  prt_spawn_info  spinfo;

  PIP_REF         pip;                             // The part template
  OBJ_REF         model;                           // CPip spawn model
  CHR_REF         attachedtochr;                   // For torch flame
  Uint16          vertoffset;                      // The vertex it's on (counting backward from max vertex)
  PRTTYPE         type;                            // Transparency mode, 0-2
  Uint16          alpha_fp8;
  Uint16          facing;                          // Direction of the part
  TEAM_REF        team;                            // Team

  CPhysAccum      accum;                       //
  COrientation    ori;
  COrientation    ori_old;

  float           level;                           // Height of tile
  Uint8           spawncharacterstate;             //
  Uint16          rotate;                          // Rotation direction
  Sint16          rotateadd;                       // Rotation rate
  Uint32          onwhichfan;                      // Where the part is
  Uint16          size_fp8;                        // Size of particle
  Sint16          sizeadd_fp8;                     // Change in size
  bool_t          inview;                          // Render this one?
  Uint32          image_fp8;                       // Which image
  Uint32          imageadd_fp8;                    // Animation rate
  Uint32          imagemax_fp8;                    // End of image loop
  Uint32          imagestt_fp8;                    // Start of image loop
  Uint16          lightr_fp8;                      // Light level
  Uint16          lightg_fp8;                      // Light level
  Uint16          lightb_fp8;                      // Light level
  float           time;                            // Duration of particle
  float           spawntime;                       // Time until spawn
  Uint8           bumpsize;                        // Size of bumpers
  Uint8           bumpsizebig;                     //
  Uint8           bumpheight;                      // Bounding box height
  float           bumpstrength;                    // The amount of interaction
  float           weight;                          // The mass of the particle

  BData           bmpdata;                         // particle bump size data
  PAIR            damage;                          // For strength
  DAMAGE          damagetype;                      // Damage type
  CHR_REF         owner;                           // The character that is attacking

  CHR_REF         target;                          // Who it's chasing

  DYNALIGHT_PRT   dyna;
};
typedef struct CPrt_t CPrt;
#ifdef __cplusplus
  typedef TList<CPrt_t, PRTLST_COUNT> PrtList_t;
  typedef TPList<CPrt_t, PRTLST_COUNT> PPrt;
#else
  typedef CPrt PrtList_t[PRTLST_COUNT];
  typedef CPrt * PPrt;
#endif


CPrt * Prt_new   ( CPrt * pprt );
bool_t Prt_delete( CPrt * pprt );
CPrt * Prt_renew ( CPrt * pprt );

INLINE CHR_REF prt_get_owner( struct CGame_t * gs, PRT_REF iprt );
INLINE CHR_REF prt_get_target( struct CGame_t * gs, PRT_REF iprt );
INLINE CHR_REF prt_get_attachedtochr( struct CGame_t * gs, PRT_REF iprt );

void    PrtList_free_one( struct CGame_t * gs, PRT_REF particle );
void    end_one_particle( struct CGame_t * gs, PRT_REF particle );
PRT_REF PrtList_get_free( struct CGame_t * gs, bool_t is_critical );

struct CPrt_t *     PrtList_getPPrt(struct CGame_t * gs, PRT_REF iprt);
struct CProfile_t * PrtList_getPObj(struct CGame_t * gs, PRT_REF iprt);
struct CPip_t *     PrtList_getPPip(struct CGame_t * gs, PRT_REF iprt);
struct CCap_t *     PrtList_getPCap(struct CGame_t * gs, PRT_REF iprt);

OBJ_REF PrtList_getRObj(struct CGame_t * gs, PRT_REF iprt);
PIP_REF PrtList_getRPip(struct CGame_t * gs, PRT_REF iprt);
CAP_REF PrtList_getRCap(struct CGame_t * gs, PRT_REF iprt);
MAD_REF PrtList_getRMad(struct CGame_t * gs, PRT_REF iprt);

extern Uint16          particletexture;                            // All in one bitmap

#define VALID_PRT_RANGE(XX)   (((XX)>=0) && ((XX)<PRTLST_COUNT))
#define VALID_PRT(LST, XX)    ( VALID_PRT_RANGE(XX) && EKEY_VALID(LST[XX]) )
#define VALIDATE_PRT(LST, XX) ( VALID_PRT(LST, XX) ? (XX) : (INVALID_PRT) )
#define RESERVED_PRT(LST, XX) ( VALID_PRT(LST, XX) && LST[XX].reserved && !LST[XX].active   )
#define ACTIVE_PRT(LST, XX)   ( VALID_PRT(LST, XX) && LST[XX].active   && !LST[XX].reserved )
#define PENDING_PRT(LST, XX)  ( VALID_PRT(LST, XX) && (LST[XX].active || LST[XX].req_active) && !LST[XX].reserved )



#define CALCULATE_PRT_U0(CNT)  (((.05f+(CNT&15))/16.0f)*(( float ) gs->TxTexture[particletexture].imgW / ( float ) gs->TxTexture[particletexture].txW))
#define CALCULATE_PRT_U1(CNT)  (((.95f+(CNT&15))/16.0f)*(( float ) gs->TxTexture[particletexture].imgW / ( float ) gs->TxTexture[particletexture].txW))
#define CALCULATE_PRT_V0(CNT)  (((.05f+(CNT/16))/16.0f) * ((float)gs->TxTexture[particletexture].imgW/(float)gs->TxTexture[particletexture].imgH)*(( float ) gs->TxTexture[particletexture].imgH / ( float ) gs->TxTexture[particletexture].txH))
#define CALCULATE_PRT_V1(CNT)  (((.95f+(CNT/16))/16.0f) * ((float)gs->TxTexture[particletexture].imgW/(float)gs->TxTexture[particletexture].imgH)*(( float ) gs->TxTexture[particletexture].imgH / ( float ) gs->TxTexture[particletexture].txH))

void PrtList_resynch( struct CGame_t * gs );
void move_particles( struct CGame_t * gs, float dUpdate );
void attach_particles( struct CGame_t * gs );
PRT_REF prt_spawn_info_init( prt_spawn_info * psi, struct CGame_t * gs, float intensity, vect3 pos,
                           Uint16 facing, OBJ_REF model, PIP_REF pip,
                           CHR_REF characterattach, Uint32 offset, TEAM_REF team,
                           CHR_REF characterorigin, Uint16 multispawn, CHR_REF oldtarget);

PRT_REF req_spawn_one_particle( prt_spawn_info si );

Uint32 prt_hitawall( struct CGame_t * gs, PRT_REF particle, vect3 * norm );



PIP_REF PipList_load_one( struct CGame_t * gs, const char * szModpath, const char * szObjectname, const char * szFname, PIP_REF override );

bool_t prt_calculate_bumpers(struct CGame_t * gs, PRT_REF iprt);

bool_t prt_is_over_water( struct CGame_t * gs, PRT_REF cnt );
void reset_particles( struct CGame_t * gs, char* modname );

void PipList_load_global( struct CGame_t * gs );