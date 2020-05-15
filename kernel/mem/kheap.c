//This is my implementation of a kernel heap. It is a bunch of virtual memory "filled" with physical addresses.

//Note: every address in the heap needs to be accounted for by the linked list!!! The "useful" entries, will be the
//only ones part of the linked list
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include <mem.h>

#define PAGE_SIZE 4096
#define MAX_HEAP_SIZE 0x400000 //4M for now 
#define MAX_HEAP_ENTRIES 256//Max addresses the heap gives you
#define STANDARD_INCREASE 16 //Increases the size of the heap by 16 pages

extern uint32_t __kernel_heap[];//Ensure that this is page aligned

typedef struct _heap_entry 
{
    bool is_useful; //Part of heap??

    void* start_address;
    uint32_t size;
    bool is_hole;
    struct _heap_entry* next_entry;
    struct _heap_entry* prev_entry;
}heap_entry;

typedef struct _heap
{
    //Static stuff
    uint32_t max_entry_count;
    uint32_t max_size;

    //Dynamic stuff
    uint32_t entry_count;
    uint32_t size;
    void* heap_start_address;
    heap_entry* start_entry;
    heap_entry* last_entry;

}heap;


static heap_entry heap_entry_pool[MAX_HEAP_ENTRIES];
static heap kernel_heap;

static heap_entry* allocate_heap_entry()
{
  uint32_t lazy =0;

  for(uint32_t i=0;i<MAX_HEAP_ENTRIES;i++)
  {
    uint32_t temp = (lazy+i)%MAX_HEAP_ENTRIES;
    if(heap_entry_pool[temp].is_useful) continue;
    heap_entry_pool[temp].is_useful = true;
    lazy = temp+1;
    kernel_heap.entry_count++;
    return &heap_entry_pool[temp];
  }
  //TODO: Perhaps we can use a linked list to grow this list dynamically too!!
  return NULL;
}

static bool expand_heap(uint32_t page_count)
{
  uint32_t req_size = page_count * PAGE_SIZE;
  if (req_size + kernel_heap.size > kernel_heap.max_size) goto failure;
  void* vma = kernel_heap.heap_start_address + (req_size);
  for(uint32_t i=0;i<page_count;i++)
  {
    uint32_t pma = pmmngr_allocate_block();
    if(!pma) goto failure;
    if(!map_page(vma,pma,false,true)) goto failure; //(vma,pma,isuser,iswritable)
    vma += PAGE_SIZE;
  }
  //Now, vma "holds" a size of page_count;

  heap_entry* pointer = kernel_heap.last_entry;
  if(pointer -> is_hole)
    pointer -> size += req_size;
  else
  {
    heap_entry* temp = allocate_heap_entry();
    if(!temp) goto failure;
    temp -> start_address = vma;
    temp -> size = req_size;
    temp -> is_hole = true;
    temp -> next_entry = NULL; //Last entry
    temp -> prev_entry = pointer;
    pointer -> next_entry = temp;
    kernel_heap.last_entry = temp;
  }
  kernel_heap.size += req_size;
  return true;

  failure: //TODO: Handle failures better - revert already mapped pages
  return false;
}



static void initialize_kernel_heap()
{
    for(int i = 0;i< MAX_HEAP_ENTRIES;i++)
        heap_entry_pool[i].is_useful = false;
    
    kernel_heap.max_size = MAX_HEAP_SIZE;
    kernel_heap.max_entry_count = MAX_HEAP_ENTRIES;
    
    kernel_heap.entry_count = 0;
    kernel_heap.size = 0;
    kernel_heap.heap_start_address = __kernel_heap;
    kernel_heap.start_entry = allocate_heap_entry();
    kernel_heap.last_entry = kernel_heap.start_entry;
     
    kernel_heap.start_entry->size = 0;
    kernel_heap.start_entry->is_hole = true;
    kernel_heap.start_entry->next_entry = NULL;
    kernel_heap.start_entry->prev_entry = NULL;
}

static void* kernel_heap_alloc(uint32_t size) //This is the main part... It allocs stuffon heap
{
  while(1) //Keep expanding the heap
  {
    for(heap_entry* iterator = kernel_heap.start_entry ; iterator ; iterator=iterator->next_entry)
    {
      if (!iterator -> is_hole) continue;
      if (iterator -> size == size)
      {
        iterator -> is_hole = false;
        return iterator -> start_address;
      }
      if (iterator -> size > size)
      {
        heap_entry* temp = allocate_heap_entry();
        if(!temp) goto failure;
        temp -> start_address = iterator -> start_address + size;
        temp -> size = (iterator -> size) - size;
        temp -> is_hole = true;
        temp -> next_entry = iterator -> next_entry;
        temp -> prev_entry = iterator;

        iterator -> size = size;
        iterator -> is_hole = false;
        iterator -> next_entry = temp;

        return iterator -> start_address;
      }
    }
    //Now, we've seen that the heap is small 
    if(!expand_heap(STANDARD_INCREASE)) goto failure;
  }
failure:
  return NULL;
}

static bool kernel_heap_free(void* address)
{
  //Find the corresponding entry... Maybe we can have a binary tree or something
  for(heap_entry* iterator = kernel_heap.start_entry ; iterator ; iterator=iterator->next_entry)
  {
    if ((uint32_t)address > (uint32_t)iterator->start_address) goto failure;
    if (address == iterator->start_address)
    {
      if(iterator->next_entry && iterator->next_entry->is_hole) //If next entry is free join it
      {
        iterator -> size += iterator -> next_entry -> size;
        iterator -> next_entry -> is_useful = false;
        iterator -> next_entry = iterator -> next_entry -> next_entry;
        if(iterator -> next_entry) iterator -> next_entry -> prev_entry = iterator;
      }
      if(iterator->prev_entry && iterator->prev_entry->is_hole) 
      {
        iterator -> start_address = iterator -> prev_entry -> start_address;
        iterator -> size += iterator -> prev_entry -> size;
        iterator -> prev_entry -> is_useful = false;
        iterator -> prev_entry = iterator -> prev_entry -> prev_entry;
        if(iterator -> prev_entry) iterator -> prev_entry -> next_entry = iterator;
      }
      iterator->is_hole = true;
      if (!iterator -> next_entry) kernel_heap.last_entry = iterator; //TODO: Maybe free the pages too??
      return true;
    }
  }
failure:
  return false;
}
static bool is_kheap = false;
void* kmalloc(uint32_t size)
{
  if (!is_kheap)
  {
    initialize_kernel_heap();
    is_kheap = true;
  }
  return kernel_heap_alloc(size);
}

bool kfree(void* address)
{
  void* h_st =  kernel_heap.heap_start_address;
  void* h_en =  h_st + kernel_heap.size;
  if((address < h_st) || (address > h_en) || !is_kheap) return false;
  return kernel_heap_free(address);
}
