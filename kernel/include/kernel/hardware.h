#ifndef _KERNEL_HARDWARE_H
#define _KERNEL_HARDWARE_H

#include<stdbool.h>

//Keyboard
extern bool _is_keyboard_interrupt;
void keyboard_handler();

bool kbc_init();
char get_latest_char();
uint8_t get_latest_scan_code();
void wait_for_keyboard();

//ATA
void read_sectors_ATA_PIO(uint32_t target_address, uint32_t LBA, uint8_t sector_count);

//PIC
void pic_init();
void send_EOI_master();
void send_EOI_slave();

//Timer

extern bool _is_timer_interrupt;
void timer_handler();

void set_timer(uint16_t delay); 
void wait_for_timer();
uint32_t get_tick_count();
#endif
