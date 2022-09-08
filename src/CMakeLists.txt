cmake_minimum_required(VERSION 3.20)
project(Serene)

set(CMAKE_CXX_STANDARD 23)

include_directories(../include/)
add_executable(Serene
        BYTECODE.h
        BYTECODE.cpp

        VM/

        main.cpp)