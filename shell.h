#ifndef SHELL_HEADER_
#define SHELL_HEADER_

int executeCommand();
char ** parseInput(char line[], const char *delimiter);
char * readInput();

#define KEY 9670;
#define FLOPPY "./package/floppy3";

struct PATH
{
   int cluster;
   char cwd[4096];
};

#endif
