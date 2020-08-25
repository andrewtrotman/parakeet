/*
	K_TREE.CPP
	----------
*/
#pragma once

#include "k_tree.h"

namespace k_tree
	{
	/*
		K_TREE::UNITTEST()
		------------------
	*/
	void k_tree::unittest(void)
		{
		k_tree tree;

		std::cout << tree << "\n\n";

		for (size_t which = 0; which < MAX_NODE_WIDTH * 4; which++)
			{
			object &data = *object::new_object();
			for (size_t dimension = 0; dimension < object::DIMENSIONS; dimension++)
				{
				if (which < (MAX_NODE_WIDTH / 2))
					data.vector[dimension] = (rand() % 20) / 10.0;
				else
					data.vector[dimension] = ((rand() % 20) + 70) / 10.0;
				}
			tree.push_back(data);
std::cout << tree << "\n";
			}

		std::cout << "TREE\n";
		std::cout << tree;
		}
	}
