# Tide project helper functions that are useful but not core to the build graph.

include_guard(GLOBAL)

macro(tide_find_pkg_library PREFIX PKG_NAME HEADER_NAME LIB_NAME)
    if(PkgConfig_FOUND)
        pkg_check_modules(${PREFIX} QUIET ${PKG_NAME})
    endif()

    if(NOT ${PREFIX}_FOUND)
        find_path(${PREFIX}_INCLUDE_DIRS NAMES ${HEADER_NAME}
            PATHS /usr/local/include /usr/include)
        find_library(${PREFIX}_LIBRARIES NAMES ${LIB_NAME}
            PATHS /usr/local/lib /usr/lib /usr/lib64)
        if(${PREFIX}_INCLUDE_DIRS AND ${PREFIX}_LIBRARIES)
            set(${PREFIX}_FOUND TRUE)
        endif()
    endif()

    if(NOT ${PREFIX}_FOUND)
        message(FATAL_ERROR "${PKG_NAME} not found. Please install ${PKG_NAME} or provide PKG_CONFIG_PATH.")
    endif()
endmacro()

macro(tide_link_pkg_library TARGET_NAME PREFIX)
    if(${PREFIX}_FOUND)
        target_link_libraries(${TARGET_NAME} PUBLIC ${${PREFIX}_LIBRARIES})
        if(${PREFIX}_INCLUDE_DIRS)
            target_include_directories(${TARGET_NAME} PUBLIC ${${PREFIX}_INCLUDE_DIRS})
        endif()
    endif()
endmacro()

function(tide_setup_compile_flags TARGET_NAME)
    target_compile_features(${TARGET_NAME} PUBLIC cxx_std_11)
    target_compile_options(${TARGET_NAME} PRIVATE
        -g
        -O0
        -Wall
        -Wno-deprecated
        -Werror
        -Wno-unused-function
    )
endfunction()

function(tide_setup_link_flags TARGET_NAME)
    if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.13)
        target_link_options(${TARGET_NAME} PRIVATE -rdynamic)
    else()
        set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS "-rdynamic")
    endif()
endfunction()
