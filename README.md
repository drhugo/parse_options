# parse_options

A header-only library for parsing options in C++.  To use, download the file `parse_options.hpp` and include it with your build.  To see a complete example, please refer to the file `parse_options.cpp`.  For a tests (using [doctest](https://github.com/doctest/doctest)) see the file `test_parse_options.cpp`.

Sample Use:

```
int main( int argc, const char *argv[] )
{
  struct
  {
    bool verbose{false};
    std::filesystem::path input;
    std::filesystem::path output;
    int integer{0};
  } options;

  parse_options::OptionParser parser( "This is the test framework for the option parser" );

  parser.add( "verbose", "Print semi-useful stuff", &options.verbose );
  parser.add( "input_path", "The path to read information from", &options.input );
  parser.add( "output_path", "The path to write data to", &options.output );
  parser.add( "real_long_option_name", "This option has a lot of text to test wrapping", &options.integer );

  int status = 0;

  try
  {
    parser.parse( argc, argv );
	
	for( const auto& one: parser.non_option_args() )
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
	
  return status;
}  

```