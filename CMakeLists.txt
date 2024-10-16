cmake_minimum_required(VERSION 3.10)

project(LoReHyPar LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} /Od /W4 /Zi /DDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} /O2 /W4")
else()
    set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -Wextra -ggdb3 -DDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O2 -Wall -Wextra")
endif()

add_library(${PROJECT_NAME} STATIC hypar.cpp preprocess.cpp coarsen.cpp initialpar.cpp refine.cpp tests.cpp)
include_directories(PUBLIC "${PROJECT_SOURCE_DIR}")
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR})

add_executable(partitioner main.cpp)
target_link_libraries(partitioner ${PROJECT_NAME})