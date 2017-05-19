cmake_minimum_required(VERSION 2.8.7)

project(guest)

### enforce compile 32 bits on 64 bits OS
# LIST(APPEND CMAKE_CXX_FLAGS -m32)
# LIST(APPEND CMAKE_C_FLAGS -m32)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include_directories("${PROJECT_SOURCE_DIR}/lib/include" "${PROJECT_BINARY_DIR}/lib/boost/boost-prefix/src/boost_1_59_0")
link_directories(${CMAKE_BINARY_DIR}/bin "${PROJECT_BINARY_DIR}/bin/boost")

add_subdirectory(lib)
add_subdirectory(util)
add_subdirectory(evals)
