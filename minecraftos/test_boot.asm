MBALIGN  equ  1 << 0
MEMINFO  equ  1 << 1
VIDEOMODE equ 1 << 2
FLAGS    equ  MBALIGN | MEMINFO | VIDEOMODE
MAGIC    equ  0x1BADB002
CHECKSUM equ -(MAGIC + FLAGS)

section .multiboot
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM
    dd 0, 0, 0, 0, 0
    dd 0 ; linear
    dd 640
    dd 480
    dd 32

section .bss
align 16
stack_top:
resb 4096

section .text
global _start
extern kernel_main
_start:
    mov esp, stack_top
    push ebx
    push eax
    call kernel_main
.hang: hlt
    jmp .hang
