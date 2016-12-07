/*
 * File:   rmdir.h
 * Author: Anthony Taylor
 * Assignment: Final Project Shell
 * Description: Header file for rmdir.c, holds all of the function definitions
 */

#ifndef RMDIR_HEADER
#define RMDIR_HEADER

char ** parseInput(char line[], const char *delimiter);
int checkDirectory(char *directory, int cluster);
int countDirectories(char** directories);
char * addSpaces(char* directory);
char * generatePath(char** directories, int directoryCount);
int read_cluster(int marker, unsigned char* sect, char* directory);
char *trimwhitespace(char *str);

#endif
