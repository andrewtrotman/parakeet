#
# CMAKELISTS.TXT
# --------------

project(K-TREE)

#
#CMake verison 3.2 is required to set CMAKE_CXX_STANDARD to C++14.
#

cmake_minimum_required (VERSION 3.2)

# set(CMAKE_VERBOSE_MAKEFILE on)

message("Build Type:" ${CMAKE_BUILD_TYPE})

#
#	Set the debug flags based on the compiler
#	_GLIBCXX_USE_CXX11_ABI=1 is necessary for g++ to allow C++ stateful custom allocators in std::string
#
if(WIN32)
 	add_definitions("/W3 /nologo")
else()
	add_definitions("-Wall")
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
