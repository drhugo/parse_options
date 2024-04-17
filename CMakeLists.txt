cmake_minimum_required(VERSION 3.27)
project(parse_options)

set(CMAKE_CXX_STANDARD 17)

# Prepare doctest for other targets to use
find_package(doctest REQUIRED)

include_directories(/usr/local/include)

add_executable(tests
        test_parse_options.cpp
        parse_options.hpp)

target_compile_features(tests PRIVATE cxx_std_17)
target_link_libraries(tests PRIVATE doctest::doctest)

add_executable(parse_options
        parse_options.cpp
        parse_options.hpp)

