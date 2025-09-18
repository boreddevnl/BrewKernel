; ==================================================================
; BrewKernel -- Keyboard Handling Routines
; ==================================================================

global check_keyboard
global read_scan_code
global scan_code_to_ascii
global is_shift_pressed

section .data
; Keyboard state
shift_pressed db 0

; Scan code to ASCII mapping table (unshifted keys)
ascii_table:
    db 0,    27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 8    ; 0-0E
    db 9,    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 10      ; 0F-1C
    db 0,    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', 39, '`'           ; 1D-29
    db 0,    '\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*'       ; 2A-36
    db 0,    32,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0       ; 37-44
    db 0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0            ; 45-51
    times 208 db 0  ; Fill rest of 256 byte table with zeros

; Scan code to ASCII mapping table (shifted keys)
ascii_shifted_table:
    db 0,    27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 8    ; 0-0E
    db 9,    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 10      ; 0F-1C
    db 0,    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', 34, '~'           ; 1D-29
    db 0,    '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*'       ; 2A-36
    db 0,    32,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0       ; 37-44
    db 0,    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0            ; 45-51
    times 208 db 0  ; Fill rest of 256 byte table with zeros

section .text
bits 64

; ------------------------------------------------------------------
; check_keyboard -- Check if a key is available
; OUT: RAX = 0 if no key pressed, otherwise 1

check_keyboard:
    push rbx
    
    ; Check keyboard status port (0x64)
    in al, 0x64
    test al, 1          ; Check if bit 0 is set (output buffer status)
    jz .no_key
    
    mov rax, 1          ; Key is available
    jmp .done
    
.no_key:
    xor rax, rax        ; No key available
    
.done:
    pop rbx
    ret

; ------------------------------------------------------------------
; read_scan_code -- Read a scan code from the keyboard
; OUT: RAX = scan code

read_scan_code:
    push rbx
    
    ; Read from keyboard data port (0x60)
    in al, 0x60
    
    ; Convert to 64-bit return value
    movzx rax, al
    
    pop rbx
    ret

; ------------------------------------------------------------------
; is_shift_pressed -- Check if shift is currently pressed
; OUT: RAX = 1 if shift is pressed, 0 otherwise

is_shift_pressed:
    mov al, [shift_pressed]
    movzx rax, al
    ret

; ------------------------------------------------------------------
; update_shift_state -- Internal function to update shift key state
; IN: RDI = scan code
; Preserves all registers except RAX

update_shift_state:
    push rbx
    
    ; Check for left shift press
    cmp rdi, 0x2A
    je .shift_press
    
    ; Check for right shift press
    cmp rdi, 0x36
    je .shift_press
    
    ; Check for left shift release
    cmp rdi, 0xAA
    je .shift_release
    
    ; Check for right shift release
    cmp rdi, 0xB6
    je .shift_release
    
    jmp .done
    
.shift_press:
    mov byte [shift_pressed], 1
    jmp .done
    
.shift_release:
    mov byte [shift_pressed], 0
    
.done:
    pop rbx
    ret

; ------------------------------------------------------------------
; scan_code_to_ascii -- Convert a scan code to an ASCII character
; IN: RDI = scan code
; OUT: RAX = ASCII character (0 if not a printable character)

scan_code_to_ascii:
    push rbx
    
    ; First update the shift state
    call update_shift_state
    
    ; Check if scan code is within valid range (0-51h)
    cmp rdi, 0x51
    ja .not_ascii
    
    ; Check if this is a key release (bit 7 set)
    test rdi, 0x80
    jnz .not_ascii
    
    ; Get ASCII value from appropriate table based on shift state
    mov al, [shift_pressed]
    test al, al
    jz .unshifted
    
    ; Use shifted table
    mov al, [ascii_shifted_table + rdi]
    jmp .convert
    
.unshifted:
    mov al, [ascii_table + rdi]
    
.convert:
    movzx rax, al
    jmp .done
    
.not_ascii:
    xor rax, rax        ; Return 0 for invalid scan codes
    
.done:
    pop rbx
    ret