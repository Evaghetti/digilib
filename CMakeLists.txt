project(digilib C)
cmake_minimum_required(VERSION 3.16)

set(CMAKE_SHARED_LIBRARY_PREFIX "")

set(PLATFORM "pc" CACHE STRING "Which platform to build")
set_property(CACHE PLATFORM PROPERTY STRINGS "pc")

set(DEVICE "Digital Monster" CACHE STRING "Which VPet to simulate")
set_property(CACHE DEVICE PROPERTY STRINGS "Digital Monster")

option(BUILD_DIGITERMINAL off)

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

if(${BUILD_DIGITERMINAL})
    message(STATUS "Will build digiterminal")
    add_executable(digiterminal digiterminal.c)
    target_link_libraries(digiterminal digilib)
endif()
