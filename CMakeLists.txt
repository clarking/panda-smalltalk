cmake_minimum_required(VERSION 3.19)
project(panda)

set(PANDA_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} "-g  -std=gnu11")
add_compile_definitions(ST_DEBUG)
set(PANDA_SRC
        src/main.c
        src/array.c
        src/association.c
        src/behavior.c
        src/character.c
        src/compiler.c
        src/dictionary.c
        src/float.c
        src/generator.c
        src/heap.c
        src/identity-hashtable.c
        src/input.c
        src/large-integer.c
        src/lexer.c
        src/machine.c
        src/memory.c
        src/node.c
        src/object.c
        src/parser.c
        src/primitives.c
        src/symbol.c
        src/system.c
        src/unicode.c
        src/universe.c
        src/utils.c)

add_subdirectory(libs/libmpa)
add_subdirectory(libs/libtommath)

add_executable(panda ${PANDA_SRC})
target_include_directories(panda PUBLIC src/ libs/libmpa libs/libtommath)

target_link_libraries(panda -lm -lreadline -ldl -lpthread libmpa libtommath)
