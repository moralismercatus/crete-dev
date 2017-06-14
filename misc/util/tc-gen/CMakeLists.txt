cmake_minimum_required(VERSION 2.8.7)

LIST(APPEND CMAKE_CXX_FLAGS -std=c++11)

project(tc-gen)

add_executable(crete-tc-gen tc_gen.cpp)

target_link_libraries(crete-tc-gen crete_test_case boost_system boost_filesystem boost_program_options)
