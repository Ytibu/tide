cmake_minimum_required(VERSION 3.10)
project(tide)

include(GNUInstallDirs)

option(TIDE_BUILD_TESTS "Build test executables under tests/" ON)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/utils.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/libraries.cmake)
tide_define_project_macros()
tide_setup_compile_flags()
tide_setup_libraries()

# 输出详细编译命令（构建日志可见）
set(CMAKE_VERBOSE_MAKEFILE ON)
# 导出 compile_commands.json（供 clangd/IDE/排查编译命令使用）
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 统一管理各类产物输出目录
set(TIDE_BIN_DIR ${PROJECT_SOURCE_DIR}/bin)
set(TIDE_TEST_BIN_DIR ${PROJECT_SOURCE_DIR}/test_bin)
set(TIDE_LIB_DIR ${PROJECT_SOURCE_DIR}/lib)

find_package(SQLite3 REQUIRED)

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
    tide/email/email.cc
    tide/email/smtp.cc
    tide/database/fox_thread.cc
    tide/database/sqlite.cc
    tide/database/redis.cc
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

    )


add_library(tide SHARED ${LIB_SRC})
target_link_libraries(tide PUBLIC ${SQLite3_LIBRARIES})
target_link_libraries(tide PUBLIC pthread)
target_link_libraries(tide PUBLIC yaml-cpp)
target_link_libraries(tide PUBLIC protobuf)
target_link_libraries(tide PUBLIC jsoncpp)
target_link_libraries(tide PUBLIC hiredis)
target_link_libraries(tide PUBLIC hiredis_vip)
target_link_libraries(tide PUBLIC event)

tide_link_libraries(tide)
# 动态库输出到 lib/
set_target_properties(tide PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${TIDE_LIB_DIR}
    ARCHIVE_OUTPUT_DIRECTORY ${TIDE_LIB_DIR}
)

# 主程序输出到 bin/
add_executable(tide_main tide/main.cc)
add_dependencies(tide_main tide)
tide_link_executable(tide_main)
set_target_properties(tide_main PROPERTIES
    OUTPUT_NAME tide
    RUNTIME_OUTPUT_DIRECTORY ${TIDE_BIN_DIR}
)

# 安装规则：库文件安装到 lib/，头文件安装到 include/tide/
install(TARGETS tide
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# 安装头文件，保持目录结构
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
    # 测试程序统一输出到 test_bin/
    set_target_properties(${test_name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${TIDE_TEST_BIN_DIR}
    )
endforeach()
endif()