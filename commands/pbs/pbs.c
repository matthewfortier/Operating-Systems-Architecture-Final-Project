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
#include "pbs.h"

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

void readBootSector(){
  unsigned char* boot;            // example buffer

  int mostSignificantBits;
  int leastSignificantBits;
  int temp_bits1;
  int temp_bits2;
  int temp;
  int cx;
  int i = 0;
  int count = 0;
  char path[] = "./package/floppy1";
  char label[11];
  char type[8];

  // You must set two global variables for the disk access functions:
  //      FILE_SYSTEM_ID         BYTES_PER_SECTOR

  // Use this for an image of a floppy drive
  FILE_SYSTEM_ID = fopen(path, "r+");

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


  // Bytes per sector
  mostSignificantBits  = ( ( (int) boot[12] ) << 8 ) & 0x0000ff00;
  leastSignificantBits =   ( (int) boot[11] )        & 0x000000ff;
  temp = mostSignificantBits | leastSignificantBits;

  pbs.bytes_per_sector = temp;

  // Sectors per cluster
  pbs.sectors_per_cluster = (int) boot[13];

  // Number of reserved sectors
  mostSignificantBits  = ( ( (int) boot[15] ) << 8 ) & 0x0000ff00;
  leastSignificantBits =   ( (int) boot[14] )        & 0x000000ff;
  temp = mostSignificantBits | leastSignificantBits;

  pbs.nurs = temp;

  // Sectors per cluster
  pbs.nof = (int) boot[16];

  // Maximum number of root directory entries
  mostSignificantBits  = ( ( (int) boot[18] ) << 8 ) & 0x0000ff00;
  leastSignificantBits =   ( (int) boot[17] )        & 0x000000ff;
  temp = mostSignificantBits | leastSignificantBits;

  pbs.nore = temp;

  // Total sector count
  mostSignificantBits  = ( ( (int) boot[20] ) << 8 ) & 0x0000ff00;
  leastSignificantBits =   ( (int) boot[19] )        & 0x000000ff;
  temp = mostSignificantBits | leastSignificantBits;

  pbs.total_sector_count = temp;

  // Sectors per FAT
  mostSignificantBits  = ( ( (int) boot[23] ) << 8 ) & 0x0000ff00;
  leastSignificantBits =   ( (int) boot[22] )        & 0x000000ff;
  temp = mostSignificantBits | leastSignificantBits;

  pbs.spf = temp;

  // Sectors per track
  mostSignificantBits  = ( ( (int) boot[25] ) << 8 ) & 0x0000ff00;
  leastSignificantBits =   ( (int) boot[24] )        & 0x000000ff;
  temp = mostSignificantBits | leastSignificantBits;

  pbs.spt = temp;

  // Number of heads
  mostSignificantBits  = ( ( (int) boot[27] ) << 8 ) & 0x0000ff00;
  leastSignificantBits =   ( (int) boot[26] )        & 0x000000ff;
  temp = mostSignificantBits | leastSignificantBits;

  pbs.noh = temp;

  // Boot signature
  cx = snprintf(pbs.bs, 6, "0x%0x", boot[38]);

  // Volume ID
  mostSignificantBits  = ( ( (int) boot[42] ) << 24 ) & 0xff000000;
  leastSignificantBits = ( ( (int) boot[41] ) << 16 ) & 0x00ff0000;
  temp_bits2 = ( ( (int) boot[40] ) << 8 ) & 0x0000ff00;
  temp_bits1 = ( (int) boot[39] ) & 0x000000ff;
  temp = mostSignificantBits | leastSignificantBits | temp_bits2 | temp_bits1;

  cx = snprintf(pbs.vol_id, 16, "0x%0x", temp);

  // Volume Label
  for(i = 43; i < 54; i++){
    label[count] = (char) boot[i];
    count++;
  }
  label[10] = '\0';
  strcpy(pbs.vol_label, label);

  count = 0;

  // File System Type
  for(i = 54; i < 62; i++){
    type[count] = (char) boot[i];
    count++;
  }
  type[7] = '\0';
  strcpy(pbs.fst, type);
}

void printBootSector(){
  printf("Bytes per sector%15s %d\n", "=", pbs.bytes_per_sector);
  printf("Sectors per cluster%12s %d\n", "=", pbs.sectors_per_cluster);
  printf("Number of FATs%17s %d\n", "=", pbs.nof);
  printf("Number of reserved sectors%5s %d\n", "=", pbs.nurs);
  printf("Number of root entries%9s %d\n", "=", pbs.nore);
  printf("Total sector count%13s %d\n", "=", pbs.total_sector_count);
  printf("Sectors per FAT%16s %d\n", "=",pbs.spf);
  printf("Sectors per track%14s %d\n", "=",pbs.spt);
  printf("Number of heads%16s %d\n", "=",pbs.noh);
  printf("Boot signature (in hex)%8s %s\n", "=",pbs.bs);
  printf("Volume ID (in hex)%13s %s\n", "=",pbs.vol_id);
  printf("Volume label%19s %s\n", "=",pbs.vol_label);
  printf("File system type%15s %s\n", "=",pbs.fst);
  printf("\n");
}

/******************************************************************************
 * main: an example of reading an item in the boot sector
 *****************************************************************************/

int main()
{
  readBootSector();
  printBootSector();
  return 0;
}
