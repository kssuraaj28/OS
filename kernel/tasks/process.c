#define PROCESS_STATE_SLEEPING 0
#define PROCESS_STATE_ACTIVE 1

#define MAX_THREAD 4

typedef struct process{




} process_t;

//This should just load a process in memory, we can execute later
process_t create_process_image(char* name)
{
	load();
	parse();
	place();
	create_page_directory();

}
