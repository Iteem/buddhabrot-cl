cmake_minimum_required( VERSION 3.12 )
project( buddhabrot_cl )

set( CMAKE_CXX_STANDARD 14 )

# extlibs
find_package( SFML 2.5 COMPONENTS graphics REQUIRED)
find_package( OpenCL REQUIRED )
include_directories( ${OpenCL_INCLUDE_DIRS} )
link_directories( ${OpenCL_LIBRARY} )


add_executable( buddhabrot_cl main.cpp )
target_link_libraries( buddhabrot_cl sfml-graphics ${OpenCL_LIBRARY} )

# Copy data directory to uild cache directory to be able to run examples from
# there. Useful for testing stuff.
add_custom_target(
    export_files
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/kernels" "${PROJECT_BINARY_DIR}/kernels"
)
