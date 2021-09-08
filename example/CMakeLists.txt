#
# CMAKELISTS.TXT
# --------------
#

include_directories(../source)

add_executable(k_tree_example k_tree_example.cpp disk.cpp disk.h)
target_link_libraries(k_tree_example k_tree_lib ${CMAKE_THREAD_LIBS_INIT} ${ATOMIC})

add_executable(knn_from_tree knn_from_tree.cpp disk.cpp disk.h cluster.h)
target_link_libraries(knn_from_tree k_tree_lib ${CMAKE_THREAD_LIBS_INIT} ${ATOMIC})

add_executable(generate_points generate_points.cpp)
target_link_libraries(generate_points)
