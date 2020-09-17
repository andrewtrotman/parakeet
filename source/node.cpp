/*
	NODE.CPP
	--------
	Copyright (c) 2020 Andrew Trotman
	Released under the 2-clause BSD license (See:https://en.wikipedia.org/wiki/BSD_licenses)
*/
#include <stdint.h>

#include <limits>
#include <iomanip>

#include "k_tree.h"
#include "node.h"

namespace k_tree
	{
	/*
		NODE::NODE()
		------------
	*/
	node::node() :
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
		answer->child = (new (memory->malloc(sizeof(node *) * (max_children + 1))) node *[max_children + 1]),
		answer->centroid = centroid->new_object(memory);

		if (first_child == nullptr)
			answer->leaves_below_this_point = answer->children = 0;
		else
			{
			answer->child[0] = first_child;
			answer->children = 1;
			}

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
		Correctly return the number of children at this node, under the knowledge that the
		node might be undergoing update at this very moment
	*/
	size_t node::number_of_children(void) const
		{
		size_t child_count = children;				// since this can change while this method is running is another thread updates the count
		/*
			There is a set of potential race conditions here that we need to take care of:
				children might have been updated, but child[children] might not have been updated yet
				the node is over-full and children > max_children
		*/
		if (child_count > max_children)
			child_count = max_children;

		if (child_count > 0 && child[child_count - 1] == nullptr)
			child_count--;

		return child_count;
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
		float min_distance = what->distance_squared(child[0]->centroid);

		/*
			Now check the distance to the others
		*/
		for (size_t which = 1; which < child_count; which++)
			{
			float distance = what->distance_squared(child[which]->centroid);
			if (distance < min_distance)
				{
				min_distance = distance;
				closest_child = which;
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

		leaves_below_this_point = 0;
		centroid->zero();
		for (size_t which = 0; which < child_count; which++)
			{
			leaves_below_this_point += child[which]->leaves_below_this_point;
			centroid->fused_multiply_add(*child[which]->centroid, child[which]->leaves_below_this_point);
			}

		*centroid /= leaves_below_this_point;
		}

	/*
		NODE::SPLIT()
		-------------
		Split this node into two new children - knowing that the node is full (i.e child[0..max_children] are all non-null)
	*/
	void node::split(allocator *memory, node **child_1_out, node **child_2_out) const
		{
		size_t place_in;
		size_t assignment[max_children + 1];
		size_t first_cluster_size;
		size_t second_cluster_size;
		float old_sum_distance = std::numeric_limits<float>::max();;
		float new_sum_distance = old_sum_distance / 2;

		/*
			allocate space
		*/
		object *centroid_1 = centroid->new_object(memory);
		object *centroid_2 = centroid->new_object(memory);
		node *child_1 = *child_1_out = new_node(memory, (node *)nullptr);
		node *child_2 = *child_2_out = new_node(memory, (node *)nullptr);

		/*
			Start with the first and last members of the current node.  It should really be 2 random elements, but close enough!
		*/
		*centroid_1 = *child[0]->centroid;
		*centroid_2 = *child[max_children]->centroid;

		/*
			The stopping condition is that the sum squared distance from the cluster centres has become constant (so no more shuffling can happen)
		*/
		while (old_sum_distance > (1.0 + float_resolution) * new_sum_distance)
			{
			old_sum_distance = new_sum_distance;
			new_sum_distance = 0;
			first_cluster_size = second_cluster_size = 0;
			for (size_t which = 0; which <= max_children; which++)
				{
				/*
					Compute the distance (squared) to each of the two new cluster centroids
				*/
				float distance_to_first = centroid_1->distance_squared(child[which]->centroid);
				float distance_to_second = centroid_2->distance_squared(child[which]->centroid);

				/*
					Choose a cluster, tie_break on the size of the cluster (put in the smallest to avoid empty clusters)
				*/
				if (distance_to_first == distance_to_second)
					place_in = first_cluster_size < second_cluster_size ? 0 : 1;
				else if (distance_to_first < distance_to_second)
					place_in = 0;
				else
					place_in = 1;

				/*
					Accumulate the stats for each new centroid (including the partial sum so that we can compute the centroid next)
				*/
				if (place_in == 0)
					{
					assignment[which] = 0;
					new_sum_distance += distance_to_first;
					first_cluster_size++;
					}
				else
					{
					assignment[which] = 1;
					new_sum_distance += distance_to_second;
					second_cluster_size++;
					}
				}

			/*
				Rebuild the centroids: first compute the sum
			*/
			centroid_1->zero();
			centroid_2->zero();
			for (size_t which = 0; which <= max_children; which++)
				{
				if (assignment[which] == 0)
					*centroid_1 += *child[which]->centroid;
				else
					*centroid_2 += *child[which]->centroid;
				}

			/*
				Rebuild then centroids: then average
			*/
			*centroid_1 /= first_cluster_size;
			*centroid_2 /= second_cluster_size;
			}

		/*
			At this point we have the new centroids and we have which node goes where in assignment[] so we populate the two new nodes
		*/
		for (size_t which = 0; which <= max_children; which++)
			if (assignment[which] == 0)
				{
				child_1->child[child_1->children] = child[which];
				child_1->children++;
				}
			else
				{
				child_2->child[child_2->children] = child[which];
				child_2->children++;
				}
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
			This can be guaranteed if the split count hasn't changed. If it has the we need to undo the slot assignment (atomically) and return re-try.
			We have the old split count stored in context->split_count and we want to increase the count in the tree the (effectively) take a lock
			if we're allowed, or fail if we're not allowed so we can use a compare-and-swap, all in the knowledge that since children is now equal to
			max_children, that all other split attempts failed.  So if compare-and-swap fails then we can undo the slot assignment and force a retry.

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
		auto another = context->split_count;
		another.end++;
		if (!context->tree->split_count.compare_exchange_strong(context->split_count, another))
			{
/*
FIX THIS:
Turn the above line into an assignment rather than a compare_exchange_stong()
*/
			puts("SOMETHING WENT WRONG - THE LOCK COUNT IS WRONG!!!!");
			exit(1);
			}
		/*
			We should update context->split.end, but there is no point as it will never be used again.
		*/
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
		size_t my_slot = children++;				// this is atomic so we get a guaranteed unique location. Between here and use we can guaraneett that child[children] == nullptr as child[] is initialised to 0

		if (my_slot < max_children)
			{
			/*
				We have a slot and we can fill it without a split
			*/
			auto got = node::new_node(context->memory, data);
// flush
std::atomic_thread_fence(std::memory_order_seq_cst);
			child[my_slot] = got;
			return result_success;
			}
		else if (my_slot > max_children)
			return result_retry;						// fail as we'll over-fill a leaf and another thread is already in line to split it
		else
			{
			/*
				Get the lock if we can (and signal a retry if we cannot
			*/
			if (!take_lock(context))
				{
				children--;					// free up the slot for someone else (or the retry that will happen)
// flush
std::atomic_thread_fence(std::memory_order_seq_cst);
				return result_retry;
				}

			/*
				At this point, any one in the tree who wants to split will fail because we have the lock.  So we can update the tree knownign that one one else can.
				But we don't actually update the tree, we do the split then tell the node above to update the tree

IT SHOULD BE POSSIBLE TO PUT THE LOCK AFTER THE SPLIT THEN FAIL - IT'LL WASTE MORE MEMORY, BUT MIGHT BE FASTER
			*/
			auto got = node::new_node(context->memory, data);
// flush
			std::atomic_thread_fence(std::memory_order_seq_cst);
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
		if (child[0]->isleaf())
			did_split = add_to_leaf(context, data, child_1, child_2);
		else
			{
			size_t best_child = closest(data);
			did_split = child[best_child]->add_to_node(context, data, child_1, child_2);
			if (did_split == result_split)
				{
				did_split = result_success;
				/*
					A split happened below, we must do a replace and add, and we have the lock
				*/
				child[best_child] = *child_1;
				child[children] = *child_2;
// flush
std::atomic_thread_fence(std::memory_order_seq_cst);
				children++;
				/*
					there is a squential consistency barrier here so that we know that child[children] has been updated before
					children was incremented - so all the pointers are valid.
				*/

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
			compute_mean();

		/*
			Return whether or not we caused a split, and therefore replacement is necessary
		*/
		return did_split;
		}

	/*
		NODE::TEXT_RENDER()
		-------------------
		Serialise the object in a human-readable format and down the given stream
	*/
	void node::text_render(std::ostream &stream) const
		{
		stream << children << ' ' << leaves_below_this_point << ' ' << *centroid << "\n";

		for (size_t who = 0; who < children; who++)
			child[who]->text_render(stream);
		}
	}

