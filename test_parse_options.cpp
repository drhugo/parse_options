//
// Created by Hugo Ayala on 4/16/24.
//
#include <string>
#include <vector>

#include "parse_options.hpp"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest/doctest.h>

class cli_helper
{
  public:
    explicit cli_helper( const std::string_view& cmd_line )
    {
      auto len = cmd_line.size();
      int ii = 0;

      if( ii < len )
        {
          do
            {
              int nn = cmd_line.find( ' ', ii + 1 );

              if( ii < nn )   // we found a string
                {
                  arglist_.emplace_back( cmd_line.substr( ii, nn - ii ));
                  ii = nn + 1;  // skip over the space
                }
              else if( nn == std::string::npos )    // the end of the string?
                {
                  arglist_.emplace_back( cmd_line.substr( ii, len - ii + 1 ));
                  ii = len;
                }

            } while( ii < len );
        }

      // We have to wait to build this array after all strings have been allocated
      for( auto& one : arglist_ )
        {
          argv_.push_back( one.c_str());
        }
    }

    int argc() const { return argv_.size(); }

    const char** argv() { return argv_.data(); }

  private:
    std::vector<std::string> arglist_;
    std::vector<const char*> argv_;
};

/* ----------------------------------------------------------------------------
 * Test Cases
---------------------------------------------------------------------------- */
TEST_CASE( "Simple Integration Test" )
{
  struct booleanOptionRecord
  {
    bool option{false};
  } booleanOption;

  parse_options::OptionParser parser( "" );

  parser.add( "boolean", "A boolean option", &booleanOption.option );

  REQUIRE( booleanOption.option == false );

  cli_helper ch( "program_name --boolean ignored" );
  parser.parse( ch.argc(), ch.argv());
  CHECK( booleanOption.option == true );

  REQUIRE( parser.non_option_args().size() == 1 );
  CHECK( parser.non_option_args().at( 0 ) == "ignored" );
}

TEST_CASE( "Partial Name Matching" )
{
  // Check that for a given option, single dash, double dash and partial names all will match
  struct
  {
    bool option{false};
  } testOption;

  parse_options::OptionParser parser( "" );

  parser.add( "boolean", "A boolean option", &testOption.option );

  const char* argv[2];

  argv[0] = "program_name";

  SUBCASE( "single dash" )
    {
      REQUIRE( testOption.option == false );
      argv[1] = "-boolean";
      parser.parse( 2, argv );
      CHECK( testOption.option == true );
    }
  SUBCASE( "double dash" )
    {
      REQUIRE( testOption.option == false );
      argv[1] = "--boolean";
      parser.parse( 2, argv );
      CHECK( testOption.option == true );
    }
  SUBCASE( "ignored" )
    {
      REQUIRE( testOption.option == false );
      argv[1] = "boolean";
      parser.parse( 2, argv );
      CHECK( testOption.option == false );
    }
  SUBCASE( "partial one dash" )
    {
      REQUIRE( testOption.option == false );
      argv[1] = "-bool";
      parser.parse( 2, argv );
      CHECK( testOption.option == true );
    }
  SUBCASE( "partial two dash" )
    {
      REQUIRE( testOption.option == false );
      argv[1] = "--bool";
      parser.parse( 2, argv );
      CHECK( testOption.option == true );
    }
  SUBCASE( "too long" )
    {
      REQUIRE( testOption.option == false );
      argv[1] = "--boolean_extra";
      CHECK_THROWS_AS( parser.parse( 2, argv ), std::invalid_argument );
    }
}

TEST_CASE( "Parse Different Types" )
{
  struct testOptionRecord
  {
    bool bool_option{false};
    int int_number{0};
    float float_value{0.f};
    std::string str_value;
  } testOption;

  parse_options::OptionParser parser( "Testing general case" );

  parser.add( "boolean", "A boolean option", &testOption.bool_option );
  parser.add( "integer", "An integer option", &testOption.int_number );
  parser.add( "float", "A floating point number", &testOption.float_value );
  parser.add( "string", "A string value", &testOption.str_value );

  SUBCASE( "Test booleans" )
    {
      REQUIRE( testOption.bool_option == false );
      cli_helper ch( "program --boolean" );
      parser.parse( ch.argc(), ch.argv());
      CHECK( testOption.bool_option == true );
    }
  SUBCASE( "Test integers" )
    {
      REQUIRE( testOption.int_number == 0 );
      cli_helper ch( "program --integer 42" );
      parser.parse( ch.argc(), ch.argv());
      CHECK( testOption.int_number == 42 );
    }

  SUBCASE( "Test Float" )
    {
      REQUIRE( testOption.float_value == 0.f );
      cli_helper ch( "program --float 3.1415" );
      parser.parse( ch.argc(), ch.argv());
      CHECK( testOption.float_value == doctest::Approx( 3.1415 ));
    }
  SUBCASE( "Test String" )
    {
      CHECK( testOption.str_value.empty());
      cli_helper ch( "program --string that_is_not_my_name" );
      parser.parse( ch.argc(), ch.argv());
      CHECK( testOption.str_value == "that_is_not_my_name" );
    }
}

TEST_CASE( "Check Errors" )
{
  struct
  {
    int int_value{0};
    bool missing_bool;
  } testOptions;

  parse_options::OptionParser parser( "Checks for errors" );

  SUBCASE( "negative numbers" )
    {
      parser.add( "integer", "An option that expects an integer", &testOptions.int_value );

      cli_helper ch( "program --integer -1" );
      parser.parse( ch.argc(), ch.argv());
      CHECK( testOptions.int_value == -1 );
    }
  SUBCASE( "found option instead" )
    {
      parser.add( "integer", "A test integer", &testOptions.int_value );
      parser.add( "missing", "A test boolean named 'missing'", &testOptions.missing_bool );

      cli_helper ch( "program --integer --missing" );

      CHECK_THROWS_WITH( parser.parse( ch.argc(), ch.argv()),
                         doctest::Contains( "parsing parameter failed" ));
    }
  SUBCASE( "parsing failed" )
    {
      parser.add( "integer", "An option that expects an integer", &testOptions.int_value );

      cli_helper ch( "program --integer one" );

      CHECK_THROWS_WITH( parser.parse( ch.argc(), ch.argv()),
                         doctest::Contains( "parsing parameter failed" ));
    }
  SUBCASE( "too many arguments" )
    {
      parser.add( "integer", "An option that takes one and only one integer", &testOptions.int_value );

      const char* argv[3];
      argv[0] = "program";
      argv[1] = "--integer";
      argv[2] = "1 2 3";

      CHECK_THROWS_WITH( parser.parse( 3, argv ), doctest::Contains( "too many arguments" ));
    }
  SUBCASE( "missing arguments" )
    {
      parser.add( "integer", "An option that takes one and only one integer", &testOptions.int_value );

      const char* argv[3];
      argv[0] = "program";
      argv[1] = "--integer";
      argv[2] = "";

      SUBCASE( "empty string" )
        {
          CHECK_THROWS_WITH( parser.parse( 3, argv ), doctest::Contains( "empty value string" ));
        }

      SUBCASE( "missing argument" )
        {
          // notice argc = 2 here, so the last parameter passed is '--integer'
          CHECK_THROWS_WITH( parser.parse( 2, argv ), doctest::Contains( "missing argument" ));
        }
    }
  SUBCASE( "ignore option value" )
    {
      parser.add<int>( "integer", "An option that consumes and argument but is ignored", nullptr );
      parser.add<bool>( "missing_bool", "An option where the value  should be silently ignored", nullptr );

      cli_helper ch( "program --integer 1 --missing_bool" );

      CHECK_NOTHROW( parser.parse( ch.argc(), ch.argv()));
    }
}

TEST_CASE( "Check Usage" )
{
  struct
  {
    bool one;
    bool two;
    bool three;
  } testOption;

  parse_options::OptionParser parser( "Tool description" );
  parser.add( "one", "This is the first option", &testOption.one );
  parser.add( "two", "This is the second option", &testOption.two );
  parser.add( "twenty_letters_long", "This is the third option", &testOption.three );

  std::string usage = parser.usage();
  CHECK( usage ==       "Tool description\n\n"
                        "OPTIONS:\n\n"
                        "  --one             This is the first option\n"
                        "  --two             This is the second option\n"
                        "  --twenty_letters_long\n"
                        "                    This is the third option\n" );
}