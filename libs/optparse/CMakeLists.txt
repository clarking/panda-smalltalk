cmake_minimum_required(VERSION 3.19)
project(optparse)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(optparse_src
        fudd.c
        notpython.c
        optparse.c)

add_library(optparse ${optparse_src})