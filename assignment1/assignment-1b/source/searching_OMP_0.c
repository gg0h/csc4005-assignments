
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/resource.h>
#include <omp.h>

////////////////////////////////////////////////////////////////////////////////
// Program main
////////////////////////////////////////////////////////////////////////////////

char *textData;
int textLength;

char *patternData;
int patternLength;

time_t t0, t1;

void outOfMemory()
{
	fprintf(stderr, "Out of memory\n");
	exit(0);
}

void readFromFile(FILE *f, char **data, int *length)
{
	int ch;
	int allocatedLength;
	char *result;
	int resultLength = 0;

	allocatedLength = 0;
	result = NULL;

	ch = fgetc(f);
	while (ch >= 0)
	{
		resultLength++;
		if (resultLength > allocatedLength)
		{
			allocatedLength += 10000;
			result = (char *)realloc(result, sizeof(char) * allocatedLength);
			if (result == NULL)
				outOfMemory();
		}
		result[resultLength - 1] = ch;
		ch = fgetc(f);
	}
	*data = result;
	*length = resultLength;
}

int readData(int testNumber)
{
	FILE *f;
	char fileName[1000];
#ifdef DOS
	sprintf(fileName, "inputs\\test%d\\text.txt", testNumber);
#else
	sprintf(fileName, "inputs/test%d/text.txt", testNumber);
#endif
	f = fopen(fileName, "r");
	if (f == NULL)
		return 0;
	readFromFile(f, &textData, &textLength);
	fclose(f);
#ifdef DOS
	sprintf(fileName, "inputs\\test%d\\pattern.txt", testNumber);
#else
	sprintf(fileName, "inputs/test%d/pattern.txt", testNumber);
#endif
	f = fopen(fileName, "r");
	if (f == NULL)
		return 0;
	readFromFile(f, &patternData, &patternLength);
	fclose(f);

	printf("Read test number %d\n", testNumber);

	return 1;
}

int hostMatch(long *comparisons)
{

	int lastI = textLength - patternLength;  // last index to be checked, any further len of pattern would bypass end of array
	*comparisons = 0;

	int startingMatchIndex = -1; //index at which the pattern match begins, update in loop when found
	printf("Running with %d processors ======================= \n", omp_get_num_procs());

	long int tmpComparisons;	
	#pragma omp parallel shared(tmpComparisons, startingMatchIndex) num_threads(4)
	{
		
		#pragma omp for reduction(+: tmpComparisons)
		for (int i = 0; i <= textLength - patternLength; i++) {
			// cannot break, but at least remove work after pattern found
			// if (startingMatchIndex != -1) continue;

			//printf("Parallel For Iteration %d calling from thread %d \n", i, omp_get_thread_num() );
			int j;

			/* For current index i, check for pattern match */
			for (j = 0; j < patternLength; j++)
			{
				tmpComparisons++;
				if (patternData[j] != textData[j + i])
					break;
			}
			// this check will only pass if starting from index i in text all positions match the pattern
			// patternData[0, ..., patternLength -1]  == textData[i, ..., i+ patternLength - 1]
			if (patternLength == j)
			{
				startingMatchIndex = i;
				printf("Match found for pattern starting at index %d \n", startingMatchIndex);
			}
		}
		
	}
	*comparisons = tmpComparisons;
	return startingMatchIndex;

	
}
void processData(int iterations)
{
	unsigned int result;
	long comparisons;

	printf("Text length = %d\n", textLength);
	printf("Pattern length = %d\n", patternLength);

	// perform multiple iterations of the search algorithm and take an average
	for(int i = 0; i< iterations; i++)
		result = hostMatch(&comparisons);
	if (result == -1)
		printf("Pattern not found\n");
	else
		printf("Pattern found at position %d\n", result);
	printf("# comparisons = %ld\n", comparisons);
}

int main(int argc, char **argv)
{
	int testNumber;

	struct rusage usage;
	struct timeval startTime, endTime;

	// perform multiple iterations of the naive search and take an average to get time measurements for lower text * pattern products, they will round to 0 without this.
	int iterations = 1;

	testNumber = 0;
	// clock_t c0, c1;
	while (readData(testNumber))
	{
		
		// I changed the method to measure CPU time, as the existing method had low precison (2DP) when running on kelvin (in my experience)
		getrusage(RUSAGE_SELF, &usage);
		startTime = usage.ru_utime;
		// c0 = clock();
		processData(iterations);

		getrusage(RUSAGE_SELF, &usage);
		endTime = usage.ru_utime;
		// c1 = clock();
		// printf("Test %d elapsed CPU time = %.6f\n\n", testNumber, ((double)c1 - (double)c0) / CLOCKS_PER_SEC);	


		printf("CPU time TOTAL = %.06fs \n", (double)(endTime.tv_sec - startTime.tv_sec) +
           1e-6*(endTime.tv_usec - startTime.tv_usec));
		printf("CPU time AVERAGE ITERATION = %.10fs \n\n", (double)((endTime.tv_sec - startTime.tv_sec) +
           1e-6*(endTime.tv_usec - startTime.tv_usec))/iterations);
		testNumber++;
	}
}
