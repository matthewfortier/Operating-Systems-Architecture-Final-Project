/*
 * File:   touch.c
 * Author: Matthew Fortier
 * Assignment: Final Project Shell
 * Description: Creates new new file
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
int searchForFile(char* buffer);
void createFile(char* buffer);

int main(int argc, char** argv)
{
   char *buffer;
   int found;
   // Check arguments
   if (argc != 2)
   {
      printf("Invalid number of arguments: touch x \n");
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
   buffer = (char*) malloc(32 * sizeof(char));

   found = searchForFile(buffer);

   if (found)
   {
      printf("ERROR: File Already Exists\n");
   }
   else
   {
      // Reset the buffer before starting
      buffer = (char*) malloc(32 * sizeof(char));
      createFile(buffer);
   }

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

int searchForFile(char* entry)
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

void createFile(char* entry)
{
   int fat;
   int sector = global_path->cluster;
   int attributes;

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
            entry[11] = 0x20;

            entry[26] = sector & 255;
            entry[27] = (sector >> 8) & 255;

            // Write entry to sector
            for (j = 0; j < BYTES_PER_ENTRY; j++)
            {
               sect[j + i] = entry[j];
            }

            // Write the sector

            if (global_path->cluster == 0)
            {
               write_sector(sector + 19, sect);
            }
            else
            {
               write_sector(sector + 31, sect);
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
