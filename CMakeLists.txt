cmake_minimum_required(VERSION 3.10)
project(tide)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/utils.cmake)
tide_define_project_macros()

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function")

include_directories(/usr/local/include)
link_directories(/usr/local/lib)

set(LIB_SRC
    tide/log.cc
    tide/utils.cc
    tide/config.cc
    )

add_library(tide SHARED ${LIB_SRC})
target_link_libraries(tide yaml-cpp)


# 自动查找 tests/ 目录下所有 .cc 文件，分别生成同名可执行文件
file(GLOB TEST_SOURCES tests/*.cc)
foreach(test_src ${TEST_SOURCES})
    get_filename_component(test_name ${test_src} NAME_WE)
    add_executable(${test_name} ${test_src})
    add_dependencies(${test_name} tide)
    target_link_libraries(${test_name} tide)
endforeach()

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)