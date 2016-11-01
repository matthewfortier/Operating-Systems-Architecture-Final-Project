/******************************************************************************
 * main: Sample for starting the FAT project.
 *
 * Authors:  Andy Kinley, Archana Chidanandan, David Mutchler and others.
 *           March, 2004.
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../package/fatSupport.h"
#include "../../commands/pbs/pbs.h"

// 13 is NOT the correct number -- you fix it!
#define BYTES_IN_SECTOR 512

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


int main(int argc, char *argv[]) {
  unsigned char* sect;
  char path[] = "./package/floppy1";
  char filename[9];
  char extension[4];
  int inRange = 0;
  int i;
  int bytes;

  FILE_SYSTEM_ID = fopen(path, "r+");

  if (FILE_SYSTEM_ID == NULL)
  {
     printf("Could not open the floppy drive or image.\n");
     exit(1);
  }
  BYTES_PER_SECTOR = BYTES_IN_SECTOR;
  sect = (unsigned char*) malloc((BYTES_PER_SECTOR) * sizeof(unsigned char));
  bytes = read_sector(19, sect);
  printf("%d\n", bytes);
  for( i = 64; i < 72; i++ ) {
    printf(": %X\n", sect[i]);
    filename[i-64] = sect[i];
  }
  filename[8] = '\0';
  printf("%s\n", filename);

  for( i = 72; i < 75; i++ ) {
    printf(": %X\n", sect[i]);
    extension[i-72] = sect[i];
  }
  extension[3] = '\0';
  printf("%s\n", extension);

  return 0;
}
