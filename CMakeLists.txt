cmake_minimum_required (VERSION 3.17)
project (power LANGUAGES CXX)

include(GNUInstallDirs)

add_compile_options( -std=c++17 )
add_compile_options( -g )
add_compile_options( -Wfatal-errors )

subdirs(src)

