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
	const size_t MAX_NODE_WIDTH = 1024;

	/*
		CLASS K_TREE
		------------
	*/
	class k_tree
		{
		public:
			/*
				CLASS K_TREE::NODE
				------------------
			*/
			class node
				{
				public:
					size_t width;			/// the number of elements in the arrays that are actually used
					object member[MAX_NODE_WIDTH];
					node *children[MAX_NODE_WIDTH];
					node *parent;

				public:
					/*
						K_TREE::NODE::NODE()
						--------------------
					*/
					node() :
						width(0)
						{
						/* Nothing */
						}

					/*
						K_TREE::NODE::CLOSEST()
						-----------------------
						@return a reference to the closest node
						@param distance_to_closest [out] The distance to the closest node
					*/
					object &closest(object what, double &distance_to_closest)
						{
						/*
							Initialise to the distance to the first element in the list
						*/
						size_t node = 0;
						double min_distance = object::distance_squared(what, member[0]);

						/*
							Now check the distance to the others
						*/
						for (size_t which = 1; which < width; which++)
							{
							double distance = object::distance_squared(what, member[which]);
							if (distance < min_distance)
								{
								min_distance = distance;
								node = which;
								}
							}
						distance_to_closest = min_distance;
						return member[node];
						}

					/*
						K_TREE::NODE::SPLIT()
						---------------------
						k-means clustering where k = 2
					*/
					void split(object *centroid)
						{
						size_t assignment[MAX_NODE_WIDTH];
						size_t first_cluster_size = 0;
						double old_sum_distance = std::numeric_limits<double>::max();;
						double new_sum_distance = old_sum_distance / 2;

						/*
							Start with the first and last members (it should be 2 random elements)
						*/
						centroid[0] = member[0];
						centroid[1] = member[width - 1];

						/*
							The stopping condition is that the sum squared distance from the cluster centres has become constant (so no more shuffling can happen)
						*/
						while (old_sum_distance > (1.0 + double_resolution) * new_sum_distance)
							{
//std::cout << "->" << centroid[0] << centroid[1] << "\n";
							old_sum_distance = new_sum_distance;
							new_sum_distance = 0;
							first_cluster_size = 0;
							for (size_t which = 0; which < width; which++)
								{
								/*
									Compute the distance (squared) to each of the two new cluster centroids
								*/
								double distance_to_first = object::distance_squared(member[which], centroid[0]);
								double distance_to_second = object::distance_squared(member[which], centroid[1]);

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
							centroid[0].zero();
							centroid[1].zero();
//std::cout << "->" << centroid[0] << centroid[1] << "\n";
							for (size_t which = 0; which < width; which++)
								{
								if (assignment[which] == 0)
									centroid[0] += member[which];
								else
									centroid[1] += member[which];
								}
//std::cout << "->" << centroid[0] << centroid[1] << "\n";

							/*
								Rebuild then centroids, then average
							*/
							centroid[0] /= first_cluster_size;
							centroid[1] /= width - first_cluster_size;
//std::cout << "->" << centroid[0] << centroid[1] << "\n";
							}
						}
				};
		public:

			/*
				K_TREE::UNITTEST()
				------------------
			*/
			static void unittest(void)
				{
				node blob;

				blob.width = 10;
				for (size_t which = 0; which < blob.width; which++)
					for (size_t dimension = 0; dimension < object::DIMENSIONS; dimension++)
						if (which < blob.width / 2)
							blob.member[which].vector[dimension] = (rand() % 20) / 10.0;
						else
							blob.member[which].vector[dimension] = ((rand() % 20) + 70) / 10.0;

				object center[2];
				blob.split(center);


				for (size_t which = 0; which < blob.width; which++)
					std::cout << blob.member[which] << "\n";

				std::cout << "\n";

				std::cout << center[0] << "\n";
				std::cout << center[1] << "\n";
				}
		};
	}
