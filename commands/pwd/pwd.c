/*
 * File:   pwd.c
 * Author: Matthew Fortier
 * Assignment: Final Project Shell
 * Description: Prints the current working directory
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

      // Prints the global variable
   printf("Path: %s\n", global_path->cwd);

   return 0;
}
