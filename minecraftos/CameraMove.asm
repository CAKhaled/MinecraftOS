global read_scancode

section .text

; uint8_t read_scancode();
; Returns the scancode from the keyboard if available, otherwise returns 0.
read_scancode:
    in al, 0x64       ; Read Keyboard Controller Status Register
    test al, 1        ; Check bit 0 (Output Buffer Status)
    jz .no_data       ; If 0, no data is available
    test al, 0x20     ; Check bit 5 (Mouse Data)
    jnz .no_data      ; If 1, data belongs to the mouse, DO NOT read it here!
    in al, 0x60       ; Read the scancode from the data port
    ret
.no_data:
    mov eax, 0        ; Return 0
    ret
