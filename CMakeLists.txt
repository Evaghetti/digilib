project(digilib C)
cmake_minimum_required(VERSION 3.16)

set(CMAKE_SHARED_LIBRARY_PREFIX "")

set(PLATFORM "pc" CACHE STRING "Which platform to build")
set_property(CACHE PLATFORM PROPERTY STRINGS "pc")

set(DEVICE "Digital Monster" CACHE STRING "Which VPet to simulate")
set_property(CACHE DEVICE PROPERTY STRINGS "Digital Monster")

option(BUILD_DIGIVICE off)

# Needs python to prepare digimons
find_package(Python REQUIRED Interpreter)
add_custom_command(
    OUTPUT include/digiworld.h source/digiworld.c
    COMMAND ${Python_EXECUTABLE} generate-digimon-header.py
    DEPENDS import-digimon.csv
)

set(SRCS source/digimon.c source/digiapi.c source/digiworld.c source/digibattle_classic.c source/digiworld.c)
set(SRCS ${SRCS} source/platform/${PLATFORM}/digihardware.c source/platform/${PLATFORM}/digicomm.c source/platform/${PLATFORM}/logging.c)

# Build the lib
include_directories(include)
add_library(digilib SHARED ${SRCS})
target_link_libraries(digilib pthread)

message(STATUS "Using ${DEVICE}'s times for events")
if(${DEVICE} STREQUAL "Digital Monster")
    target_compile_definitions(digilib PUBLIC TIME_TO_GET_HUNGRY=60)
    target_compile_definitions(digilib PUBLIC TIME_TO_GET_WEAKER=65)
    target_compile_definitions(digilib PUBLIC TIME_TO_POOP=180)
    target_compile_definitions(digilib PUBLIC TIME_TO_GET_CARE_MISTAKE=20)
endif()

if(${BUILD_DIGIVICE})
    message(STATUS "Will build digivice")
    add_executable(digivice digivice.c source/digivice/game.c source/digivice/texture.c source/digivice/animation.c source/digivice/menu.c source/digivice/avatar.c source/digivice/button.c source/digivice/globals.c)
    include_directories(include/digivice)
    target_link_libraries(digivice digilib SDL2 SDL2_image SDL2_ttf)
endif()
