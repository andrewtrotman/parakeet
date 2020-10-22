/*
	OBJECT.H
	--------
	Copyright (c) 2020 Andrew Trotman
	Released under the 2-clause BSD license (See:https://en.wikipedia.org/wiki/BSD_licenses)
*/
#pragma once

#include <assert.h>
#include <string.h>
#include <immintrin.h>

#include <iostream>

#include "allocator.h"

namespace k_tree
	{
	std::ostream &operator<<(std::ostream &stream, const class object &thing);

	/*
		CLASS OBJECT
		------------
	*/
	class object
		{
		friend class node;
		friend class k_tree;
		static_assert(sizeof(float) == 4);			// floats must be 32-bit for the SIMD code to work

		public:
			size_t dimensions;			// the number of dimensions of the float
			float *vector;											// this object is simply a vector of floats (with SIMD single precision for operations)

		private:
			/*
				OBJECT::OBJECT()
				----------------
				Can only be constructed through new_object
			*/
			object()
				{
				/* Nothing */
				}

			/*
				OBJECT::OBJECT()
				----------------
				Can only be constructed through new_object
			*/
			object(size_t dimensions) :
				dimensions(dimensions)
				{
				/* Nothing */
				}

		public:
			/*
				OBJECT::NEW_OBJECT()
				--------------------
				Return a new object placement-constructed in memory provided by the allocator
			*/
			object *new_object(allocator *allocator)
				{
				/*
					Allocate a new object
				*/
				object *answer = new (allocator->malloc(sizeof(object))) object;

				/*
					Set the number of dimensions
				*/
				answer->dimensions = dimensions;

				/*
					Allocate space for the vector.
					To work with AVX2 this needs to rounded up to the nearest 8.  For AVX-512 round up to the nearest 16.
				*/
				#ifdef __AVX512F__
					size_t floats_per_word = 16;
				#elif defined(__AVX2__)
					size_t floats_per_word = 8;
				#else
					static_assert(false, "Must have either AVX2 or AVX512F");
				#endif

				size_t width = (dimensions + floats_per_word - 1) / floats_per_word * floats_per_word;
				answer->vector = (float *)allocator->malloc(sizeof(*vector) * width);
				memset(answer->vector, 0, sizeof(*answer->vector ) * width);

				/*
					Make sure we've flushed to memory before we return (to prevent instruction re-ordering)
				*/
				std::atomic_thread_fence(std::memory_order_seq_cst);

				return answer;
				}

			/*
				SIMD::HORIZONTAL_SUM()
				----------------------
				Calculate the horizontal sum of the 32-bit integers in an AVX2 register
				Returns the sum of all the members of the parameter
			*/
			static float horizontal_sum(__m256 elements)
				{
				/*
					shift left by 1 integer and add
					H G F E D C B A
					0 H G F 0 D C B
				*/
				__m256 bottom = _mm256_castsi256_ps(_mm256_bsrli_epi128(_mm256_castps_si256(elements), 4));
				elements = _mm256_add_ps(elements, bottom);

				/*
					shift left by 2 integers and add
					H0 GH FG EF D0 CD BC AB
					00 00 H0 GH 00 00 D0 CD
				*/
				bottom = _mm256_castsi256_ps(_mm256_bsrli_epi128(_mm256_castps_si256(elements), 8));
				elements = _mm256_add_ps(elements, bottom);
				/*
					We have: H000 GH00 EFGH D000 CD00 BCD0 ABCD
				*/

				/*
					shuffle to get: EFGH EFGH EFGH EFGH ABCD ABCD ABCD ABCD
					permute to get: 0000 0000 0000 0000 EFGH EFGH EFGH EFGH
				*/
				__m256 missing = _mm256_castsi256_ps(_mm256_shuffle_epi32(_mm256_castps_si256(elements), _MM_SHUFFLE(0, 0, 0, 0)));
				missing = _mm256_castsi256_ps(_mm256_permute2x128_si256(_mm256_setzero_si256(), _mm256_castps_si256(missing), 3));

				/*
					add
					EFGH EFGH EFGH EFGH ABCD ABCD ABCD ABCD
					0000 0000 0000 0000 EFGH EFGH EFGH EFGH
					leaving ABCDEFGH in the bottom worf
				*/
				__m256 answer = _mm256_add_ps(elements, missing);

				/*
					extract the bottom word
				*/
				return  _mm256_cvtss_f32(answer);
				}

			/*
				OBJECT::DISTANCE_SQUARED()
				--------------------------
				Return the square of the Euclidean distance between parameters a and b using SIMD operations
			*/
			float distance_squared(const object *b)
				{
				float total = 0;
				#ifdef __AVX512F__
					for (size_t dimension = 0; dimension < dimensions; dimension += 16)
						{
						__m512 diff = _mm512_sub_ps(_mm512_loadu_ps(vector + dimension), _mm512_loadu_ps(b->vector + dimension));
						__m512 result = _mm512_mul_ps(diff, diff);
						total += _mm512_reduce_add_ps(result);
						}
				#else
					for (size_t dimension = 0; dimension < dimensions; dimension += 8)
						{
						__m256 diff = _mm256_sub_ps(_mm256_loadu_ps(vector + dimension), _mm256_loadu_ps(b->vector + dimension));
						__m256 result = _mm256_mul_ps(diff, diff);
						total += horizontal_sum(result);
						}

				#endif
				return total;
				}

			/*
				OBJECT::DISTANCE_SQUARED_LINEAR()
				---------------------------------
				Return the square of the Euclidean distance between parameters a and b without using SIMD operations
			*/
			double distance_squared_linear(const object *b)
				{
				double total = 0;

				for (size_t dimension = 0; dimension < dimensions; dimension++)
					total += (vector[dimension] - b->vector[dimension]) * (vector[dimension] - b->vector[dimension]);

				return total;
				}

			/*
				OBJECT::ZERO()
				--------------
				Set all elements in the vector to zero
			*/
			void zero(void)
				{
				memset(vector, 0, sizeof(*vector) * dimensions);
				}

			/*
				OBJECT::OPERATOR=()
				-------------------
			*/
			void operator=(const object &operand)
				{
				memcpy(vector, operand.vector, sizeof(*vector) * dimensions);
				}

			/*
				OBJECT::OPERATOR+=()
				--------------------
				this += operand
			*/
			void operator+=(const object &operand)
				{
				#ifdef __AVX512F__
					for (size_t dimension = 0; dimension < dimensions; dimension += 16)
						_mm512_storeu_ps(vector + dimension, _mm512_add_ps(_mm512_loadu_ps(vector + dimension), _mm512_loadu_ps(operand.vector + dimension)));
				#else
					for (size_t dimension = 0; dimension < dimensions; dimension += 8)
						_mm256_storeu_ps(vector + dimension, _mm256_add_ps(_mm256_loadu_ps(vector + dimension), _mm256_loadu_ps(operand.vector + dimension)));
				#endif
				}

			/*
				OBJECT::OPERATOR/=()
				--------------------
				this /= operand
			*/
			void operator/=(float constant)
				{
				#ifdef __AVX512F__
					__m512 divisor = _mm512_set1_ps(constant);

					for (size_t dimension = 0; dimension < dimensions; dimension += 16)
						_mm512_storeu_ps(vector + dimension, _mm512_div_ps(_mm512_loadu_ps(vector + dimension), divisor));
				#else
					__m256 divisor = _mm256_set1_ps(constant);

					for (size_t dimension = 0; dimension < dimensions; dimension += 8)
						_mm256_storeu_ps(vector + dimension, _mm256_div_ps(_mm256_loadu_ps(vector + dimension), divisor));
				#endif
				}

			/*
				OBJECT::FUSED_MULTIPLY_ADD()
				----------------------------
				this += operand * constant
			*/
			void fused_multiply_add(object &operand, float constant)
				{
				#ifdef __AVX512F__
					__m512 factor = _mm512_set1_ps(constant);
					for (size_t dimension = 0; dimension < dimensions; dimension += 16)
						_mm512_storeu_ps(vector + dimension, _mm512_fmadd_ps(_mm512_loadu_ps(operand.vector + dimension), factor, _mm512_loadu_ps(vector + dimension)));
				#else
					__m256 factor = _mm256_set1_ps(constant);
					for (size_t dimension = 0; dimension < dimensions; dimension += 8)
						_mm256_storeu_ps(vector + dimension, _mm256_fmadd_ps(_mm256_loadu_ps(operand.vector + dimension), factor, _mm256_loadu_ps(vector + dimension)));
				#endif
				}

			/*
				OBJECT::FUSED_SUBTRACT_DIVIDE()
				-------------------------------
				this += (operand - this) / constant
			*/
			void fused_subtract_divide(object &operand, float constant)
				{
				#ifdef __AVX512F__
					static_assert(false, "fused_subtract_divide() for AVX512 is not implemented yet");
				#else
					__m256 factor = _mm256_set1_ps(constant);
					for (size_t dimension = 0; dimension < dimensions; dimension += 8)
						{
						__m256 me = _mm256_loadu_ps(vector + dimension);
						__m256 answer = _mm256_add_ps(me, _mm256_div_ps(_mm256_sub_ps(_mm256_loadu_ps(operand.vector + dimension), me), factor));
						_mm256_storeu_ps(vector + dimension, answer);
						}
				#endif
				}

			/*
				OBJECT::UNITTEST()
				------------------
				Unit test this class
			*/
			static void unittest(void)
				{
				#ifdef __AVX512F__
					std::cout << "Using AVX-512\n";
				#else
					std::cout << "Using AVX2\n";
				#endif
				object initial(8);
				allocator memory;

				object *o1 = initial.new_object(&memory);
				object *o2 = initial.new_object(&memory);

				const float v1[] = {1, 2, 3, 4, 5, 6, 7, 8};
				const float v2[] = {9, 8, 7, 6, 5, 4, 3, 2};

				for (size_t loader = 0; loader < initial.dimensions; loader++)
					{
					o1->vector[loader] = v1[loader];
					o2->vector[loader] = v2[loader];
					}

				float sum = horizontal_sum(_mm256_loadu_ps(v1));
//std::cout << "Sum:" << sum << "\n";
				assert(sum == 36);


				float linear = o1->distance_squared_linear(o2);
				float simd = o1->distance_squared(o2);
//std::cout << "Linear:" << linear << " SIMD:" << simd << "\n";
				assert(simd == linear);


				*o1 += *o2;
//std::cout << "+=:" << o1 << "\n";
				assert(horizontal_sum(_mm256_loadu_ps(o1->vector)) == 80);


				*o1 /= 5;
//std::cout << "/=:" << o1 << "\n";
				assert(horizontal_sum(_mm256_loadu_ps(o1->vector)) == 16);


				o1->fused_multiply_add(*o1, 5);
//std::cout << "FMA():" << o1 << "\n";
				assert(horizontal_sum(_mm256_loadu_ps(o1->vector)) == 96);


				o1->zero();
//std::cout << "Zero():" << o1 << "\n";
				assert(horizontal_sum(_mm256_loadu_ps(o1->vector)) == 0);


				puts("object::PASS\n");
				}
		};

	/*
		OPERATOR<<()
		------------
	*/
	inline std::ostream &operator<<(std::ostream &stream, const object &thing)
		{
		for (size_t dimension = 0; dimension < thing.dimensions; dimension++)
			stream << thing.vector[dimension] << ' ';

		return stream;
		}
	}
