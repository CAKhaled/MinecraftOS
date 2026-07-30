global update_background_music

section .data
; Divisors for notes (1193180 / freq)
; Beethoven - Für Elise (Most famous piano melody)
melody:
    ; E5, D#5, E5, D#5, E5, B4, D5, C5, A4
    dw 1810, 1918, 1810, 1918, 1810, 2415, 2032, 2281, 2711, 0, 0
    ; C4, E4, A4, B4
    dw 4571, 3626, 2711, 2415, 0, 0
    ; E4, G#4, B4, C5
    dw 3626, 2875, 2415, 2281, 0, 0
    ; E4, E5, D#5, E5, D#5, E5, B4, D5, C5, A4
    dw 3626, 1810, 1918, 1810, 1918, 1810, 2415, 2032, 2281, 2711, 0, 0
    
melody_len equ ($ - melody) / 2

current_note dd 0
frame_counter dd 0
frames_per_note dd 12  ; Fast tempo for Für Elise

section .text

update_background_music:
    push eax
    push ebx
    push ecx

    ; Increment frame counter
    mov eax, [frame_counter]
    inc eax
    mov [frame_counter], eax
    
    ; Check if we should mute (create a gap between notes)
    mov ebx, [frames_per_note]
    sub ebx, 2
    cmp eax, ebx
    jne .check_next_note
    
    ; Mute speaker (creates staccato/piano feel)
    in al, 0x61
    and al, 0xFC
    out 0x61, al
    jmp .done

.check_next_note:
    mov ebx, [frames_per_note]
    cmp eax, ebx
    jl .done
    
    ; Reset frame counter
    mov dword [frame_counter], 0
    
    ; Get current note index
    mov eax, [current_note]
    
    ; Play note
    movzx ecx, word [melody + eax * 2]
    
    ; If divisor is 0, it's a rest
    cmp ecx, 0
    je .rest_note
    
    ; Set PIT command
    mov al, 0xB6
    out 0x43, al
    
    ; Send divisor in ecx
    mov al, cl
    out 0x42, al
    mov al, ch
    out 0x42, al
    
    ; Enable speaker
    in al, 0x61
    or al, 3
    out 0x61, al
    jmp .next_note_logic

.rest_note:
    ; Mute speaker for rest
    in al, 0x61
    and al, 0xFC
    out 0x61, al

.next_note_logic:
    ; Move to next note
    mov eax, [current_note]
    inc eax
    cmp eax, melody_len
    jl .save_note
    mov eax, 0 ; loop melody
.save_note:
    mov [current_note], eax

.done:
    pop ecx
    pop ebx
    pop eax
    ret
