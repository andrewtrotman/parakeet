/*
	K_TREE_EXAMPLE.CPP
	------------------
	Copyright (c) 2020 Andrew Trotman
	Released under the 2-clause BSD license (See:https://en.wikipedia.org/wiki/BSD_licenses)
*/
#include <atomic>
#include <thread>
#include <fstream>
#include <sstream>
#include <iostream>
#include <unordered_map>

#include "disk.h"
#include "timer.h"
#include "k_tree.h"
#include "fast_double_parser.h"

/*
	CLASS JOB
	---------
	An individual piexe of work that needs to be done (i.e. added to the tree)
*/
class job
	{
	public:
		std::atomic<uint8_t> has_been_processed;			// this is true if a thread has taken responsibility for adding this to the tree, else false
		k_tree::object *vector;									// this is the vector to add

	public:
		job() = delete;

		/*
			JOB::JOB()
			----------
			Create a workload object ready for insertion into the list of work to be done
		*/
		job(k_tree::object *vector) :
			has_been_processed(false),
			vector(vector)
			{
			/* Nothing */
			}

		/*
			JOB::JOB()
			----------
			Move Constructor
		*/
		job(job &&original) :
			has_been_processed(original.has_been_processed.load()),
			vector(original.vector)
			{
			/*
				Invalidate the original object.
			*/
			original.has_been_processed = true;
			original.vector = nullptr;
			}
	};

/*
	CLASS WORKER()
	--------------
*/
class worker
	{
	public:
		k_tree::allocator memory;
		k_tree::k_tree *tree;
		std::vector<job*> *work_list;
		bool movie_mode;
		const char *outfilename;

	public:
		worker(k_tree::k_tree *tree, std::vector<job*> *work_list, bool movie_mode, const char *outfilename) :
			tree(tree),
			work_list(work_list),
			movie_mode(movie_mode),
			outfilename(outfilename)
			{
			/* Nothing */
			}
	};

/*
	THREAD_WORK()
	-------------
	Entry point for each thread.  The work is to add some nodes from vector_list to tree
*/
void thread_work(worker *parameters)
	{
	size_t end = parameters->work_list->size();
	size_t index = 0;

	while (index < end)
		{
		job *task = (*parameters->work_list)[index];
		if (task->has_been_processed)
			index++;
		else
			{
			uint8_t expected = false;
			if (task->has_been_processed.compare_exchange_strong(expected, true))
				{
				parameters->tree->push_back(&parameters->memory, task->vector);
				/*
					If we're in movie mode the dump the tree in a format we can read in GNU Octave
				*/
				if (parameters->movie_mode)
					{
					std::ostringstream filename;
					filename << "movie." << index << ".txt";

					std::ofstream outfile(filename.str().c_str());
					parameters->tree->text_render_movie(outfile);
					outfile.close();
					}
				}
			index++;
			}
		}
	}

/*
	THREAD_ASCII_TO_FLOAT()
	-----------------------
*/
void thread_ascii_to_float(std::vector<uint8_t *> &lines, std::vector<job *> &vector_list, k_tree::object &example_vector, k_tree::allocator &memory, size_t start, size_t stop)
	{
	for (size_t current_line = start; current_line < stop; current_line++)
		{
		const auto *line = lines[current_line];

		size_t dimension = 0;
		k_tree::object *objectionable = example_vector.new_object(&memory);
		char *pos = (char *)line;

		do
			{
			while (isspace(*pos))
				pos++;
			if (*pos == '\0')
				break;
			double value_as_double;
			float value;
			bool isok = fast_double_parser::parse_number(pos, &value_as_double);
			if (isok)
				value = (float)value_as_double;
			else
				value = (float)atof(pos);
			while (*pos != '\0' && !isspace(*pos))
				pos++;
			objectionable->vector[dimension] = value;
			dimension++;
			}
		while (*pos != '\0');

		vector_list[current_line] = new (memory.malloc(sizeof(job))) job(objectionable);
		}
	}

/*
	ADD_LIST_TO_TREE()
	------------------
*/
int add_list_to_tree(k_tree::allocator *memory, size_t dimensions, std::vector<job *>&vector_list, size_t tree_order, char *outfilename, size_t thread_count, bool movie_mode, bool dump)
	{
	k_tree::k_tree tree(memory, tree_order, dimensions);

	auto timer = JASS::timer::start();
	/*
		Add them to the tree
		Start a bunch of threads to each do some of the work
	*/
	std::unordered_map<std::thread *, worker *> thread_pool;
	for (size_t which = 0; which < thread_count ; which++)
		{
		worker *work = new worker(&tree, &vector_list, movie_mode, outfilename);
		thread_pool[new std::thread(thread_work, work)] = work;
		}

	/*
		Wait until all the threads have finished
	*/
	for (auto &[thread, work] : thread_pool)
		{
		(void)work;					// remove the warning
		thread->join();
		}

	auto took = JASS::timer::stop(timer);
	std::cout << "D:" << dimensions << " O:" << tree_order << " T:" << thread_count << "  ClusteringTook:" << took.milliseconds() << " milliseconds\n";

	/*
		Fix the leaf count value
	*/
	timer = JASS::timer::start();
	tree.normalise_counts();
	took = JASS::timer::stop(timer);
	std::cout << "Re-adjustment:" << took.milliseconds() << " milliseconds\n";

	/*
		Dump the tree to the output file
	*/
	if (dump)
		{
		timer = JASS::timer::start();
		std::ofstream outfile(outfilename);
		if (movie_mode)
			tree.text_render_movie(outfile);
		else
			{
//			tree.text_render_penultimate(outfile);
//			tree.text_render_penultimate_and_below(outfile);
			outfile << tree;
			}
		outfile.close();
		took = JASS::timer::stop(timer);
		std::cout << "Serialising:" << took.milliseconds() << " milliseconds\n";
		}

	/*
		Clean up
	*/
	for (auto &[thread, work] : thread_pool)
		{
		delete thread;
		delete work;
		}

	return 0;
	}

/*
	BUILD()
	-------
	Build the k-tree from the input data
*/
int build(char *infilename, size_t tree_order, char *outfilename, size_t thread_count, bool movie_mode)
	{
	thread_count = thread_count <= 0 ? 1 : thread_count;

	k_tree::allocator memory;
	std::vector<job *> vector_list;

	auto timer = JASS::timer::start();

	/*
		Read the source file into memory - and check that we got a file
	*/
	std::string file_contents;
	size_t bytes = read_entire_file(infilename, file_contents);
	if (bytes <= 0)
		exit(printf("Cannot read vector file: '%s'\n", infilename));

	/*
		Break it into lines
	*/
	std::vector<uint8_t *> lines;
	buffer_to_list(lines, file_contents);

	/*
		Count the dimensionality of the first vector (the others should be the same)
	*/
	size_t dimensions = dimensionality((char *)lines[0]);

	k_tree::object *example_vector = k_tree::object::snag(&memory, dimensions, nullptr);

	/*
		Convert from ASCII to floats in parallel
	*/
	std::vector<std::thread>thread_pool_deascii;
	vector_list.resize(lines.size());		// create space for each completed pointer
	size_t gap = lines.size() / thread_count;
	for (size_t which = 0; which < thread_count ; which++)
		{
		/*
			If were the last "block" then make sure we get to the end (we assume lines.size() is not evenly dividible by the thread count)
		*/
		k_tree::allocator *local_allocator = new k_tree::allocator;
		if (which == thread_count - 1)
			thread_pool_deascii.push_back(std::thread(thread_ascii_to_float, std::ref(lines), std::ref(vector_list), std::ref(*example_vector), std::ref(*local_allocator), gap * which, lines.size()));
		else
			thread_pool_deascii.push_back(std::thread(thread_ascii_to_float, std::ref(lines), std::ref(vector_list), std::ref(*example_vector), std::ref(*local_allocator), gap * which, gap * which + gap));
		}

	/*
		Wait until all the threads have finished
	*/
	for (auto &completed : thread_pool_deascii)
		completed.join();

	auto took = JASS::timer::stop(timer);
	std::cout << "Reading :" << took.milliseconds() << " milliseconds\n";

	return add_list_to_tree(&memory, dimensions, vector_list, tree_order, outfilename, thread_count, movie_mode, true);
	}

/*
	BUILD_BIN()
	-----------
	Build the k-tree from binary input data.  The format is:
	<size_t width>
	<vector<float>>...
	Where each vectoris of <width> size
*/
int build_bin(char *infilename, size_t tree_order, char *outfilename, size_t thread_count, bool movie_mode, bool dump)
	{
	thread_count = thread_count <= 0 ? 1 : thread_count;

	k_tree::allocator memory;
	std::vector<job *> vector_list;

	auto timer = JASS::timer::start();
	/*
		Read the source file into memory - and check that we got a file
	*/
	std::string file_contents;
	size_t bytes = read_entire_file(infilename, file_contents);
	if (bytes <= 0)
		exit(printf("Cannot read vector file: '%s'\n", infilename));
	char *file_contents_pointer = file_contents.data();
	char *file_contents_end = file_contents_pointer + bytes;

	/*
		Read the dimensinality
	*/
	size_t dimensions = *(size_t *)file_contents_pointer;
	file_contents_pointer += sizeof(size_t);

	/*
		Load all the jobs
	*/
	size_t count = 0;
	while (file_contents_pointer < file_contents_end)
		{
		k_tree::object *objectionable = k_tree::object::snag(&memory, dimensions, (float *)file_contents_pointer);
		vector_list.push_back(new (memory.malloc(sizeof(job))) job(objectionable));
		file_contents_pointer += dimensions * sizeof(float);
		count++;
		}

	auto took = JASS::timer::stop(timer);
	std::cout << "Reading :" << took.milliseconds() << " milliseconds\n";

	return add_list_to_tree(&memory, dimensions, vector_list, tree_order, outfilename, thread_count, movie_mode, dump);
	}

/*
	LOAD()
	------
	Load a serialised k-tree, deserialise it, then re-serialise it
*/
int load(char *infilename, size_t tree_order, char *outfilename)
	{
	/*
		Read the serialised tree into memory
	*/
	std::string file;
	read_entire_file(infilename, file);

	/*
		Work out the dimensionality of the vector
	*/
	size_t dimensions = 0;
	char *pos = file.data();
	do
		{
		while (*pos == ' ')
			pos++;
		if (*pos == '\0' || *pos == '\r' || *pos == '\n')
			break;
		dimensions++;
		while (*pos != '\0' && *pos != '\r' && *pos != '\n' && *pos != ' ')
			pos++;
		}
	while (*pos != '\0' && *pos != '\n' && *pos != '\r');

	dimensions -= 2;		// subtract the leaf-count and the child-count to get the dimensionality

	/*
		Allocate the k-tree
	*/
	k_tree::allocator memory;
	k_tree::k_tree tree(&memory, tree_order, dimensions);

	/*
		Turn the file into a stream and deserialise it
	*/
	std::istringstream instream(file);
	tree.deserialise(memory, instream, *tree.get_example_object());

	/*
		Serialise the tree again
	*/
	std::ofstream outfile(outfilename);
	outfile << tree;
	outfile.close();

	return 0;
	}

/*
	UNITTEST()
	----------
	Unit test each of the classes we use
*/
int unittest(void)
	{
	k_tree::object::unittest();
	k_tree::k_tree::unittest();

	return 0;
	}

/*
	USAGE()
	-------
	Shlomo's original program took:
		Usage: ktree  ['build'|'load']  in_file  tree_order  out_file
	Where
		build | load is to build the k-tree or load one from disk (currently IGNORED)
		in_file is the ascii file of vectors, one pre line and human readable in ASCII
		tree_order is the branching factor of the k-tree
		out_file (IGNORED)
*/
int usage(char *exename)
	{
	std::cout << "Usage:" << exename << " build  <in_file> <tree_order> <outfile> <thread_count>\n";
	std::cout << "      " << exename << " load  <in_file> <tree_order> <outfile>\n";
	std::cout << "      " << exename << " movie  <in_file> <tree_order> <outfile>\n";
	std::cout << "      " << exename << " build_bin  <infile> <tree_order> <outfile> <thread_count>\n";
	std::cout << "      " << exename << " unittest\n";
	return 0;
	}

/*
	MAIN()
	------
*/
int main(int argc, char *argv[])
	{
	try
		{
		if (argc == 2 && strcmp(argv[1], "unittest") == 0)
			return unittest();

		/*
			Check the tree order is "reasonable"
		*/
		size_t tree_order = atoi(argv[3]);
	//	if (tree_order < 2 || tree_order > 1'000'000)
	//		exit(printf("Tree order must be between 2 and 1,000,000\n"));
	
		if (argc == 6 && strcmp(argv[1], "build") == 0)
			return build(argv[2], tree_order, argv[4], atoi(argv[5]), false);
		else if (argc == 5 && strcmp(argv[1], "load") == 0)
			return load(argv[2], tree_order, argv[4]);
		else if (argc == 6 && strcmp(argv[1], "build_bin") == 0)
			return build_bin(argv[2], tree_order, argv[4], atoi(argv[5]), false, true);
		else if (argc == 6 && strcmp(argv[1], "build_bin_only") == 0)
			return build_bin(argv[2], tree_order, argv[4], atoi(argv[5]), false, false);
		else if (argc == 5 && strcmp(argv[1], "movie") == 0)
			return build(argv[2], tree_order, argv[4], 1, true);
		else
			return usage(argv[0]);
		}
	catch (const std::exception& e)
		{
		puts(e.what());
		}
	catch (...)
		{
		puts("Caught an unknown error");	
		}
	puts("Finished");
	}
