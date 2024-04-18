#include <iostream>
#include <string.h>

#include <string>
#include <vector>
#include <filesystem>

#include "parse_options.hpp"

// This program is a test of how to parse options in C++

struct programOptions
{
  bool verbose{false};
  std::filesystem::path input;
  std::filesystem::path output;
  std::vector<std::string> source;    // the source files
  int integer{0};
};

/* ----------------------------------------------------------------------------
 * process_test
---------------------------------------------------------------------------- */
int process_test( const programOptions& options, const std::string_view& file_path )
{
  std::cout << "Verbose: " << options.verbose << "\n";
  std::cout << "input_path: " << options.input << "\n";
  std::cout << "output_path: " << options.output << "\n";
  std::cout << "file_path: " << file_path << "\n";

  return 0;
}

/* ----------------------------------------------------------------------------
 * main
---------------------------------------------------------------------------- */
int main( int argc, char* argv[] )
{
  programOptions options;
  parse_options::OptionParser parser( "This is the test framework for the option parser" );

  parser.add( "verbose", "Print semi-useful stuff", &options.verbose );
  parser.add( "input_path", "The path to read information from", &options.input );
  parser.add( "output_path", "The path to write data to", &options.output );
  parser.add( "real_long_option_name", "This option has a lot of text to test wrapping", &options.integer );

  int status = 0;

  if( 1 < argc )
    {
      try
        {
          parser.parse( argc, argv );

          // If we didn't throw, then out options were parsed correctly.

          for( const auto& one : parser.non_option_args())
            {
              status = process_test( options, one );

              if( status != 0 ) break;
            }
        }

      catch( std::invalid_argument& e1 )
        {
          std::cerr << "# Failed to parse program options\n";
          std::cerr << "# " << e1.what();

          status = 1;
        }

      catch( const std::exception& e2 )
        {
          std::cerr << "ERROR: " << e2.what() << "\n";

          status = 2;
        }
    }
  else
    {
      std::cout << parser.usage().c_str();
    }

  return status;
}
