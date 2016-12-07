/*
 * File:   cd.h
 * Author: Matthew Fortier and Anthony Taylor
 * Assignment: Final Project Shell
 * Description: Header file for cd.c, holds all of the function definitions
 */

#ifndef CD_HEADER
#define CD_HEADER

char ** parseInput(char line[], const char *delimiter);
char ** removeLastElement(char** directories);
int checkDirectory(char *directory, int cluster);
int countDirectories(char** directories);
char * addSpaces(char* directory);
char * generatePath(char** directories, int directoryCount);
int read_cluster(int marker, unsigned char* sect, char* directory);
char *trimwhitespace(char *str);
void removeSubstring(char *s,const char *toremove);

#endif
