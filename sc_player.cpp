// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include <simcraft.h>

namespace { // ANONYMOUS NAMESPACE ==========================================

// trigger_judgement_of_wisdom ==============================================

static void trigger_judgement_of_wisdom( action_t* action )
{
  player_t* p = action -> player;

  double jow = p -> sim -> target -> debuffs.judgement_of_wisdom;
  if( jow == 0 ) return;

  double max_mana = p -> resource_max[ RESOURCE_MANA ];

  if( max_mana <= 0 )
    return;

  if( ! rand_t::roll( 0.25 ) )
    return;

  p -> procs.judgement_of_wisdom -> occur();

  p -> resource_gain( RESOURCE_MANA, max_mana * 0.01, p -> gains.judgement_of_wisdom );
}

// trigger_focus_magic_feedback =============================================

static void trigger_focus_magic_feedback( spell_t* spell )
{
  struct expiration_t : public event_t
  {
    player_t* mage;

    expiration_t( sim_t* sim, player_t* p ) : event_t( sim, p ), mage( p -> buffs.focus_magic )
    {
      name = "Focus Magic Feedback Expiration";
      mage -> aura_gain( "Focus Magic Feedback" );
      mage -> buffs.focus_magic_feedback = 1;
      sim -> add_event( this, 10.0 );
    }
    virtual void execute()
    {
      mage -> aura_loss( "Focus Magic Feedback" );
      mage -> buffs.focus_magic_feedback = 0;
      mage -> expirations.focus_magic_feedback = 0;
    }
  };

  player_t* p = spell -> player;

  if( ! p -> buffs.focus_magic ) return;

  event_t*& e = p -> buffs.focus_magic -> expirations.focus_magic_feedback;

  if( e )
  {
    e -> reschedule( 10.0 );
  }
  else
  {
    e = new ( p -> sim ) expiration_t( p -> sim, p );
  }
}

} // ANONYMOUS NAMESPACE ===================================================

// ==========================================================================
// Player - Gear
// ==========================================================================

// player_t::gear_t::allocate_spell_power_budget( sim_t* sim ) ==============

void player_t::gear_t::allocate_spell_power_budget( sim_t* sim )
{
  if( sim -> debug ) report_t::log( sim, "Allocating spell_power budget...." );   

  if(  hit_rating   == 0 &&
       crit_rating  == 0 &&
       haste_rating == 0 )
  {
    spell_power[ SCHOOL_MAX ] = spell_power_budget;
    return;
  }

  if( budget_slots == 0 ) budget_slots = 16;

  double spell_power_statmod = 0.855;

  double slot_power = spell_power_budget / (double) budget_slots;
  double slot_hit   = hit_rating         / (double) budget_slots;
  double slot_crit  = crit_rating        / (double) budget_slots;
  double slot_haste = haste_rating       / (double) budget_slots;

  double target = pow( slot_power * spell_power_statmod, 1.5 );

  double cost = pow( slot_hit,   1.5 ) +
                pow( slot_crit,  1.5 ) +
                pow( slot_haste, 1.5 );

  spell_power[ SCHOOL_MAX ] = (int16_t) ( budget_slots * pow( target - cost, 1.0 / 1.5 ) / spell_power_statmod );
  
  if( sim -> debug ) report_t::log( sim, "slot_power=%.1f  slot_hit=%.1f  slot_crit=%.1f  slot_haste=%.1f  target=%.1f  cost=%.1f  spell_power=%d\n",
                   slot_power, slot_hit, slot_crit, slot_haste, target, cost, spell_power[ SCHOOL_MAX ] );
}

// player_t::gear_t::allocate_attack_power_budget( sim_t* sim ) =============

void player_t::gear_t::allocate_attack_power_budget( sim_t* sim )
{
  if( sim -> debug ) report_t::log( sim, "Allocating attack_power budget...." );   

  // double attack_power_statmod = 0.5;

  assert(0);
}

// ==========================================================================
// Player
// ==========================================================================

// player_t::player_t =======================================================

player_t::player_t( sim_t*             s, 
                    int8_t             t,
		    const std::string& n ) :
  sim(s), name_str(n), next(0), type(t), level(70), party(0), gcd_ready(0), base_gcd(1.5), sleeping(0), pet_list(0),
  // Haste
  base_haste_rating(0), initial_haste_rating(0), haste_rating(0), haste(1.0),
  // Spell Mechanics
  base_spell_power(0),
  base_spell_hit(0),         initial_spell_hit(0),         spell_hit(0),
  base_spell_crit(0),        initial_spell_crit(0),        spell_crit(0),
  base_spell_penetration(0), initial_spell_penetration(0), spell_penetration(0),
  base_mp5(0),               initial_mp5(0),               mp5(0),
  spell_power_per_intellect(0), initial_spell_power_per_intellect(0),
  spell_power_per_spirit(0),    initial_spell_power_per_spirit(0),
  spell_crit_per_intellect(0),  initial_spell_crit_per_intellect(0),
  mp5_per_intellect(0), spirit_regen_while_casting(0), 
  last_cast(0),
  // Attack Mechanics
  base_attack_power(0),       initial_attack_power(0),        attack_power(0),
  base_attack_hit(0),         initial_attack_hit(0),          attack_hit(0),
  base_attack_expertise(0),   initial_attack_expertise(0),    attack_expertise(0),
  base_attack_crit(0),        initial_attack_crit(0),         attack_crit(0),
  base_attack_penetration(0), initial_attack_penetration(0),  attack_penetration(0),
  attack_power_per_strength(0), initial_attack_power_per_strength(0),
  attack_power_per_agility(0),  initial_attack_power_per_agility(0),
  attack_crit_per_agility(0),   initial_attack_crit_per_agility(0),
  position( POSITION_BACK ),
  // Resources 
  resource_constrained(0),           resource_constrained_count(0),
  resource_constrained_total_dmg(0), resource_constrained_total_time(0),
  mana_per_intellect(0), health_per_stamina(0),
  // Consumables
  elixir_guardian( ELIXIR_NONE ),
  elixir_battle( ELIXIR_NONE ),
  flask( FLASK_NONE ),
  // Events
  executing(0), channeling(0),
  // Actions
  action_list(0),
  // Reporting
  quiet(0), last_foreground_action(0),
  last_action(0), total_seconds(0), total_waiting(0), iteration_dmg(0), total_dmg(0), 
  dps(0), dps_min(0), dps_max(0), dps_std_dev(0), dps_error(0), dpr(0), rps_gain(0), rps_loss(0),
  proc_list(0), gain_list(0), stats_list(0), uptime_list(0)
{
  if( sim -> debug ) report_t::log( sim, "Creating Player %s", name() );
  next = sim -> player_list;
  sim -> player_list = this;

  for( int i=0; i < ATTRIBUTE_MAX; i++ )
  {
    attribute[ i ] = attribute_base[ i ] = attribute_initial[ i ] = 0;

    attribute_multiplier[ i ] = attribute_multiplier_initial[ i ] = 1.0;
  }
  for( int i=0; i <= SCHOOL_MAX; i++ )
  {
    initial_spell_power[ i ] = spell_power[ i ] = 0;
  }
  for( int i=0; i < RESOURCE_MAX; i++ )
  {
    resource_base[ i ] = resource_initial[ i ] = resource_max[ i ] = resource_current[ i ] = 0;

    resource_lost[ i ] = resource_gained[ i ] = 0;
  }
}

// player_t::~player_t =====================================================

player_t::~player_t() 
{
  while( action_t* a = action_list )
  {
    action_list = a -> next;
    delete a;
  }
  while( proc_t* p = proc_list )
  {
    proc_list = p -> next;
    delete p;
  }
  while( gain_t* g = gain_list )
  {
    gain_list = g -> next;
    delete g;
  }
  while( stats_t* s = stats_list )
  {
    stats_list = s -> next;
    delete s;
  }
  while( uptime_t* u = uptime_list )
  {
    uptime_list = u -> next;
    delete u;
  }
}

// player_t::init ==========================================================

void player_t::init() 
{
  if( sim -> debug ) report_t::log( sim, "Initializing player %s", name() );   

  init_rating();
  init_base();
  init_core();
  init_spell();
  init_attack();
  init_weapon( &main_hand_weapon, main_hand_str );
  init_weapon(  &off_hand_weapon,  off_hand_str ); off_hand_weapon.main = false;
  init_weapon(    &ranged_weapon,    ranged_str );
  init_resources();
  init_consumables();
  init_actions();
  init_stats();
}
 
// player_t::init_core ======================================================

void player_t::init_core() 
{
  if( initial_haste_rating == 0 ) initial_haste_rating = base_haste_rating + gear.haste_rating + gear.haste_rating_enchant;

  for( int i=0; i < ATTRIBUTE_MAX; i++ )
  {
    if( attribute_initial[ i ] == 0 )
    {
      int16_t value = gear.attribute[ i ] + gear.attribute_enchant[ i ];
      if( value == 0 ) value = sim -> gear_default.attribute[ i ];
      value += sim -> gear_delta.attribute[ i ];
      attribute_initial[ i ] = attribute_base[ i ] + value;
    }
    attribute[ i ] = attribute_initial[ i ];
  }
}

// player_t::init_spell =====================================================

void player_t::init_spell() 
{
  if( gear.spell_power_budget != 0 &&
      gear.spell_power[ SCHOOL_MAX ] == 0 ) 
  {
    gear.allocate_spell_power_budget( sim );
  }

  if( initial_spell_power[ SCHOOL_MAX ] == 0 )
  {
    for( int i=0; i <= SCHOOL_MAX; i++ )
    {
      initial_spell_power[ i ] = gear.spell_power[ i ] + gear.spell_power_enchant[ i ];
    }
    if( initial_spell_power[ SCHOOL_MAX ] == 0 )
    {
      initial_spell_power[ SCHOOL_MAX ] = sim -> gear_default.spell_power;
    }
    initial_spell_power[ SCHOOL_MAX ] += sim -> gear_delta.spell_power;
    initial_spell_power[ SCHOOL_MAX ] += base_spell_power;
  }

  if( initial_spell_hit == 0 )
  {
    int16_t value = gear.hit_rating + gear.hit_rating_enchant;
    if( value == 0 ) value = sim -> gear_default.hit_rating;
    value += sim -> gear_delta.hit_rating;
    initial_spell_hit = base_spell_hit + value / rating.spell_hit;
  }
  if( initial_spell_crit == 0 )
  {
    int16_t value = gear.crit_rating + gear.crit_rating_enchant;
    if( value == 0 ) value = sim -> gear_default.crit_rating;
    value += sim -> gear_delta.crit_rating;
    initial_spell_crit = base_spell_crit + value / rating.spell_crit;
  }
  if( initial_spell_penetration == 0 )
  {
    initial_spell_penetration = ( base_spell_penetration +   
				  gear.spell_penetration + 
				  gear.spell_penetration_enchant );
  }
  if( initial_mp5 == 0 ) 
  {
    initial_mp5 = base_mp5 + gear.mp5 + gear.mp5_enchant;

    // FIXME! All these static buffs will go away as class support comes along.
    if( buffs.blessing_of_wisdom ) initial_mp5 += ( level <= 70 ) ? 49 : 91;
  }
}

// player_t::init_attack ====================================================

void player_t::init_attack() 
{
  if( gear.attack_power_budget != 0 &&
      gear.attack_power == 0 ) 
  {
    gear.allocate_attack_power_budget( sim );
  }

  if( initial_attack_power == 0 )
  {
    int16_t value = gear.attack_power + gear.attack_power_enchant;
    if( value == 0 ) value = sim -> gear_default.attack_power;
    value += sim -> gear_delta.attack_power;
    initial_attack_power = base_attack_power + value;
  }
  if( initial_attack_hit == 0 )
  {
    int16_t value = gear.hit_rating + gear.hit_rating_enchant;
    if( value == 0 ) value = sim -> gear_default.hit_rating;
    value += sim -> gear_delta.hit_rating;
    initial_attack_hit = base_attack_hit + value / rating.attack_hit;
  }
  if( initial_attack_crit == 0 )
  {
    int16_t value = gear.crit_rating + gear.crit_rating_enchant;
    if( value == 0 ) value = sim -> gear_default.crit_rating;
    value += sim -> gear_delta.crit_rating;
    initial_attack_crit = base_attack_crit + value / rating.attack_crit;
  }
  if( initial_attack_expertise == 0 )
  {
    int16_t value = gear.expertise_rating + gear.expertise_rating_enchant;
    if( value == 0 ) value = sim -> gear_default.expertise_rating;
    value += sim -> gear_delta.expertise_rating;
    initial_attack_expertise = base_attack_expertise + value / rating.expertise;
  }
  if( initial_attack_penetration == 0 )
  {
    int16_t value = gear.armor_penetration_rating + gear.armor_penetration_rating_enchant;
    if( value == 0 ) value = sim -> gear_default.armor_penetration_rating;
    value += sim -> gear_delta.armor_penetration_rating;
    initial_attack_penetration = base_attack_penetration + value / rating.armor_penetration;
  }

}

// player_t::init_weapon ===================================================

void player_t::init_weapon( weapon_t*    w, 
			    std::string& encoding )
{
  if( encoding.empty() ) return;

  std::vector<std::string> splits;
  int size = util_t::string_split( splits, encoding, "," );

  for( int i=0; i < size; i++ )
  {
    std::string& s = splits[ i ];
    int t;

    for( t=0; t < WEAPON_MAX; t++ )
    {
      const char* name = util_t::weapon_type_string( t );
      if( s == name ) break;
    }

    if( t < WEAPON_MAX )
    {
      w -> type = t;
    }
    else
    {
      std::string parm, value;
      bool invalid = false;

      if( 2 != util_t::string_split( s, "=", "S S", &parm, &value ) )
      {
	invalid = true;
      }
      if( parm == "dmg" || parm == "damage" )
      {
	w -> damage = atof( value.c_str() );
	assert( w -> damage != 0 );
      }
      else if( parm == "speed" )
      {
	w -> swing_time = atof( value.c_str() );
	assert( w -> swing_time != 0 );
      }
      else if( parm == "school" )
      {
	for( int j=0; j <= SCHOOL_MAX; j++ )
	{
	  if( j == SCHOOL_MAX ) invalid = true;
	  if( value == util_t::school_type_string( j ) )
	  {
	    w -> school = j;
	    break;
	  }
	}
      }
      else if( parm == "enchant" )
      {
	for( int j=0; j <= WEAPON_ENCHANT_MAX; j++ )
	{
	  if( j == WEAPON_ENCHANT_MAX ) invalid = true;
	  if( value == util_t::weapon_enchant_type_string( j ) )
	  {
	    w -> enchant = j;
	    break;
	  }
	}
      }
      else if( parm == "buff" )
      {
	for( int j=0; j <= WEAPON_BUFF_MAX; j++ )
	{
	  if( j == WEAPON_BUFF_MAX ) invalid = true;
	  if( value == util_t::weapon_buff_type_string( j ) )
	  {
	    w -> buff = j;
	    break;
	  }
	}
      }
      else invalid = true;

      if( invalid )
      {
	printf( "Invalid weapon encoding: %s\n", encoding.c_str() );
	assert(0);
      }
    }
  }

  if( w == &main_hand_weapon ) assert( w -> type >= WEAPON_NONE && w -> type < WEAPON_2H );
  if( w ==  &off_hand_weapon ) assert( w -> type >= WEAPON_NONE && w -> type < WEAPON_1H );
  if( w ==    &ranged_weapon ) assert( w -> type == WEAPON_NONE || ( w -> type > WEAPON_2H && w -> type < WEAPON_RANGED ) );
}

// player_t::init_resources ================================================

void player_t::init_resources( bool force ) 
{
  double resource_bonus[ RESOURCE_MAX ];
  for( int i=0; i < RESOURCE_MAX; i++ ) resource_bonus[ i ] = 0;

  double ci = intellect();
  double cs = stamina();

  resource_bonus[ RESOURCE_MANA   ] = ( ci - 20 ) * mana_per_intellect + 20;
  resource_bonus[ RESOURCE_HEALTH ] = ( cs - 20 ) * health_per_stamina + 20;
  
  for( int i=0; i < RESOURCE_MAX; i++ )
  {
    if( force || resource_initial[ i ] == 0 )
    {
      resource_initial[ i ] = resource_base[ i ] + resource_bonus[ i ] + gear.resource[ i ] + gear.resource_enchant[ i ];
    }
    resource_current[ i ] = resource_max[ i ] = resource_initial[ i ];
  }

  resource_constrained = 0;
  resource_constrained_count = 0;
  resource_constrained_total_dmg = 0;
  resource_constrained_total_time = 0;
}

// player_t::init_consumables ==============================================

void player_t::init_consumables()
{
  consumable_t::init_flask  ( this );
  consumable_t::init_elixirs( this );
  consumable_t::init_food   ( this );
}

// player_t::init_actions ==================================================

void player_t::init_actions() 
{
  if( ! action_list_prefix.empty() || 
      ! action_list_str.empty()    || 
      ! action_list_postfix.empty() )
  {
    std::vector<std::string> splits;
    int size = 0;

    if( ! action_list_prefix.empty() )
    {
      if( sim -> debug ) report_t::log( sim, "Player %s: action_list_prefix=%s", name(), action_list_prefix.c_str() );   
      size = util_t::string_split( splits, action_list_prefix, "/" );
    }
    if( ! action_list_str.empty() )
    {
      if( sim -> debug ) report_t::log( sim, "Player %s: action_list_str=%s", name(), action_list_str.c_str() );   
      size = util_t::string_split( splits, action_list_str, "/" );
    }
    if( ! action_list_postfix.empty() )
    {
      if( sim -> debug ) report_t::log( sim, "Player %s: action_list_postfix=%s", name(), action_list_postfix.c_str() );   
      size = util_t::string_split( splits, action_list_postfix, "/" );
    }

    for( int i=0; i < size; i++ )
    {
      std::string action_name    = splits[ i ];
      std::string action_options = "";

      std::string::size_type cut_pt = action_name.find_first_of( "," );       

      if( cut_pt != action_name.npos )
      {
	action_options = action_name.substr( cut_pt + 1 );
	action_name    = action_name.substr( 0, cut_pt );
      }

      if( ! action_t::create_action( this, action_name, action_options ) )
      {
	printf( "util_t::player: Unknown action: %s\n", splits[ i ].c_str() );
	assert( false );
      }
    }
  }

  if( ! action_list_skip.empty() )
  {
    if( sim -> debug ) report_t::log( sim, "Player %s: action_list_skip=%s", name(), action_list_skip.c_str() );   
    std::vector<std::string> splits;
    int size = util_t::string_split( splits, action_list_skip, "/" );
    for( int i=0; i < size; i++ )
    {
      action_t* action = find_action( splits[ i ] );
      if( action ) action -> background = true;
    }
  }
}

// player_t::init_rating ===================================================

void player_t::init_rating() 
{
  rating.init( level );
}

// player_t::init_stats ====================================================

void player_t::init_stats() 
{
  for( int i=0; i < RESOURCE_MAX; i++ )
  {
    resource_lost[ i ] = resource_gained[ i ] = 0;
  }

  gains.ashtongue_talisman    = get_gain( "ashtongue_talisman" );
  gains.dark_rune             = get_gain( "dark_rune" );
  gains.innervate             = get_gain( "innervate" );
  gains.judgement_of_wisdom   = get_gain( "judgement_of_wisdom" );
  gains.mana_gem              = get_gain( "mana_gem" );
  gains.mana_potion           = get_gain( "mana_potion" );
  gains.mana_spring           = get_gain( "mana_spring" );
  gains.mana_tide             = get_gain( "mana_tide" );
  gains.mark_of_defiance      = get_gain( "mark_of_defiance" );
  gains.mp5_regen             = get_gain( "mp5" );
  gains.replenishment         = get_gain( "replenishment" );
  gains.restore_mana          = get_gain( "restore_mana" );
  gains.spellsurge            = get_gain( "spellsurge" );
  gains.spirit_regen          = get_gain( "spirit" );
  gains.vampiric_touch        = get_gain( "vampiric_touch" );
  gains.water_elemental_regen = get_gain( "water_elemental" );
  gains.tier4_2pc             = get_gain( "tier4_2pc" );
  gains.tier4_4pc             = get_gain( "tier4_4pc" );
  gains.tier5_2pc             = get_gain( "tier5_2pc" );
  gains.tier5_4pc             = get_gain( "tier5_4pc" );
  gains.tier6_2pc             = get_gain( "tier6_2pc" );
  gains.tier6_4pc             = get_gain( "tier6_4pc" );

  procs.ashtongue_talisman           = get_proc( "ashtongue_talisman" );
  procs.elder_scribes                = get_proc( "elder_scribes" );
  procs.eternal_sage                 = get_proc( "eternal_sage" );
  procs.eye_of_magtheridon           = get_proc( "eye_of_magtheridon" );
  procs.judgement_of_wisdom          = get_proc( "judgement_of_wisdom" );
  procs.lightning_capacitor          = get_proc( "lightning_capacitor" );
  procs.mark_of_defiance             = get_proc( "mark_of_defiance" );
  procs.mystical_skyfire             = get_proc( "mystical_skyfire" );
  procs.quagmirrans_eye              = get_proc( "quagmirrans_eye" );
  procs.sextant_of_unstable_currents = get_proc( "sextant_of_unstable_currents" );
  procs.shiffars_nexus_horn          = get_proc( "shiffars_nexus_horn" );
  procs.spellstrike                  = get_proc( "spellstrike" );
  procs.timbals_crystal              = get_proc( "timbals_crystal" );
  procs.windfury                     = get_proc( "windfury" );
  procs.wrath_of_cenarius            = get_proc( "wrath_of_cenarius" );
  procs.tier4_2pc                    = get_proc( "tier4_2pc" );
  procs.tier4_4pc                    = get_proc( "tier4_4pc" );
  procs.tier5_2pc                    = get_proc( "tier5_2pc" );
  procs.tier5_4pc                    = get_proc( "tier5_4pc" );
  procs.tier6_2pc                    = get_proc( "tier6_2pc" );
  procs.tier6_4pc                    = get_proc( "tier6_4pc" );
}

// player_t::composite_attack_power ========================================

double player_t::composite_attack_power() 
{
  double ap = attack_power;

  ap += attack_power_per_strength * strength();
  ap += attack_power_per_agility  * agility();

  double best_buff=0;
  if( buffs.blessing_of_might > best_buff ) best_buff = buffs.blessing_of_might;
  if( buffs.battle_shout      > best_buff ) best_buff = buffs.battle_shout;
  ap += best_buff;

  return ap;
}

// player_t::composite_attack_crit =========================================

double player_t::composite_attack_crit()
{
  return attack_crit + attack_crit_per_agility * agility();
}

// player_t::composite_spell_power ========================================

double player_t::composite_spell_power( int8_t school ) 
{
  double sp = spell_power[ school ];

  if( school == SCHOOL_FROSTFIRE )
  {
    sp = std::max( spell_power[ SCHOOL_FROST ],
		   spell_power[ SCHOOL_FIRE  ] );
  }

  if( school != SCHOOL_MAX ) sp += spell_power[ SCHOOL_MAX ];

  sp += spell_power_per_intellect * intellect();
  sp += spell_power_per_spirit    * spirit();

  return sp;
}

// player_t::composite_spell_crit ==========================================

double player_t::composite_spell_crit()
{
  return spell_crit + spell_crit_per_intellect * intellect();
}

// player_t::composite_attack_power_multiplier =============================

double player_t::composite_attack_power_multiplier()
{
  return buffs.unleashed_rage ? 1.10 : 1.0;
}

// player_t::composite_attribute_multiplier ================================

double player_t::composite_attribute_multiplier( int8_t attr )
{
  double m = attribute_multiplier[ attr ]; 
  if( buffs.blessing_of_kings ) m *= 1.10; 
  return m;
}

// player_t::attribute() ===================================================

double player_t::strength()  { return composite_attribute_multiplier( ATTR_STRENGTH  ) * attribute[ ATTR_STRENGTH  ]; }
double player_t::agility()   { return composite_attribute_multiplier( ATTR_AGILITY   ) * attribute[ ATTR_AGILITY   ]; }
double player_t::stamina()   { return composite_attribute_multiplier( ATTR_STAMINA   ) * attribute[ ATTR_STAMINA   ]; }
double player_t::intellect() { return composite_attribute_multiplier( ATTR_INTELLECT ) * attribute[ ATTR_INTELLECT ]; }
double player_t::spirit()    { return composite_attribute_multiplier( ATTR_SPIRIT    ) * attribute[ ATTR_SPIRIT    ]; }

// player_t::reset =========================================================

void player_t::reset() 
{
  if( sim -> debug ) report_t::log( sim, "Reseting player %s", name() );   

  last_cast = 0;
  gcd_ready = 0;

  haste_rating = initial_haste_rating;
  recalculate_haste();

  for( int i=0; i < ATTRIBUTE_MAX; i++ )
  {
    attribute           [ i ] = attribute_initial           [ i ];
    attribute_multiplier[ i ] = attribute_multiplier_initial[ i ];
  }

  for( int i=0; i <= SCHOOL_MAX; i++ )
  {
    spell_power[ i ] = initial_spell_power[ i ];
  }

  spell_hit         = initial_spell_hit;
  spell_crit        = initial_spell_crit;
  spell_penetration = initial_spell_penetration;
  mp5               = initial_mp5;

  attack_power       = initial_attack_power;
  attack_hit         = initial_attack_hit;
  attack_expertise   = initial_attack_expertise;
  attack_crit        = initial_attack_crit;
  attack_penetration = initial_attack_penetration;
  
  spell_power_per_intellect = initial_spell_power_per_intellect;
  spell_power_per_spirit    = initial_spell_power_per_spirit;
  spell_crit_per_intellect  = initial_spell_crit_per_intellect;
  
  attack_power_per_strength = initial_attack_power_per_strength;
  attack_power_per_agility  = initial_attack_power_per_agility;
  attack_crit_per_agility   = initial_attack_crit_per_agility;

  last_foreground_action = 0;
  last_action = 0;

  for( int i=0; i < RESOURCE_MAX; i++ )
  {
    resource_current[ i ] = resource_max[ i ] = resource_initial[ i ];
  }
  resource_constrained = 0;

  executing = 0;
  channeling = 0;
  iteration_dmg = 0;
 
  main_hand_weapon.buff = WEAPON_BUFF_NONE;
   off_hand_weapon.buff = WEAPON_BUFF_NONE;
     ranged_weapon.buff = WEAPON_BUFF_NONE;

  main_hand_weapon.buff_bonus = 0;
   off_hand_weapon.buff_bonus = 0;
     ranged_weapon.buff_bonus = 0;

  elixir_battle   = ELIXIR_NONE;
  elixir_guardian = ELIXIR_NONE;
  flask           = FLASK_NONE;

  buffs.reset();
  expirations.reset();
  cooldowns.reset();
  
  if( action_list )
  {
    for( action_t* a = action_list; a; a = a -> next ) 
    {
      a -> reset();
    }
    if( type != PLAYER_PET && ! sleeping ) schedule_ready();
  }

  if( sleeping ) quiet = 1;
}

// player_t::schedule_ready =================================================

void player_t::schedule_ready( double delta_time,
			       bool   waiting )
{
  executing = 0;
  channeling = 0;

  double gcd_adjust = gcd_ready - ( sim -> current_time + delta_time );
  if( gcd_adjust > 0 ) delta_time += gcd_adjust;

  if( waiting )
  {
    total_waiting += delta_time;
  }
  else
  {
    double lag = 0;

    if( type == PLAYER_PET )
    {
      lag = sim -> pet_lag;
    }
    else
    {
      lag = sim -> lag;

      if( last_foreground_action && last_foreground_action -> channeled ) 
      {
	lag += sim -> channel_penalty;
      }

      if( gcd_adjust > 0 ) 
      {
	lag += sim -> gcd_penalty;
      }
    }

    if( lag > 0 ) lag += ( ( rand() % 11 ) - 5 ) * 0.01;
    if( lag < 0 ) lag = 0;

    delta_time += lag;
  }

  if( last_foreground_action ) 
  {
    last_foreground_action -> stats -> total_execute_time += delta_time;
  }
  
  new ( sim ) player_ready_event_t( sim, this, delta_time );
}

// player_t::execute_action =================================================

action_t* player_t::execute_action()
{
  action_t* action=0;

  for( action = action_list; action; action = action -> next )
  {
    if( action -> background )
      continue;

    if( action -> ready() )
      break;
  }

  if( action ) 
  {
    action -> schedule_execute();
  }
  else
  {
    check_resources();
  }

  last_foreground_action = action;

  return action;
}

// player_t::regen =========================================================

void player_t::regen( double periodicity )
{
  double spirit_regen = periodicity * spirit_regen_per_second();

  if( buffs.innervate )
  {
    spirit_regen *= 4.0;

    resource_gain( RESOURCE_MANA, spirit_regen, gains.innervate );
  }
  else if( recent_cast() )
  {
    spirit_regen *= spirit_regen_while_casting;

    resource_gain( RESOURCE_MANA, spirit_regen, gains.spirit_regen );
  }

  double mp5_regen = periodicity * ( mp5 + intellect() * mp5_per_intellect ) / 5.0;

  resource_gain( RESOURCE_MANA, mp5_regen, gains.mp5_regen );

  if( buffs.replenishment )
  {
    double replenishment_regen = periodicity * resource_max[ RESOURCE_MANA ] * 0.0025 / 1.0;

    resource_gain( RESOURCE_MANA, replenishment_regen, gains.replenishment );
  }

  if( buffs.water_elemental_regen )
  {
    double water_elemental_regen = periodicity * resource_max[ RESOURCE_MANA ] * 0.006 / 5.0;

    resource_gain( RESOURCE_MANA, water_elemental_regen, gains.water_elemental_regen );
  }
}

// player_t::resource_loss =================================================

void player_t::resource_loss( int8_t resource,
                              double amount )
{
  if( amount == 0 ) return;

  if( sim -> infinite_resource[ resource ] == 0 )
  {
    resource_current[ resource ] -= amount;
  }

  resource_lost[ resource ] += amount;

  if( resource == RESOURCE_MANA )
  {
    last_cast = sim -> current_time;
  }

  if( sim -> debug ) report_t::log( sim, "Player %s spends %.0f %s", name(), amount, util_t::resource_type_string( resource ) );
}

// player_t::resource_gain =================================================

void player_t::resource_gain( int8_t  resource,
                              double  amount,
                              gain_t* source )
{
  if( sleeping ) return;

  double actual_amount = std::min( amount, resource_max[ resource ] - resource_current[ resource ] );
  if( actual_amount > 0 )
  {
    resource_current[ resource ] += actual_amount;
  }

  resource_gained [ resource ] += actual_amount;
  if( source ) source -> add( actual_amount );

  if( sim -> log ) 
    report_t::log( sim, "%s gains %.0f (%.0f) %s from %s", 
                   name(), actual_amount, amount, util_t::resource_type_string( resource ), source ? source -> name() : "unknown" );
}

// player_t::resource_available ============================================

bool player_t::resource_available( int8_t resource,
				   double cost )
{
  if( resource == RESOURCE_NONE || cost == 0 )
  {
    return true;
  }

  return resource_current[ resource ] >= cost;
}

// player_t::check_resources ===============================================

void player_t::check_resources()
{
  return; // FIXME! Work on this later.

  if( ! resource_constrained )
  {
    for( action_t* a = action_list; a; a = a -> next )
    {
      if( ! a -> background && a -> harmful )
      {
	if( resource_available( a -> resource, a -> cost() ) )
	{
	  // Not resource constrained.
	  return;
	}
      }
    }

    resource_constrained = 1;
    resource_constrained_count++;
    resource_constrained_total_time += sim -> current_time;
    resource_constrained_total_dmg  += iteration_dmg;

    if( sim -> debug ) report_t::log( sim, "Player %s is resource constrained.", name() );
  }
}

// player_t::summon_pet =====================================================

void player_t::summon_pet( const std::string& pet_name )
{
  for( pet_t* p = pet_list; p; p = p -> next_pet )
  {
    if( p -> name_str == pet_name )
    {
      p -> summon();
      return;
    }
  }
  assert(0);
}

// player_t::dismiss_pet ====================================================

void player_t::dismiss_pet( const std::string& pet_name )
{
  for( pet_t* p = pet_list; p; p = p -> next_pet )
  {
    if( p -> name_str == pet_name )
    {
      p -> dismiss();
      return;
    }
  }
  assert(0);
}

// player_t::action_start ===================================================

void player_t::action_start( action_t* action )
{
  gcd_ready = sim -> current_time + action -> gcd();

  if( ! action -> background )
  {
    executing = action -> event;
  }

  if( action -> type == ACTION_SPELL )
  {
    unique_gear_t::spell_start_event( (spell_t*) action );
        enchant_t::spell_start_event( (spell_t*) action );
                   spell_start_event( (spell_t*) action );
  }
  else if( action -> type == ACTION_ATTACK )
  {
    unique_gear_t::attack_start_event( (attack_t*) action );
        enchant_t::attack_start_event( (attack_t*) action );
                   attack_start_event( (attack_t*) action );
  }
}

// player_t::action_miss ====================================================

void player_t::action_miss( action_t* action )
{
  if( action -> type == ACTION_SPELL )
  {
    unique_gear_t::spell_miss_event( (spell_t*) action );
        enchant_t::spell_miss_event( (spell_t*) action );
                   spell_miss_event( (spell_t*) action );
  }
  else if( action -> type == ACTION_ATTACK )
  {
    unique_gear_t::attack_miss_event( (attack_t*) action );
        enchant_t::attack_miss_event( (attack_t*) action );
                   attack_miss_event( (attack_t*) action );
  }
}

// player_t::action_hit =====================================================

void player_t::action_hit( action_t* action )
{
  if( action -> type == ACTION_SPELL )
  {
    unique_gear_t::spell_hit_event( (spell_t*) action );
        enchant_t::spell_hit_event( (spell_t*) action );
                   spell_hit_event( (spell_t*) action );
  }
  else if( action -> type == ACTION_ATTACK )
  {
    unique_gear_t::attack_hit_event( (attack_t*) action );
        enchant_t::attack_hit_event( (attack_t*) action );
                   attack_hit_event( (attack_t*) action );
  }
}

// player_t::action_tick ====================================================

void player_t::action_tick( action_t* action )
{
  if( action -> type == ACTION_SPELL )
  {
    unique_gear_t::spell_tick_event( (spell_t*) action );
        enchant_t::spell_tick_event( (spell_t*) action );
                   spell_tick_event( (spell_t*) action );
  }
  else if( action -> type == ACTION_ATTACK )
  {
    unique_gear_t::attack_tick_event( (attack_t*) action );
        enchant_t::attack_tick_event( (attack_t*) action );
	           attack_tick_event( (attack_t*) action );
  }
}

// player_t::action_damage ==================================================

void player_t::action_damage( action_t* action, 
			      double    amount,
			      int8_t    dmg_type )
{
  if( action -> type == ACTION_SPELL )
  {
    unique_gear_t::spell_damage_event( (spell_t*) action, amount, dmg_type );
        enchant_t::spell_damage_event( (spell_t*) action, amount, dmg_type );
                   spell_damage_event( (spell_t*) action, amount, dmg_type );
  }
  else if( action -> type == ACTION_ATTACK )
  {
    unique_gear_t::attack_damage_event( (attack_t*) action, amount, dmg_type );
        enchant_t::attack_damage_event( (attack_t*) action, amount, dmg_type );
                   attack_damage_event( (attack_t*) action, amount, dmg_type );
  }
}

// player_t::action_finish ==================================================

void player_t::action_finish( action_t* action )
{
  if( action -> type == ACTION_SPELL )
  {
    unique_gear_t::spell_finish_event( (spell_t*) action );
        enchant_t::spell_finish_event( (spell_t*) action );
                   spell_finish_event( (spell_t*) action );
  }
  else if( action -> type == ACTION_ATTACK )
  {
    unique_gear_t::attack_finish_event( (attack_t*) action );
        enchant_t::attack_finish_event( (attack_t*) action );
                   attack_finish_event( (attack_t*) action );
  }
}

// player_t::spell_start_event ==============================================

void player_t::spell_start_event( spell_t* spell )
{
}

// player_t::spell_miss_event ===============================================

void player_t::spell_miss_event( spell_t* spell )
{
}

// player_t::spell_hit_event ================================================

void player_t::spell_hit_event( spell_t* spell )
{
  trigger_judgement_of_wisdom( spell );

  if ( spell -> result == RESULT_CRIT )
  {
    trigger_focus_magic_feedback( spell );
  }
}

// player_t::spell_tick_event ===============================================

void player_t::spell_tick_event( spell_t* spell )
{
}

// player_t::spell_damage_event =============================================

void player_t::spell_damage_event( spell_t* spell, 
				   double   amount,
				   int8_t   dmg_type )
{
}

// player_t::spell_finish_event =============================================

void player_t::spell_finish_event( spell_t* spell )
{
}

// player_t::attack_start_event =============================================

void player_t::attack_start_event( attack_t* attack )
{
}

// player_t::attack_miss_event ==============================================

void player_t::attack_miss_event( attack_t* attack )
{
}

// player_t::attack_hit_event ===============================================

void player_t::attack_hit_event( attack_t* attack )
{
  trigger_judgement_of_wisdom( attack );
}

// player_t::attack_tick_event ==============================================

void player_t::attack_tick_event( attack_t* attack )
{
}

// player_t::attack_damage_event ============================================

void player_t::attack_damage_event( attack_t* attack, 
				    double   amount,
				    int8_t   dmg_type )
{
}

// player_t::attack_finish_event ============================================

void player_t::attack_finish_event( attack_t* attack )
{
}

// player_t::share_cooldown =================================================

void player_t::share_cooldown( const std::string& group,
			       double             duration )
{
  double ready = sim -> current_time + duration;

  for( action_t* a = action_list; a; a = a -> next )
  {
    if( group == a -> cooldown_group )
    {
      if( ready > a -> cooldown_ready )
      {
	a -> cooldown_ready = ready;
      }
    }
  }
}

// player_t::share_duration =================================================

void player_t::share_duration( const std::string& group,
			       double             ready )
{
  for( action_t* a = action_list; a; a = a -> next )
  {
    if( a -> duration_group == group )
    {
      if( a -> duration_ready < ready )
      {
	a -> duration_ready = ready;
      }
    }
  }
}

// player_t::recent_cast ====================================================

bool player_t::recent_cast()
{
  return ( last_cast > 0 ) && ( ( last_cast + 5.0 ) > sim -> current_time );
}

// player_t::find_action ====================================================

action_t* player_t::find_action( const std::string& str )
{
  for( action_t* a = action_list; a; a = a -> next )
    if( str == a -> name_str )
      return a;

  return 0;
}

// player_t::calculate_spirit_regen =========================================

double player_t::spirit_regen_per_second()
{
  double base_60 = 0.011000;
  double base_70 = 0.009327;
  double base_80 = 0.005575;

  double base_regen = rating_t::interpolate( level, base_60, base_70, base_80 );

  double mana_per_second  = sqrt( intellect() ) * spirit() * base_regen;

  return mana_per_second;
}

// player_t::init_mana_costs ================================================

void player_t::init_mana_costs( rank_t* rank_list )
{
   for( int i=0; rank_list[ i ].level; i++ )
   {
     rank_t& r = rank_list[ i ];

     // Look for ranks in which the cost of an action is a percentage of base mana
     if( r.cost > 0 && r.cost < 1 )
     {
       r.cost *= resource_base[ RESOURCE_MANA ];
     }
   }
}

// player_t::aura_gain ======================================================

void player_t::aura_gain( const char* aura_name )
{
  // FIXME! Aura-tracking here.

  if( sim -> log ) report_t::log( sim, "Player %s gains %s", name(), aura_name );
}

// player_t::aura_loss ======================================================

void player_t::aura_loss( const char* aura_name )
{
  // FIXME! Aura-tracking here.

  if( sim -> log ) report_t::log( sim, "Player %s loses %s", name(), aura_name );
}

// player_t::get_gain =======================================================

gain_t* player_t::get_gain( const std::string& name )
{
  gain_t* g=0;

  for( g = gain_list; g; g = g -> next )
  {
    if( g -> name_str == name )
      return g;
  }

  g = new gain_t( name );

  gain_t** tail = &gain_list;

  while( *tail && name > ( (*tail) -> name_str ) )
  {
    tail = &( (*tail) -> next );
  }

  g -> next = *tail;
  *tail = g;

  return g;
}

// player_t::get_proc =======================================================

proc_t* player_t::get_proc( const std::string& name )
{
  proc_t* p=0;

  for( p = proc_list; p; p = p -> next )
  {
    if( p -> name_str == name )
      return p;
  }

  p = new proc_t( name );

  proc_t** tail = &proc_list;

  while( *tail && name > ( (*tail) -> name_str ) )
  {
    tail = &( (*tail) -> next );
  }

  p -> next = *tail;
  *tail = p;

  return p;
}

// player_t::get_stats ======================================================

stats_t* player_t::get_stats( const std::string& n )
{
  stats_t* stats=0;

  for( stats = stats_list; stats; stats = stats -> next )
  {
    if( stats -> name_str == n )
      return stats;
  }

  if( ! stats )
  {
    stats = new stats_t( n, this );
    stats -> init();
    stats_t** tail= &stats_list;
    while( *tail && n > ( (*tail) -> name_str ) )
    {
      tail = &( (*tail) -> next );
    }
    stats -> next = *tail;
    *tail = stats;
  }

  return stats;
}

// player_t::get_uptime =====================================================

uptime_t* player_t::get_uptime( const std::string& name )
{
  uptime_t* u=0;

  for( u = uptime_list; u; u = u -> next )
  {
    if( u -> name_str == name )
      return u;
  }

  u = new uptime_t( name );

  uptime_t** tail = &uptime_list;

  while( *tail && name > ( (*tail) -> name_str ) )
  {
    tail = &( (*tail) -> next );
  }

  u -> next = *tail;
  *tail = u;

  return u;
}

// Wait Until Ready Action ===================================================

struct wait_until_ready_t : public action_t
{
  wait_until_ready_t( player_t* player, const std::string& options_str ) : 
    action_t( ACTION_OTHER, "wait", player )
  {
  }

  virtual void execute() 
  {
    trigger_gcd = 0.1;
    
    for( action_t* a = player -> action_list; a; a = a -> next )
    {
      if( a -> background ) continue;
      
      if( a -> cooldown_ready > sim -> current_time )
      {
	double delta_time = a -> cooldown_ready - sim -> current_time;
	if( delta_time > trigger_gcd ) trigger_gcd = delta_time;
      }

      if( a -> duration_ready > sim -> current_time )
      {
	double delta_time = a -> duration_ready - ( sim -> current_time + a -> execute_time() );
	if( delta_time > trigger_gcd ) trigger_gcd = delta_time;
      }
    }

    player -> total_waiting += trigger_gcd;
  }
};

// Restore Mana Action =====================================================

struct restore_mana_t : public action_t
{
  double mana;

  restore_mana_t( player_t* player, const std::string& options_str ) : 
    action_t( ACTION_OTHER, "restore_mana", player ), mana(0)
  {
    option_t options[] =
    {
      { "mana", OPT_FLT, &mana },
      { NULL }
    };
    parse_options( options, options_str );

    trigger_gcd = 0;
  }

  virtual void execute() 
  {
    double mana_missing = player -> resource_max[ RESOURCE_MANA ] - player -> resource_current[ RESOURCE_MANA ];
    double mana_gain = mana;

    if( mana_gain == 0 || mana_gain > mana_missing ) mana_gain = mana_missing;

    if( mana_gain > 0 )
    {
      player -> resource_gain( RESOURCE_MANA, mana_gain, player -> gains.restore_mana );
    }
  }
};

// player_t::create_action ==================================================

action_t* player_t::create_action( const std::string& name,
				   const std::string& options_str )
{
  if( name == "restore_mana" ) return new     restore_mana_t( this, options_str );
  if( name == "wait"         ) return new wait_until_ready_t( this, options_str );

  return 0;
}

// player_t::parse_wowhead ==================================================

const struct Initializer
{
  char key;
  int first, second;

  operator std::pair< const char, std::pair< int, int > >() const
  {
    return std::make_pair(key, std::make_pair(first, second));
  }
} initializers[] =
{
  { '0', 0, 0 },  { 'z', 0, 1 },  { 'M', 0, 2 },  { 'c', 0, 3 }, { 'm', 0, 4 },  { 'V', 0, 5 },
  { 'o', 1, 0 },  { 'k', 1, 1 },  { 'R', 1, 2 },  { 's', 1, 3 }, { 'a', 1, 4 },  { 'q', 1, 5 },
  { 'b', 2, 0 },  { 'd', 2, 1 },  { 'r', 2, 2 },  { 'f', 2, 3 }, { 'w', 2, 4 },  { 'i', 2, 5 },
  { 'h', 3, 0 },  { 'u', 3, 1 },  { 'G', 3, 2 },  { 'I', 3, 3 }, { 'N', 3, 4 },  { 'A', 3, 5 },
  { 'L', 4, 0 },  { 'p', 4, 1 },  { 'T', 4, 2 },  { 'j', 4, 3 }, { 'n', 4, 4 },  { 'y', 4, 5 },
  { 'x', 5, 0 },  { 't', 5, 1 },  { 'g', 5, 2 },  { 'e', 5, 3 }, { 'v', 5, 4 },  { 'E', 5, 5 } 
};
const int num_initializers = sizeof(initializers) / sizeof(Initializer);

void player_t::parse_wowhead( talent_translation_t translation[][3], const std::string& talent_string )
{
  std::map< char, std::pair< int, int > > wowhead_map(initializers, initializers + num_initializers);

  int tree = 0;
  int talent = 0;

  for( unsigned int i=1; i<talent_string.length(); i++ )
  {
    if ( talent_string[i] == 'Z' )
    {
      if ( tree == 2 )
      {
        fprintf( sim -> output_file, "Malformed talent string. Too many trees specified.\n" );
        assert(0);
      }
      tree++;
      talent = 0;
    }
    else if ( wowhead_map.count(talent_string[i]) == 0 )
    {
      fprintf( sim -> output_file, "Malformed talent string. No mapping known for character: '%c'\n", talent_string[i] );
      assert(0);
    }
    else
    {
      talent_translation_t& t = translation[talent  ][tree];
      talent_translation_t& s = translation[talent+1][tree];
      
      if ( t.index < 1 )
      {
        fprintf( sim -> output_file, "Malformed talent string. Too many characters for tree: %d\n", tree+1 );
        assert(0);
      }
      else
      {
        if ( t.address != NULL ) *( t.address ) = wowhead_map[talent_string[i]].first;
        if ( s.address != NULL ) *( s.address ) = wowhead_map[talent_string[i]].second;
      }

      talent+=2;
    }
  }
}

// player_t::parse_talents ==================================================

void player_t::parse_talents( talent_translation_t* translation,
			      const std::string&    talent_string )
{
  for( int i=0; translation[ i ].index != 0; i++ )
  {
    talent_translation_t& t = translation[ i ];

    *( t.address ) = talent_string[ t.index - 1 ] - '0';
  }
}

// player_t::parse_option ===================================================

bool player_t::parse_option( const std::string& name,
			     const std::string& value )
{
  option_t options[] =
  {
    // Player - General
    { "level",                                OPT_INT8,   &( level                                          ) },
    { "party",                                OPT_INT8,   &( party                                          ) },
    { "gcd",                                  OPT_FLT,    &( base_gcd                                       ) },
    { "sleeping",                             OPT_INT8,   &( sleeping                                       ) },
    // Player - Haste										          
    { "haste_rating",                         OPT_INT16,  &( initial_haste_rating                           ) },
    // Player - Attributes									          
    { "strength",                             OPT_FLT,    &( attribute_initial[ ATTR_STRENGTH  ]            ) },
    { "agility",                              OPT_FLT,    &( attribute_initial[ ATTR_AGILITY   ]            ) },
    { "stamina",                              OPT_FLT,    &( attribute_initial[ ATTR_STAMINA   ]            ) },
    { "intellect",                            OPT_FLT,    &( attribute_initial[ ATTR_INTELLECT ]            ) },
    { "spirit",                               OPT_FLT,    &( attribute_initial[ ATTR_SPIRIT    ]            ) },
    { "base_strength",                        OPT_FLT,    &( attribute_base   [ ATTR_STRENGTH  ]            ) },
    { "base_agility",                         OPT_FLT,    &( attribute_base   [ ATTR_AGILITY   ]            ) },
    { "base_stamina",                         OPT_FLT,    &( attribute_base   [ ATTR_STAMINA   ]            ) },
    { "base_intellect",                       OPT_FLT,    &( attribute_base   [ ATTR_INTELLECT ]            ) },
    { "base_spirit",                          OPT_FLT,    &( attribute_base   [ ATTR_SPIRIT    ]            ) },
    { "strength_multiplier",                  OPT_FLT,    &( attribute_multiplier_initial[ ATTR_STRENGTH  ] ) },
    { "agility_multiplier",                   OPT_FLT,    &( attribute_multiplier_initial[ ATTR_AGILITY   ] ) },
    { "stamina_multiplier",                   OPT_FLT,    &( attribute_multiplier_initial[ ATTR_STAMINA   ] ) },
    { "intellect_multiplier",                 OPT_FLT,    &( attribute_multiplier_initial[ ATTR_INTELLECT ] ) },
    { "spirit_multiplier",                    OPT_FLT,    &( attribute_multiplier_initial[ ATTR_SPIRIT    ] ) },
    // Player - Spell Mechanics
    { "spell_power",                          OPT_FLT,    &( initial_spell_power[ SCHOOL_MAX    ]           ) },
    { "spell_power_arcane",                   OPT_FLT,    &( initial_spell_power[ SCHOOL_ARCANE ]           ) },
    { "spell_power_fire",                     OPT_FLT,    &( initial_spell_power[ SCHOOL_FIRE   ]           ) },
    { "spell_power_frost",                    OPT_FLT,    &( initial_spell_power[ SCHOOL_FROST  ]           ) },
    { "spell_power_holy",                     OPT_FLT,    &( initial_spell_power[ SCHOOL_HOLY   ]           ) },
    { "spell_power_nature",                   OPT_FLT,    &( initial_spell_power[ SCHOOL_NATURE ]           ) },
    { "spell_power_shadow",                   OPT_FLT,    &( initial_spell_power[ SCHOOL_SHADOW ]           ) },
    { "spell_hit",                            OPT_FLT,    &( initial_spell_hit                              ) },
    { "spell_crit",                           OPT_FLT,    &( initial_spell_crit                             ) },
    { "spell_penetration",                    OPT_FLT,    &( initial_spell_penetration                      ) },
    { "mp5",                                  OPT_FLT,    &( initial_mp5                                    ) },
    { "base_spell_power",                     OPT_FLT,    &( base_spell_power                               ) },
    { "base_spell_hit",                       OPT_FLT,    &( base_spell_hit                                 ) },
    { "base_spell_crit",                      OPT_FLT,    &( base_spell_crit                                ) },
    { "base_spell_penetration",               OPT_FLT,    &( base_spell_penetration                         ) },
    { "base_mp5",                             OPT_FLT,    &( base_mp5                                       ) },
    { "spell_power_per_intellect",            OPT_FLT,    &( spell_power_per_intellect                      ) },
    { "spell_power_per_spirit",               OPT_FLT,    &( spell_power_per_spirit                         ) },
    { "spell_crit_per_intellect",             OPT_FLT,    &( spell_crit_per_intellect                       ) },
    // Player - Attack Mechanics							       
    { "attack_power",                         OPT_FLT,    &( initial_attack_power                           ) },
    { "attack_hit",                           OPT_FLT,    &( initial_attack_hit                             ) },
    { "attack_expertise",                     OPT_FLT,    &( initial_attack_expertise                       ) },
    { "attack_crit",                          OPT_FLT,    &( initial_attack_crit                            ) },
    { "attack_penetration",                   OPT_FLT,    &( initial_attack_penetration                     ) },
    { "base_attack_power",                    OPT_FLT,    &( base_attack_power                              ) },
    { "base_attack_hit",                      OPT_FLT,    &( base_attack_hit                                ) },
    { "base_attack_expertise",                OPT_FLT,    &( base_attack_expertise                          ) },
    { "base_attack_crit",                     OPT_FLT,    &( base_attack_crit                               ) },
    { "base_attack_penetration",              OPT_FLT,    &( base_attack_penetration                        ) },
    { "attack_power_per_strength",            OPT_FLT,    &( attack_power_per_strength                      ) },
    { "attack_power_per_agility",             OPT_FLT,    &( attack_power_per_agility                       ) },
    { "attack_crit_per_agility",              OPT_FLT,    &( attack_crit_per_agility                        ) },
    // Player - Weapons
    { "main_hand",                            OPT_STRING, &( main_hand_str                                  ) },
    { "off_hand",                             OPT_STRING, &( off_hand_str                                   ) },
    { "ranged",                               OPT_STRING, &( ranged_str                                     ) },
    // Player - Resources
    { "health",                               OPT_FLT,    &( resource_initial[ RESOURCE_HEALTH ]            ) },
    { "mana",                                 OPT_FLT,    &( resource_initial[ RESOURCE_MANA   ]            ) },
    { "rage",                                 OPT_FLT,    &( resource_initial[ RESOURCE_RAGE   ]            ) },
    { "energy",                               OPT_FLT,    &( resource_initial[ RESOURCE_ENERGY ]            ) },
    { "focus",                                OPT_FLT,    &( resource_initial[ RESOURCE_FOCUS  ]            ) },
    { "runic",                                OPT_FLT,    &( resource_initial[ RESOURCE_RUNIC  ]            ) },
    { "base_health",                          OPT_FLT,    &( resource_base   [ RESOURCE_HEALTH ]            ) },
    { "base_mana",                            OPT_FLT,    &( resource_base   [ RESOURCE_MANA   ]            ) },
    { "base_rage",                            OPT_FLT,    &( resource_base   [ RESOURCE_RAGE   ]            ) },
    { "base_energy",                          OPT_FLT,    &( resource_base   [ RESOURCE_ENERGY ]            ) },
    { "base_focus",                           OPT_FLT,    &( resource_base   [ RESOURCE_FOCUS  ]            ) },
    { "base_runic",                           OPT_FLT,    &( resource_base   [ RESOURCE_RUNIC  ]            ) },
    // Player - Action Priority List								        
    { "pre_actions",                          OPT_STRING, &( action_list_prefix                             ) },
    { "actions",                              OPT_STRING, &( action_list_str                                ) },
    { "post_actions",                         OPT_STRING, &( action_list_postfix                            ) },
    { "skip_actions",                         OPT_STRING, &( action_list_skip                               ) },
    // Player - Reporting
    { "quiet",                                OPT_INT8,   &( quiet                                          ) },
    // Player - Gear - Attributes								            
    { "gear_strength",                        OPT_INT16,  &( gear.attribute        [ ATTR_STRENGTH  ]       ) },
    { "gear_agility",                         OPT_INT16,  &( gear.attribute        [ ATTR_AGILITY   ]       ) },
    { "gear_stamina",                         OPT_INT16,  &( gear.attribute        [ ATTR_STAMINA   ]       ) },
    { "gear_intellect",                       OPT_INT16,  &( gear.attribute        [ ATTR_INTELLECT ]       ) },
    { "gear_spirit",                          OPT_INT16,  &( gear.attribute        [ ATTR_SPIRIT    ]       ) },
    { "enchant_strength",                     OPT_INT16,  &( gear.attribute_enchant[ ATTR_STRENGTH  ]       ) },
    { "enchant_agility",                      OPT_INT16,  &( gear.attribute_enchant[ ATTR_AGILITY   ]       ) },
    { "enchant_stamina",                      OPT_INT16,  &( gear.attribute_enchant[ ATTR_STAMINA   ]       ) },
    { "enchant_intellect",                    OPT_INT16,  &( gear.attribute_enchant[ ATTR_INTELLECT ]       ) },
    { "enchant_spirit",                       OPT_INT16,  &( gear.attribute_enchant[ ATTR_SPIRIT    ]       ) },
    // Player - Gear - Spell									            
    { "gear_spell_power",                     OPT_INT16,  &( gear.spell_power[ SCHOOL_MAX    ]              ) },
    { "gear_spell_power_arcane",              OPT_INT16,  &( gear.spell_power[ SCHOOL_ARCANE ]              ) },
    { "gear_spell_power_fire",                OPT_INT16,  &( gear.spell_power[ SCHOOL_FIRE   ]              ) },
    { "gear_spell_power_frost",               OPT_INT16,  &( gear.spell_power[ SCHOOL_FROST  ]              ) },
    { "gear_spell_power_holy",                OPT_INT16,  &( gear.spell_power[ SCHOOL_HOLY   ]              ) },
    { "gear_spell_power_nature",              OPT_INT16,  &( gear.spell_power[ SCHOOL_NATURE ]              ) },
    { "gear_spell_power_shadow",              OPT_INT16,  &( gear.spell_power[ SCHOOL_SHADOW ]              ) },
    { "gear_spell_penetration",               OPT_INT16,  &( gear.spell_penetration                         ) },
    { "gear_mp5",                             OPT_INT16,  &( gear.mp5                                       ) },
    { "enchant_spell_power",                  OPT_INT16,  &( gear.spell_power_enchant[ SCHOOL_MAX    ]      ) },
    { "enchant_spell_power_arcane",           OPT_INT16,  &( gear.spell_power_enchant[ SCHOOL_ARCANE ]      ) },
    { "enchant_spell_power_fire",             OPT_INT16,  &( gear.spell_power_enchant[ SCHOOL_FIRE   ]      ) },
    { "enchant_spell_power_frost",            OPT_INT16,  &( gear.spell_power_enchant[ SCHOOL_FROST  ]      ) },
    { "enchant_spell_power_holy",             OPT_INT16,  &( gear.spell_power_enchant[ SCHOOL_HOLY   ]      ) },
    { "enchant_spell_power_nature",           OPT_INT16,  &( gear.spell_power_enchant[ SCHOOL_NATURE ]      ) },
    { "enchant_spell_power_shadow",           OPT_INT16,  &( gear.spell_power_enchant[ SCHOOL_SHADOW ]      ) },
    { "enchant_spell_penetration",            OPT_INT16,  &( gear.spell_penetration_enchant                 ) },
    { "enchant_mp5",                          OPT_INT16,  &( gear.mp5_enchant                               ) },
    // Player - Gear - Attack									            
    { "gear_attack_power",                    OPT_INT16,  &( gear.attack_power                              ) },
    { "gear_expertise_rating",                OPT_INT16,  &( gear.expertise_rating                          ) },
    { "gear_armor_penetration_rating",        OPT_INT16,  &( gear.armor_penetration_rating                  ) },
    { "enchant_attack_power",                 OPT_INT16,  &( gear.attack_power_enchant                      ) },
    { "enchant_attack_expertise_rating",      OPT_INT16,  &( gear.expertise_rating_enchant                  ) },
    { "enchant_attack_penetration",           OPT_INT16,  &( gear.armor_penetration_rating_enchant          ) },
    // Player - Gear - Common									            
    { "gear_haste_rating",                    OPT_INT16,  &( gear.haste_rating                              ) },
    { "gear_hit_rating",                      OPT_INT16,  &( gear.hit_rating                                ) },
    { "gear_crit_rating",                     OPT_INT16,  &( gear.crit_rating                               ) },
    { "enchant_haste_rating",                 OPT_INT16,  &( gear.haste_rating_enchant                      ) },
    { "enchant_hit_rating",                   OPT_INT16,  &( gear.hit_rating_enchant                        ) },
    { "enchant_crit_rating",                  OPT_INT16,  &( gear.crit_rating_enchant                       ) },
    // Player - Gear - Resource									            
    { "gear_health",                          OPT_INT16,  &( gear.resource        [ RESOURCE_HEALTH ]       ) },
    { "gear_mana",                            OPT_INT16,  &( gear.resource        [ RESOURCE_MANA   ]       ) },
    { "gear_rage",                            OPT_INT16,  &( gear.resource        [ RESOURCE_RAGE   ]       ) },
    { "gear_energy",                          OPT_INT16,  &( gear.resource        [ RESOURCE_ENERGY ]       ) },
    { "gear_focus",                           OPT_INT16,  &( gear.resource        [ RESOURCE_FOCUS  ]       ) },
    { "gear_runic",                           OPT_INT16,  &( gear.resource        [ RESOURCE_RUNIC  ]       ) },
    { "enchant_health",                       OPT_INT16,  &( gear.resource_enchant[ RESOURCE_HEALTH ]       ) },
    { "enchant_mana",                         OPT_INT16,  &( gear.resource_enchant[ RESOURCE_MANA   ]       ) },
    { "enchant_rage",                         OPT_INT16,  &( gear.resource_enchant[ RESOURCE_RAGE   ]       ) },
    { "enchant_energy",                       OPT_INT16,  &( gear.resource_enchant[ RESOURCE_ENERGY ]       ) },
    { "enchant_focus",                        OPT_INT16,  &( gear.resource_enchant[ RESOURCE_FOCUS  ]       ) },
    { "enchant_runic",                        OPT_INT16,  &( gear.resource_enchant[ RESOURCE_RUNIC  ]       ) },
    // Player - Gear - Budgeting								            
    { "spell_power_budget",                   OPT_INT16,  &( gear.spell_power_budget                        ) },
    { "attack_power_budget",                  OPT_INT16,  &( gear.attack_power_budget                       ) },
    { "budget_slots",                         OPT_INT16,  &( gear.budget_slots                              ) },
    // Player - Gear - Unique									            
    { "ashtongue_talisman",                   OPT_INT8,   &( gear.ashtongue_talisman                        ) },
    { "chaotic_skyfire",                      OPT_INT8,   &( gear.chaotic_skyfire                           ) },
    { "darkmoon_crusade",                     OPT_INT8,   &( gear.darkmoon_crusade                          ) },
    { "darkmoon_wrath",                       OPT_INT8,   &( gear.darkmoon_wrath                            ) },
    { "elder_scribes",                        OPT_INT8,   &( gear.elder_scribes                             ) },
    { "eternal_sage",                         OPT_INT8,   &( gear.eternal_sage                              ) },
    { "eye_of_magtheridon",                   OPT_INT8,   &( gear.eye_of_magtheridon                        ) },
    { "lightning_capacitor",                  OPT_INT8,   &( gear.lightning_capacitor                       ) },
    { "mark_of_defiance",                     OPT_INT8,   &( gear.mark_of_defiance                          ) },
    { "mystical_skyfire",                     OPT_INT8,   &( gear.mystical_skyfire                          ) },
    { "quagmirrans_eye",                      OPT_INT8,   &( gear.quagmirrans_eye                           ) },
    { "sextant_of_unstable_currents",         OPT_INT8,   &( gear.sextant_of_unstable_currents              ) },
    { "shiffars_nexus_horn",                  OPT_INT8,   &( gear.shiffars_nexus_horn                       ) },
    { "spellstrike",                          OPT_INT8,   &( gear.spellstrike                               ) },
    { "wrath_of_cenarius",                    OPT_INT8,   &( gear.wrath_of_cenarius                         ) },
    { "spellsurge",                           OPT_INT8,   &( gear.spellsurge                                ) },
    { "talisman_of_ascendance",               OPT_INT8,   &( gear.talisman_of_ascendance                    ) },
    { "timbals_crystal",                      OPT_INT8,   &( gear.timbals_crystal                           ) },
    { "zandalarian_hero_charm",               OPT_INT8,   &( gear.zandalarian_hero_charm                    ) },
    { "tier4_2pc",                            OPT_INT8,   &( gear.tier4_2pc                                 ) },
    { "tier4_4pc",                            OPT_INT8,   &( gear.tier4_4pc                                 ) },
    { "tier5_2pc",                            OPT_INT8,   &( gear.tier5_2pc                                 ) },
    { "tier5_4pc",                            OPT_INT8,   &( gear.tier5_4pc                                 ) },
    { "tier6_2pc",                            OPT_INT8,   &( gear.tier6_2pc                                 ) },
    { "tier6_4pc",                            OPT_INT8,   &( gear.tier6_4pc                                 ) },
    // Player - Consumables									            
    { "flask",                                OPT_STRING, &( flask_str                                      ) },
    { "elixirs",                              OPT_STRING, &( elixirs_str                                    ) },
    { "food",                                 OPT_STRING, &( food_str                                       ) },
    // Player - Buffs - FIXME! These will go away eventually, and be converted into player actions
    { "battle_shout",                         OPT_INT8,   &( buffs.battle_shout                             ) },
    { "blessing_of_kings",                    OPT_INT8,   &( buffs.blessing_of_kings                        ) },
    { "blessing_of_might",                    OPT_INT8,   &( buffs.blessing_of_might                        ) },
    { "blessing_of_salvation",                OPT_INT8,   &( buffs.blessing_of_salvation                    ) },
    { "blessing_of_wisdom",                   OPT_INT8,   &( buffs.blessing_of_wisdom                       ) },
    { "sanctity_aura",                        OPT_INT8,   &( buffs.sanctity_aura                            ) },
    { "sanctified_retribution",               OPT_INT8,   &( buffs.sanctified_retribution                   ) },
    { "swift_retribution",                    OPT_INT8,   &( buffs.swift_retribution                        ) },
    { NULL, OPT_UNKNOWN }
  };
  
  if( name.empty() )
  {
    option_t::print( sim, options );
    return false;
  }

  return option_t::parse( sim, options, name, value );
}
