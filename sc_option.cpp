// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simcraft.h"

namespace { // ANONYMOUS NAMESPACE ==========================================

// is_white_space ===========================================================

static bool is_white_space( char c )
{
  return( c == ' ' || c == '\t' || c == '\n' || c == '\r' );
}

// only_white_space =========================================================

static bool only_white_space( char* s )
{
  while( *s )
  {
    if( ! is_white_space( *s ) )
      return false;
    s++;
  }
  return true;
}

// remove_space =============================================================

static void remove_white_space( std::string& buffer,
				char*        line )
{
  while( is_white_space( *line ) ) line++;

  char* endl = line + strlen( line ) - 1;

  while( endl > line && is_white_space( *endl ) ) 
  {
    *endl = '\0';
    endl--;
  }

  buffer = line;
}

} // ANONYMOUS NAMESPACE ====================================================

// option_t::print ==========================================================

void option_t::print( sim_t* sim, option_t* options )
{
  FILE* f = sim -> output_file;

  for( int i=0; options[ i ].name; i++ )
  {
    option_t& o = options[ i ];

    fprintf( sim -> output_file, "\t%s : ", o.name );
    
    switch( o.type )
    {
    case OPT_STRING: fprintf( f, "%s\n",    ( (std::string*) o.address ) -> c_str()         ); break;
    case OPT_CHAR_P: fprintf( f, "%s\n",   *( (char**)       o.address )                    ); break;
    case OPT_BOOL:   fprintf( f, "%d\n",   *( (bool*)        o.address ) ? 1 : 0            ); break;
    case OPT_INT:    fprintf( f, "%d\n",   *( (int*)         o.address )                    ); break;
    case OPT_FLT:    fprintf( f, "%.2f\n", *( (double*)      o.address )                    ); break;
    case OPT_LIST:     
      {
	std::vector<std::string>& v = *( (std::vector<std::string>*) o.address );
	for( unsigned i=0; i < v.size(); i++ ) fprintf( f, "%s%s", (i?" ":""), v[ i ].c_str() );
	fprintf( f, "\n" );
      }
      break;
    case OPT_APPEND:     break;
    case OPT_DEPRECATED: break;
    default: assert(0);
    }
  }
}

// option_t::parse ==========================================================

bool option_t::parse( sim_t*             sim,
		      option_t*          options,
		      const std::string& name,
		      const std::string& value )
{
  for( int i=0; options[ i ].name; i++ )
  {
    option_t& o = options[ i ];
      
    if( name == o.name )
    {
      switch( o.type )
      {
      case OPT_STRING: *( (std::string*) o.address ) = value;                                break;
      case OPT_APPEND: *( (std::string*) o.address ) += value;                               break;
      case OPT_CHAR_P: *( (char**)       o.address ) = util_t::dup( value.c_str() );         break;
      case OPT_BOOL:   *( (bool*)        o.address ) = atoi( value.c_str() ) ? true : false; break;
      case OPT_INT:    *( (int*)         o.address ) = atoi( value.c_str() );                break;
      case OPT_FLT:    *( (double*)      o.address ) = atof( value.c_str() );                break;
      case OPT_LIST:   ( (std::vector<std::string>*) o.address ) -> push_back( value );      break;
      case OPT_DEPRECATED: 
	printf( "simcraft: option '%s' has been deprecated.\n", o.name );
	if( o.address ) printf( "simcraft: please use '%s' instead.\n", (char*) o.address ); 
	exit(0);
      default: assert(0);
      }
      return true;
    }	 
  }

  return false;
}

// option_t::parse ==========================================================

bool option_t::parse( sim_t* sim,
		      FILE*  file )
{
  char buffer[ 1024 ];
  while( fgets( buffer, 1024, file ) )
  {
    if( *buffer == '#' ) continue;
    if( only_white_space( buffer ) ) continue;
    option_t::parse( sim, buffer );
  }
  return true;
}

// option_t::parse ==========================================================

bool option_t::parse( sim_t* sim,
		      char*  line )
{
  if( sim -> debug ) report_t::log( sim, "option_t::parse: %s", line );

  std::string buffer;

  remove_white_space( buffer, line );

  std::vector<std::string> tokens;

  int num_tokens = util_t::string_split( tokens, buffer, " \t\n" );

  for( int i=0; i < num_tokens; i++ )
    if( ! parse( sim, tokens[ i ] ) )
      return false;

  return true;
}

// option_t::parse ==========================================================

bool option_t::parse( sim_t*       sim,
		      std::string& token )
{
  if( sim -> debug ) report_t::log( sim, "option_t::parse: %s", token.c_str() );

  if( token == "-" )
  {
    parse( sim, stdin );
    return true;
  }

  std::string::size_type cut_pt = token.find_first_of( "=" );

  if( cut_pt == token.npos )
  {
    FILE* file = fopen( token.c_str(), "r" );
    if( ! file )
    {
      fprintf( sim -> output_file, "simcraft: Unexpected parameter '%s'.  Expected format: name=value\n", token.c_str() );
      return false;
    }
    parse( sim, file );
    fclose( file );      
    return true;
  }
  
  std::string name, value;

  name  = token.substr( 0, cut_pt );
  value = token.substr( cut_pt + 1 );

  if( name == "output" )
  {
    if( ! sim -> parent )
    {
      if( sim -> output_file != stdout ) fclose( sim -> output_file );

      sim -> output_file = fopen( value.c_str(), "w" );
      if( ! sim -> output_file )
      {
	fprintf( stderr, "simcraft: Unable to open output file '%s'\n", value.c_str() );
	exit(0);
      }
    }
  }
  else if( name == "input" )
  {
    FILE* file = fopen( value.c_str(), "r" );
    if( ! file )
    {
      fprintf( sim -> output_file, "simcraft: Unable to open input parameter file '%s'\n", value.c_str() );
      exit(0);
    }
    parse( sim, file );
    fclose( file );      
  }
  else if( name == "druid" ) 
  { 
    sim -> active_player = player_t::create_druid( sim, value ); 
    assert( sim -> active_player );
  }
  else if( name == "hunter" ) 
  { 
    sim -> active_player = player_t::create_hunter( sim, value ); 
  }
  else if( name == "mage" ) 
  { 
    sim -> active_player = player_t::create_mage( sim, value ); 
  }
  else if( name == "priest" ) 
  { 
    sim -> active_player = player_t::create_priest( sim, value );
    assert( sim -> active_player );
  }
  else if( name == "rogue" ) 
  { 
    sim -> active_player = player_t::create_rogue( sim, value );
    assert( sim -> active_player );
  }
  else if( name == "shaman" )
  {
    sim -> active_player = player_t::create_shaman( sim, value );
    assert( sim -> active_player );
  }
  else if( name == "warlock" )
  {
    sim -> active_player = player_t::create_warlock( sim, value );
  }
  else if( name == "pet" ) 
  { 
    sim -> active_player = sim -> active_player -> create_pet( value );
    assert( sim -> active_player );
  }
  else if( name == "active"  ) 
  { 
    if( value == "owner" )
    {
      sim -> active_player = sim -> active_player -> cast_pet() -> owner;
    }
    else
    {
      for( sim -> active_player = sim -> player_list; sim -> active_player; sim -> active_player = sim -> active_player -> next )
      {
	if( value == sim -> active_player -> name() )
	  break;
      }
      assert( sim -> active_player );
    }
  }
  else if( name == "talents" )
  {
    sim -> active_player -> talents_str = value;

    std::string talent_string, address_string;
    int encoding = ENCODING_NONE;

    std::string::size_type cut_pt; 
    if( ( cut_pt = value.find_first_of( "=" ) ) != value.npos ) 
    {
       talent_string = value.substr( cut_pt + 1 );
      address_string = value.substr( 0, cut_pt );

      if( address_string.find( "worldofwarcraft" ) != value.npos )
      {
	encoding = ENCODING_BLIZZARD;
      }
      else if( address_string.find( "mmo-champion" ) != value.npos )
      {
        encoding = ENCODING_MMO;
      }
      else if( address_string.find( "wowhead" ) != value.npos )
      {
        encoding = ENCODING_WOWHEAD;
      }
    }

    if( encoding == ENCODING_NONE || ! sim -> active_player -> parse_talents( talent_string, encoding ) )
    {
      printf( "simcraft: Unable to decode talent string %s for %s\n", value.c_str(), sim -> active_player -> name() );
      exit( 0 );
    }
  }
  else
  {
    if( ! sim -> parse_option( name, value ) )
    {
      fprintf( sim -> output_file, "simcraft: Unknown option/value pair: '%s' : '%s'\n", name.c_str(), value.c_str() );
      return false;
    }
  }
  
  return true;
}

// option_t::parse ==========================================================

bool option_t::parse( sim_t* sim,
		      int    argc, 
		      char** argv )
{
  sim -> argc = argc;
  sim -> argv = argv;

  if( argc <= 1 ) return false;

  for( int i=1; i < argc; i++ )
  {
    if( ! option_t::parse( sim, argv[ i ] ) )
      return false;
  }

  if( sim -> max_time <= 0 && sim -> target -> initial_health <= 0 )
  {
    fprintf( sim -> output_file, "simcraft: One of -max_time or -target_health must be specified.\n" );
    return false;
  }

  if( sim -> parent ) 
  {
    sim -> debug = 0;
    sim -> log = 0;
  }
  if( sim -> debug ) 
  {
    sim -> log = 1;
    sim -> print_options();
  }
  if( sim -> log )
  {
    sim -> iterations = 1;
    sim -> threads = 1;
  }

  return true;
}

