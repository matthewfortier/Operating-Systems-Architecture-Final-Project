/*
 * File:   mkdir.c
 * Author: Anthony Taylor
 * Assignment: Final Project Shell
 * Description: Creates new directory
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
#include "../../package/fatSupport.h"
#include "../../shell.h"

FILE* FILE_SYSTEM_ID;
struct PATH *global_path;
char *file_info[2]; // To hold the filename and extension

#define BYTES_IN_SECTOR 512
#define BYTES_PER_ENTRY 32

int parseFilename(char* input);
void fillNewDir(int newDir, int aboveDir);
int searchForFile(char* buffer, int cluster);
void createDirectory(char* buffer, int cluster);
int checkDirectory(char *directory, int cluster);
int countDirectories(char** directories);
char * addSpaces(char* directory);
char * generatePath(char** directories, int directoryCount);
int read_cluster(int marker, unsigned char* sect, char* directory);
char *trimwhitespace(char *str);


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
   int entry = 1;
   int check = 1;
   int cluster;
   char *temp = global_path->cwd;

	buffer = (char*) malloc(32 * sizeof(char));

	//test for too many
	if (argv[2] != NULL && argv[1] != NULL)
   {
		printf("Too many arguements given\n");
		return;
   }

   // Test for no arguments
   if (argv[1] == NULL)
   {
		printf("No arguements given\n");
   }

   // Test for root directory
   else if (strcmp(argv[1], "/") == 0)
   {
      printf("Can not create root directory!!\n");
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



		entry = searchForFile(buffer, cluster);

		if (entry == 1)
		 {
			printf("Directory already Exists\n");
		 }
		 else
		 {
			buffer = (char*) malloc(32 * sizeof(char));
			createDirectory(buffer, cluster);
			printf("Directory created\n");
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

		if (entry == 1)
		 {
			printf("Directory already Exists\n");
		 }
		 else
		 {
			buffer = (char*) malloc(32 * sizeof(char));
			createDirectory(buffer, cluster);
			printf("Directory created\n");
		 }
	}
}

void fillNewDir(int newDir, int aboveDir)
{

   int fat;
   int sector = newDir;
   int attributes;
	char* entry = (char*) malloc(BYTES_PER_ENTRY * sizeof(char));

   char *sect = (char*) malloc(BYTES_PER_SECTOR * sizeof(char));

   int i = 0;
   int j = 0;
	int s = 0;
   int bytes;
   int success = 0;

      // Read in current sector
      if (sector == 0)
      {
         bytes = read_sector(sector + 19, sect);
      }
      else
      {
         bytes = read_sector(sector + 31, sect);
      }

      i = 0;

		// Read each entry
		for (j = 0; j < BYTES_PER_ENTRY; j++)
		{
			entry[j] = sect[i + j];
		}

		//. entry pointing to current dir
		//Name and extension
		entry[0] = '.';
		entry[1] = '\0';
		entry[8] = '\0';

		// Set file attributes
		entry[11] = 0x10;

		//FLC
		entry[26] = newDir & 255;
		entry[27] = (newDir >> 8) & 255;

		//FILE size
		for(s = 28; s < BYTES_PER_ENTRY; s++)
		{
			entry[s] = 0;
		}

		// Write entry to sector
		for (j = 0; j < BYTES_PER_ENTRY; j++)
		{
			sect[i + j] = entry[j];
		}

		//Move to next entry
		i+=BYTES_PER_ENTRY;
		entry = (char*) malloc(BYTES_PER_ENTRY * sizeof(char));

		// Read each entry
		for (j = 0; j < BYTES_PER_ENTRY; j++)
		{
			entry[j] = sect[i + j];
		}

		//.. entry pointing to above dir
		//Name and extension
		entry[0] = '.';
		entry[1] = '.';
		entry[2] = '\0';
		entry[8] = '\0';

		// Set file attributes
		entry[11] = 0x10;

		//FLC
		entry[26] = aboveDir & 255;
		entry[27] = (aboveDir >> 8) & 255;

		//FILE size
		for(s = 28; s < BYTES_PER_ENTRY; s++)
		{
			entry[s] = 0;
		}

		// Write entry to sector
		for (j = 0; j < BYTES_PER_ENTRY; j++)
		{
			sect[i + j] = entry[j];
		}

		if (sector == 0)
		{
			write_sector(sector + 19, sect);
		}
		else
		{
			write_sector(sector + 31, sect);
		}

}

int parseFilename(char* input)
{
   char *temp;
   int i = 0;

   temp = strtok(input, " .");
   while (temp != NULL)
   {
		printf("PARSE i = %d TEMP = %s\n", i, temp);
      file_info[i] = malloc(sizeof(temp));
      strcpy(file_info[i], temp);
      temp = strtok(NULL, " .");
      i++;
   }

	if(file_info[1] == NULL)
	{
		file_info[1] = " ";
	}

   return 0;
}

int searchForFile(char* entry, int cluster)
{
   int fat;
   int sector = cluster;
   int attributes;

   char *sect = (char*) malloc(BYTES_PER_SECTOR * sizeof(char));
   char *fat_buffer = (char*) malloc(9 * BYTES_PER_SECTOR * sizeof(char));

   // Make strings for later comparisons
   char *current = (char*) malloc(strlen(file_info[0]) + strlen(file_info[1]) * sizeof(char));
   char *given = (char*) malloc(strlen(file_info[0]) + strlen(file_info[1]) * sizeof(char));

   int i = 0;
   int j = 0;
   int bytes;

	if (FILE_SYSTEM_ID == NULL)
	{
		printf("Could not open the floppy drive or image.\n");
		exit(1);
	}

   // Read in the FAT table
   for (i = 1; i <= 9; i++)
   {
      if (read_sector(i, fat_buffer + BYTES_PER_SECTOR * (i - 1)) == -1)
      {
         fprintf(stderr, "Something has gone wrong -- could not read the sector\n");
      }
   }

   // Search sector and potentially following sectors
   while (1)
   {
      // Read in current sector
      if (cluster == 0)
      {
         bytes = read_sector(sector + 19, sect);
      }
      else
      {
         bytes = read_sector(sector + 31, sect);
      }

		i = 0;
      // Search each entry in current sector
      for (i = 0; i < 512; i += 32)
      {

         // Reset entry for next entry
         entry = (char*) malloc(BYTES_PER_ENTRY * sizeof(char));

         // Load in entry to entry buffer
         for (j = 0; j < BYTES_PER_ENTRY; j++)
         {
            entry[j] = sect[j + i];
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

         // Add null terminating characters
         current[strlen(file_info[0]) + strlen(file_info[1])] = '\0';
         given[strlen(file_info[0]) + strlen(file_info[1])] = '\0';

         // Compare the two filenames
         if (strcmp(current, given) == 0)
         {
            // Found
            return 1;
         }
      }

      // Determine if we should move on to next sector
      fat = get_fat_entry(sector, fat_buffer);

      if (fat >= 0xFF8 && fat <= 0xFFF) // Last cluster
         break;
      else if (fat >= 0xFF0 && fat <= 0xFF6) // Reserved cluster
         break;
      else if (fat == 0x00) // Unused cluster
         break;
      else if (fat == 0xFF7) // Unused cluster
         break;
      else
         sector = fat;
   }

   free(sect);
   sect = NULL;
   free(entry);
   entry = NULL;

   return 0;
}

void createDirectory(char* entry, int cluster)
{
   int fat;
   int sector = cluster;
   int attributes;

   char *sect = (char*) malloc(BYTES_PER_SECTOR * sizeof(char));
   char *fat_buffer = (char*) malloc(9 * BYTES_PER_SECTOR * sizeof(char));

   int i = 0;
   int j = 0;
	int d = 0;
   int bytes;
   int success = 0;

	if (FILE_SYSTEM_ID == NULL)
	{
		printf("Could not open the floppy drive or image.\n");
		exit(1);
	}

   // Read in the FAT table
   for (i = 1; i <= 9; i++)
   {
      if (read_sector(i, fat_buffer + BYTES_PER_SECTOR * (i - 1)) == -1)
      {
         fprintf(stderr, "Something has gone wrong -- could not read the sector\n");
      }
   }
	for ( d = 2 + 1; d <= 20; d++ )
	{
		printf("Entry %d: %X\n", d, get_fat_entry(d, fat_buffer)); // Prints the fat_entry for each sector between the arguments
	}

   while (1)
   {
      // Read in current sector
      if (cluster == 0)
      {
         bytes = read_sector(sector + 19, sect);
      }
      else
      {
         bytes = read_sector(sector + 31, sect);
      }

      i = 0;
      while (i < BYTES_PER_SECTOR)
      {
         // Read each entry
         for (j = 0; j < BYTES_PER_ENTRY; j++)
         {
            entry[j] = sect[i + j];
         }

         // Check the first byte of each filename for empty entries
         if (entry[0] == 0xE5 || entry[0] == 0x00)
         {
            // If there is an empty entry, start overwriting the entry

            // Clear filename
            for (j = 0; j < 8; j++)
            {
               entry[j] = 32;
            }

            // Set the filename
            for (j = 0; j < strlen(file_info[0]); j++)
            {
               entry[j] = file_info[0][j];
            }

            // Set file extension
            for (j = 0; j < strlen(file_info[1]); j++)
            {
               entry[j + 8] = file_info[1][j];
            }

            // Set file attributes
            entry[11] = 0x10;

				//Find open FAT position
				d = 3;
				while(fat != 0)
				{
					fat = get_fat_entry(d, fat_buffer);
					d++;
				}

				set_fat_entry(d - 1, '\xFFF', fat_buffer);

				entry[26] = (d - 1) & 255;
            entry[27] = ((d - 1) >> 8) & 255;

				fillNewDir((d-1), cluster);

            // Write entry to sector
            for (j = 0; j < BYTES_PER_ENTRY; j++)
            {
               sect[j + i] = entry[j];
            }

            // Write the sector

            if (cluster == 0)
            {
               write_sector(sector + 19, sect);
            }
            else
            {
               write_sector(sector + 31, sect);
            }

				// Writes in all of the first FAT table
				for ( d = 1; d <= 9; d++ )
				{
					bytes = write_sector(d, fat_buffer + (BYTES_PER_SECTOR * sizeof(unsigned char)) * (d-1));                     // Reads each sector and adds it to the end of the buffer
				}


            success = 1;

            break;
         }
         i += 32;
      }

      // If the sector was full, move to the next one
      if (success)
      {
         break;
      }
      else
      {
         sector++;
      }
   }

   //Clear up some space
   free(entry);
   entry = NULL;
   free(sect);
   sect = NULL;
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
