/*
	DISK.H
	------
	Copyright (c) 2021 Andrew Trotman
	Released under the 2-clause BSD license (See:https://en.wikipedia.org/wiki/BSD_licenses)
*/

#pragma once

#include <string>
#include <vector>

size_t read_entire_file(const std::string &filename, std::string &into);
void buffer_to_list(std::vector<uint8_t *> &line_list, std::string &buffer);
size_t dimensionality(const char *line);
