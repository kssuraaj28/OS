#ifndef _KERNEL_VIRTMEM_H
#define _KERNEL_VIRTMEM_H

#include<stdint.h>
#include<stdbool.h>
void vmmngr_init();
void remove_identity_map();
bool map_page(uint32_t vir,uint32_t phy,bool isUser,bool isWritable);
uint32_t virtual_to_physical(uint32_t* virtual_address);
//void free_page(uint32_t* entry);
#endif 

