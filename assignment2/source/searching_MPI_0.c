
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <mpi.h>

/**
 * Naive pattern search, embarrasingly parallel implementation
 *    _____ __________.___                __    __                                                           .__     
 *   /     \\______   \   | ___________ _/  |__/  |_  ___________  ____     ______ ____ _____ _______   ____ |  |__  
 *  /  \ /  \|     ___/   | \____ \__  \\   __\   __\/ __ \_  __ \/    \   /  ___// __ \\__  \\_  __ \_/ ___\|  |  \ 
 * /    Y    \    |   |   | |  |_> > __ \|  |  |  | \  ___/|  | \/   |  \  \___ \\  ___/ / __ \|  | \/\  \___|   Y  \
 * \____|__  /____|   |___| |   __(____  /__|  |__|  \___  >__|  |___|  / /____  >\___  >____  /__|    \___  >___|  /
 *         \/               |__|       \/                \/           \/       \/     \/     \/            \/     \/ 
 * Usage: 
 * mpicc searching_MPI_0.c -O2 --std=gnu99 -o searching_MPI_0
 * mpirun -np 8 searching_MPI_0 <numberOfPatternFiles>
 * 
**/


char *textData;
int textLength;

char *patternData;
int patternLength;

clock_t c0, c1;
time_t t0, t1;

void outOfMemory()
{
	fprintf (stderr, "Out of memory\n");
	exit (0);
}

void readFromFile (FILE *f, char **data, int *length)
{
	int ch;
	int allocatedLength;
	char *result;
	int resultLength = 0;

	allocatedLength = 0;
	result = NULL;

	

	ch = fgetc (f);
	while (ch >= 0)
	{
		resultLength++;
		if (resultLength > allocatedLength)
		{
			allocatedLength += 10000;
			result = (char *) realloc (result, sizeof(char)*allocatedLength);
			if (result == NULL)
				outOfMemory();
		}
		result[resultLength-1] = ch;
		ch = fgetc(f);
	}
	*data = result;
	*length = resultLength;
}

void readText () {
	FILE *f;
	char fileName[1000];
	printf("test\n");
	sprintf (fileName, "inputs/text.txt");
	f = fopen (fileName, "r");
	if (f == NULL)
	{
		printf("Error cannot read file: %s", fileName);
		exit(0);
	}
	readFromFile (f, &textData, &textLength);
	fclose (f);
}


int readPattern (int testNumber)
{
	FILE *f;
	char fileName[1000];
	sprintf (fileName, "inputs/pattern%d.txt", testNumber);
	f = fopen (fileName, "r");
	if (f == NULL)
		return 0;
	readFromFile (f, &patternData, &patternLength);
	fclose (f);

	printf ("Read Pattern number %d\n", testNumber);

	return 1;

}



int hostMatch(long *comparisons)
{
	int i,j,k, lastI;
	
	i=0;
	j=0;
	k=0;
	lastI = textLength-patternLength;
        *comparisons=0;

	while (i<=lastI && j<patternLength)
	{
                (*comparisons)++;
		if (textData[k] == patternData[j])
		{
			k++;
			j++;
		}
		else
		{
			i++;
			k=i;
			j=0;
		}
	}
	if (j == patternLength)
		return i;
	else
		return -1;
}
void processData()
{
	unsigned int result;
        long comparisons;

	printf ("Text length = %d\n", textLength);
	printf ("Pattern length = %d\n", patternLength);

	result = hostMatch(&comparisons);
	if (result == -1)
		printf ("Pattern not found\n");
	else
		printf ("Pattern found at position %d\n", result);
        printf ("# comparisons = %ld\n", comparisons);

}

int main(int argc, char **argv)
{

	// Initialize the MPI environment
	MPI_Init(NULL, &argv);

	// synchronise for timing
	MPI_Barrier(MPI_COMM_WORLD);
	double start = MPI_Wtime();

	int numPatterns;
	if (!argv[1])
	{
		// default to 8 patterns unless specified in arg
		numPatterns = 8;
	}
	else
	{
		// cast string cmdline arg to integer
		numPatterns = atoi(argv[1]);
	}

	// Find out rank, size
	int worldRank;
	MPI_Comm_rank(MPI_COMM_WORLD, &worldRank);
	int worldSize;
	MPI_Comm_size(MPI_COMM_WORLD, &worldSize);

	int testNumber;
	// read text once
	readText();

	testNumber = 1;
	while (testNumber <= numPatterns)
	{	
		// This will divide the test cases evenly between the processes
		// when worldSize == 2, pattern is 1, 0, 1, 0, 1, 0 ...
		// when worldSize == 4, pattern is 1, 2, 3, 0, 1, 2 ...
		// when worldSize == 8, pattern is 1, 2, 3, 4 ... 7, 0
		// if worldSize is greater than 8, not all processes are used, as only 8 testCases
		if (testNumber % worldSize == worldRank) 
		{
			printf("Processing test number %d in process %d\n", testNumber, worldRank);
			readPattern(testNumber);
			processData();
		}
		testNumber++;
	}

	// syncrhonise for timing
	MPI_Barrier(MPI_COMM_WORLD);
	double totalTime = MPI_Wtime();

	// implemented this MPI time calculation to check consistency of time obtain using /bin/time
	double avgTime, minTime, maxTime;

	// maximum search time from all processes
	MPI_Reduce(&totalTime, &maxTime, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
	// minimum search time from all processes
	MPI_Reduce(&totalTime, &minTime, 1, MPI_DOUBLE, MPI_MIN, 0,MPI_COMM_WORLD);
	// total sum of time from all processes (take average when printing)
	MPI_Reduce(&totalTime, &avgTime, 1, MPI_DOUBLE, MPI_SUM, 0,MPI_COMM_WORLD);

	if (worldRank == 0) 
   	{
		avgTime /= worldSize;
		printf("Minimum search time: %lf Maximum search time: %lf Average search time: %lf\n", minTime, maxTime, avgTime);
   	}

	MPI_Finalize();

}
