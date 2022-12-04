cmake_minimum_required(VERSION 3.19)
project(panda)

set(PANDA_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} "-g  -std=gnu11")
add_compile_definitions(ST_DEBUG)
set(PANDA_SRC
        src/main.c
        src/st-array.c
        src/st-association.c
        src/st-behavior.c
        src/st-character.c
        src/st-compiler.c
        src/st-dictionary.c
        src/st-float.c
        src/st-generator.c
        src/st-heap.c
        src/st-identity-hashtable.c
        src/st-input.c
        src/st-large-integer.c
        src/st-lexer.c
        src/st-machine.c
        src/st-memory.c
        src/st-node.c
        src/st-object.c
        src/st-parser.c
        src/st-primitives.c
        src/st-symbol.c
        src/st-system.c
        src/st-unicode.c
        src/st-universe.c
        src/st-utils.c)

add_subdirectory(libs/libmpa)
add_subdirectory(libs/libtommath)

add_executable(panda ${PANDA_SRC})
target_include_directories(panda PUBLIC src/ libs/libmpa libs/libtommath)

target_link_libraries(panda -lm -lreadline -ldl -lpthread libmpa libtommath)
