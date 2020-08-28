/*
	K_TREE.CPP
	----------
	Copyright (c) 2020 Andrew Trotman
	Released under the 2-clause BSD license (See:https://en.wikipedia.org/wiki/BSD_licenses)
*/
#include "k_tree.h"

namespace k_tree
	{
	/*
		K_TREE::K_TREE()
		----------------
	*/
	k_tree::k_tree(allocator &memory) :
		root(nullptr),
		memory(memory)
		{
		/* Nothing */
		}

	/*
		K_TREE::PUSH_BACK()
		-------------------
	*/
	void k_tree::push_back(k_tree *of, object *data)
		{
		bool did_split = false;
		node *child_1;
		node *child_2;

		/*
			Add to the tree
		*/
		if (root == nullptr)
			{
			/*
				The very first add to the tree so create a node with one child
			*/
			node *leaf = node::new_node(of->memory, data);
			root = node::new_node(of->memory, leaf);
			root->compute_mean();
			}
		else
			did_split = root->add_to_node(of->memory, data, &child_1, &child_2);

		/*
			Adding caused a split at the top level so we create a new root consisting of the two children
		*/
		if (did_split)
			{
			node *new_root = node::new_node(of->memory, child_1);
			new_root->child[1] = child_2;
			new_root->children = 2;
			root = new_root;
			child_1->compute_mean();
			child_2->compute_mean();
			root->compute_mean();
			}
		}

	/*
		K_TREE::PUSH_BACK()
		-------------------
	*/
	void k_tree::push_back(object *data)
		{
		push_back(this, data);
		}

	/*
		K_TREE::TEXT_RENDER()
		---------------------
	*/
	void k_tree::text_render(std::ostream &stream) const
		{
		if (root != nullptr)
			root->text_render(stream, 0);
		}

	/*
		K_TREE::UNITTEST()
		------------------
		Test the k-tree code
	*/
	void k_tree::unittest(void)
		{
		allocator memory;
		k_tree tree(memory);
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
