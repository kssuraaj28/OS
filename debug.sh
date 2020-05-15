#!/bin/bash
set -e
./build.sh
./disk.sh
qemu-system-i386 -s -hda disk.img &
gdb -ex "target remote localhost:1234" -ex "symbol-file kernel/kernel.elf" -ex "b kmain" -ex "continue" 
