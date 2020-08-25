/*
	K_TREE.H
	--------
*/
#pragma once

#include <limits>

#include "object.h"

namespace k_tree
	{
	const double double_resolution = 0.000001;
	const size_t MAX_NODE_WIDTH = 4;

	/*
		CLASS K_TREE
		------------
	*/
	class k_tree
		{
		friend std::ostream &operator<<(std::ostream &stream, const k_tree &thing);
		public:
			/*
				CLASS K_TREE::DATA_CHILD
				------------------------
			*/
			class node;
			class data_child
				{
				public:
					object *centroid;	// the centroid of the child
					node *child;		// the pointer to the child node

				public:
					data_child()
						{
						/* Nothing */
						}
					data_child(object *centroid, node *child) :
						centroid(centroid),
						child(child)
						{
						/* Nothing */
						}
				};
			static_assert(sizeof(data_child) == 16);		// necessary for the 128-bit (16-byte) compare exchange instruction on x64 and ARM64

			/*
				CLASS K_TREE::NODE
				------------------
			*/
			class node
				{
				public:
					/*
						ENUM NODE_TYPE
						--------------
					*/
					enum node_type
						{
						leaf_node,									// is currently a leaf node of the k-tree
						internal_node								// is and internal node (non-leaf) of the k-tree
						};

				public:
					node_type type;								// Is this an internal node or a leaf node
					size_t width;									// The number of elements in the arrays that are actually used
					node *parent;									// The parent of this node
					data_child pair[MAX_NODE_WIDTH];			// The pair of centroid and child pointer (the child has that centroid)

				public:
					/*
						K_TREE::NODE::NODE()
						--------------------
					*/
					node() :
						type(leaf_node),
						width(0),
						parent(nullptr)
						{
						/* Nothing */
						}

					/*
						K_TREE::NODE::NEW_NODE()
						------------------------
					*/
					static node *new_node()
						{
						return new node();
						}

					/*
						K_TREE::NODE::ADD()
						-------------------
					*/
					data_child add(node **source, data_child &another)
						{
						if (type == leaf_node)
							return add_to_leaf(source, another);
						else
							{
							size_t descendant = closest(*another.centroid);
							return pair[descendant].child->add(&pair[descendant].child, another);
							}
						}

					/*
						K_TREE::NODE::ADD_TO_LEAF()
						---------------------------
					*/
					data_child add_to_leaf(node **source, data_child &another)
						{
						if (width < MAX_NODE_WIDTH)
							{
							/*
								We fit in the current leaf node
							*/
							pair[width] = another;
							width++;

							return data_child(nullptr, nullptr);
							}
						else
							{
							/*
								We are a leaf that has become full so we must split and propegate upwards
							*/
							data_child child_0(object::new_object(), node::new_node());
							data_child child_1(object::new_object(), node::new_node());

							split(child_0, child_1);

							/*
								What pointed to us gets updated to child_0
								What's left over gets passed back
							*/
							*source = child_0.child;
							return child_1;
							}
						}

					/*
						K_TREE::NODE::CLOSEST()
						-----------------------
						@return a reference to the closest node
						@param distance_to_closest [out] The distance to the closest node
					*/
					size_t closest(object &what) const
						{
						/*
							Initialise to the distance to the first element in the list
						*/
						size_t node = 0;
						double min_distance = object::distance_squared(what, *pair[0].centroid);

						/*
							Now check the distance to the others
						*/
						for (size_t which = 1; which < width; which++)
							{
							double distance = object::distance_squared(what, *pair[which].centroid);
							if (distance < min_distance)
								{
								min_distance = distance;
								node = which;
								}
							}

						return node;
						}

					/*
						K_TREE::NODE::SPLIT()
						---------------------
						k-means clustering where k = 2
					*/
					void split(data_child &child_0, data_child &child_1)
						{
						size_t assignment[MAX_NODE_WIDTH];
						size_t first_cluster_size = 0;
						double old_sum_distance = std::numeric_limits<double>::max();;
						double new_sum_distance = old_sum_distance / 2;

						/*
							Start with the first and last members (it should be 2 random elements).  This also guarantees that there will be at least on member in each cluster.
						*/
						*child_0.centroid = *pair[0].centroid;
						*child_1.centroid = *pair[width - 1].centroid;

						/*
							The stopping condition is that the sum squared distance from the cluster centres has become constant (so no more shuffling can happen)
						*/
						while (old_sum_distance > (1.0 + double_resolution) * new_sum_distance)
							{
							old_sum_distance = new_sum_distance;
							new_sum_distance = 0;
							first_cluster_size = 0;
							for (size_t which = 0; which < width; which++)
								{
								/*
									Compute the distance (squared) to each of the two new cluster centroids
								*/
								double distance_to_first = object::distance_squared(*pair[which].centroid, *child_0.centroid);
								double distance_to_second = object::distance_squared(*pair[which].centroid, *child_1.centroid);

								/*
									Accumulate the stats for each new centroid (including the partial sum so that we can compute the centroid next)
								*/
								if (distance_to_first <= distance_to_second)
									{
									assignment[which] = 0;
									new_sum_distance += distance_to_first;
									first_cluster_size++;
									}
								else
									{
									assignment[which] = 1;
									new_sum_distance += distance_to_second;
									}
								}

							/*
								Rebuild then centroids, first compute the sum
							*/
							child_0.centroid->zero();
							child_1.centroid->zero();
							for (size_t which = 0; which < width; which++)
								{
								if (assignment[which] == 0)
									*child_0.centroid += *pair[which].centroid;
								else
									*child_1.centroid += *pair[which].centroid;
								}

							/*
								Rebuild then centroids, then average
							*/
							*child_0.centroid /= first_cluster_size;
							*child_1.centroid /= width - first_cluster_size;
							}

						/*
							At this point we have the new centroids and we have which node goes where in assignment[] so we populate the two new nodes
						*/
						size_t squash = 0;
						size_t expand = 0;
						for (size_t which = 0; which < width; which++)
							{
							if (assignment[which] == 0)
								child_0.child->pair[squash++] = pair[which];
							else
								child_1.child->pair[expand++] = pair[which];
							}
						child_0.child->type = type;
						child_0.child->width = squash;

						child_1.child->type = type;
						child_1.child->width = expand;
						}
				};

		public:
			data_child root;

		public:
			/*
				K_TREE::K_TREE()
				----------------
			*/
			k_tree() :
				root(object::new_object(), node::new_node())
				{
				/* Nothing */
				}

			/*
				K_TREE::GET_ROOT()
				------------------
			*/
			data_child &get_root(void)
				{
				return root;
				}

			/*
				K_TREE::PUSH_BACK()
				-------------------
			*/
			void push_back(object &another)
				{
				data_child thing(&another, nullptr);
				data_child result = root.child->add(&root.child, thing);
				if (result.centroid != nullptr)
					{
					/*
						Replace the root.  The old root and the one that doesn't fit form the new root (with 2 children)
					*/
					data_child new_root(nullptr, node::new_node());
					new_root.child->add(&new_root.child, root);
					new_root.child->add(&new_root.child, result);
					new_root.child->type = node::internal_node;
					root = new_root;
					}
				}

			/*
				K_TREE::TEXT_RENDER()
				---------------------
			*/
			std::ostream &text_render(std::ostream &stream, const node &from, size_t depth = 0) const
				{
				for (size_t descendant = 0; descendant < from.width; descendant++)
					{
					for (size_t space = 0; space < depth; space++)
						stream << ' ';
					stream << *from.pair[descendant].centroid << "\n";
					if (from.pair[descendant].child != NULL)
						text_render(stream, *from.pair[descendant].child, depth + 1);
					}
				return stream;
				}

			/*
				K_TREE::UNITTEST()
				------------------
			*/
			static void unittest(void)
				{
				k_tree tree;

std::cout << tree << "\n\n";

				for (size_t which = 0; which < MAX_NODE_WIDTH * 3; which++)
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

#ifdef NEVER
				std::cout << "SOURCE\n";
				for (size_t which = 0; which < tree.get_root().child->width; which++)
					std::cout << *tree.get_root().child->pair[which].centroid << "\n";

				data_child center_0(object::new_object(), node::new_node());
				data_child center_1(object::new_object(), node::new_node());
				tree.get_root().child->split(center_0, center_1);

				std::cout << "CHILD 1\n";
				for (size_t which = 0; which < center_0.child->width; which++)
					std::cout << *center_0.child->pair[which].centroid << "\n";

				std::cout << "CHILD 2\n";
				for (size_t which = 0; which < center_1.child->width; which++)
					std::cout << *center_1.child->pair[which].centroid << "\n";

				std::cout << "\n";

				std::cout << *center_0.centroid << "\n";
				std::cout << *center_1.centroid << "\n";
#endif

				std::cout << "TREE\n";
				std::cout << tree;
				}
		};

	/*
		OPERATOR<<()
		------------
	*/
	inline std::ostream &operator<<(std::ostream &stream, const k_tree &thing)
		{
		return thing.text_render(stream, *thing.root.child);
		}

	}
