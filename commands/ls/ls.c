/*
 * File:   ls.c
 * Author: Matthew Fortier
 * Assignment: Final Project Shell
 * Description: Lists files in selected directory
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
 #include <ctype.h>
 #include "../../package/fatSupport.h"
 #include "../../shell.h"

#define BYTES_IN_SECTOR 512

FILE* FILE_SYSTEM_ID;
int BYTES_PER_SECTOR;

extern int read_sector(int sector_number, char* buffer);
extern int write_sector(int sector_number, char* buffer);

extern unsigned int  get_fat_entry(int fat_entry_number, char* fat);
extern void set_fat_entry(int fat_entry_number, int value, char* fat);

void read_cluster(int marker, unsigned char* sect);
char *trimwhitespace(char *str);
char * addSpaces(char* directory);

struct PATH *global_path;

int main(int argc, char *argv[])
{
   unsigned char* sect;
   char path[] = FLOPPY;
   char filename[9];
   char extension[4];
   int inRange = 0;
   int i;
   int bytes;

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

   FILE_SYSTEM_ID = fopen(path, "r+");

   if (FILE_SYSTEM_ID == NULL)
   {
      printf("Could not open the floppy drive or image.\n");
      exit(1);
   }
   BYTES_PER_SECTOR = BYTES_IN_SECTOR;

   sect = (unsigned char*) malloc((BYTES_PER_SECTOR) *sizeof(unsigned char));
   if (global_path->cluster == 0)        // Test to see if cluster is pointing to the root directory
   {
      bytes = read_sector(global_path->cluster + 19, sect);
   }
   else {
      bytes = read_sector(global_path->cluster + 31, sect);
   }

      // Prints out all header info
   printf("Name              Type     File Size       FLC\n");
   for (i = 0; i < BYTES_PER_SECTOR; i += 32)        // Loops through each entry by incrementing by 32
   {
      read_cluster(i, sect);
   }

   fclose(FILE_SYSTEM_ID);
   free(sect);

   return 0;
}

void read_cluster(int marker, unsigned char* sect)
{
   int i;
   char filename[9];
   char extension[4];
   char *fullName;
   char *stringTemp;
   char attributeArray[0];
   int attributes;
   int print = 0;
   int file = 0;
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

   filename[7] = '\0';           // Make sure to add null terminating character

   if ((int) sect[marker + 11] != 16 && (int) sect[marker + 11] != 32)
      return;

      // If the first byte is 0x00, then there are no more entries
   if (filename[0] != 0x00 && filename[0] != 0xFFFFFFE5)
   {
         // Get the file extension for this entry
      for (i = marker + 8; i < marker + 11; i++)
      {
         extension[i - (marker + 8)] = sect[i];
      }

      extension[3] = '\0';           // Make sure to add null terminating character
      attributes = sect[marker + 11];           // Set the attributes for this entry
      temp = attributes;

      while (temp)
      {
         if (temp == 0x0F || temp == 0x40 || temp == 0x80)
         {
            print = 1;
            break;
         }
         temp >>= 1;
      }

         // Get the FLC
      mostSignificantBits  = ( ( (int) sect[marker + 27] ) << 8 ) & 0x0000ff00;
      leastSignificantBits =   ( (int) sect[marker + 26] )        & 0x000000ff;
      temp = mostSignificantBits | leastSignificantBits;

      FLC = temp;

      // Get File Size (in bytes)

      mostSignificantBits  = ( ( (int) sect[marker + 31] ) << 24 ) & 0xff000000;
      leastSignificantBits = ( ( (int) sect[marker + 30] ) << 16 ) & 0x00ff0000;
      temp_bits2 = ( ( (int) sect[marker + 29] ) << 8 ) & 0x0000ff00;
      temp_bits1 = ( (int) sect[marker + 28] ) & 0x000000ff;
      temp = mostSignificantBits | leastSignificantBits | temp_bits2 | temp_bits1;

      size = temp;

         // Print out ls information ignoring anything that is not a file or directory
      if (print == 0)
      {
         if (attributes == 0x10)
         {
            strcpy(stringTemp, trimwhitespace(filename)); // Strip all whitespace for printing
            strcat(stringTemp, trimwhitespace(extension));
            fullName = addSpaces(stringTemp);
            strcpy(type, "Dir");
            printf("%s %11s %13d %9d\n", fullName, type, size, FLC);
         }
         else {
            strcpy(stringTemp, trimwhitespace(filename)); // Strip all whitespace for printing
            if(strlen(trimwhitespace(extension)) > 0)
            {
               strcat(stringTemp, ".");
            }
            strcat(stringTemp, trimwhitespace(extension));
            fullName = addSpaces(stringTemp);
            strcpy(type, "File");
            printf("%s %11s %13d %9d\n", fullName, type, size, FLC);
         }
      }

   }
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

char * addSpaces(char* directory)
{
   static char filename[11];
   memset(filename, 32, 10);
   int i;
   for (i = 0; i < strlen(directory); i++)
   {
      filename[i] = directory[i];
   }
   filename[11] = '\0';
   return filename;
}
