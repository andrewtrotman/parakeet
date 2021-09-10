/*
	CLUSTER.H
	---------
	Copyright (c) 2021 Andrew Trotman
	Released under the 2-clause BSD license (See:https://en.wikipedia.org/wiki/BSD_licenses)
*/
#pragma once

#include "object.h"

class cluster
	{
	public:
		class distance
			{
			public:
				cluster *cluster;
				float size;

			public:
				distance(class cluster *cluster, float size) :
					cluster(cluster),
					size(size)
					{
					/* Nothing */
					}

				bool operator<(const distance &with) const
					{
					return size < with.size;
					}
			};
			
		class point_distance
			{
			public:
				k_tree::object *point;
				float size;

			public:
				point_distance(k_tree::object *point, float size) :
					point(point),
					size(size)
					{
					/* Nothing */
					}
				point_distance() :
					point(nullptr),
					size(std::numeric_limits<float>::max())
					{
					/* Nothing */
					}

				bool operator<(const point_distance &with) const
					{
					return size < with.size;
					}
			};

	public:
		k_tree::object *centroid;
		std::vector<k_tree::object *> point;
	};

typedef std::vector<cluster *> cluster_set;
