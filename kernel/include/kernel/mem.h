#ifndef _KERNEL_MEM_H
#define _KERNEL_MEM_H

#include <stdint.h>
#include <stdbool.h>

//Physical memory allocator
void  pmmngr_init(uint32_t mapentrycount);
uint32_t pmmngr_allocate_block();
bool pmmngr_free_block(uint32_t address);
uint32_t allocate_special_block();
bool free_special_block(uint32_t address);

//Virtual memory
void vmmngr_init();
void remove_identity_map();
bool map_page(void* vir,uint32_t phy,bool isUser,bool isWritable);
uint32_t virtual_to_physical(uint32_t* virtual_address);
void free_vma(uint32_t* vma);

//Kernel heap

void* kalloc(uint32_t size);
bool kfree(void* address);

#endif
