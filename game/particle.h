#pragma once

#include "object.h"

struct sGame;

#define TURNSPD                         .01         // Cutoff for turning or same direction

#define SPAWN_NOCHARACTER        255                 // For particles that spawn characters...

#define PIPLST_COUNT           1024                    // Particle templates
#define PRTLST_COUNT              512         // Max number of particles

#define DYNAFANS  12
#define MAXFALLOFF 1400

enum e_particle_alpha_type
{
  PRTTYPE_LIGHT = 0,                         // Magic effect particle
  PRTTYPE_SOLID,                             // Sprite particle
  PRTTYPE_ALPHA,                             // Smoke particle
};
typedef enum e_particle_alpha_type PRTALPHA;

enum e_dyna_mode
{
  DYNA_OFF = 0,
  DYNA_ON,
  DYNA_LOCAL,
};
typedef enum e_dyna_mode DYNA_MODE;

#define MAXDYNA                           64        // Number of dynamic lights
#define MAXDYNADIST                     2700        // Leeway for offscreen lights


//Dynamic Lightning effects

#define DYNALIGHT_MEMBERS                \
  float  level;      /* Light level    */ \
  float  falloff;    /* Light falloff  */

struct s_dynalight_info
{
  DYNALIGHT_MEMBERS
  bool_t permanent;
  vect3  pos;        // Light position
  int    distance;      // The distances
};
typedef struct s_dynalight_info DYNALIGHT_INFO;

#ifdef __cplusplus
  typedef TList<DYNALIGHT_INFO, MAXDYNA> DLightList_t;
  typedef TPList<DYNALIGHT_INFO, MAXDYNA> PDLight;
#else
  typedef DYNALIGHT_INFO   DLightList_t[MAXDYNA];
  typedef DYNALIGHT_INFO * PDLight;
#endif

size_t DLightList_clear( struct sGame * gs );
size_t DLightList_prune( struct sGame * gs );
size_t DLightList_add( struct sGame * gs, DYNALIGHT_INFO * di );

struct s_dynalight_pip
{
  DYNALIGHT_MEMBERS

  bool_t    on;                  // Dynamic light?
  DYNA_MODE mode;                // Dynamic light on?
  float     leveladd;            // Dyna light changes
  float     falloffadd;          //
};
typedef struct s_dynalight_pip DYNALIGHT_PIP;

// Particle profiles
struct sPip
{
  egoboo_key_t      ekey;
  bool_t          Loaded;

  bool_t          force;                        // Force spawn?
  STRING          fname;
  STRING          comment;
  PRTALPHA         type;                         // Transparency mode
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
typedef struct sPip Pip_t;

#ifdef __cplusplus
  typedef TList<sPip, PIPLST_COUNT> PipList_t;
  typedef TPList<sPip, PIPLST_COUNT> PPip_t;
#else
  typedef Pip_t PipList_t[PIPLST_COUNT];
  typedef Pip_t * PPip_t;
#endif

Pip_t * Pip_new(Pip_t * ppip);
bool_t Pip_delete(Pip_t * ppip);
Pip_t * Pip_renew(Pip_t * ppip);

#define VALID_PIP_RANGE(XX) (((XX)>=0) && ((XX)<PIPLST_COUNT))
#define VALID_PIP(LST, XX)    ( VALID_PIP_RANGE(XX) && EKEY_VALID(LST[XX]) )
#define VALIDATE_PIP(LST, XX) ( VALID_PIP(LST, XX) ? (XX) : (INVALID_PIP) )
#define LOADED_PIP(LST, XX)   ( VALID_PIP(LST, XX) && LST[XX].Loaded )


struct s_dynalight_prt
{
  DYNALIGHT_MEMBERS

  bool_t    on;                  // Dynamic light?
  DYNA_MODE mode;                // Dynamic light on?
};
typedef struct s_dynalight_prt DYNALIGHT_PRT;

struct s_prt_spawn_info
{
  egoboo_key_t ekey;

  struct sGame * gs;
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
typedef struct s_prt_spawn_info PRT_SPAWN_INFO;

PRT_SPAWN_INFO * prt_spawn_info_new(PRT_SPAWN_INFO * psi, struct sGame * gs);
bool_t           prt_spawn_info_delete(PRT_SPAWN_INFO * psi);

struct sPrt
{
  egoboo_key_t      ekey;
  bool_t          reserved;         // Is it going to be used?
  bool_t          req_active;      // Are we going to auto-activate ASAP?
  bool_t          active;          // is it currently on?
  bool_t          gopoof;          // Is poof requested?
  bool_t          freeme;          // Is PrtList_free_one() requested?

  PRT_SPAWN_INFO  spinfo;

  PIP_REF         pip;                             // The part template
  OBJ_REF         model;                           // Pip_t spawn model
  CHR_REF         attachedtochr;                   // For torch flame
  Uint16          vertoffset;                      // The vertex it's on (counting backward from max vertex)
  PRTALPHA         type;                            // Transparency mode, 0-2
  Uint16          alpha_fp8;
  Uint16          facing;                          // Direction of the part
  TEAM_REF        team;                            // Team

  PhysAccum_t      accum;                       //
  Orientation_t    ori;
  Orientation_t    ori_old;

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

  BData_t           bmpdata;                         // particle bump size data
  PAIR            damage;                          // For strength
  DAMAGE          damagetype;                      // Damage type
  CHR_REF         owner;                           // The character that is attacking

  CHR_REF         target;                          // Who it's chasing

  DYNALIGHT_PRT   dyna;
};
typedef struct sPrt Prt_t;

#ifdef __cplusplus
  typedef TList<sPrt, PRTLST_COUNT> PrtList_t;
  typedef TPList<sPrt, PRTLST_COUNT> PPrt_t;
#else
  typedef Prt_t PrtList_t[PRTLST_COUNT];
  typedef Prt_t * PPrt_t;
#endif


Prt_t * Prt_new   ( Prt_t * pprt );
bool_t Prt_delete( Prt_t * pprt );
Prt_t * Prt_renew ( Prt_t * pprt );

INLINE CHR_REF prt_get_owner( struct sGame * gs, PRT_REF iprt );
INLINE CHR_REF prt_get_target( struct sGame * gs, PRT_REF iprt );
INLINE CHR_REF prt_get_attachedtochr( struct sGame * gs, PRT_REF iprt );

void    PrtList_free_one( struct sGame * gs, PRT_REF particle );
void    end_one_particle( struct sGame * gs, PRT_REF particle );
PRT_REF PrtList_get_free( struct sGame * gs, bool_t is_critical );

struct sPrt *     PrtList_getPPrt(struct sGame * gs, PRT_REF iprt);
struct sProfile * PrtList_getPObj(struct sGame * gs, PRT_REF iprt);
struct sPip *     PrtList_getPPip(struct sGame * gs, PRT_REF iprt);
struct sCap *     PrtList_getPCap(struct sGame * gs, PRT_REF iprt);

OBJ_REF PrtList_getRObj(struct sGame * gs, PRT_REF iprt);
PIP_REF PrtList_getRPip(struct sGame * gs, PRT_REF iprt);
CAP_REF PrtList_getRCap(struct sGame * gs, PRT_REF iprt);
MAD_REF PrtList_getRMad(struct sGame * gs, PRT_REF iprt);

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

void PrtList_resynch( struct sGame * gs );
void move_particles( struct sGame * gs, float dUpdate );
void attach_particles( struct sGame * gs );
PRT_REF prt_spawn_info_init( PRT_SPAWN_INFO * psi, struct sGame * gs, float intensity, vect3 pos,
                           Uint16 facing, OBJ_REF model, PIP_REF pip,
                           CHR_REF characterattach, Uint32 offset, TEAM_REF team,
                           CHR_REF characterorigin, Uint16 multispawn, CHR_REF oldtarget);

PRT_REF req_spawn_one_particle( PRT_SPAWN_INFO si );

Uint32 prt_hitawall( struct sGame * gs, PRT_REF particle, vect3 * norm );



PIP_REF PipList_load_one( struct sGame * gs, const char * szModpath, const char * szObjectname, const char * szFname, PIP_REF override );

bool_t prt_calculate_bumpers(struct sGame * gs, PRT_REF iprt);

bool_t prt_is_over_water( struct sGame * gs, PRT_REF cnt );
void reset_particles( struct sGame * gs, char* modname );

void PipList_load_global( struct sGame * gs );