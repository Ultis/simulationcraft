// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simcraft.h"

// ==========================================================================
// Scaling
// ==========================================================================

// scaling_t::scaling_t =====================================================


scaling_t::scaling_t( sim_t* s ) : sim(s), calculate_scale_factors(0) 
{
  for( int i=ATTRIBUTE_NONE+1; i < ATTRIBUTE_MAX; i++ )
  {
    gear.attribute[ i ] = 250;
  }
  gear.armor_penetration_rating = 250;
  gear.attack_power             = 250;
  gear.crit_rating              = 250;
  gear.expertise_rating         = -150;
  gear.haste_rating             = 250;
  gear.hit_rating               = -200;
  gear.spell_power              = 250;
}

// scaling_t::analyze_attributes ============================================

void scaling_t::analyze_attributes()
{
  int num_players = sim -> players_by_name.size();
  if( num_players == 0 ) return;

  for( int i=0; i < ATTRIBUTE_MAX; i++ )
  {
    if( gear.attribute[ i ] != 0 )
    {
      fprintf( stdout, "\nGenerating scale factors for %s...\n", util_t::attribute_type_string( i ) );
      fflush( stdout );

      sim_t* child_sim = new sim_t( sim );
      child_sim -> gear_delta.attribute[ i ] = gear.attribute[ i ] / calculate_scale_factors;
      child_sim -> execute();

      sim_t* ref_sim = sim;
      if ( calculate_scale_factors == 2 )
      {
        ref_sim = new sim_t( sim );
        ref_sim -> gear_delta.attribute[ i ] = -( gear.attribute[ i ] / 2 );
        ref_sim -> execute();
      }

      for( int j=0; j < num_players; j++ )
      {
	player_t*       p =       sim -> players_by_name[ j ];
	player_t*   ref_p =   ref_sim -> find_player( p -> name() );
	player_t* child_p = child_sim -> find_player( p -> name() );
	
	double f = ( child_p -> dps - ref_p -> dps ) / gear.attribute[ i ];

	if( f > 0.09 ) p -> scaling.attribute[ i ] = f;
      }

      if( ref_sim != sim ) delete ref_sim;
      delete child_sim;
    }
  }
}

// scaling_t::analyze_spell_power ===========================================

void scaling_t::analyze_spell_power()
{
  int num_players = sim -> players_by_name.size();
  if( num_players == 0 ) return;

  if( gear.spell_power != 0 )
  {
    fprintf( stdout, "\nGenerating scale factors for spell power...\n" );
    fflush( stdout );

    sim_t* child_sim = new sim_t( sim );
    child_sim -> gear_delta.spell_power = ( calculate_scale_factors == 2 ) ? gear.spell_power / 2 : gear.spell_power;
    child_sim -> execute();

    sim_t* child_sim2 = NULL;
    if ( calculate_scale_factors == 2 )
    {
      child_sim2 = new sim_t( sim );
      child_sim2 -> gear_delta.spell_power = -( gear.spell_power / 2 );
      child_sim2 -> execute();
    }

    for( int j=0; j < num_players; j++ )
    {
      player_t* p = sim -> players_by_name[ j ];
      player_t* child_p = child_sim -> find_player( p -> name() );
	
      double f;
        
      switch ( calculate_scale_factors )
      {
        case 2:
          f = ( child_p -> dps - p -> dps ) / ( gear.spell_power / 2 );
          child_p = child_sim2 -> find_player( p -> name() );
          f += ( child_p -> dps - p -> dps ) / -( gear.spell_power / 2 );
          f /= 2.0;
          break;
        case 1: 
        default:
          f = ( child_p -> dps - p -> dps ) / gear.spell_power;
          break;
      }

      if( f > 0.09 ) p -> scaling.spell_power = f;
    }

    if ( calculate_scale_factors == 2 )
      delete child_sim2;
    delete child_sim;
  }
}

// scaling_t::analyze_attack_power ==========================================

void scaling_t::analyze_attack_power()
{
  int num_players = sim -> players_by_name.size();
  if( num_players == 0 ) return;

  if( gear.attack_power != 0 )
  {
    fprintf( stdout, "\nGenerating scale factors for attack power...\n" );
    fflush( stdout );

    sim_t* child_sim = new sim_t( sim );
    child_sim -> gear_delta.attack_power = ( calculate_scale_factors == 2 ) ? gear.attack_power / 2 : gear.attack_power;
    child_sim -> execute();

    sim_t* child_sim2 = NULL;
    if ( calculate_scale_factors == 2 )
    {
      child_sim2 = new sim_t( sim );
      child_sim2 -> gear_delta.attack_power = -( gear.attack_power / 2 );
      child_sim2 -> execute();
    }

    for( int j=0; j < num_players; j++ )
    {
      player_t* p = sim -> players_by_name[ j ];
      player_t* child_p = child_sim -> find_player( p -> name() );
	
      double f;
        
      switch ( calculate_scale_factors )
      {
        case 2:
          f = ( child_p -> dps - p -> dps ) / ( gear.attack_power / 2 );
          child_p = child_sim2 -> find_player( p -> name() );
          f += ( child_p -> dps - p -> dps ) / -( gear.attack_power / 2 );
          f /= 2.0;
          break;
        case 1: 
        default:
          f = ( child_p -> dps - p -> dps ) / gear.attack_power;
          break;
      }

      if( f > 0.09 ) p -> scaling.attack_power = f;
    }

    if ( calculate_scale_factors == 2 )
      delete child_sim2;
    delete child_sim;
  }
}

// scaling_t::analyze_expertise =============================================

void scaling_t::analyze_expertise()
{
  int num_players = sim -> players_by_name.size();
  if( num_players == 0 ) return;

  if( gear.expertise_rating != 0 )
  {
    fprintf( stdout, "\nGenerating scale factors for expertise rating...\n" );
    fflush( stdout );

    sim_t* child_sim = new sim_t( sim );
    child_sim -> gear_delta.expertise_rating = gear.expertise_rating;
    child_sim -> execute();

    sim_t* child_sim2 = NULL;
    if ( calculate_scale_factors == 2 )
    {
      child_sim2 = new sim_t( sim );
      child_sim2 -> gear_delta.expertise_rating = gear.expertise_rating / 2;
      child_sim2 -> execute();
    }

    for( int j=0; j < num_players; j++ )
    {
      player_t* p = sim -> players_by_name[ j ];
      player_t* child_p = child_sim -> find_player( p -> name() );
	
      double f = ( child_p -> dps - p -> dps ) / gear.expertise_rating;
        
      if ( calculate_scale_factors == 2 )
      {
        child_p = child_sim2 -> find_player( p -> name() );
        f += ( child_p -> dps - p -> dps ) / ( gear.expertise_rating / 2 );
        f /= 2.0;
      }

      if( f > 0.09 ) p -> scaling.expertise_rating = f;
    }

    if ( calculate_scale_factors == 2 )
      delete child_sim2;
    delete child_sim;
  }
}

// scaling_t::analyze_armor_penetration =====================================

void scaling_t::analyze_armor_penetration()
{
  int num_players = sim -> players_by_name.size();
  if( num_players == 0 ) return;

  if( gear.armor_penetration_rating != 0 )
  {
    fprintf( stdout, "\nGenerating scale factors for armor penetration rating...\n" );
    fflush( stdout );

    sim_t* child_sim = new sim_t( sim );
    child_sim -> gear_delta.armor_penetration_rating = ( calculate_scale_factors == 2 ) ? gear.armor_penetration_rating / 2 : 
                                                                                          gear.armor_penetration_rating;
    child_sim -> execute();

    sim_t* child_sim2 = NULL;
    if ( calculate_scale_factors == 2 )
    {
      child_sim2 = new sim_t( sim );
      child_sim2 -> gear_delta.armor_penetration_rating = -( gear.armor_penetration_rating / 2 );
      child_sim2 -> execute();
    }

    for( int j=0; j < num_players; j++ )
    {
      player_t* p = sim -> players_by_name[ j ];
      player_t* child_p = child_sim -> find_player( p -> name() );
	
      double f;
        
      switch ( calculate_scale_factors )
      {
        case 2:
          f = ( child_p -> dps - p -> dps ) / ( gear.armor_penetration_rating / 2 );
          child_p = child_sim2 -> find_player( p -> name() );
          f += ( child_p -> dps - p -> dps ) / -( gear.armor_penetration_rating / 2 );
          f /= 2.0;
          break;
        case 1: 
        default:
          f = ( child_p -> dps - p -> dps ) / gear.armor_penetration_rating;
          break;
      }

      if( f > 0.09 ) p -> scaling.armor_penetration_rating = f;
    }

    if ( calculate_scale_factors == 2 )
      delete child_sim2;
    delete child_sim;
  }
}

// scaling_t::analyze_hit ===================================================

void scaling_t::analyze_hit()
{
  int num_players = sim -> players_by_name.size();
  if( num_players == 0 ) return;

  if( gear.hit_rating != 0 )
  {
    fprintf( stdout, "\nGenerating scale factors for hit rating...\n" );
    fflush( stdout );

    sim_t* child_sim = new sim_t( sim );
    child_sim -> gear_delta.hit_rating = gear.hit_rating;
    child_sim -> execute();

    sim_t* child_sim2 = NULL;
    if ( calculate_scale_factors == 2 )
    {
      child_sim2 = new sim_t( sim );
      child_sim2 -> gear_delta.hit_rating = gear.hit_rating / 2;
      child_sim2 -> execute();
    }

    for( int j=0; j < num_players; j++ )
    {
      player_t* p = sim -> players_by_name[ j ];
      player_t* child_p = child_sim -> find_player( p -> name() );
	
      double f = ( child_p -> dps - p -> dps ) / gear.hit_rating;
        
      if ( calculate_scale_factors == 2 )
      {
        child_p = child_sim2 -> find_player( p -> name() );
        f += ( child_p -> dps - p -> dps ) / ( gear.hit_rating / 2 );
        f /= 2.0;
      }

      if( f > 0.09 ) p -> scaling.hit_rating = f;
    }

    if ( calculate_scale_factors == 2 )
      delete child_sim2;
    delete child_sim;
  }

}

// scaling_t::analyze_crit ==================================================

void scaling_t::analyze_crit()
{
  int num_players = sim -> players_by_name.size();
  if( num_players == 0 ) return;

  if( gear.crit_rating != 0 )
  {
    fprintf( stdout, "\nGenerating scale factors for crit rating...\n" );
    fflush( stdout );

    sim_t* child_sim = new sim_t( sim );
    child_sim -> gear_delta.crit_rating = ( calculate_scale_factors == 2 ) ? gear.crit_rating / 2 : 
                                                                             gear.crit_rating;
    child_sim -> execute();

    sim_t* child_sim2 = NULL;
    if ( calculate_scale_factors == 2 )
    {
      child_sim2 = new sim_t( sim );
      child_sim2 -> gear_delta.crit_rating = -( gear.crit_rating / 2 );
      child_sim2 -> execute();
    }

    for( int j=0; j < num_players; j++ )
    {
      player_t* p = sim -> players_by_name[ j ];
      player_t* child_p = child_sim -> find_player( p -> name() );
	
      double f;
        
      switch ( calculate_scale_factors )
      {
        case 2:
          f = ( child_p -> dps - p -> dps ) / ( gear.crit_rating / 2 );
          child_p = child_sim2 -> find_player( p -> name() );
          f += ( child_p -> dps - p -> dps ) / -( gear.crit_rating / 2 );
          f /= 2.0;
          break;
        case 1: 
        default:
          f = ( child_p -> dps - p -> dps ) / gear.crit_rating;
          break;
      }

      if( f > 0.09 ) p -> scaling.crit_rating = f;
    }

    if ( calculate_scale_factors == 2 )
      delete child_sim2;
    delete child_sim;
  }
}

// scaling_t::analyze_haste =================================================

void scaling_t::analyze_haste()
{
  int num_players = sim -> players_by_name.size();
  if( num_players == 0 ) return;

  if( gear.haste_rating != 0 )
  {
    fprintf( stdout, "\nGenerating scale factors for haste rating...\n" );
    fflush( stdout );

    sim_t* child_sim = new sim_t( sim );
    child_sim -> gear_delta.haste_rating = ( calculate_scale_factors == 2 ) ? gear.haste_rating / 2 : 
                                                                              gear.haste_rating;
    child_sim -> execute();

    sim_t* child_sim2 = NULL;
    if ( calculate_scale_factors == 2 )
    {
      child_sim2 = new sim_t( sim );
      child_sim2 -> gear_delta.haste_rating = -( gear.haste_rating / 2 );
      child_sim2 -> execute();
    }

    for( int j=0; j < num_players; j++ )
    {
      player_t* p = sim -> players_by_name[ j ];
      player_t* child_p = child_sim -> find_player( p -> name() );
	
      double f;
        
      switch ( calculate_scale_factors )
      {
        case 2:
          f = ( child_p -> dps - p -> dps ) / ( gear.haste_rating / 2 );
          child_p = child_sim2 -> find_player( p -> name() );
          f += ( child_p -> dps - p -> dps ) / -( gear.haste_rating / 2 );
          f /= 2.0;
          break;
        case 1: 
        default:
          f = ( child_p -> dps - p -> dps ) / gear.haste_rating;
          break;
      }

      if( f > 0.09 ) p -> scaling.haste_rating = f;
    }

    if ( calculate_scale_factors == 2 )
      delete child_sim2;
    delete child_sim;
  }
}

// scaling_t::analyze =======================================================

void scaling_t::analyze()
{
  if( ! calculate_scale_factors ) return;

  analyze_attributes();
  analyze_spell_power();
  analyze_attack_power();
  analyze_expertise();
  analyze_armor_penetration();
  analyze_hit();
  analyze_crit();
  analyze_haste();
}

// scaling_t::parse_option ==================================================

bool scaling_t::parse_option( const std::string& name,
			      const std::string& value )
{
  option_t options[] =
  {
    { "calculate_scale_factors",        OPT_INT, &( calculate_scale_factors              ) },
    { "scale_strength",                 OPT_INT, &( gear.attribute[ ATTR_STRENGTH  ]     ) },
    { "scale_agility",                  OPT_INT, &( gear.attribute[ ATTR_AGILITY   ]     ) },
    { "scale_stamina",                  OPT_INT, &( gear.attribute[ ATTR_STAMINA   ]     ) },
    { "scale_intellect",                OPT_INT, &( gear.attribute[ ATTR_INTELLECT ]     ) },
    { "scale_spirit",                   OPT_INT, &( gear.attribute[ ATTR_SPIRIT    ]     ) },
    { "scale_spell_power",              OPT_INT, &( gear.spell_power                     ) },
    { "scale_attack_power",             OPT_INT, &( gear.attack_power                    ) },
    { "scale_expertise_rating",         OPT_INT, &( gear.expertise_rating                ) },
    { "scale_armor_penetration_rating", OPT_INT, &( gear.armor_penetration_rating        ) },
    { "scale_hit_rating",               OPT_INT, &( gear.hit_rating                      ) },
    { "scale_crit_rating",              OPT_INT, &( gear.crit_rating                     ) },
    { "scale_haste_rating",             OPT_INT, &( gear.haste_rating                    ) },
    { NULL, OPT_UNKNOWN }
  };

  if( name.empty() )
  {
    option_t::print( sim, options );
    return false;
  }

  return option_t::parse( sim, options, name, value );
}
