#ifndef _KERNEL_MEM_H
#define _KERNEL_MEM_H

#include <stdint.h>
#include <stdbool.h>

#define PAGE_TABLE 0xFFC00000		/**< Description here */
#define PAGE_DIRECTORY 0xFFFFF000	/**< Description here */
#define PAGE_SIZE 4096

//Linker symbols TODO: Replace begin with kernel_begin or something and kernel_heap by kmem
extern char __begin[];
extern char __end[];
extern char	__VGA_text_memory[];
extern char __kernel_heap[];

//kmem.c
struct _kmem{
  //Some lock here
  void* start;
  uint32_t size;
  uint32_t max_size;
};

extern struct _kmem kmem;
int expand_kmem(int page_count);

//phymem.c
void  pmmngr_init(uint32_t mapentrycount);
uint32_t pmmngr_allocate_block();
bool pmmngr_free_block(uint32_t address);
uint32_t allocate_special_block();
bool free_special_block(uint32_t address);

//virtmem.c
void vmmngr_init();
void remove_identity_map();
bool map_page(void* vir,uint32_t phy,bool isUser,bool isWritable);
uint32_t virtual_to_physical(uint32_t* virtual_address);
void free_vma(uint32_t* vma);

//Kernel heap

//void* kalloc(uint32_t size);
//bool kfree(void* address);

//kchunk.c
void* kernel_chunk_alloc();
void kernel_chunk_free(void* address);

#endif
