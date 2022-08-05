project(digilib C)
cmake_minimum_required(VERSION 3.16)

if ( CMAKE_COMPILER_IS_GNUCC )
    set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -Wall -Werror")
endif()
if ( MSVC )
    set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} /W4 /WX")
endif()

set(CMAKE_SHARED_LIBRARY_PREFIX "")

set(PLATFORM "pc" CACHE STRING "Which platform to build")
set_property(CACHE PLATFORM PROPERTY STRINGS "pc")

set(DEVICE "Digital Monster" CACHE STRING "Which VPet to simulate (if Custom is selected, set TIME_TO_GET_HUNGRY, TIME_TO_GET_WEAKER, TIME_TO_POOP and TIME_TO_GET_CARE_MISTAKE, all are UInt16)")
set_property(CACHE DEVICE PROPERTY STRINGS "Digital Monster" "Custom")

option(BUILD_DIGIVICE off)

# Needs python to prepare digimons
find_package(Python REQUIRED Interpreter)
add_custom_command(
    OUTPUT include/digiworld.h source/digiworld.c
    COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate-digimon-header.py ${CMAKE_CURRENT_SOURCE_DIR}/import-digimon.csv ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS import-digimon.csv
)

# Build the lib
include_directories(include)
add_library(digilib SHARED)
target_sources(digilib PRIVATE source/digimon.c source/digiapi.c source/digiworld.c source/digibattle_classic.c source/digiworld.c)
target_sources(digilib PRIVATE source/platform/${PLATFORM}/digihardware.c source/platform/${PLATFORM}/logging.c)

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

if(${BUILD_DIGIVICE})
    message(STATUS "Will build digivice")
    set(SOURCES_DIGIVICE digivice.c source/digivice/game.c source/digivice/texture.c source/digivice/animation.c source/digivice/menu.c source/digivice/avatar.c source/digivice/button.c source/digivice/battle.c source/digivice/globals.c source/digivice/w0rld.c)
    if (${CMAKE_SYSTEM_NAME} STREQUAL "Android")
        set(DIGIVICE_TARGET "libmain")
        add_library(${DIGIVICE_TARGET} SHARED)
        target_compile_definitions(${DIGIVICE_TARGET} PUBLIC _NEEDS_SDL_MAIN_ _USE_DISPLAY_MODE_ _ANDROID_BUILD_)
        set(SOURCES_DIGIVICE ${SOURCES_DIGIVICE} source/digivice/dv_android.c source/digivice/w0rld_unix.c)
    else()
        set(DIGIVICE_TARGET "digivice")

        # Get SDL2 include dir
        find_package(SDL2 REQUIRED) 
        
        add_executable(${DIGIVICE_TARGET})
        include_directories(${SDL2_INCLUDE_DIR})
        if (WIN32)
        else()
            set(SOURCES_DIGIVICE ${SOURCES_DIGIVICE} source/digivice/w0rld_unix.c)
        endif()
    endif()
    
    target_sources(${DIGIVICE_TARGET} PRIVATE ${SOURCES_DIGIVICE})
    target_link_libraries(${DIGIVICE_TARGET} digilib SDL2 SDL2main SDL2_image SDL2_ttf SDL2_net)
    include_directories(include/digivice)
endif()
