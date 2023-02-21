cmake_minimum_required (VERSION 3.0.0)
project (afa-emptiness-checker)

set (CMAKE_CXX_STANDARD 17)

find_library(LIBMATA mata REQUIRED)

add_subdirectory(libvata)
add_subdirectory(src)