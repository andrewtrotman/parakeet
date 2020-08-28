/*
	ALLOCATOR.H
	-----------
	Copyright (c) 2020 Andrew Trotman
	Released under the 2-clause BSD license (See:https://en.wikipedia.org/wiki/BSD_licenses)
*/
#pragma once

#include <vector>

namespace k_tree
	{
	/*
		CLASS ALLOCATOR
		---------------
	*/
	class allocator
		{
		private:
			std::vector<uint8_t *> blocks;
			uint8_t *chunk;
			size_t size;
			size_t used;

		public:
			/*
				ALLOCATOR::ALLOCATOR()
				----------------------
			*/
			allocator(size_t block_size = 1024*1024*1024) :
				chunk(nullptr),
				size(block_size),
				used(block_size)
				{
				/* Nothing */
				}

			/*
				ALLOCATOR::~ALLOCATOR()
				----------------------
			*/
			virtual ~allocator()
				{
				for (const auto block : blocks)
					delete [] block;
				}

			/*
				ALLOCATOR::MALLOC()
				-------------------
			*/
			void *malloc(size_t bytes)
				{
				if (used + bytes > size)
					{
					chunk = new uint8_t[size];
					blocks.push_back(chunk);
					used = 0;
					}

				uint8_t *start = chunk + used;
				used += bytes;
				return start;
				}
		};
	}
