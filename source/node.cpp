/*
	NODE.CPP
	--------
	Copyright (c) 2020 Andrew Trotman
	Released under the 2-clause BSD license (See:https://en.wikipedia.org/wiki/BSD_licenses)
*/
#include <math.h>
#include <stdint.h>

#include <thread>
#include <limits>
#include <iomanip>
#include <algorithm>

#include "k_tree.h"
#include "node.h"

namespace k_tree
	{
	/*
		NODE::NODE()
		------------
	*/
	node::node() noexcept :
		state(state_unsplit),
		max_children(0),
		children(0),
		child(nullptr),
		centroid(nullptr),
		leaves_below_this_point(1)
		{
		/* Nothing */
		}

	/*
		NODE::NEW_NODE()
		----------------
	*/
	node *node::new_node(allocator *memory, object *data) const
		{
		node *answer = new (memory->malloc(sizeof(node))) node();
		answer->max_children = max_children;
		answer->centroid = data;

		/*
			Make sure this has been done before proceeding.
		*/
		std::atomic_thread_fence(std::memory_order_seq_cst);

		return answer;
		}

	/*
		NODE::NEW_NODE()
		----------------
	*/
	node *node::new_node(allocator *memory, node *first_child) const
		{
		node *answer = new (memory->malloc(sizeof(node))) node();
		answer->max_children = max_children;
		size_t bytes = sizeof(*answer->child) * (max_children + 1);
		answer->child = new (memory->malloc(bytes)) std::atomic<node *>[max_children + 1];
		memset(answer->child, 0, bytes);
		answer->centroid = centroid->new_object(memory);

		if (first_child == nullptr)
			answer->leaves_below_this_point = answer->children = 0;
		else
			{
			answer->child[0] = first_child;
			answer->children = 1;
			}

		/*
			Make sure this has been done before proceeding.
		*/
		std::atomic_thread_fence(std::memory_order_seq_cst);

		return answer;
		}

	/*
		NODE::ISLEAF()
		--------------
		Return whether or not this node is a leaf
	*/
	bool node::isleaf(void) const
		{
		return child == nullptr;
		}

	/*
		NODE::NUMBER_OF_CHILDREN()
		--------------------------
		Correctly return the number of children at this node, under the knowledge that the node might be undergoing update at this very moment
	*/
	size_t node::number_of_children(void) const
		{
		size_t child_count = children;				// since this can change while this method is running is another thread updates the count
		return std::min(child_count, max_children);
		}

	/*
		NODE::CLOSEST()
		---------------
		Returns the index of the child closest to the parameter what
	*/
	size_t node::closest(object *what) const
		{
		size_t child_count = number_of_children();

		/*
			Initialise to the distance to the first element in the list
		*/
		size_t closest_child = 0;
		float min_distance = what->distance_squared(child[0].load()->centroid);

		/*
			Now check the distance to the others
		*/
		for (size_t which = 1; which < child_count; which++)
			{
			/*
				Make sure child[which] has been written to (because children might be updated but child[which] has not yet been updated
			*/
			if (child[which].load() != nullptr)
				{
				float distance = what->distance_squared(child[which].load()->centroid);
				if (distance < min_distance)
					{
					min_distance = distance;
					closest_child = which;
					}
				}
			}

		return closest_child;
		}

	/*
		NODE::COMPUTE_MEAN()
		--------------------
		Compute the centroid of the children below this node.  Note that this must be a weighted average as not all branched have the
		same number of children and this resulting centroid should be the middle of the leaves not the middle of the children.  This also
		recomputes that count from the children (which might have recently changed)
	*/
	void node::compute_mean(void)
		{
		size_t child_count = number_of_children();
		size_t new_leaf_count = 0;

		centroid->zero();
		for (size_t which = 0; which < child_count; which++)
			{
			auto current_child = child[which].load();
			if (current_child != nullptr)					// Make sure child[which] has been written to (because children might be updated but child[which] has not yet been updated
				{
				new_leaf_count += current_child->leaves_below_this_point;
				centroid->fused_multiply_add(*current_child->centroid, (float)current_child->leaves_below_this_point);
				}
			}

		leaves_below_this_point = new_leaf_count;
		*centroid /= (float)new_leaf_count;
		}

	/*
		NODE::SPLIT()
		-------------
		Split this node into two new children - knowing that the node is full (i.e child[0..max_children] are all non-null)
		Returns:
			True on success (the data was split into 2 clusters)
			Fase on failure (the data all ended up in one cluster)

		For improvements on standard k-means see:
			C. C. Aggarwal, A. Hinneburg, D. A. Keim (2001), On the Surprising Behavior of Distance Metrics in High Dimensional Space, ICDT 2001
			C. Elkan (2003) Using the Triangle Inequality to Accelerate k-Means, ICML-2003
	*/
	bool node::split(allocator *memory, node **child_1_out, node **child_2_out, size_t initial_first_cluster) const
		{
		size_t cluster_size[2];
		float old_sum_distance = std::numeric_limits<float>::max();
		float new_sum_distance = old_sum_distance / 2;

		/*
			Each point gets an assignment to a data point (kept in assignment[]).  If we keep the distance to
			that when we compute it, then we can compute the worst posible case as the last computed distance plus
			any shift in that centroid (kept in distance_to_assignment[]).  All points are initially assigned to cluster 0.
			Each time we move the center we keep the delta (in delta[0] and delta[1]), and add that to the estimated distance
			each time.
		*/
#ifdef _MSC_VER
		size_t *assignment = (size_t *)alloca(sizeof(*assignment) * (max_children + 1));
		float *distance_to_assignment = (float *)alloca(sizeof(*distance_to_assignment) * (max_children + 1));		// this is an over estimate of the distance (i.e d <= distance_to_other)
		float *distance_to_other = (float *)alloca(sizeof(*distance_to_other) * (max_children + 1));			// this is and under estimate of the distance (i.e d >= distance_to_other)
#else
		size_t assignment[max_children + 1];
		float distance_to_assignment[max_children + 1];		// this is an over estimate of the distance (i.e d <= distance_to_other)
		float distance_to_other[max_children + 1];			// this is and under estimate of the distance (i.e d >= distance_to_other)
#endif

		for (size_t which = 0; which <= max_children; which++)
			{
			assignment[which] = 0;
			distance_to_assignment[which] = std::numeric_limits<float>::max();
			distance_to_other[which] = 0;
			}

		/*
			Allocate space and initialiase
		*/
		object *centroid[2];
		object *new_centroid[2];
		node *child[2];
		float delta[2];
		child[0] = *child_1_out = new_node(memory, (node *)nullptr);
		child[1] = *child_2_out = new_node(memory, (node *)nullptr);
		for (size_t which = 0; which < 2; which++)
			{
			centroid[which] = this->centroid->new_object(memory);
			new_centroid[which] = this->centroid->new_object(memory);
			delta[which] = 0;
			}

		/*
			Start with the first member, then find the furthest away member and use that as the second point
		*/
		*centroid[0] = *this->child[initial_first_cluster].load()->centroid;

		size_t best_choice = 1;
		double smallest_distance = centroid[0]->distance_squared(this->child[1].load()->centroid);
		for (size_t which = 2; which <= max_children; which++)
			{
			float distance = centroid[0]->distance_squared(this->child[which].load()->centroid);
			if (distance < smallest_distance)
				{
				best_choice = which;
				smallest_distance = distance;
				}
			}
		*centroid[1] = *this->child[best_choice].load()->centroid;

		/*
			The stopping condition is that the sum squared distance from the cluster centres has become constant (so no more shuffling can happen)
		*/
		while (old_sum_distance > (1.0 + float_resolution) * new_sum_distance)
			{
			/*
				Compute the distance between the centroids
				Lemma 1 from C. Elkan (2003) Using the Triangle Inequality to Accelerate k-Means, ICML-2003
				if d(c1,c2) >= 2d(x,c1) then d(x,c2) > d(x,c1)
				in other words, if the distance from the data point to the first cluster is less than half the distance between the clusters then the first cluster must be the closest
			*/
			float half_distance_between_centroids = sqrt(centroid[0]->distance_squared(centroid[1])) / (float)2.0;
			half_distance_between_centroids *= half_distance_between_centroids;

			old_sum_distance = new_sum_distance;
			new_sum_distance = 0;
			for (size_t which = 0; which < 2; which++)
				cluster_size[which] = 0;
			for (size_t which = 0; which <= max_children; which++)
				{
				/*
					Compute the distance (squared) to each of the two new cluster centroids
				*/
				size_t other = assignment[which] == 0 ? 1 : 0;
				distance_to_other[which] -= delta[assignment[other]];					// the max we might have moved this cluster towards the point
				distance_to_assignment[which] += delta[assignment[which]];			// the max we might have moved this cluster away from the point
				if (distance_to_assignment[which] < distance_to_other[which])
					cluster_size[assignment[which]]++;		// we're closer to one than the other
				else if (distance_to_assignment[which] < half_distance_between_centroids)
					cluster_size[assignment[which]]++;		// we're less then halfway between the two centroids despire the bounds crossing
				else
					{
					distance_to_assignment[which] = centroid[assignment[which]]->distance_squared(this->child[which].load()->centroid);

					if (distance_to_assignment[which] >= half_distance_between_centroids)
						distance_to_other[which] = centroid[other]->distance_squared(this->child[which].load()->centroid);
					else if (distance_to_assignment[which] >= distance_to_other[which])
						distance_to_other[which] = centroid[other]->distance_squared(this->child[which].load()->centroid);

					/*
						Choose a cluster, tie_break on the size of the cluster (put in the smallest in an attempt to avoid empty clusters)
					*/
					if (distance_to_assignment[which] > distance_to_other[which])
						{
						assignment[which] = other;
						std::swap(distance_to_assignment[which], distance_to_other[which]);
						}
					else if (distance_to_assignment[which] == distance_to_other[which])
						{
						size_t place_in = cluster_size[0] < cluster_size[1] ? 0 : 1;

						assignment[which] = place_in;
						if (place_in != assignment[which])
							{
							assignment[which] = other;
							std::swap(distance_to_assignment[which], distance_to_other[which]);
							}
						}

					cluster_size[assignment[which]]++;
					new_sum_distance += distance_to_assignment[which];
					}
				}

			/*
				Build the new centroids: first compute the sum
			*/
			for (size_t which = 0; which < 2; which++)
				new_centroid[which]->zero();

			for (size_t which = 0; which <= max_children; which++)
				*new_centroid[assignment[which]] += *this->child[which].load()->centroid;

			/*
				Build the centroids: then average
			*/
			for (size_t which = 0; which < 2; which++)
				*new_centroid[which] /= (float)cluster_size[which];

			/*
				Compute the centroid shifts
			*/
			for (size_t which = 0; which < 2; which++)
				delta[which] = new_centroid[which]->distance_squared(centroid[which]);

			/*
				Use the new centroids
			*/
			for (size_t which = 0; which < 2; which++)
				std::swap(centroid[which], new_centroid[which]);
			}

		/*
			At this point we have the new centroids and we have which node goes where in assignment[] so we populate the two new nodes
		*/
		for (size_t which = 0; which <= max_children; which++)
			{
			child[assignment[which]]->child[child[assignment[which]]->children] = this->child[which].load();
			child[assignment[which]]->children++;
			}

		return cluster_size[0] != 0 && cluster_size[1] != 0;			// return true if we are able to split into two clusters, or false if everyhting ended up in one cluster.
		}

	/*
		NODE::SPLIT()
		-------------
		Split this node into two new children - knowing that the node is full (i.e child[0..max_children] are all non-null)
		Returns:
			True on success (the data was split into 2 clusters)
			Fase on failure (the data all ended up in one cluster)
	*/
	bool node::split(allocator *memory, node **child_1_out, node **child_2_out) const
		{
		/*
			Before we do a split we need to make sure all other processes that are writing to the child[] array of this node have completed.  This can happen
			because childen is incremented by two threads, and the second tried to do the split before the first updates the pointer in the child[] array.  To
			catch this we spin until the array is full (i.e. contains no nullptr values)
		*/
		for (auto p = child; p < child + max_children; p++)
			while (*p == nullptr)
				{
				/* Spinlock as we wait for all the threads to finish writing into child[] */
				}

		if (!split(memory, child_1_out, child_2_out, 0))
			{
			/*
				We failed to split. This might be because the node consists of vectors that are identical, or it might be
				that the vectors have all been moving while we did the clistering so we ended up with everything in one
				cluster.  Assuming the former, we'll split the clister into two equal sized clusters by alternating which
				cluster we put each child into.  An alternative is to call split() with a different initial starting
				cluster (i.e. not 0).  Or we chould check to see if we're all the same vector and if so split randomly.
			*/
			(*child_1_out)->children = 0;
			(*child_2_out)->children = 0;
			for (size_t which = 0; which <= max_children; which++)
				if ((which & 1) == 0)
					{
					(*child_1_out)->child[(*child_1_out)->children] = child[which].load();
					(*child_1_out)->children++;
					}
				else
					{
					(*child_2_out)->child[(*child_2_out)->children] = child[which].load();
					(*child_2_out)->children++;
					}
			}
		return true;
		}

	/*
		NODE::TAKE_LOCK()
		-----------------
		Take the split lock (signal that we want to do a split).
		return true is we get the lock, false otherwise (and so result_retry should happen)
	*/
	bool node::take_lock(context *context)
		{
		/*
			We're only alowed to split if the return path hasn't changed.
			This can be guaranteed if the split count hasn't changed. If it has the we need to re-try.
			We have the old split count stored in context->split_count and we want to increase the count in the tree the (effectively) take a lock
			if we're allowed, or fail if we're not allowed so we can use a compare-and-swap, all in the knowledge that since children is now equal to
			max_children, that all other split attempts failed.  So if compare-and-swap fails then we force a retry.

			Note that the split_count is counting two things, entry and exit from the split operaiton.  This is because a thread might start a split and then get
			swapped out.  A second thread enters, gets the split count, notices a split is needed, does the split and exits all while the first thread is
			swapped out.  So on entry we get the split count, which is two counts (begin_split and end_split).  Once we start a split by incrementing
			begin_split all other threads will either see that the split count has incremented or that another thread is doing the split because the two
			counts are not the same and so a split happened while the second thread was in the tree.
		*/
		if (context->split_count.begin != context->split_count.end)
			{
			/*
				A split was happening when we entered the tree so we fail. This is because our return path might now be invalid an so we might not
				be able to update somewhere in the tree (that is on the return path from the recursion)
			*/
			return false;
			}

		/*
			Increase the split count start
		*/
		auto another = context->tree->split_count.load();
		another.begin++;

		/*
			Make sure no one else has split in the mean time.
			If our copy of the split count doesn't match that of the tree the someone else did a split so we retry.
			Otherwise we take the lock and we get to split.

			Note that this will do a CMPXCHG16B as sizeof(another) == 16
		*/
		if (!context->tree->split_count.compare_exchange_strong(context->split_count, another))
			return false;

		/*
			Update our copy of the split count - so that a compare_exchange() will work when we release the lock
		*/
		context->split_count.begin++;

		return true;
		}

	/*
		NODE::RELEASE_LOCK()
		--------------------
		Release the split lock
	*/
	void node::release_lock(context *context)
		{
		context->split_count.end++;
		context->tree->split_count.store(context->split_count);
		}

	/*
		NODE::ADD_TO_LEAF()
		-------------------
		Add the given data to the current leaf node.
		Returns:
			result_success: The data was added to the leaf
			result_retry: The data failed to be added to the tree because a split is in action
			result_split: The node above must do a replacement and an add
	*/
	node::result node::add_to_leaf(context *context, object *data, node **child_1, node **child_2)
		{
		/*
			Get a slot in the leaf node.
			There are three possible outcomes
				1. a value less than max_children (in which case we can put the object there) as no splitting going on
				2. a slot larger than max_children (in which case someone else wants to split this node so we return result_retry to signal that the thread should try again)
				3. the max_children slot (in which case we split)
			In case 3, we split into two new nodes.  We do not need to lock the leaf because all subsequent
			add attempts will fail as the node is full. Since we never discard the node, the node will always be full
			so any threads currently in the tree (and that end up here) will fail until the tree is re-structured.
			The tree update for a split happens at the level above, so in this case we do the grunt work here
			and return result_split to tell the level above to update the tree
		*/
		size_t my_slot = children++;				// this is atomic so we get a guaranteed unique location. Between here and use we can guarantee that child[children] == nullptr as child[] is initialised to 0

		if (my_slot < max_children)
			{
			/*
				We have a slot and we can fill it without a split
			*/
			auto got = node::new_node(context->memory, data);
			child[my_slot] = got;
			return result_success;
			}
		else
			{
			/*
				State an intention to split this node
			*/
			split_state current_state = state_unsplit;
			if (!state.compare_exchange_strong(current_state, state_split))
				return result_retry;						// Someone else is already splitting (or has split) this node so retry the insertion

			/*
				Get the lock if we can (and signal a retry if we cannot)
			*/
			if (!take_lock(context))
				{
				/*
					We have the lock on the current node, so release that and signal a retry
				*/
				state = state_unsplit;
				return result_retry;
				}

			/*
				At this point the node has never been split and we have permission to split it.
				We also know that the last slot has never been written into and so its ours
			*/
			my_slot = max_children;
			auto got = node::new_node(context->memory, data);
			child[my_slot] = got;

			split(context->memory, child_1, child_2);
			(*child_1)->compute_mean();
			(*child_2)->compute_mean();

			return result_split;
			}
		}

	/*
		NODE::ADD_TO_NODE()
		-------------------
		Add the given data to the current tree at or below this point.
		Returns:
			result_success: The data was added to the tree (here or below) successfully
			result_retry: The data failed to be added to the tree because a split is in action
			result_split: The node above must do a replacement and an add

		NOTE: if retry_split was returned then we have the lock and must unlock once the split has finished propegating
	*/
	node::result node::add_to_node(context *context, object *data, node **child_1, node **child_2)
		{
		node::result did_split = result_success;
		if (child[0].load()->isleaf())
			did_split = add_to_leaf(context, data, child_1, child_2);
		else
			{
			size_t best_child = closest(data);
			did_split = child[best_child].load()->add_to_node(context, data, child_1, child_2);
			if (did_split == result_split)
				{
				did_split = result_success;
				/*
					A split happened below, we must do a replace and add, remember that we already have the lock
				*/
				child[best_child] = *child_1;
				child[children] = *child_2;
				children++;

				if (children > max_children)
					{
					split(context->memory, child_1, child_2);
					(*child_1)->compute_mean();
					(*child_2)->compute_mean();
					did_split = result_split;
					/*
						Keep the lock.  It will be released by the next (or higher) node up the tree.
					*/
					}
				else
					release_lock(context);
				}
			}

		/*
			Update the mean for the current node as data has been added somewhere below here.  But only if we added to the node (i.e. not on a retry).
		*/
		if (did_split != result_retry)
			{
			/*
				We can avoid re-calculating the mean by computing the delta (the amount it shifts) and adding that instead.  The maths (thanks to Shlomo)
				is below, where M is the current mean, X is the new point, and n is the number of points alreay in the mean
					M + delta = (X + n * M) / (n + 1)
					delta = (X + n * M) / (n + 1) – M
					delta = ((X + n * M) - (n + 1) * M) /  (n + 1)
					delta = (X - M) / (n + 1)
				in the context of this method:
					centroid += (data - centroid) / (leaves_below_this_point + 1)
					leaves_below_this_point++;
				Note that there is an accumulation of rounding errors.  If you want to compute the mean at each node each time then use this line instead:
					compute_mean();
			*/
			centroid->fused_subtract_divide(*data, (float)(leaves_below_this_point + 1));
			leaves_below_this_point++;
			}

		/*
			Return whether or not we caused a split, and therefore replacement is necessary
		*/
		return did_split;
		}

	/*
		NODE::NORMALISE_COUNTS()
		------------------------
		Fix the broken leaved-below counts that happened because of parallel updates
	*/
	void node::normalise_counts(void)
		{
		/*
			Correct the counts in the children
		*/
		size_t child_count = children.load() < max_children ? children.load() : max_children;
		for (size_t who = 0; who < child_count; who++)
			child[who].load()->normalise_counts();

		/*
			Add up the counts here
		*/
		if (isleaf())
			leaves_below_this_point = 1;
		else
			{
			leaves_below_this_point = 0;
			for (size_t who = 0; who < child_count; who++)
				leaves_below_this_point += child[who].load()->leaves_below_this_point;
			}
		}

	/*
		NODE::DESERIALISE()
		-------------------
	*/
	node *node::deserialise(allocator &memory, std::istream &stream, object &example_object)
		{
		size_t new_children;
		size_t new_leaves_below_this_point;

		/*
			Get the number of children and the number of leaves below this point
		*/
		stream >> new_children;
		stream >> new_leaves_below_this_point;

		/*
			If we're a leaf node (we have no children) then create a leaf and return it
		*/
		if (new_children == 0)
			{
			object *new_vector = example_object.new_object(&memory);
			for (size_t dimension = 0; dimension < example_object.dimensions; dimension++)
				stream >> new_vector->vector[dimension];
			node * answer = new_node(&memory, new_vector);

			return answer;
			}

		node *answer = new_node(&memory, (node *)nullptr);
		answer->children = new_children;
		answer->leaves_below_this_point = new_leaves_below_this_point;

		/*
			Else, we're a internal node
		*/
		for (size_t dimension = 0; dimension < example_object.dimensions; dimension++)
			stream >> answer->centroid->vector[dimension];

		for (size_t which_child = 0; which_child < new_children; which_child++)
			answer->child[which_child] = deserialise(memory, stream, example_object);

		return answer;
		}

	/*
		NODE::TEXT_RENDER()
		-------------------
		Serialise the object in a human-readable format and down the given stream
	*/
	std::ostream &node::text_render(std::ostream &stream) const
		{
		stream << (children.load() < max_children ? children.load() : max_children) << ' ' << leaves_below_this_point << ' ' << *centroid << "\n";

		size_t child_count = children.load() < max_children ? children.load() : max_children;
		for (size_t who = 0; who < child_count; who++)
			child[who].load()->text_render(stream);

		return stream;
		}

	/*
		NODE::TEXT_RENDER_PENULTIMATE()
		-------------------------------
		Dump the level above the leaves (the bottom-level clusters)
	*/
	std::ostream &node::text_render_penultimate(std::ostream &stream) const
		{
		if (children != 0)
			{
			if (child[0].load()->isleaf())
				stream << (children.load() < max_children ? children.load() : max_children) << ' ' << leaves_below_this_point << ' ' << *centroid << "\n";

			size_t child_count = children.load() < max_children ? children.load() : max_children;
			for (size_t who = 0; who < child_count; who++)
				child[who].load()->text_render_penultimate(stream);
			}

		return stream;
		}

	/*
		NODE::TEXT_RENDER_MOVIE()
		-------------------------
		Serialise the object in a human-readable format and down the given stream.  The first character is a symbol representing the depth of the tree, then the coordinate
	*/
	std::ostream &node::text_render_movie(std::ostream &stream, uint32_t depth) const
		{
		static const char *symbols = "+x*o^dsphv><";
		static uint32_t last_symbol = sizeof(symbols);

		if (isleaf())
			stream << '.';
		else if (depth < last_symbol)
			stream << symbols[depth];
		else
			stream << symbols[last_symbol];

		stream << ' ' << *centroid << "\n";

		size_t child_count = children.load() < max_children ? children.load() : max_children;
		for (size_t who = 0; who < child_count; who++)
			child[who].load()->text_render_movie(stream, depth + 1);

		return stream;
		}
	}
