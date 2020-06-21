/* Process memory layout:
 * text-data-bss-stack-heap-kernel
 * */



#include <stdint.h>
#include <interrupt.h>

// I really need to treat trap as a function call. Return value MUST be in eax.
// I expect a usecase like... hmmm, I'm not sure... Maybe I can provide headers
// in someplace

#define MAX_THREAD 4 //Do we need this??
#define MAX_PROC 16  //TODO

#define KERNEL_STACK_SIZE 1024

typedef uint32_t reg_t;

enum procstate {UNUSED, EMBRYO, SLEEPING, RUNNABLE, RUNNING };

struct context{ //Non caller saved
reg_t eip;  //The reason eip is present in the context is extremely clever....
reg_t ebp;
reg_t ebx;
reg_t esi;
reg_t edi;
};

struct proc{
  uint32_t* kernel_stack;  //Need to update the TSS during context switch
  uint32_t sz;
  uint32_t pid;
  enum procstate state;
  uint32_t pdbr;
  struct trapframe* tf; //Trap frame for current syscall....  For USER mode threads
  struct context* context; //This is for the KERNEL mode regs We swtch here to run process
  struct proc* parent;
  // We need file stuff later
};


struct proc_table{
  struct proc array[MAX_PROC]; //TODO: Make this dynamic
  struct proc_table *next;
  //Some locks and all
} ptable;

//This should just load a process in memory, we can execute later
struct proc create_process_image(char* name)
{
	load();
	parse();
	place();
	create_page_directory();

}

inline void init_proc_table(struct proc_table* ptr)
{
  //Once we get a good slab allocator, we can do something with this this??
  memset(ptr,0,sizeof(struct proc_table));
}

int32_t alloc_proc() //This should return the PID of the process -- Only seach and allocate process
{
  static bool is_proc_init=0;
  if (!is_proc_init) {init_proc_table(&ptable); is_proc_init = 1;}

  struct proc_table* ptr = &ptable;
  struct proc* np;
  uint32_t pid = 0;

  while (ptr)
  {
    for (int i=0; i<MAX_PROC; i++)
    {
      if(ptr->array[i].state !=  UNUSED) continue;

      pid +=i;
      np = &ptr->array[i];

      //xv6 sets up the kernel stack as well as the trap frame and context,,,, Hmm.... I chose not to dothis
      return pid;
    }
    pid += MAX_PROC
    ptr = ptr -> next;
  }
return -1;
}
