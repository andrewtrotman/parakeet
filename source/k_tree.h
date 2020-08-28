/*
	K_TREE.H
	--------
	Copyright (c) 2020 Andrew Trotman
	Released under the 2-clause BSD license (See:https://en.wikipedia.org/wiki/BSD_licenses)
*/
#pragma once

#include <stdint.h>

#include <limits>
#include <iomanip>

#include "object.h"
#include "allocator.h"

namespace k_tree
	{
	std::ostream &operator<<(std::ostream &stream, const class k_tree &thing);			// forward declare the output operator
	const float float_resolution = 0.000001;														// floats his close are considered equal

	const size_t max_children = 4;

	/*
		CLASS NODE
		----------
	*/
	class node
		{
		public:
			object *centroid;						// the centroid of this cluster
			size_t children;						// the number of children of this node
			size_t leaves_below_this_point;	// the number of leaves below this node
			node **child;							// the immediate descendants of this node

		private:
			node() = delete;
			/*
				NODE::NODE()
				------------
			*/
			node(allocator &memory, object *data) :
				centroid(data),
				children(0),
				leaves_below_this_point(1),
				child(nullptr)
				{
				/* Nothing */
				}

			/*
				NODE::NODE()
				------------
			*/
			node(allocator &memory, node *first_child) :
				centroid(object::new_object(memory)),
				children(1),
				leaves_below_this_point(1),
				child(new (memory.malloc(sizeof(node *) * (max_children + 1))) node *[max_children + 1])
				{
				if (first_child == nullptr)
					leaves_below_this_point = children = 0;
				else
					child[0] = first_child;
				}

		public:
			/*
				NODE::NEW_NODE()
				----------------
			*/
			static node *new_node(allocator &memory, object *data)
				{
				return new (memory.malloc(sizeof(node))) node(memory, data);
				}

			/*
				NODE::NEW_NODE()
				----------------
			*/
			static node *new_node(allocator &memory, node *first_child)
				{
				return new (memory.malloc(sizeof(node))) node(memory, first_child);
				}

			/*
				NODE::ISLEAF()
				--------------
				Return whether or not this node is a leaf
			*/
			bool isleaf(void) const
				{
				return child == nullptr;
				}

			/*
				NODE::CLOSEST()
				---------------
				Returns the index of the child closest to the parameter what
			*/
			size_t closest(object *what) const
				{
				/*
					Initialise to the distance to the first element in the list
				*/
				size_t closest_child = 0;
				float min_distance = object::distance_squared(what, child[0]->centroid);

				/*
					Now check the distance to the others
				*/
				for (size_t which = 1; which < children; which++)
					{
					float distance = object::distance_squared(what, child[which]->centroid);
					if (distance < min_distance)
						{
						min_distance = distance;
						closest_child = which;
						}
					}

				return closest_child;
				}

			/*
				NODE::TEXT_RENDER()
				-------------------
				Serialise the object in a human-readable format and down the given stream
			*/
			void text_render(std::ostream &stream, size_t depth) const
				{
				stream << "(" << std::setw(3) << leaves_below_this_point << ")";
				stream << "{" << std::setw(3) << children << "}";
				if (depth != 0)
					stream << std::setw(depth * 2) << ' ';
				stream <<  *centroid << "\n";

				for (size_t who = 0; who < children; who++)
					child[who]->text_render(stream, depth + 1);
				}

			/*
				NODE::COMPUTE_MEAN()
				--------------------
				Compute the centroid of the children below this node.  Note that this must be a weighted average as not all branched have the
				same number of children and this resulting centroid should be the middle of the leaves not the middle of the children.  This also
				recomputes that count from the children (which might have recently changed)
			*/
			void compute_mean(void)
				{
				leaves_below_this_point = 0;
				centroid->zero();
				for (size_t which = 0; which < children; which++)
					{
					leaves_below_this_point += child[which]->leaves_below_this_point;
					centroid->fused_multiply_add(*child[which]->centroid, child[which]->leaves_below_this_point);
					}

				*centroid /= leaves_below_this_point;
				}

			/*
				NODE::SPLIT()
				-------------
				Split this node into two new children
			*/
			void split(allocator &memory, node **child_1_out, node **child_2_out) const
				{
				size_t place_in;
				size_t assignment[max_children + 1];
				size_t first_cluster_size;
				size_t second_cluster_size;
				float old_sum_distance = std::numeric_limits<float>::max();;
				float new_sum_distance = old_sum_distance / 2;

				/*
					allocate space
				*/
				object *centroid_1 = object::new_object(memory);
				object *centroid_2 = object::new_object(memory);
				node *child_1 = *child_1_out = new_node(memory, (node *)nullptr);
				node *child_2 = *child_2_out = new_node(memory, (node *)nullptr);

				/*
					Start with the first and last members of the current node.  It should really be 2 random elements, but close enough!
				*/
				*centroid_1 = *child[0]->centroid;
				*centroid_2 = *child[children - 1]->centroid;

				/*
					The stopping condition is that the sum squared distance from the cluster centres has become constant (so no more shuffling can happen)
				*/
				while (old_sum_distance > (1.0 + float_resolution) * new_sum_distance)
					{
					old_sum_distance = new_sum_distance;
					new_sum_distance = 0;
					first_cluster_size = second_cluster_size = 0;
					for (size_t which = 0; which < children; which++)
						{
						/*
							Compute the distance (squared) to each of the two new cluster centroids
						*/
						float distance_to_first = object::distance_squared(child[which]->centroid, centroid_1);
						float distance_to_second = object::distance_squared(child[which]->centroid, centroid_2);

						/*
							Choose a cluster, tie_break on the size of the cluster (put in the smallest to avoid empty clusters)
						*/
						if (distance_to_first == distance_to_second)
							place_in = first_cluster_size < second_cluster_size ? 0 : 1;
						else if (distance_to_first < distance_to_second)
							place_in = 0;
						else
							place_in = 1;

						/*
							Accumulate the stats for each new centroid (including the partial sum so that we can compute the centroid next)
						*/
						if (place_in == 0)
							{
							assignment[which] = 0;
							new_sum_distance += distance_to_first;
							first_cluster_size++;
							}
						else
							{
							assignment[which] = 1;
							new_sum_distance += distance_to_second;
							second_cluster_size++;
							}
						}

					/*
						Rebuild the centroids: first compute the sum
					*/
					centroid_1->zero();
					centroid_2->zero();
					for (size_t which = 0; which < children; which++)
						{
						if (assignment[which] == 0)
							*centroid_1 += *child[which]->centroid;
						else
							*centroid_2 += *child[which]->centroid;
						}

					/*
						Rebuild then centroids: then average
					*/
					*centroid_1 /= first_cluster_size;
					*centroid_2 /= second_cluster_size;
					}

				/*
					At this point we have the new centroids and we have which node goes where in assignment[] so we populate the two new nodes
				*/
				for (size_t which = 0; which < children; which++)
					if (assignment[which] == 0)
						{
						child_1->child[child_1->children] = child[which];
						child_1->children++;
						}
					else
						{
						child_2->child[child_2->children] = child[which];
						child_2->children++;
						}
				}

			/*
				NODE::ADD_TO_LEAF()
				-------------------
				Add the given data to the current leaf node.
				Returns whether or not there was a split (and so the node above must do a replacement and an add)
			*/
			bool add_to_leaf(allocator &memory, object *data, node **child_1, node **child_2)
				{
				node *another = node::new_node(memory, data);
				child[children] = another;
				children++;
				if (children > max_children)
					{
					split(memory, child_1, child_2);
					(*child_1)->compute_mean();
					(*child_2)->compute_mean();
					return true;
					}
				return false;
				}

			/*
				NODE::ADD_TO_NODE()
				-------------------
				Add the given data to the current tree at or below this point.
				Returns whether or not there was a split (and so the node above must do a replacement and an add)
			*/
			bool add_to_node(allocator &memory, object *data, node **child_1, node **child_2)
				{
				bool did_split = false;
				if (child[0]->isleaf())
					did_split = add_to_leaf(memory, data, child_1, child_2);
				else
					{
					size_t best_child = closest(data);
					did_split = child[best_child]->add_to_node(memory, data, child_1, child_2);
					if (did_split)
						{
						did_split = false;
						child[best_child] = *child_1;
						child[children] = *child_2;
						children++;

						if (children > max_children)
							{
							split(memory, child_1, child_2);
							(*child_1)->compute_mean();
							(*child_2)->compute_mean();
							did_split = true;
							}
						}
					}
				/*
					Update the mean for the cuttent node as data has been added somewher below here.
				*/
				compute_mean();

				/*
					Return whether or not we caused a split, and therefore replacement is necessary
				*/
				return did_split;
				}
		};

	/*
		CLASS K_TREE
		------------
	*/
	class k_tree
		{
		public:
			node *root;						// the root of the k-tree
			allocator &memory;			// all memory allocation happens through this allocator

		public:
			/*
				K_TREE::K_TREE()
				----------------
			*/
			k_tree(allocator &memory) :
				root(nullptr),
				memory(memory)
				{
				/* Nothing */
				}

			/*
				K_TREE::PUSH_BACK()
				-------------------
			*/
			void push_back(allocator &memory, object *data)
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
					node *leaf = node::new_node(memory, data);
					root = node::new_node(memory, leaf);
					root->compute_mean();
					}
				else
					did_split = root->add_to_node(memory, data, &child_1, &child_2);

				/*
					Adding caused a split at the top level so we create a new root consisting of the two children
				*/
				if (did_split)
					{
					node *new_root = node::new_node(memory, child_1);
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
			void push_back(object *data)
				{
				push_back(memory, data);
				}

			/*
				K_TREE::TEXT_RENDER()
				---------------------
			*/
			void text_render(std::ostream &stream) const
				{
				if (root != nullptr)
					root->text_render(stream, 0);
				}

			/*
				K_TREE::UNITTEST()
				------------------
			*/
			static void unittest(void);
		};

	/*
		OPERATOR<<()
		------------
	*/
	inline std::ostream &operator<<(std::ostream &stream, const k_tree &thing)
		{
		thing.text_render(stream);
		return stream;
		}
	}
