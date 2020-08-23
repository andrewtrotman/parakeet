#
# CMAKELISTS.TXT
# --------------
#
set(SOURCE
	k_tree.h
	k_tree.cpp
	version.h
	)

add_library(k_tree_lib ${SOURCE})
include_directories(.)

source_group ("Source Files" FILES ${SOURCE})
