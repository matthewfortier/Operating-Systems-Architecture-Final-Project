/*
 * File:   cd.c
 * Author: Matthew Fortier and Anthony Taylor
 * Assignment: Final Project Shell
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
#include "cd.h"
#include "../../package/fatSupport.h"
#include "../../shell.h"

struct PATH *global_path;

#define BYTES_IN_SECTOR 512

int main(int argc, char** argv) {
  // SHARED MEMORY
  int shmid;
  key_t key = KEY;
  size_t shmsize = sizeof(struct PATH);

  shmid = shmget(key, shmsize, 0666);
  if(shmid < 0){
    perror("shmget");
    exit(1);
  }
  global_path = shmat(shmid, (void *) 0, 0);
  if(global_path == (void *) -1){
    perror("shmat");
    exit(1);
  }

  char** directories;
  int directoryCount;
  int i;
  int cluster;
  char *temp = global_path->cwd;

  if (argv[1] == NULL){ // Test for no arguments
    memcpy(global_path->cwd, "/", sizeof("/"));
    global_path->cluster = 0;
  } else if (strcmp(argv[1], "/") == 0) { // Test for root directory
    memcpy(global_path->cwd, "/", sizeof("/"));
    global_path->cluster = 0;
  } else if (argv[1][0] == '/'){ // Test for absolute path
    cluster = 0; // start at root cluster
    directories = parseInput(argv[1], "/"); // gets an array of all directories
    directoryCount = countDirectories(directories);
    for ( i = 0; i < directoryCount; i++){ // tests each directory sequentially
      printf("Directory:%s\n", directories[i]);
      printf("Cluster:%d\n", cluster);
      cluster = checkDirectory(addSpaces(directories[i]), cluster);
      printf("%d\n", cluster);
    }
    char *new_path = generatePath(directories, directoryCount);
    if (cluster != -1){ // Check to see if commands succeeded
      global_path->cluster = cluster; // Reset clusetr to new cluster
      memcpy(global_path->cwd, new_path, 4096); // Set global path to new path
      printf("%s\n", "Test");
      printf("Cluster:%d\n", cluster);
    } else {
      printf("%s: directory or file does not exist\n", new_path);
    }
  } else if (strcmp(argv[1], ".") == 0){
    //do nothing
  } else if (strcmp(argv[1], "..") == 0){
    // go back a directory until root
  } else {
    cluster = global_path->cluster;
    directories = parseInput(argv[1], "/");
    directoryCount = countDirectories(directories);
    for ( i = 0; i < directoryCount; i++){
      cluster = checkDirectory(addSpaces(directories[i]), cluster);
      printf("%d\n", cluster);
    }
    char *new_path = generatePath(directories, directoryCount);
    if (cluster != -1){
      printf("%s\n", "Test");
      global_path->cluster = cluster;
      if (strcmp(temp, "/") != 0)
        strcat(temp, "/");
      strcat(temp, argv[1]);
      memcpy(global_path->cwd, temp, sizeof(temp));
      printf("%s\n", "Test");
      printf("Cluster:%d\n", cluster);
    } else {
      printf("%s/%s:directory or file does not exist\n", global_path->cwd, argv[1]);
    }
  }

  return 0;
}

int countDirectories(char** directories){
  int i = 0;
  int count = 0;

  while(directories[i] != NULL){
    count++;
    i++;
  }
  return count;
}

char * addSpaces(char* directory){
  static char filename[8];
  memset(filename, ' ', 7);
  int i;
  for (i = 0; i < strlen(directory); i++){
    filename[i] = directory[i];
  }
  filename[8] = '\0';
  return filename;
}

char * generatePath(char** directories, int directoryCount){
  static char path[] = "";
  int i;
  for (i = 0; i < directoryCount; i++){
    strcat(path, "/");
    strcat(path, directories[i]);
  }
  printf("Path: %s\n", path);
  return path;
}

int checkDirectory(char *directory, int cluster){
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

  sect = (unsigned char*) malloc((BYTES_PER_SECTOR) * sizeof(unsigned char));
  if (cluster == 0){
    bytes = read_sector(cluster + 19, sect);
  } else {
    bytes = read_sector(cluster + 32, sect);
  }

  for (i = 0; i < BYTES_PER_SECTOR; i += 32){
    FLC = read_cluster(i, sect, directory);
    printf("%d\n", FLC);
    if (FLC != -1){
      return FLC;
    }
  }
  return -1;
}

int read_cluster(int marker, unsigned char* sect, char* directory){
  int i;
  char filename[9];
  char extension[4];
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
  for (i = marker; i < marker + 7; i++){
    filename[i - marker] = sect[i];
  }

  // If the first byte is 0x00, then there are no more entries
  if (filename[0] == 0x00) {
    printf("%s\n", "Directory is empty");
    return -1;
  }

  filename[7] = '\0'; // Make sure to add null terminating character

  // Get the file extension for this entry
  for (i = marker + 8; i < marker + 11; i++){
    extension[i - (marker + 8)] = sect[i];
  }

  extension[3] = '\0'; // Make sure to add null terminating character
  attributes = sect[marker + 11]; // Set the attributes for this entry

  // Get the FLC
  mostSignificantBits  = ( ( (int) sect[marker + 27] ) << 8 ) & 0x0000ff00;
  leastSignificantBits =   ( (int) sect[marker + 26] )        & 0x000000ff;
  temp = mostSignificantBits | leastSignificantBits;

  FLC = temp;

  printf("%s%s|\n", filename, directory);
  if (strcmp(filename, directory) == 0){
    return FLC;
  } else {
    printf("%s\n", "Not the same");
  }

  // Get File Size (in bytes)

  // Volume ID
  mostSignificantBits  = ( ( (int) sect[marker + 31] ) << 24 ) & 0xff000000;
  leastSignificantBits = ( ( (int) sect[marker + 30] ) << 16 ) & 0x00ff0000;
  temp_bits2 = ( ( (int) sect[marker + 29] ) << 8 ) & 0x0000ff00;
  temp_bits1 = ( (int) sect[marker + 28] ) & 0x000000ff;
  temp = mostSignificantBits | leastSignificantBits | temp_bits2 | temp_bits1;

  size = temp;

  // Print out ls information ignoring anything that is not a file or directory
  // if (attributes != 0x0F && attributes != 0x08){
  //   if (attributes == 0x10){
  //     strcpy(type, "Dir");
  //     printf("%s %s %10s %13d %9d\n", filename, extension, type, size, FLC);
  //   } else {
  //     strcpy(type, "File");
  //     printf("%s.%s %10s %13d %9d\n", filename, extension, type, size, FLC);
  //   }
  // }

  return -1;
}

char ** parseInput(char line[], const char *delimiter){

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
   char* cmd = strtok(line, delimiter); // gets the first token of the line

   while (cmd != NULL) // continues to get the next token until a null value
   {
        i++;
        temp[i-1] = cmd; // assigns each valid argument to the array
        cmd = strtok(NULL, " "); // gets the next token

        if(cmd == NULL)
        {
            temp[i] = NULL;
        }
   }

   return temp;
}
