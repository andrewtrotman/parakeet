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

	k_tree::k_tree(allocator *memory, size_t tree_order, size_t vector_order) noexcept :
		split_count(),
		parameters(nullptr),
		root(nullptr)
		{
		parameters = new (memory->malloc(sizeof(*parameters))) node();
		parameters->max_children = tree_order;
		parameters->centroid = new (memory->malloc(sizeof(*parameters->centroid))) object();
		parameters->centroid->dimensions = vector_order;
		}

	/*
		K_TREE::ATTEMPT_PUSH_BACK()
		---------------------------
		Add to the tree
	*/
	node::result k_tree::attempt_push_back(allocator *memory, object *data)
		{
		node::context context(this, memory, split_count);
		node::result did_split = node::result_success;
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
			if (!node::take_lock(&context))
				return node::result_retry;

			node *leaf = parameters->new_node(memory, data);
			node *new_root = parameters->new_node(memory, leaf);
			new_root->compute_mean();

			/*
				Force the writes to happen then update root
			*/
			std::atomic_thread_fence(std::memory_order_seq_cst);
			root = new_root;

			node::release_lock(&context);
			}
		else
			did_split = root.load()->add_to_node(&context, data, &child_1, &child_2);

		/*
			Adding caused a split at the top level so we create a new root consisting of the two children.  If this happens then
			the tree is also already locked.
		*/
		if (did_split == node::result_split)
			{
			child_1->compute_mean();
			child_2->compute_mean();

			node *new_root = parameters->new_node(memory, child_1);
			new_root->child[1] = child_2;
			new_root->children = 2;
			new_root->compute_mean();

			/*
				Force the writes to happen then update root
			*/
			std::atomic_thread_fence(std::memory_order_seq_cst);
			root = new_root;

			/*
				The tree is locked, so we unlock it
			*/
			node::release_lock(&context);

			did_split = node::result_success;	// the node above (there isn't one) doesn't need to split
			}
		return did_split;
		}

	/*
		K_TREE::PUSH_BACK()
		-------------------
		Add to the tree
	*/
	void k_tree::push_back(allocator *memory, object *data)
		{
		/*
			A write to the tree can result in success, retry, or split.  Split is taken care of
			in attempt_push_back() so the only outcomes we can get are retry and success.  We
			loop here, retrying each time, until we get success.
		*/
		node::result got;
		do
			got = attempt_push_back(memory, data);
		while (got != node::result_success);
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
		K_TREE::NORMALISE_COUNTS()
		--------------------------
		Fix the broken node counts that happened because of parallel updates
	*/
	void k_tree::normalise_counts(void)
		{
		root.load()->normalise_counts();
		}

	/*
		K_TREE::DESERIALISE()
		-------------------
	*/
	void k_tree::deserialise(allocator &memory, std::istream &stream, object &example_object)
		{
		/*
			Create a fake root and then use that to deserialse the tree
		*/
		node *fake_root = parameters->new_node(&memory, parameters->new_node(&memory,&example_object));
		root = fake_root->deserialise(memory, stream, example_object);
		}

	/*
		K_TREE::TEXT_RENDER()
		---------------------
		Serialsise a human-readable version of tree to the stream
	*/
	std::ostream &k_tree::text_render(std::ostream &stream) const
		{
		if (root != nullptr)
			root.load()->text_render(stream);
		return stream;
		}

	/*
		K_TREE::TEXT_RENDER_MOVIE()
		---------------------------
		Serialsise a human-readable version of tree to the stream
	*/
	std::ostream &k_tree::text_render_movie(std::ostream &stream) const
		{
		if (root != nullptr)
			root.load()->text_render_movie(stream);
		return stream;
		}

	/*
		K_TREE::TEXT_RENDER_PENULTIMATE()
		---------------------------------
		Dump the level above the leaves (the bottom-level clusters)
	*/
	std::ostream &k_tree::text_render_penultimate(std::ostream &stream) const
		{
		if (root != nullptr)
			root.load()->text_render_penultimate(stream);
		return stream;
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
