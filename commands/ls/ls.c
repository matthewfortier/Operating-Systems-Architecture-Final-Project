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

int read_cluster(int marker, unsigned char* sect);
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

   if (argc > 3)
   {
     printf("ERROR: too many arguments: ls <flag> <pathname>\n");
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

   FILE_SYSTEM_ID = fopen(path, "r+");

   if (FILE_SYSTEM_ID == NULL)
   {
      printf("Could not open the floppy drive or image.\n");
      exit(1);
   }
   BYTES_PER_SECTOR = BYTES_IN_SECTOR;

   char** directories;
   char** tempDirectories;
   int directoryCount;
   int cluster;
   int fat;
   char *fat_buffer = (char*) malloc(9 * BYTES_PER_SECTOR * sizeof(char));
   sect = (unsigned char*) malloc((BYTES_PER_SECTOR) *sizeof(unsigned char));



   if (argc == 2)
   {
     if (strcmp(argv[1], "-l") == 0)
     {
       if (global_path->cluster == 0)        // Test to see if cluster is pointing to the root directory
       {
          bytes = read_sector(global_path->cluster + 19, sect);
       }
       else {
          bytes = read_sector(global_path->cluster + 31, sect);
       }
     }
     else if (strcmp(argv[1], "/") == 0)
     {
       bytes = read_sector(19, sect);
     }
     else
     {
       cluster = (argv[1][0] == '/') ? 0 : global_path->cluster;
       directories = parseInput(argv[1], "/");
       directoryCount = countDirectories(directories);
       for (i = 0; i < directoryCount; i++)
       {
         cluster = checkDirectory(addSpaces(directories[i]), cluster);
       }
       if (cluster != -1)
       {
         bytes = read_sector(cluster + 31, sect);

       }
       else
       {
         printf("Directory not found\n");
         return 1;
       }
     }
   }
   else if (argc == 3)
   {
     if (strcmp(argv[2], "/") == 0)
     {
       bytes = read_sector(19, sect);
     }
     else
     {
       cluster = (argv[2][0] == '/') ? 0 : global_path->cluster;
       directories = parseInput(argv[2], "/");
       directoryCount = countDirectories(directories);
       for (i = 0; i < directoryCount; i++)
       {
         cluster = checkDirectory(addSpaces(directories[i]), cluster);
       }
       if (cluster != -1)
       {
         bytes = read_sector(cluster + 31, sect);
       }
       else
       {
         printf("Directory not found\n");
         return 1;
       }
     }
   }
   else
   {
     if (global_path->cluster == 0)        // Test to see if cluster is pointing to the root directory
     {
        bytes = read_sector(global_path->cluster + 19, sect);
     }
     else {
        bytes = read_sector(global_path->cluster + 31, sect);
     }
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

int read_cluster(int marker, unsigned char* sect)
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
      FLC = search_cluster(i, sect, directory);           // Returns the First Logical Cluster for the directory given
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

int search_cluster(int marker, unsigned char* sect, char* directory)
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
/* Unnessecary for this function but may be useful

   // Get File Size (in bytes)
   mostSignificantBits  = ( ( (int) sect[marker + 31] ) << 24 ) & 0xff000000;
   leastSignificantBits = ( ( (int) sect[marker + 30] ) << 16 ) & 0x00ff0000;
   temp_bits2 = ( ( (int) sect[marker + 29] ) << 8 ) & 0x0000ff00;
   temp_bits1 = ( (int) sect[marker + 28] ) & 0x000000ff;
   temp = mostSignificantBits | leastSignificantBits | temp_bits2 | temp_bits1;

   size = temp;

   Print out ls information ignoring anything that is not a file or directory
   if (attributes != 0x0F && attributes != 0x08){
   if (attributes == 0x10){
    strcpy(type, "Dir");
    printf("%s %s %10s %13d %9d\n", filename, extension, type, size, FLC);
   } else {
    strcpy(type, "File");
    printf("%s.%s %10s %13d %9d\n", filename, extension, type, size, FLC);
   }
   }
 */
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
      char* path = strdup(line);
      char ** res  = NULL;
      char *  p    = strtok (path, delimiter);
      int n_spaces = 0, i;


      /* split string and append tokens to 'res' */

      while (p) {
        res = realloc (res, sizeof (char*) * ++n_spaces);

        if (res == NULL)
          exit (-1); /* memory allocation failed */

        p[strlen(p)] = '\0';
        res[n_spaces-1] = p;

        p = strtok (NULL, delimiter);
      }

      /* realloc one extra element for the last NULL */

      res = realloc (res, sizeof (char*) * (n_spaces+1));
      res[n_spaces] = 0;

      /* print the result */

      // for (i = 0; i < (n_spaces+1); ++i)
      //   printf ("res[%d] = %s\n", i, res[i]);

      return res;
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
