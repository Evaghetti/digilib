project(digilib C)
cmake_minimum_required(VERSION 3.16)

set(CMAKE_SHARED_LIBRARY_PREFIX "")

include_directories(include)

set(SRCS source/digimon.c source/digiapi.c source/digiworld.c source/digibattle_classic.c source/platform/pc/digihardware.c source/platform/pc/digicomm.c)

add_library(digilib SHARED ${SRCS})
target_link_libraries(digilib pthread)

add_executable(digiterminal digiterminal.c)
target_link_libraries(digiterminal digilib)