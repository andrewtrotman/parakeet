/*
	NODE.H
	------
	Copyright (c) 2020 Andrew Trotman
	Released under the 2-clause BSD license (See:https://en.wikipedia.org/wiki/BSD_licenses)
*/
#pragma once

#include <stdint.h>

#include <atomic>

#include "object.h"
#include "allocator.h"

namespace k_tree
	{
	class k_tree;
	/*
		CLASS NODE
		----------
		A node (and a leaf) in the k-tree
	*/
	class node
		{
		friend class k_tree;

		public:
			/*
				ENUM STATUS
				-----------
				On addition to a node or leaf we might get success, a split is necessary, or we have to retry because a different thread wants to split this node
			*/
			typedef enum
				{
				result_success,		// all good
				result_retry,			// someone else is splitting this node so exit and retry
				result_split			// a split is necessary
				} result;

			/*
				ENUM SPLIT_STATE
				----------------
			*/
			typedef enum
				{
				state_unsplit,			// this node has never been split
				state_split				// this has been split or is being split (and so the backpath is invalid)
				} split_state;

			/*
				CLASS NODE::CONTEXT
				-------------------
				The current context of a node in the tree (which tree, which allocator, etc)
			*/
			class context
				{
				public:
					/*
						CLASS NODE::CONTEXT::IN_OUT_COUNT
						---------------------------------
						Stores the number of splits that have happened.  If begin != end then a split is currently happening
					*/
					class in_out_count
						{
						public:
							uint64_t begin;				// number of splits that have been started
							uint64_t end;					// number of splits that have completed
						public:
							/*
								NODE::CONTEXT::IN_OUT_COUNT::IN_OUT_COUNT()
								-------------------------------------------
							*/
							in_out_count() noexcept :
								begin(0),
								end(0)
								{
								/* Nothing */
								}
						};
					static_assert(sizeof(in_out_count) == 16);		// make sure this will all happen with a x86_64 CMPXCHG16B instruction
					
				public:
					k_tree *tree;					// we're in this tree
					allocator *memory;			// we use this allocator to allocate memory
					in_out_count split_count;	// the number of splits the tree has undergone (used to see if the return path might have changed, and therefore a split cannot happen)

				public:
					context(k_tree *tree, allocator *memory, in_out_count split_count) noexcept :
						tree(tree),
						memory(memory),
						split_count(split_count)
						{
						/* Nothing */
						}
				};

		private:
			static constexpr float float_resolution = 0.000001;														// floats his close are considered equal

		public:
			std::atomic<split_state> state;			// is the node undergoing split or has it already been split?
			size_t max_children;							//	the order of the tree at this node (constant per tree as it propegates when a new node is created)
			std::atomic<size_t> children;				// the number of children currently at this node
			std::atomic<node *>*child;					// the immediate descendants of this node
			object *centroid;								// the centroid of this cluster
			std::atomic<size_t> leaves_below_this_point;			// the number of leaves below this node

		private:
			/*
				NODE::NODE()
				------------
			*/
			node() noexcept;

			/*
				NODE::TAKE_LOCK()
				-----------------
				Take the split lock (signal that we want to do a split).
				return true is we get the lock, false otherwise (and so result_retry should happen)
			*/
			static bool take_lock(context *context);

			/*
				NODE::RELEASE_LOCK()
				--------------------
				Release the split lock
			*/
			static void release_lock(context *context);

			/*
				NODE::SPLIT()
				-------------
				Split this node into two new children - knowing that the node is full (i.e child[0..max_children] are all non-null)
				Returns:
					True on success (the data was split into 2 clusters)
					Fase on failure (the data all ended up in one cluster)
			*/
			bool split(allocator *memory, node **child_1_out, node **child_2_out, size_t initial_first_cluster) const;

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
				NODE::NUMBER_OF_CHILDREN()
				--------------------------
				Return the number of children at this node under the knowledge that the node might be undergoing an update.
			*/
			size_t number_of_children(void) const;

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
				Split this node into two new children - knowing that the node is full (i.e child[0..max_children] are all non-null)
				Returns:
					True on success (the data was split into 2 clusters)
					Fase on failure (the data all ended up in one cluster)
			*/
			bool split(allocator *memory, node **child_1_out, node **child_2_out) const;

			/*
				NODE::ADD_TO_LEAF()
				-------------------
				Add the given data to the current leaf node.
				Returns whether or not there was a split (and so the node above must do a replacement and an add)
			*/
			node::result add_to_leaf(context *context, object *data, node **child_1, node **child_2);

			/*
				NODE::ADD_TO_NODE()
				-------------------
				Add the given data to the current tree at or below this point.
				Returns whether or not there was a split (and so the node above must do a replacement and an add)
			*/
			node::result add_to_node(context *context, object *data, node **child_1, node **child_2);

			/*
				NODE::NORMALISE_COUNTS()
				------------------------
				Fix the broken leaved-below counts that happened because of parallel updates
			*/
			void normalise_counts(void);

			/*
				NODE::DESERIALISE()
				-------------------
				Deserialise a previously serialised k-tree.  Serialised using cout << ktree;
			*/
			void deserialise(allocator &memory, std::istream &stream, object &example_object);

			/*
				NODE::TEXT_RENDER()
				-------------------
				Serialise the object in a human-readable format and down the given stream
			*/
			std::ostream &text_render(std::ostream &stream) const;

			/*
				NODE::TEXT_RENDER_MOVIE()
				-------------------------
				Serialise the object in a human-readable format and down the given stream
			*/
			std::ostream &text_render_movie(std::ostream &stream, uint32_t depth = 0) const;

			/*
				NODE::TEXT_RENDER_PENULTIMATE()
				-------------------------------
				Dump the level above the leaves (the bottom-level clusters)
			*/
			std::ostream &text_render_penultimate(std::ostream &stream) const;

		};
	}
