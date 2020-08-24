/*
	OBJECT.H
	--------
*/
#pragma once

namespace k_tree
	{
	/*
		CLASS OBJECT
		------------
	*/
	template <size_t N>
	class object
		{
		public:
			double vector[N];

		public:
			/*
				OBJECT::DISTANCE_SQUARED()
				--------------------------
			*/
			double distance_squared(object &from) const
				{
				double total = 0;

				for (size_t dimension = 0; dimension < N; dimension++)
					total += (vector[dimension] - from.vector[dimension]) * (vector[dimension] - from.vector[dimension])

				return total;
				}
		}
	}

