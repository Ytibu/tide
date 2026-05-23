cmake_minimum_required(VERSION 3.10)
project(tide)

include(GNUInstallDirs)

option(TIDE_BUILD_TESTS "Build test executables under tests/" ON)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/utils.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/libraries.cmake)
tide_define_project_macros()
tide_setup_compile_flags()
tide_setup_libraries()

set(CMAKE_VERBOSE_MAKEFILE ON)

set(LIB_SRC
    tide/scheduler.cc
    tide/worker.cc
    tide/socket_stream.cc
    tide/utils.cc
    tide/config.cc
    tide/mutex.cc
    tide/timer.cc
    tide/address.cc
    tide/library.cc
    tide/log.cc
	tide/http/http11/http11_parser.c
    tide/http/http11/http11_parser.rl
    tide/http/http11/httpclient_parser.c
	tide/http/http.cc
    tide/http/http_parser.cc
    tide/http/http_server.cc
    tide/http/http_connection.cc
    tide/http/http_session.cc
	tide/http/servlet.cc
    tide/http/ws_connection.cc
    tide/http/ws_server.cc
    tide/http/ws_session.cc
    tide/http/ws_servlet.cc
    tide/util/hash_util.cc
    tide/util/json_util.cc
    tide/thread.cc
    tide/socket.cc
    tide/fiber.cc
    tide/application.cc
    tide/daemon.cc
    tide/uri.cc
    tide/env.cc
    tide/iomanager.cc
    tide/stream.cc
    tide/fd_manager.cc
    tide/tcp_server.cc
    tide/hook.cc
    tide/module.cc
    tide/bytearray.cc
    tide/library.cc
    # tide/my_module.cc
    )

add_library(tide SHARED ${LIB_SRC})
tide_link_libraries(tide)

install(TARGETS tide
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY tide/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/tide
    FILES_MATCHING PATTERN "*.h"
)


if(TIDE_BUILD_TESTS)
# 自动查找 tests/ 目录下所有 .cc 文件，分别生成同名可执行文件
file(GLOB TEST_SOURCES tests/*.cc)
foreach(test_src ${TEST_SOURCES})
    get_filename_component(test_name ${test_src} NAME_WE)
    add_executable(${test_name} ${test_src})
    add_dependencies(${test_name} tide)
    tide_link_executable(${test_name})
endforeach()
endif()

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)