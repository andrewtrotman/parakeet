/*
	K_TREE.H
	--------
	Copyright (c) 2020 Andrew Trotman
	Released under the 2-clause BSD license (See:https://en.wikipedia.org/wiki/BSD_licenses)
*/
#pragma once

#include <stdint.h>

#include <atomic>

#include "node.h"

namespace k_tree
	{
	class node;
	std::ostream &operator<<(std::ostream &stream, const class k_tree &thing);			// forward declare the output operator

	/*
		CLASS K_TREE
		------------
		K-tree.  See:
			S. Geva (2000) K-tree: a height balanced tree structured vector quantizer, Neural Networks for Signal Processing X.
			Proceedings of the 2000 IEEE Signal Processing Society Workshop, pp. 271-280, doi: 10.1109/NNSP.2000.889418.
		This is a b-tree of vectors where each node stored a k-means clustering of the nodes beneath it.
	*/
	class k_tree
		{
		public:
			std::atomic<node::context::in_out_count> split_count;		// the number of splits we've seen (counts starts and finishes of the split)

			node *parameters;				// the sole purpose of parameters is to store the order (branchine factor) of the tree and the width of the vectors it holds.
			std::atomic<node *>root;	// the root of the k-tree (must be std::atomic<> because it is written to and read from multiple threads without a barrier).

		private:
			/*
				K_TREE::ATTEMPT_PUSH_BACK()
				---------------------------
				Add to the tree
			*/
			node::result attempt_push_back(allocator *memory, object *data);

		public:
			/*
				K_TREE::K_TREE()
				----------------
				Constructor
			*/
			k_tree(allocator *memory, size_t tree_order, size_t vector_order) noexcept;

			/*
				K_TREE::PUSH_BACK()
				-------------------
				Add to the tree
			*/
			void push_back(allocator *memory, object *data);

			/*
				K_TREE::GET_EXAMPLE_OBJECT
				--------------------------
				Return an example vector
			*/
			object *get_example_object(void);

			/*
				K_TREE::NORMALISE_COUNTS()
				--------------------------
				Fix the broken leaved-below counts that happened because of parallel updates.
				This does a single-threaded sequential linear pass over the entre tree counting
				up the number of leaves at each node.
			*/
			void normalise_counts(void);

			/*
				K_TREE::DESERIALISE()
				---------------------
				Deserialise a previously serialised k-tree.  Serialised using cout << ktree;
			*/
			void deserialise(allocator &memory, std::istream &stream, object &example_object);

			/*
				K_TREE::TEXT_RENDER()
				---------------------
				Serialsise a human-readable version of tree to the stream
			*/
			std::ostream &text_render(std::ostream &stream) const;

			/*
				K_TREE::TEXT_RENDER_MOVIE()
				---------------------------
				Serialsise a human-readable version of tree to the stream
			*/
			std::ostream &text_render_movie(std::ostream &stream) const;

			/*
				K_TREE::TEXT_RENDER_PENULTIMATE()
				---------------------------------
				Dump the level above the leaves (the bottom-level clusters)
			*/
			std::ostream &text_render_penultimate(std::ostream &stream) const;

			/*
				K_TREE::TEXT_RENDER_PENULTIMATE_AND_BELOW()
				-------------------------------------------
				Dump the level above the leaves (the bottom-level clusters) and the leaves
			*/
			std::ostream &text_render_penultimate_and_below(std::ostream &stream) const;

			/*
				K_TREE::UNITTEST()
				------------------
				Test the class
			*/
			static void unittest(void);
		};

	/*
		OPERATOR<<()
		------------
	*/
	inline std::ostream &operator<<(std::ostream &stream, const k_tree &thing)
		{
		std::ios_base::fmtflags f(stream.flags());

		stream.precision(6);
		stream << std::fixed;
		thing.text_render(stream);

		stream.flags(f);
		return stream;
		}
	}
