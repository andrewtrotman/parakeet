/*
	K_TREE_EXAMPLE.CPP
*/
#include <iostream>

#include "version.h"
#include "k_tree.h"

int main(int argc, char *argv[])
	{
	std::cout << k_tree::version_string << "\n";
	k_tree::k_tree::unittest();

	return 0;
	}
