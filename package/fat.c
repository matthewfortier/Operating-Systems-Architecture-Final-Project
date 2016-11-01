/******************************************************************************
 * main: Sample for starting the FAT project.
 *
 * Authors:  Andy Kinley, Archana Chidanandan, David Mutchler and others.
 *           March, 2004.
 *****************************************************************************/

#include <stdio.h>
#include "fatSupport.h"

// 13 is NOT the correct number -- you fix it!
#define BYTES_TO_READ_IN_BOOT_SECTOR 512

/******************************************************************************
 * You must set these global variables:
 *    FILE_SYSTEM_ID -- the file id for the file system (here, the floppy disk
 *                      filesystem)
 *    BYTES_PER_SECTOR -- the number of bytes in each sector of the filesystem
 *
 * You may use these support functions (defined in FatSupport.c)
 *    read_sector
 *    write_sector
 *    get_fat_entry
 *    set_fat_entry
 *****************************************************************************/

FILE* FILE_SYSTEM_ID;
int BYTES_PER_SECTOR;
struct PBS pbs;

extern int read_sector(int sector_number, char* buffer);
extern int write_sector(int sector_number, char* buffer);

extern unsigned int  get_fat_entry(int fat_entry_number, char* fat);
extern void set_fat_entry(int fat_entry_number, int value, char* fat);

extern void readBootSector();
extern void printBootSector();

/******************************************************************************
 * main: an example of reading an item in the boot sector
 *****************************************************************************/

int main()
{
   unsigned char* boot;            // example buffer

   int mostSignificantBits;
   int leastSignificantBits;
   int bytesPerSector;

   // You must set two global variables for the disk access functions:
   //      FILE_SYSTEM_ID         BYTES_PER_SECTOR

   // Use this for an image of a floppy drive
   FILE_SYSTEM_ID = fopen("floppy1", "r+");

   if (FILE_SYSTEM_ID == NULL)
   {
      printf("Could not open the floppy drive or image.\n");
      exit(1);
   }

   // Set it to this only to read the boot sector
   BYTES_PER_SECTOR = BYTES_TO_READ_IN_BOOT_SECTOR;

   // Then reset it per the value in the boot sector

   boot = (unsigned char*) malloc(BYTES_PER_SECTOR * sizeof(unsigned char));

   if (read_sector(0, boot) == -1)
      printf("Something has gone wrong -- could not read the boot sector\n");


   // 12 (not 11) because little endian
   mostSignificantBits  = ( ( (int) boot[12] ) << 8 ) & 0x0000ff00;
   leastSignificantBits =   ( (int) boot[11] )        & 0x000000ff;
   bytesPerSector = mostSignificantBits | leastSignificantBits;

  pbs.bytes_per_sector = bytesPerSector;

   printf("%d\n", pbs.bytes_per_sector);

   return 0;
}
