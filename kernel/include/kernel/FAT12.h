#ifndef _KERNEL_FAT12_H
#define _KERNEL_FAT12_H

#include <stdint.h>
#include <stdbool.h>

void initialize_FAT(uint32_t data_sect,uint32_t root_sect, uint32_t fat_sect);
bool FAT_find_file(char* filename);

#endif
