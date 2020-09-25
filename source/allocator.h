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
		Custom zone-based allocator
	*/
	class allocator
		{
		private:
			std::vector<uint8_t *> blocks;				// a list of the block we have allocated
			uint8_t *chunk;									// the current chunk we are allocating from
			size_t size;										// the size of the current block (in bytes)
			size_t used;										// the number of bytes of the current block that we have used
			bool use_global_malloc;							// should we use the C/C++ runtime malloc method?

		public:
			/*
				ALLOCATOR::ALLOCATOR()
				----------------------
				Constructor
			*/
			allocator(size_t block_size = 1'073'741'824 /* 1GB */, bool use_global_malloc = false) :
				chunk(nullptr),
				size(block_size),
				used(block_size),
				use_global_malloc(use_global_malloc)
				{
				/* Nothing */
				}

			/*
				ALLOCATOR::~ALLOCATOR()
				----------------------
				Destructor
			*/
			virtual ~allocator()
				{
				for (const auto block : blocks)
					delete [] block;
				}

			/*
				ALLOCATOR::MALLOC()
				-------------------
				Allocate a block of memory, initialise it to 0, and return it to the caller
			*/
			void *malloc(size_t bytes)
				{
				if (use_global_malloc)
					{
					uint8_t *data = new uint8_t [bytes];
					return (void *)data;
					}
				else
					{
					if (used + bytes > size)
						{
						chunk = new uint8_t[size];
						blocks.push_back(chunk);
						used = 0;
						}

					uint8_t *start = chunk + used;
					used += bytes;
					return (void *)start;
					}
				}
		};
	}
