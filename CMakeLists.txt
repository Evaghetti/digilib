project(digilib C)
cmake_minimum_required(VERSION 3.16)

if ( CMAKE_COMPILER_IS_GNUCC )
    set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -Wall -Werror")
endif()
if ( MSVC )
    set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} /W4 /WX")
endif()

set(CMAKE_SHARED_LIBRARY_PREFIX "")
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/toolchain/sdl2)

set(PLATFORM "pc" CACHE STRING "Which platform to build")
set_property(CACHE PLATFORM PROPERTY STRINGS "pc")

set(DEVICE "Digital Monster" CACHE STRING "Which VPet to simulate (if Custom is selected, set TIME_TO_GET_HUNGRY, TIME_TO_GET_WEAKER, TIME_TO_POOP and TIME_TO_GET_CARE_MISTAKE, all are UInt16)")
set_property(CACHE DEVICE PROPERTY STRINGS "Digital Monster" "Custom")

option(BUILD_DIGIVICE off)

# Needs python to prepare digimons
find_package(Python REQUIRED Interpreter)
add_custom_command(
    OUTPUT include/digiworld.h source/digiworld.c
    COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate-digimon-header.py ${CMAKE_CURRENT_SOURCE_DIR}/import-digimon.csv ${PROJECT_BINARY_DIR}
    DEPENDS import-digimon.csv
)

# Build the lib
add_library(digilib SHARED)
target_include_directories(
    digilib 
    PRIVATE include
    PRIVATE ${PROJECT_BINARY_DIR}/include
)
target_sources(digilib PRIVATE source/digimon.c source/digiapi.c source/digiworld.c source/digibattle_classic.c source/digihal.c ${PROJECT_BINARY_DIR}/source/digiworld.c)

message(STATUS "Using ${DEVICE}'s times for events")
if(${DEVICE} STREQUAL "Digital Monster")
    set(TIME_TO_GET_HUNGRY 60)
    set(TIME_TO_GET_WEAKER 65)
    set(TIME_TO_POOP 180)
    set(TIME_TO_GET_CARE_MISTAKE 20)
else()
    if (NOT DEFINED TIME_TO_GET_HUNGRY)
        message(FATAL_ERROR "You did not set the variable TIME_TO_GET_HUNGRY")
    endif()
    if (NOT DEFINED TIME_TO_GET_WEAKER)
        message(FATAL_ERROR "You did not set the variable TIME_TO_GET_WEAKER")
    endif()
    if (NOT DEFINED TIME_TO_POOP)
        message(FATAL_ERROR "You did not set the variable TIME_TO_POOP")
    endif()
    if (NOT DEFINED TIME_TO_GET_CARE_MISTAKE)
        message(FATAL_ERROR "You did not set the variable TIME_TO_GET_CARE_MISTAKE")
    endif()
endif()

target_compile_definitions(digilib PUBLIC TIME_TO_GET_HUNGRY=${TIME_TO_GET_HUNGRY})
target_compile_definitions(digilib PUBLIC TIME_TO_GET_WEAKER=${TIME_TO_GET_WEAKER})
target_compile_definitions(digilib PUBLIC TIME_TO_POOP=${TIME_TO_POOP})
target_compile_definitions(digilib PUBLIC TIME_TO_GET_CARE_MISTAKE=${TIME_TO_GET_CARE_MISTAKE})

# TODO: change when making logic of vpet
if (${BUILD_DIGIVICE})
    # Get SDL2 include dir
    find_package(SDL2 REQUIRED)
    find_package(SDL2_image REQUIRED)

    add_custom_command(
        OUTPUT include/digivice/sprites.h source/digivice/sprites.c
        COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate-sprites-header.py ${CMAKE_CURRENT_SOURCE_DIR}/import-digimon.csv ${PROJECT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/resource
        DEPENDS import-digimon.csv
    )
    add_executable(digitest digitest.c ${PROJECT_BINARY_DIR}/source/digivice/sprites.c source/digivice/sprites.c)
    target_link_libraries(digitest digilib SDL2::Main SDL2::Image)
    target_include_directories(
        digitest 
        PRIVATE include
        PRIVATE ${PROJECT_BINARY_DIR}/include
        PRIVATE ${PROJECT_BINARY_DIR}/include/digivice
        PRIVATE ${SDL2_INCLUDE_DIR}
    )
endif()