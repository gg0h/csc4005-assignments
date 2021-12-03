
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

FILE* resultFile;
int textIndex, patternIndex, operationMode;

void outOfMemory()
{
	fprintf(stderr, "Out of memory\n");
	exit(0);
}

/**
 * @brief - Function to read from a file and allocate space to store the contents of the file in a local array
 * 
 * @param f - file pointer 
 * @param data - pointer to an array 
 * @param length - pointer to variable to store length of file
 */

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
 * @brief Function to search the content read from the text file for the pattern sequence read from the pattern file, looking for the first occurrence of the pattern.
 * Searching is performed using a parallel implementation of a naive pattern searching algorithm
 * Upon finding the occurrence the function will write to the results file textindex, patternIndex, -2
 * if the pattern sequence is not found in the text textindex, patternIndex, -1 is written to the results file
 * 
 * @return int - starting index of the pattern sequence in the text file
 */

int searchForFirstOccurrence() {
    int lastI = textLength - patternLength;  // last index to be checked, any further len of pattern would bypass end of text array

	unsigned int startingMatchIndex = -1; //index at which the pattern match begins, update in loop when found
	printf("Running with %d processors ======================= \n", omp_get_num_procs());

	#pragma omp parallel shared(startingMatchIndex) firstprivate(textData, patternData, lastI, patternLength, resultFile, textIndex, patternIndex) default(none)
	{
		#pragma omp master
		{
			printf("Running with %d threads ======================= \n", omp_get_num_threads());
		}

		// synchronise here for tmpComparison initialization before entering loop
		#pragma omp barrier

		#pragma omp for schedule(static, 100)
		for (int i = 0; i <= lastI; i++) {
			// if the pattern match position has not been updated (found), do work. Otherwise no work. Cannot break in OMP for loop so this removes work from iterations after found.
			if (startingMatchIndex == -1)
			{
				//printf("Parallel For Iteration %d calling from thread %d \n", i, omp_get_thread_num() );
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
					startingMatchIndex = i;
					printf("Match found for pattern starting at index %d \n", startingMatchIndex);
                    // write textIndex, patternindex and a -2 for not found to result file
                    fprintf(resultFile,"%d %d %d\n", textIndex, patternIndex, -2); 
				}
			}
		}

		
	}
    if (startingMatchIndex == -1) {
        printf("Pattern not found in text.\n");
        // write textIndex, patternindex and a -1 for not found to result file
        fprintf(resultFile,"%d %d %d\n", textIndex, patternIndex, -1); 
    }
	return startingMatchIndex;
}

/**
 * @brief Function to search the content read from the text file for the pattern sequence read from the pattern file, looking for every occurrence of the pattern.
 * Searching is performed using a parallel implementation of a naive pattern searching algorithm
 * Upon finding an occurrence of the pattern it will written to the results file with the textindex, patternIndex and the index with the text content at which the pattern starts 
 * if the pattern sequence is not found in the text textindex, patternIndex, -1 is written to the results file
 * 
 * @return int - number of times pattern occured in text
 */

int searchForEveryOccurrence()
{

	int lastI = textLength - patternLength;  // last index to be checked, any further len of pattern would bypass end of text array

	int found = 0; // count of found matches
	printf("Running with %d processors ======================= \n", omp_get_num_procs());

	#pragma omp parallel shared(found) firstprivate(textData, patternData, lastI, patternLength, resultFile, textIndex, patternIndex) default(none)
	{
		#pragma omp master
		{
			printf("Running with %d threads ======================= \n", omp_get_num_threads());
		}

		// synchronise here for tmpComparison initialization before entering loop
		#pragma omp barrier

		#pragma omp for schedule(static, 100)
		for (int i = 0; i <= lastI; i++) {

            //printf("Parallel For Iteration %d calling from thread %d \n", i, omp_get_thread_num() );
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
                // critical here to prevent startingMatchIndex from changing between assignment and print
                #pragma atomic update	
                found += 1;
                printf("Match found for pattern starting at index %d \n", i);
                // write textIndex, patternindex and index of found occurrence to result file
                fprintf(resultFile,"%d %d %d\n", textIndex, patternIndex, i);
            }
		}

		// if found not incremented no match. single construct because we only want to notify once.
		#pragma omp single
		{
			if (found == 0) {
				printf("Pattern not found in text.\n");
                // write textIndex, patternindex and a -1 for not found to result file
                fprintf(resultFile,"%d %d %d\n", textIndex, patternIndex, -1); 
            }
			printf("Found %d occurences of the pattern in the text\n", found);
		}
		
	}
	return found;
}

/**
 * @brief function to perform searches based on the operation mode
 * 
 */
void processData()
{
	unsigned int result;

	printf("Text length = %d\n", textLength);
	printf("Pattern length = %d\n", patternLength);

    if (operationMode == 0) {
        searchForFirstOccurrence();
    } else if (operationMode == 1) {
        searchForEveryOccurrence();
        
    }

	
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

int main(int argc, char **argv)
{

    FILE* controlFile;
    

    char controlFileName[] = "./inputs/control.txt";
    char resultFileName[] = "./result_OMP.txt";

    controlFile = fopen(controlFileName, "r");
    resultFile = fopen(resultFileName, "w");

    if (controlFile == NULL)
    {
        printf("Error reading file %s\n", controlFileName);
        exit(EXIT_FAILURE);
    } else if (resultFile == NULL)
    {
        printf("Error reading file %s\n", resultFileName);
        exit(EXIT_FAILURE);
    }

    char * controlFileLine = NULL;
    size_t lineLength = 0;
    ssize_t read;
    
    // while there are more lines to read in the control file
    while ((read = getline(&controlFileLine, &lineLength, controlFile)) != -1) {
        // if empty lines are reached stop reading
        if( strcmp(controlFileLine, "\r\n") == 0 || strcmp(controlFileLine, "\n") == 0 )
            break;
        
        // printf("Retrieved line of length %zu:\n", read);

        // process line from the control file to extract commands
        parseControlFileLine(controlFileLine);

        // printf("Read from line %s: text %d pattern %d mode %d \n", controlFileLine, textIndex, patternIndex, operationMode );
 
        // using the file index parsed from the control file line, read in the correct pattern and text files
        readData(textIndex, patternIndex);
        printf("Using operation mode %d\n", operationMode);

        // check edge cases of input files (see function for specifics), not found if any of these are true
        if(checkEdgeCases() == 1) {
            printf("Edge case exception reached\n\n");
            fprintf(resultFile,"%d %d %d\n", textIndex, patternIndex, -1);
            continue;
        }
            

        // with files read successfully, use operation mode switch to proceed with how to search 
        processData();
         

        // free allocated text and pattern when finished with them
        free(patternData);
        free(textData);
        printf("\n");
    }


               
    fclose(controlFile);
    fclose(resultFile);

    exit(EXIT_SUCCESS);

}
