/*
	OBJECT.H
	--------
	Copyright (c) 2020 Andrew Trotman
	Released under the 2-clause BSD license (See:https://en.wikipedia.org/wiki/BSD_licenses)
*/
#pragma once

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
		static_assert(sizeof(float) == 4);			// floats must be 32-bit for the SIMD code to work

		public:
			static const int DIMENSIONS = 8;			// the number of dimensions of the float
			float vector[DIMENSIONS];					// this object is simply a vector of floats (with SIMD single precision for operations)

		public:
			/*
				OBJECT::OBJECT()
				----------------
				Constructor
			*/
			object()
				{
				/* Nothing */
				}

			/*
				OBJECT::OBJECT()
				----------------
				Copy constructor
			*/
			object(object &with)
				{
				memcpy(vector, with.vector, sizeof(vector));
				}

			/*
				OBJECT::NEW_OBJECT()
				--------------------
				Return a new object placement-constructed in memory provided by the allocator
			*/
			static object *new_object(allocator &allocator)
				{
				return new ((object *)allocator.malloc(sizeof(object))) object();
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
				__m256 bottom = _mm256_bsrli_epi128(elements, 4);
				elements = _mm256_add_ps(elements, bottom);

				/*
					shift left by 2 integers and add
					H0 GH FG EF D0 CD BC AB
					00 00 H0 GH 00 00 D0 CD
				*/
				bottom = _mm256_bsrli_epi128(elements, 8);
				elements = _mm256_add_ps(elements, bottom);
				/*
					We have: H000 GH00 EFGH D000 CD00 BCD0 ABCD
				*/

				/*
					shuffle to get: EFGH EFGH EFGH EFGH ABCD ABCD ABCD ABCD
					permute to get: 0000 0000 0000 0000 EFGH EFGH EFGH EFGH
				*/
				__m256 missing = _mm256_shuffle_epi32(elements, _MM_SHUFFLE(0, 0, 0, 0));
				missing = _mm256_permute2x128_si256(_mm256_setzero_si256(), missing, 3);

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
			static float distance_squared(const object *a, const object *b)
				{
				float total = 0;

				for (size_t dimension = 0; dimension < DIMENSIONS; dimension += 8)
					{
					__m256 diff = _mm256_sub_ps(_mm256_loadu_ps(a->vector + dimension), _mm256_loadu_ps(b->vector + dimension));
					__m256 result = _mm256_mul_ps(diff, diff);
					total += horizontal_sum(result);
					}

				return total;
				}

			/*
				OBJECT::DISTANCE_SQUARED_LINEAR()
				---------------------------------
				Return the square of the Euclidean distance between parameters a and b without using SIMD operations
			*/
			static double distance_squared_linear(const object *a, const object *b)
				{
				double total = 0;

				for (size_t dimension = 0; dimension < DIMENSIONS; dimension++)
					total += (a->vector[dimension] - b->vector[dimension]) * (a->vector[dimension] - b->vector[dimension]);

				return total;
				}

			/*
				OBJECT::ZERO()
				--------------
				Set all elements in the vector to zero
			*/
			void zero()
				{
				memset(vector, 0, sizeof(vector));
				}

			/*
				OBJECT::OPERATOR+=()
				--------------------
				this += operand
			*/
			void operator+=(const object &operand)
				{
				for (size_t dimension = 0; dimension < DIMENSIONS; dimension += 8)
					_mm256_storeu_ps(vector + dimension, _mm256_add_ps(_mm256_loadu_ps(vector + dimension), _mm256_loadu_ps(operand.vector + dimension)));
				}

			/*
				OBJECT::OPERATOR/=()
				--------------------
				this /= operand
			*/
			void operator/=(float constant)
				{
				__m256 divisor = _mm256_set1_ps(constant);

				for (size_t dimension = 0; dimension < DIMENSIONS; dimension += 8)
					_mm256_storeu_ps(vector + dimension, _mm256_div_ps(_mm256_loadu_ps(vector + dimension), divisor));
				}

			/*
				OBJECT::FUSED_MULTIPLY_ADD()
				----------------------------
				this += operand * constant
			*/
			void fused_multiply_add(object &operand, float constant)
				{
				__m256 factor = _mm256_set1_ps(constant);
				for (size_t dimension = 0; dimension < DIMENSIONS; dimension += 8)
					_mm256_storeu_ps(vector + dimension, _mm256_fmadd_ps(_mm256_loadu_ps(operand.vector + dimension), factor, _mm256_loadu_ps(vector + dimension)));
				}

			/*
				OBJECT::UNITTEST()
				------------------
				Unit test this class
			*/
			static void unittest(void)
				{
				object o1;
				object o2;
				const float v1[] = {1, 2, 3, 4, 5, 6, 7, 8};
				const float v2[] = {9, 8, 7, 6, 5, 4, 3, 2};

				for (size_t loader = 0; loader < DIMENSIONS; loader++)
					{
					o1.vector[loader] = v1[loader];
					o2.vector[loader] = v2[loader];
					}

				float sum = horizontal_sum(_mm256_loadu_ps(v1));
//std::cout << "Sum:" << sum << "\n";
				assert(sum == 36);


				float linear = distance_squared_linear(&o1, &o2);
				float simd = distance_squared(&o1, &o2);
//std::cout << "Linear:" << linear << " SIMD:" << simd << "\n";
				assert(simd == linear);


				o1 += o2;
//std::cout << "+=:" << o1 << "\n";
				assert(horizontal_sum(_mm256_loadu_ps(o1.vector)) == 80);


				o1 /= 5;
//std::cout << "/=:" << o1 << "\n";
				assert(horizontal_sum(_mm256_loadu_ps(o1.vector)) == 16);


				o1.fused_multiply_add(o1, 5);
//std::cout << "FMA():" << o1 << "\n";
				assert(horizontal_sum(_mm256_loadu_ps(o1.vector)) == 96);


				object o3 = o1;
//std::cout << "CopyConstruct:" << o3 << "\n";
				assert(horizontal_sum(_mm256_loadu_ps(o3.vector)) == 96);


				o1.zero();
//std::cout << "Zero():" << o1 << "\n";
				assert(horizontal_sum(_mm256_loadu_ps(o1.vector)) == 0);


				puts("object::PASS\n");
				}
		};

	/*
		OPERATOR<<()
		------------
	*/
	inline std::ostream &operator<<(std::ostream &stream, const object &thing)
		{
		stream << "[ ";
		stream << thing.vector[0];
		for (size_t dimension = 1; dimension < thing.DIMENSIONS; dimension++)
			stream << ',' << thing.vector[dimension];
		stream << " ]";

		return stream;
		}
	}
