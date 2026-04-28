cmake_minimum_required(VERSION 3.10)
project(tide)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function")


set(LIB_SRC
    tide/log.cc
    tide/utils.cc
    )

add_library(tide SHARED ${LIB_SRC})

add_executable(test tests/test.cc)
add_dependencies(test tide)
target_link_libraries(test tide)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)