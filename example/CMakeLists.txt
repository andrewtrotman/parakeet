#
# CMAKELISTS.TXT
# --------------
#

include_directories(../source)

add_executable(k_tree_example k_tree_example.cpp)
target_link_libraries(k_tree_example k_tree_lib ${CMAKE_THREAD_LIBS_INIT})

