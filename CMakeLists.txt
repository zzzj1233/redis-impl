cmake_minimum_required(VERSION 3.16)
project(redis-impl C)

set(CMAKE_C_STANDARD 99)

add_executable(sds src/sds.c)
add_executable(adlist src/adlist.c src/sds.c src/sds.h)
