//This is my implementation of a kernel heap. It is a bunch of virtual memory "filled" with physical addresses. 
//
// TODO: THIS IS NOT INCLUDED, i.e. It is not compiled

//Note: every address in the heap needs to be accounted for by the linked list!!! The "useful" entries, will be the only ones part of the linked list. TODO: Extend heap entry pool using linked list
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include <string.h>

#include <mem.h>
#include <tty.h>

#define MAX_HEAP_SIZE 0x400000 //4M for now 
#define STANDARD_INCREASE 16 //Increases the size of the heap by 16 pages
#define MIN_ALLOC_SIZE 16 //A kalloc will give you min 16 bytes!!
#define H_ENTRY_POOL_SIZE 256//Max addresses the heap gives you

#define BUILD_BUG_ON(condition) ((void)sizeof(char[1 - 2*!!(condition)]))
#define POOL_INCREASE 2

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

typedef struct _heap //TODO: separate kmem from heap
{
    //Static stuff
//  uint32_t max_entry_count;
    uint32_t max_size;

    //Dynamic stuff
    uint32_t entry_count;
    uint32_t size;
    void* heap_start_address;
    heap_entry* start_entry;
    heap_entry* last_entry;

}heap;

typedef struct _heap_entry_pool
{

  heap_entry array[H_ENTRY_POOL_SIZE];
  heap_entry next_pool;
  uint32_t lazy;

}heap_entry_pool;

not compilable

//Compile time structures
static heap_entry_pool static_pool;
static heap kernel_heap;

static heap_entry* allocate_heap_entry(heap_entry_pool* pool)
{
  for(uint32_t i=0;i<H_ENTRY_POOL_SIZE;i++)
  {
    uint32_t temp = (pool -> lazy + i)%H_ENTRY_POOL_SIZE;
    if(pool -> array[temp].is_useful) continue;
    pool -> array[temp].is_useful = true;
    pool -> lazy = (temp+1)%H_ENTRY_POOL_SIZE;
    kernel_heap.entry_count++;
    return &(pool -> array[temp]);
  }

  //At this point all entries are in use ...
  if (!pool -> next_pool.is_useful)
    return NULL; //Basic allocation fails

  return allocate_heap_entry(pool -> next_pool.start_address);

  //TODO: Perhaps we can use a linked list to grow this list dynamically too!!
  //The last pool entry can point to a new heap pool present in the heap
}

static void* expand_heap(uint32_t page_count)
{
  uint32_t req_size = page_count * PAGE_SIZE;
  if (req_size + kernel_heap.size > kernel_heap.max_size) goto failure;
  void* vma = kernel_heap.heap_start_address + kernel_heap.size;
  for(uint32_t i=0;i<page_count;i++)
  {
    uint32_t pma = pmmngr_allocate_block();
    if(!pma) goto failure;
    if(!map_page(vma,pma,false,true)) goto failure; //(vma,pma,isuser,iswritable)
    vma += PAGE_SIZE;
  }
  //Now, vma "holds" a size of page_count;
  kernel_heap.size += req_size;
  return vma;

  failure:
  vma = kernel_heap.heap_start_address + kernel_heap.size;
  for(uint32_t i=0;i<page_count;i++)
  {
    uint32_t pma = virtual_to_physical(vma);
    if(!pma) break;
    pmmngr_free_block(pma);
    free_vma(vma);
    vma += PAGE_SIZE;
  }
  return NULL;
}


static bool make_new_pool(heap_entry_pool* pool)
{
  if(pool -> next_pool.is_useful) make_new_pool(pool -> next_pool.start_address);
  
  //This is called after increasing heap or after wanting to split a hole
  void* vma = kernel_heap.last_entry -> start_address + kernel_heap.last_entry -> size;
  uint32_t size_diff =  (kernel_heap.heap_start_address + kernel_heap.size) - vma;
  if(size_diff < POOL_INCREASE)
  {
    if(!expand_heap(POOL_INCREASE)) return false; //TODO: DEBUG POINT check if everything fits
    size_diff += POOL_INCREASE * PAGE_SIZE;
  }

  //First deal with pool->next_pool, then deal with linked_pool
  heap_entry_pool* linked_pool = vma;

  pool -> next_pool.is_useful = true;
  pool -> next_pool.start_address = vma;
  pool -> next_pool.size = sizeof(heap_entry_pool);
  pool -> next_pool.is_hole = false;
  pool -> next_pool.next_entry = &linked_pool->array[0];
  pool -> next_pool.prev_entry = kernel_heap.last_entry;
  kernel_heap.last_entry -> next_entry = &pool -> next_pool;


  memset(linked_pool,0,sizeof(heap_entry_pool));
  linked_pool -> array[0].is_useful = true;
  linked_pool -> array[0].start_address = vma + sizeof(heap_entry_pool);
  linked_pool -> array[0].size = size_diff - sizeof(heap_entry_pool);
  linked_pool -> array[0].is_hole = true;
  linked_pool -> array[0].next_entry = NULL;
  linked_pool -> array[0].prev_entry = &(pool -> next_pool);
  kernel_heap.last_entry = &(linked_pool -> array[0]);
  return true;
}

static void initialize_kernel_heap()
{

    BUILD_BUG_ON(sizeof(heap_entry_pool) > PAGE_SIZE*POOL_INCREASE);

    memset(&static_pool,0,sizeof(static_pool)); //Making all entries useless
    
    kernel_heap.max_size = MAX_HEAP_SIZE;
//    kernel_heap.max_entry_count = MAX_HEAP_ENTRIES;
    
    kernel_heap.entry_count = 0;
    kernel_heap.size = 0;
    kernel_heap.heap_start_address = __kernel_heap;
    kernel_heap.start_entry = allocate_heap_entry(&static_pool);
    kernel_heap.last_entry = kernel_heap.start_entry;
     
    kernel_heap.start_entry->start_address = __kernel_heap;
    kernel_heap.start_entry->size = 0;
    kernel_heap.start_entry->is_hole = true;
    kernel_heap.start_entry->next_entry = NULL;
    kernel_heap.start_entry->prev_entry = NULL;
}

static void* kernel_heap_alloc(uint32_t size) //This is the main part... It allocs stuffon heap
{
  while(1)
  {
    for(heap_entry* iterator = kernel_heap.start_entry;iterator;iterator=iterator->next_entry)
    {

      if (!iterator -> is_hole) continue;
      if (iterator -> size == size)
      {
        iterator -> is_hole = false;
        return iterator -> start_address;
      }
      if (iterator -> size > size)
      {
        iterator -> is_hole = false;
        heap_entry* temp = allocate_heap_entry(&static_pool);
        if(!temp) 
        {
          if(!make_new_pool(&static_pool)) goto failure;
          temp = allocate_heap_entry(&static_pool);
        }
        temp -> start_address = iterator -> start_address + size;
        temp -> size = (iterator -> size) - size;
        temp -> is_hole = true;
        temp -> next_entry = iterator -> next_entry;
        temp -> prev_entry = iterator;

        iterator -> size = size;
        iterator -> next_entry = temp;
        if (kernel_heap.last_entry == iterator) kernel_heap.last_entry = temp; //if temp.next == null

        return iterator -> start_address;
      }
    }
    //Now, we've seen that the heap is small 
    void* vma = expand_heap(STANDARD_INCREASE);
    if(!vma) goto failure;

    if(kernel_heap.last_entry -> is_hole)
      kernel_heap.last_entry -> size += STANDARD_INCREASE * PAGE_SIZE;
    else
    {
      heap_entry* temp = allocate_heap_entry(&static_pool);
      if(!temp)
      {
        if(!make_new_pool(&static_pool)) goto failure;
        temp = allocate_heap_entry(&static_pool);
      }
    temp -> start_address = vma;
    temp -> size = STANDARD_INCREASE * PAGE_SIZE;
    temp -> is_hole = true;
    temp -> next_entry = NULL; //Last entry
    temp -> prev_entry = kernel_heap.last_entry;
    kernel_heap.last_entry -> next_entry = temp;
    kernel_heap.last_entry = temp;
    }
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
        kernel_heap.entry_count --;
        iterator -> next_entry = iterator -> next_entry -> next_entry;
        if(iterator -> next_entry) iterator -> next_entry -> prev_entry = iterator;
      }
      if(iterator->prev_entry && iterator->prev_entry->is_hole) 
      {
        iterator -> start_address = iterator -> prev_entry -> start_address;
        iterator -> size += iterator -> prev_entry -> size;
        iterator -> prev_entry -> is_useful = false;
        kernel_heap.entry_count --;
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
void* kalloc(uint32_t size)
{
  if(size < MIN_ALLOC_SIZE) size = MIN_ALLOC_SIZE;
  if (!is_kheap)
  {
    initialize_kernel_heap();
    is_kheap = true;
  }

#ifdef __DEBUG
  void* ret = kernel_heap_alloc(size);
  uint32_t counter = 0;
  uint32_t sz = 0;
  for(heap_entry* iterator = kernel_heap.start_entry ; iterator ; iterator=iterator->next_entry)
  {
    counter++; sz += iterator -> size;
  }
  if(sz != kernel_heap.size) 
  {
  monitor_puts("\nDebug mode, entry count = ");printint(counter);
  monitor_puts("\nDebug mode, size = ");printint(sz);
  monitor_puts("\nDebug mode, actual size = ");printint(kernel_heap.size);
    monitor_puts("\nYou messed up");
    for(;;);
  }
  return ret;
#endif

  return kernel_heap_alloc(size);
}

bool kfree(void* address)
{
  void* h_st =  kernel_heap.heap_start_address;
  void* h_en =  h_st + kernel_heap.size;
  if((address < h_st) || (address > h_en) || !is_kheap) return false;
  return kernel_heap_free(address);
}


