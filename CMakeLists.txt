cmake_minimum_required(VERSION 3.0.2)

project(tgnews)

set(CMAKE_MACOSX_RPATH 1)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "-pthread -funroll-loops -O3 -march=native -Wall -Wextra")

add_subdirectory(thirdparty/fasttext EXCLUDE_FROM_ALL)
add_subdirectory(thirdparty/tinyxml2)
add_subdirectory(thirdparty/eigen EXCLUDE_FROM_ALL)

set(SOURCE_FILES
    src/detect.cpp
    src/main.cpp
    src/parser.cpp
    src/clustering/clustering.cpp
    src/clustering/dbscan.cpp
    src/clustering/slink.cpp
)

set(HEADER_FILES
    src/detect.h
    src/document.h
    src/parser.h
    src/clustering/clustering.h
    src/clustering/dbscan.h
    src/clustering/slink.h
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

find_package(OpenMP)
if (OPENMP_FOUND)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()

find_package(MLPACK REQUIRED)
include_directories(${MLPACK_INCLUDE_DIRS})
link_directories(${MLPACK_LIBRARIES})

set(Boost_USE_MULTITHREADED ON)
find_package(Boost COMPONENTS program_options filesystem REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIR})

set(LIB_LIST
    ${Boost_LIBRARIES}
    ${MLPACK_LIBRARIES}
)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/fasttext/src")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/thirdparty")

add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})
target_link_libraries(${PROJECT_NAME} PUBLIC ${LIB_LIST} PRIVATE fasttext-static PRIVATE tinyxml2 PRIVATE eigen)
