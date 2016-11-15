/*
 * File:   cd.c
 * Author: Matthew Fortier
 * Assignment: Final Project Shell
 * Description: Changes the directory based on arguments and current directory
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

   printf("Removing: %s\n", argv[1]);

   return 0;
}
