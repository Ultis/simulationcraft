// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simcraft.h"

// ==========================================================================
// Mage
// ==========================================================================

enum { ROTATION_NONE=0, ROTATION_DPS, ROTATION_DPM, ROTATION_MAX };

struct mage_t : public player_t
{
  // Active
  spell_t* active_ignite;
  pet_t*   active_water_elemental;

  // Buffs
  buff_t* buffs_arcane_blast;
  buff_t* buffs_arcane_power;
  buff_t* buffs_brain_freeze;
  buff_t* buffs_clearcasting;
  buff_t* buffs_combustion;
  buff_t* buffs_fingers_of_frost;
  buff_t* buffs_ghost_charge;
  buff_t* buffs_hot_streak;
  buff_t* buffs_hot_streak_crits;
  buff_t* buffs_icy_veins;
  buff_t* buffs_incanters_absorption;
  buff_t* buffs_mage_armor;
  buff_t* buffs_missile_barrage;
  buff_t* buffs_molten_armor;

  // Gains
  gain_t* gains_clearcasting;
  gain_t* gains_empowered_fire;
  gain_t* gains_evocation;
  gain_t* gains_mana_gem;
  gain_t* gains_master_of_elements;

  // Procs
  proc_t* procs_deferred_ignite;
  proc_t* procs_mana_gem;

  // Up-Times
  uptime_t* uptimes_arcane_blast[ 4 ];
  uptime_t* uptimes_dps_rotation;
  uptime_t* uptimes_dpm_rotation;
  uptime_t* uptimes_focus_magic_feedback;
  uptime_t* uptimes_water_elemental;

  // Random Number Generation
  rng_t* rng_empowered_fire;
  rng_t* rng_frostbite;
  rng_t* rng_ghost_charge;
  rng_t* rng_improved_scorch;
  rng_t* rng_improved_water_elemental;
  rng_t* rng_winters_chill;
  rng_t* rng_winters_grasp;

  // Options
  std::string focus_magic_target_str;
  std::string armor_type_str;
  double      ghost_charge_pct;

  // Rotation (DPS vs DPM)
  struct rotation_t
  {
    int    current;
    double mana_gain;
    double dps_mana_loss;
    double dpm_mana_loss;
    double dps_time;
    double dpm_time;

    void reset() { memset( ( void* ) this, 0x00, sizeof( rotation_t ) ); current = ROTATION_DPS; }
    rotation_t() { reset(); }
  };
  rotation_t rotation;

  struct talents_t
  {
    int  arcane_barrage;
    int  arcane_concentration;
    int  arcane_empowerment;
    int  arcane_flows;
    int  arcane_focus;
    int  arcane_impact;
    int  arcane_instability;
    int  arcane_meditation;
    int  arcane_mind;
    int  arcane_potency;
    int  arcane_power;
    int  arcane_subtlety;
    int  arctic_winds;
    int  brain_freeze;
    int  burning_soul;
    int  burnout;
    int  chilled_to_the_bone;
    int  cold_as_ice;
    int  cold_snap;
    int  combustion;
    int  critical_mass;
    int  deep_freeze;
    int  empowered_arcane_missiles;
    int  empowered_fire;
    int  empowered_frost_bolt;
    int  fingers_of_frost;
    int  fire_power;
    int  focus_magic;
    int  frost_channeling;
    int  frostbite;
    int  hot_streak;
    int  ice_floes;
    int  ice_shards;
    int  icy_veins;
    int  ignite;
    int  improved_fire_ball;
    int  improved_fire_blast;
    int  improved_frost_bolt;
    int  improved_scorch;
    int  improved_water_elemental;
    int  incanters_absorption;
    int  incineration;
    int  living_bomb;
    int  master_of_elements;
    int  mind_mastery;
    int  missile_barrage;
    int  molten_fury;
    int  netherwind_presence;
    int  piercing_ice;
    int  playing_with_fire;
    int  precision;
    int  presence_of_mind;
    int  pyroblast;
    int  pyromaniac;
    int  shatter;
    int  slow;
    int  spell_impact;
    int  spell_power;
    int  student_of_the_mind;
    int  summon_water_elemental;
    int  torment_the_weak;
    int  winters_chill;
    int  winters_grasp;
    int  world_in_flames;

    talents_t() { memset( ( void* ) this, 0x0, sizeof( talents_t ) ); }
  };
  talents_t talents;

  struct glyphs_t
  {
    int arcane_barrage;
    int arcane_blast;
    int arcane_missiles;
    int arcane_power;
    int fire_ball;
    int frost_bolt;
    int ice_lance;
    int improved_scorch;
    int living_bomb;
    int mage_armor;
    int mana_gem;
    int mirror_image;
    int molten_armor;
    int water_elemental;
    int frostfire;
    glyphs_t() { memset( ( void* ) this, 0x0, sizeof( glyphs_t ) ); }
  };
  glyphs_t glyphs;

  mage_t( sim_t* sim, const std::string& name ) : player_t( sim, MAGE, name )
  {
    // Active
    active_ignite          = 0;
    active_water_elemental = 0;
    
    armor_type_str = "molten"; // Valid values: molten|mage
    ghost_charge_pct = 1.0;
    distance = 30;
  }

  // Character Definition
  virtual void      init_base();
  virtual void      init_glyphs();
  virtual void      init_buffs();
  virtual void      init_gains();
  virtual void      init_procs();
  virtual void      init_uptimes();
  virtual void      init_rng();
  virtual void      init_actions();
  virtual void      combat_begin();
  virtual void      reset();
  virtual bool      get_talent_trees( std::vector<int*>& arcane, std::vector<int*>& fire, std::vector<int*>& frost );
  virtual std::vector<option_t>& get_options();
  virtual bool      save( FILE* file, int save_type=SAVE_ALL );
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual pet_t*    create_pet   ( const std::string& name );
  virtual int       decode_set( item_t& item );
  virtual int       primary_resource() SC_CONST { return RESOURCE_MANA; }
  virtual int       primary_role() SC_CONST     { return ROLE_SPELL; }
  virtual int       primary_tree() SC_CONST;
  virtual double    composite_spell_power( int school ) SC_CONST;

  // Event Tracking
  virtual void   regen( double periodicity );
  virtual double resource_gain( int resource, double amount, gain_t* source=0, action_t* action=0 );
  virtual double resource_loss( int resource, double amount, action_t* action=0 );
};

namespace { // ANONYMOUS NAMESPACE ==========================================

// stack_winters_chill =====================================================

static void stack_winters_chill( spell_t* s,
                                 double   chance )
{
  if ( s -> school != SCHOOL_FROST &&
       s -> school != SCHOOL_FROSTFIRE ) return;

  struct expiration_t : public event_t
  {
    expiration_t( sim_t* sim ) : event_t( sim )
    {
      name = "Winters Chill Expiration";
      sim -> add_event( this, 15.0 );
    }
    virtual void execute()
    {
      if ( sim -> log ) log_t::output( sim, "Target %s loses Winters Chill", sim -> target -> name() );
      sim -> target -> debuffs.winters_chill = 0;
      sim -> target -> expirations.winters_chill = 0;
    }
  };

  if ( chance < 1.0 )
  {
    mage_t* p = s -> player -> cast_mage();

    if ( p -> rng_winters_chill -> roll( chance ) )
      return;
  }

  target_t* t = s -> sim -> target;

  if ( t -> debuffs.winters_chill < 5 )
  {
    t -> debuffs.winters_chill += 1;

    if ( s -> sim -> log )
      log_t::output( s -> sim, "Target %s gains Winters Chill %d",
                     t -> name(), t -> debuffs.winters_chill );
  }

  event_t*& e = t -> expirations.winters_chill;

  if ( e )
  {
    e -> reschedule( 15.0 );
  }
  else
  {
    e = new ( s -> sim ) expiration_t( s -> sim );
  }
}

// ==========================================================================
// Mage Spell
// ==========================================================================

struct mage_spell_t : public spell_t
{
  bool may_torment;
  int dps_rotation;
  int dpm_rotation;
  int arcane_power;
  int icy_veins;

  mage_spell_t( const char* n, player_t* player, int s, int t ) :
      spell_t( n, player, RESOURCE_MANA, s, t ),
      may_torment( false ),
      dps_rotation( 0 ),
      dpm_rotation( 0 ),
      arcane_power( 0 ),
      icy_veins( 0 )
  {
    mage_t* p = player -> cast_mage();
    base_hit += p -> talents.precision * 0.01;
  }

  virtual void   parse_options( option_t*, const std::string& );
  virtual bool   ready();
  virtual double cost() SC_CONST;
  virtual double haste() SC_CONST;
  virtual void   execute();
  virtual void   consume_resource();
  virtual void   player_buff();
};

// ==========================================================================
// Pet Water Elemental
// ==========================================================================

struct water_elemental_pet_t : public pet_t
{
  struct water_bolt_t : public spell_t
  {
    water_bolt_t( player_t* player ):
        spell_t( "water_bolt", player, RESOURCE_MANA, SCHOOL_FROST, TREE_FROST )
    {
      base_cost          = 0;
      base_execute_time  = 2.5;
      base_dd_min        = 256 + ( player -> level - 50 ) * 11.5;
      base_dd_max        = 328 + ( player -> level - 50 ) * 11.5;
      direct_power_mod   = ( 5.0 / 6.0 );
      may_crit           = true;
    }
    virtual void player_buff()
    {
      spell_t::player_buff();
      player_spell_power += player -> cast_pet() -> owner -> composite_spell_power( SCHOOL_FROST ) / 3.0;
    }
  };

  water_elemental_pet_t( sim_t* sim, player_t* owner ) :
      pet_t( sim, owner, "water_elemental" )
  {
    action_list_str = "water_bolt";
  }
  virtual void init_base()
  {
    // Stolen from Priest's Shadowfiend
    attribute_base[ ATTR_STRENGTH  ] = 145;
    attribute_base[ ATTR_AGILITY   ] =  38;
    attribute_base[ ATTR_STAMINA   ] = 190;
    attribute_base[ ATTR_INTELLECT ] = 133;

    health_per_stamina = 7.5;
    mana_per_intellect = 5;
  }
  virtual void summon()
  {
    pet_t::summon();

    mage_t* o = cast_pet() -> owner -> cast_mage();

    o -> active_water_elemental = this;

    if ( o -> talents.improved_water_elemental )
    {
      for ( player_t* p = sim -> player_list; p; p = p -> next )
      {
        if ( p -> buffs.water_elemental == 0 ) p -> aura_gain( "Water Elemental Regen" );
        p -> buffs.water_elemental++;
      }
    }
  }
  virtual void dismiss()
  {
    pet_t::dismiss();

    mage_t* o = cast_pet() -> owner -> cast_mage();

    if ( o -> talents.improved_water_elemental )
    {
      for ( player_t* p = sim -> player_list; p; p = p -> next )
      {
        p -> buffs.water_elemental--;
        if ( p -> buffs.water_elemental == 0 ) p -> aura_loss( "Water Elemental Regen" );
      }
    }

    o -> active_water_elemental = 0;
  }
  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str )
  {
    if ( name == "water_bolt" ) return new water_bolt_t( this );

    return pet_t::create_action( name, options_str );
  }
};

// ==========================================================================
// Pet Mirror Image
// ==========================================================================

struct mirror_image_pet_t : public pet_t
{
  int num_images;
  int num_rotations;
  std::vector<action_t*> sequences;
  int sequence_finished;

  struct mirror_blast_t : public spell_t
  {
    action_t* next_in_sequence;

    mirror_blast_t( mirror_image_pet_t* mirror_image, action_t* nis ):
        spell_t( "mirror_blast", mirror_image, RESOURCE_MANA, SCHOOL_FIRE, TREE_FIRE ),
        next_in_sequence( nis )
    {
      base_cost         = 0;
      base_execute_time = 0;
      base_dd_min       = 92;
      base_dd_max       = 103;
      direct_power_mod  = 0.15;
      may_crit          = true;
      background        = true;
    }
    virtual void player_buff()
    {
      mirror_image_pet_t* p = ( mirror_image_pet_t* ) player;
      spell_t::player_buff();
      player_spell_power += player -> cast_pet() -> owner -> composite_spell_power( SCHOOL_FIRE ) / p -> num_images;
    }
    virtual void execute()
    {
      mirror_image_pet_t* p = ( mirror_image_pet_t* ) player;
      mage_t* o = p -> owner -> cast_mage();
      spell_t::execute();
      if ( o -> glyphs.mirror_image && result_is_hit() )
      {
        stack_winters_chill( this, 1.00 );
      }
      if( next_in_sequence ) 
      {
        next_in_sequence -> schedule_execute();
      }
      else
      {
        mirror_image_pet_t* mi = (mirror_image_pet_t*) player;
        mi -> sequence_finished++;
        if( mi -> sequence_finished == mi -> num_images ) mi -> dismiss();
      }
    }
  };

  struct mirror_bolt_t : public spell_t
  {
    action_t* next_in_sequence;

    mirror_bolt_t( mirror_image_pet_t* mirror_image, action_t* nis ):
        spell_t( "mirror_bolt", mirror_image, RESOURCE_MANA, SCHOOL_FROST, TREE_FROST ),
        next_in_sequence( nis )
    {
      base_cost         = 0;
      base_execute_time = 3.0;
      base_dd_min       = 163;
      base_dd_max       = 169;
      direct_power_mod  = 0.30;
      may_crit          = true;
      background        = true;
    }
    virtual void execute()
    {
      spell_t::execute();
      if( next_in_sequence ) 
      {
        next_in_sequence -> schedule_execute();
      }
      else
      {
        mirror_image_pet_t* mi = (mirror_image_pet_t*) player;
        mi -> sequence_finished++;
        if( mi -> sequence_finished == mi -> num_images ) mi -> dismiss();
      }
    }
    virtual void player_buff()
    {
      mirror_image_pet_t* p = ( mirror_image_pet_t* ) player;
      spell_t::player_buff();
      player_spell_power += player -> cast_pet() -> owner -> composite_spell_power( SCHOOL_FROST ) / p -> num_images;
    }
  };

  mirror_image_pet_t( sim_t* sim, player_t* owner ) :
    pet_t( sim, owner, "mirror_image", true /*guardian*/ ), num_images( 3 ), num_rotations( 4 ), sequence_finished( 0 )
  {}
  virtual void init_base()
  {
    // Stolen from Priest's Shadowfiend
    attribute_base[ ATTR_STRENGTH  ] = 145;
    attribute_base[ ATTR_AGILITY   ] =  38;
    attribute_base[ ATTR_STAMINA   ] = 190;
    attribute_base[ ATTR_INTELLECT ] = 133;

    health_per_stamina = 7.5;
    mana_per_intellect = 5;
  }
  virtual void init_actions()
  {
    for ( int i=0; i < num_images; i++ )
    {
      action_t* front=0;

      for ( int j=0; j < num_rotations; j++ )
      {
        front = new mirror_bolt_t ( this, front );
        front = new mirror_bolt_t ( this, front );
        front = new mirror_blast_t( this, front );
      }
      sequences.push_back( front );
    }
  }
  virtual void summon()
  {
    pet_t::summon();

    sequence_finished = 0;

    for ( int i=0; i < num_images; i++ )
    {
      sequences[ i ] -> schedule_execute();
    }
  }
  virtual void interrupt()
  {
    pet_t::interrupt();

    dismiss(); // FIXME! Interrupting them is too hard, just dismiss for now.
  }
};

// trigger_tier5_4pc ========================================================

static void trigger_tier5_4pc( spell_t* s )
{
  struct expiration_t : public event_t
  {
    expiration_t( sim_t* sim, player_t* p ) : event_t( sim, p )
    {
      name = "Tier5 Buff Expiration";
      player -> aura_gain( "Tier5 Buff" );
      player -> spell_power[ SCHOOL_MAX ] += 70;
      sim -> add_event( this, 6.0 );
    }
    virtual void execute()
    {
      player -> aura_loss( "Tier5 Buff" );
      player -> spell_power[ SCHOOL_MAX ] -= 70;
      player -> expirations.tier5_4pc = 0;
    }
  };

  mage_t* p = s -> player -> cast_mage();

  if ( p -> set_bonus.tier5_4pc() )
  {
    p -> procs.tier5_4pc -> occur();

    event_t*& e = p -> expirations.tier5_4pc;

    if ( e )
    {
      e -> reschedule( 6.0 );
    }
    else
    {
      e = new ( s -> sim ) expiration_t( s -> sim, p );
    }
  }
}

// trigger_tier8_2pc ========================================================

static void trigger_tier8_2pc( spell_t* s )
{
  mage_t* p = s -> player -> cast_mage();

  if ( ! p -> set_bonus.tier8_2pc() )
    return;

  // http://ptr.wowhead.com/?spell=64867
  if ( ! p -> rngs.tier8_2pc -> roll( 0.25 ) )
    return;

  struct expiration_t : public event_t
  {
    expiration_t( sim_t* sim, player_t* p ) : event_t( sim, p )
    {
      name = "Tier 8 2 Piece Expiration";
      player -> aura_gain( "Tier 8 2 Piece" );
      player -> spell_power[ SCHOOL_MAX ] += 350;
      player -> cooldowns.tier8_2pc        = sim -> current_time + 45;
      player -> buffs.tier8_2pc            = 1;
      sim -> add_event( this, 15.0 );
    }
    virtual void execute()
    {
      player -> aura_loss( "Tier 8 2 Piece" );
      player -> spell_power[ SCHOOL_MAX ] -= 350;
      player -> buffs.tier8_2pc            = 0;
      player -> expirations.tier8_2pc      = 0;
    }
  };

  if ( s -> sim -> cooldown_ready( p -> cooldowns.tier8_2pc ) )
  {
    p -> procs.tier8_2pc -> occur();

    event_t*& e = p -> expirations.tier8_2pc;

    if ( e )
    {
      e -> reschedule( 15.0 );
    }
    else
    {
      e = new ( s -> sim ) expiration_t( s -> sim, p );
    }
  }
}

// trigger_tier8_4pc ========================================================

static bool trigger_tier8_4pc( spell_t* s )
{
  mage_t* p = s -> player -> cast_mage();

  if ( ! p -> set_bonus.tier8_4pc() )
    return false;

  if ( ! p -> rngs.tier8_4pc -> roll( 0.10 ) )
    return false;

  p -> procs.tier8_4pc -> occur();

  return true;
}

// trigger_ignite ===========================================================

static void trigger_ignite( spell_t* s,
                            double   dmg )
{
  if ( s -> school != SCHOOL_FIRE &&
       s -> school != SCHOOL_FROSTFIRE ) return;

  mage_t* p = s -> player -> cast_mage();

  if ( p -> talents.ignite == 0 ) return;

  struct ignite_t : public mage_spell_t
  {
    ignite_t( player_t* player ) :
        mage_spell_t( "ignite", player, SCHOOL_FIRE, TREE_FIRE )
    {
      base_tick_time = 2.0;
      num_ticks      = 2;
      trigger_gcd    = 0;
      background     = true;
      proc           = true;
      may_resist     = false;
      reset();
    }
    virtual void target_debuff( int dmg_type ) {}
    virtual void player_buff() {}
    virtual void tick()
    {
      mage_t* p = player -> cast_mage();
      mage_spell_t::tick();
      if( sim -> P320 && p -> talents.empowered_fire )
      {
        if( p -> rng_empowered_fire -> roll( p -> talents.empowered_fire / 3.0 ) )
        {
          p -> resource_gain( RESOURCE_MANA, p -> resource_base[ RESOURCE_MANA ] * 0.01, p -> gains_empowered_fire );
        }
      }
    }
  };

  double ignite_dmg = dmg * p -> talents.ignite * 0.08;

  if ( s -> sim -> merge_ignite )
  {
    s -> result = RESULT_HIT;
    s -> assess_damage( ( ignite_dmg ), DMG_OVER_TIME );
    s -> update_stats( DMG_OVER_TIME );
    s -> result = RESULT_CRIT;
    return;
  }

  if ( ! p -> active_ignite ) p -> active_ignite = new ignite_t( p );

  if ( p -> active_ignite -> ticking )
  {
    p -> procs_deferred_ignite -> occur();

    if ( s -> sim -> debug ) log_t::output( s -> sim, "Player %s defers Ignite.", p -> name() );

    int num_ticks = p -> active_ignite -> num_ticks;
    int remaining_ticks = num_ticks - p -> active_ignite -> current_tick;

    ignite_dmg += p -> active_ignite -> base_td * remaining_ticks;

    p -> active_ignite -> cancel();
  }

  p -> active_ignite -> base_td = ignite_dmg / 2.0;
  p -> active_ignite -> schedule_tick();
}

// trigger_burnout ==========================================================

static void trigger_burnout( spell_t* s )
{
  mage_t* p = s -> player -> cast_mage();

  if ( ! p -> talents.burnout )
    return;

  if ( ! s -> resource_consumed )
    return;

  p -> resource_loss( RESOURCE_MANA, s -> resource_consumed * p -> talents.burnout * 0.01 );
}

// trigger_master_of_elements ===============================================

static void trigger_master_of_elements( spell_t* s, double adjust )
{
  mage_t* p = s -> player -> cast_mage();

  if ( s -> resource_consumed == 0 )
    return;

  if ( p -> talents.master_of_elements == 0 )
    return;

  p -> resource_gain( RESOURCE_MANA, adjust * s -> base_cost * p -> talents.master_of_elements * 0.10, p -> gains_master_of_elements );
}

// trigger_combustion =======================================================

static void trigger_combustion( spell_t* s )
{
  if ( s -> school != SCHOOL_FIRE &&
       s -> school != SCHOOL_FROSTFIRE ) return;

  mage_t* p = s -> player -> cast_mage();

  if ( ! p -> buffs_combustion -> check() ) return;

  if( s -> result_is_hit() )
  {
    p -> buffs_combustion -> current_value++;

    if ( s -> result == RESULT_CRIT ) 
    {
      p -> buffs_combustion -> decrement();
    }
  }
}

// trigger_arcane_concentration =============================================

static void trigger_arcane_concentration( spell_t* s )
{
  mage_t* p = s -> player -> cast_mage();

  if ( s -> dual ) return;

  p -> buffs_clearcasting -> trigger();
}

// trigger_frostbite ========================================================

static void trigger_frostbite( spell_t* s )
{
  if ( s -> school != SCHOOL_FROST &&
       s -> school != SCHOOL_FROSTFIRE ) return;

  struct expiration_t : public event_t
  {
    expiration_t( sim_t* sim ) : event_t( sim )
    {
      name = "Frozen Expiration";
      if ( sim -> log ) log_t::output( sim, "Target %s gains Frozen", sim -> target -> name() );
      sim -> target -> debuffs.frozen = sim -> current_time;
      sim -> add_event( this, 5.0 );
    }
    virtual void execute()
    {
      if ( sim -> log ) log_t::output( sim, "Target %s loses Frozen", sim -> target -> name() );
      sim -> target -> debuffs.frozen = 0;
      sim -> target -> expirations.frozen = 0;
    }
  };

  mage_t*   p = s -> player -> cast_mage();
  target_t* t = s -> sim -> target;

  int level_diff = t -> level - p -> level;

  if ( ( level_diff <= 1 ) && p -> rng_frostbite -> roll( p -> talents.frostbite * 0.05 ) )
  {
    event_t*& e = t -> expirations.frozen;

    if ( e )
    {
      e -> reschedule( 5.0 );
    }
    else
    {
      e = new ( s -> sim ) expiration_t( s -> sim );
    }
  }
}

// trigger_winters_grasp ========================================================

static void trigger_winters_grasp( spell_t* s )
{
  if ( s -> school != SCHOOL_FROST &&
       s -> school != SCHOOL_FROSTFIRE ) return;

  struct expiration_t : public event_t
  {
    expiration_t( sim_t* sim ) : event_t( sim )
    {
      name = "Winters Grasp Expiration";
      if ( sim -> log ) log_t::output( sim, "Target %s gains Winters Grasp", sim -> target -> name() );
      sim -> target -> debuffs.frozen = sim -> current_time;
      sim -> target -> debuffs.winters_grasp = 1;
      sim -> add_event( this, 5.0 );
    }
    virtual void execute()
    {
      if ( sim -> log ) log_t::output( sim, "Target %s loses Winters Grasp", sim -> target -> name() );
      sim -> target -> debuffs.frozen = 0;
      sim -> target -> debuffs.winters_grasp = 0;
      sim -> target -> expirations.winters_grasp = 0;
    }
  };

  mage_t* p = s -> player -> cast_mage();

  if ( p -> rng_winters_grasp -> roll( p -> talents.winters_grasp * 0.05 ) )
  {
    event_t*& e = s -> sim -> target -> expirations.winters_grasp;

    if ( e )
    {
      e -> reschedule( 5.0 );
    }
    else
    {
      e = new ( s -> sim ) expiration_t( s -> sim );
    }
  }
}

// clear_fingers_of_frost =========================================================

static void clear_fingers_of_frost( spell_t* s )
{
  mage_t* p = s -> player -> cast_mage();

  p -> buffs_ghost_charge -> expire();

  if ( s -> may_crit && p -> buffs_fingers_of_frost -> check() )
  {
    p -> buffs_fingers_of_frost -> decrement();

    if( ! p -> buffs_fingers_of_frost -> check() )
    {
      if ( p -> gcd_ready < p -> sim -> current_time )
      {
        double value = p -> rng_ghost_charge -> roll( p -> ghost_charge_pct ) ? +1.0 : -1.0;

        p -> buffs_ghost_charge -> start( 1, value );
      }
    }
  }
}

// trigger_fingers_of_frost =======================================================

static void trigger_fingers_of_frost( spell_t* s )
{
  mage_t* p = s -> player -> cast_mage();

  p -> buffs_fingers_of_frost -> trigger( 2 );
}

// trigger_brain_freeze ===========================================================

static void trigger_brain_freeze( spell_t* s )
{
  if ( s -> school != SCHOOL_FROST &&
       s -> school != SCHOOL_FROSTFIRE ) return;

  mage_t* p = s -> player -> cast_mage();

  p -> buffs_brain_freeze -> trigger();
}

// consume_brain_freeze ============================================================

static void consume_brain_freeze( spell_t* s )
{
  mage_t* p = s -> player -> cast_mage();

  if ( p -> buffs_brain_freeze -> check() )
    if ( ! trigger_tier8_4pc( s ) )
      p -> buffs_brain_freeze -> expire();
}

// trigger_hot_streak ==============================================================

static void trigger_hot_streak( spell_t* s )
{
  sim_t* sim = s -> sim;
  mage_t*  p = s -> player -> cast_mage();

  if ( p -> talents.hot_streak )
  {
    int result = s -> result;

    if ( sim -> smooth_rng && s -> result_is_hit() )
    {
      // Decouple Hot Streak proc from actual crit to reduce wild swings during RNG smoothing.
      result = sim -> rng -> roll( s -> total_crit() ) ? RESULT_CRIT : RESULT_HIT;
    }
    if ( result == RESULT_CRIT )
    {
      p -> buffs_hot_streak_crits -> increment();
      
      if( p -> buffs_hot_streak_crits -> stack() == 2 )
      {
        p -> buffs_hot_streak -> trigger();
        p -> buffs_hot_streak_crits -> expire();
      }
    }
    else
    {
      p -> buffs_hot_streak_crits -> expire();
    }
  }
}

// stack_improved_scorch =========================================================

static void stack_improved_scorch( spell_t* s )
{
  mage_t* p = s -> player -> cast_mage();

  if ( ! p -> talents.improved_scorch ) return;

  struct expiration_t : public event_t
  {
    expiration_t( sim_t* sim ) : event_t( sim )
    {
      name = "Improved Scorch Expiration";
      sim -> add_event( this, 30.0 );
    }
    virtual void execute()
    {
      if ( sim -> log ) log_t::output( sim, "%s loses Improved Scorch", sim -> target -> name() );
      sim -> target -> debuffs.improved_scorch = 0;
      sim -> target -> expirations.improved_scorch = 0;
    }
  };

  if ( p -> rng_improved_scorch -> roll( p -> talents.improved_scorch / 3.0 ) )
  {
    target_t* t = s -> sim -> target;

    if ( t -> debuffs.improved_scorch < 5 )
    {
      if ( p -> glyphs.improved_scorch )
      {
        t -> debuffs.improved_scorch += 5;
        if ( t -> debuffs.improved_scorch > 5 ) t -> debuffs.improved_scorch = 5;
      }
      else
      {
        t -> debuffs.improved_scorch += 1;
      }
      if ( s -> sim -> log ) log_t::output( s -> sim, "%s gains Improved Scorch %d", t -> name(), t -> debuffs.improved_scorch );
    }

    event_t*& e = t -> expirations.improved_scorch;

    if ( e )
    {
      e -> reschedule( 30.0 );
    }
    else
    {
      e = new ( s -> sim ) expiration_t( s -> sim );
    }
  }
}

// trigger_missile_barrage =========================================================

static void trigger_missile_barrage( spell_t* s )
{
  mage_t* p = s -> player -> cast_mage();

  if ( p -> talents.missile_barrage )
  {
    p -> buffs_missile_barrage -> trigger();
  }
}

// trigger_replenishment ===========================================================

static void trigger_replenishment( spell_t* s )
{
  mage_t* p = s -> player -> cast_mage();

  if ( ! p -> talents.improved_water_elemental )
    return;

  if ( ! p -> rng_improved_water_elemental -> roll( p -> talents.improved_water_elemental / 3.0 ) )
    return;

  p -> trigger_replenishment();
}

// trigger_incanters_absorption ====================================================

static void trigger_incanters_absorption( mage_t* p,
                                          double damage )
{
  if( ! p -> talents.incanters_absorption )
    return;

  buff_t* buff = p -> buffs_incanters_absorption;

  double bonus_spell_power = damage * p -> talents.incanters_absorption * 0.05;

  if( buff -> check() )
  {
    bonus_spell_power += buff -> value() * buff -> remains() / buff -> duration;
  }

  double bonus_max = p -> resource_max[ RESOURCE_HEALTH ] * 0.05;

  if( bonus_spell_power > bonus_max ) bonus_spell_power = bonus_max;

  buff -> increment( 1, bonus_spell_power );
}

// target_is_frozen ========================================================

static int target_is_frozen( mage_t* p )
{
  if( p -> buffs_fingers_of_frost -> may_react() ||
      p -> sim -> time_to_think( p -> sim -> target -> debuffs.frozen ) )
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

// =========================================================================
// Mage Spell
// =========================================================================

// mage_spell_t::parse_options =============================================

void mage_spell_t::parse_options( option_t*          options,
                                  const std::string& options_str )
{
  option_t base_options[] =
    {
      { "dps",          OPT_BOOL, &dps_rotation },
      { "dpm",          OPT_BOOL, &dpm_rotation },
      { "arcane_power", OPT_BOOL, &arcane_power },
      { "icy_veins",    OPT_BOOL, &icy_veins    },
      { NULL, OPT_UNKNOWN, NULL }
    };
  std::vector<option_t> merged_options;
  spell_t::parse_options( merge_options( merged_options, options, base_options ), options_str );
}

// mage_spell_t::ready =====================================================

bool mage_spell_t::ready()
{
  mage_t* p = player -> cast_mage();

  if ( dps_rotation )
    if ( p -> rotation.current != ROTATION_DPS )
      return false;

  if ( dpm_rotation )
    if ( p -> rotation.current != ROTATION_DPM )
      return false;

  if ( arcane_power )
    if ( ! p -> buffs_arcane_power -> check() )
      return false;

  if ( icy_veins )
    if ( ! p -> buffs_icy_veins -> check() )
      return false;

  return spell_t::ready();
}

// mage_spell_t::cost ======================================================

double mage_spell_t::cost() SC_CONST
{
  mage_t* p = player -> cast_mage();
  if ( p -> buffs_clearcasting -> check() ) return 0;
  double c = spell_t::cost();
  if ( p -> buffs_arcane_power -> check() ) c += base_cost * 0.20;
  return c;
}

// mage_spell_t::haste =====================================================

double mage_spell_t::haste() SC_CONST
{
  mage_t* p = player -> cast_mage();
  double h = spell_t::haste();
  if ( p -> buffs_icy_veins -> up() ) h *= 1.0 / ( 1.0 + 0.20 );
  if ( p -> talents.netherwind_presence )
  {
    h *= 1.0 / ( 1.0 + p -> talents.netherwind_presence * 0.02 );
  }
  return h;
}

// mage_spell_t::execute ===================================================

void mage_spell_t::execute()
{
  mage_t* p = player -> cast_mage();

  p -> uptimes_dps_rotation -> update( p -> rotation.current == ROTATION_DPS );
  p -> uptimes_dpm_rotation -> update( p -> rotation.current == ROTATION_DPM );
  p -> uptimes.tier8_2pc    -> update( p -> buffs.tier8_2pc == 1 );

  spell_t::execute();

  if ( result_is_hit() )
  {
    clear_fingers_of_frost( this );
    trigger_arcane_concentration( this );
    trigger_frostbite( this );
    trigger_winters_grasp( this );
    stack_winters_chill( this, p -> talents.winters_chill / 3.0 );

    if ( result == RESULT_CRIT )
    {
      trigger_burnout( this );
      trigger_ignite( this, direct_dmg );
      trigger_master_of_elements( this, 1.0 );
      trigger_tier5_4pc( this );
    }
  }
  trigger_combustion( this );
  trigger_brain_freeze( this );
}

// mage_spell_t::consume_resource ==========================================

void mage_spell_t::consume_resource()
{
  mage_t* p = player -> cast_mage();
  spell_t::consume_resource();
  if ( p -> buffs_clearcasting -> up() )
  {
    // Treat the savings like a mana gain.
    double amount = spell_t::cost();
    if ( amount > 0 )
    {
      p -> buffs_clearcasting -> expire();
      p -> gains_clearcasting -> add( amount );
    }
  }
}

// mage_spell_t::player_buff ===============================================

void mage_spell_t::player_buff()
{
  mage_t* p = player -> cast_mage();

  spell_t::player_buff();

  if ( p -> talents.molten_fury )
  {
    if ( sim -> target -> health_percentage() < 35 )
    {
      player_multiplier *= 1.0 + p -> talents.molten_fury * 0.06;
    }
  }

  if( may_torment && sim -> target -> debuffs.snared() )
  {
    player_multiplier *= 1.0 + p -> talents.torment_the_weak * 0.04;
  }

  if ( school == SCHOOL_ARCANE )
  {
    int ab_stack = p ->  buffs_arcane_blast -> stack();

    player_multiplier *= 1.0 + ab_stack * ( 0.15 + ( p -> glyphs.arcane_blast ? 0.03 : 0.00 ) );

    for ( int i=0; i < 4; i++ )
    {
      p -> uptimes_arcane_blast[ i ] -> update( i == ab_stack );
    }
  }
  else if ( school == SCHOOL_FIRE ||
            school == SCHOOL_FROSTFIRE )
  {
    player_crit += ( p -> buffs_combustion -> value() * 0.10 );
  }
  if ( p -> talents.shatter && may_crit )
  {
    if ( sim -> target -> debuffs.frozen )
    {
      player_crit += p -> talents.shatter * 0.5/3;
    }
    else if ( p -> talents.fingers_of_frost )
    {
      if( p -> buffs_fingers_of_frost -> up() )
      {
        player_crit += p -> talents.shatter * 0.5/3;
      }
      else if( time_to_execute == 0 )
      {
        if( p -> buffs_ghost_charge -> value() > 0 )
        {
          player_crit += p -> talents.shatter * 0.5/3;
        }
      }
    }
  }

  if ( p -> buffs_arcane_power -> up() ) player_multiplier *= 1.20;

  if ( p -> talents.playing_with_fire )
  {
    player_multiplier *= 1.0 + p -> talents.playing_with_fire * 0.01;
  }

  if ( p -> buffs_clearcasting -> up() && ! dual )
  {
    player_crit += p -> talents.arcane_potency * 0.15;
  }

  if ( p -> buffs_molten_armor -> up() )
  {
    double spirit_contribution = p -> glyphs.molten_armor ? 0.55 : 0.35;

    if( p -> set_bonus.tier9_2pc() ) spirit_contribution += 0.15;

    player_crit += p -> spirit() * spirit_contribution / p -> rating.spell_crit;
  }

  if ( p -> buffs.focus_magic_feedback )
  {
    player_crit += 0.03;
  }
  p -> uptimes_focus_magic_feedback -> update( p -> buffs.focus_magic_feedback != 0 );

  if ( sim -> debug )
    log_t::output( sim, "mage_spell_t::player_buff: %s hit=%.2f crit=%.2f power=%.2f penetration=%.0f mult=%.2f",
                   name(), player_hit, player_crit, player_spell_power, player_penetration, player_multiplier );
}

// =========================================================================
// Mage Spells
// =========================================================================

// Arcane Barrage Spell ====================================================

struct arcane_barrage_t : public mage_spell_t
{
  arcane_barrage_t( player_t* player, const std::string& options_str ) :
      mage_spell_t( "arcane_barrage", player, SCHOOL_ARCANE, TREE_ARCANE )
  {
    mage_t* p = player -> cast_mage();
    check_talent( p -> talents.arcane_barrage );

    option_t options[] =
      {
        { NULL, OPT_UNKNOWN, NULL }
      };
    parse_options( options, options_str );

    static rank_t ranks[] =
      {
        { 80, 3, 936, 1144, 0, 0.18 },
        { 70, 2, 709,  865, 0, 0.18 },
        { 60, 1, 386,  470, 0, 0.18 },
        { 0, 0, 0, 0, 0, 0 }
      };
    init_rank( ranks );

    base_execute_time = 0;
    may_crit          = true;
    direct_power_mod  = ( 2.5/3.5 );
    cooldown          = 3.0;
    base_cost        *= 1.0 - p -> talents.precision     * 0.01;
    base_cost        *= 1.0 - p -> talents.arcane_focus  * 0.01;
    base_cost        *= 1.0 - p -> glyphs.arcane_barrage * 0.20;
    base_cost        *= 1.0 - util_t::talent_rank( p -> talents.frost_channeling, 3, 0.04, 0.07, 0.10 );
    base_multiplier  *= 1.0 + p -> talents.arcane_instability * 0.01;
    base_crit        += p -> talents.arcane_instability * 0.01;
    base_hit         += p -> talents.arcane_focus * 0.01;

    base_crit_bonus_multiplier *= 1.0 + ( ( p -> talents.spell_power * 0.25 ) +
                                          ( p -> talents.burnout     * 0.10 ) +
                                          ( p -> set_bonus.tier7_4pc() ? 0.05 : 0.00 ) );

    may_torment = true;
  }

  virtual void execute()
  {
    mage_t* p = player -> cast_mage();
    mage_spell_t::execute();
    if ( result_is_hit() ) trigger_missile_barrage( this );
    p -> buffs_arcane_blast -> expire();
  }
};

// Arcane Blast Spell =======================================================

struct arcane_blast_t : public mage_spell_t
{
  int max_buff;

  arcane_blast_t( player_t* player, const std::string& options_str ) :
      mage_spell_t( "arcane_blast", player, SCHOOL_ARCANE, TREE_ARCANE ), max_buff( 0 )
  {
    mage_t* p = player -> cast_mage();

    option_t options[] =
      {
        { "ap_burn", OPT_DEPRECATED, ( void* ) "arcane_power" },
        { "max",     OPT_INT,        &max_buff              },
        { NULL, OPT_UNKNOWN, NULL }
      };
    parse_options( options, options_str );

    static rank_t ranks[] =
      {
        { 80, 4, 1185, 1377, 0, 0.08 },
        { 76, 3, 1047, 1215, 0, 0.08 },
        { 71, 2, 897,  1041, 0, 0.08 },
        { 64, 1, 842,  978,  0, 195  },
        { 0, 0, 0, 0, 0, 0 }
      };
    init_rank( ranks );

    if( sim -> P320 ) base_cost *= 0.88;

    base_execute_time = 2.5;
    may_crit          = true;
    direct_power_mod  = ( 2.5/3.5 );
    base_cost        *= 1.0 - p -> talents.precision     * 0.01;
    base_cost        *= 1.0 - p -> talents.arcane_focus  * 0.01;
    base_cost        *= 1.0 - util_t::talent_rank( p -> talents.frost_channeling, 3, 0.04, 0.07, 0.10 );
    base_multiplier  *= 1.0 + p -> talents.spell_impact * 0.02;
    base_multiplier  *= 1.0 + p -> talents.arcane_instability * 0.01;
    base_crit        += p -> talents.arcane_instability * 0.01;
    base_crit        += p -> talents.arcane_impact * 0.02;
    base_crit        += p -> talents.incineration * 0.02;
    base_hit         += p -> talents.arcane_focus * 0.01;
    direct_power_mod += p -> talents.arcane_empowerment * 0.03;

    base_crit_bonus_multiplier *= 1.0 + ( ( p -> talents.spell_power * 0.25   ) +
                                          ( p -> talents.burnout     * 0.10   ) +
                                          ( p -> set_bonus.tier7_4pc() ? 0.05 : 0.00 ) );

    if ( p -> set_bonus.tier5_2pc() ) base_multiplier *= 1.05;
    if ( p -> set_bonus.tier9_4pc() ) base_crit       += 0.05;

    may_torment = true;
  }

  virtual double cost() SC_CONST
  {
    mage_t* p = player -> cast_mage();
    double c = mage_spell_t::cost();
    if ( c != 0 )
    {
      c += base_cost * p -> buffs_arcane_blast -> stack() * 2.00;
      if ( p -> set_bonus.tier5_2pc() ) c += base_cost * 0.05;
    }
    return c;
  }

  virtual void execute()
  {
    mage_t* p = player -> cast_mage();

    mage_spell_t::execute();

    if ( result_is_hit() )
    {
      trigger_missile_barrage( this );
      trigger_tier8_2pc( this );
    }

    p -> buffs_arcane_blast -> increment();
  }

  virtual bool ready()
  {
    mage_t* p = player -> cast_mage();

    if ( ! mage_spell_t::ready() )
      return false;

    if ( max_buff > 0 )
      if ( p -> buffs_arcane_blast -> check() >= max_buff )
        return false;

    return true;
  }
};

// Arcane Missiles Spell ====================================================

struct arcane_missiles_tick_t : public mage_spell_t
{
  bool clearcast;

  arcane_missiles_tick_t( player_t* player ) : mage_spell_t( "arcane_missiles", player, SCHOOL_ARCANE, TREE_ARCANE )
  {
    mage_t* p = player -> cast_mage();

    static rank_t ranks[] =
      {
        { 80, 13, 361, 362, 0, 0 },
        { 79, 13, 360, 360, 0, 0 },
        { 75, 12, 320, 320, 0, 0 },
        { 70, 11, 280, 280, 0, 0 },
        { 69, 10, 260, 260, 0, 0 },
        { 63,  9, 240, 240, 0, 0 },
        { 60,  8, 230, 230, 0, 0 },
        { 0, 0, 0, 0, 0, 0 }
      };
    init_rank( ranks );

    dual       = true;
    background = true;
    may_crit   = true;

    direct_power_mod  = 1.0 / 3.5; // bonus per missle

    base_multiplier  *= 1.0 + p -> talents.arcane_instability * 0.01;
    base_crit        += p -> talents.arcane_instability * 0.01;
    base_hit         += p -> talents.arcane_focus * 0.01;
    direct_power_mod += p -> talents.arcane_empowerment * 0.03;        // bonus per missle

    base_crit_bonus_multiplier *= 1.0 + ( ( p -> talents.spell_power * 0.25 ) +
                                          ( p -> talents.burnout     * 0.10 ) +
                                          ( p -> glyphs.arcane_missiles ? 0.25 : 0.00 ) +
                                          ( p -> set_bonus.tier7_4pc()  ? 0.05 : 0.00 ) );

    if ( p -> set_bonus.tier6_4pc() ) base_multiplier *= 1.05;
    if ( p -> set_bonus.tier9_4pc() ) base_crit       += 0.05;

    clearcast = false;
    may_torment = true;
  }

  virtual void player_buff()
  {
    mage_t* p = player -> cast_mage();
    mage_spell_t::player_buff();
    if ( clearcast )
    {
      player_crit += p -> talents.arcane_potency * 0.15;
    }
  }

  virtual void execute()
  {
    mage_spell_t::execute();

    tick_dmg = direct_dmg;
    update_stats( DMG_OVER_TIME );

    if ( result == RESULT_CRIT )
    {
      trigger_master_of_elements( this, 0.20 );
      trigger_tier5_4pc( this );
    }
  }
};

struct arcane_missiles_t : public mage_spell_t
{
  arcane_missiles_tick_t* arcane_missiles_tick;
  int barrage;
  int clearcast;

  arcane_missiles_t( player_t* player, const std::string& options_str ) :
      mage_spell_t( "arcane_missiles", player, SCHOOL_ARCANE, TREE_ARCANE ), barrage( 0 ), clearcast( 0 )
  {
    mage_t* p = player -> cast_mage();

    option_t options[] =
      {
        { "barrage",    OPT_BOOL, &barrage    },
        { "clearcast",  OPT_BOOL, &clearcast  },
        { NULL, OPT_UNKNOWN, NULL }
      };
    parse_options( options, options_str );

    harmful        = false;
    channeled      = true;
    num_ticks      = 5;
    base_tick_time = 1.0;

    base_cost  = 0.31 * p -> resource_base[ RESOURCE_MANA ];
    base_cost *= 1.0 - p -> talents.precision     * 0.01;
    base_cost *= 1.0 - p -> talents.arcane_focus  * 0.01;
    base_cost *= 1.0 - util_t::talent_rank( p -> talents.frost_channeling, 3, 0.04, 0.07, 0.10 );

    arcane_missiles_tick = new arcane_missiles_tick_t( p );
  }

  virtual void execute()
  {
    mage_t* p = player -> cast_mage();

    if ( p -> buffs_missile_barrage -> up() )
    {
      base_tick_time = 0.5;

      if ( ! trigger_tier8_4pc( this ) )
      {
        p -> buffs_missile_barrage -> expire();
      }
    }
    else
    {
      base_tick_time = 1.0;
    }

    arcane_missiles_tick -> clearcast = p -> buffs_clearcasting -> up();

    mage_spell_t::execute();
  }

  virtual void tick()
  {
    if ( sim -> debug ) log_t::output( sim, "%s ticks (%d of %d)", name(), current_tick, num_ticks );
    arcane_missiles_tick -> execute();
    update_time( DMG_OVER_TIME );
  }

  virtual void last_tick()
  {
    mage_t* p = player -> cast_mage();
    mage_spell_t::last_tick();
    p -> buffs_arcane_blast -> expire();
  }

  virtual bool ready()
  {
    mage_t* p = player -> cast_mage();

    if ( ! mage_spell_t::ready() )
      return false;

    if ( barrage )
      if ( ! p -> buffs_missile_barrage -> may_react() )
        return false;

    if ( clearcast )
      if( !  p -> buffs_clearcasting -> may_react() )
        return false;

    return true;
  }
};

// Arcane Power Spell ======================================================

struct arcane_power_t : public mage_spell_t
{
  arcane_power_t( player_t* player, const std::string& options_str ) :
      mage_spell_t( "arcane_power", player, SCHOOL_ARCANE, TREE_ARCANE )
  {
    mage_t* p = player -> cast_mage();
    check_talent( p -> talents.arcane_power );
    trigger_gcd = 0;
    cooldown = 120.0;
    cooldown *= 1.0 - p -> talents.arcane_flows * 0.15;
  }

  virtual void execute()
  {
    mage_t* p = player -> cast_mage();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    consume_resource();
    update_ready();
    p -> buffs_arcane_power -> start();
  }
};

// Slow Spell ================================================================

struct slow_t : public mage_spell_t
{
  struct expiration_t : public event_t
  {
    expiration_t( sim_t* sim ) : event_t( sim )
    {
      name = "Slow Expiration";
      if ( sim -> log ) log_t::output( sim, "Target %s gains Slow", sim -> target -> name() );
      sim -> target -> debuffs.slow = 1;
      sim -> add_event( this, 15.0 );
    }
    virtual void execute()
    {
      if ( sim -> log ) log_t::output( sim, "Target %s loses Slow", sim -> target -> name() );
      sim -> target -> debuffs.slow = 0;
    }
  };

  slow_t( player_t* player, const std::string& options_str ) :
      mage_spell_t( "slow", player, SCHOOL_ARCANE, TREE_ARCANE )
  {
    mage_t* p = player -> cast_mage();
    check_talent( p -> talents.slow );
    base_cost = p -> resource_base[ RESOURCE_MANA ] * 0.12;
  }

  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );
    consume_resource();
    update_ready();
    new ( sim ) expiration_t( sim );
  }

  virtual bool ready()
  {
    if ( ! mage_spell_t::ready() )
      return false;

    return( ! sim -> target -> debuffs.snared() );
  }
};

// Focus Magic Spell ========================================================

struct focus_magic_t : public mage_spell_t
{
  player_t* focus_magic_target;

  focus_magic_t( player_t* player, const std::string& options_str ) :
      mage_spell_t( "focus_magic", player, SCHOOL_ARCANE, TREE_ARCANE )
  {
    mage_t* p = player -> cast_mage();
    check_talent( p -> talents.focus_magic );

    std::string target_str = p -> focus_magic_target_str;
    option_t options[] =
      {
        { "target", OPT_STRING, &target_str },
        { NULL, OPT_UNKNOWN, NULL }
      };
    parse_options( options, options_str );

    if ( target_str.empty() )
    {
      // If no target specified, assume 100% up-time by forcing "buffs.focus_magic_feedback = 1"
      focus_magic_target = p;
    }
    else
    {
      focus_magic_target = sim -> find_player( target_str );

      assert ( focus_magic_target != 0 );
      assert ( focus_magic_target != p );
    }
  }

  virtual void execute()
  {
    mage_t* p = player -> cast_mage();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    if ( focus_magic_target == p )
    {
      if ( sim -> log ) log_t::output( sim, "%s grants SomebodySomewhere Focus Magic", p -> name() );
      p -> buffs.focus_magic_feedback = 1;
    }
    else
    {
      if ( sim -> log ) log_t::output( sim, "%s grants %s Focus Magic", p -> name(), focus_magic_target -> name() );
      focus_magic_target -> buffs.focus_magic = p;
    }
  }

  virtual bool ready()
  {
    mage_t* p = player -> cast_mage();

    if ( focus_magic_target == p )
    {
      return p -> buffs.focus_magic_feedback == 0;
    }
    else
    {
      return focus_magic_target -> buffs.focus_magic == 0;
    }
  }
};

// Evocation Spell ==========================================================

struct evocation_t : public mage_spell_t
{
  evocation_t( player_t* player, const std::string& options_str ) :
      mage_spell_t( "evocation", player, SCHOOL_ARCANE, TREE_ARCANE )
  {
    mage_t* p = player -> cast_mage();

    base_tick_time = 2.0;
    num_ticks      = 4;
    channeled      = true;
    cooldown       = 240;
    cooldown      -= p -> talents.arcane_flows * 60.0;
    harmful        = false;

    if ( p -> set_bonus.tier6_2pc() ) num_ticks++;
  }

  virtual void tick()
  {
    mage_t* p = player -> cast_mage();
    double mana = p -> resource_max[ RESOURCE_MANA ] * 0.15;
    p -> resource_gain( RESOURCE_MANA, mana, p -> gains_evocation );
  }

  virtual bool ready()
  {
    if ( ! mage_spell_t::ready() )
      return false;

    return ( player -> resource_current[ RESOURCE_MANA ] /
             player -> resource_max    [ RESOURCE_MANA ] ) < 0.30;
  }
};

// Presence of Mind Spell ===================================================

struct presence_of_mind_t : public mage_spell_t
{
  action_t* fast_action;

  presence_of_mind_t( player_t* player, const std::string& options_str ) :
      mage_spell_t( "presence_of_mind", player, SCHOOL_ARCANE, TREE_ARCANE )
  {
    mage_t* p = player -> cast_mage();
    check_talent( p -> talents.presence_of_mind );

    cooldown = 120.0;
    if ( p -> set_bonus.tier4_4pc() ) cooldown -= 24.0;
    cooldown *= 1.0 - p -> talents.arcane_flows * 0.15;

    if ( options_str.empty() )
    {
      log_t::output( sim, "simcraft: The presence_of_mind action must be coupled with a second action." );
      exit( 0 );
    }

    std::string spell_name    = options_str;
    std::string spell_options = "";

    std::string::size_type cut_pt = spell_name.find_first_of( "," );

    if ( cut_pt != spell_name.npos )
    {
      spell_options = spell_name.substr( cut_pt + 1 );
      spell_name    = spell_name.substr( 0, cut_pt );
    }

    fast_action = p -> create_action( spell_name.c_str(), spell_options.c_str() );
    fast_action -> base_execute_time = 0;
    fast_action -> background = true;
  }

  virtual void execute()
  {
    mage_t* p = player -> cast_mage();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    p -> aura_gain( "Presence of Mind" );
    p -> last_foreground_action = fast_action;
    p -> spell_crit += p -> talents.arcane_potency * 0.15;
    fast_action -> execute();
    p -> spell_crit -= p -> talents.arcane_potency * 0.15;
    p -> aura_loss( "Presence of Mind" );
    update_ready();
  }

  virtual bool ready()
  {
    return( mage_spell_t::ready() && fast_action -> ready() );
  }
};

// Fire Ball Spell =========================================================

struct fire_ball_t : public mage_spell_t
{
  int brain_freeze;
  int frozen;
  int ghost_charge;

  fire_ball_t( player_t* player, const std::string& options_str ) :
    mage_spell_t( "fire_ball", player, SCHOOL_FIRE, TREE_FIRE ), 
    brain_freeze( 0 ), frozen( -1 ), ghost_charge( -1 )
  {
    mage_t* p = player -> cast_mage();

    option_t options[] =
      {
        { "brain_freeze", OPT_BOOL, &brain_freeze },
        { "frozen",       OPT_BOOL, &frozen       },
        { "ghost_charge", OPT_BOOL, &ghost_charge },
        { NULL, OPT_UNKNOWN, NULL }
      };
    parse_options( options, options_str );

    static rank_t ranks[] =
      {
        { 80, 16, 898, 1143, 29, 0.19 },
        { 78, 16, 888, 1132, 29, 0.19 },
        { 74, 15, 783,  997, 25, 0.19 },
        { 70, 14, 717,  913, 23, 0.19 },
        { 66, 13, 633,  805, 21, 425  },
        { 62, 12, 596,  760, 19, 410  },
        { 60, 11, 561,  715, 18, 395  },
        { 0, 0, 0, 0, 0, 0 }
      };
    init_rank( ranks );

    may_crit          = true;
    base_execute_time = 3.5;
    direct_power_mod  = base_execute_time / 3.5;
    base_tick_time    = 2.0;
    num_ticks         = 4;
    tick_power_mod    = 0;
    base_cost        *= 1.0 - p -> talents.precision     * 0.01;
    base_cost        *= 1.0 - util_t::talent_rank( p -> talents.frost_channeling, 3, 0.04, 0.07, 0.10 );
    base_execute_time -= p -> talents.improved_fire_ball * 0.1;
    base_multiplier   *= 1.0 + p -> talents.fire_power * 0.02;
    base_multiplier   *= 1.0 + p -> talents.arcane_instability * 0.01;
    base_multiplier   *= 1.0 + p -> talents.spell_impact * 0.02;
    base_crit         += p -> talents.arcane_instability * 0.01;
    base_crit         += p -> talents.critical_mass * 0.02;
    base_crit         += p -> talents.pyromaniac * 0.01;
    direct_power_mod  += p -> talents.empowered_fire * 0.05;

    base_crit_bonus_multiplier *= 1.0 + ( ( p -> talents.spell_power * 0.25 ) +
                                          ( p -> talents.burnout     * 0.10 ) +
                                          ( p -> set_bonus.tier7_4pc() ? 0.05 : 0.00 ) );

    base_crit += p -> talents.improved_scorch * 0.01;

    if ( p -> set_bonus.tier6_4pc() ) base_multiplier *= 1.05;
    if ( p -> set_bonus.tier9_4pc() ) base_crit       += 0.05;

    if ( p -> glyphs.fire_ball ) 
    {
      base_crit += 0.05;
      num_ticks = 0;
    }

    may_torment = true;
  }

  virtual double cost() SC_CONST
  {
    mage_t* p = player -> cast_mage();
    if ( p -> buffs_brain_freeze -> check() ) return 0;
    return mage_spell_t::cost();
  }

  virtual double execute_time() SC_CONST
  {
    mage_t* p = player -> cast_mage();
    if ( p -> buffs_brain_freeze -> up() ) return 0;
    return mage_spell_t::execute_time();
  }

  virtual void execute()
  {
    mage_spell_t::execute();
    duration_ready = 0; // Never wait for DoT component to finish.
    if ( result_is_hit() )
    {
      trigger_missile_barrage( this );
      trigger_tier8_2pc( this );
    }
    trigger_hot_streak( this );
    consume_brain_freeze( this );
  }

  virtual bool ready()
  {
    mage_t* p = player -> cast_mage();

    if ( ! mage_spell_t::ready() )
      return false;

    if ( brain_freeze )
      if ( ! p -> buffs_brain_freeze -> may_react() )
        return false;

    if ( ghost_charge != -1 )
      if( ghost_charge != ( p -> buffs_ghost_charge -> check() ? 1 : 0 ) )
        return false;

    if ( frozen != -1 )
      if ( frozen != target_is_frozen( p ) )
        return false;

    return true;
  }
};

// Fire Blast Spell ========================================================

struct fire_blast_t : public mage_spell_t
{
  fire_blast_t( player_t* player, const std::string& options_str ) :
      mage_spell_t( "fire_blast", player, SCHOOL_FIRE, TREE_FIRE )
  {
    mage_t* p = player -> cast_mage();

    option_t options[] =
      {
        { NULL, OPT_UNKNOWN, NULL }
      };
    parse_options( options, options_str );

    static rank_t ranks[] =
      {
        { 80, 11, 925, 1095, 0, 0.21 },
        { 74, 10, 760,  900, 0, 0.21 },
        { 70,  9, 664,  786, 0, 465  },
        { 61,  8, 539,  637, 0, 400  },
        { 54,  7, 431,  509, 0, 340  },
        { 0, 0, 0, 0, 0, 0 }
      };
    init_rank( ranks );

    base_execute_time = 0;
    may_crit          = true;
    cooldown          = 8.0;
    direct_power_mod  = ( 1.5/3.5 );
    base_cost        *= 1.0 - p -> talents.precision     * 0.01;
    base_cost        *= 1.0 - util_t::talent_rank( p -> talents.frost_channeling, 3, 0.04, 0.07, 0.10 );
    cooldown         -= p -> talents.improved_fire_blast * 0.5;
    base_multiplier  *= 1.0 + p -> talents.fire_power * 0.02;
    base_multiplier  *= 1.0 + p -> talents.arcane_instability * 0.01;
    base_crit        += p -> talents.arcane_instability * 0.01;
    base_multiplier  *= 1.0 + p -> talents.spell_impact * 0.02;
    base_crit        += p -> talents.incineration * 0.02;
    base_crit        += p -> talents.critical_mass * 0.02;
    base_crit        += p -> talents.pyromaniac * 0.01;

    base_crit_bonus_multiplier *= 1.0 + ( ( p -> talents.spell_power * 0.25 ) +
                                          ( p -> talents.burnout     * 0.10 ) +
                                          ( p -> set_bonus.tier7_4pc() ? 0.05 : 0.00 ) );
  }
  virtual void execute()
  {
    mage_spell_t::execute();
    trigger_hot_streak( this );
  }
};

// Living Bomb Spell ========================================================

struct living_bomb_t : public mage_spell_t
{
  double explosion;

  living_bomb_t( player_t* player, const std::string& options_str ) :
      mage_spell_t( "living_bomb", player, SCHOOL_FIRE, TREE_FIRE ), explosion( 0 )
  {
    mage_t* p = player -> cast_mage();
    check_talent( p -> talents.living_bomb );

    option_t options[] =
      {
        { NULL, OPT_UNKNOWN, NULL }
      };
    parse_options( options, options_str );

    static rank_t ranks[] =
      {
        { 80, 3, 759, 759, 690, 0.22 },
        { 70, 2, 568, 568, 512, 0.22 },
        { 60, 1, 336, 336, 306, 0.22 },
        { 0, 0, 0, 0, 0, 0 }
      };
    init_rank( ranks );

    base_execute_time = 0;
    base_tick_time    = 3.0;
    num_ticks         = 4;
    direct_power_mod  = 0.40;
    tick_power_mod    = base_tick_time / 15;
    may_crit          = true;
    tick_may_crit     = p -> glyphs.living_bomb != 0;
    base_cost        *= 1.0 - p -> talents.precision     * 0.01;
    base_cost        *= 1.0 - util_t::talent_rank( p -> talents.frost_channeling, 3, 0.04, 0.07, 0.10 );
    base_multiplier  *= 1.0 + p -> talents.fire_power * 0.02;
    base_multiplier  *= 1.0 + p -> talents.arcane_instability * 0.01;
    base_crit        += p -> talents.arcane_instability * 0.01;
    base_crit        += p -> talents.critical_mass * 0.02;
    base_crit        += p -> talents.pyromaniac * 0.01;
    base_crit        += p -> talents.world_in_flames * 0.02;

    base_crit_bonus_multiplier *= 1.0 + ( ( p -> talents.spell_power * 0.25 ) +
                                          ( p -> talents.burnout     * 0.10 ) +
                                          ( p -> set_bonus.tier7_4pc() ? 0.05 : 0.00 ) );
  }

  // Odd thing to handle: The direct-damage comes at the last tick instead of the beginning of the spell.

  virtual void tick()
  {
    mage_spell_t::tick();
    if( sim -> P320 && tick_may_crit ) trigger_hot_streak( this );
  }

  virtual void last_tick()
  {
    target_debuff( DMG_DIRECT );
    calculate_result();
    if ( result_is_hit() )
    {
      calculate_direct_damage();
      direct_dmg = explosion;
      if ( direct_dmg > 0 )
      {
        assess_damage( direct_dmg, DMG_DIRECT );
        if ( result == RESULT_CRIT )
        {
          trigger_burnout( this );
          trigger_ignite( this, direct_dmg );
          trigger_master_of_elements( this, 1.0 );
        }
        if( sim -> P320 && tick_may_crit ) trigger_hot_streak( this );
      }
    }
    update_stats( DMG_DIRECT );
    mage_spell_t::last_tick();
  }

  virtual double calculate_direct_damage()
  {
    mage_spell_t::calculate_direct_damage();
    explosion = direct_dmg;
    direct_dmg = 0;
    return direct_dmg;
  }
};

// Pyroblast Spell ========================================================

struct pyroblast_t : public mage_spell_t
{
  int hot_streak;

  pyroblast_t( player_t* player, const std::string& options_str ) :
      mage_spell_t( "pyroblast", player, SCHOOL_FIRE, TREE_FIRE ), hot_streak( 0 )
  {
    mage_t* p = player -> cast_mage();
    check_talent( p -> talents.pyroblast );

    option_t options[] =
      {
        { "hot_streak", OPT_BOOL, &hot_streak },
        { NULL, OPT_UNKNOWN, NULL }
      };
    parse_options( options, options_str );

    static rank_t ranks[] =
      {
        { 80, 12, 1210, 1531, 75, 0.22 },
        { 77, 12, 1190, 1510, 75, 0.22 },
        { 73, 11, 1014, 1286, 64, 0.22 },
        { 70, 10,  939, 1191, 59, 500  },
        { 66,  9,  846, 1074, 52, 460  },
        { 60,  8,  708,  898, 45, 440  },
        { 0, 0, 0, 0, 0, 0 }
      };
    init_rank( ranks );

    base_execute_time = 5.0;
    base_tick_time    = 3.0;
    num_ticks         = 4;
    may_crit          = true;
    direct_power_mod  = 1.15;
    tick_power_mod    = 0.20 / num_ticks;
    base_cost        *= 1.0 - p -> talents.precision     * 0.01;
    base_cost        *= 1.0 - util_t::talent_rank( p -> talents.frost_channeling, 3, 0.04, 0.07, 0.10 );
    base_multiplier  *= 1.0 + p -> talents.fire_power * 0.02;
    base_multiplier  *= 1.0 + p -> talents.arcane_instability * 0.01;
    base_crit        += p -> talents.arcane_instability * 0.01;
    base_crit        += p -> talents.critical_mass * 0.02;
    base_crit        += p -> talents.pyromaniac * 0.01;
    base_crit        += p -> talents.world_in_flames * 0.02;

    base_crit_bonus_multiplier *= 1.0 + ( ( p -> talents.spell_power * 0.25 ) +
                                          ( p -> talents.burnout     * 0.10 ) +
                                          ( p -> set_bonus.tier7_4pc() ? 0.05 : 0.00 ) );
  }

  virtual void execute()
  {
    mage_t* p = player -> cast_mage();

    mage_spell_t::execute();

    if( p -> buffs_hot_streak -> check() )
      if ( ! trigger_tier8_4pc( this ) )
        p -> buffs_hot_streak -> expire();

    // When performing Hot Streak Pyroblasts, do not wait for DoT to complete.
    if ( hot_streak ) duration_ready=0;
  }

  virtual double execute_time() SC_CONST
  {
    mage_t* p = player -> cast_mage();
    if ( p -> buffs_hot_streak -> up() ) return 0;
    return mage_spell_t::execute_time();
  }

  virtual bool ready()
  {
    mage_t* p = player -> cast_mage();

    if ( ! mage_spell_t::ready() )
      return false;

    if ( hot_streak )
      if ( ! p -> buffs_hot_streak -> may_react() )
        return false;

    return true;
  }
};

// Scorch Spell ==========================================================

struct scorch_t : public mage_spell_t
{
  int debuff;

  scorch_t( player_t* player, const std::string& options_str ) :
      mage_spell_t( "scorch", player, SCHOOL_FIRE, TREE_FIRE ), debuff( 0 )
  {
    mage_t* p = player -> cast_mage();

    option_t options[] =
      {
        { "debuff",    OPT_BOOL, &debuff     },
        { NULL, OPT_UNKNOWN, NULL }
      };
    parse_options( options, options_str );

    static rank_t ranks[] =
      {
        { 80, 11, 382, 451, 0, 0.08 },
        { 78, 11, 376, 444, 0, 0.08 },
        { 73, 10, 321, 379, 0, 0.08 },
        { 70,  9, 305, 361, 0, 180  },
        { 64,  8, 269, 317, 0, 165  },
        { 58,  7, 233, 275, 0, 150  },
        { 0, 0, 0, 0, 0, 0 }
      };
    init_rank( ranks );

    base_execute_time = 1.5;
    may_crit          = true;
    direct_power_mod  = ( 1.5/3.5 );
    base_cost        *= 1.0 - p -> talents.precision     * 0.01;
    base_cost        *= 1.0 - util_t::talent_rank( p -> talents.frost_channeling, 3, 0.04, 0.07, 0.10 );
    base_multiplier  *= 1.0 + p -> talents.fire_power         * 0.02;
    base_multiplier  *= 1.0 + p -> talents.arcane_instability * 0.01;
    base_multiplier  *= 1.0 + p -> talents.spell_impact       * 0.02;

    base_crit        += p -> talents.arcane_instability * 0.01;
    base_crit        += p -> talents.incineration       * 0.02;
    base_crit        += p -> talents.critical_mass      * 0.02;
    base_crit        += p -> talents.pyromaniac         * 0.01;

    base_crit_bonus_multiplier *= 1.0 + ( ( p -> talents.spell_power * 0.25 ) +
                                          ( p -> talents.burnout     * 0.10 ) +
                                          ( p -> set_bonus.tier7_4pc() ? 0.05 : 0.00 ) );

    base_crit += p -> talents.improved_scorch * 0.01;

    if ( debuff ) check_talent( p -> talents.improved_scorch );
  }

  virtual void execute()
  {
    mage_spell_t::execute();
    if ( result_is_hit() ) stack_improved_scorch( this );
    trigger_hot_streak( this );
  }

  virtual bool ready()
  {
    if ( ! mage_spell_t::ready() )
      return false;

    if ( debuff )
    {
      target_t* t = sim -> target;

      if ( t -> debuffs.improved_shadow_bolt )
        return false;

      event_t* e = t -> expirations.improved_scorch;

      if ( e && sim -> current_time < ( e -> occurs() - 6.0 ) )
        return false;

      if ( ! e && t -> debuffs.improved_scorch > 0 ) // override scenario
        return false;
    }

    return true;
  }
};

// Combustion Spell =========================================================

struct combustion_t : public mage_spell_t
{
  combustion_t( player_t* player, const std::string& options_str ) :
      mage_spell_t( "combustion", player, SCHOOL_FIRE, TREE_FIRE )
  {
    mage_t* p = player -> cast_mage();
    check_talent( p -> talents.combustion );
    cooldown = 180;
  }

  virtual void execute()
  {
    mage_t* p = player -> cast_mage();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    p -> buffs_combustion -> start( 3, 0.0 );
    update_ready();
  }
};

// Frost Bolt Spell =========================================================

struct frost_bolt_t : public mage_spell_t
{
  int frozen;

  frost_bolt_t( player_t* player, const std::string& options_str ) :
    mage_spell_t( "frost_bolt", player, SCHOOL_FROST, TREE_FROST ), frozen(-1)
  {
    mage_t* p = player -> cast_mage();

    travel_speed = 21.0; // set before options to allow override

    option_t options[] =
      {
        { "frozen", OPT_BOOL, &frozen },
        { NULL, OPT_UNKNOWN, NULL }
      };
    parse_options( options, options_str );

    double pct = sim -> P320 ? 0.11 : 0.13;

    static rank_t ranks[] =
      {
        { 80, 16, 803, 866, 0, pct },
        { 79, 16, 799, 861, 0, pct },
        { 75, 15, 702, 758, 0, pct },
        { 70, 14, 630, 680, 0, pct },
        { 68, 13, 597, 644, 0, 330 },
        { 62, 12, 522, 563, 0, 300 },
        { 60, 11, 515, 555, 0, 290 },
        { 56, 10, 429, 463, 0, 260 },
        { 50, 9,  353, 383, 0, 225 },
        { 44, 8,  292, 316, 0, 195 },
        { 0, 0, 0, 0, 0, 0 }
      };
    init_rank( ranks );

    base_execute_time = 3.0;
    may_crit          = true;
    direct_power_mod  = ( base_execute_time / 3.5 ) * 0.95;
    base_cost        *= 1.0 - p -> talents.precision     * 0.01;
    base_cost        *= 1.0 - util_t::talent_rank( p -> talents.frost_channeling, 3, 0.04, 0.07, 0.10 );
    base_execute_time -= p -> talents.improved_frost_bolt * 0.1;
    base_multiplier   *= 1.0 + p -> talents.piercing_ice * 0.02;
    base_multiplier   *= 1.0 + p -> talents.arctic_winds * 0.01;
    base_multiplier   *= 1.0 + p -> talents.arcane_instability * 0.01;
    base_multiplier   *= 1.0 + p -> talents.chilled_to_the_bone * 0.01;
    base_crit         += p -> talents.arcane_instability * 0.01;
    direct_power_mod  += p -> talents.empowered_frost_bolt * 0.05;

    if( sim -> P320 )
    {
      base_execute_time -= p -> talents.empowered_frost_bolt * 0.1;
    }
    else
    {
      base_crit += p -> talents.empowered_frost_bolt * 0.02;
    }

    base_crit_bonus_multiplier *= 1.0 + ( ( p -> talents.ice_shards  * 1.0/3 ) +
                                          ( p -> talents.spell_power * 0.25  ) +
                                          ( p -> talents.burnout     * 0.10  ) +
                                          ( p -> set_bonus.tier7_4pc() ? 0.05 : 0.00 ) );

    base_crit += p -> talents.winters_chill * 0.01;

    if ( p -> set_bonus.tier6_4pc() ) base_multiplier *= 1.05;
    if ( p -> set_bonus.tier9_4pc() ) base_crit       += 0.05;
    if ( p -> glyphs.frost_bolt     ) base_multiplier *= 1.05;

    may_torment = true;
  }

  virtual void execute()
  {
    mage_spell_t::execute();
    if ( result_is_hit() )
    {
      trigger_missile_barrage( this );
      trigger_replenishment( this );
      trigger_tier8_2pc( this );
    }
  }

  virtual void travel( int    travel_result,
                       double travel_dmg )
  {
    mage_spell_t::travel( travel_result, travel_dmg );
    trigger_fingers_of_frost( this );
  }

  virtual bool ready()
  {
    mage_t* p = player -> cast_mage();

    if ( frozen != -1 )
      if ( frozen != target_is_frozen( p ) )
        return false;

    return mage_spell_t::ready();
  }
};

// Ice Lance Spell =========================================================

struct ice_lance_t : public mage_spell_t
{
  int frozen;
  int ghost_charge;

  ice_lance_t( player_t* player, const std::string& options_str ) :
      mage_spell_t( "ice_lance", player, SCHOOL_FROST, TREE_FROST ), 
      frozen( -1 ), ghost_charge( -1 )
  {
    mage_t* p = player -> cast_mage();

    option_t options[] =
      {
        { "frozen",       OPT_BOOL, &frozen       },
        { "ghost_charge", OPT_BOOL, &ghost_charge },
        { NULL, OPT_UNKNOWN, NULL }
      };
    parse_options( options, options_str );

    double pct = sim -> P320 ? 0.06 : 0.07;

    static rank_t ranks[] =
      {
        { 80, 3, 223, 258, 0, pct },
        { 78, 3, 221, 255, 0, pct },
        { 72, 2, 182, 210, 0, pct },
        { 66, 1, 161, 187, 0, 150 },
        { 0, 0, 0, 0, 0, 0 }
      };
    init_rank( ranks );

    base_execute_time = 0.0;
    may_crit          = true;
    direct_power_mod  = ( 0.5/3.5 );
    base_cost        *= 1.0 - p -> talents.precision     * 0.01;
    base_cost        *= 1.0 - util_t::talent_rank( p -> talents.frost_channeling, 3, 0.04, 0.07, 0.10 );
    base_multiplier  *= 1.0 + p -> talents.piercing_ice * 0.02;
    base_multiplier  *= 1.0 + p -> talents.spell_impact * 0.02;
    base_multiplier  *= 1.0 + p -> talents.arcane_instability * 0.01;
    base_multiplier  *= 1.0 + p -> talents.arctic_winds * 0.01;
    base_multiplier  *= 1.0 + p -> talents.chilled_to_the_bone * 0.01;
    base_crit        += p -> talents.arcane_instability * 0.01;

    base_crit_bonus_multiplier *= 1.0 + ( ( p -> talents.ice_shards  * 1.0/3 ) +
                                          ( p -> talents.spell_power * 0.25  ) +
                                          ( p -> talents.burnout     * 0.10  ) +
                                          ( p -> set_bonus.tier7_4pc() ? 0.05 : 0.00 ) );
  }

  virtual void player_buff()
  {
    mage_t*   p = player -> cast_mage();
    target_t* t = sim -> target;

    mage_spell_t::player_buff();

    if ( p -> buffs_ghost_charge -> value() > 0 || p -> buffs_fingers_of_frost -> up() || t -> debuffs.frozen )
    {
      if ( p -> glyphs.ice_lance && t -> level > p -> level )
      {
        player_multiplier *= 4.0;
      }
      else
      {
        player_multiplier *= 3.0;
      }
    }
  }

  virtual bool ready()
  {
    mage_t* p = player -> cast_mage();

    if ( ! mage_spell_t::ready() )
      return false;

    if ( ghost_charge != -1 )
      if( ghost_charge != ( p -> buffs_ghost_charge -> check() ? 1 : 0 ) )
        return false;

    if ( frozen != -1 )
      if ( frozen != target_is_frozen( p ) )
        return false;

    return true;
  }
};

// Frostfire Bolt Spell ======================================================

struct frostfire_bolt_t : public mage_spell_t
{
  int dot_wait;
  int frozen;
  int ghost_charge;

  frostfire_bolt_t( player_t* player, const std::string& options_str ) :
    mage_spell_t( "frostfire_bolt", player, SCHOOL_FROSTFIRE, TREE_FROST ), 
    dot_wait( 0 ), frozen( -1 ), ghost_charge( -1 )
  {
    mage_t* p = player -> cast_mage();

    travel_speed = 21.0; // set before options to allow override

    option_t options[] =
      {
        { "dot_wait",     OPT_BOOL, &dot_wait     },
        { "frozen",       OPT_BOOL, &frozen       },
        { "ghost_charge", OPT_BOOL, &ghost_charge },
        { NULL, OPT_UNKNOWN, NULL }
      };
    parse_options( options, options_str );

    static rank_t ranks[] =
      {
        { 80, 2, 722, 838, 30, 0.14 },
        { 75, 1, 629, 731, 20, 0.14 },
        { 0, 0, 0, 0, 0, 0 }
      };
    init_rank( ranks );

    base_execute_time = 3.0;
    may_crit          = true;
    direct_power_mod  = base_execute_time / 3.5;

    base_tick_time    = 3.0;
    num_ticks         = 3;
    tick_power_mod    = 0;

    base_cost        *= 1.0 - p -> talents.precision     * 0.01;
    base_cost        *= 1.0 - util_t::talent_rank( p -> talents.frost_channeling, 3, 0.04, 0.07, 0.10 );
    base_multiplier  *= 1.0 + p -> talents.fire_power * 0.02;
    base_multiplier  *= 1.0 + p -> talents.arcane_instability * 0.01;
    base_multiplier  *= 1.0 + p -> talents.piercing_ice * 0.02;
    base_multiplier  *= 1.0 + p -> talents.arctic_winds * 0.01;
    base_multiplier  *= 1.0 + p -> talents.chilled_to_the_bone * 0.01;
    base_crit        += p -> talents.arcane_instability * 0.01;
    base_crit        += p -> talents.critical_mass * 0.02;
    base_crit        += p -> talents.pyromaniac * 0.01;
    direct_power_mod += p -> talents.empowered_fire * 0.05;

    base_crit_bonus_multiplier *= 1.0 + ( ( p -> talents.ice_shards  * 1.0/3 ) +
                                          ( p -> talents.spell_power * 0.25  ) +
                                          ( p -> talents.burnout     * 0.10  ) +
                                          ( p -> set_bonus.tier7_4pc() ? 0.05 : 0.00 ) );

    base_crit += p -> talents.improved_scorch * 0.01;

    if ( p -> set_bonus.tier9_4pc() ) base_crit += 0.05;

    if ( p -> glyphs.frostfire )
    {
      base_multiplier *= 1.02;
      base_crit += 0.02;
    }

    may_torment = true;
  }

  virtual void execute()
  {
    mage_spell_t::execute();
    if ( ! dot_wait ) duration_ready = 0;
    if ( result_is_hit() )
    {
      trigger_missile_barrage( this );
      trigger_tier8_2pc( this );
    }
    trigger_hot_streak( this );
  }

  virtual void travel( int    travel_result,
                       double travel_dmg )
  {
    mage_spell_t::travel( travel_result, travel_dmg );
    trigger_fingers_of_frost( this );
  }

  virtual bool ready()
  {
    mage_t* p = player -> cast_mage();

    if ( ghost_charge != -1 )
      if( ghost_charge != ( p -> buffs_ghost_charge -> check() ? 1 : 0 ) )
        return false;

    if ( frozen != -1 )
      if ( frozen != target_is_frozen( p ) )
        return false;

    return mage_spell_t::ready();
  }
};

// Icy Veins Spell =========================================================

struct icy_veins_t : public mage_spell_t
{
  icy_veins_t( player_t* player, const std::string& options_str ) :
      mage_spell_t( "icy_veins", player, SCHOOL_FROST, TREE_FROST )
  {
    mage_t* p = player -> cast_mage();
    check_talent( p -> talents.icy_veins );

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    base_cost = 200;
    trigger_gcd = 0;
    cooldown = 180.0;
    cooldown *= 1.0 - p -> talents.ice_floes * ( 0.20 / 3.0 );
  }

  virtual void execute()
  {
    mage_t* p = player -> cast_mage();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    consume_resource();
    update_ready();
    p -> buffs_icy_veins -> start();
  }

  virtual bool ready()
  {
    mage_t* p = player -> cast_mage();
    if( p -> buffs_icy_veins -> check() )
      return false;
    return mage_spell_t::ready();
  }
};

// Cold Snap Spell ==========================================================

struct cold_snap_t : public mage_spell_t
{
  cold_snap_t( player_t* player, const std::string& options_str ) :
      mage_spell_t( "cold_snap", player, SCHOOL_FROST, TREE_FROST )
  {
    mage_t* p = player -> cast_mage();
    check_talent( p -> talents.cold_snap );

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    cooldown = 600;
    cooldown *= 1.0 - p -> talents.cold_as_ice * 0.10;
  }

  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );

    for ( action_t* a = player -> action_list; a; a = a -> next )
    {
      if ( a -> school == SCHOOL_FROST )
      {
        a -> cooldown_ready = 0;
      }
    }

    update_ready();
  }
};

// Molten Armor Spell =====================================================

struct molten_armor_t : public mage_spell_t
{
  molten_armor_t( player_t* player, const std::string& options_str ) :
      mage_spell_t( "molten_armor", player, SCHOOL_FIRE, TREE_FIRE )
  {
    trigger_gcd = 0;
  }

  virtual void execute()
  {
    mage_t* p = player -> cast_mage();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    p -> buffs_mage_armor -> expire();
    p -> buffs_molten_armor -> start();
  }

  virtual bool ready()
  {
    mage_t* p = player -> cast_mage();
    return ! p -> buffs_molten_armor -> check();
  }
};

// Mage Armor Spell =======================================================

struct mage_armor_t : public mage_spell_t
{
  mage_armor_t( player_t* player, const std::string& options_str ) :
      mage_spell_t( "mage_armor", player, SCHOOL_ARCANE, TREE_ARCANE )
  {
    trigger_gcd = 0;
  }

  virtual void execute()
  {
    mage_t* p = player -> cast_mage();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    p -> buffs_molten_armor -> expire();
    p -> buffs_mage_armor -> start();
  }

  virtual bool ready()
  {
    mage_t* p = player -> cast_mage();
    return ! p -> buffs_mage_armor -> check();
  }
};

// Arcane Brilliance Spell =================================================

struct arcane_brilliance_t : public mage_spell_t
{
  double bonus;

  arcane_brilliance_t( player_t* player, const std::string& options_str ) :
      mage_spell_t( "arcane_brilliance", player, SCHOOL_ARCANE, TREE_ARCANE ), bonus( 0 )
  {
    trigger_gcd = 0;

    bonus = util_t::ability_rank( player -> level,  60.0,80,  40.0,70,  31.0,0 );
  }

  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );

    for ( player_t* p = sim -> player_list; p; p = p -> next )
    {
      p -> buffs.arcane_brilliance = bonus;
      p -> init_resources( true );
    }
  }

  virtual bool ready()
  {
    return( player -> buffs.arcane_brilliance < bonus );
  }
};

// Counterspell ============================================================

struct counterspell_t : public mage_spell_t
{
  counterspell_t( player_t* player, const std::string& options_str ) :
      mage_spell_t( "counterspell", player, SCHOOL_ARCANE, TREE_ARCANE )
  {
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    trigger_gcd = 0;
    base_dd_min = base_dd_max = 1;
    may_miss = may_resist = false;
    base_cost = player -> resource_base[ RESOURCE_MANA ] * 0.09;
    base_spell_power_multiplier = 0;
    cooldown = 24;
  }

  virtual bool ready()
  {
    if( ! sim -> target -> casting ) return false;
    return mage_spell_t::ready();
  }
};

// Water Elemental Spell ================================================

struct water_elemental_spell_t : public mage_spell_t
{
  struct water_elemental_expiration_t : public event_t
  {
    water_elemental_expiration_t( sim_t* sim, player_t* player ) : event_t( sim, player )
    {
      mage_t* p = player -> cast_mage();
      p -> summon_pet( "water_elemental" );
      sim -> add_event( this, 45.0 + p -> talents.improved_water_elemental * 5.0 );
    }
    virtual void execute()
    {
      player -> dismiss_pet( "water_elemental" );
    }
  };

  water_elemental_spell_t( player_t* player, const std::string& options_str ) :
      mage_spell_t( "water_elemental", player, SCHOOL_FROST, TREE_FROST )
  {
    mage_t* p = player -> cast_mage();
    check_talent( p -> talents.summon_water_elemental );

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    base_cost  = p -> resource_base[ RESOURCE_MANA ] * 0.16;
    base_cost *= 1.0 - p -> talents.frost_channeling * ( 0.1/3 );
    cooldown   = p -> glyphs.water_elemental ? 150 : 180;
    cooldown  *= 1.0 - p -> talents.cold_as_ice * 0.10;
    harmful    = false;
  }

  virtual void execute()
  {
    consume_resource();
    update_ready();
    new ( sim ) water_elemental_expiration_t( sim, player );
  }

  virtual bool ready()
  {
    if ( ! mage_spell_t::ready() )
      return false;

    return player -> cast_mage() -> active_water_elemental == 0;
  }
};

// Mirror Image Spell ======================================================

struct mirror_image_spell_t : public mage_spell_t
{
  mirror_image_spell_t( player_t* player, const std::string& options_str ) :
      mage_spell_t( "mirror_image", player, SCHOOL_ARCANE, TREE_ARCANE )
  {
    mage_t* p = player -> cast_mage();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    base_cost   = p -> resource_base[ RESOURCE_MANA ] * 0.10;
    base_cost  *= 1.0 - p -> talents.frost_channeling * ( 0.1/3 );
    cooldown    = 180;
    trigger_gcd = 0;
    harmful     = false;
  }

  virtual void execute()
  {
    consume_resource();
    update_ready();
    player -> summon_pet( "mirror_image" );
  }
};

// Mana Gem =================================================================

struct mana_gem_t : public action_t
{
  double trigger;
  double min;
  double max;
  int charges;
  int used;

  mana_gem_t( player_t* player, const std::string& options_str ) :
      action_t( ACTION_USE, "mana_gem", player ), 
      trigger( 0 ), min( 3330 ), max( 3500 ), charges( 3 ), used( 0 )
  {
    mage_t* p = player -> cast_mage();

    option_t options[] =
      {
        { "min",     OPT_FLT, &min     },
        { "max",     OPT_FLT, &max     },
        { "trigger", OPT_FLT, &trigger },
        { "charges", OPT_FLT, &charges },
        { NULL, OPT_UNKNOWN, NULL }
      };
    parse_options( options, options_str );

    if ( min == 0 && max == 0 ) min = max = trigger;

    if ( min > max ) std::swap( min, max );

    if ( max == 0 ) max = trigger;
    if ( trigger == 0 ) trigger = max;
    assert( max > 0 && trigger > 0 );

    if ( p -> glyphs.mana_gem )
    {
      min *= 1.40;
      max *= 1.40;
      trigger *= 1.40;
    }
    if ( p -> set_bonus.tier7_2pc() )
    {
      min *= 1.40;
      max *= 1.40;
      trigger *= 1.40;
    }

    cooldown = 120.0;
    cooldown_group = "rune";
    trigger_gcd = 0;
    harmful = false;
  }

  virtual void execute()
  {
    mage_t* p = player -> cast_mage();

    if ( sim -> log ) log_t::output( sim, "%s uses Mana Gem", p -> name() );

    p -> procs_mana_gem -> occur();
    used++;

    double gain = sim -> rng -> range( min, max );

    if ( p -> set_bonus.tier7_2pc() )
    {
      struct expiration_t : public event_t
      {
        expiration_t( sim_t* sim, player_t* p ) : event_t( sim, p )
        {
          name = "Tier7 2-Piece Expiration";
          player -> aura_gain( "Tier7 2-Piece" );
          player -> spell_power[ SCHOOL_MAX ] += 225;
          sim -> add_event( this, 15.0 );
        }
        virtual void execute()
        {
          player -> aura_loss( "Tier7 2-Piece" );
          player -> spell_power[ SCHOOL_MAX ] -= 225;
        }
      };

      new ( sim ) expiration_t( sim, p );
    }

    p -> resource_gain( RESOURCE_MANA, gain, p -> gains_mana_gem );
    p -> share_cooldown( cooldown_group, cooldown );
  }

  virtual bool ready()
  {
    if ( cooldown_ready > sim -> current_time )
      return false;

    if ( used >= charges )
      return false;

    return( ( player -> resource_max    [ RESOURCE_MANA ] -
              player -> resource_current[ RESOURCE_MANA ] ) > trigger ); 
  }

  virtual void reset()
  {
    action_t::reset();
    used = 0;
  }
};

// Choose Rotation ==========================================================

struct choose_rotation_t : public action_t
{
  double last_time;

  choose_rotation_t( player_t* p, const std::string& options_str ) :
      action_t( ACTION_USE, "choose_rotation", p ), last_time( 0 )
  {
    cooldown = 10;

    option_t options[] =
      {
        { "cooldown", OPT_FLT, &cooldown },
        { NULL, OPT_UNKNOWN, NULL }
      };
    parse_options( options, options_str );

    if ( cooldown < 1.0 )
    {
      util_t::printf( "simcraft: choose_rotation cannot have cooldown less than 1.0sec\n" );
      exit( 0 );
    }

    trigger_gcd = 0;
    harmful = false;
  }

  virtual void execute()
  {
    mage_t* p = player -> cast_mage();

    if ( sim -> log ) log_t::output( sim, "%s Considers Spell Rotation", p -> name() );

    // It is important to smooth out the regen rate by averaging out the returns from Evocation and Mana Gems.
    // In order for this to work, the resource_gain() method must filter out these sources when
    // tracking "rotation.mana_gain".

    double regen_rate = p -> rotation.mana_gain / sim -> current_time;

    // Evocation
    regen_rate += p -> resource_max[ RESOURCE_MANA ] * 0.60 / ( 240.0 - p -> talents.arcane_flows * 60.0 );

    // Mana Gem
    regen_rate += 3400 * ( 1.0 + p -> glyphs.mana_gem * 0.40 ) * ( 1.0 + p -> set_bonus.tier7_2pc() * 0.40 ) / 120.0;

    if ( p -> rotation.current == ROTATION_DPS )
    {
      p -> rotation.dps_time += ( sim -> current_time - last_time );

      double consumption_rate = ( p -> rotation.dps_mana_loss / p -> rotation.dps_time ) - regen_rate;

      if ( consumption_rate > 0 )
      {
        double oom_time = p -> resource_current[ RESOURCE_MANA ] / consumption_rate;

        if ( oom_time < sim -> target -> time_to_die() )
        {
          if ( sim -> log ) log_t::output( sim, "%s switches to DPM spell rotation", p -> name() );

          p -> rotation.current = ROTATION_DPM;
        }
      }
    }
    else if ( p -> rotation.current == ROTATION_DPM )
    {
      p -> rotation.dpm_time += ( sim -> current_time - last_time );

      double consumption_rate = ( p -> rotation.dpm_mana_loss / p -> rotation.dpm_time ) - regen_rate;

      if ( consumption_rate > 0 )
      {
        double oom_time = p -> resource_current[ RESOURCE_MANA ] / consumption_rate;

        if ( oom_time > sim -> target -> time_to_die() )
        {
          if ( sim -> log ) log_t::output( sim, "%s switches to DPS spell rotation", p -> name() );

          p -> rotation.current = ROTATION_DPS;
        }
      }
      else
      {
        if ( sim -> log ) log_t::output( sim, "%s switches to DPS rotation (negative consumption)", p -> name() );

        p -> rotation.current = ROTATION_DPS;
      }
    }
    last_time = sim -> current_time;

    update_ready();
  }

  virtual bool ready()
  {
    return( sim -> current_time >= cooldown_ready &&
            sim -> current_time >= cooldown );
  }

  virtual void reset()
  {
    action_t::reset();
    last_time=0;
  }
};

} // ANONYMOUS NAMESPACE ===================================================

// ==========================================================================
// Mage Character Definition
// ==========================================================================

// mage_t::create_action ====================================================

action_t* mage_t::create_action( const std::string& name,
                                 const std::string& options_str )
{
  if ( name == "arcane_barrage"    ) return new          arcane_barrage_t( this, options_str );
  if ( name == "arcane_blast"      ) return new            arcane_blast_t( this, options_str );
  if ( name == "arcane_brilliance" ) return new       arcane_brilliance_t( this, options_str );
  if ( name == "arcane_missiles"   ) return new         arcane_missiles_t( this, options_str );
  if ( name == "arcane_power"      ) return new            arcane_power_t( this, options_str );
  if ( name == "choose_rotation"   ) return new         choose_rotation_t( this, options_str );
  if ( name == "cold_snap"         ) return new               cold_snap_t( this, options_str );
  if ( name == "combustion"        ) return new              combustion_t( this, options_str );
  if ( name == "counterspell"      ) return new            counterspell_t( this, options_str );
  if ( name == "evocation"         ) return new               evocation_t( this, options_str );
  if ( name == "fire_ball"         ) return new               fire_ball_t( this, options_str );
  if ( name == "fire_blast"        ) return new              fire_blast_t( this, options_str );
  if ( name == "focus_magic"       ) return new             focus_magic_t( this, options_str );
  if ( name == "frost_bolt"        ) return new              frost_bolt_t( this, options_str );
  if ( name == "frostfire_bolt"    ) return new          frostfire_bolt_t( this, options_str );
  if ( name == "ice_lance"         ) return new               ice_lance_t( this, options_str );
  if ( name == "icy_veins"         ) return new               icy_veins_t( this, options_str );
  if ( name == "living_bomb"       ) return new             living_bomb_t( this, options_str );
  if ( name == "mage_armor"        ) return new              mage_armor_t( this, options_str );
  if ( name == "mirror_image"      ) return new      mirror_image_spell_t( this, options_str );
  if ( name == "molten_armor"      ) return new            molten_armor_t( this, options_str );
  if ( name == "presence_of_mind"  ) return new        presence_of_mind_t( this, options_str );
  if ( name == "pyroblast"         ) return new               pyroblast_t( this, options_str );
  if ( name == "scorch"            ) return new                  scorch_t( this, options_str );
  if ( name == "slow"              ) return new                    slow_t( this, options_str );
  if ( name == "water_elemental"   ) return new   water_elemental_spell_t( this, options_str );
  if ( name == "mana_gem"          ) return new                mana_gem_t( this, options_str );

  return player_t::create_action( name, options_str );
}

// mage_t::create_pet ======================================================

pet_t* mage_t::create_pet( const std::string& pet_name )
{
  pet_t* p = find_pet( pet_name );

  if ( p ) return p;

  if ( pet_name == "water_elemental" ) return new water_elemental_pet_t( sim, this );
  if ( pet_name == "mirror_image"    ) return new mirror_image_pet_t   ( sim, this );

  return 0;
}

// mage_t::init_glyphs ====================================================

void mage_t::init_glyphs()
{
  memset( ( void* ) &glyphs, 0x0, sizeof( glyphs_t ) );

  std::vector<std::string> glyph_names;
  int num_glyphs = util_t::string_split( glyph_names, glyphs_str, ",/" );
  
  for( int i=0; i < num_glyphs; i++ )
  {
    std::string& n = glyph_names[ i ];

    if     ( n == "arcane_barrage"  ) glyphs.arcane_barrage = 1;
    else if( n == "arcane_blast"    ) glyphs.arcane_blast = 1;
    else if( n == "arcane_missiles" ) glyphs.arcane_missiles = 1;
    else if( n == "arcane_power"    ) glyphs.arcane_power = 1;
    else if( n == "fire_ball"       ) glyphs.fire_ball = 1;
    else if( n == "fireball"        ) glyphs.fire_ball = 1;
    else if( n == "frost_bolt"      ) glyphs.frost_bolt = 1;
    else if( n == "frostbolt"       ) glyphs.frost_bolt = 1;
    else if( n == "ice_lance"       ) glyphs.ice_lance = 1;
    else if( n == "improved_scorch" ) glyphs.improved_scorch = 1;
    else if( n == "living_bomb"     ) glyphs.living_bomb = 1;
    else if( n == "mage_armor"      ) glyphs.mage_armor = 1;
    else if( n == "mana_gem"        ) glyphs.mana_gem = 1;
    else if( n == "mirror_image"    ) glyphs.mirror_image = 1;
    else if( n == "molten_armor"    ) glyphs.molten_armor = 1;
    else if( n == "water_elemental" ) glyphs.water_elemental = 1;
    else if( n == "frostfire"       ) glyphs.frostfire = 1;
    // To prevent warnings....
    else if( n == "arcane_intellect" ) ;
    else if( n == "blast_wave"       ) ;
    else if( n == "blink"            ) ;
    else if( n == "evocation"        ) ;
    else if( n == "fire_blast"       ) ;
    else if( n == "fire_ward"        ) ;
    else if( n == "frost_armor"      ) ;
    else if( n == "frost_ward"       ) ;
    else if( n == "ice_barrier"      ) ;
    else if( n == "ice_block"        ) ;
    else if( n == "icy_veins"        ) ;
    else if( n == "slow_fall"        ) ;
    else if( n == "the_penguin"      ) ;
    else if( ! sim -> parent ) util_t::printf( "simcraft: Player %s has unrecognized glyph %s\n", name(), n.c_str() );
  }
}

// mage_t::init_base =======================================================

void mage_t::init_base()
{
  attribute_base[ ATTR_STRENGTH  ] =  35;
  attribute_base[ ATTR_AGILITY   ] =  41;
  attribute_base[ ATTR_STAMINA   ] =  60;
  attribute_base[ ATTR_INTELLECT ] = 179;
  attribute_base[ ATTR_SPIRIT    ] = 179;

  attribute_multiplier_initial[ ATTR_INTELLECT ] *= 1.0 + talents.arcane_mind * 0.03;
  attribute_multiplier_initial[ ATTR_SPIRIT    ] *= 1.0 + talents.student_of_the_mind * ( 0.1 / 3.0 );

  base_spell_crit = 0.00907381;
  initial_spell_crit_per_intellect = rating_t::interpolate( level, 0.01/60.0, 0.01/80.0, 0.01/166.79732 );
  initial_spell_power_per_intellect = talents.mind_mastery * 0.03;

  base_attack_power = -10;
  base_attack_crit  = 0.0345777;
  initial_attack_power_per_strength = 1.0;
  initial_attack_crit_per_agility = rating_t::interpolate( level, 0.01/16.0, 0.01/24.9, 0.01/51.84598 );

  // FIXME! Make this level-specific.
  resource_base[ RESOURCE_HEALTH ] = 3200;
  resource_base[ RESOURCE_MANA   ] = rating_t::interpolate( level, 1183, 2241, 3268 );

  health_per_stamina = 10;
  mana_per_intellect = 15;
}

// mage_t::init_buffs ======================================================

void mage_t::init_buffs()
{
  player_t::init_buffs();

  // buff_t( sim, player, name, max_stack, duration, cooldown, proc_chance, quiet )

  buffs_arcane_blast         = new buff_t( sim, this, "arcane_blast",         3, 10.0 );
  buffs_arcane_power         = new buff_t( sim, this, "arcane_power",         1, ( glyphs.arcane_power ? 18.0 : 15.0 ) );
  buffs_brain_freeze         = new buff_t( sim, this, "brain_freeze",         1, 15.0, 0, talents.brain_freeze * 0.05 );
  buffs_clearcasting         = new buff_t( sim, this, "clearcasting",         1, 10.0, 0, talents.arcane_concentration * 0.02 );
  buffs_combustion           = new buff_t( sim, this, "combustion",           3 );
  buffs_fingers_of_frost     = new buff_t( sim, this, "fingers_of_frost",     2,    0, 0, talents.fingers_of_frost * 0.15/2 );
  buffs_hot_streak_crits     = new buff_t( sim, this, "hot_streak_crits",     2,    0, 0, 1.0, true );
  buffs_hot_streak           = new buff_t( sim, this, "hot_streak",           1, 10.0, 0, talents.hot_streak / 3.0 );
  buffs_icy_veins            = new buff_t( sim, this, "icy_veins",            1, 20.0 );
  buffs_incanters_absorption = new buff_t( sim, this, "incanters_absorption", 1, 10.0 );
  buffs_missile_barrage      = new buff_t( sim, this, "missile_barrage",      1, 15.0, 0, talents.missile_barrage * 0.04 );

  buffs_ghost_charge = new buff_t( sim, this, "ghost_charge" );
  buffs_mage_armor   = new buff_t( sim, this, "mage_armor" );
  buffs_molten_armor = new buff_t( sim, this, "molten_armor" );
}

// mage_t::init_gains ======================================================

void mage_t::init_gains()
{
  player_t::init_gains();

  gains_clearcasting       = get_gain( "clearcasting" );
  gains_empowered_fire     = get_gain( "empowered_fire" );
  gains_evocation          = get_gain( "evocation" );
  gains_mana_gem           = get_gain( "mana_gem" );
  gains_master_of_elements = get_gain( "master_of_elements" );
}

// mage_t::init_procs ======================================================

void mage_t::init_procs()
{
  player_t::init_procs();

  procs_deferred_ignite = get_proc( "deferred_ignite",      sim );
  procs_mana_gem        = get_proc( "mana_gem"            , sim );
}

// mage_t::init_uptimes ====================================================

void mage_t::init_uptimes()
{
  player_t::init_uptimes();

  uptimes_arcane_blast[ 0 ]    = get_uptime( "arcane_blast_0" );
  uptimes_arcane_blast[ 1 ]    = get_uptime( "arcane_blast_1" );
  uptimes_arcane_blast[ 2 ]    = get_uptime( "arcane_blast_2" );
  uptimes_arcane_blast[ 3 ]    = get_uptime( "arcane_blast_3" );
  uptimes_dps_rotation         = get_uptime( "dps_rotation" );
  uptimes_dpm_rotation         = get_uptime( "dpm_rotation" );
  uptimes_focus_magic_feedback = get_uptime( "focus_magic_feedback" );
  uptimes_water_elemental      = get_uptime( "water_elemental" );
}

// mage_t::init_rng ========================================================

void mage_t::init_rng()
{
  player_t::init_rng();

  rng_empowered_fire           = get_rng( "empowered_fire"           );
  rng_frostbite                = get_rng( "frostbite"                );
  rng_ghost_charge             = get_rng( "ghost_charge"             );
  rng_improved_scorch          = get_rng( "improved_scorch"          );
  rng_improved_water_elemental = get_rng( "improved_water_elemental" );
  rng_winters_chill            = get_rng( "winters_chill"            );
  rng_winters_grasp            = get_rng( "winters_grasp"            );
}

// mage_t::init_actions ======================================================

void mage_t::init_actions()
{
  if( action_list_str.empty() )
  {
    action_list_str = "flask,type=frost_wyrm/food,type=tender_shoveltusk_steak/arcane_brilliance";
    if( talents.focus_magic ) action_list_str += "/focus_magic";
    action_list_str += "/counterspell";
    if( talents.improved_scorch ) action_list_str += "/scorch,debuff=1";
    action_list_str += "/mirror_image";
    int num_items = items.size();
    for( int i=0; i < num_items; i++ )
    {
      if( items[ i ].use.active() )
      {
        action_list_str += "/use_item,name=";
        action_list_str += items[ i ].name();
      }
    }
    if( talents.combustion   ) action_list_str += "/combustion";
    if( talents.arcane_power ) action_list_str += "/arcane_power";
    if( talents.icy_veins    ) action_list_str += "/icy_veins";
    if( talents.presence_of_mind ) 
    {
      action_list_str += "/presence_of_mind,";
      action_list_str += ( talents.pyroblast ) ? "pyroblast" : "arcane_blast";
    }
    if( primary_tree() == TREE_ARCANE )
    {
      action_list_str += "/mana_gem/evocation";
      action_list_str += "/choose_rotation";
      action_list_str += "/arcane_blast,max=2";
      action_list_str += "/arcane_missiles,barrage=1,dpm=1";
      action_list_str += "/arcane_blast,max=3";
      action_list_str += "/arcane_missiles";
      action_list_str += "/mana_potion";
      if( talents.arcane_barrage ) action_list_str += "/arcane_barrage,moving=1"; // when moving
      action_list_str += "/fire_blast,moving=1"; // when moving
    }
    else if( primary_tree() == TREE_FROST )
    {
      action_list_str += "/mana_gem,trigger=1000/speed_potion";
      action_list_str += "/frost_bolt,frozen=1";
      if( talents.summon_water_elemental ) action_list_str += "/water_elemental";
      if( talents.cold_snap              ) action_list_str += "/cold_snap";
      if( talents.brain_freeze           ) action_list_str += "/fire_ball,brain_freeze=1";
      action_list_str += "/frost_bolt";
      action_list_str += "/evocation";
      action_list_str += "/ice_lance,moving=1";  // when moving
      action_list_str += "/fire_blast,moving=1"; // when moving
    }
    else if( primary_tree() == TREE_FIRE )
    {
      action_list_str += "/mana_gem,trigger=1000/speed_potion";
      if( talents.living_bomb ) action_list_str += "/living_bomb";
      if( talents.hot_streak  ) action_list_str += "/pyroblast,hot_streak=1";
      if( talents.piercing_ice && talents.ice_shards )
      {
        action_list_str += "/frostfire_bolt";
      }
      else
      {
        action_list_str += "/fire_ball";
      }
      action_list_str += "/evocation";
      action_list_str += "/fire_blast,moving=1"; // when moving
      action_list_str += "/ice_lance,moving=1";  // when moving
    }
    else action_list_str = "/arcane_missiles/evocation";

    action_list_default = 1;
  }

  player_t::init_actions();
}

// mage_t::primary_tree ====================================================

int mage_t::primary_tree() SC_CONST
{
  if( talents.arcane_empowerment   ) return TREE_ARCANE;
  if( talents.empowered_fire       ) return TREE_FIRE;
  if( talents.empowered_frost_bolt ) return TREE_FROST;

  return TALENT_TREE_MAX;
}

// mage_t::composite_spell_power ===========================================

double mage_t::composite_spell_power( int school ) SC_CONST
{
  double sp = player_t::composite_spell_power( school );

  if( talents.incanters_absorption )
  {
    sp += buffs_incanters_absorption -> value();
  }

  return sp;
}

// mage_t::combat_begin ====================================================

void mage_t::combat_begin()
{
  player_t::combat_begin();

  if ( ! armor_type_str.empty() )
  {
    if ( sim -> log ) log_t::output( sim, "%s equips %s armor", name(), armor_type_str.c_str() );

    if ( armor_type_str == "mage" )
    {
      buffs_mage_armor -> start();
    }
    else if ( armor_type_str == "molten" )
    {
      buffs_molten_armor -> start();
    }
    else
    {
      util_t::printf( "simcraft: Unknown armor type '%s' for player %s\n", armor_type_str.c_str(), name() );
      exit( 0 );
    }
  }
}

// mage_t::reset ===========================================================

void mage_t::reset()
{
  player_t::reset();

  // Active
  active_water_elemental = 0;

  rotation.reset();
}

// mage_t::regen  ==========================================================

void mage_t::regen( double periodicity )
{
  mana_regen_while_casting  = 0;
  mana_regen_while_casting += util_t::talent_rank( talents.arcane_meditation, 3, 0.17, 0.33, 0.50 );
  mana_regen_while_casting += util_t::talent_rank( talents.pyromaniac,        3, 0.17, 0.33, 0.50 );

  if ( buffs_mage_armor -> up() )
  {
    mana_regen_while_casting += 0.50 + ( glyphs.mage_armor ? 0.20 : 0.00 );
  }

  player_t::regen( periodicity );

  uptimes_water_elemental -> update( active_water_elemental != 0 );
}

// mage_t::resource_gain ===================================================

double mage_t::resource_gain( int       resource,
                              double    amount,
                              gain_t*   source,
                              action_t* action )
{
  double actual_amount = player_t::resource_gain( resource, amount, source, action );

  if ( source != gains_evocation &&
       source != gains_mana_gem )
  {
    rotation.mana_gain += actual_amount;
  }

  return actual_amount;
}

// mage_t::resource_loss ===================================================

double mage_t::resource_loss( int       resource,
                              double    amount,
                              action_t* action )
{
  double actual_amount = player_t::resource_loss( resource, amount, action );

  if( resource == RESOURCE_MANA )
  {
    if ( rotation.current == ROTATION_DPS )
    {
      rotation.dps_mana_loss += actual_amount;
    }
    else if ( rotation.current == ROTATION_DPM )
    {
      rotation.dpm_mana_loss += actual_amount;
    }
  }
  else if( resource == RESOURCE_HEALTH )
  {
    trigger_incanters_absorption( this, amount );
  }

  return actual_amount;
}

// mage_t::get_talent_trees ================================================

bool mage_t::get_talent_trees( std::vector<int*>& arcane,
                               std::vector<int*>& fire,
                               std::vector<int*>& frost )
{
  talent_translation_t translation[][3] =
    {
      { {  1, &( talents.arcane_subtlety      ) }, {  1, &( talents.improved_fire_blast ) }, {  1, &( talents.frostbite                ) } },
      { {  2, &( talents.arcane_focus         ) }, {  2, &( talents.incineration        ) }, {  2, &( talents.improved_frost_bolt      ) } },
      { {  3, NULL                              }, {  3, &( talents.improved_fire_ball  ) }, {  3, &( talents.ice_floes                ) } },
      { {  4, NULL                              }, {  4, &( talents.ignite              ) }, {  4, &( talents.ice_shards               ) } },
      { {  5, NULL                              }, {  5, NULL                             }, {  5, NULL                                  } },
      { {  6, &( talents.arcane_concentration ) }, {  6, &( talents.world_in_flames     ) }, {  6, &( talents.precision                ) } },
      { {  7, NULL                              }, {  7, NULL                             }, {  7, NULL                                  } },
      { {  8, &( talents.spell_impact         ) }, {  8, NULL                             }, {  8, &( talents.piercing_ice             ) } },
      { {  9, &( talents.student_of_the_mind  ) }, {  9, &( talents.pyroblast           ) }, {  9, &( talents.icy_veins                ) } },
      { { 10, &( talents.focus_magic          ) }, { 10, &( talents.burning_soul        ) }, { 10, NULL                                  } },
      { { 11, NULL                              }, { 11, &( talents.improved_scorch     ) }, { 11, NULL                                  } },
      { { 12, NULL                              }, { 12, NULL                             }, { 12, &( talents.frost_channeling         ) } },
      { { 13, &( talents.arcane_meditation    ) }, { 13, &( talents.master_of_elements  ) }, { 13, &( talents.shatter                  ) } },
      { { 14, &( talents.torment_the_weak     ) }, { 14, &( talents.playing_with_fire   ) }, { 14, &( talents.cold_snap                ) } },
      { { 15, NULL                              }, { 15, &( talents.critical_mass       ) }, { 15, NULL                                  } },
      { { 16, &( talents.presence_of_mind     ) }, { 16, NULL                             }, { 16, NULL                                  } },
      { { 17, &( talents.arcane_mind          ) }, { 17, NULL                             }, { 17, &( talents.cold_as_ice              ) } },
      { { 18, NULL                              }, { 18, &( talents.fire_power          ) }, { 18, &( talents.winters_chill            ) } },
      { { 19, &( talents.arcane_instability   ) }, { 19, &( talents.pyromaniac          ) }, { 19, NULL                                  } },
      { { 20, &( talents.arcane_potency       ) }, { 20, &( talents.combustion          ) }, { 20, NULL                                  } },
      { { 21, &( talents.arcane_empowerment   ) }, { 21, &( talents.molten_fury         ) }, { 21, &( talents.arctic_winds             ) } },
      { { 22, &( talents.arcane_power         ) }, { 22, NULL                             }, { 22, &( talents.empowered_frost_bolt     ) } },
      { { 23, &( talents.incanters_absorption ) }, { 23, &( talents.empowered_fire      ) }, { 23, &( talents.fingers_of_frost         ) } },
      { { 24, &( talents.arcane_flows         ) }, { 24, NULL                             }, { 24, &( talents.brain_freeze             ) } },
      { { 25, &( talents.mind_mastery         ) }, { 25, NULL                             }, { 25, &( talents.summon_water_elemental   ) } },
      { { 26, &( talents.slow                 ) }, { 26, &( talents.hot_streak          ) }, { 26, &( talents.improved_water_elemental ) } },
      { { 27, &( talents.missile_barrage      ) }, { 27, &( talents.burnout             ) }, { 27, &( talents.chilled_to_the_bone      ) } },
      { { 28, &( talents.netherwind_presence  ) }, { 28, &( talents.living_bomb         ) }, { 28, NULL                                  } },
      { { 29, &( talents.spell_power          ) }, {  0, NULL                             }, {  0, NULL                                  } },
      { { 30, &( talents.arcane_barrage       ) }, {  0, NULL                             }, {  0, NULL                                  } },
      { {  0, NULL                              }, {  0, NULL                             }, {  0, NULL                                  } }
    };

  return get_talent_translation( arcane, fire, frost, translation );
}

// mage_t::get_options ====================================================

std::vector<option_t>& mage_t::get_options()
{
  if( options.empty() )
  {
    player_t::get_options();

    option_t mage_options[] =
    {
      // @option_doc loc=player/mage/talents title="Talents"
      { "arcane_barrage",            OPT_INT,   &( talents.arcane_barrage            ) },
      { "arcane_concentration",      OPT_INT,   &( talents.arcane_concentration      ) },
      { "arcane_empowerment",        OPT_INT,   &( talents.arcane_empowerment        ) },
      { "arcane_flows",              OPT_INT,   &( talents.arcane_flows              ) },
      { "arcane_focus",              OPT_INT,   &( talents.arcane_focus              ) },
      { "arcane_impact",             OPT_INT,   &( talents.arcane_impact             ) },
      { "arcane_instability",        OPT_INT,   &( talents.arcane_instability        ) },
      { "arcane_meditation",         OPT_INT,   &( talents.arcane_meditation         ) },
      { "arcane_mind",               OPT_INT,   &( talents.arcane_mind               ) },
      { "arcane_potency",            OPT_INT,   &( talents.arcane_potency            ) },
      { "arcane_power",              OPT_INT,   &( talents.arcane_power              ) },
      { "arcane_subtlety",           OPT_INT,   &( talents.arcane_subtlety           ) },
      { "arctic_winds",              OPT_INT,   &( talents.arctic_winds              ) },
      { "brain_freeze",              OPT_INT,   &( talents.brain_freeze              ) },
      { "burning_soul",              OPT_INT,   &( talents.burning_soul              ) },
      { "burnout",                   OPT_INT,   &( talents.burnout                   ) },
      { "chilled_to_the_bone",       OPT_INT,   &( talents.chilled_to_the_bone       ) },
      { "cold_as_ice",               OPT_INT,   &( talents.cold_as_ice               ) },
      { "cold_snap",                 OPT_INT,   &( talents.cold_snap                 ) },
      { "combustion",                OPT_INT,   &( talents.combustion                ) },
      { "critical_mass",             OPT_INT,   &( talents.critical_mass             ) },
      { "deep_freeze",               OPT_INT,   &( talents.deep_freeze               ) },
      { "empowered_arcane_missiles", OPT_INT,   &( talents.empowered_arcane_missiles ) },
      { "empowered_fire",            OPT_INT,   &( talents.empowered_fire            ) },
      { "empowered_frost_bolt",      OPT_INT,   &( talents.empowered_frost_bolt      ) },
      { "fingers_of_frost",          OPT_INT,   &( talents.fingers_of_frost          ) },
      { "fire_power",                OPT_INT,   &( talents.fire_power                ) },
      { "focus_magic",               OPT_INT,   &( talents.focus_magic               ) },
      { "frost_channeling",          OPT_INT,   &( talents.frost_channeling          ) },
      { "frostbite",                 OPT_INT,   &( talents.frostbite                 ) },
      { "hot_streak",                OPT_INT,   &( talents.hot_streak                ) },
      { "ice_floes",                 OPT_INT,   &( talents.ice_floes                 ) },
      { "ice_shards",                OPT_INT,   &( talents.ice_shards                ) },
      { "ignite",                    OPT_INT,   &( talents.ignite                    ) },
      { "improved_fire_ball",        OPT_INT,   &( talents.improved_fire_ball        ) },
      { "improved_fire_blast",       OPT_INT,   &( talents.improved_fire_blast       ) },
      { "improved_frost_bolt",       OPT_INT,   &( talents.improved_frost_bolt       ) },
      { "improved_scorch",           OPT_INT,   &( talents.improved_scorch           ) },
      { "improved_water_elemental",  OPT_INT,   &( talents.improved_water_elemental  ) },
      { "incanters_absorption",      OPT_INT,   &( talents.incanters_absorption      ) },
      { "incineration",              OPT_INT,   &( talents.incineration              ) },
      { "living_bomb",               OPT_INT,   &( talents.living_bomb               ) },
      { "master_of_elements",        OPT_INT,   &( talents.master_of_elements        ) },
      { "mind_mastery",              OPT_INT,   &( talents.mind_mastery              ) },
      { "missile_barrage",           OPT_INT,   &( talents.missile_barrage           ) },
      { "molten_fury",               OPT_INT,   &( talents.molten_fury               ) },
      { "netherwind_presence",       OPT_INT,   &( talents.netherwind_presence       ) },
      { "piercing_ice",              OPT_INT,   &( talents.piercing_ice              ) },
      { "playing_with_fire",         OPT_INT,   &( talents.playing_with_fire         ) },
      { "precision",                 OPT_INT,   &( talents.precision                 ) },
      { "presence_of_mind",          OPT_INT,   &( talents.presence_of_mind          ) },
      { "pyroblast",                 OPT_INT,   &( talents.pyroblast                 ) },
      { "pyromaniac",                OPT_INT,   &( talents.pyromaniac                ) },
      { "shatter",                   OPT_INT,   &( talents.shatter                   ) },
      { "slow",                      OPT_INT,   &( talents.slow                      ) },
      { "spell_impact",              OPT_INT,   &( talents.spell_impact              ) },
      { "spell_power",               OPT_INT,   &( talents.spell_power               ) },
      { "student_of_the_mind",       OPT_INT,   &( talents.student_of_the_mind       ) },
      { "torment_the_weak",          OPT_INT,   &( talents.torment_the_weak          ) },
      { "water_elemental",           OPT_INT,   &( talents.summon_water_elemental    ) },
      { "winters_chill",             OPT_INT,   &( talents.winters_chill             ) },
      { "winters_grasp",             OPT_INT,   &( talents.winters_grasp             ) },
      { "world_in_flames",           OPT_INT,   &( talents.world_in_flames           ) },
      // @option_doc loc=player/mage/misc title="Misc"
      { "armor_type",                OPT_STRING, &( armor_type_str                   ) },
      { "focus_magic_target",        OPT_STRING, &( focus_magic_target_str           ) },
      { "ghost_charge_pct",          OPT_FLT,    &( ghost_charge_pct                 ) },
      { NULL, OPT_UNKNOWN, NULL }
    };

    option_t::copy( options, mage_options );
  }

  return options;
}

// mage_t::save =============================================================

bool mage_t::save( FILE* file, int save_type )
{
  player_t::save( file, save_type );

  if ( save_type == SAVE_ALL || save_type == SAVE_ACTIONS )
  {
    if( ! armor_type_str.empty() ) util_t::fprintf( file, "armor_type=%s\n", armor_type_str.c_str() );
  }

  if ( save_type == SAVE_ALL )
  {
    if( ! focus_magic_target_str.empty() ) util_t::fprintf( file, "focus_magic_target=%s\n", focus_magic_target_str.c_str() );
  }

  return true;
}

// mage_t::decode_set =======================================================

int mage_t::decode_set( item_t& item )
{
  if( strstr( item.name(), "frostfire"  ) ) return SET_T7;
  if( strstr( item.name(), "kirin_tor"  ) ) 
    if( item.slot == SLOT_HEAD      ||
        item.slot == SLOT_SHOULDERS ||
        item.slot == SLOT_CHEST     ||
        item.slot == SLOT_HANDS     ||
        item.slot == SLOT_LEGS      )
      return SET_T8;
  if( strstr( item.name(), "sunstrider" ) ) return SET_T9;
  if( strstr( item.name(), "khadgar"    ) ) return SET_T9;
  return SET_NONE;
}

// ==========================================================================
// PLAYER_T EXTENSIONS
// ==========================================================================

// player_t::create_mage  ===================================================

player_t* player_t::create_mage( sim_t* sim, const std::string& name )
{
  mage_t* p = new mage_t( sim, name );

  new    mirror_image_pet_t( sim, p );
  new water_elemental_pet_t( sim, p );

  return p;
}

// player_t::init_mage ======================================================

void player_t::init_mage( sim_t* sim )
{
  for( player_t* p = sim -> player_list; p; p = p -> next )
  {
  }
}

// player_t::combat_begin_mage ==============================================

void player_t::combat_begin_mage( sim_t* sim )
{
  for( player_t* p = sim -> player_list; p; p = p -> next )
  {
  }
}
