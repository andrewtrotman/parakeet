/*
	K_TREE.CPP
	----------
	Copyright (c) 2020 Andrew Trotman
	Released under the 2-clause BSD license (See:https://en.wikipedia.org/wiki/BSD_licenses)
*/
#include "k_tree.h"

namespace k_tree
	{
	void k_tree::unittest(void)
		{
		k_tree tree;
		allocator memory;
		size_t total_adds = max_children * 4;

		for (size_t which = 0; which < total_adds; which++)
			{
			object &data = *object::new_object(memory);
			for (size_t dimension = 0; dimension < object::DIMENSIONS; dimension++)
				if (which < (max_children / 2))
					data.vector[dimension] = (rand() % 20) / 10.0;
				else
					data.vector[dimension] = ((rand() % 20) + 70) / 10.0;

std::cout << "-----------> " << data << "\n";
			tree.push_back(&data);
std::cout << tree << "\n";
			}

std::cout << "TREE (" << total_adds << " adds)\n";
std::cout << tree;
		}
	}
