#ifndef CD_HEADER
#define CD_HEADER

char ** parseInput(char line[], const char *delimiter);
int checkDirectory(char *directory, int cluster);
int countDirectories(char** directories);
char * addSpaces(char* directory);
char * generatePath(char** directories, int directoryCount);
int read_cluster(int marker, unsigned char* sect, char* directory);

#endif
