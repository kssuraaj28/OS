#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <mem.h>

#define PAGE_SIZE 4096
#define MAX_KMEM_SIZE 0x100000
#define INCREASE_COUNT 16  

struct _kmem kmem = {__kernel_heap,0,MAX_KMEM_SIZE};

int expand_kmem(int page_count) //Increase heap by page_count pages
{
  if (kmem.size + (page_count* PAGE_SIZE) > kmem.max_size) return 0;
  void* vma = kmem.start + kmem.size;
  for (int i=0;i<page_count;i++)
  {
    uint32_t pma = pmmngr_allocate_block();
    if(!pma) return i;
    if(!map_page((void*)vma,pma,false,true))
    {
      pmmngr_free_block(pma);
      return i;
    }
    vma += PAGE_SIZE;
    kmem.size += PAGE_SIZE;
  }
  return page_count;
}
