/* ------------------------------------------------------------------------------
MIT License

Copyright (c) 2024 Hugo M. Ayala

  Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
------------------------------------------------------------------------------ */

#ifndef PARSE_OPTIONS_HPP
#define PARSE_OPTIONS_HPP

#include <string>
#include <stdexcept>
#include <vector>
#include <sstream>

namespace parse_options
{
#define PARSE_OPTIONS_VERSION "1.0.0"

  class OptionRecord;   // Forward declare this class for the

  /// @class: OptionRecord
  /// @Description: This is the base class for the options of all types.  It contains the name
  /// and the description of each type of option.

  class OptionRecord
  {
    public:
      virtual ~OptionRecord() = default;

      virtual void parse( const char* value ) = 0;

      bool has_parameter() const { return has_parameter_; }

      bool matches( const std::string_view& arg_str ) const
      {
        return (name_.compare( 0, arg_str.size(), arg_str.data()) == 0);
      }

      const std::string& name() const { return name_; }

      const std::string& description() const { return description_; }

    protected:

      OptionRecord( const std::string_view& name, const std::string_view& description, bool has_parameter ) :
        name_( name ),
        description_( description ),
        has_parameter_( has_parameter ) {};

      std::string error_message( const std::string_view& err_str, const std::string_view& value )
      {
        std::ostringstream fmt;
        fmt << "Error: " << err_str << "\n";
        fmt << "  parameter: " << name_;

        if( not value.empty())
          {
            fmt << "  value: \"" << value << "\"\n";
          }

        return fmt.str();
      }

      std::string name_;
      std::string description_;
      bool has_parameter_;
  };


  /// @Class: ValueOption
  /// @Description: This is a generic class for an option that requires an parameter to be provided
  /// There is a specialized template for <bool> where the option is not required.
  template<class T>
  class ValueOption : public OptionRecord
  {
    public:
      ValueOption( const std::string_view& name, const std::string_view& description, T* dst_ptr ) :
        OptionRecord( name, description, not std::is_same<bool, T>::value ), dst_ptr_( dst_ptr ) {};

      void parse( const char* value ) override
      {
        if( value )
          {
            std::string value_str( value );
            std::istringstream is( value_str );
            int num_read = 0;

            T parsed_value;

            while( is.good())
              {
                if( is.peek() != EOF)
                  is >> parsed_value;
                else
                  break;

                num_read += 1;
              }

            if( not is.fail())
              {
                if( num_read == 1 )
                  {
                    if( dst_ptr_ )    // If this is null, the client wants us to silently ignore this parameter
                      {
                        *dst_ptr_ = parsed_value;
                      }
                  }
                else if( 1 < num_read )
                  {
                    throw std::invalid_argument( error_message( "too many arguments", value ));
                  }
                else
                  {
                    throw std::invalid_argument( error_message( "empty value string", value ));
                  }
              }
            else
              {
                throw std::invalid_argument( error_message( "parsing parameter failed", value ));
              }
          }
        else
          {
            throw std::invalid_argument( error_message( "missing argument", "" ));
          }
      }

    protected:
      T* dst_ptr_;    // Where to store the parsed value
  };

  /// @Class: SwitchOption
  /// @Description: This is a specialized version for boolean options that do not have parameters
  class SwitchOption : public ValueOption<bool>
  {
    public:
      SwitchOption( const std::string_view& opt_name, const std::string_view& description, bool* dst_ptr ) :
        ValueOption<bool>( opt_name, description, dst_ptr ) {}

      void parse( const char* /* param */ ) override
      {
        if( dst_ptr_ )
          {
            *dst_ptr_ = true;
          }
      }
  };

  class OptionParser
  {
    public:
      explicit OptionParser( const std::string_view& description = "" ) : description_( description ) {}

      ~OptionParser()
      {
        for( auto* one : option_ )
          {
            delete one;
          }

        option_.clear();
      }

      /// @Method: Add an option specifying the type and name
      /// @param T is the type for the option, either std::string, or int
      /// @param opt_name The name of the option, minus any dashes
      template<typename T>
      void add( const std::string_view& opt_name,
                const std::string_view& description,
                T* dst_ptr )
      {
        option_.push_back( new ValueOption<T>( opt_name, description, dst_ptr ));
      }

      /// @Method: Add a boolean option
      /// This is specialized version of the general add method for just booleans
      template<>
      void add<bool>( const std::string_view& opt_name,
                      const std::string_view& description,
                      bool* dst_ptr )
      {
        option_.push_back( new SwitchOption( opt_name, description, dst_ptr ));
      }

      /// @Method: parse
      /// @param argc The number of arguments as passed to main
      /// @param argv The list of pointers to the initializers
      void parse( int argc, const char* const argv[] )
      {
        for( int ii = 1; ii < argc; ii += 1 )
          {
            const char* pp = argv[ii];
            size_t plen = std::strlen( pp );

            if( 0 < plen )
              {
                int pi = 0;   // parameter index

                if( pp[0] == '-' )    // this is an option
                  {
                    pi = 1;
                    if( 1 < plen and pp[1] == '-' ) // check for '--'
                      {
                        pi = 2;
                      }

                    std::string param( &pp[pi], plen - pi );    // extract the parameter name

                    // loop over all of the options and see if this one that we recognize

                    bool found = false;

                    for( auto* one : option_ )
                      {
                        if( one->matches( param ))
                          {
                            found = true;

                            if( one->has_parameter() and ii + 1 < argc )   // extract the parameter
                              {
                                ii += 1;
                                one->parse( argv[ii] );
                                break;
                              }
                            else
                              {
                                one->parse( nullptr );
                              }
                          }
                      }

                    if( not found )
                      {
                        std::string err_str( "ERROR: unrecognized option: " );
                        err_str.append( pp );
                        err_str.append( "\n" );

                        throw std::invalid_argument( err_str );
                      }
                  }
                else
                  {
                    non_option_args_.emplace_back( pp, plen );
                  }
              } // else, the string is empty -- ignore it
          } // end for loop over the arguments
      }

      /// @Method: non_option_args
      /// @returns A vector contained all of the parameters not used as options
      const std::vector<std::string>& non_option_args() const { return non_option_args_; }

      const std::string usage() const
      {
        std::string u_str = description_ + "\n\nOPTIONS:\n\n";

        const int break_col = 20;

        for( const auto& one : option_ )
          {
            u_str.append( "  --" );
            u_str.append( one->name());

            int num_align = break_col - (one->name().length() + 4);
            if( 0 < num_align )
              {
                u_str.append( num_align, ' ' );
              }
            else
              {
                u_str.append( "\n" );
                u_str.append( break_col, ' ' );
              }
            u_str.append( one->description());
            u_str.append( "\n" );
          }

        return u_str;
      }

    protected:

      std::string description_;
      std::vector<OptionRecord*> option_;
      std::vector<std::string> non_option_args_;
  };
}

#endif //PARSE_OPTIONS_HPP
