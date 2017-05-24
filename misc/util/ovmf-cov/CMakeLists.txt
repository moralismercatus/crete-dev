cmake_minimum_required(VERSION 2.8.7)

LIST(APPEND CMAKE_CXX_FLAGS -std=c++11)

project(ovmf-cov)

add_executable(crete-ovmf-cov ovmf_cov.cpp)

target_link_libraries(crete-ovmf-cov boost_system boost_filesystem boost_program_options)
