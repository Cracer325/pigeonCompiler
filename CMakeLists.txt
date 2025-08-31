cmake_minimum_required(VERSION 3.20)
project(pigeon)

set(CMAKE_CXX_STANDARD 20)

add_executable(pigeon src/main.cpp
        src/tokenization.hpp
        src/parser.hpp
        src/generation.hpp
        src/arena.hpp)
