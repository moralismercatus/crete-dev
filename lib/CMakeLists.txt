cmake_minimum_required(VERSION 2.8.7)

project(lib)

add_subdirectory(asio)
add_subdirectory(boost)
add_subdirectory(cluster)
add_subdirectory(elf-reader)
add_subdirectory(replay-preload)
add_subdirectory(logger)
add_subdirectory(proc-reader)
add_subdirectory(test-case)
add_subdirectory(stp)

include(ExternalProject)

ExternalProject_Add(
    fmtlib

    GIT_REPOSITORY
        https://github.com/fmtlib/fmt.git
    CMAKE_ARGS
        -DBUILD_SHARED_LIBS=TRUE
        -DCMAKE_CXX_FLAGS=-D_GLIBCXX_USE_CXX11_ABI=0
        -DCMAKE_LIBRARY_OUTPUT_DIRECTORY=${CMAKE_BINARY_DIR}/bin

    UPDATE_COMMAND ""
    INSTALL_COMMAND
        ln -sf
        ${CMAKE_BINARY_DIR}/lib/fmtlib-prefix/src/fmtlib/include/fmt
        ${CMAKE_SOURCE_DIR}/lib/include
)
