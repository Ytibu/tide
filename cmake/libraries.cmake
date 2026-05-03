# Tide project libraries and linking configuration

macro(tide_setup_libraries)
    # Include directories for third-party libs
    include_directories(/usr/local/include)
    link_directories(/usr/local/lib)
    
    # Define all required libraries
    set(TIDE_LIBRARIES
        yaml-cpp
        pthread
    )
endmacro()

macro(tide_link_libraries target)
    # Link third-party libraries to a target
    target_link_libraries(${target} ${TIDE_LIBRARIES})
endmacro()

macro(tide_link_executable target)
    # Link both tide library and all third-party libraries to a test executable
    target_link_libraries(${target} tide ${TIDE_LIBRARIES})
endmacro()

function(tide_setup_compile_flags)
    # Set C++ compilation flags
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -rdynamic -O3 -std=c++11 -pthread -Wall -Wno-deprecated -Werror -Wno-unused-function"
        PARENT_SCOPE)
endfunction()
