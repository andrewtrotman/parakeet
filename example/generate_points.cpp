/*
	GENERATE_POINTS.CPP
	-------------------
	Generate some number of points in n-dimensional space around k centroids
*/
#include <stdlib.h>

#include <random>
#include <vector>
#include <algorithm>

/*
	DRAND48()
	---------
*/
#ifdef _MSC_VER
inline double drand48()
	{
	return double(rand())/(RAND_MAX+1.0);
	}
#endif

/*
	CLASS POINT
	-----------
*/
class point
	{
	public:
		std::vector<float> data;
	public:
		point() :
			data()
		{
		/* Nothing */
		}
	};

std::vector<point> centroid;
std::vector<point> data_point;

/*
	USAGE()
	-------
*/
int usage(const char *exename)
	{
	printf("Usage: %s <dimensions> <points> <centers>\n", exename);
	return 1;
	}

/*
	MAIN()
	------
*/
int main(int argc, const char *argv[])
	{
	std::default_random_engine generator;

	if (argc != 4)
		exit(usage(argv[0]));

	size_t dimensions = atol(argv[1]);
	size_t points = atol(argv[2]);
	size_t centers = atol(argv[3]);

	/*
		Generate the centers, then generate pointe around that centre.
	*/
	for (size_t centre = 0; centre < centers; centre++)
		{
		point another;
		for (size_t index = 0; index < dimensions; index++)
			another.data.push_back((float)(drand48() * 20 - 10));			// generate in the range (-10..10)
		centroid.push_back(another);

		/*
			Generate points around the center
		*/
		for (size_t which = 0; which < points; which++)
			{
			point location;
			for (size_t index = 0; index < dimensions; index++)
				{
				std::normal_distribution<float> normal_distribution(another.data[index], (float)0.005 * centre);
				location.data.push_back(normal_distribution(generator));
				}
			data_point.push_back(location);
			}
		}
	/*
		Shuffle the points
	*/
		std::shuffle(data_point.begin(), data_point.end(), generator);

	/*
		Write the points to a file
	*/
	FILE *fp = fopen("a.out.bin", "w+b");
	FILE *fp_txt = fopen("a.out.txt", "w+b");
	fwrite(&dimensions, sizeof(dimensions), 1, fp);
	for (const auto &single : data_point)
		{
		for (size_t index = 0; index < single.data.size(); index++)
			{
			fwrite(&single.data[index], sizeof(single.data[index]), 1, fp);
			fprintf(fp_txt, "%s%f", (index == 0 ? "" : " "), single.data[index]);
			}
		fprintf(fp_txt, "\n");
		}
	fclose(fp);
	fclose(fp_txt);

	/*
		Done
	*/
	return 0;
	}
