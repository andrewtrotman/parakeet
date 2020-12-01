/*
	NODE.H
	------
	Copyright (c) 2020 Andrew Trotman
	Released under the 2-clause BSD license (See:https://en.wikipedia.org/wiki/BSD_licenses)
*/
#pragma once

#include <stdint.h>

#include "object.h"
#include "allocator.h"

namespace k_tree
	{
	/*
		CLASS NODE
		----------
		A node (and a leaf) in the k-tree
	*/
	class node
		{
		friend class k_tree;

		private:
			static constexpr float float_resolution = (float)0.000001;														// floats his close are considered equal

		public:
			size_t max_children;					//	the order of the tree at this node (constant per tree as it propegates when a new node is created)
			size_t children;						// the number of children of this node
			node **child;							// the immediate descendants of this node
			object *centroid;						// the centroid of this cluster
			size_t leaves_below_this_point;	// the number of leaves below this node

		private:
			/*
				NODE::NODE()
				------------
			*/
			node();

		public:
			/*
				NODE::NEW_NODE()
				----------------
			*/
			node *new_node(allocator *memory, object *data) const;

			/*
				NODE::NEW_NODE()
				----------------
			*/
			node *new_node(allocator *memory, node *first_child) const;

			/*
				NODE::ISLEAF()
				--------------
				Return whether or not this node is a leaf
			*/
			bool isleaf(void) const;

			/*
				NODE::CLOSEST()
				---------------
				Returns the index of the child closest to the parameter what
			*/
			size_t closest(object *what) const;

			/*
				NODE::COMPUTE_MEAN()
				--------------------
				Compute the centroid of the children below this node.  Note that this must be a weighted average as not all branched have the
				same number of children and this resulting centroid should be the middle of the leaves not the middle of the children.  This also
				recomputes that count from the children (which might have recently changed)
			*/
			void compute_mean(void);

			/*
				NODE::SPLIT()
				-------------
				Split this node into two new children
			*/
			void split(allocator *memory, node **child_1_out, node **child_2_out) const;

			/*
				NODE::ADD_TO_LEAF()
				-------------------
				Add the given data to the current leaf node.
				Returns whether or not there was a split (and so the node above must do a replacement and an add)
			*/
			bool add_to_leaf(allocator *memory, object *data, node **child_1, node **child_2);

			/*
				NODE::ADD_TO_NODE()
				-------------------
				Add the given data to the current tree at or below this point.
				Returns whether or not there was a split (and so the node above must do a replacement and an add)
			*/
			bool add_to_node(allocator *memory, object *data, node **child_1, node **child_2);

			/*
				NODE::TEXT_RENDER()
				-------------------
				Serialise the object in a human-readable format and down the given stream
			*/
			void text_render(std::ostream &stream) const;
		};
	}
