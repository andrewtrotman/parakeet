/*
	K_TREE.CPP
	----------
*/
#include "k_tree.h"

namespace k_tree
	{
	/*
		K_TREE::UNITTEST()
		------------------
	*/
	void k_tree::unittest(void)
		{
		allocator memory;
		k_tree tree(memory);

		std::cout << tree << "\n\n";

		for (size_t which = 0; which < MAX_NODE_WIDTH * 4; which++)
			{
			object &data = *object::new_object(memory);
			for (size_t dimension = 0; dimension < object::DIMENSIONS; dimension++)
				{
				if (which < (MAX_NODE_WIDTH / 2))
					data.vector[dimension] = (rand() % 20) / 10.0;
				else
					data.vector[dimension] = ((rand() % 20) + 70) / 10.0;
				}
std::cout << "-> " << data << "\n";
			tree.push_back(memory, data);
std::cout << tree << "\n";
			}

		std::cout << "TREE\n";
		std::cout << tree;
		}
	}
