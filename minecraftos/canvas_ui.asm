; canvas_ui.asm - Fast UI drawing helpers (cdecl, 32-bit)
; Used alongside canvas_ui.c to accelerate horizontal fills

bits 32
section .text

; void ui_fill_hline_asm(uint8_t* dest, uint8_t color, uint32_t pixels)
; Fills 'pixels' bytes starting at 'dest' with 'color' using REP STOSB
global ui_fill_hline_asm
ui_fill_hline_asm:
    push    edi
    mov     edi, [esp + 8]      ; dest pointer
    movzx   eax, byte [esp + 12] ; color byte
    mov     ecx, [esp + 16]     ; pixel count
    test    ecx, ecx
    jz      .done
    rep     stosb
.done:
    pop     edi
    ret


; void ui_fill_rect_fast_asm(uint8_t* buf, int stride,
;                             int x, int y, int w, int h, uint8_t color)
; Fills a w×h rectangle into buf with given stride (320)
; cdecl args: buf, stride, x, y, w, h, color
global ui_fill_rect_fast_asm
ui_fill_rect_fast_asm:
    push    ebp
    mov     ebp, esp
    push    edi
    push    esi
    push    ebx

    ; args at ebp+8..ebp+32
    mov     edi, [ebp + 8]      ; buf base
    mov     eax, [ebp + 12]     ; stride (320)
    mov     ecx, [ebp + 16]     ; x
    mov     edx, [ebp + 20]     ; y
    ; compute start pointer: buf + y*stride + x
    imul    edx, eax            ; y * stride
    add     edx, ecx            ; + x
    add     edi, edx            ; edi = &buf[y*stride + x]

    mov     esi, [ebp + 24]     ; w  (columns)
    mov     ebx, [ebp + 28]     ; h  (rows)
    movzx   eax, byte [ebp + 32] ; color

    ; stride - w = bytes to skip to next row
    mov     edx, [ebp + 12]     ; stride
    sub     edx, esi            ; skip = stride - w

.row_loop:
    test    ebx, ebx
    jz      .done2
    mov     ecx, esi            ; pixels per row
    rep     stosb               ; fill row
    add     edi, edx            ; skip to next row start
    dec     ebx
    jmp     .row_loop
.done2:
    pop     ebx
    pop     esi
    pop     edi
    pop     ebp
    ret
