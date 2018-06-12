cmake_minimum_required(VERSION 3.9)
project(prefix_tree)

set(CMAKE_CXX_STANDARD 17)

add_executable(prefix_tree main.cpp prefix_tree.h charset.h iterator.h)