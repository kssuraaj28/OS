struct context{ //Non caller saved
reg_t eip;  //The reason eip is present in the context is extremely clever....
reg_t ebp;
reg_t ebx;
reg_t esi;
reg_t edi;
};

; context_switch( struct context** old, struct context * new)
global context_switch
context_switch:  ;We could do a pusha, then switch stack, then popa
cli

mov eax, [esp + 4]
mov edx, [esp + 8]

push ebp  ;We save context on the kernel stack
push ebx
push esi
push edi 

;Now we have to switch stacks
mov [eax], esp
mov esp, edx

pop edi 
pop esi
pop ebx
pop ebp  

sti
ret
