#ifndef _KERNEL_INTERRUPT_H
#define _KERNEL_INTERRUPT_H
#include<stdint.h>

typedef uint32_t reg32_t;
typedef uint32_t seg16_t;
/** Interrupt frame*/
typedef struct interrupt_frame {
	seg16_t gs;
	seg16_t fs;
	seg16_t es;
	seg16_t ds;

	reg32_t edi;
	reg32_t esi;
	reg32_t ebp;
	reg32_t esp; //Ignore this
	reg32_t ebx;
	reg32_t edx;
	reg32_t ecx;
	reg32_t eax;

	uint32_t vector_number;
	uint32_t error_code;
	reg32_t eip;
	seg16_t cs;
	uint32_t flag;
}interrupt_frame_t;

void install_ir(uint32_t index,uint16_t flags, uint16_t sel, uint32_t* handler_address);
void interrupt_init();
#endif
