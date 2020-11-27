/*
	GENERATE_POINTS.CPP
	-------------------
	Generate some number of points in n-dimensional space around k centroids
*/
#include <stdlib.h>

#include <random>

/*
	CLASS POINT
	-----------
*/
class point
	{
	public:
		float x;
		float y;

	public:
		/*
			POINT::POINT()
			--------------
		*/
		point(float x, float y) :
			x(x),
			y(y)
			{
			/* Nothing */
			}
	};

std::vector<point> centroid;

/*
	USAGE()
	-------
*/
int usage(const char *exename)
	{
	printf("Usage: %s <points> <centers>\n", exename);
	return 1;
	}

/*
	MAIN()
	------
*/
int main(int argc, const char *argv[])
	{
	if (argc != 3)
		exit(usage(argv[0]));

	size_t points = atol(argv[1]);
	size_t centers = atol(argv[2]);

	/*
		Generate the centers
	*/
	for (size_t centre = 0; centre < centers; centre++)
		centroid.push_back(point(drand48() * 20 - 10, drand48() * 20 - 10));			// generate in the range (-10..10)

	/*
		Generate points around each centre
	*/
	FILE *fp = fopen("a.out.bin", "w+b");
	FILE *fp_txt = fopen("a.out.txt", "w+b");
	size_t dimensions = 2;
	fwrite(&dimensions, sizeof(dimensions), 1, fp);
	std::default_random_engine generator;
	for (size_t centre = 0; centre < centers; centre++)
		{
		std::normal_distribution<float> x_distribution(centroid[centre].x, 0.005 * centre);
		std::normal_distribution<float> y_distribution(centroid[centre].y, 0.005 * centre);
		for (size_t point = 0; point < points; point++)
			{
			float x = x_distribution(generator);
			float y = y_distribution(generator);

			fwrite(&x, sizeof(x), 1, fp);
			fwrite(&y, sizeof(y), 1, fp);
			fprintf(fp_txt, "%f %f\n", x, y);
			}
		}

	fclose(fp);
	fclose(fp_txt);
	return 0;
	}
