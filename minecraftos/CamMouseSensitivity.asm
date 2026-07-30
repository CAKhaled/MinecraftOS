global mouse_inb
global mouse_outb

section .text

; uint8_t mouse_inb(uint16_t port)
mouse_inb:
    push ebp
    mov ebp, esp
    mov dx, [ebp+8] ; port
    in al, dx
    pop ebp
    ret

; void mouse_outb(uint16_t port, uint8_t val)
mouse_outb:
    push ebp
    mov ebp, esp
    mov dx, [ebp+8] ; port
    mov al, [ebp+12] ; val
    out dx, al
    pop ebp
    ret
