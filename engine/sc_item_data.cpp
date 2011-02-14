// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

namespace   // ANONYMOUS NAMESPACE ==========================================
{
  
static std::string stat_to_str( int stat, int stat_amount )
{
  std::string stat_str;
  char        stat_buf[64];
  stat_type          s = util_t::translate_item_mod( stat );
  
  if ( ! stat_amount || s == STAT_NONE ) return "";
  
  snprintf( stat_buf, sizeof( stat_buf ), "%d%s", stat_amount, util_t::stat_type_abbrev( s ) );
  stat_str = stat_buf;
  
  return armory_t::format( stat_str );
}

static std::string encode_stats( const std::vector<std::string>& stats )
{
  std::ostringstream s;
  
  for ( unsigned i = 0; i < stats.size(); i++ )
  {
    s << stats[ i ];
    
    if ( i < stats.size() - 1 )
      s << "_";
  }
  
  return s.str();
}

static size_t encode_item_enchant_stats( const item_enchantment_data_t* enchantment, std::vector<std::string>& stats )
{
  assert( enchantment );
  
  for ( int i = 0; i < 3; i++ )
  {
    if ( enchantment -> ench_type[ i ] != ITEM_ENCHANTMENT_STAT )
      continue;

    std::string stat_str = stat_to_str( enchantment -> ench_prop[ i ], enchantment -> ench_amount[ i ] );
    if ( ! stat_str.empty() ) stats.push_back( stat_str );
  }
  
  return stats.size();
}

static size_t encode_item_stats( const item_data_t* item, std::vector<std::string>& stats )
{
  assert( item );
  
  for ( int i = 0; i < 10; i++ )
  {
    if ( item -> stat_type[ i ] < 0 )
      continue;

    std::string stat_str = stat_to_str( item -> stat_type[ i ], item -> stat_val[ i ] );
    if ( ! stat_str.empty() ) stats.push_back( stat_str );
  }
  
  return stats.size();
}
  
static bool parse_item_quality( item_t& item, const item_data_t* item_data )
{
  assert( item_data );

  item.armory_quality_str.clear();

  if ( item_data -> quality == 5 )
    item.armory_quality_str = "artifact";
  else if ( item_data -> quality == 4 )
    item.armory_quality_str = "epic";
  else if ( item_data -> quality == 3 )
    item.armory_quality_str = "rare";
  else if ( item_data -> quality == 3 )
    item.armory_quality_str = "uncommon";

  return true;
}

static bool parse_item_level( item_t& item, const item_data_t* item_data )
{
  assert( item_data );

  item.armory_ilevel_str.clear();

  item.armory_ilevel_str = util_t::to_string( item_data -> level );

  return true;
}

static bool parse_item_name( item_t& item, const item_data_t* item_data )
{
  assert( item_data );

  item.armory_name_str.clear();

  item.armory_name_str = item_data -> name;

  armory_t::format( item.armory_name_str );

  return true;
}

static bool parse_item_heroic( item_t& item, const item_data_t* item_data )
{
  assert( item_data );

  item.armory_heroic_str.clear();

  if ( item_data -> flags_1 & ITEM_FLAG_HEROIC )
    item.armory_heroic_str = "1";

  return true;
}

static bool parse_item_armor_type( item_t& item, const item_data_t* item_data )
{
  assert( item_data );

  item.armory_armor_type_str.clear();

  if ( item_data -> item_class == ITEM_CLASS_ARMOR )
  {
    if ( item_data -> item_subclass == ITEM_SUBCLASS_ARMOR_CLOTH )
      item.armory_armor_type_str = "cloth";
    else if ( item_data -> item_subclass == ITEM_SUBCLASS_ARMOR_LEATHER )
      item.armory_armor_type_str = "leather";
    else if ( item_data -> item_subclass == ITEM_SUBCLASS_ARMOR_MAIL )
      item.armory_armor_type_str = "mail";
    else if ( item_data -> item_subclass == ITEM_SUBCLASS_ARMOR_PLATE )
      item.armory_armor_type_str = "plate";
  }

  return true;
}

static bool parse_item_stats( item_t&            item,
                              const item_data_t* item_data )
{
  std::vector<std::string> stats;
  uint32_t armor;
  assert( item_data );

  item.armory_stats_str.clear();

  if ( ( armor = item_database_t::armor_value( item, item_data -> id ) ) > 0 )
  {
    char b[64];
    snprintf( b, sizeof( b ), "%darmor", armor );
    stats.push_back( b );
  }
  
  if ( encode_item_stats( item_data, stats ) > 0 )
    item.armory_stats_str = encode_stats( stats );

  return true;
}

static bool parse_weapon_type( item_t&            item,
                               const item_data_t* item_data )
{
  double   speed;
  unsigned min_dam, max_dam;
  char     stat_buf[64];

  item.armory_weapon_str.clear();

  if ( item_data -> item_class != ITEM_CLASS_WEAPON )
    return true;

  speed   = item.player -> player_data.weapon_speed( item_data -> id );
  min_dam = item_database_t::weapon_dmg_min( item, item_data -> id );
  max_dam = item_database_t::weapon_dmg_max( item, item_data -> id );

  if ( ! speed || ! min_dam || ! max_dam )
    return true;

  weapon_type w = util_t::translate_weapon_subclass( ( item_subclass_weapon ) item_data -> item_subclass );
  if ( w == WEAPON_NONE || w == WEAPON_WAND )
    return true;

  snprintf( stat_buf, sizeof( stat_buf ), "%s_%4.2fspeed_%umin_%umax", 
    util_t::weapon_type_string( w ), speed, min_dam, max_dam );
  item.armory_weapon_str = stat_buf;

  return true;
}

static bool parse_gems( item_t&            item, 
                        const item_data_t* item_data, 
                        const std::string  gem_ids[ 3 ] )
{
  bool match = true;
  const item_enchantment_data_t* socket_bonus = 0;
  std::vector<std::string> stats;

  item.armory_gems_str.clear();

  for ( unsigned i = 0; i < 3; i++ )
  {
    if ( gem_ids[ i ].empty() || gem_ids[ i ] == "" )
    {
      // Check if there's a gem slot, if so, this is ungemmed item.
      if ( item_data -> socket_color[ i ] )
        match = false;
      continue;
    }

    if ( ! ( item_t::parse_gem( item, gem_ids[ i ] ) & item_data -> socket_color[ i ] ) )
      match = false;
  }

  // Socket bonus
  if ( match && ( socket_bonus = item.player -> player_data.find_item_enchantment( item_data -> id_socket_bonus ) ) )
  {
    if ( encode_item_enchant_stats( socket_bonus, stats ) > 0 && ! item.armory_gems_str.empty() )
      item.armory_gems_str += "_";

    item.armory_gems_str += encode_stats( stats );
  }
  
  return true;
}

static bool parse_enchant( item_t&            item, 
                           const item_data_t* item_data,
                           const std::string& enchant_id )
{
  if ( enchant_id.empty() || enchant_id == "none" ) return true;
  
  const item_enchantment_data_t* item_enchant;
  long                                    eid = strtol( enchant_id.c_str(), 0, 10 );
  bool                              has_spell = true;
  std::vector<std::string> stats;
  
  if ( !( item_enchant = item.player -> player_data.find_item_enchantment( eid ) ) )
  {
    item.player -> sim -> errorf("Unable to find enchant id %u from item enchantment database", eid );
    return true;
  }
  
  item.armory_enchant_str.clear();
  
  for ( unsigned i = 0; i < 3; i++ )
  {
    if ( item_enchant -> ench_type[ i ] != ITEM_ENCHANTMENT_STAT )
    {
      has_spell = true;
      break;
    }
  }
  
  // For now, if there's a spell in the enchant, defer back to old ways
  if ( has_spell )
  {
    return enchant_t::download( item, enchant_id );
  }
  else
  {
    if ( encode_item_enchant_stats( item_enchant, stats ) > 0 )
      item.armory_enchant_str = encode_stats( stats );
  }
  
  return true;
}

}  // ANONYMOUS NAMESPACE ====================================================

// item_database_t::initialize_item_sources ===========================================

bool item_database_t::initialize_item_sources( const item_t& item, std::vector<std::string>& source_list )
{
  source_list = item.sim -> item_db_sources;
  
  if ( ! item.option_data_source_str.empty() )
  {
    std::vector<std::string> item_sources_split;
    util_t::string_split( item_sources_split, item.option_data_source_str, ":/|", false );
    
    source_list.clear();
    
    for ( unsigned i = 0; i < item_sources_split.size(); i++ )
    {
      if ( ! util_t::str_compare_ci( item_sources_split[ i ], "local" ) &&
           ! util_t::str_compare_ci( item_sources_split[ i ], "mmoc" ) && 
           ! util_t::str_compare_ci( item_sources_split[ i ], "wowhead" ) &&
           ! util_t::str_compare_ci( item_sources_split[ i ], "ptrhead" ) && 
           ! util_t::str_compare_ci( item_sources_split[ i ], "armory" ) )
      {
        continue;
      }

      source_list.push_back( armory_t::format( item_sources_split[ i ] ) );
    }
    
    if ( source_list.empty() )
    {
      return false;
    }
  }
  
  return true;
}

// item_database_t::random_suffix_type ===========================================

int item_database_t::random_suffix_type( const item_t& item )
{
  int       f = -1;
  weapon_t* w;

  if ( ( w = item.weapon() ) )
  {
    switch ( w -> type )
    {
      // Two-hand weapons use the first point allocation budget
      case WEAPON_AXE_2H:
      case WEAPON_MACE_2H:
      case WEAPON_POLEARM:
      case WEAPON_SWORD_2H:
      case WEAPON_STAFF:
      {
        f = 0;
        break;
      }
      // Various ranged types use the fifth point allocation budget
      case WEAPON_BOW:
      case WEAPON_CROSSBOW:
      case WEAPON_GUN:
      case WEAPON_THROWN:
      case WEAPON_WAND:
      {
        f = 4;
        break;
      }
      // One-hand/Off-hand/Main-hand weapons use the fourth point allocation budget
      default:
      {
        f = 3;
        break;
      }
    }
  }
  // Armor handling goes by slot
  else
  {
    switch ( item.slot )
    {
      case SLOT_HEAD:
      case SLOT_CHEST:
      case SLOT_LEGS:
      {
        f = 0;
        break;
      }
      case SLOT_SHOULDERS:
      case SLOT_WAIST:
      case SLOT_FEET:
      case SLOT_HANDS:
      case SLOT_TRINKET_1:
      case SLOT_TRINKET_2:
      {
        f = 1;
        break;
      }
      case SLOT_NECK:
      case SLOT_WRISTS:
      case SLOT_FINGER_1:
      case SLOT_FINGER_2:
      case SLOT_OFF_HAND: // Shields, off hand items
      case SLOT_BACK:
      {
        f = 2;
        break;
      }
      // Ranged non-weapons are relics, which do not have a point allocation
      case SLOT_RANGED:
      case SLOT_TABARD:
      {
        return f;
      }
      default:
        return f;
    }
  }
  
  return f;
}

// item_database_t::armor_value ===========================================

uint32_t item_database_t::armor_value( const item_t& item_struct, unsigned item_id )
{
  const item_data_t* item = item_struct.player -> player_data.find_item( item_id );
  
  if ( ! item ) return 0;
  
  if ( item -> item_class == ITEM_CLASS_ARMOR && item -> item_subclass == ITEM_SUBCLASS_ARMOR_SHIELD )
    return (uint32_t) floor( item_struct.player -> player_data.total_armor_shield( item -> level, item -> quality ) + 0.5 );
  
  double m_invtype = 0, m_quality = 0, total_armor = 0;
  
  switch ( item -> inventory_type )
  {
    case INVTYPE_HEAD:
    case INVTYPE_SHOULDERS:
    case INVTYPE_CHEST:
    case INVTYPE_WAIST:
    case INVTYPE_LEGS:
    case INVTYPE_FEET:
    case INVTYPE_WRISTS:
    case INVTYPE_HANDS:
    case INVTYPE_CLOAK:
    case INVTYPE_ROBE:
    {
      total_armor = item_struct.player -> player_data.total_armor( item -> level, item -> item_subclass );
      m_quality   = item_struct.player -> player_data.m_armor_quality( item -> level, item -> quality );
      if ( item -> inventory_type == INVTYPE_ROBE )
        m_invtype = item_struct.player -> player_data.m_armor_invtype( INVTYPE_CHEST, item -> item_subclass );
      else
        m_invtype = item_struct.player -> player_data.m_armor_invtype( item -> inventory_type, item -> item_subclass );
      break;
    }
    default: return 0;
  }

  return (uint32_t) floor( total_armor * m_quality * m_invtype + 0.5 );
}

// item_database_t::weapon_dmg_min/max ===========================================

uint32_t item_database_t::weapon_dmg_min( const item_t& item, unsigned item_id )
{
  return (uint32_t) floor( item.player -> player_data.weapon_dps( item_id ) * item.player -> player_data.weapon_speed( item_id ) * 0.8 + 0.5 );
}

uint32_t item_database_t::weapon_dmg_max( const item_t& item, unsigned item_id )
{
  return (uint32_t) floor( item.player -> player_data.weapon_dps( item_id ) * item.player -> player_data.weapon_speed( item_id ) * 1.2 + 0.5 );
}

// item_database_t::download_slot ===========================================

bool item_database_t::download_slot( item_t&            item, 
                                     const std::string& item_id, 
                                     const std::string& enchant_id, 
                                     const std::string& addon_id, 
                                     const std::string& reforge_id,
                                     const std::string& rsuffix_id,
                                     const std::string  gem_ids[ 3 ] )
{
  player_t* p                  = item.player;
  long iid                     = strtol( item_id.c_str(), 0, 10 );
  const item_data_t* item_data = p -> player_data.m_items_index[ iid ];

  if ( ! item_data || ! iid ) return false;

  parse_item_name( item, item_data );
  parse_item_quality( item, item_data );
  parse_item_level( item, item_data );
  parse_item_heroic( item, item_data );
  parse_item_armor_type( item, item_data );
  parse_item_stats( item, item_data );
  parse_weapon_type( item, item_data );
  parse_gems( item, item_data, gem_ids );

  if ( ! parse_enchant( item, item_data, enchant_id ) )
  {
    item.sim -> errorf( "Player %s unable to parse enchant id %s for item \"%s\" at slot %s.\n", p -> name(), enchant_id.c_str(), item.name(), item.slot_name() );
  }
  
  if ( ! enchant_t::download_addon( item, addon_id ) )
  {
    item.sim -> errorf( "Player %s unable to parse addon id %s for item \"%s\" at slot %s.\n", p -> name(), addon_id.c_str(), item.name(), item.slot_name() );
  }

  if ( ! enchant_t::download_reforge( item, reforge_id ) )
  {
    item.sim -> errorf( "Player %s unable to parse reforge id %s for item \"%s\" at slot %s.\n", p -> name(), reforge_id.c_str(), item.name(), item.slot_name() );
  }

  if ( ! enchant_t::download_rsuffix( item, rsuffix_id ) )
  {
    item.sim -> errorf( "Player %s unable to determine random suffix '%s' for item '%s' at slot %s.\n", p -> name(), rsuffix_id.c_str(), item.name(), item.slot_name() );
  }
  
  if ( item.sim -> debug )
  {
    log_t::output( item.sim, "item_db: n=[%s] q=[%s] l=[%s] h=[%s] a=[%s] s=[%s] w=[%s] g=[%s] e=[%s] r=[%s] rs=[%s] a=[%s]",
      item.armory_name_str.c_str(),
      item.armory_quality_str.c_str(),
      item.armory_ilevel_str.c_str(),
      item.armory_heroic_str.c_str(),
      item.armory_armor_type_str.c_str(),
      item.armory_stats_str.c_str(),
      item.armory_weapon_str.c_str(),
      item.armory_gems_str.c_str(),
      item.armory_enchant_str.c_str(),
      item.armory_reforge_str.c_str(),
      item.armory_random_suffix_str.c_str(),
      item.armory_addon_str.c_str() );
  }
  
  return true;
}

bool item_database_t::download_item( item_t& item, const std::string& item_id )
{
  player_t* p                  = item.player;
  long iid                     = strtol( item_id.c_str(), 0, 10 );
  const item_data_t* item_data = p -> player_data.m_items_index[ iid ];

  if ( ! item_data || ! iid ) return false;

  parse_item_name( item, item_data );
  parse_item_quality( item, item_data );
  parse_item_level( item, item_data );
  parse_item_heroic( item, item_data );
  parse_item_armor_type( item, item_data );
  parse_item_stats( item, item_data );
  parse_weapon_type( item, item_data );

  if ( item.sim -> debug )
  {
    log_t::output( item.sim, "item_db: n=[%s] q=[%s] l=[%s] h=[%s] a=[%s] s=[%s] w=[%s] g=[%s] e=[%s] r=[%s] rs=[%s] a=[%s]",
      item.armory_name_str.c_str(),
      item.armory_quality_str.c_str(),
      item.armory_ilevel_str.c_str(),
      item.armory_heroic_str.c_str(),
      item.armory_armor_type_str.c_str(),
      item.armory_stats_str.c_str(),
      item.armory_weapon_str.c_str(),
      item.armory_gems_str.c_str(),
      item.armory_enchant_str.c_str(),
      item.armory_reforge_str.c_str(),
      item.armory_random_suffix_str.c_str(),
      item.armory_addon_str.c_str() );
  }

  return true;
}

bool item_database_t::download_glyph( player_t* player, std::string& glyph_name, const std::string& glyph_id )
{
  long gid                 = strtol( glyph_id.c_str(), 0, 10 );
  const item_data_t* glyph = player -> player_data.find_item( gid );

  if ( ! gid || ! glyph ) return false;
  
  glyph_name = glyph -> name;
  
  if(      glyph_name.substr( 0, 9 ) == "Glyph of " ) glyph_name.erase( 0, 9 );
  else if( glyph_name.substr( 0, 8 ) == "Glyph - "  ) glyph_name.erase( 0, 8 );
  armory_t::format( glyph_name );
  
  return true;
}


int item_database_t::parse_gem( item_t& item, const std::string& gem_id )
{
  long gid                                 = strtol( gem_id.c_str(), 0, 10 );
  const gem_property_data_t* gem_prop      = 0;
  const item_enchantment_data_t* item_ench = 0;
  const item_data_t* gem                   = 0;
  int gc                                   = SOCKET_COLOR_NONE;
  std::vector<std::string> stats;
  
  if ( ( gem = item.player -> player_data.m_items_index[ gid ] ) && 
       ( gem_prop = item.player -> player_data.find_gem_property( gem -> gem_properties ) ) )
  {
    // For now, make meta gems simply parse the name out
    if ( gem_prop -> color == 1 )
    {
      std::string gem_name = gem -> name;
      size_t cut_pt = gem_name.rfind( " Diamond");
      if ( cut_pt != gem_name.npos )
      {
        gem_name = gem_name.substr( 0, cut_pt );
        armory_t::format( gem_name );
        item.armory_gems_str += gem_name;
      }
    }
    // Fetch gem stats
    else if ( ( item_ench = item.player -> player_data.find_item_enchantment( gem_prop -> enchant_id ) ) )
    {
      encode_item_enchant_stats( item_ench, stats );
    }
    
    gc = gem_prop -> color;
  }

  if ( stats.size() > 0 && ! item.armory_gems_str.empty() )
    item.armory_gems_str += "_";

  item.armory_gems_str += encode_stats( stats );

  return gc;
}