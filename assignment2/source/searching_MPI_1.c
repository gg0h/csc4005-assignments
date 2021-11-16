
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <mpi.h>

/**
 * Naive pattern search, fine grain master/slave model implementation
 *    _____ __________.___                __    __                                                           .__     
 *   /     \\______   \   | ___________ _/  |__/  |_  ___________  ____     ______ ____ _____ _______   ____ |  |__  
 *  /  \ /  \|     ___/   | \____ \__  \\   __\   __\/ __ \_  __ \/    \   /  ___// __ \\__  \\_  __ \_/ ___\|  |  \ 
 * /    Y    \    |   |   | |  |_> > __ \|  |  |  | \  ___/|  | \/   |  \  \___ \\  ___/ / __ \|  | \/\  \___|   Y  \
 * \____|__  /____|   |___| |   __(____  /__|  |__|  \___  >__|  |___|  / /____  >\___  >____  /__|    \___  >___|  /
 *         \/               |__|       \/                \/           \/       \/     \/     \/            \/     \/ 
 * Usage: 
 * mpicc searching_MPI_1.c -O2 --std=gnu99 -o searching_MPI_1
 * mpirun -np 8 searching_MPI_1 <numberOfPatternFiles>
 * 
**/


char *textData;
int textLength;

int textChunkSize;
char *textChunkProc;

char *patternData;
int patternLength;

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

void readText()
{
	FILE *f;
	char fileName[1000];
	sprintf(fileName, "inputs/text.txt");
	f = fopen(fileName, "r");
	if (f == NULL)
	{
		printf("Error cannot read file: %s", fileName);
		exit(0);
	}
	readFromFile(f, &textData, &textLength);
	fclose(f);
}

int readPattern(int patternNumber)
{
	FILE *f;
	char fileName[1000];
	sprintf(fileName, "inputs/pattern%d.txt", patternNumber);
	f = fopen(fileName, "r");
	if (f == NULL)
		return 0;
	readFromFile(f, &patternData, &patternLength);
	fclose(f);

	printf("Read Pattern number %d\n", patternNumber);

	return 1;
}

int hostMatch(long *comparisons)
{
	int i, j, k, lastI;

	i = 0;
	j = 0;
	k = 0; 
	lastI = textChunkSize - patternLength;
	*comparisons = 0;

	while (i <= lastI && j < patternLength)
	{
		(*comparisons)++;
		if (textChunkProc[k] == patternData[j])
		{
			k++;
			j++;
		}
		else
		{
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


	if (worldRank == 0)
	{
		//declare array for copying, variable to hold start/end of chunk indicies
		char *textChunkContentTemp;
		int TextChunkStart, TextChunkEnd;

		// text initialization
		readText();

		// Step 1: divide text into chunks and send to each process
		for (int process = 0; process < worldSize; process++)
		{

			// because we know the dataset and number of processes we will use, we do not need to handle case where chunks cannot be divided evenly
			TextChunkStart = process * (textLength / worldSize);
			TextChunkEnd = process * (textLength / worldSize) + (textLength / worldSize) - 1;

			// calculate size of each text chunk
			textChunkSize = TextChunkEnd - TextChunkStart + 1;
			// if curent iteration rank 0, we are on master process, initialize the master text chunk
			if (process == 0)
			{	
				//allocate space for text chunk
				textChunkProc = (char *)malloc(textChunkSize * sizeof(char));
				// copy chunk from overall textData into allocate chunk
				for (int i = TextChunkStart, j = 0; i <= TextChunkEnd, j < textChunkSize; i++, j++)
				{
					textChunkProc[j] = textData[i];
				}
			}
			// otherwise, read the chunk into temporary array from overall textData, send size followed by chunk to current process
			else
			{
				textChunkContentTemp = (char *)malloc(textChunkSize * sizeof(char));
				for (int i = TextChunkStart, j = 0; i <= TextChunkEnd, j < textChunkSize; i++, j++)
				{
					textChunkContentTemp[j] = textData[i];
				}
				MPI_Send(&textChunkSize, 1, MPI_INT, process, 0, MPI_COMM_WORLD);
				MPI_Send(textChunkContentTemp, textChunkSize, MPI_CHAR, process, 1, MPI_COMM_WORLD);
			}
		}
		free(textData);
		free(textChunkContentTemp);
	}
	else
	{
		// recv text array size
		MPI_Recv(&textChunkSize, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		// allocate array for text chunk
		textChunkProc = (char *)malloc(textChunkSize * sizeof(char));
		// receive text array chunk
		MPI_Recv(textChunkProc, textChunkSize, MPI_CHAR, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		printf("This is process %d, received size of %d and allocated. First Char is %c\n", worldRank, textChunkSize, textChunkProc[0]);
	}

	//Step 2: iterate pattern files, send to each process, and reduce results

	int patternNumber = 1, localResult[2], masterResult[2], totalIndex;
	long comparisons;
	while (patternNumber <= numPatterns)
	{
		//read pattern
		if (worldRank == 0)
			readPattern(patternNumber);

		//master broadcasts pattern length to all other procs
		MPI_Bcast(&patternLength, 1, MPI_INT, 0, MPI_COMM_WORLD);
		// on processes other than master, allocate patternData to recv from broadcast
		if (worldRank != 0)
			patternData = (char *)malloc(patternLength * sizeof(char));
		
		//master broadcasts pattern to all other procs
		MPI_Bcast(patternData, patternLength, MPI_CHAR, 0, MPI_COMM_WORLD);
		
		// each process search their text for pattern
		// using this method, we get get the rank of process when reducing
		localResult[0] = hostMatch(&comparisons);
		localResult[1] = worldRank;
		printf("process %d here my result is %d\n", worldRank, localResult[0]);

		
		// This implementation assumes only one occurence of pattern in text
		MPI_Reduce(localResult, masterResult, 1, MPI_2INT, MPI_MAXLOC, 0, MPI_COMM_WORLD);
		if (worldRank == 0)
		{
			// if reduce max of every process is -1, then found in 0 occurences in text chunks
			if (masterResult[0] == -1)
				printf("Pattern number %d not found!\n", patternNumber);
			else
			{
				// calculate the overall index in the text of the found pattern: (numberOfPreceedingChunks * chunkSize) + indexInThisChunk
				totalIndex = (masterResult[1] * textChunkSize) + masterResult[0];
				printf("Pattern number %d found at position %d, by process %d\n",patternNumber, totalIndex, masterResult[1]);

			}
				
		}
		// done with this pattern now
		free(patternData);
		patternNumber++;
	}

	free(textChunkProc);

	// record elapsed time for process, synchronise for reduction
	double totalTime = MPI_Wtime() - start;
	MPI_Barrier(MPI_COMM_WORLD);

	double avgTime, minTime, maxTime;

	// maximum search time from all processes
	MPI_Reduce(&totalTime, &maxTime, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
	// minimum search time from all processes
	MPI_Reduce(&totalTime, &minTime, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);
	// total sum of time from all processes (take average when printing)
	MPI_Reduce(&totalTime, &avgTime, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

	if (worldRank == 0)
	{
		avgTime /= worldSize;
		printf("Minimum search time: %lf Maximum search time: %lf Average search time: %lf\n", minTime, maxTime, avgTime);
	}

	MPI_Finalize();
}
