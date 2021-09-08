/*
	DISK.CPP
	--------
	Copyright (c) 2021 Andrew Trotman
	Released under the 2-clause BSD license (See:https://en.wikipedia.org/wiki/BSD_licenses)
*/
#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <string>
#include <vector>

#include "disk.h"

/*
	READ_ENTIRE_FILE()
	------------------
	This uses a combination of "C" FILE I/O and C++ strings in order to copy the contents of a file into an internal buffer.
	There are many different ways to do this, but this is the fastest according to this link: http://insanecoding.blogspot.co.nz/2011/11/how-to-read-in-file-in-c.html
	Note that there does not appear to be a way in C++ to avoid the initialisation of the string buffer.

	Returns the length of the file in bytes - which is also the size of the string buffer once read.
*/
size_t read_entire_file(const std::string &filename, std::string &into)
	{
	FILE *fp;
	// "C" pointer to the file
#ifdef _MSC_VER
	struct __stat64 details;				// file system's details of the file
#else
	struct stat details;				// file system's details of the file
#endif
	size_t file_length = 0;			// length of the file in bytes

	/*
		Fopen() the file then fstat() it.  The alternative is to stat() then fopen() - but that is wrong because the file might change between the two calls.
	*/
	if ((fp = fopen(filename.c_str(), "rb")) != nullptr)
		{
#ifdef _MSC_VER
		if (_fstat64(fileno(fp), &details) == 0)
#else
		if (fstat(fileno(fp), &details) == 0)
#endif
			if ((file_length = details.st_size) != 0)
				{
				into.resize(file_length);
				if (fread(&into[0], details.st_size, 1, fp) != 1)
					into.resize(0);				// LCOV_EXCL_LINE	// happens when reading the file_size buyes failes (i.e. disk or file failure).
				}
		fclose(fp);
		}

	return file_length;
	}

/*
	BUFFER_TO_LIST()
	----------------
	Turn a single std::string into a vector of uint8_t * (i.e. "C" Strings). Note that these pointers are in-place.  That is,
	they point into buffer so any change to the uint8_t or to buffer effect each other.

	Note: This method removes blank lines from the input file.
*/
void buffer_to_list(std::vector<uint8_t *> &line_list, std::string &buffer)
	{
	uint8_t *pos;
	size_t line_count = 0;

	/*
		Walk the buffer counting how many lines we think are in there.
	*/
	pos = (uint8_t *)&buffer[0];
	while (*pos != '\0')
		{
		if (*pos == '\n' || *pos == '\r')
			{
			/*
				a seperate line is a consequative set of '\n' or '\r' lines.  That is, it removes blank lines from the input file.
			*/
			while (*pos == '\n' || *pos == '\r')
				pos++;
			line_count++;
			}
		else
			pos++;
		}

	/*
		resize the vector to the right size, but first clear it.
	*/
	line_list.clear();
	line_list.reserve(line_count);

	/*
		Now rewalk the buffer turning it into a vector of lines
	*/
	pos = (uint8_t *)&buffer[0];
	if (*pos != '\n' && *pos != '\r' && *pos != '\0')
		line_list.push_back(pos);
	while (*pos != '\0')
		{
		if (*pos == '\n' || *pos == '\r')
			{
			*pos++ = '\0';
			/*
				a seperate line is a consequative set of '\n' or '\r' lines.  That is, it removes blank lines from the input file.
			*/
			while (*pos == '\n' || *pos == '\r')
				pos++;
			if (*pos != '\0')
				line_list.push_back(pos);
			}
		else
			pos++;
		}
	}

/*
	DIMENSIONALITY()
	----------------
	Return the dimensionality of the vector passed in line
*/
size_t dimensionality(const char *line)
	{
	size_t dimensions = 0;
	const char *pos = line;
	do
		{
		while (isspace(*pos))
			pos++;
		if (*pos == '\0')
			break;
		dimensions++;
		while (*pos != '\0' && !isspace(*pos))
			pos++;
		}
	while (*pos != '\0');

	return dimensions;
	}
