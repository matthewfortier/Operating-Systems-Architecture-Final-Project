/*
 * File:   df.c
 * Author: Matthew Fortier
 * Assignment: Final Project Shell
 * Description: Displays info on all sectors
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

#define BYTES_IN_SECTOR 512
#define BYTES_PER_ENTRY 32
#define MAX_SECTORS 2847

void searchFileSystem();

int main(int argc, char** argv)
{
   char *buffer;
   int found;
   // Check arguments
   if (argc != 1)
   {
      printf("Invalid number of arguments: df \n");
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

   searchFileSystem();

   fclose(FILE_SYSTEM_ID);

   return 0;
}

void searchFileSystem()
{
   int fat;
   int sector = 31;
   int blocks = 0;
   int used = 0;
   int bytes;

   char *sect = (char*) malloc(BYTES_PER_SECTOR * sizeof(char));
   char *fat_buffer = (char*) malloc(9 * BYTES_PER_SECTOR * sizeof(char));

   int i = 0;
   int j = 0;

   // Read in the FAT table
   for (i = 1; i <= 9; i++)
   {
      if (read_sector(i, fat_buffer + BYTES_PER_SECTOR * (i - 1)) == -1)
      {
         fprintf(stderr, "Something has gone wrong -- could not read the sector\n");
      }
   }

   while (sector < MAX_SECTORS)
   {
      // Read given sector
      bytes = read_sector(sector, sect);

      // Get the FAT entry
      fat = get_fat_entry(sector, sect);

      // If the sector is not empty, increment used
      if (fat != 0)
      {
         used += 1;
      }

      // Move on to the next sector
      sector++;
   }

   // Display the results
   printf("512K-blocks          Used        Available    Use %%\n");
   printf("%11d %13d %16d %8.2f\n", MAX_SECTORS, used, (MAX_SECTORS - used), ((float) used / (float) MAX_SECTORS) * 100);
}
