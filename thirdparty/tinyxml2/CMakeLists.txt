cmake_minimum_required(VERSION 2.8.4)
project(tinyxml2)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread -funroll-loops -O3 -march=native")


set(HEADER_FILES
    tinyxml2.h
)

set(SOURCE_FILES
    tinyxml2.cpp
)

add_library(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})
