cmake_minimum_required(VERSION 3.10)

project(LoReHyPar LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    # MSVC
    set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} /Od /W4 /Zi /DDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} /O2 /W4")
else()
    # GCC/Clang
    set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -Wextra -ggdb3 -DDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O2 -Wall -Wextra")
endif()

# add_subdirectory(include/boost)
add_library(${PROJECT_NAME} hypar.cpp funcs.cpp)

target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR})

add_executable(hypar_shell main.cpp)
target_link_libraries(hypar_shell ${PROJECT_NAME})