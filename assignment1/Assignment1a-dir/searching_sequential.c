
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/resource.h>

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
	int i, j, k, lastI;

	i = 0;   //current starting position on the text, from which a match can be checked
	j = 0;   //current postion on the pattern
	k = 0;   //position on the text being checked against a pattern position
	lastI = textLength - patternLength;  // last index to be checked, any further len of pattern would bypass end of array
	*comparisons = 0;

	while (i <= lastI && j < patternLength)
	{
		(*comparisons)++;
		if (textData[k] == patternData[j])
		{
			// text position k matches start of pattern,
			k++;
			j++;
			// if entire pattern matches, latter while condition broken so loop finishes
		}
		else
		{
			// advanced i for starting position on text to check pattern, reset k to i position, reset j to start of pattern (0)
			i++;
			k = i;
			j = 0;
		}
	}
	if (j == patternLength)
		return i;
	else
		return -1;
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
	int iterations = 25;

	testNumber = 0;
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
		//printf("Test %d elapsed CPU time = %.6f\n\n", testNumber, ((double)c1 - (double)c0) / CLOCKS_PER_SEC);	


		printf("CPU time TOTAL = %.06fs \n", (double)(endTime.tv_sec - startTime.tv_sec) +
           1e-6*(endTime.tv_usec - startTime.tv_usec));
		printf("CPU time AVERAGE ITERATION = %.10fs \n\n", (double)((endTime.tv_sec - startTime.tv_sec) +
           1e-6*(endTime.tv_usec - startTime.tv_usec))/iterations);
		testNumber++;
	}
}
