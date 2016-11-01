/*
 * File:   shell.c
 * Author: Matthew Fortier and Anthony Taylor
 * Assignment: Final Project Shell
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern char current_path[4096];

int main(int argc, char const *argv[]) {
  printf("%s\n", current_path);
  return 0;
}
