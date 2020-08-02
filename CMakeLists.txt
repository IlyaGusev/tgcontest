cmake_minimum_required(VERSION 3.9)

project(tgnews)

include(CTest)

set(CMAKE_MACOSX_RPATH 1)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
set(TGNEWS_CXX_FLAGS "-pthread;-Wall;-Wextra;-Wno-sign-compare;-fno-omit-frame-pointer")
set(TGNEWS_CXX_DEBUG_FLAGS "-g;-O2;-fsanitize=address")
set(TGNEWS_LNK_DEBUG_FLAGS "-fsanitize=address")
set(TGNEWS_CXX_RELEASE_FLAGS "-funroll-loops;-Ofast;-march=x86-64;-mtune=generic;-msse3;-mssse3;-msse4;-msse4.1;-msse4.2")

set(BUILD_TESTING 0)
add_subdirectory(thirdparty/fasttext EXCLUDE_FROM_ALL)
add_subdirectory(thirdparty/tinyxml2)
add_subdirectory(thirdparty/eigen EXCLUDE_FROM_ALL)

set(BUILD_DROGON_SHARED OFF CACHE BOOL "" FORCE)
set(BUILD_CTL OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_ORM OFF CACHE BOOL "" FORCE)
set(COZ_PROFILING OFF CACHE BOOL "" FORCE)
add_subdirectory(thirdparty/drogon EXCLUDE_FROM_ALL)
unset(COZ_PROFILIN)
unset(BUILD_ORM)
unset(BUILD_EXAMPLES)
unset(BUILD_CTL)

set(ROCKSDB_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(WITH_BENCHMARK_TOOLS OFF CACHE BOOL "" FORCE)
set(WITH_CORE_TOOLS OFF CACHE BOOL "" FORCE)
set(WITH_TOOLS OFF CACHE BOOL "" FORCE)
set(WITH_TESTS OFF CACHE BOOL "" FORCE)
set(WITH_GFLAGS OFF CACHE BOOL "" FORCE)
add_subdirectory(thirdparty/rocksdb EXCLUDE_FROM_ALL)
unset(WITH_GFLAGS)
unset(WITH_TESTS)
unset(WITH_TOOLS)
unset(WITH_CORE_TOOLS)
unset(WITH_BENCHMARK_TOOLS)

set(LIB_ONLY True)
add_subdirectory(thirdparty/onmt_tokenizer EXCLUDE_FROM_ALL)
unset(LIB_ONLY)

unset(BUILD_TESTING)

find_package(Boost COMPONENTS program_options filesystem unit_test_framework REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIR})

if (NOT APPLE)
    find_package(OpenMP)
    if (OPENMP_FOUND OR OPENMP_CXX_FOUND)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")
    endif()
endif()

find_package(Torch REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")

find_package(Protobuf REQUIRED)

set(SOURCE_FILES
    src/agency_rating.cpp
    src/annotator.cpp
    src/cluster.cpp
    src/clusterer.cpp
    src/clustering/slink.cpp
    src/controller.cpp
    src/db_document.cpp
    src/detect.cpp
    src/document.cpp
    src/embedders/tfidf_embedder.cpp
    src/embedders/ft_embedder.cpp
    src/embedders/token_indexer.cpp
    src/embedders/torch_embedder.cpp
    src/nasty.cpp
    src/rank.cpp
    src/run_server.cpp
    src/server_clustering.cpp
    src/thread_pool.cpp
    src/util.cpp
)

file(GLOB PROTO_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/proto/*.proto")

set(LIB_LIST
    ${Boost_LIBRARIES}
    ${Protobuf_LIBRARIES}
    ${TORCH_LIBRARIES}
    tinyxml2
    OpenNMTTokenizer
    fasttext-static
    eigen
    rocksdb
    drogon
)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/thirdparty")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/rocksdb/include")
target_include_directories(fasttext-static SYSTEM PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/fasttext/src")
target_include_directories(OpenNMTTokenizer SYSTEM PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/onmt_tokenizer/include")

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})
include_directories(${CMAKE_BINARY_DIR})

add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${PROTO_SRCS} src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE ${LIB_LIST})
target_compile_options(${PROJECT_NAME} PUBLIC "${TGNEWS_CXX_FLAGS}")
target_compile_options(${PROJECT_NAME} PUBLIC "$<$<CONFIG:Debug>:${TGNEWS_CXX_DEBUG_FLAGS}>")
target_link_libraries(${PROJECT_NAME} PUBLIC "$<$<CONFIG:Debug>:${TGNEWS_LNK_DEBUG_FLAGS}>")
target_compile_options(${PROJECT_NAME} PUBLIC "$<$<CONFIG:Release>:${TGNEWS_CXX_RELEASE_FLAGS}>")

enable_testing()

file(GLOB TEST_SRCS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} test/*.cpp)
add_definitions(-DTEST_PATH=${CMAKE_CURRENT_SOURCE_DIR}/test)
foreach(testSrc ${TEST_SRCS})
    get_filename_component(testName ${testSrc} NAME_WE)
    add_executable(${testName} ${SOURCE_FILES} ${PROTO_SRCS} ${testSrc})
    target_link_libraries(${testName} PRIVATE ${LIB_LIST})
    add_test(NAME ${testName} COMMAND ${testName})
endforeach(testSrc)
