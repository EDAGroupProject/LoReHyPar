cmake_minimum_required(VERSION 3.15)

project(LoReHyPar)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -Wextra -ggdb3 -DZDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O2 -Wall -Wextra")

add_executable(hypar_shell main.cpp)

include_directories(${PROJECT_SOURCE_DIR})

add_library(${PROJECT_NAME} hypar.cpp)

target_link_libraries(hypar_shell ${PROJECT_NAME})