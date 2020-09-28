#
# CMAKELISTS.TXT
# --------------

project(k_tree)

#
# CMake verison 3.2 is required to set CMAKE_CXX_STANDARD to C++17.
#

cmake_minimum_required (VERSION 3.2)

# set(CMAKE_VERBOSE_MAKEFILE on)

message("Build Type:" ${CMAKE_BUILD_TYPE})

#
# Set the warning flags based on the compiler
#
if(WIN32)
 	add_definitions("/W3 /nologo")
else()
	add_definitions("-Wall -march=native")
	if (LINUX)
		set(ATOMIC atomic)
	endif()
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads)

#
# Build the library
#
add_subdirectory(source)

#
# Build the documentation examples
#
add_subdirectory(example)
