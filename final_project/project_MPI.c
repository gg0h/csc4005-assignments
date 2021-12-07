
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <mpi.h>

/**
 * Naive pattern search, fine grain master/slave model implementation
 * 
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

int worldRank, worldSize;

int operationMode, textIndex, patternIndex;

FILE* controlFile;
FILE* resultFile;
char * controlFileLine;
int searchStartPositionsLength, searchStartPositionsLengthDiv, searchStartPositionsLengthMod;

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

/**
 * @brief Function to read the contents of pattern and text files of specified index into local arrays
 * 
 * @param textNumber - text file number to be read
 * @param patternNumber - pattern file number to be read 
 * @return int - return 1 on success
 */
int readData(int textNumber, int patternNumber)
{
    // Read text file
	FILE *f;
	char fileName[1000];
#ifdef DOS
	sprintf(fileName, "inputs\\text%d.txt", textNumber);
#else
	sprintf(fileName, "inputs/text%d.txt", textNumber);
#endif
	f = fopen(fileName, "r");
	if (f == NULL)
		return 0;
	readFromFile(f, &textData, &textLength);
	fclose(f);

	printf("Read text number %d\n", textNumber);

    // Read pattern file
#ifdef DOS
	sprintf(fileName, "inputs\\pattern%d.txt", patternNumber);
#else
	sprintf(fileName, "inputs/pattern%d.txt", patternNumber);
#endif
	f = fopen(fileName, "r");
	if (f == NULL)
		return 0;
	readFromFile(f, &patternData, &patternLength);
	fclose(f);

	printf("Read pattern number %d\n", patternNumber);

	return 1;
}



/**
 * @brief Function to parse the operation mode, text file index and pattern file index from a line of the control file
 * parses these commands from strings that match the format "%d %d %d" and reads them as operationMode, textIndex, patternIndex in that order
 * 
 * @param controlString - the control string to parse, a line of the control file
 */

void parseControlFileLine(char * controlString)
{
    // ensure format matches, and if so read the three values from the string
    if (sscanf(controlString, "%d %d %d", &operationMode, &textIndex, &patternIndex) != 3) {
        printf("Error in control file format: %s\n", controlString);
        exit(EXIT_FAILURE);
    }
}

/**
 * @brief function to check conditions on the properties of input pattern and text files that will
 * prevent a successful search from ever occuring. 
 * 
 * @return int - 1 if condition found that invalidates the search, else 0
 */

int checkEdgeCases() {
    // EDGE CASES

    // pattern length exceeds text length
    if (patternLength > textLength)
        return 1; 

    // pattern file empty
    if (textLength == 0)
        return 1;

    // text file empty
    if (patternLength == 0)
        return 1;

    return 0;
}


int calculateOverallIndexInText(int processRank, int indexInChunk)
{
     return(processRank * searchStartPositionsLengthDiv) + \
                        (processRank < searchStartPositionsLengthMod ? processRank : searchStartPositionsLengthMod) + indexInChunk;
}

/**
 * @brief 
 * 
 * @return int 
 */
int searchFirstOccurence()
{
	int i, j, k, lastI;

	i = 0;
	j = 0;
	k = 0; 
	lastI = textChunkSize - patternLength;

	while (i <= lastI && j < patternLength)
	{
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

/**
 * @brief A function to find all occurrences of the pattern in text. Features a dynamically growing array of found indexes in overall text
 * 
 * @return int - number of occurences
 */
int * searchEveryOccurence(int * found)
{   
    int lastI = textLength - patternLength;  // last index to be checked, any further len of pattern would bypass end of text array

	*found = 0; // count of found matches
    int foundSize = 0; // current size of found array
    
    int* tmp = NULL;
    int* foundArray = NULL;

	for (int i = 0; i <= lastI; i++) {

        int j;

        // For current index i, check for pattern match character by character, if at any point pattern does not match, break. 
        for (j = 0; j < patternLength; j++)
        {
            if (patternData[j] != textData[j + i])
                break;
        }
        // this check will only pass if starting from index i in text all positions match the pattern
        // patternData[0, ..., patternLength -1]  == textData[i, ..., i+ patternLength - 1]
        if (patternLength == j)
        {
            // array full, make larger
            if (*found == foundSize) {
                //need more space in the array

                foundSize += 20;
                tmp = realloc(foundArray, foundSize); // get a new larger array
                if (!tmp)
                    MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE); // if cannot allocate array abort

                foundArray = tmp;
            }
            printf("found at index %d in chunk on proc %d\n", i, worldRank);
            printf("overall index %d\n", calculateOverallIndexInText(worldRank, i));
            foundArray[*found] = calculateOverallIndexInText(worldRank, i);
            printf("overall index from array %d\n", foundArray[*found]);
            *found += 1;
        }
    }
    return foundArray;
}



int main(int argc, char **argv)
{

	// Initialize the MPI environment
	MPI_Init(NULL, &argv);

	// synchronise for timing
	MPI_Barrier(MPI_COMM_WORLD);
	double start = MPI_Wtime();

	// Find out rank, size
	MPI_Comm_rank(MPI_COMM_WORLD, &worldRank);

	MPI_Comm_size(MPI_COMM_WORLD, &worldSize);

    if(worldRank == 0)
    {
        // filenames
        char controlFileName[] = "./inputs/control.txt";
        char resultFileName[] = "./result_MPI.txt";

        // read files
        controlFile = fopen(controlFileName, "r");
        resultFile = fopen(resultFileName, "w");

        // error handlings for file reads
        if (controlFile == NULL)
        {
            printf("Error reading file %s\n", controlFileName);
            exit(EXIT_FAILURE);
        } else if (resultFile == NULL)
        {
            printf("Error reading file %s\n", resultFileName);
            exit(EXIT_FAILURE);
        }
    }
        
    // repeat until condition is met that each process will break from this loop
    while(1) 
    {
        // master process
        if (worldRank == 0)
	    {   
            
            // declare variables for reading control file lines
            controlFileLine = NULL;
            size_t lineLength = 0;
            ssize_t read;

            //declare variable to hold start/end of chunk indicies and to temporarily store the size for each process
            int textChunkStart, textChunkEnd, textChunkSizeTemp;

            // if the end of the control file is reached or only empty lines remain, quit
            if ((read = getline(&controlFileLine, &lineLength, controlFile)) == -1 ||  strcmp(controlFileLine, "\r\n") == 0 || strcmp(controlFileLine, "\n") == 0) {
                // send -1 to indicate end of file reached so other processes may quit, omit master process
                textChunkSize = -1;
                for (int process = 1; process < worldSize; process++)
                    MPI_Send(&textChunkSize, 1, MPI_INT, process, 0, MPI_COMM_WORLD);
                // break out of the while loop
                break;
            }

            // process line from the control file to extract commands
            parseControlFileLine(controlFileLine);
            
            // printf("Read from line %s: text %d pattern %d mode %d \n", controlFileLine, textIndex, patternIndex, operationMode );
    
            // using the file index parsed from the control file line, read in the correct pattern and text files
            readData(textIndex, patternIndex);
            printf("Using operation mode %d\n", operationMode);

            // check edge cases of input files (see function for specifics), not found if any of these are true
            if(checkEdgeCases() == 1) {
                printf("Edge case exception reached\n\n");
                // don't need to send anything to other processes in this case, outcome of the search can be inferred from properties of files
                fprintf(resultFile,"%d %d %d\n", textIndex, patternIndex, -1);
                continue;
            }

            /**
             *  In this text chunking approach we will divide the section of textData from which pattern searches may start into chunks
             * e.g. in string,
             * textData = "ABCDEF"; patternData = "DEF";
             * matches in textData may start only from at least the character "D", so the string of possible starts will be "ABCD", this string can be derived knowing the pattern length
             * We are dividing this string (e.g. "ABCD") into as even sized chunks as the length and number of processes allow.
             * after this, we will extend the length of each of these chunks by patternLength -1. This allows the full textData to be searched and ensures
             * patterns that would occur over chunk boundaries are found
             * 
             * To return to the example, with 4 processes the textData = "ABCDEF" with patternData = "DEF" would divide into chunks
             * P0: "A" P1:"B", P2: "C", P3: "D"
             * which would be extended to:
             * P0: "ABC" P1:"BCD", P2: "CDE", P3: "DEF"  - 1 search per process
             * 
             * When pattern length is close enough to the text length, higher number processes may be assigned an empty chunk (no work) 
             * due to limited number of potential searches. In these cases we have added logic for the process to handle this upon receiving chunk size 0.
             * 
             * This approach optimises the amount of work done by each process; by dividing as evenly as possible the potential searches, 
             * we divide as evenly as possible the number of times the search algorithm is performed by each process (i.e. work) 
             * This will minimise idling of processes as they wait for other processes to finish searching.
             */ 

            
            // the length of the string with all of the positions within the text file from which a pattern match may start
            searchStartPositionsLength = textLength - (patternLength -1 );

            // integer division of substring searchStartPositionsLength and world size
            searchStartPositionsLengthDiv = searchStartPositionsLength / worldSize;
            // modulo of substring searchStartPositionsLength and world size
            searchStartPositionsLengthMod = searchStartPositionsLength % worldSize;

            for (int process = 0; process < worldSize; process++)
            {
                // these calculation will divide chunks of as close to even size as possible to distribute among processes
                // e.g. 4 processes, searchStartPositionsLength len 10 "ABCDEFGHIJ" ->  chunk 1: "ABC" chunk 2: "DEF" chunk 3: "GH", chunk 4: "IJ"
                textChunkStart = (process * searchStartPositionsLengthDiv) + (process < searchStartPositionsLengthMod ? process : searchStartPositionsLengthMod);
                textChunkEnd = (process + 1) * searchStartPositionsLengthDiv + ((process + 1) < searchStartPositionsLengthMod ? (process + 1) : searchStartPositionsLengthMod);
                // Chunk indicies signify chunk is textData[start : end - 1]

                if (textChunkStart == textChunkEnd) {
                    // chunk size is empty, send size of 0 and continue the loop
                    textChunkSizeTemp = 0;
                    MPI_Send(&textChunkSizeTemp, 1, MPI_INT, process, 0, MPI_COMM_WORLD);
                    continue;
                }

                // extend the end of the chunk  
                textChunkEnd += (patternLength - 1);

                // calculate size of each text chunk and store temporarily (master needs to remember it's own size for later)
                textChunkSizeTemp = textChunkEnd - textChunkStart;

                // if curent iteration rank 0, we are on master process, initialize the master text chunk
                if (process == 0)
                {	
                    textChunkSize = textChunkSizeTemp;
                    //allocate space for text chunk
                    
                    textChunkProc = (char *)malloc(textChunkSize * sizeof(char));

                    // TODO memcpy

                    // copy chunk from overall textData into allocate chunk
                    for (int i = textChunkStart, j = 0; i < textChunkEnd, j < textChunkSize; i++, j++)
                    {
                        textChunkProc[j] = textData[i];
                    }

                }
                // otherwise, read the chunk into temporary array from overall textData, send size followed by chunk to current process
                else
                {

                    // send the size of the incoming chunk so that the process may allocate space for it
                    MPI_Send(&textChunkSizeTemp, 1, MPI_INT, process, 0, MPI_COMM_WORLD);
                    // send data from the textData array, starting from the offset "textChunkStart" calculated earlier. This means no temporary array need be allocated
                    MPI_Send(textData + textChunkStart, textChunkSizeTemp, MPI_CHAR, process, 1, MPI_COMM_WORLD);
                }
            }
        
        }
        // slave processes
        else 
        {
            // recv text array size
            MPI_Recv(&textChunkSize, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            
            // if a -1 is sent, the end of the file has been reached
            if (textChunkSize == -1)
            {
                break;
            }
            // if a 0 is sent, the text chunk for this process has size 0 (is empty), so this process will do no work this iteration
            // this only occurs when worldSize is greater than the number of starting positions in the text from which a search may start
            // i.e. worldSize > (textLength - (patternLength -1 ))
            else if (textChunkSize != 0)
            {
                // allocate array for text chunk
                textChunkProc = (char *)malloc(textChunkSize * sizeof(char));

                // receive text array chunk
                MPI_Recv(textChunkProc, textChunkSize, MPI_CHAR, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }


        }

        // searching

        MPI_Barrier(MPI_COMM_WORLD);

        

        //master broadcasts pattern length to all other procs
		MPI_Bcast(&patternLength, 1, MPI_INT, 0, MPI_COMM_WORLD);
		// on processes other than master, allocate patternData to recv from broadcast
		if (worldRank != 0)
			patternData = (char *)malloc(patternLength * sizeof(char));
		
		//master broadcasts pattern to all other procs
		MPI_Bcast(patternData, patternLength, MPI_CHAR, 0, MPI_COMM_WORLD);

        // master broadcast oepration mode to use
        MPI_Bcast(&operationMode, 1, MPI_INT, 0, MPI_COMM_WORLD);

        

        // OPERATION MODE, SWAP FUNCTION TO SEARCH
        if (operationMode == 0)
        {
            // using this method, we get get the rank of process when reducing
            int localResult[2], masterResult[2], totalIndex;

            // store process rank
            localResult[1] = worldRank;
            // don't search if there is an empty chunk for this process
            localResult[0] = textChunkSize != 0 ? searchFirstOccurence() : -1;

            MPI_Reduce(localResult, masterResult, 1, MPI_2INT, MPI_MAXLOC, 0, MPI_COMM_WORLD);
            if (worldRank == 0)
            {
                // if reduce max of every process is -1, then found in 0 occurences in text chunks
                if (masterResult[0] == -1){
                    printf("Pattern number %d not found!\n", patternIndex);
                    fprintf(resultFile,"%d %d %d\n", textIndex, patternIndex, -1);
                }
                else
                {
                    // calculate the start index for this process again, and add the found index for the process
                    totalIndex = calculateOverallIndexInText(masterResult[1], masterResult[0]);
                    printf("Pattern number %d found at position %d in text %d, by process %d\n", patternIndex,  totalIndex, textIndex, masterResult[1]);
                    fprintf(resultFile,"%d %d %d\n", textIndex, patternIndex, -2);
                }
                    
            }
        }
            
        else if (operationMode == 1)
        {   
            //Broadcast these values for calculating index in total text array (so each process may calculate it's own)
            MPI_Bcast(&searchStartPositionsLengthDiv, 1, MPI_INT, 0, MPI_COMM_WORLD);
            MPI_Bcast(&searchStartPositionsLengthMod, 1, MPI_INT, 0, MPI_COMM_WORLD);

            // search
            int found;
            int* foundArray;

            foundArray = searchEveryOccurence(&found);
            // MPI_Gather lengths of the "found" arrays - array containing overall text index for each occurence
            if (!foundArray)
                foundArray = malloc( 0 * sizeof(int)); // this cannot be null for gather
                    

            int *foundLengths = NULL;

            // Only root has the received data 
            if (worldRank == 0)
                foundLengths = malloc( worldSize * sizeof(int)) ;

            // gather lengths of the foundArray from each process
            MPI_Gather(&found, 1, MPI_INT,
                    foundLengths, 1, MPI_INT,
                    0, MPI_COMM_WORLD);

            
            // MPI_Gatherv arrays with index in overall text precalculated by each process

            int totalNumFound = 0;
            int *displacements = NULL;
            int *totalFoundArray = NULL;

            if (worldRank == 0) {
                displacements = malloc( worldSize * sizeof(int) );

                displacements[0] = 0;
                totalNumFound += foundLengths[0];

                for (int i = 1; i < worldSize; i++) {
                    totalNumFound += foundLengths[i];   /* plus one for space or \0 after words */
                    displacements[i] = displacements[i-1] + foundLengths[i-1];
                }

                // allocate array, pre-fill with 0
                totalFoundArray = malloc(totalNumFound * sizeof(int));            
                for (int i = 0; i < totalNumFound; i++)
                    totalFoundArray[i] = 0;
            }


            // gather the arrays of found indexes
            MPI_Gatherv(foundArray, found, MPI_INT,
                totalFoundArray, foundLengths, displacements, MPI_INT,
                0, MPI_COMM_WORLD);

            if (worldRank == 0)
            {
                // if not found
                if(totalNumFound == 0)
                {
                    printf("Not Found\n");
                    fprintf(resultFile,"%d %d %d\n", textIndex, patternIndex, -1);
                }
                else 
                {
                    // iterate through total found array
                    for (int i =0; i< totalNumFound; i++)
                    {
                        printf("%d\n", totalFoundArray[i]);
                        fprintf(resultFile,"%d %d %d\n", textIndex, patternIndex, totalFoundArray[i]);
                    }
                        
                    // MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
                }
                
            }
            
        }
        
        MPI_Barrier(MPI_COMM_WORLD);


    }

	

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



