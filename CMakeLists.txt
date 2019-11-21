cmake_minimum_required(VERSION 2.8.4)
project(tgnews)

include_directories(fasttext/fasttext)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread -funroll-loops -O3 -march=native")

add_subdirectory(thirdparty/fasttext)

set(HEADER_FILES
    src/tinyxml2.h
)

set(SOURCE_FILES
    src/main.cpp
    src/tinyxml2.cpp
)

set(Boost_USE_MULTITHREADED ON)
find_package(Boost COMPONENTS program_options filesystem REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIR})

set(LIB_LIST
    ${Boost_LIBRARIES}
)

add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})
target_link_libraries(${PROJECT_NAME} PUBLIC ${LIB_LIST} PRIVATE fasttext-static)
