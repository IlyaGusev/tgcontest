cmake_minimum_required(VERSION 3.0.2)

project(tgnews)

set(CMAKE_MACOSX_RPATH 1)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "-pthread -funroll-loops -O3 -march=native -Wall -Wextra")

add_subdirectory(thirdparty/fasttext EXCLUDE_FROM_ALL)
add_subdirectory(thirdparty/tinyxml2)

set(SOURCE_FILES
    src/detect.cpp
    src/main.cpp
    src/parser.cpp
)

set(HEADER_FILES
    src/detect.h
    src/document.h
    src/parser.h
)

set(Boost_USE_MULTITHREADED ON)
find_package(Boost COMPONENTS program_options filesystem REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIR})
set(LIB_LIST
    ${Boost_LIBRARIES}
)

add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})
target_link_libraries(${PROJECT_NAME} PUBLIC ${LIB_LIST} PRIVATE fasttext-static PRIVATE tinyxml2)
