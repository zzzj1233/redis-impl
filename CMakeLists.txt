cmake_minimum_required(VERSION 3.16)
project(redis-impl C)

set(CMAKE_C_STANDARD 99)

add_executable(sds src/sds.c)
add_executable(dict src/dict.c src/sds.c src/sds.h)
add_executable(adlist src/adlist.c src/sds.c src/sds.h)
add_executable(intset src/intset.c src/sds.c src/sds.h)
add_executable(ziplist src/ziplist.c src/sds.c src/sds.h src/util.c src/util.h)
add_executable(skplist src/skplist.c src/skplist.h)
