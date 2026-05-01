# Shared CMake helper macros for Tide project.

macro(tide_define_project_macros)
    # Rewrite __FILE__ absolute paths to project-relative paths at compile time.
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-fmacro-prefix-map=${PROJECT_SOURCE_DIR}/=)
    endif()
endmacro()
