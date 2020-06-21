#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include <mem.h>


#define PAGE_SIZE 4096
#define MAX_KMEM_SIZE 0x100000
#define CHUNK_SIZE 1024
#define INCREASE_COUNT 1  //TODO

static uint32_t* chunk_free_list = NULL; //Maybe not use uint32_t*?

void* kernel_chunk_alloc()
{
  if (!chunk_free_list)
  {
    void* vma = kmem.start + kmem.size;
    int page_count = expand_kmem(INCREASE_COUNT);
    if(!page_count) return NULL;
    int chunk_count = (page_count * PAGE_SIZE) / CHUNK_SIZE;

    for (int i=0; i<chunk_count; i++)
    {
      *(uint32_t*)vma = (uint32_t)chunk_free_list;
      chunk_free_list = vma;
      vma += CHUNK_SIZE;
    }
  }
  void* vma = chunk_free_list;
  chunk_free_list = (uint32_t*)*chunk_free_list;
  return vma;
}

void kernel_chunk_free(void* address)
{
  *(uint32_t*)address = (uint32_t)chunk_free_list;
  chunk_free_list = address;
}
