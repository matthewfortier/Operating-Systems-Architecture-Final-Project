/*
 * File:   shell.c
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
#include "shell.h"
#include "./package/fatSupport.h"

FILE* FILE_SYSTEM_ID;
int BYTES_PER_SECTOR;
struct PATH *global_path;

int executeCommand();
char ** parseInput(char line[]);
char * readInput();

int main(int argc, char** argv)
{
    int status = 1;
    int compare;
    int ret;
    char c;
    char *line;
    char** args;
    char *buff;
    char path[] = "./package/floppy1";

    // SHARED MEMORY

    int shmid;
    key_t key = KEY;
    size_t shmsize = sizeof(struct PATH);

    shmid = shmget(key, shmsize, IPC_CREAT | 0666);
    if(shmid < 0){
      perror("shmget");
      exit(1);
    }
    global_path = shmat(shmid, (void *) 0, 0);
    if(global_path == (void *) -1){
      perror("shmat");
      exit(1);
    }

    // You must set two global variables for the disk access functions:
    //      FILE_SYSTEM_ID         BYTES_PER_SECTOR

    // Use this for an image of a floppy drive
    FILE_SYSTEM_ID = fopen(path, "r+");

    if (FILE_SYSTEM_ID == NULL)
    {
       printf("Could not open the floppy drive or image.\n");
       exit(1);
    }

    memcpy(global_path->cwd, "/", 1);
    global_path->cluster = 0;

    // start the shell - prompt, read, parse, execute
    do{
        // 1. Prompt for input
        printf("> ");
        // disable buffering if need be
        // 2. Read input

        line = readInput(); // read input from user
        buff = malloc(strlen(line) * sizeof(char)); // allocate a buffer for the size of the string
        if(buff == NULL)
        {
            perror("buffer allocation error");
        }

        strcpy(buff, line); // copy the string to the buffer

        // 3. Parse input
        args = parseInput(buff); // parse input tokenizes the string and returns an array of tokens

        //args[0] = ls
        //args[1] = -l
        //args[2] = null

        // 4. Execute

        if (strcmp(line, "help") == 0) // if the input is "help" display the help info
        {
            printf("Copyright © Matthew Fortier and Joseph Healy\n");
            printf("Assignment: Create your own shell\n");
            printf("Description: The program emulates a Bash shell with limited capability\n");
            printf("Features added: cd works correctly, hostname, and cwd to the input line");

            printf("\n\ncd <directory> - changes the working directory\n");
            printf("exit - closes the program\n\n");
            status = 0; // set the status to 0 so it continues to repeat
        } else if (strcmp(line, "exit") == 0) { // if th input equal "exit" change status to end loop to end program
            status = 1;
        }
        else {
            status = executeCommand(args); // if the input equals a simple command, execute that command
        }

    } while(status == 0);



    exit(0);
}

int executeCommand(char *cmd[])
{
	pid_t x, y;
	int status;

	x = fork(); // creates child process

	if (x == -1){
		    perror("fork");
	}else if (x == 0){
		// This is run by child
      if(strcmp(cmd[0], "pbs") == 0){
        execv("./commands/pbs/pbs", cmd);
      } else if (strcmp(cmd[0], "pfe") == 0){
        execv("./commands/pfe/pfe", cmd);
      } else if (strcmp(cmd[0], "pwd") == 0){
        printf("%s\n", global_path->cwd);
      } else if (strcmp(cmd[0], "cd") == 0) {
        execv("./commands/cd/cd", cmd);
      } else if (strcmp(cmd[0], "ls") == 0) {
        execv("./commands/ls/ls", cmd);
      } else {
        printf("%s does not exist\n", cmd[0]);
      }
      exit(1);
	 }else{
	    // this is run by parent
	    do {
	    	y = waitpid(x, &status, WUNTRACED); //WUNTRACED, is an option for waitPID, it waits for child to terminate
	    } while (!WIFEXITED(status) && !WIFSIGNALED(status)); // WIFEXITED, and WIFSIGNALED are activated when process terminates
	  }

	  return 0;
}

char ** parseInput(char line[]){

   int k = 0;
   int count = 0;
   char *ptr = line;

   while((ptr = strchr(ptr,' ')) != NULL)
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
   char* cmd = strtok(line," "); // gets the first token of the line

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

char* readInput()
{
    int stringsize = 10; //size of string
    int position =  0; 	//position in string
    char* returnVal = malloc(sizeof(char) * stringsize); //allocating size of string so that it can be changed later, if the user goes over the original size
    if(returnVal == NULL)
    {
            perror("readInput memory allocation error");
    }
    int c ;

    while(1)
    {
        c = getchar(); //reads one char at a time
        if(c == -1)
        {
            perror("getchar");
        }

        if(c == '\n')
        {
            return returnVal;
        }
        else
        {
            returnVal[position] = c;
        }
        position++;

        if(position >= stringsize) //check to see if string needs to be resized
        {
            stringsize = stringsize * 2;
            returnVal = realloc(returnVal, stringsize); //resize string
        }
    }
}
