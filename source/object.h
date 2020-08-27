/*
	OBJECT.H
	--------
*/
#pragma once

#include <immintrin.h>

#include <iostream>

#include "allocator.h"

namespace k_tree
	{
	/*
		CLASS OBJECT
		------------
	*/
	class object
		{
		public:
			static const int DIMENSIONS = 2;
			double vector[DIMENSIONS];

		public:
			object()
				{
				/* Nothing */
				}

			/*
				OBJECT::OBJECT()
				----------------
				copy constructor
			*/
			object(object &with)
				{
				memcpy(vector, with.vector, sizeof(vector));
				}

			/*
				OBJECT::NEW_OBJECT()
				--------------------
			*/
			static object *new_object(allocator &allocator)
				{
				return new ((object *)allocator.malloc(sizeof(object))) object();
				}

			/*
				OBJECT::CUMULATIVE_SUM_1()
				--------------------------
			*/
			static double cumulative_sum_1(__m256d elements)
				{
				/*
					shift left by 1 value and add
					D C B A
					0 D 0 B
				*/
				__m256d bottom = _mm256_bsrli_epi128(elements, 8);
				elements = _mm256_add_pd(elements, bottom);

				/*
					we alread got :D0 CD B0 AB
					permute to get:CD CD CD CD
				*/
				__m256d missing = _mm256_permute2x128_si256(elements, elements, 1);
				/*
					add
					D0 CD B0 AB
					CD CD CD CD
					leaves ABCD in the bottom word, which we can then extract
				*/
				__m256d answer = _mm256_add_pd(elements, missing);

				return _mm256_cvtsd_f64(answer);
				}

			/*
				OBJECT::CUMULATIVE_SUM()
				------------------------
			*/
			static double cumulative_sum(__m256d elements)
				{
				/*
					Extract the low and high 128-bit parts giving DC and BA
				*/
				__m128d high = _mm256_extractf128_pd(elements, 1);
				__m128d low  = _mm256_castpd256_pd128(elements);

				/*
					Add
					D C
					B A
				*/
				__m128d sum  = _mm_add_pd(low, high);

				/*
					Extract the high word (DB) and broadcast giving DB DB
				*/
				high = _mm_unpackhi_pd(sum, sum);

				/*
					Add the broadcast
					DB DB
					DB AC
					giving the result in the low word
				*/
				sum = _mm_add_sd(sum, high);

				/*
					Extract the low word
				*/
				return  _mm_cvtsd_f64(sum);
				}

			/*
				OBJECT::DISTANCE_SQUARED_SIMD()
				-------------------------------
			*/
			static double distance_squared_simd(const object &a, const object &b)
				{
				double total = 0;

				for (size_t dimension = 0; dimension < DIMENSIONS; dimension += 4)
					{
					__m256d a_vec = _mm256_loadu_pd(&a.vector[dimension]);
					__m256d b_vec = _mm256_loadu_pd(&b.vector[dimension]);
					__m256d diff = _mm256_sub_pd(a_vec, b_vec);
					__m256d result = _mm256_mul_pd(diff, diff);
					total += cumulative_sum(result);
					}

				return total;
				}

			/*
				OBJECT::DISTANCE_SQUARED()
				--------------------------
			*/
			static double distance_squared(const object &a, const object &b)
				{
				double total = 0;

				for (size_t dimension = 0; dimension < DIMENSIONS; dimension++)
					total += (a.vector[dimension] - b.vector[dimension]) * (a.vector[dimension] - b.vector[dimension]);

				return total;
				}

			/*
				OBJECT::DISTANCE_SQUARED()
				--------------------------
			*/
			static double distance_squared(const object *a, const object *b)
				{
				return distance_squared(*a, *b);
				}

			/*
				OBJECT::ZERO()
				--------------
			*/
			void zero()
				{
				memset(vector, 0, sizeof(vector));
				}

			/*
				OBJECT::OPERATOR+=()
				--------------------
			*/
			void operator+=(const object &operand)
				{
				for (size_t dimension = 0; dimension < DIMENSIONS; dimension++)
					vector[dimension] += operand.vector[dimension];
				}

			/*
				OBJECT::OPERATOR+=()
				--------------------
			*/
			void operator+=(const object *operand)
				{
				for (size_t dimension = 0; dimension < DIMENSIONS; dimension++)
					vector[dimension] += operand->vector[dimension];
				}

			/*
				OBJECT::OPERATOR/=()
				--------------------
			*/
			void operator/=(double constant)
				{
				for (size_t dimension = 0; dimension < DIMENSIONS; dimension++)
					vector[dimension] /= constant;
				}

			/*
				OBJECT::FUSED_MULTIPLY_ADD()
				----------------------------
			*/
			void fused_multiply_add(object *operand, double factor)
				{
				for (size_t dimension = 0; dimension < DIMENSIONS; dimension++)
					vector[dimension] += operand->vector[dimension] * factor;
				}

			/*
				OBJECT::UNITTEST()
				------------------
			*/
			static void unittest(void)
				{
				object o1;
				object o2;
				const double v1[] = {1.1, 2,2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8};
				const double v2[] = {9.9, 8.8, 7.7, 6.6, 5.5, 4.4, 3.3, 2.2};

				for (size_t loader = 0; loader < DIMENSIONS; loader++)
					{
					o1.vector[loader] = v1[loader];
					o2.vector[loader] = v2[loader];
					}

				double linear = distance_squared(o1, o2);
				double simd = distance_squared_simd(o1, o2);

				std::cout << "Linear:" << linear << " SIMD:" << simd << "\n";
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
