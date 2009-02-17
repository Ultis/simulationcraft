// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

// Platform Initialization ==================================================

#if defined( _MSC_VER ) || defined( __MINGW__ ) || defined( _WINDOWS ) || defined( WIN32 )
#  define WIN32_LEAN_AND_MEAN
#  define VC_EXTRALEAN
#  define _CRT_SECURE_NO_WARNINGS
#endif

#if defined( _MSC_VER )
#include "./vs/stdint.h"
#else
#include <stdint.h>
#endif

#include <stdarg.h>
#include <float.h>
#include <time.h>
#include <string>
#include <queue>
#include <vector>
#include <map>
#include <algorithm>
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Patch Specific Modeling ==================================================

struct patch_t
{
   uint64_t mask;
   uint64_t encode( int arch, int version, int revision ) { return arch*10000 + version*100 + revision; }
   patch_t        ( int arch, int version, int revision ) {         mask = encode( arch, version, revision ); }
   void set       ( int arch, int version, int revision ) {         mask = encode( arch, version, revision ); }
   bool before    ( int arch, int version, int revision ) { return mask <  encode( arch, version, revision ); }
   bool after     ( int arch, int version, int revision ) { return mask >= encode( arch, version, revision ); }
   patch_t() { mask = encode( 0, 0, 0 ); }
};

// Forward Declarations ======================================================

struct action_t;
struct attack_t;
struct base_stats_t;
struct callback_t;
struct druid_t;
struct event_t;
struct gain_t;
struct hunter_t;
struct mage_t;
struct option_t;
struct pet_t;
struct player_t;
struct priest_t;
struct proc_t;
struct rank_t;
struct rating_t;
struct report_t;
struct rng_t;
struct rogue_t;
struct scaling_t;
struct shaman_t;
struct sim_t;
struct spell_t;
struct stats_t;
struct talent_translation_t;
struct target_t;
struct uptime_t;
struct warlock_t;
struct warrior_t;
struct weapon_t;

// Random Number Generation ==================================================

struct rng_t
{
  static rng_t* init( int sfmt );
  virtual int roll( double chance );
  virtual double real();
  virtual ~rng_t() {}
};

// Event =====================================================================

struct event_t
{
  event_t*  next;
  sim_t*    sim;
  player_t* player;
  uint32_t  id;
  double    time;
  double    reschedule_time;
  int       canceled;
  const char* name;
  event_t( sim_t* s, player_t* p=0, const char* n=0 ) :
    next(0), sim(s), player(p), reschedule_time(0), canceled(0), name(n)
  {
    if( ! name ) name = "unknown";
  }
  void reschedule( double new_time );
  double occurs() { return reschedule_time != 0 ? reschedule_time : time; }
  virtual void execute() { printf( "%s\n", name ? name : "(no name)" ); assert(0); }
  virtual ~event_t() {}
  static void cancel( event_t*& e ) { if( e ) { e -> canceled = 1;                 e=0; } }
  static void  early( event_t*& e ) { if( e ) { e -> canceled = 1; e -> execute(); e=0; } }
  // Simple free-list memory manager.
  static void* operator new( size_t, sim_t* );
  static void* operator new( size_t ) throw();  // DO NOT USE!
  static void  operator delete( void* );
  static void  operator delete( void*, sim_t* ) {}
  static void deallocate( event_t* e );
};

struct event_compare_t
{
  bool operator () (event_t* lhs, event_t* rhs ) const
  {
    return( lhs -> time == rhs -> time ) ? ( lhs -> id > rhs -> id ) : ( lhs -> time > rhs -> time );
  }
};

// Simulation Engine =========================================================

enum race_type { RACE_NONE=0, RACE_BEAST, RACE_DRAGONKIN, RACE_GIANT, RACE_HUMANOID, RACE_MAX };

enum player_type {
  PLAYER_NONE=0,
  DEATH_KNIGHT, DRUID, HUNTER, MAGE, PALADIN, PRIEST, ROGUE, SHAMAN, WARLOCK, WARRIOR,
  PLAYER_PET, PLAYER_GUARDIAN,
  PLAYER_MAX
};

enum dmg_type { DMG_DIRECT=0, DMG_OVER_TIME=1 };

enum attribute_type { ATTRIBUTE_NONE=0, ATTR_STRENGTH, ATTR_AGILITY, ATTR_STAMINA, ATTR_INTELLECT, ATTR_SPIRIT, ATTRIBUTE_MAX };

enum resource_type {
  RESOURCE_NONE=0,
  RESOURCE_HEALTH, RESOURCE_MANA,  RESOURCE_RAGE, RESOURCE_ENERGY, RESOURCE_FOCUS, RESOURCE_RUNIC,
  RESOURCE_MAX };

enum result_type {
  RESULT_NONE=0,
  RESULT_MISS,   RESULT_RESIST, RESULT_DODGE, RESULT_PARRY, RESULT_BLOCK,
  RESULT_GLANCE, RESULT_CRUSH,  RESULT_CRIT,  RESULT_HIT,   RESULT_TICK,
  RESULT_MAX
};

enum action_type { ACTION_USE=0, ACTION_SPELL, ACTION_ATTACK, ACTION_OTHER, ACTION_MAX };

enum school_type {
  SCHOOL_NONE=0,
  SCHOOL_ARCANE,    SCHOOL_BLEED,  SCHOOL_CHAOS,  SCHOOL_FIRE,     SCHOOL_FROST,
  SCHOOL_FROSTFIRE, SCHOOL_HOLY,   SCHOOL_NATURE, SCHOOL_PHYSICAL, SCHOOL_SHADOW,
  SCHOOL_MAX
};

enum talent_tree_type {
  TREE_NONE=0,
  TREE_BALANCE,       TREE_FERAL,        TREE_RESTORATION, // DRUID
  TREE_BEAST_MASTERY, TREE_MARKSMANSHIP, TREE_SURVIVAL,    // HUNTER
  TREE_ARCANE,        TREE_FIRE,         TREE_FROST,       // MAGE
  TREE_DISCIPLINE,    TREE_HOLY,         TREE_SHADOW,      // PRIEST
  TREE_ASSASSINATION, TREE_COMBAT,       TREE_SUBTLETY,    // ROGUE
  TREE_ELEMENTAL,     TREE_ENHANCEMENT,                    // SHAMAN
  TREE_AFFLICTION,    TREE_DEMONOLOGY,   TREE_DESTRUCTION, // WARLOCK
  TREE_ARMS,          TREE_PROTECTION,   TREE_FURY,        // WARRIOR
  TALENT_TREE_MAX
};

enum weapon_type {
  WEAPON_NONE=0,
  WEAPON_DAGGER,   WEAPON_FIST,                                                                    WEAPON_SMALL,
  WEAPON_BEAST,    WEAPON_SWORD,    WEAPON_MACE,     WEAPON_AXE,                                   WEAPON_1H,
  WEAPON_BEAST_2H, WEAPON_SWORD_2H, WEAPON_MACE_2H,  WEAPON_AXE_2H, WEAPON_STAFF,  WEAPON_POLEARM, WEAPON_2H,
  WEAPON_BOW,      WEAPON_CROSSBOW, WEAPON_GUN,      WEAPON_WAND,   WEAPON_THROWN,                 WEAPON_RANGED,
  WEAPON_MAX
};

enum weapon_enchant_type { WEAPON_ENCHANT_NONE=0, BERSERKING, MONGOOSE, EXECUTIONER, DEATH_FROST, SCOPE, WEAPON_ENCHANT_MAX };

enum weapon_buff_type {
  WEAPON_BUFF_NONE=0,
  ANESTHETIC_POISON, DEADLY_POISON, FIRE_STONE, FLAMETONGUE, INSTANT_POISON,
  SHARPENING_STONE,  SPELL_STONE,   WINDFURY,   WIZARD_OIL,  WOUND_POISON,
  WEAPON_BUFF_MAX
};

enum slot_type { SLOT_NONE=0, SLOT_MAIN_HAND, SLOT_OFF_HAND, SLOT_RANGED, SLOT_MAX };

enum elixir_type {
  ELIXIR_NONE=0,
  ELIXIR_DRAENIC_WISDOM,
  ELIXIR_MAJOR_MAGEBLOOD,
  ELIXIR_GUARDIAN,
  ELIXIR_ADEPT,
  ELIXIR_FEL_STRENGTH,
  ELIXIR_GREATER_ARCANE,
  ELIXIR_MAJOR_AGILITY,
  ELIXIR_MAJOR_FIRE_POWER,
  ELIXIR_MAJOR_FROST_POWER,
  ELIXIR_MAJOR_SHADOW_POWER,
  ELIXIR_MAJOR_STRENGTH,
  ELIXIR_MASTERY,
  ELIXIR_MONGOOSE,
  ELIXIR_ONSLAUGHT,
  ELIXIR_SAGES,
  ELIXIR_BATTLE,
  ELIXIR_MAX
};

enum flask_type {
  FLASK_NONE=0,
  FLASK_BLINDING_LIGHT,
  FLASK_DISTILLED_WISDOM,
  FLASK_ENDLESS_RAGE,
  FLASK_FROST_WYRM,
  FLASK_MIGHTY_RESTORATION,
  FLASK_PURE_DEATH,
  FLASK_RELENTLESS_ASSAULT,
  FLASK_SUPREME_POWER,
  FLASK_MAX
};

enum food_type {
  FOOD_NONE=0,
  FOOD_TENDER_SHOVELTUSK_STEAK,
  FOOD_SMOKED_SALMON,
  FOOD_SNAPPER_EXTREME,
  FOOD_POACHED_BLUEFISH,
  FOOD_BLACKENED_BASILISK,
  FOOD_GOLDEN_FISHSTICKS,
  FOOD_CRUNCHY_SERPENT,
  FOOD_BLACKENED_DRAGONFIN,
  FOOD_GREAT_FEAST,
  FOOD_FISH_FEAST,
  FOOD_MAX
};

enum position_type { POSITION_NONE=0, POSITION_FRONT, POSITION_BACK, POSITION_RANGED, POSITION_MAX };

enum encoding_type { ENCODING_NONE=0, ENCODING_BLIZZARD, ENCODING_MMO, ENCODING_WOWHEAD, ENCODING_MAX };

struct thread_t
{
  static void launch( sim_t* );
  static void wait( sim_t* );
};

struct sim_t
{
  int         argc;
  char**      argv;
  sim_t*      parent;
  std::string patch_str;
  patch_t     patch;
  rng_t*      rng;
  event_t*    free_list;
  player_t*   player_list;
  player_t*   active_player;
  target_t*   target;
  double      lag, pet_lag, channel_penalty, gcd_penalty, reaction_time, regen_periodicity;
  double      current_time, max_time;
  int         events_remaining, max_events_remaining;
  int         events_processed, total_events_processed;
  int         seed, id, iterations, current_iteration, threads;
  int         infinite_resource[ RESOURCE_MAX ];
  int         optimal_raid, potion_sickness, average_dmg, log, debug, timestamp, sfmt;

  std::vector<std::string> party_encoding;

  // Timing Wheel Event Management
  std::vector<event_t*> timing_wheel;
  int    wheel_seconds, wheel_size, wheel_mask, timing_slice;
  double wheel_granularity;

  // Default Gear Profile
  struct gear_default_t
  {
    int attribute[ ATTRIBUTE_MAX ];
    int spell_power;
    int attack_power;
    int expertise_rating;
    int armor_penetration_rating;
    int hit_rating;
    int crit_rating;
    int haste_rating;

    gear_default_t() { memset( (void*) this, 0x00, sizeof( gear_default_t ) ); }
  };
  gear_default_t gear_default;

  // Delta Gear Profile
  struct gear_delta_t
  {
    int attribute[ ATTRIBUTE_MAX ];
    int spell_power;
    int attack_power;
    int expertise_rating;
    int armor_penetration_rating;
    int hit_rating;
    int crit_rating;
    int haste_rating;

    gear_delta_t() { memset( (void*) this, 0x00, sizeof( gear_delta_t ) ); }
  };
  gear_delta_t gear_delta;

  // Static Buffs (until appropriate player class implemented)
  struct buff_t
  {
    int battle_shout;
    int blessing_of_kings;
    int blessing_of_might;
    int blessing_of_wisdom;
    int leader_of_the_pack;
    int sanctified_retribution;
    int swift_retribution;
    buff_t() { memset( (void*) this, 0x0, sizeof( buff_t ) ); }
  };
  buff_t buffs;

  // Reporting
  report_t*  report;
  scaling_t* scaling;
  double     raid_dps, total_dmg, total_seconds, elapsed_cpu_seconds;
  int        merge_ignite;
  std::vector<player_t*> players_by_rank;
  std::vector<player_t*> players_by_name;
  std::string html_file_str, wiki_file_str;
  FILE* output_file;
  FILE* html_file;
  FILE* wiki_file;

  // Multi-Threading
  std::vector<sim_t*> children;
  void* thread_handle;

  sim_t( sim_t* parent=0 );
 ~sim_t();

  void      combat( int iteration );
  void      combat_begin();
  void      combat_end();
  void      add_event( event_t*, double delta_time );
  void      reschedule_event( event_t* );
  void      flush_events();
  void      cancel_events( player_t* );
  event_t*  next_event();
  void      reset();
  bool      init();
  void      analyze();
  void      merge( sim_t& other_sim );
  void      merge();
  void      iterate();
  void      partition();
  void      execute();
  bool      parse_option( const std::string& name, const std::string& value );
  void      print_options();
  bool      time_to_think( double proc_time ) { if( proc_time == 0 ) return false; return current_time - proc_time > reaction_time; }
  bool      cooldown_ready( double cooldown_time ) { return cooldown_time <= current_time; }
  int       roll( double chance ) { return rng -> roll( chance ); }
  player_t* find_player( const std::string& name );
};

// Scaling ===================================================================

struct scaling_t
{
  sim_t* sim;
  int calculate_scale_factors;

  // Gear delta for determining scale factors

  struct gear_t
  {
    int attribute[ ATTRIBUTE_MAX ];
    int spell_power;
    int attack_power;
    int expertise_rating;
    int armor_penetration_rating;
    int hit_rating;
    int crit_rating;
    int haste_rating;

    gear_t() { memset( (void*) this, 0x00, sizeof( gear_t ) ); }
  };
  gear_t gear;

  scaling_t( sim_t* s );

  void analyze();
  void analyze_attributes();
  void analyze_spell_power();
  void analyze_attack_power();
  void analyze_expertise();
  void analyze_armor_penetration();
  void analyze_hit();
  void analyze_crit();
  void analyze_haste();
  bool parse_option( const std::string& name, const std::string& value );
};

// Gear Rating Conversions ===================================================

struct rating_t
{
  double haste;
  double spell_hit, spell_crit;
  double attack_hit, attack_crit;
  double expertise, armor_penetration;
  rating_t() { memset( this, 0x00, sizeof(rating_t) ); }
  void init( int level );
  static double interpolate( int level, double val_60, double val_70, double val_80 );
};

// Weapon ====================================================================

struct weapon_t
{
  int    type, school;
  double damage;
  double swing_time;
  int    enchant, buff;
  double enchant_bonus, buff_bonus;
  int    slot;

  int    group();
  double normalized_weapon_speed();
  double proc_chance_on_swing( double PPM, double adjusted_swing_time=0 );

  weapon_t( int t=WEAPON_NONE, double d=0, double st=2.0, int s=SCHOOL_PHYSICAL ) :
    type(t), school(s), damage(d), swing_time(st),
    enchant(WEAPON_ENCHANT_NONE), buff(WEAPON_BUFF_NONE),
    enchant_bonus(0), buff_bonus(0), slot(SLOT_NONE) {}
};

// Player ====================================================================

struct player_t
{
  sim_t*      sim;
  std::string name_str, talents_str;
  player_t*   next;
  int         type, level, party;
  double      gcd_ready, base_gcd;
  int         sleeping;
  rating_t    rating;
  pet_t*      pet_list;

  // Haste
  int    base_haste_rating, initial_haste_rating, haste_rating;
  double haste;

  // Attributes
  double attribute                   [ ATTRIBUTE_MAX ];
  double attribute_base              [ ATTRIBUTE_MAX ];
  double attribute_initial           [ ATTRIBUTE_MAX ];
  double attribute_multiplier        [ ATTRIBUTE_MAX ];
  double attribute_multiplier_initial[ ATTRIBUTE_MAX ];

  // Spell Mechanics
  double base_spell_power,       initial_spell_power[ SCHOOL_MAX+1 ], spell_power[ SCHOOL_MAX+1 ];
  double base_spell_hit,         initial_spell_hit,                   spell_hit;
  double base_spell_crit,        initial_spell_crit,                  spell_crit;
  double base_spell_penetration, initial_spell_penetration,           spell_penetration;
  double base_mp5,               initial_mp5,                         mp5;
  double spell_power_multiplier,    initial_spell_power_multiplier;
  double spell_power_per_intellect, initial_spell_power_per_intellect;
  double spell_power_per_spirit,    initial_spell_power_per_spirit;
  double spell_crit_per_intellect,  initial_spell_crit_per_intellect;
  double mp5_per_intellect;
  double spirit_regen_while_casting;
  double energy_regen_per_second;
  double focus_regen_per_second;
  double last_cast;

  // Attack Mechanics
  double base_attack_power,       initial_attack_power,        attack_power;
  double base_attack_hit,         initial_attack_hit,          attack_hit;
  double base_attack_expertise,   initial_attack_expertise,    attack_expertise;
  double base_attack_crit,        initial_attack_crit,         attack_crit;
  double base_attack_penetration, initial_attack_penetration,  attack_penetration;
  double attack_power_multiplier,   initial_attack_power_multiplier;
  double attack_power_per_strength, initial_attack_power_per_strength;
  double attack_power_per_agility,  initial_attack_power_per_agility;
  double attack_crit_per_agility,   initial_attack_crit_per_agility;
  int    position;

  // Weapons
  std::string main_hand_str,    off_hand_str,    ranged_str;
  weapon_t    main_hand_weapon, off_hand_weapon, ranged_weapon;

  // Resources
  double  resource_base   [ RESOURCE_MAX ];
  double  resource_initial[ RESOURCE_MAX ];
  double  resource_max    [ RESOURCE_MAX ];
  double  resource_current[ RESOURCE_MAX ];
  double  mana_per_intellect;
  double  health_per_stamina;

  // Consumables
  std::string flask_str, elixirs_str, food_str;
  int elixir_guardian;
  int elixir_battle;
  int flask;
  int food;

  // Events
  event_t* executing;
  event_t* channeling;
  bool     in_combat;

  // Action Priority List
  action_t*   action_list;
  std::string action_list_prefix;
  std::string action_list_str;
  std::string action_list_postfix;
  std::string action_list_skip;

  // Reporting
  int    quiet;
  action_t* last_foreground_action;
  double    last_action_time, total_seconds;
  double    total_waiting;
  double    iteration_dmg, total_dmg;
  double    resource_lost  [ RESOURCE_MAX ];
  double    resource_gained[ RESOURCE_MAX ];
  double    dps, dps_min, dps_max, dps_std_dev, dps_error;
  double    dpr, rps_gain, rps_loss;
  proc_t*   proc_list;
  gain_t*   gain_list;
  stats_t*  stats_list;
  uptime_t* uptime_list;
  std::vector<double> timeline_resource;
  std::vector<double> timeline_dmg;
  std::vector<double> timeline_dps;
  std::vector<double> iteration_dps;
  std::vector<int> distribution_dps;

  struct gear_t
  {
    // Attributes
    int attribute        [ ATTRIBUTE_MAX ];
    int attribute_enchant[ ATTRIBUTE_MAX ];
    // Spell Gear
    int spell_power[ SCHOOL_MAX+1 ], spell_power_enchant[ SCHOOL_MAX+1 ];
    int spell_penetration,           spell_penetration_enchant;
    int mp5, mp5_enchant;
    // Attack Gear
    int attack_power,             attack_power_enchant;
    int armor_penetration_rating, armor_penetration_rating_enchant;
    int expertise_rating,         expertise_rating_enchant;
    // Common Gear
    int hit_rating, hit_rating_enchant;
    int crit_rating, crit_rating_enchant;
    int haste_rating, haste_rating_enchant;
    // Resource Gear
    int resource        [ RESOURCE_MAX ];
    int resource_enchant[ RESOURCE_MAX ];
    // Budgeting
    int spell_power_budget;
    int attack_power_budget;
    int  budget_slots;
    // Unique Gear
    int  ashtongue_talisman;
    int  chaotic_skyflare;
    int  darkmoon_crusade;
    int  darkmoon_greatness;
    int  darkmoon_wrath;
    int  dying_curse;
    int  egg_of_mortal_essence;
    int  elder_scribes;
    int  ember_skyflare;
    int  embrace_of_the_spider;
    int  eternal_sage;
    int  extract_of_necromatic_power;
    int  eye_of_magtheridon;
    int  forge_ember;
    int  fury_of_the_five_flights;
    int  illustration_of_the_dragon_soul;
    int  lightning_capacitor;
    int  mark_of_defiance;
    int  mirror_of_truth;
    int  mystical_skyfire;
    int  quagmirrans_eye;
    int  relentless_earthstorm;
    int  sextant_of_unstable_currents;
    int  shiffars_nexus_horn;
    int  spellstrike;
    int  spellsurge;
    int  sundial_of_the_exiled;
    int  talisman_of_ascendance;
    int  thunder_capacitor;
    int  timbals_crystal;
    int  wrath_of_cenarius;
    int  zandalarian_hero_charm;
    int  tier4_2pc, tier4_4pc;
    int  tier5_2pc, tier5_4pc;
    int  tier6_2pc, tier6_4pc;
    int  tier7_2pc, tier7_4pc;
    gear_t() { memset( (void*) this, 0x00, sizeof( gear_t ) ); }

    void allocate_spell_power_budget( sim_t* );
    void allocate_attack_power_budget( sim_t* );
  };
  gear_t gear;

  struct buff_t
  {
    // Dynamic Buffs
    double    arcane_brilliance;
    int       berserking_mh;
    int       berserking_oh;
    double    divine_spirit;
    int       bloodlust;
    double    cast_time_reduction;
    int       darkmoon_crusade;
    int       darkmoon_wrath;
    double    demonic_pact;
    pet_t*    demonic_pact_pet;
    int       elemental_oath;
    int       executioner;
    double    flametongue_totem;
    player_t* focus_magic;
    int       focus_magic_feedback;
    double    fortitude;
    int       fury_of_the_five_flights;
    int       illustration_of_the_dragon_soul;
    double    improved_divine_spirit;
    int       improved_moonkin_aura;
    int       innervate;
    int       glyph_of_innervate;
    int       leader_of_the_pack;
    int       lightning_capacitor;
    double    mana_cost_reduction;
    double    mark_of_the_wild;
    int       moonkin_aura;
    int       mongoose_mh;
    int       mongoose_oh;
    int       power_infusion;
    int       replenishment;
    int       shadow_form;
    double    strength_of_earth;
    int       talisman_of_ascendance;
    int       thunder_capacitor;
    double    totem_of_wrath;
    int       tricks_of_the_trade;
    int       trueshot_aura;
    int       unleashed_rage;
    int       violet_eye;
    double    windfury_totem;
    int       water_elemental;
    int       wrath_of_air;
    int       zandalarian_hero_charm;
    int       tier4_2pc, tier4_4pc;
    int       tier5_2pc, tier5_4pc;
    int       tier6_2pc, tier6_4pc;
    int       tier7_2pc, tier7_4pc;
    void reset() { memset( (void*) this, 0x0, sizeof( buff_t ) ); }
    buff_t() { reset(); }
  };
  buff_t buffs;

  struct expirations_t
  {
    double spellsurge;
    event_t* ashtongue_talisman;
    event_t* berserking_mh;
    event_t* berserking_oh;
    event_t* darkmoon_crusade;
    event_t* darkmoon_wrath;
    event_t* executioner;
    event_t* eye_of_magtheridon;
    event_t* focus_magic_feedback;
    event_t* mongoose_mh;
    event_t* mongoose_oh;
    event_t* spellstrike;
    event_t* tricks_of_the_trade;
    event_t* wrath_of_cenarius;
    event_t* fury_of_the_five_flights;
    event_t* illustration_of_the_dragon_soul;
    event_t *tier4_2pc, *tier4_4pc;
    event_t *tier5_2pc, *tier5_4pc;
    event_t *tier6_2pc, *tier6_4pc;
    event_t *tier7_2pc, *tier7_4pc;
    void reset() { memset( (void*) this, 0x00, sizeof( expirations_t ) ); }
    expirations_t() { reset(); }
  };
  expirations_t expirations;

  struct cooldowns_t
  {
    double bloodlust;
    double darkmoon_greatness;
    double elder_scribes;
    double eternal_sage;
    double honor_among_thieves;
    double mark_of_defiance;
    double mirror_of_truth;
    double mystical_skyfire;
    double quagmirrans_eye;
    double sextant_of_unstable_currents;
    double shiffars_nexus_horn;
    double spellsurge;
    double sundial_of_the_exiled;
    double egg_of_mortal_essence;
    double dying_curse;
    double embrace_of_the_spider;
    double forge_ember;
    double tier4_2pc, tier4_4pc;
    double tier5_2pc, tier5_4pc;
    double tier6_2pc, tier6_4pc;
    double tier7_2pc, tier7_4pc;
    void reset() { memset( (void*) this, 0x00, sizeof( cooldowns_t ) ); }
    cooldowns_t() { reset(); }
  };
  cooldowns_t cooldowns;

  struct uptimes_t
  {
    uptime_t* berserking_mh;
    uptime_t* berserking_oh;
    uptime_t* executioner;
    uptime_t* mongoose_mh;
    uptime_t* mongoose_oh;
    uptime_t* replenishment;
    uptime_t *tier4_2pc, *tier4_4pc;
    uptime_t *tier5_2pc, *tier5_4pc;
    uptime_t *tier6_2pc, *tier6_4pc;
    uptime_t *tier7_2pc, *tier7_4pc;
    void reset() { memset( (void*) this, 0x00, sizeof( uptimes_t ) ); }
    uptimes_t() { reset(); }
  };
  uptimes_t uptimes;

  struct gains_t
  {
    gain_t* ashtongue_talisman;
    gain_t* blessing_of_wisdom;
    gain_t* dark_rune;
    gain_t* energy_regen;
    gain_t* focus_regen;
    gain_t* innervate;
    gain_t* glyph_of_innervate;
    gain_t* judgement_of_wisdom;
    gain_t* mana_gem;
    gain_t* mana_potion;
    gain_t* mana_spring;
    gain_t* mana_tide;
    gain_t* mark_of_defiance;
    gain_t* mirror_of_truth;
    gain_t* mp5_regen;
    gain_t* replenishment;
    gain_t* restore_mana;
    gain_t* spellsurge;
    gain_t* spirit_regen;
    gain_t* vampiric_touch;
    gain_t* water_elemental;
    gain_t *tier4_2pc, *tier4_4pc;
    gain_t *tier5_2pc, *tier5_4pc;
    gain_t *tier6_2pc, *tier6_4pc;
    gain_t *tier7_2pc, *tier7_4pc;
    void reset() { memset( (void*) this, 0x00, sizeof( gains_t ) ); }
    gains_t() { reset(); }
  };
  gains_t gains;

  struct procs_t
  {
    proc_t* ashtongue_talisman;
	proc_t* darkmoon_greatness;
    proc_t* dying_curse;
    proc_t* elder_scribes;
    proc_t* embrace_of_the_spider;
    proc_t* eternal_sage;
    proc_t* extract_of_necromatic_power;
    proc_t* eye_of_magtheridon;
    proc_t* forge_ember;
    proc_t* judgement_of_wisdom;
    proc_t* lightning_capacitor;
    proc_t* mark_of_defiance;
    proc_t* mirror_of_truth;
    proc_t* mystical_skyfire;
    proc_t* quagmirrans_eye;
    proc_t* sextant_of_unstable_currents;
    proc_t* shiffars_nexus_horn;
    proc_t* spellstrike;
    proc_t* sundial_of_the_exiled;
    proc_t* egg_of_mortal_essence;
    proc_t* thunder_capacitor;
    proc_t* timbals_crystal;
    proc_t* windfury;
    proc_t* wrath_of_cenarius;
    proc_t *tier4_2pc, *tier4_4pc;
    proc_t *tier5_2pc, *tier5_4pc;
    proc_t *tier6_2pc, *tier6_4pc;
    proc_t *tier7_2pc, *tier7_4pc;
    void reset() { memset( (void*) this, 0x00, sizeof( procs_t ) ); }
    procs_t() { reset(); }
  };
  procs_t procs;

  struct actions_t
  {
    action_t* extract_of_necromatic_power_discharge;
    action_t* lightning_discharge;
    action_t* thunder_discharge;
    action_t* timbals_discharge;
    void reset() { memset( (void*) this, 0x00, sizeof( actions_t ) ); }
    actions_t() { reset(); }
  };
  actions_t actions;

  struct scaling_t
  {
    double attribute[ ATTRIBUTE_MAX ];
    double spell_power;
    double attack_power;
    double expertise_rating;
    double armor_penetration_rating;
    double hit_rating;
    double crit_rating;
    double haste_rating;

    scaling_t() { memset( (void*) this, 0x00, sizeof( scaling_t ) ); }
  };
  scaling_t scaling;

  player_t( sim_t* sim, int type, const std::string& name );

  virtual ~player_t();

  virtual const char* name() { return name_str.c_str(); }

  virtual void init();
  virtual void init_base() = 0;
  virtual void init_core();
  virtual void init_spell();
  virtual void init_attack();
  virtual void init_weapon( weapon_t*, std::string& );
  virtual void init_unique_gear();
  virtual void init_resources( bool force = false );
  virtual void init_consumables();
  virtual void init_actions();
  virtual void init_rating();
  virtual void init_stats();

  virtual void reset();
  virtual void combat_begin();
  virtual void combat_end();

  virtual double composite_attack_power();
  virtual double composite_attack_crit();
  virtual double composite_attack_expertise()   { return attack_expertise;   }
  virtual double composite_attack_hit()         { return attack_hit;         }
  virtual double composite_attack_penetration() { return attack_penetration; }

  virtual double composite_spell_power( int school );
  virtual double composite_spell_crit();
  virtual double composite_spell_hit()         { return spell_hit;         }
  virtual double composite_spell_penetration() { return spell_penetration; }

  virtual double composite_attack_power_multiplier();
  virtual double composite_spell_power_multiplier() { return spell_power_multiplier; }
  virtual double composite_attribute_multiplier( int attr );

  virtual double strength();
  virtual double agility();
  virtual double stamina();
  virtual double intellect();
  virtual double spirit();

  virtual void      schedule_ready( double delta_time=0, bool waiting=false );
  virtual action_t* execute_action();

  virtual void raid_event( action_t* ) {}
  virtual void regen( double periodicity=2.0 );
  virtual void resource_gain( int resource, double amount, gain_t* g=0 );
  virtual void resource_loss( int resource, double amount );
  virtual bool resource_available( int resource, double cost );
  virtual int  primary_resource() { return RESOURCE_NONE; }

  virtual void  summon_pet( const char* name );
  virtual void dismiss_pet( const char* name );

  // Managing action_xxx events:
  // (1) To "throw" an event, ALWAYS invoke the action_xxx function.
  // (2) To "catch" an event, ALWAYS implement a spell_xxx or attack_xxx virtual function in player sub-class.
  // Disregarding these instructions may result in serious injury and/or death.

  virtual void action_start ( action_t* );
  virtual void action_miss  ( action_t* );
  virtual void action_hit   ( action_t* );
  virtual void action_tick  ( action_t* );
  virtual void action_damage( action_t*, double amount, int dmg_type );
  virtual void action_heal  ( action_t*, double amount );
  virtual void action_finish( action_t* );

  virtual void spell_start_event ( spell_t* );
  virtual void spell_miss_event  ( spell_t* );
  virtual void spell_hit_event   ( spell_t* );
  virtual void spell_tick_event  ( spell_t* );
  virtual void spell_damage_event( spell_t*, double amount, int dmg_type );
  virtual void spell_heal_event  ( spell_t*, double amount );
  virtual void spell_finish_event( spell_t* );

  virtual void attack_start_event ( attack_t* );
  virtual void attack_miss_event  ( attack_t* );
  virtual void attack_hit_event   ( attack_t* );
  virtual void attack_tick_event  ( attack_t* );
  virtual void attack_damage_event( attack_t*, double amount, int dmg_type );
  virtual void attack_heal_event  ( attack_t*, double amount );
  virtual void attack_finish_event( attack_t* );

  virtual bool get_talent_trees( std::vector<int*>& tree1, std::vector<int*>& tree2, std::vector<int*>& tree3, talent_translation_t translation[][3] );
  virtual bool get_talent_trees( std::vector<int*>& tree1, std::vector<int*>& tree2, std::vector<int*>& tree3 ) { return false; }
  virtual bool parse_talents( std::vector<int*>& talent_tree, const std::string& talent_string );
  virtual bool parse_talents( const std::string& talent_string );
  virtual bool parse_talents_mmo( const std::string& talent_string );
  virtual bool parse_talents_wowhead( const std::string& talent_string );
  virtual bool parse_talents( const std::string& talent_string, int encoding );

  virtual bool      parse_option ( const std::string& name, const std::string& value );
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual pet_t*    create_pet   ( const std::string& name ) { return 0; }
  virtual pet_t*    find_pet     ( const std::string& name );

  // Class-Specific Methods

  static player_t * create_druid  ( sim_t* sim, std::string& name );
  static player_t * create_hunter ( sim_t* sim, std::string& name );
  static player_t * create_mage   ( sim_t* sim, std::string& name );
  static player_t * create_priest ( sim_t* sim, std::string& name );
  static player_t * create_rogue  ( sim_t* sim, std::string& name );
  static player_t * create_shaman ( sim_t* sim, std::string& name );
  static player_t * create_warlock( sim_t* sim, std::string& name );

  bool is_pet() { return type == PLAYER_PET || type == PLAYER_GUARDIAN; }

  druid_t  * cast_druid  () { assert( type == DRUID      ); return (druid_t  *) this; }
  hunter_t * cast_hunter () { assert( type == HUNTER     ); return (hunter_t *) this; }
  mage_t   * cast_mage   () { assert( type == MAGE       ); return (mage_t   *) this; }
  priest_t * cast_priest () { assert( type == PRIEST     ); return (priest_t *) this; }
  rogue_t  * cast_rogue  () { assert( type == ROGUE      ); return (rogue_t  *) this; }
  shaman_t * cast_shaman () { assert( type == SHAMAN     ); return (shaman_t *) this; }
  warlock_t* cast_warlock() { assert( type == WARLOCK    ); return (warlock_t*) this; }
  pet_t*     cast_pet    () { assert( is_pet()           ); return (pet_t    *) this; }

  bool      in_gcd() { return gcd_ready > sim -> current_time; }
  bool      recent_cast();
  action_t* find_action( const std::string& );
  void      share_cooldown( const std::string& name, double ready );
  void      share_duration( const std::string& name, double ready );
  void      recalculate_haste()  {  haste = 1.0 / ( 1.0 + haste_rating / rating.haste ); }
  double    spirit_regen_per_second();
  bool      dual_wield() { return main_hand_weapon.type != WEAPON_NONE && off_hand_weapon.type != WEAPON_NONE; }
  void      aura_gain( const char* name );
  void      aura_loss( const char* name );
  gain_t*   get_gain  ( const std::string& name );
  proc_t*   get_proc  ( const std::string& name );
  stats_t*  get_stats ( const std::string& name );
  uptime_t* get_uptime( const std::string& name );
};

// Pet =======================================================================

struct pet_t : public player_t
{
  std::string full_name_str;
  player_t* owner;
  pet_t* next_pet;

  double   stamina_per_owner;
  double intellect_per_owner;

  double summon_time;

  pet_t( sim_t* sim, player_t* owner, const std::string& name, bool guardian=false );

  virtual double composite_attack_hit() { return owner -> composite_attack_hit(); }
  virtual double composite_spell_hit()  { return owner -> composite_spell_hit();  }

  virtual double stamina();
  virtual double intellect();

  virtual void init();
  virtual void reset();
  virtual void summon();
  virtual void dismiss();

  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual const char* name() { return full_name_str.c_str(); }
};

// Target ====================================================================

struct target_t
{
  sim_t*      sim;
  std::string name_str, race_str;
  int      race;
  int      level;
  int     spell_resistance[ SCHOOL_MAX ];
  int     initial_armor, armor;
  int     block_value;
  int      shield;
  double      initial_health, current_health;
  double      total_dmg;
  uptime_t*   uptime_list;

  struct cooldowns_t
  {
    double place_holder;
    void reset() { memset( (void*) this, 0x00, sizeof( cooldowns_t ) ); }
    cooldowns_t() { reset(); }
  };
  cooldowns_t cooldowns;

  struct debuff_t
  {
    // Static De-Buffs (until appropriate player class implemented)
    int    blood_frenzy;
    int    crypt_fever;
    int    judgement_of_wisdom;
    int    razorice;
    int    snare;
    double sunder_armor;
    int    thunder_clap;
    // Dynamic De-Buffs
    int    temporary_debuffs;
    int    affliction_effects;
    int    curse_of_elements;
    double expose_armor;
    double faerie_fire;
    int    ferocious_inspiration;
    double frozen;
    double hemorrhage;
    int    hemorrhage_charges;
    double hunters_mark;
    int    improved_faerie_fire;
    int    improved_scorch;
    int    mangle;
    int    master_poisoner;
    int    misery;
    int    misery_stack;
    int    earth_and_moon;
    int    poisoned;
    int    savage_combat;
    int    slow;
    int    totem_of_wrath;
    int    winters_chill;
    int    winters_grasp;
    debuff_t() { memset( (void*) this, 0x0, sizeof( debuff_t ) ); }
    void reset()
    {
      size_t delta = ( (uintptr_t) &temporary_debuffs ) - ( (uintptr_t) this );
      memset( (void*) &temporary_debuffs, 0x0, sizeof( debuff_t ) - delta );
    }
    bool snared() { return frozen || slow || snare || thunder_clap; }
  };
  debuff_t debuffs;

  struct expirations_t
  {
    event_t* curse_of_elements;
    event_t* expose_armor;
    event_t* faerie_fire;
    event_t* ferocious_inspiration;
    event_t* frozen;
    event_t* earth_and_moon;
    event_t* hemorrhage;
    event_t* hunters_mark; 
    event_t* improved_faerie_fire;
    event_t* improved_scorch;
    event_t* mangle;
    event_t* nature_vulnerability;
    event_t* shadow_vulnerability;
    event_t* shadow_weaving;
    event_t* winters_chill;
    event_t* winters_grasp;
    void reset() { memset( (void*) this, 0x00, sizeof( expirations_t ) ); }
    expirations_t() { reset(); }
  };
  expirations_t expirations;

  struct uptimes_t
  {
    uptime_t* blood_frenzy;
    uptime_t* improved_scorch;
    uptime_t* mangle;
    uptime_t* master_poisoner;
    uptime_t* savage_combat;
    uptime_t* totem_of_wrath;
    uptime_t* winters_chill;
    uptime_t* winters_grasp;
    void reset() { memset( (void*) this, 0x00, sizeof( uptimes_t ) ); }
    uptimes_t() { reset(); }
  };
  uptimes_t uptimes;

  target_t( sim_t* s );
 ~target_t();

  void init();
  void reset();
  void combat_begin();
  void combat_end();
  void assess_damage( double amount, int school, int type );
  void recalculate_health();
  double time_to_die();
  double health_percentage();
  double composite_armor();
  uptime_t* get_uptime( const std::string& name );
  bool parse_option( const std::string& name, const std::string& value );
  const char* name() { return name_str.c_str(); }
};

// Stats =====================================================================

struct stats_t
{
  std::string name_str;
  sim_t* sim;
  player_t* player;
  stats_t* next;
  int school;
  bool channeled;
  bool analyzed;
  bool initialized;

  double resource_consumed;
  double frequency, num_executes, num_ticks;
  double total_execute_time, total_tick_time;
  double total_dmg;
  double dps, dpe, dpet, dpr;
  double total_intervals, num_intervals, last_execute;

  struct stats_results_t
  {
    double count, min_dmg, max_dmg, avg_dmg, total_dmg;
  };
  stats_results_t execute_results[ RESULT_MAX ];
  stats_results_t    tick_results[ RESULT_MAX ];

  int num_buckets;
  std::vector<double> timeline_dmg;
  std::vector<double> timeline_dps;

  void consume_resource( double r ) { resource_consumed += r; }
  void add( double amount, int dmg_type, int result, double time );
  void init();
  void reset( action_t* );
  void analyze();
  void merge( stats_t* other );
  stats_t( const std::string& name, player_t* );
};

// Rank ======================================================================

struct rank_t
{
  int level, index;
  double dd_min, dd_max, tick, cost;
};

// Base Stats ================================================================

struct base_stats_t
{
  int level;
  double health, mana;
  double strength, agility, stamina, intellect, spirit;
  double spell_crit, melee_crit;
};

// Action ====================================================================

struct action_t
{
  sim_t* sim;
  int type;
  std::string name_str;
  player_t* player;
  int school, resource, tree, result;
  bool binary, channeled, background, repeating, aoe, harmful, proc, heal;
  bool may_miss, may_resist, may_dodge, may_parry, may_glance, may_block, may_crush, may_crit;
  double min_gcd, trigger_gcd;
  double base_execute_time, base_tick_time, base_cost;
  double base_dd_min, base_dd_max, base_td_init;
  double base_dd_multiplier, base_td_multiplier, power_multiplier;
  double   base_multiplier,   base_hit,   base_crit,   base_power,   base_penetration;
  double player_multiplier, player_hit, player_crit, player_power, player_penetration;
  double target_multiplier, target_hit, target_crit, target_power, target_penetration;
  double   base_crit_multiplier,   base_crit_bonus_multiplier, base_crit_bonus;
  double player_crit_multiplier, player_crit_bonus_multiplier;
  double target_crit_multiplier, target_crit_bonus_multiplier;
  double player_dd_adder, target_dd_adder;
  double resource_consumed;
  double direct_dmg, base_direct_dmg, direct_power_mod;
  double   tick_dmg, base_tick_dmg,     tick_power_mod;
  int num_ticks, current_tick, added_ticks;
  int ticking;
  std::string cooldown_group, duration_group;
  double cooldown, cooldown_ready, duration_ready;
  weapon_t* weapon;
  double weapon_multiplier;
  bool normalize_weapon_damage;
  bool normalize_weapon_speed;
  stats_t* stats;
  event_t* execute_event;
  event_t* tick_event;
  double time_to_execute, time_to_tick;
  int rank_index, bloodlust_active;
  double min_time_to_die, max_time_to_die;
  double min_health_percentage, max_health_percentage;
  std::string sync_str;
  action_t*   sync_action;
  action_t** observer;
  action_t* next;
  action_t* sequence;

  action_t( int type, const char* name, player_t* p=0, int r=RESOURCE_NONE, int s=SCHOOL_NONE, int t=TREE_NONE );
  virtual ~action_t() {}

  virtual void      parse_options( option_t*, const std::string& options_str );
  virtual option_t* merge_options( std::vector<option_t>&, option_t*, option_t* );
  virtual rank_t*   init_rank( rank_t* rank_list );

  virtual double cost();
  virtual double haste()        { return 1.0;               }
  virtual double gcd()          { return trigger_gcd;       }
  virtual double execute_time() { return base_execute_time; }
  virtual double tick_time()    { return base_tick_time;    }
  virtual void   player_buff();
  virtual void   target_debuff( int dmg_type );
  virtual void   calculate_result() { assert(0); }
  virtual bool   result_is_hit();
  virtual bool   result_is_miss();
  virtual double calculate_direct_damage();
  virtual double calculate_tick_damage();
  virtual double armor();
  virtual double resistance();
  virtual void   consume_resource();
  virtual void   execute();
  virtual void   tick();
  virtual void   last_tick();
  virtual void   assess_damage( double amount, int dmg_type );
  virtual void   schedule_execute();
  virtual void   schedule_tick();
  virtual void   refresh_duration();
  virtual void   extend_duration( int extra_ticks );
  virtual void   update_ready();
  virtual void   update_stats( int type );
  virtual bool   ready();
  virtual void   reset();
  virtual void   cancel();
  virtual const char* name() { return name_str.c_str(); }

  virtual double total_multiplier() { return   base_multiplier * player_multiplier * target_multiplier; }
  virtual double total_power()      { return ( base_power      + player_power      + target_power       ) * power_multiplier; }
  virtual double total_hit()        { return   base_hit        + player_hit        + target_hit;        }
  virtual double total_crit()       { return   base_crit       + player_crit       + target_crit;       }
  virtual double total_crit_bonus();

  // Some actions require different multipliers for the "direct" and "tick" portions.

  virtual double total_dd_multiplier() { return total_multiplier() * base_dd_multiplier; }
  virtual double total_td_multiplier() { return total_multiplier() * base_td_multiplier; }

  static action_t* create_action( player_t*, const std::string& name, const std::string& options );
};

// Attack ====================================================================

struct attack_t : public action_t
{
  double base_expertise, player_expertise, target_expertise;

  attack_t( const char* n=0, player_t* p=0, int r=RESOURCE_NONE, int s=SCHOOL_PHYSICAL, int t=TREE_NONE );
  virtual ~attack_t() {}

  // Attack Overrides
  virtual void   parse_options( option_t*, const std::string& options_str );
  virtual double haste();
  virtual double execute_time();
  virtual void   player_buff();
  virtual void   target_debuff( int dmg_type );
  virtual int    build_table( double* chances, int* results );
  virtual void   calculate_result();

  // Passthru Methods
  virtual double cost()                              { return action_t::cost();                    }
  virtual double gcd()                               { return action_t::gcd();                     }
  virtual double tick_time()                         { return action_t::tick_time();               }
  virtual double calculate_direct_damage()           { return action_t::calculate_direct_damage(); }
  virtual double calculate_tick_damage()             { return action_t::calculate_tick_damage();   }
  virtual double resistance()                        { return action_t::resistance();              }
  virtual void   consume_resource()                  { action_t::consume_resource();               }
  virtual void   execute()                           { action_t::execute();                        }
  virtual void   tick()                              { action_t::tick();                           }
  virtual void   last_tick()                         { action_t::last_tick();                      }
  virtual void   assess_damage( double a, int t ) { action_t::assess_damage( a, t );            }
  virtual void   schedule_execute()                  { action_t::schedule_execute();               }
  virtual void   schedule_tick()                     { action_t::schedule_tick();                  }
  virtual bool   ready()                             { return action_t::ready();                   }
  virtual void   reset()                             { action_t::reset();                          }

  virtual double total_expertise() { return base_expertise + player_expertise + target_expertise; }
};

// Spell =====================================================================

struct spell_t : public action_t
{
  spell_t( const char* n=0, player_t* p=0, int r=RESOURCE_NONE, int s=SCHOOL_PHYSICAL, int t=TREE_NONE );
  virtual ~spell_t() {}

  // Spell Overrides
  virtual void   parse_options( option_t*, const std::string& options_str );
  virtual double haste();
  virtual double gcd();
  virtual double execute_time();
  virtual double tick_time();
  virtual void   player_buff();
  virtual void   target_debuff( int dmg_type );
  virtual double level_based_miss_chance( int player, int target );
  virtual void   calculate_result();

  // Passthru Methods
  virtual double cost()                              { return action_t::cost();                    }
  virtual double calculate_direct_damage()           { return action_t::calculate_direct_damage(); }
  virtual double calculate_tick_damage()             { return action_t::calculate_tick_damage();   }
  virtual double resistance()                        { return action_t::resistance();              }
  virtual void   consume_resource()                  { action_t::consume_resource();               }
  virtual void   execute()                           { action_t::execute();                        }
  virtual void   tick()                              { action_t::tick();                           }
  virtual void   last_tick()                         { action_t::last_tick();                      }
  virtual void   assess_damage( double a, int t ) { action_t::assess_damage( a, t );            }
  virtual void   schedule_execute()                  { action_t::schedule_execute();               }
  virtual void   schedule_tick()                     { action_t::schedule_tick();                  }
  virtual bool   ready()                             { return action_t::ready();                   }
  virtual void   reset()                             { action_t::reset();                          }
};

// Player Ready Event ========================================================

struct player_ready_event_t : public event_t
{
  player_ready_event_t( sim_t* sim, player_t* p, double delta_time );
  virtual void execute();
};

// Action Execute Event ======================================================

struct action_execute_event_t : public event_t
{
  action_t* action;
  action_execute_event_t( sim_t* sim, action_t* a, double time_to_execute );
  virtual void execute();
};

// Action Tick Event =========================================================

struct action_tick_event_t : public event_t
{
  action_t* action;
  action_tick_event_t( sim_t* sim, action_t* a, double time_to_tick );
  virtual void execute();
};

// Regen Event ===============================================================

struct regen_event_t : public event_t
{
   regen_event_t( sim_t* sim );
   virtual void execute();
};

// Unique Gear ===============================================================

struct unique_gear_t
{
  static void spell_start_event ( spell_t* ) {}
  static void spell_miss_event  ( spell_t* );
  static void spell_hit_event   ( spell_t* );
  static void spell_tick_event  ( spell_t* );
  static void spell_damage_event( spell_t*, double amount, int dmg_type );
  static void spell_heal_event  ( spell_t*, double amount );
  static void spell_finish_event( spell_t* );

  static void attack_start_event ( attack_t* ) {}
  static void attack_miss_event  ( attack_t* ) {}
  static void attack_hit_event   ( attack_t* );
  static void attack_tick_event  ( attack_t* ) {}
  static void attack_damage_event( attack_t*, double amount, int dmg_type );
  static void attack_heal_event  ( attack_t*, double amount ) {}
  static void attack_finish_event( attack_t* ) {}

  static bool parse_option( player_t*, const std::string& name, const std::string& value );

  static action_t* create_action( player_t*, const std::string& name, const std::string& options );
  static void init( player_t* );
};

// Enchants ===================================================================

struct enchant_t
{
  static void spell_start_event ( spell_t* ) {}
  static void spell_miss_event  ( spell_t* ) {}
  static void spell_hit_event   ( spell_t* ) {}
  static void spell_tick_event  ( spell_t* ) {}
  static void spell_damage_event( spell_t*, double amount, int dmg_type ) {}
  static void spell_heal_event  ( spell_t*, double amount ) {}
  static void spell_finish_event( spell_t* );

  static void attack_start_event ( attack_t* ) {}
  static void attack_miss_event  ( attack_t* ) {}
  static void attack_hit_event   ( attack_t* );
  static void attack_tick_event  ( attack_t* ) {}
  static void attack_damage_event( attack_t*, double amount, int dmg_type ) {}
  static void attack_heal_event  ( attack_t*, double amount ) {}
  static void attack_finish_event( attack_t* ) {}

  static void trigger_flametongue_totem( attack_t* );
  static void trigger_windfury_totem   ( attack_t* );
};

// Consumable ================================================================

struct consumable_t
{
  static void init_flask  ( player_t* );
  static void init_elixirs( player_t* );
  static void init_food   ( player_t* );

  static action_t* create_action( player_t*, const std::string& name, const std::string& options );
};

// Gain ======================================================================

struct gain_t
{
  std::string name_str;
  double amount;
  gain_t* next;
  gain_t( const std::string& n ) : name_str(n), amount(0) {}
  void add( double a ) { amount += a; }
  void merge( gain_t* other ) { amount += other -> amount; }
  const char* name() { return name_str.c_str(); }
};

// Proc ======================================================================

struct proc_t
{
  std::string name_str;
  double count;
  double frequency;
  proc_t* next;
  proc_t( const std::string& n ) : name_str(n), count(0), frequency(0) {}
  void occur() { count++; }
  void merge( proc_t* other ) { count += other -> count; }
  const char* name() { return name_str.c_str(); }
};

// Up-Time ===================================================================

struct uptime_t
{
  std::string name_str;
  int up, down;
  uptime_t* next;
  uptime_t( const std::string& n ) : name_str(n), up(0), down(0) {}
  void   update( bool is_up ) { if( is_up ) up++; else down++; }
  double percentage() { return (up==0) ? 0 : (100.0*up/(up+down)); }
  void   merge( uptime_t* other ) { up += other -> up; down += other -> down; }
  const char* name() { return name_str.c_str(); }
};

// Report =====================================================================

struct report_t
{
  sim_t* sim;

  int report_actions;
  int report_attack_stats;
  int report_chart;
  int report_core_stats;
  int report_dpr;
  int report_dps;
  int report_gains;
  int report_miss;
  int report_rps;
  int report_name;
  int report_performance;
  int report_procs;
  int report_raid_dps;
  int report_scaling;
  int report_spell_stats;
  int report_statistics;
  int report_tag;
  int report_uptime;
  int report_waiting;

  report_t( sim_t* s );
  bool parse_option( const std::string& name, const std::string& value );
  void print_action      ( stats_t* );
  void print_actions     ( player_t* );
  void print_core_stats  ( player_t* );
  void print_spell_stats ( player_t* );
  void print_attack_stats( player_t* );
  void print_gains();
  void print_procs();
  void print_uptime();
  void print_waiting();
  void print_performance();
  void print_scale_factors();
  void print();
  const char* chart_raid_dps( std::string& s );
  int         chart_raid_dpet( std::string& s , std::vector<std::string>& images );
  const char* chart_raid_downtime( std::string& s );
  const char* chart_raid_gear( std::string& s );
  const char* chart_raid_uptimes( std::string& s );
  const char* chart_action_dpet      ( std::string& s, player_t* );
  const char* chart_action_dmg       ( std::string& s, player_t* );
  const char* chart_gains            ( std::string& s, player_t* );
  const char* chart_uptimes_and_procs( std::string& s, player_t* );
  const char* chart_timeline_resource( std::string& s, player_t* );
  const char* chart_timeline_dps     ( std::string& s, player_t* );
  const char* chart_distribution_dps ( std::string& s, player_t* );
  const char* gear_weights_lootrank( std::string& s, player_t* );
  const char* gear_weights_wowhead ( std::string& s, player_t* );
  void html_scale_factors();
  void wiki_scale_factors();
  void html_trigger_menu();
  void chart_html();
  void chart_wiki();
  void chart();
  static void timestamp( sim_t* sim );
  static void va_printf( sim_t*, const char* format, va_list );
  inline static void log( sim_t* sim, const char* format, ... )
  {
    va_list vap;
    va_start( vap, format );
    va_printf( sim, format, vap );
  }
};

// Talent Translation =========================================================

struct talent_translation_t
{
  int  index;
  int* address;
};

// Options ====================================================================

enum option_type_t { OPT_STRING=0, OPT_APPEND, OPT_CHAR_P, OPT_BOOL, OPT_INT, OPT_FLT, OPT_LIST, OPT_DEPRECATED, OPT_UNKNOWN };

struct option_t
{
  const char* name;
  int type;
  void*  address;

  static void print( sim_t*, option_t* );
  static bool parse( sim_t*, option_t*, const std::string& name, const std::string& value );
  static bool parse( sim_t*, FILE* file );
  static bool parse( sim_t*, char* line );
  static bool parse( sim_t*, std::string& token );
  static bool parse( sim_t*, int argc, char** argv );
};

// Utilities =================================================================

struct util_t
{
  static double talent_rank( int num, int max, double increment );
  static double talent_rank( int num, int max, double value1, double value2, ... );

  static int talent_rank( int num, int max, int increment );
  static int talent_rank( int num, int max, int value1, int value2, ... );

  static double ability_rank( int player_level, double ability_value, int ability_level, ... );
  static int    ability_rank( int player_level, int    ability_value, int ability_level, ... );

  static char* dup( const char* );

  static const char* race_type_string          ( int type );
  static const char* player_type_string        ( int type );
  static const char* attribute_type_string     ( int type );
  static const char* dmg_type_string           ( int type );
  static const char* result_type_string        ( int type );
  static const char* resource_type_string      ( int type );
  static const char* school_type_string        ( int type );
  static const char* talent_tree_string        ( int tree );
  static const char* weapon_type_string        ( int type );
  static const char* weapon_enchant_type_string( int type );
  static const char* weapon_buff_type_string   ( int type );
  static const char* elixir_type_string        ( int type );
  static const char* flask_type_string         ( int type );
  static const char* food_type_string          ( int type );

  static int string_split( std::vector<std::string>& results, const std::string& str, const char* delim );
  static int string_split( const std::string& str, const char* delim, const char* format, ... );
};
