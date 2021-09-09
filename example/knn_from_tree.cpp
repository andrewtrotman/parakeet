/*
	KNN_FROM_TREE.CPP
	-----------------
	Copyright (c) 2020 Andrew Trotman
	Released under the 2-clause BSD license (See:https://en.wikipedia.org/wiki/BSD_licenses)

	Find the k nearest neighbours to a given point using the clisters exported from the k-tree
	The format of the tree is:
		<children> <leaves_below> <cluster_center>
	The format of the query is:
		<point>
	Where <cluster_center> and <point> are floating point vectors
*/
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <algorithm>

#include "disk.h"
#include "cluster.h"
#include "allocator.h"
#include "fast_double_parser.h"

/*
	TEXT_TO_VECTOR()
	----------------
*/
std::vector<float> *text_to_vector(size_t dimensions, const char *text)
	{
	const char *pos = text;
	size_t dimension = 0;

	std::vector<float> *answer = new std::vector<float>();
	answer->resize(dimensions);

	do
		{
		while (isspace(*pos))
			pos++;
		if (*pos == '\0')
			break;

		double value_as_double;
		float value;
		bool isok = fast_double_parser::parse_number(pos, &value_as_double);
		if (isok)
			value = (float)value_as_double;
		else
			value = (float)atof(pos);
		while (*pos != '\0' && !isspace(*pos))
			pos++;
		(*answer)[dimension] = value;
		dimension++;
		}
	while (*pos != '\0');

	return answer;
	}

/*
	TEXT_TO_VECTOR()
	----------------
*/
std::vector<float> *text_to_vector(size_t &children, size_t dimensions, const char *text)
	{
	const char *pos = text;

	children = atol(pos);

 	pos = strchr(pos, ' ') + 1;
	pos = strchr(pos, ' ') + 1;

	return text_to_vector(dimensions, pos);
	}

/*
	READ_TREE()
	-----------
*/
void read_tree(const char *filename, k_tree::allocator &memory, cluster_set &space)
	{
	std::string contents;
	std::vector<uint8_t *> lines;

	/*
		Read the file and convert into a list of lines
	*/
	read_entire_file(filename, contents);
	buffer_to_list(lines, contents);

	/*
		Count the dimensionality of the vectors
		Note that the input format starts with <children> and <leaves_below> and therefore has 2 more values in it than the dimensionality of the vector
	*/
	size_t dimensions = dimensionality((char *)lines[0]) - 2;
	k_tree::object *object_template = k_tree::object::snag(&memory, dimensions, nullptr);

	/*
		The first line in the file must be a node, it then specifies how many children it has
		which we can read and plonk into an struct, so we need a vector of structs of vectors
	*/
	for (size_t line = 0; line < lines.size(); line++)
		{
		size_t children;

		std::vector<float> *point = text_to_vector(children, dimensions, (char *)lines[line]);
		cluster *point_of_interest = new cluster();
		point_of_interest->centroid = object_template->new_object(&memory, dimensions, &(*point)[0]);
		point_of_interest->point.resize(children);
		if (children == 0)
			exit(printf("This data isn't clusters, is it a tree?\n"));

		for (size_t child = 0; child < children; child++, line++)
			{
			size_t zero;
			std::vector<float> *point = text_to_vector(zero, dimensions, (char *)lines[line + 1]);
			point_of_interest->point[child] = object_template->new_object(&memory, dimensions, &(*point)[0]);
			if (zero != 0)
				exit(printf("This data isn't clusters, is it a tree?\n"));
			}

		space.push_back(point_of_interest);
		}
	}

/*
	READ_QUERIES()
	--------------
*/
void read_queries(const char *filename, k_tree::allocator &memory, std::vector<k_tree::object *> &query_list)
	{
	std::string contents;
	std::vector<uint8_t *> lines;

	/*
		Read the file and convert into a list of lines
	*/
	read_entire_file(filename, contents);
	buffer_to_list(lines, contents);

	/*
		Count the dimensionality of the vectors
	*/
	size_t dimensions = dimensionality((char *)lines[0]);
	k_tree::object *object_template = k_tree::object::snag(&memory, dimensions, nullptr);

	/*
		Read the queries
	*/
	for (size_t which = 0; which < lines.size(); which++)
		{
		std::vector<float> *point = text_to_vector(dimensions, (char *)lines[which]);
		query_list.push_back(object_template->new_object(&memory, dimensions, &(*point)[0]));
		}
	}

/*
	RANK_CLUSTERS()
	---------------
*/
void rank_clusters(std::vector<cluster::distance> &ordering, cluster_set space, k_tree::object *query)
	{
	/*
		Find the distance to each cluster
	*/
	for (const auto point : space)
		{
		float distance = point->centroid->distance_squared(query);
		ordering.push_back(cluster::distance(point, distance));
		}

	/*
		Rank the clusters from closest to futherest away
	*/
	std::sort(ordering.begin(), ordering.end());

	/*
		TO DO: Rank the points within the clusters starting with the nearest cluster
	*/
	}

/*
	TEXT_RENDER()
	-------------
*/
void text_render(cluster_set &space)
	{
	for (const auto &cluster : space)
		{
		std::cout << *cluster->centroid << "\n";
		for (const auto &point : cluster->point)
			std::cout << "|-> " << *point << "\n";
		}
	}

/*
	TEXT_RENDER()
	-------------
*/
void text_render(std::vector<k_tree::object *> query_list)
	{
	for (const auto &query : query_list)
		std::cout << *query << "\n";
	}

/*
	USAGE()
	-------
*/
int usage(const char *exename)
	{
	printf("%s <tree> <query>\n", exename);
	return 1;
	}

/*
	MAIN()
	------
*/
int main(int argc, const char *argv[])
	{
	k_tree::allocator memory;
	std::vector<k_tree::object *> query_list;
	cluster_set space;

	/*
		Check the command line parameters
	*/
	if (argc != 3)
		exit(usage(argv[0]));

	/*
		Read the tree
	*/
	read_tree(argv[1], memory, space);

	/*
		Dump the tree
	*/
//	text_render(space);
//	std::cout << "\n\n";

	/*
		Read the queries
	*/
	read_queries(argv[2], memory, query_list);

	/*
		Dump the queries
	*/
//	text_render(query_list);
//	std::cout << "\n\n";

	for (const auto &query : query_list)
		{
		std::vector<cluster::distance> ordering;
		std::cout << "Q:" << *query << "\n";
		rank_clusters(ordering, space, query);
		for (const auto &point : ordering)
			{
			std::cout << point.size << " : " << *point.cluster->centroid << "\n";
			}
		}

	return 0;
	}
