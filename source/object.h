/*
	OBJECT.H
	--------
*/
#pragma once

#include <iostream>

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
			static object *new_object(void)
				{
				return new object();
				}

			/*
				OBJECT::DELET_OBJECT()
				--------------------
			*/
			static void delete_object(object *what)
				{
				delete what;
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
				OBJECT::OPERATOR/=()
				--------------------
			*/
			void operator/=(double constant)
				{
				for (size_t dimension = 0; dimension < DIMENSIONS; dimension++)
					vector[dimension] /= constant;
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
