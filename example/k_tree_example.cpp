/*
	K_TREE_EXAMPLE.CPP
	------------------
	Copyright (c) 2020 Andrew Trotman
	Released under the 2-clause BSD license (See:https://en.wikipedia.org/wiki/BSD_licenses)
*/
#include <iostream>

#include "k_tree.h"

/*
	BUILD()
	-------
	Build the k-tree from the input data
*/
int build(char *infilename, size_t tree_order)
	{
	return 0;
	}

/*
	UNITTEST()
	----------
	Unit test each of the classes we use
*/
int unittest(void)
	{
	k_tree::object::unittest();
	k_tree::k_tree::unittest();

	return 0;
	}

/*
	USAGE()
	-------
	Shlomo's original program took:
		Usage: ktree  ['build'|'load']  in_file  tree_order  out_file
	Where
		build | load is to build the k-tree or load one from disk (currently IGNORED)
		in_file is the ascii file of vectors, one pre line and human readable in ASCII
		tree_order is the branching factor of the k-tree
		out_file (IGNORED)
*/
int usage(char *exename)
	{
	std::cout << "Usage:" << exename << " <[build | unittest]> <in_file> <tree_order> IGNORED\n";
	return 0;
	}

/*
	MAIN()
	------
*/
int main(int argc, char *argv[])
	{
	if (argc != 4)
		if (argc == 2)
			return unittest();
		else
			return usage(argv[0]);
	else if (strcmp(argv[1], "unittest") == 0)
		return unittest();
	else if (strcmp(argv[1], "build") == 0)
		return build(argv[2], atoi(argv[3]));
	else
		return usage(argv[0]);
	}
