#
# CMAKELISTS.TXT
# --------------
#
set(SOURCE
	k_means.h
	k_means.cpp
	k_tree.h
	k_tree.cpp
	object.h
	version.h
	)

add_library(k_tree_lib ${SOURCE})
include_directories(.)

source_group ("Source Files" FILES ${SOURCE})
