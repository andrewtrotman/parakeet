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
		Constructor
	*/

	k_tree::k_tree(allocator *memory, size_t tree_order, size_t vector_order) :
		split_count(0),
		parameters(nullptr),
		root(nullptr),
		memory(memory)
		{
		parameters = new (memory->malloc(sizeof(*parameters))) node();
		parameters->max_children = tree_order;
		parameters->centroid = new (memory->malloc(sizeof(*parameters->centroid))) object();
		parameters->centroid->dimensions = vector_order;
		}

	/*
		K_TREE::PUSH_BACK()
		-------------------
		Add to the tree
	*/
	void k_tree::push_back(allocator *memory, object *data)
		{
		node::context context(this, memory, split_count);
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
			node *leaf = parameters->new_node(memory, data);
			root = parameters->new_node(memory, leaf);
			root->compute_mean();
			}
		else
			did_split = root->add_to_node(&context, data, &child_1, &child_2);

		/*
			Adding caused a split at the top level so we create a new root consisting of the two children
		*/
		if (did_split)
			{
			node *new_root = parameters->new_node(memory, child_1);
			new_root->child[1] = child_2;
			new_root->children = 2;
			root = new_root;
			child_1->compute_mean();
			child_2->compute_mean();
			root->compute_mean();
			}
		}

	/*
		K_TREE::GET_EXAMPLE_OBJECT
		--------------------------
		Return an example vector
	*/
	object *k_tree::get_example_object(void)
		{
		return parameters->centroid;
		}

	/*
		K_TREE::TEXT_RENDER()
		---------------------
		Serialsise a human-readable version of tree to the stream
	*/
	void k_tree::text_render(std::ostream &stream) const
		{
		if (root != nullptr)
			root->text_render(stream);
		}

	/*
		K_TREE::UNITTEST()
		------------------
		Test the class
	*/
	void k_tree::unittest(void)
		{
		constexpr size_t dimensions = 2;
		object initial(dimensions);
		allocator memory;
		k_tree tree(&memory, 4, dimensions);
		size_t total_adds = 16;

		for (size_t which = 0; which < total_adds; which++)
			{
			object &data = *initial.new_object(&memory);
			for (size_t dimension = 0; dimension < initial.dimensions; dimension++)
				if (which < 8)
					data.vector[dimension] = (rand() % 20) / 10.0;
				else
					data.vector[dimension] = ((rand() % 20) + 70) / 10.0;

std::cout << "-----------> " << data << "\n";
			tree.push_back(&memory, &data);
std::cout << tree << "\n";
			}

std::cout << "TREE (" << total_adds << " adds)\n";
std::cout << tree;
		}
	}
