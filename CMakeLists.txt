cmake_minimum_required(VERSION 3.21.0)
project(elfibia LANGUAGES C)

add_executable(elfibia draw-ncurses.c elfheader.c elfibia.c elfsections.c elfsegments.c)

target_link_libraries(elfibia PRIVATE ncurses menu elf)
