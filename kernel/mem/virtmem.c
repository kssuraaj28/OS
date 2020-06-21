/**  
 * @file virtualmem.c
 * @brief I need to allocate pages for page tables from below 1M, that seems better??
 * @see 
 */
/* Don't do a permanent identity map dude
 * Our strategy will be recursive identity map
 * You need this or else you're dead
 */

#define BLOCK_SIZE 4096				/**< Description here */
#define STACK 0xFFC00000			/**< Description here */
#define STACK_PHY 0x90000			/**< Description here */
#define VGA_TEXT 0xB8000			/**< Description here */

#include <stdint.h>
#include <stdbool.h>
#include <tty.h>
#include <hal.h>
#include <mem.h>

static uint32_t* _page_directory = (uint32_t*)0x9C000;  //This is the initial virtual address of the page directory


// Source: BrokenThorn 
/**< Page table entry flags */
enum PAGE_PTE_FLAGS {
	PTE_PRESENT		=	1,
	PTE_WRITABLE		=	2,
	PTE_USER		=	4,
	PTE_WRITETHOUGH		=	8,
	PTE_NOT_CACHEABLE	=	0x10,
	PTE_ACCESSED		=	0x20,
	PTE_DIRTY		=	0x40,
	PTE_PAT			=	0x80,
	PTE_CPU_GLOBAL		=	0x100,
	PTE_LV4_GLOBAL		=	0x200,
   	PTE_FRAME		=	0xFFFFF000 
};
/**< Page directory entry flags */
enum PAGE_PDE_FLAGS {
 
	PDE_PRESENT		=	1,
	PDE_WRITABLE		=	2,
	PDE_USER		=	4,
	PDE_PWT			=	8,	
	PDE_PCD			=	0x10,
	PDE_ACCESSED		=	0x20,
	PDE_DIRTY		=	0x40,
	PDE_4MB			=	0x80,
	PDE_CPU_GLOBAL		=	0x100,
	PDE_LV4_GLOBAL		=	0x200,	
   	PDE_FRAME		=	0xFFFFF000 
};

//Implementations
/** @brief Sets up recursive page tables
 * 
 * @return  
 * */
static void set_recursive_map()  
{
	uint32_t phy_dir = get_pdbr();
	uint32_t* vir_dir = _page_directory;
	vir_dir[1023]=phy_dir;
	vir_dir[1023]|=PDE_PRESENT;
	vir_dir[1023]|=PDE_WRITABLE;
	_page_directory = (uint32_t*)PAGE_DIRECTORY; 
	flush_tlb();
}


/** @brief Initializes good paging support
 *
 * @return  
 * */
void vmmngr_init() 
{
	set_recursive_map();

	if(!map_page(__VGA_text_memory,VGA_TEXT,false,true))
		for(;;); //monitor_puts("VGA remap failed");  TODO//Keep another debug print
	if(!map_page((void*)STACK-PAGE_SIZE,STACK_PHY-PAGE_SIZE,false,true))
		monitor_puts("Stack remap failed");
	if ((uint32_t)__end - (uint32_t)__begin > (2<<22))  
	{
		monitor_puts("Kernel spans more than 4M");
		for(;;);
	}
	
}

/** @brief Maps a given virtual address to a physical address
 * @param virtual_address 
 *
 * @param physical_address
 * @param isUser
 * @param isWritable
 * @return  
 * */
bool map_page(void* virtual_address_ptr,uint32_t physical_address,bool isUser,bool isWritable)   //TODO: Need to deal with freed frames  && Make this cleaner!!
{
  uint32_t virtual_address = (uint32_t)virtual_address_ptr;
	virtual_address -= (virtual_address%BLOCK_SIZE) ? virtual_address%BLOCK_SIZE : 0;
	physical_address -= (physical_address%BLOCK_SIZE) ? physical_address%BLOCK_SIZE : 0;

	uint32_t pd_index = virtual_address >> 22;
	if (!(_page_directory[pd_index] & PDE_PRESENT))
	{
		//Allocate a special page here!!
		uint32_t page_table = allocate_special_block();
		if(!page_table) return false;
		_page_directory[pd_index] = page_table|PDE_PRESENT|PDE_WRITABLE;
	}

	if(isWritable) _page_directory[pd_index] |= PDE_WRITABLE;
	if(isUser) _page_directory[pd_index]|= PDE_USER;

	uint32_t* page_table = (uint32_t*)(PAGE_TABLE | (pd_index<<12)); //This is the virtual address of the page table 
	uint32_t pt_index = ((virtual_address >> 12) & 0x3FF); //Using recursive page table technique

	page_table[pt_index] = physical_address;
	page_table[pt_index] |= PTE_PRESENT;
	page_table[pt_index] |= PTE_WRITABLE;
	
	if(isUser)
       	{
		page_table[pt_index]|= PTE_USER;
	       	if(isWritable) page_table[pt_index] |= PTE_WRITABLE;
		else page_table[pt_index] &= ~PTE_WRITABLE;
	}
	else 
	{
		page_table[pt_index] &= ~PTE_USER;
	       	page_table[pt_index] |= PTE_WRITABLE;   //Writable anyway in kmode
	}

	flush_tlb_entry(virtual_address);
	return true;
}

/** @brief Converts a virtual address into a physical address
 * @param virt - The virtual address
 * 
 * @return - The physical address
 * */
uint32_t virtual_to_physical (uint32_t* virt)
{
	uint32_t virtual_address = (uint32_t)virt;
	uint32_t pd_index = virtual_address >> 22;

	if (!(_page_directory[pd_index] & PDE_PRESENT)) return 0;

	uint32_t* page_table = (uint32_t*)(PAGE_TABLE | (pd_index<<12)); //This is the virtual address of the page table 
	uint32_t pt_index = ((virtual_address >> 12) & 0x3FF); //Using recursive page table technique

	if (!(page_table[pt_index] & PTE_PRESENT)) return 0;
	return ((page_table[pt_index] & PTE_FRAME) | (virtual_address & ~PTE_FRAME));
}

/** @brief ...
 * @param vma The virtual address
 * 
 * @return  
 * */
void free_vma(uint32_t* vma)  //Marks virtual address as not present
{
	uint32_t virtual_address = (uint32_t)vma;
	uint32_t pd_index = virtual_address >> 22;

	if (!(_page_directory[pd_index] & PDE_PRESENT)) return;

	uint32_t* page_table = (uint32_t*)(PAGE_TABLE | (pd_index<<12)); //This is the virtual address of the page table 
	uint32_t pt_index = ((virtual_address >> 12) & 0x3FF); //Using recursive page table technique
	page_table[pt_index] = 0;
}
