cmake_minimum_required(VERSION 3.19)
project(libtommath)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(libtommath_src
        mpi.c)

add_library(libtommath ${libtommath_src})
