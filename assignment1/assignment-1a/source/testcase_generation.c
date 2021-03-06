#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>


/*
* This program will generate worst case text and pattern pair files for a naive 
* sequential search by parsing the respective lengths for each in pairs, line by line 
* from an input text file
* 
* Usage:
* ./testcase_generation </path/to/text_and_pattern_length_pairs.txt> </path/to/output_directory/>
*
* where text_and_pattern_length_pairs.txt is a text file in which each line contains
* the length of the text, a space, and the length of the pattern. An example line
* might look like:
* "20 5"
*
* An example file of the correct format has been included in the current directory 
* to clear any misunderstandings
*
* "path/to/output_directory/" should be a valid path to a directory, where the 
* resulting text and pattern files will be written
*/

// max length of lines in pair input file, more than enough for our needs.
#define MAX_LINE_LENGTH 100

//functions
void processLine(char * line, int pairNumber);
struct pairLengths parsePairString(char * pairString);
void generateOutFile(char * outputDirectory,char * outputFilename, long int size);
char * generateWorstCaseString(long int size);

// struct for pair length info
struct pairLengths{
    long int textLength;
    long int patternLength;
};

char * inputFilePath;
char * outputBasePath;

int main(int argc, char * argv[])
{

    // argument parsing
    if (argc != 3)
    {
        printf("Usage:  ./testcase_generation </path/to/text_and_pattern_length_pairs.txt> </path/to/output_directory>\n");
        exit(1);
    }
    inputFilePath = argv[1];
    outputBasePath = argv[2];


    // read the text and pattern lengths line by line from the input file

    FILE* filePointer;
    char line[MAX_LINE_LENGTH]; // we are assuming here a line in the input file will not be greater than 100 chars 

    // open input file, verify can read

    filePointer = fopen(inputFilePath, "r");

    if (filePointer == NULL)
    {
        printf("Error reading file %s\n", inputFilePath);
        return 1;
    }

    int pairNumber = 0;

    // read the file line by line, generating the text and pattern files from the values on each line
    while(fgets(line, MAX_LINE_LENGTH, filePointer)) {
        printf("%s\n", line);

        processLine(line, pairNumber);
        pairNumber++;
    }

    fclose(filePointer);
    return 0;
}

void processLine(char * line, int pairNumber)
{
    //parse length pair string into struct with both lengths
    struct pairLengths pair = parsePairString(line);

    //generate output directory test<pairNum>
    char outputDirectory[256]; // concat onto base dir
    char dirName[100];
    sprintf(dirName, "/test%d", pairNumber);  // CHECK
    strncpy(outputDirectory, outputBasePath, 256);  // copy the outputBasePath cmdline arg into outputDirectory buffer
    strncat(outputDirectory, dirName, sizeof(outputDirectory) - strlen(outputDirectory) - 1); // safely concat the current testx dir to the outputDirectory

    if ((mkdir(outputDirectory, 0700)))
    {
        // if exists, quit
        printf("Directory %s already exists", outputDirectory);
        exit(1);
    }
        

    //generate text file
    generateOutFile(outputDirectory, "/text.txt", pair.textLength);

    //generate pattern file
    generateOutFile(outputDirectory, "/pattern.txt", pair.patternLength);
}

struct pairLengths parsePairString(char * pairString)
{
    struct pairLengths pair;
    char *end;

    // delimiter to split the string
    char * delim = " ";

    char *ptr = strtok(pairString, delim);
    pair.textLength = strtol(ptr, &end, 10); 
	ptr = strtok(NULL, delim);
    pair.patternLength = strtol(ptr, &end, 10);


    return pair;
}


char * generateWorstCaseString(long int size)
{
    
    if (size == 0)
    {    
        printf("Error: cannot use a size 0 pattern/text file");
        exit(1);
    }
    // allocate on heap size for the string
    char * content = malloc(size);
    if(!content)
    {
        printf("cannot allocate %ld bytes\n", size);
        exit(1);
    }
    if (size == 1)
    {
        // string 1 char, just need a B
        content[0] = 'B';
        
    }    
    else
    {
        // make every character except the last an A, last char a B
        for (int i = 0; i < size -1; i++)
        {
            content[i] = 'A';
        }
        content[size -1] = 'B';
    }
    return content;
}

void generateOutFile(char * outputDirectory,char * outputFilename, long int size)
{
    // string operations to get path to the output file
    char outputFilePath[256];
    // concat filename to outputDirectory
    strncpy(outputFilePath, outputDirectory, 256);
    strncat(outputFilePath, outputFilename, sizeof(outputFilePath) - strlen(outputFilePath) - 1);

    // get a file handle to output file
    char * content;
    FILE * outFile;
    
    outFile = fopen(outputFilePath, "w");

    //pointer to address with generated string
    content = generateWorstCaseString(size);
    //write string to file
    fwrite(content, sizeof(char), size, outFile);
    // free heap space allocated to content in function generateWorstCaseString
    free(content);

    // close file pointer
    fclose(outFile);
}