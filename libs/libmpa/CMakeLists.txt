cmake_minimum_required(VERSION 3.19)
project(libmpa)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(libmpa_src
        ptr_array.c)

add_library(libmpa ${libmpa_src})
