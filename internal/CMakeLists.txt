# Tests cmake installation
cmake_minimum_required(VERSION 3.30)
project(hmap_internal VERSION 1.0.0 LANGUAGES C)

find_package(hmap REQUIRED)

add_executable(main main.c)
target_link_libraries(main hmap::hmap)