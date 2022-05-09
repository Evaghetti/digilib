project(digilib C)
cmake_minimum_required(VERSION 3.16)

set(CMAKE_SHARED_LIBRARY_PREFIX "")

set(PLATFORM "pc" CACHE STRING "Which platform to build")

# Needs python to prepare digimons
find_package(Python REQUIRED Interpreter)
add_custom_command(
    OUTPUT include/digiworld.h source/digiworld.c
    COMMAND ${Python_EXECUTABLE} generate-digimon-header.py
    DEPENDS import-digimon.csv
)

# Build the lib
set(SRCS source/digimon.c source/digiapi.c source/digiworld.c source/digibattle_classic.c source/digiworld.c)
set(SRCS ${SRCS} source/platform/${PLATFORM}/digihardware.c source/platform/${PLATFORM}/digicomm.c)

include_directories(include)
add_library(digilib SHARED ${SRCS})
target_link_libraries(digilib pthread)

add_executable(digiterminal digiterminal.c)
target_link_libraries(digiterminal digilib)