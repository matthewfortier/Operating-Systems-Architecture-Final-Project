/*
 * File:   pbs.h
 * Author: Matthew Fortier / Based on provided fat.c
 * Assignment: Final Project Shell
 * Description: Contains function definitions and boot sectoe strcuture
 */

#ifndef PBS_HEADER
#define PBS_HEADER

void readBootSector();
void printBootSector();

struct PBS
{
        int bytes_per_sector; // bytes per sector
        int sectors_per_cluster; // sectors per cluster
        int nof; // number of FATS
        int nurs; // number of reserved sectors
        int nore; // number of root entries
        int total_sector_count; // total sector count
        int spf; // sectors per FAT
        int spt; // sectors per track
        int noh; // numbers of heads
        char bs[16]; // boot signature
        char vol_id[16];
        char vol_label[11];
        char fst[8];
};

#endif
