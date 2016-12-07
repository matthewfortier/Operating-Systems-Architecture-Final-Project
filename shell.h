#ifndef SHELL_HEADER_
#define SHELL_HEADER_

int executeCommand();
char ** parseInput(char line[], const char *delimiter);
char * readInput();
void upper_string(char s[]);
int countDirectories(char** directories);

#define KEY 9670;
#define FLOPPY "./package/floppy2";

struct PATH
{
   int cluster;
   char cwd[4096];
};

#endif
