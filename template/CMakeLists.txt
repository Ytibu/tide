# ==============================================================================
# 1. 基础项目配置
# ==============================================================================
cmake_minimum_required(VERSION 3.10)          # 要求 CMake 最低版本
project(project_name)                         # 项目名称

# 开启详细编译模式（终端会显示完整的 g++ 命令，方便调试）
set(CMAKE_VERBOSE_MAKEFILE ON)

# ==============================================================================
# 2. C++ 标准设置
# ==============================================================================
set(CMAKE_CXX_STANDARD 11)                    # 使用 C++11 标准
set(CMAKE_CXX_STANDARD_REQUIRED ON)           # 强制要求编译器支持 C++11，否则报错
set(CMAKE_CXX_EXTENSIONS OFF)                 # 关闭 GNU 扩展，只使用标准 C++

# ==============================================================================
# 3. 编译选项（调试模式）
# ==============================================================================
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -ggdb -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined")
# 各参数说明：
#   -rdynamic    : 让动态库能输出完整的符号表，方便反向追踪
#   -O0          : 关闭优化，方便调试
#   -ggdb        : 生成适用于 GDB 的调试信息
#   -Wall        : 开启大部分警告
#   -Wno-deprecated  : 忽略「使用了过时特性」的警告
#   -Werror      : 把所有警告当作错误处理，有警告就编译失败
#   -Wno-unused-function : 忽略「定义了但未使用函数」的警告
#   -Wno-builtin-macro-redefined : 忽略内建宏重定义警告

# ==============================================================================
# 4. 查找依赖库：yaml-cpp 和 jsoncpp（通过 pkg-config）
# ==============================================================================
find_package(PkgConfig REQUIRED)              # 先加载 pkg-config 工具
pkg_check_modules(YAML_CPP REQUIRED IMPORTED_TARGET yaml-cpp)   # 查找 yaml-cpp 库
pkg_check_modules(JSONCPP REQUIRED IMPORTED_TARGET jsoncpp)     # 查找 jsoncpp 库
# 后续可直接用 PkgConfig::YAML_CPP / PkgConfig::JSONCPP 来引用这些库

# ==============================================================================
# 5. 查找依赖库：Boost
# ==============================================================================
# find_package(Boost REQUIRED)                  # 查找 Boost 库（必须安装）
# include_directories(${Boost_INCLUDE_DIRS})   # 把头文件目录添加到编译搜索路径


# ==============================================================================
# 6. 查找依赖库：OpenSSL
# ==============================================================================
find_package(OpenSSL REQUIRED)                # 查找 OpenSSL 库（必须安装）
if(OPENSSL_FOUND)                             # 如果找到了
    include_directories(${OPENSSL_INCLUDE_DIR}) # 把头文件目录添加到编译搜索路径
endif()

# ==============================================================================
# 7. 查找依赖库：tide（自定义/系统安装的库）
# ==============================================================================
# 查找头文件路径（默认下找 tide/tide.h）
find_path(TIDE_INCLUDE_DIR
    NAMES tide/tide.h
    REQUIRED                                   # 找不到就停止构建
)

# 查找库文件路径（默认目录下找 libtide.so）
find_library(TIDE_LIBRARY
    NAMES tide
    REQUIRED                                   # 找不到就停止构建
)

# ==============================================================================
# 8. 构建目标：编译一个共享库 (.so)
# ==============================================================================
# 指定源码文件存放目录（相对于当前 CMakeLists.txt 的 template 子目录）
set(MODULE_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/template)

# 显式列出需要编译的源文件
set(LIB_SRC
    ${MODULE_SRC_DIR}/my_module.cc
)

# 创建共享库目标：生成 libproject_name.so
add_library(project_name SHARED ${LIB_SRC})

# 添加头文件搜索路径（仅本目标使用，不传递给依赖者）
target_include_directories(project_name
    PRIVATE
        ${TIDE_INCLUDE_DIR}
)

# 链接依赖库
target_link_libraries(project_name
    PUBLIC                                    # 链接的库对本目标及依赖本目标的代码都可见
        PkgConfig::YAML_CPP
        PkgConfig::JSONCPP
        ${OPENSSL_LIBRARIES}
        ${TIDE_LIBRARY}
)

# ==============================================================================
# 9. 输出路径设置
# ==============================================================================
# 生成的 .so 文件放到源码根目录下的 lib/ 目录
set_target_properties(project_name PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib
)

# 同时设置可执行文件（如果有的话）的输出路径为 bin/
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)