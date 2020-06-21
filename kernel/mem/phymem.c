/**  
 * @file phymem.c
 * @brief ...
 * @see 
 */
// Standard includes TODO: Fine grain special blocks, Remove the toggle and get dirty?
#include<stdint.h> 
#include<stdbool.h>
#include<mem.h>
#include<tty.h>

// Constant definitions:
#define MEM_SIZE 0x100000000			/**< Size of physical memory */
#define PMMAP 0x1000				/**< Description here */
#define KERNEL_P 0x100000			/**< Description here */ //TODO: Get all this from bootloader
#define BLOCK_SIZE 4096				/**< Description here */
#define BLOCK_SIZE_B 12				/**< Description here */
#define BITS_PER_UINT32_B 5			/**< 32 Bits per int */




// Routines internal to object:
static void pmmngr_toggle_range(uint32_t start,uint32_t end);
static inline void pmmngr_toggle_block(uint32_t block_number);
static inline uint32_t block_number(uint32_t address);
static uint8_t get_lowest_bit(uint32_t hexinp);
static uint8_t extract_bit(uint32_t hexinp,uint8_t bitnumber); 


// Data structues and user defined data types:

static uint32_t physical_memory_bitmap[MEM_SIZE>>(BLOCK_SIZE_B+BITS_PER_UINT32_B)]; 
static uint32_t special_bitmap[KERNEL_P>>(BLOCK_SIZE_B+BITS_PER_UINT32_B)]; //A special bitmap that manages less than 1M

/** E820 Memory Map entry */
typedef struct mmap_entry
{
    uint32_t  startLo;
    uint32_t  startHi;
    uint32_t  sizeLo;
    uint32_t  sizeHi;
    uint32_t  type;
    uint32_t  acpi_3_0;
} mmap_entry_t;


// Function implementations:
/** @brief Initializes the page frame allocator
 * @param mapentrycount
 * @return  
 * */
void  pmmngr_init(uint32_t mapentrycount)   
{ 
	mmap_entry_t* map_ptr= (mmap_entry_t*)PMMAP;

	for (uint32_t i=0;i<MEM_SIZE>>(BLOCK_SIZE_B+BITS_PER_UINT32_B);i++)
	       	physical_memory_bitmap[i] = 0xffffffff; //Make everything 1 -- Everything is occupied initially
	for (uint32_t i=0;i<KERNEL_P>>(BLOCK_SIZE_B+BITS_PER_UINT32_B);i++)
	       	special_bitmap[i] = 0xffffffff; //Make everything 1 -- Everything is occupied initially
	for(uint32_t i=0;i<mapentrycount;i++)
	{
		if((map_ptr -> type == 1)&&(map_ptr -> startLo >= KERNEL_P) && !(map_ptr -> startHi))
			pmmngr_toggle_range(map_ptr->startLo, map_ptr->startLo + map_ptr ->sizeLo);
		map_ptr ++;
	}
	// 0 - 0x9F000 are usually free, I'll do a safe 0x20000 to 0x80000 of special page frames for stack and so on...??
	for(uint32_t sp_address = 0x20000;sp_address<0x80000;sp_address += 8*sizeof(uint32_t)*BLOCK_SIZE)
		special_bitmap[sp_address >> (BLOCK_SIZE_B+BITS_PER_UINT32_B)] = 0; //TODO: Don't hardcode this!!
	
	//Now we free the space occupied by the kernel and this memory manager :)
	uint32_t kernel_start = (uint32_t)__begin;
	uint32_t kernel_end = (uint32_t)__end;
	pmmngr_toggle_range (KERNEL_P,KERNEL_P + kernel_end - kernel_start); //Although the bootloader maps 4M, we don't care?? about anthing that comes later.. That's just garbage?? I can test by writing to heap??
//	pmmngr_toggle_range (KERNEL_P,KERNEL_P + KERNEL_MAPPED_SIZE);
}
/** @brief Returns a physical page frame
 * 
 * @return Page frame address
 * */
uint32_t pmmngr_allocate_block()
{
	uint32_t address;
	for( uint32_t i=0;i<MEM_SIZE/(BLOCK_SIZE*sizeof(uint32_t)*8);i++)
		if (physical_memory_bitmap[i] < 0xffffffff)
		{
			uint8_t bit = get_lowest_bit(physical_memory_bitmap[i]);  //bit lies from 0 to 31
				if(bit == 0xff) return 0;
			address = (i << (BLOCK_SIZE_B+BITS_PER_UINT32_B)) + (bit << (BLOCK_SIZE_B));
			pmmngr_toggle_block(block_number(address));
			return address;
		}
	return 0;
}

/** @brief Returns a special physical page frame
 * 
 * @return Page frame address
 * */
uint32_t allocate_special_block()
{
	uint32_t address;
	for (uint32_t i = 0; i< KERNEL_P/(BLOCK_SIZE*sizeof(uint32_t)*8);i++)
	if (special_bitmap[i] < 0xffffffff)
	{
			uint8_t bit = get_lowest_bit(special_bitmap[i]);  //bit lies from 0 to 31
				if(bit == 0xff) return 0;
			address = (i << (BLOCK_SIZE_B+BITS_PER_UINT32_B)) + (bit << (BLOCK_SIZE_B));
			special_bitmap[address >> (BLOCK_SIZE_B+BITS_PER_UINT32_B)] ^= (1ul << ((address>>BLOCK_SIZE_B) & 31)); //Toggling the bit 
			return address;
	}
	return 0;
}
/** @brief Frees a special page
 * 
 * @return True/false
 * */
bool free_special_block(uint32_t address)
{
	if(address >= KERNEL_P || address % BLOCK_SIZE) return false;
	uint32_t block = block_number((uint32_t)address);
	uint32_t dword = block >> 5;
	uint8_t offset = block % 32;
	if(special_bitmap[dword]&(1<<offset)) return 0;
	special_bitmap[block >> 5] ^= (1ul << (block & 31));
	return true;
}
/** @brief ...
 * @param address 
 * @return  
 * */
bool pmmngr_free_block(uint32_t address)
{
	if((uint32_t)address % BLOCK_SIZE != 0) return 0;
	uint32_t block = block_number((uint32_t)address);
	uint32_t dword = block >> 5;
	uint8_t offset = block % 32;
	if(!extract_bit((uint32_t)(physical_memory_bitmap + dword),offset)) return 0;
	pmmngr_toggle_block(block);
	return 1;

//	return 0;

}


/** @brief ...
 * 
 * @return  
 * */
// Helper function implementations:
/** @brief ...
 * @param hexinp
 * @return  
 * */
static uint8_t get_lowest_bit(uint32_t hexinp)
{
	for(int i=0;i<32;i++)
	{
		if ((hexinp%2) == 0)
			return i;
		hexinp >>= 1;
	}
	return 0xff;
}
/** @brief ...
 * @param hexinp
 * @param bitnumber
 * @return  
 * */
static inline uint8_t extract_bit(uint32_t hexinp,uint8_t bitnumber)  //bitnumber < 32
{
	return (hexinp >> bitnumber) & 1;
}
/** @brief ...
 * @param address
 * @return  
 * */
static inline uint32_t block_number(uint32_t address)  //TODO: MACRO??
{
	return address >> BLOCK_SIZE_B;
}
/** @brief ...
 * @param block_number
 * @return  
 * */
static inline void pmmngr_toggle_block(uint32_t block_number) 
{
	physical_memory_bitmap[block_number >> 5] ^= (1ul << (block_number & 31));
}
/** start and end are addresses**/
/** @brief Toggles the bits for a specified range
 * @param start
 * @param end
 * @return  
 * */
static void pmmngr_toggle_range(uint32_t start,uint32_t end)    //Optimize the crap out of this later
{
	if (start % BLOCK_SIZE != 0){start -= (start%BLOCK_SIZE_B);}
	if (end % BLOCK_SIZE != 0){end -= (end%BLOCK_SIZE_B); end += BLOCK_SIZE;}
	while((end - start) >= BLOCK_SIZE)
	{
		
		if((end - start) >= 32* BLOCK_SIZE)
		{
			uint32_t* byte = (uint32_t*)(block_number(start) >> 3);
			byte = (uint32_t*)((uint8_t*)byte +(uint32_t)physical_memory_bitmap);
			*byte ^= 0xffffffff;
			start += (BLOCK_SIZE<<5);
			
		}
		else
		{
			pmmngr_toggle_block(block_number(start));
			start += BLOCK_SIZE;
		}
	}
}

//	_lazy_byte = (uint32_t*)start;
//static uint32_t* _lazy_byte;
//	physical_memory_bitmap = h_end;   //Debug here!
//	uint32_t kernel_block_count  = kernelsize >> SECTORS_PER_BLOCK_B;
//
//
/*
	printhex((uint32_t)physical_memory_bitmap);


	for (int i = 0; i< mapentrycount ; i++)
        {
                monitor_puts("\nStarting address:"); printhex((map_ptr+i) -> startLo);
                monitor_puts("\tSize: "); printhex((map_ptr+i) -> sizeLo);
                monitor_puts("\tType:"); printhex((map_ptr+i) -> type);
         }
*/
	
	/*
	mmap_entry_t* map_ptr= (mmap_entry_t*)PMMAP;
	for( uint32_t i=0;i<_mapentrycount;i++)
	{
		if(((map_ptr->startLo) <= (uint32_t)address)&& (((uint32_t)address-map_ptr->startLo) < map_ptr->sizeLo))
		{
			if (map_ptr -> type ==1)
			{

				uint32_t block = block_number((uint32_t)address);
				uint32_t dword = block >> 5;
				uint8_t offset = block % 32;
				if(extract_bit((uint32_t)(physical_memory_bitmap + dword),offset)) return 0;
				pmmngr_toggle_block(block);
				return 1;

			}
			else return 0;
		}
		map_ptr ++;
	}
	*/

