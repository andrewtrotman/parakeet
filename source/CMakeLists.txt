#
# CMAKELISTS.TXT
# --------------
#
set(SOURCE
	allocator.h
	fast_double_parser.h
	k_tree.h
	k_tree.cpp
	node.h
	node.cpp
	object.h
	)

add_library(k_tree_lib ${SOURCE})
include_directories(.)

source_group ("Source Files" FILES ${SOURCE})
