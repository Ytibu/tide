cmake_minimum_required(VERSION 3.10)
project(tide VERSION 0.1.0 LANGUAGES CXX C)

include(GNUInstallDirs)

# ======================
# 项目选项
# ======================
option(TIDE_BUILD_TESTS "Build test executables" ON)
option(TIDE_BUILD_SHARED "Build shared library" ON)
option(TIDE_BUILD_STATIC "Build static library" OFF)

# ======================
# 全局构建设置
# ======================
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_VERBOSE_MAKEFILE OFF CACHE BOOL "Verbose build output")

# ======================
# 导入自定义 CMake 模块
# ======================
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(utils)
include(libraries)

tide_define_project_macros()

# ======================
# 依赖查找
# ======================
find_package(SQLite3 REQUIRED)
find_package(Threads REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(Protobuf REQUIRED)
find_package(jsoncpp REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(PkgConfig QUIET)

if(PkgConfig_FOUND)
    if(EXISTS "/usr/local/lib/pkgconfig")
        set(ENV{PKG_CONFIG_PATH} "/usr/local/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
    endif()
endif()

tide_find_pkg_library(HIREDIS hiredis hiredis/hiredis.h hiredis)
tide_find_pkg_library(HIREDIS_VIP hiredis_vip hiredis-vip/hiredis.h hiredis_vip)
tide_find_pkg_library(EVENT libevent event2/event.h event libevent)

# ======================
# 源文件列表（按模块分组）
# ======================
set(TIDE_CORE_SRC
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
)

set(TIDE_HTTP_SRC
    tide/http/http11/http11_parser.c
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
)

set(TIDE_UTIL_SRC
    tide/util/hash_util.cc
    tide/util/json_util.cc
)

set(TIDE_EMAIL_SRC
    tide/email/email.cc
    tide/email/smtp.cc
)

set(TIDE_DATABASE_SRC
    tide/database/fox_thread.cc
    tide/database/sqlite.cc
    tide/database/redis.cc
)

set(TIDE_ALL_SRC
    ${TIDE_CORE_SRC}
    ${TIDE_HTTP_SRC}
    ${TIDE_UTIL_SRC}
    ${TIDE_EMAIL_SRC}
    ${TIDE_DATABASE_SRC}
)

set(TIDE_THIRD_PARTY_LIBS
    SQLite3::SQLite3
    Threads::Threads
    yaml-cpp::yaml-cpp
    protobuf::libprotobuf
    jsoncpp_lib
    OpenSSL::SSL
    OpenSSL::Crypto
)

# ======================
# 目标设置
# ======================
function(tide_apply_library_settings TARGET_NAME)
    tide_setup_compile_flags(${TARGET_NAME})

    target_include_directories(${TARGET_NAME} PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    )

    target_link_libraries(${TARGET_NAME} PUBLIC ${TIDE_THIRD_PARTY_LIBS})
    tide_link_pkg_library(${TARGET_NAME} HIREDIS)
    tide_link_pkg_library(${TARGET_NAME} HIREDIS_VIP)
    tide_link_pkg_library(${TARGET_NAME} EVENT)
endfunction()

function(tide_apply_executable_settings TARGET_NAME)
    tide_setup_compile_flags(${TARGET_NAME})
    tide_setup_link_flags(${TARGET_NAME})
endfunction()

# ======================
# 库构建（支持共享/静态）
# ======================
if(TIDE_BUILD_SHARED)
    add_library(tide SHARED ${TIDE_ALL_SRC})
    tide_apply_library_settings(tide)
    set(TIDE_LIBRARY_TARGET tide)
endif()

if(TIDE_BUILD_STATIC)
    add_library(tide_static STATIC ${TIDE_ALL_SRC})
    set_target_properties(tide_static PROPERTIES OUTPUT_NAME tide)
    tide_apply_library_settings(tide_static)
    if(NOT TIDE_LIBRARY_TARGET)
        set(TIDE_LIBRARY_TARGET tide_static)
    endif()
endif()

# ======================
# 主可执行文件
# ======================
if(TIDE_LIBRARY_TARGET)
    add_executable(tide_main tide/main.cc)
    target_link_libraries(tide_main PRIVATE ${TIDE_LIBRARY_TARGET})
    tide_apply_executable_settings(tide_main)

    set_target_properties(tide_main PROPERTIES
        OUTPUT_NAME tide
        RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin
    )
endif()

# ======================
# 测试（分别编译每个文件）
# ======================
if(TIDE_BUILD_TESTS AND TIDE_LIBRARY_TARGET)
    enable_testing()

    file(GLOB TEST_FILES tests/*.cc)
    foreach(test_file ${TEST_FILES})
        get_filename_component(test_name ${test_file} NAME_WE)
        add_executable(${test_name} ${test_file})
        target_link_libraries(${test_name} PRIVATE ${TIDE_LIBRARY_TARGET})
        tide_apply_executable_settings(${test_name})
        set_target_properties(${test_name} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin
        )

        add_test(NAME ${test_name} COMMAND ${test_name})
    endforeach()
endif()

# ======================
# 安装规则
# ======================
set(TIDE_HAS_EXPORT_TARGETS OFF)

if(TARGET tide)
    install(TARGETS tide
        EXPORT tideTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
    set(TIDE_HAS_EXPORT_TARGETS ON)
endif()

if(TARGET tide_static)
    install(TARGETS tide_static
        EXPORT tideTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
    set(TIDE_HAS_EXPORT_TARGETS ON)
endif()

install(DIRECTORY tide/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/tide
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "*.rh"
    PATTERN "*.rl" EXCLUDE
)

if(TIDE_HAS_EXPORT_TARGETS)
    install(EXPORT tideTargets
        FILE tideTargets.cmake
        NAMESPACE tide::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/tide
    )
endif()

# ======================
# 打印配置摘要
# ======================
message(STATUS "Tide Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build shared: ${TIDE_BUILD_SHARED}")
message(STATUS "  Build static: ${TIDE_BUILD_STATIC}")
message(STATUS "  Build tests: ${TIDE_BUILD_TESTS}")
message(STATUS "  Install prefix: ${CMAKE_INSTALL_PREFIX}")