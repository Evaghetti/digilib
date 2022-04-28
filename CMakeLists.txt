project(digilib C)
cmake_minimum_required(VERSION 3.16)

include_directories(include)

set(SRCS main.c)

add_executable(digilib ${SRCS})