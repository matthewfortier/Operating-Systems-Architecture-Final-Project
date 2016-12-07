/*
 * File:   rm.c
 * Author: Anthony Taylor
 * Assignment: Final Project Shell
 * Description: Deletes a file based on arguements passed in and the current working directory
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
#include "rm.h"
#include "../../package/fatSupport.h"
#include "../../shell.h"

struct PATH *global_path;

#define BYTES_IN_SECTOR 512

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

   char** directories;
   char** tempDirectories;
   int directoryCount;
   int i;
   int deleteCheck;
   int cluster;
   //char *temp = global_path->cwd;

   if (argv[1] == NULL)       // Test for no arguments
   {
      memcpy(global_path->cwd, "/", sizeof("/"));
      global_path->cluster = 0;
   }

   else if (strcmp(argv[1], "/") == 0)          // Test for root directory
   {
      printf("Can not delete root directory!!\n");
   }

   else if (argv[1][0] == '/')          // Test for absolute path
   {
      cluster = 0;          // start at root cluster
      directories = parseInput(argv[1], "/");           // gets an array of all directories
      directoryCount = countDirectories(directories);

      for ( i = 0; i < directoryCount; i++)             // tests each directory sequentially
      {
         deleteCheck = checkDirectory(addSpaces(directories[i]), cluster);
      }

      char *new_path = generatePath(directories, directoryCount);

      if (deleteCheck != -2)            // Check to see if commands succeeded
      {
         printf("File Deleted!!\n");
      }

      else
      {
         printf("%s: directory or file does not exist\n", new_path);
      }

   }

   else //Test for relative path
   {
      cluster = global_path->cluster;
      directories = parseInput(argv[1], "/");
      directoryCount = countDirectories(directories);

      for ( i = 0; i < directoryCount; i++)
      {
         cluster = checkDirectory(addSpaces(directories[i]), cluster);
      }



      if (cluster != -2)
      {
         printf("File Deleted!!\n");
      }

      else
      {
         printf("File does not exist or is a directory\n");
      }
   }

   return 0;
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

char * generatePath(char** directories, int directoryCount)
{
   static char path[] = "";
   int i;
   for (i = 0; i < directoryCount; i++)
   {
      strcat(path, "/");
      strcat(path, directories[i]);
   }
   return path;
}

int checkDirectory(char *directory, int cluster)
{
   char path[] = FLOPPY;
   unsigned char* sect;
   int bytes;
   int deleteCheck;
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
      deleteCheck = read_cluster(i, sect, directory);           // Returns the First Logical Cluster for the directory given
      if (deleteCheck >= 0)            // Returns -1 if directory does not exists and returns it
      {
         return deleteCheck;
      }
      else if (deleteCheck == -2)
      {
         printf("UNABLE TO DELETE DIRECTORY");
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
   int attributes;

// Get the filename for this entry
   for (i = marker; i < marker + 7; i++)
   {
      filename[i - marker] = sect[i];
   }

   attributes = sect[marker + 11];


   if(attributes == 0x01 || attributes == 0x02 || attributes == 0x04 || attributes == 0x20)
   {
      // Mark File as unused
      if(filename[0] = 0x00)
      {
         return 1;
      }
   }

   return -2;
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

   int i = 0;
   char* cmd = strtok(line, delimiter);     // gets the first token of the line

   while (cmd != NULL)      // continues to get the next token until a null value
   {
      i++;
      temp[i-1] = cmd;          // assigns each valid argument to the array
      cmd = strtok(NULL, " ");          // gets the next token

      if(cmd == NULL)
      {
         temp[i] = NULL;
      }
   }

   return temp;
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
