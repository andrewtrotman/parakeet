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
		k_tree::object *centroid;
		std::vector<k_tree::object *> point;
	};

typedef std::vector<cluster *> cluster_set;
