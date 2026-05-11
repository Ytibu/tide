cmake_minimum_required(VERSION 3.10)
project(tide)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/utils.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/libraries.cmake)
tide_define_project_macros()
tide_setup_compile_flags()
tide_setup_libraries()

set(CMAKE_VERBOSE_MAKEFILE ON)

set(LIB_SRC
    tide/log.cc
    tide/utils.cc
    tide/config.cc
    tide/thread.cc
    tide/fiber.cc
    tide/scheduler.cc
    tide/iomanager.cc
    tide/timer.cc
    tide/hook.cc
    tide/fd_manager.cc
    tide/address.cc
    tide/socket.cc
    tide/bytearray.cc
    tide/http.cc
    tide/http11/http11_parser.cc
    tide/http11/httpclient_parser.cc
    tide/http_parser.cc
    )

add_library(tide SHARED ${LIB_SRC})
tide_link_libraries(tide)


# 自动查找 tests/ 目录下所有 .cc 文件，分别生成同名可执行文件
file(GLOB TEST_SOURCES tests/*.cc)
foreach(test_src ${TEST_SOURCES})
    get_filename_component(test_name ${test_src} NAME_WE)
    add_executable(${test_name} ${test_src})
    add_dependencies(${test_name} tide)
    tide_link_executable(${test_name})
endforeach()

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)