/*
 * File:   pfe.c
 * Author: Matthew Fortier
 * Assignment: Final Project Shell
 * Description: Prints the FAT entires for the clusters provided
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../package/fatSupport.h"
#include "../../commands/pbs/pbs.h"
#include "../../shell.h"

#define BYTES_IN_SECTOR 512

FILE* FILE_SYSTEM_ID;
int BYTES_PER_SECTOR;
struct PBS pbs;

extern int read_sector(int sector_number, char* buffer);
extern int write_sector(int sector_number, char* buffer);

extern unsigned int  get_fat_entry(int fat_entry_number, char* fat);
extern void set_fat_entry(int fat_entry_number, int value, char* fat);

int checkRange(int x, int y)
{
   int inRange = 1;
      // Checks to make sure the provided arguments are valid
   if (x > y)
   {
      printf("ERROR: Left argument cannot be larger than the right arugument\n");
      inRange = 0;
   }
   else if (x < 2)
   {
      printf("ERROR: Left argument must be greater than or equal to 2\n");
      inRange = 0;
   }

      // Returns "Boolean" for whether if is in range
   return inRange;

}

int main(int argc, char *argv[])
{
   unsigned char* sect;
   char path[] = FLOPPY;
   int inRange = 0;
   int i;
   int bytes;

      // Checks the arguments
   if (argc > 3)
   {
      printf("ERROR: Too many arugments\n");
      exit(1);
   }
   else if (argc < 3)
   {
      printf("ERROR: Too few arugments\n");
      exit(1);
   }
   else {
      if (checkRange(atoi(argv[1]), atoi(argv[2])))
      {
         FILE_SYSTEM_ID = fopen(path, "r+");

         if (FILE_SYSTEM_ID == NULL)
         {
            printf("Could not open the floppy drive or image.\n");
            exit(1);
         }
         BYTES_PER_SECTOR = BYTES_IN_SECTOR;
         sect = (unsigned char*) malloc((9 * BYTES_PER_SECTOR) * sizeof(unsigned char));                // Reads in all of the first FAT table
         for ( i = 1; i<= 9; i++ )
         {
            bytes = read_sector(i, sect + (BYTES_PER_SECTOR * sizeof(unsigned char)) * (i-1));                     // Reads each sector and adds it to the end of the buffer
         }
         for ( i = atoi(argv[1]) + 1; i <= atoi(argv[2]); i++ )
         {
            printf("Entry %d: %X\n", i, get_fat_entry(i, sect));                     // Prints the fat_entry for each sector between the arguments
         }
      }
   }
   return 0;
}
