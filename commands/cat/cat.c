/*
 * File:   cat.c
 * Author: Matthew Fortier
 * Assignment: Final Project Shell
 * Description: Displays file contents
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
char* searchForFile(char* buffer);
void createFile(char* buffer);
void displayFile(char* buffer);

int main(int argc, char** argv)
{
   char *buffer;
   int found = 0;
   // Check arguments
   if (argc != 2)
   {
      printf("Invalid number of arguments: cat x \n");
      return 1;
   }

   // Parse filename
   if (parseFilename(argv[1]) != 0)
   {
      printf("Invalid filename\n");
      return 1;
   }

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

   if (FILE_SYSTEM_ID == NULL)
   {
      printf("Could not open the floppy drive or image.\n");
      exit(1);
   }
   BYTES_PER_SECTOR = BYTES_IN_SECTOR;

   // Get the entry that contains the requested file
   buffer = (char*) malloc(32 * sizeof(char));
   buffer = searchForFile(buffer);
   displayFile(buffer);

   fclose(FILE_SYSTEM_ID);

   return 0;
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

char* searchForFile(char* entry)
{
   int fat;
   int sector = global_path->cluster;
   int attributes;

   char *sect = (char*) malloc(BYTES_PER_SECTOR * sizeof(char));
   char *fat_buffer = (char*) malloc(9 * BYTES_PER_SECTOR * sizeof(char));

   // Make strings for later comparisons
   char *current = (char*) malloc(strlen(file_info[0]) + strlen(file_info[1]) * sizeof(char));
   char *given = (char*) malloc(strlen(file_info[0]) + strlen(file_info[1]) * sizeof(char));

   int i = 0;
   int j = 0;
   int bytes;

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
      if (global_path->cluster == 0)
      {
         bytes = read_sector(sector + 19, sect);
      }
      else
      {
         bytes = read_sector(sector + 31, sect);
      }

      // Search each entry in current sector
      for (i = 0; i < BYTES_PER_SECTOR; i += BYTES_PER_ENTRY)
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
         strcat(given, file_info[0]);
         strcat(given, file_info[1]);

         // Add null terminating characters
         current[strlen(file_info[0]) + strlen(file_info[1])] = '\0';
         given[strlen(file_info[0]) + strlen(file_info[1])] = '\0';

         // Compare the two filenames
         if (strcmp(current, given) == 0)
         {
            // Found
            return entry;
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

   return entry;
}

void displayFile(char* buffer)
{
   int fat;
   int sector;
   int temp1, temp2;
   int attributes;

   // Allocate the buffers
   char *sect = (char*) malloc(BYTES_PER_SECTOR * sizeof(char));
   char *fat_buffer = (char*) malloc(9 * BYTES_PER_SECTOR * sizeof(char));

   int i = 0;
   int j = 0;
   int bytes;
   int success = 0;

   // Read in the FAT table
   for (i = 1; i <= 9; i++)
   {
      if (read_sector(i, fat_buffer + BYTES_PER_SECTOR * (i - 1)) == -1)
      {
         fprintf(stderr, "Something has gone wrong -- could not read the sector\n");
      }
   }

   // Set the sector from the buffer
   temp1  = ( ( (int) buffer[27] ) << 8 ) & 0x0000ff00;
   temp2 =   ( (int) buffer[26] )        & 0x000000ff;
   sector = (temp1 | temp2) + 33 - 2;

   while (1)
   {
      // Read entry sector
      read_sector(sector, sect);

      // Print the file contents
      printf("%s\n", sect);

      // Set the fat
      fat = get_fat_entry(sector - 33 + 2, fat_buffer);

      if (fat >= 0xFF8 && fat <= 0xFFF) // Last cluster
         break;
      else if (fat >= 0xFF0 && fat <= 0xFF6) // Reserved cluster
         break;
      else if (fat == 0x00) // Unused cluster
         break;
      else if (fat == 0xFF7) // Unused cluster
         break;
      else
         sector = fat + 33 - 2;
   }

   // Lets clear some space
   free(sect);
   sect = NULL;
   free(buffer);
   buffer = NULL;

}
