/*
 * File:   rm.c
 * Author: Anthony Taylor
 * Assignment: Final Project Shell
 * Description: Deletes a file based on arguements passed in and the current working directory
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <errno.h>
#include <limits.h>
#include "rm.h"
#include "../../package/fatSupport.h"
#include "../../shell.h"

FILE* FILE_SYSTEM_ID;
struct PATH *global_path;
char *file_info[2]; // To hold the filename and extension

#define BYTES_IN_SECTOR 512
#define BYTES_PER_ENTRY 32
#define UNUSED 0

int main(int argc, char** argv)
{
// SHARED MEMORY
   int shmid;
   key_t key = KEY;
   size_t shmsize = sizeof(struct PATH);

   shmid = shmget(key, shmsize, 0666);
   if(shmid < 0)
   {
      perror("shmget");
      exit(1);
   }
   global_path = shmat(shmid, (void *) 0, 0);
   if(global_path == (void *) -1)
   {
      perror("shmat");
      exit(1);
   }


   char path[] = FLOPPY;
   unsigned char* sect;
   FILE_SYSTEM_ID = fopen(path, "r+");
   char** directories;
   char* directory = "";
   char** tempDirectories;
   int directoryCount;
   int i;
   char *buffer;
   int entry;
   int check = 1;
   int cluster;
   char *temp = global_path->cwd;

	//test for too many
	if (argv[2] != NULL && argv[1] != NULL)  //broken
   {
		printf("Too many arguements given\n");
   }

   // Test for no arguments
   if (argv[1] == NULL)
   {
		printf("No arguements given\n");
   }

   // Test for root directory
   else if (strcmp(argv[1], "/") == 0)
   {
      printf("Can not delete root directory!!\n");
   }

	// Test for absolute path
   else if (argv[1][0] == '/')
   {
	   // start at root cluster
      cluster = 0;
	  // gets an array of all directories
      directories = parseInput(argv[1], "/");
      directoryCount = countDirectories(directories);

		for ( i = 0; i < directoryCount - 1; i++)
		 {
			  cluster = checkDirectory(addSpaces(directories[i]), cluster);
		 }

		puts(directories[i]);
		parseFilename(directories[i]);
		puts(directories[i]);

		buffer = (char*) malloc(32 * sizeof(char));
		entry = searchForFile(buffer, cluster);
		entry /= 32;

		if (entry >= 0)
		 {
			printf("File Removed\n");
		 }
		 else
		 {
			printf("ERROR: File Doesnt Exist\n");
		 }

   }
	//Test for relative path
   else
   {
	   // start at current cluster
      cluster = global_path->cluster;

		puts(argv[1]);
		parseFilename(argv[1]);
		puts(argv[1]);

		buffer = (char*) malloc(32 * sizeof(char));
		entry = searchForFile(buffer, cluster);
		entry /= 32;

		if (entry >= 0)
		 {
			printf("File Removed\n");
		 }
		 else
		 {
			printf("ERROR: File Doesnt Exist\n");
		 }
	}
}

int searchForFile(char* entry, int cluster)
{
   int fat;
	unsigned int nextFat = 0;
   int sector = cluster;
   int attributes;
	int mostSignificantBits;
   int leastSignificantBits;
	int FLC;
	char path[] = FLOPPY;

	char *entryAfter = (char*) malloc(BYTES_PER_ENTRY * sizeof(char));
   char *sectList = (char*) malloc(BYTES_PER_SECTOR * sizeof(char));
   char *sect = (char*) malloc(BYTES_PER_SECTOR * sizeof(char));;

   // Make strings for later comparisons
   char *current = (char*) malloc(strlen(file_info[0]) + strlen(file_info[1]) * sizeof(char));
   char *given = (char*) malloc(strlen(file_info[0]) + strlen(file_info[1]) * sizeof(char));

   int i = 0;
   int j = 0;
	int d = 0;
   int bytes;

	//FILE_SYSTEM_ID = fopen(path, "r+");

	if (FILE_SYSTEM_ID == NULL)
	{
		printf("Could not open the floppy drive or image.\n");
		exit(1);
	}

	BYTES_PER_SECTOR = BYTES_IN_SECTOR;
	sect = (unsigned char*) malloc((9 * BYTES_PER_SECTOR) * sizeof(unsigned char));

	// Reads in all of the first FAT table
	for ( d = 1; d <= 9; d++ )
	{
		bytes = read_sector(d, sect + (BYTES_PER_SECTOR * sizeof(unsigned char)) * (d-1));                     // Reads each sector and adds it to the end of the buffer
	}

	// Search sector and potentially following sectors
   while (1)
   {
      // Read in current sector
      if (cluster == 0)
      {
         bytes = read_sector(sector + 19, sectList);
      }
      else
      {
         bytes = read_sector(sector + 31, sectList);
      }



      // Search each entry in current sector
      for (i = 0; i < BYTES_PER_SECTOR; i += BYTES_PER_ENTRY)
      {
         // Reset entry for next entry
         entry = (char*) malloc(BYTES_PER_ENTRY * sizeof(char));

         // Load in entry to entry buffer
         for (j = 0; j < BYTES_PER_ENTRY; j++)
         {
            entry[j] = sectList[j + i];
         }

         // Load filename from entry
         for (j = 0; j < strlen(file_info[0]); j++)
         {
            current[j] = entry[j];
         }

         // Load extension
         for (j = 0; j < strlen(file_info[1]); j++)
         {
            current[strlen(file_info[0]) + j] = entry[j + 8];
         }

         // Load attributes
         attributes = entry[11];

         // Load given filename and extension
			strcpy(given, "");
         strcat(given, file_info[0]);
         strcat(given, file_info[1]);

			// Get the FLC
			mostSignificantBits  = ( ( (int) entry[27] ) << 8 ) & 0x0000ff00;
			leastSignificantBits =   ( (int) entry[26] )        & 0x000000ff;
			FLC = mostSignificantBits | leastSignificantBits;

         // Add null terminating characters
         current[strlen(file_info[0]) + strlen(file_info[1])] = '\0';
         given[strlen(file_info[0]) + strlen(file_info[1])] = '\0';

         // Compare the two filenames
         if (strcmp(current, given) == 0)
			{
				fat = FLC;

				// Reads in all of the first FAT table
				for ( d = 1; d <= 9; d++ )
				{
					bytes = read_sector(d, sect + (BYTES_PER_SECTOR * sizeof(unsigned char)) * (d-1));                     // Reads each sector and adds it to the end of the buffer
				}

				//Remove all clusters belonging to file
				while(fat != 4095 && fat != 0)
				{
					nextFat = get_fat_entry(fat, sect);
					set_fat_entry(fat, '\x00', sect);
					fat = nextFat;
				}

				//Set first byte
				entry[0] = '\xE5';
				bytes = 0;

				// Write entry to sector
            for (j = 0; j < BYTES_PER_ENTRY; j++)
            {
               sectList[j + i] = entry[j];
            }

				if (sector == 0)
				{
					bytes = write_sector(cluster + 19, sectList);

				}
				else
				{
					bytes = write_sector(cluster + 31, sectList);

				}

				// Writes in all of the first FAT table
				for ( d = 1; d <= 9; d++ )
				{
					bytes = write_sector(d, sect + (BYTES_PER_SECTOR * sizeof(unsigned char)) * (d-1));                     // Reads each sector and adds it to the end of the buffer
				}
				return i;
			}
      }
		return -32;
   }

   free(sect);
   sect = NULL;
   free(entry);
   entry = NULL;

   return -32;
}

int parseFilename(char* input)
{
   char *temp;
   int i = 0;

   temp = strtok(input, " .");
   while (temp != NULL)
   {
      file_info[i] = malloc(sizeof(temp));
      strcpy(file_info[i], temp);
      temp = strtok(NULL, " .");
      i++;
   }

   return 0;
}

int checkDirectory(char *directory, int cluster)
{
   char path[] = FLOPPY;
   unsigned char* sect;
   int bytes;
   int FLC;
   int i;
   FILE_SYSTEM_ID = fopen(path, "r+");

   if (FILE_SYSTEM_ID == NULL)
   {
      printf("Could not open the floppy drive or image.\n");
      exit(1);
   }
   BYTES_PER_SECTOR = BYTES_IN_SECTOR;

   sect = (unsigned char*) malloc((BYTES_PER_SECTOR) *sizeof(unsigned char));
   if (cluster == 0)
   {
      bytes = read_sector(cluster + 19, sect);          // Reads in correct sector depending on if cluster is pointing to root
   }
   else {
      bytes = read_sector(cluster + 31, sect);
   }

   for (i = 0; i < BYTES_PER_SECTOR; i += 32)       // Loops through 32 byte entires and reads the cluster
   {
      FLC = read_cluster(i, sect, directory);           // Returns the First Logical Cluster for the directory given
      if (FLC >= 0)            // Returns -1 if directory does not exists and returns it
      {
         return FLC;
      }
      else if (FLC == -2)
      {
         break;
      }
   }

   fclose(FILE_SYSTEM_ID);
   free(sect);

   return -1;
}

int read_cluster(int marker, unsigned char* sect, char* directory)
{
   int i;
   char filename[9];
   char extension[4];
   char *fullName;
   char *tempString;
   int attributes;
   int FLC;
   int size;
   char type[5];
   int mostSignificantBits;
   int leastSignificantBits;
   int temp_bits1;
   int temp_bits2;
   int temp;

// Get the filename for this entry
   for (i = marker; i < marker + 7; i++)
   {
      filename[i - marker] = sect[i];
   }

// If the first byte is 0x00, then there are no more entries
   if (filename[0] == 0x00)
   {
      return -1;
   }

   filename[7] = '\0';      // Make sure to add null terminating character

// Get the file extension for this entry
   for (i = marker + 8; i < marker + 11; i++)
   {
      extension[i - (marker + 8)] = sect[i];
   }

   extension[3] = '\0';     // Make sure to add null terminating character

   tempString = trimwhitespace(filename);
   strcpy(fullName, tempString);
   tempString = trimwhitespace(extension);
   strcat(fullName, tempString);
   directory = trimwhitespace(directory);

   attributes = sect[marker + 11];      // Set the attributes for this entry

// Get the FLC
   mostSignificantBits  = ( ( (int) sect[marker + 27] ) << 8 ) & 0x0000ff00;
   leastSignificantBits =   ( (int) sect[marker + 26] )        & 0x000000ff;
   temp = mostSignificantBits | leastSignificantBits;

   FLC = temp;
   if (strcmp(fullName, directory) == 0)
   {
      if(attributes != 0x10)
      {
         return -2;
      }
      else
      {
         return FLC;
      }
   }

   return -1;
}

int countDirectories(char** directories)
{
   int i = 0;
   int count = 0;

   while(directories[i] != NULL)
   {
      count++;
      i++;
   }
   return count;
}

char ** parseInput(char line[], const char *delimiter)
{

   int k = 0;
   int count = 0;
   char *ptr = line;

   while((ptr = strchr(ptr, (intptr_t) delimiter)) != NULL)
   {
      count += 2;
      ptr++;
   }

// creates an array with the number of arguments in the commands
   char** temp = malloc(count * sizeof(char*));
   if(temp == NULL)
   {
      perror("parseInput memory allocation error");
   }

   int i = 1;
   char* cmd = strtok(line, delimiter);     // gets the first token of the line

   while (cmd != NULL)      // continues to get the next token until a null value
   {

      temp[i-1] = cmd;          // assigns each valid argument to the array
      cmd = strtok(NULL, "/");          // gets the next token

	  //printf("%c", cmd[i]);

      if(cmd == NULL)
      {
         temp[i] = NULL;
      }

	  i++;
   }

   return temp;
}

char * addSpaces(char* directory)
{
   static char filename[8];
   memset(filename, 32, 7);
   int i;
   for (i = 0; i < strlen(directory); i++)
   {
      filename[i] = directory[i];
   }
   filename[8] = '\0';
   return filename;
}

//http://stackoverflow.com/questions/122616/how-do-i-trim-leading-trailing-whitespace-in-a-standard-way
char *trimwhitespace(char *str)
{
   char *end;

      // Trim leading space
   while(isspace((unsigned char)*str))
      str++;

   if(*str == 0) // All spaces?
      return str;

      // Trim trailing space
   end = str + strlen(str) - 1;
   while(end > str && isspace((unsigned char)*end))
      end--;

      // Write new null terminator
   *(end+1) = 0;

   return str;
}
