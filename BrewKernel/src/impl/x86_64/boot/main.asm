;
; Brew Kernel
; Copyright (C) 2024-2025 boreddevnl
;
; This program is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation, either version 3 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program. If not, see <https://www.gnu.org/licenses/>.
;

global start
extern long_mode_start

section .text
bits 32
start:
    ; Initialize stack pointer
    mov esp, stack_top

    ; Perform necessary system checks before transitioning to long mode
    call check_multiboot
    call check_cpuid
    call check_long_mode

    ; Set up memory paging and enable long mode
    call setup_page_tables
    call enable_paging

    ; Load Global Descriptor Table for 64-bit mode and jump to long mode
    lgdt [gdt64.pointer]
    jmp gdt64.code_segment:long_mode_start

    hlt

; Verify if kernel was loaded by multiboot compliant bootloader
check_multiboot:
    cmp eax, 0x36d76289 ; Expect magic number 0x36d76289 in eax register
    jne .no_multiboot
    ret
.no_multiboot:
    mov al, "M"    ; M indicates Multiboot check failure
    jmp error

; Test if CPUID instruction is available by attempting to flip bit 21 in FLAGS register
check_cpuid:
    pushfd                  ; Store FLAGS
    pop eax                 ; Get FLAGS into eax
    mov ecx, eax           ; Store copy in ecx
    xor eax, 1 << 21       ; Flip ID bit
    push eax               ; Store modified FLAGS
    popfd                  ; Load modified FLAGS
    pushfd                 ; Store FLAGS again
    pop eax                ; Get FLAGS into eax
    push ecx               ; Restore original FLAGS
    popfd
    cmp eax, ecx          ; Check if bit was actually flipped
    je .no_cpuid
    ret
.no_cpuid:
    mov al, "C"           ; C indicates CPUID not available
    jmp error

; Check if CPU supports 64-bit long mode
check_long_mode:
    mov eax, 0x80000000    ; Test if extended processor info available
    cpuid
    cmp eax, 0x80000001    ; Check if CPU supports extended features
    jb .no_long_mode

    mov eax, 0x80000001    ; Get extended processor info
    cpuid
    test edx, 1 << 29      ; Test LM bit
    jz .no_long_mode
    
    ret
.no_long_mode:
    mov al, "L"            ; L indicates Long Mode not supported
    jmp error

; Function: setup_page_tables
; Sets up a basic page table hierarchy for identity mapping
; Maps first 1GB + PCI MMIO region (0xFE800000-0xFF000000)
setup_page_tables:
    ; Map level 4 table to level 3 table (L4[0] -> L3)
    mov eax, page_table_l3
    or eax, 0b11           ; Present + Writable
    mov edx, 0
    mov [page_table_l4], eax       ; Store low 32 bits
    mov [page_table_l4 + 4], edx   ; Store high 32 bits (0)
    
    ; Map level 3 table to level 2 table (L3[0] -> L2)
    ; This covers the first 1GB (0x00000000-0x3FFFFFFF)
    mov eax, page_table_l2
    or eax, 0b11           ; Present + Writable
    mov edx, 0
    mov [page_table_l3], eax       ; Store low 32 bits
    mov [page_table_l3 + 4], edx   ; Store high 32 bits (0)

    ; Identity map first 1GB using 2MB pages
    ; L2 entries 0-511 cover addresses 0x00000000-0x3FFFFFFF
    mov ecx, 0 
.loop:
    mov eax, 0x200000      ; 2MB per page
    mul ecx                ; Calculate physical address: ecx * 2MB
    or eax, 0b10000011     ; Present + Writable + Huge Page
    mov edx, 0             ; Clear upper 32 bits for 64-bit entry
    mov [page_table_l2 + ecx * 8], eax      ; Store low 32 bits
    mov [page_table_l2 + ecx * 8 + 4], edx  ; Store high 32 bits (0)

    inc ecx 
    cmp ecx, 512           ; Map 512 entries (1GB total)
    jne .loop 
    
    ; Map PCI MMIO region (0xFE800000 - 0xFF000000)
    ; Address 0xFE800000 = 4,272,629,760 bytes = ~4GB
    ; This is still in the first 4GB, so it uses L4[0] -> L3[0] -> L2
    ; But L2 only has 512 entries covering 1GB, so we need a different L3 entry
    ; Actually, 0xFE800000 / (512 * 2MB) = 0xFE800000 / 0x40000000 = 3.99
    ; So we need L3[3] for addresses 0xC0000000-0xFFFFFFFF
    
    ; Set up L3[3] to point to a second L2 table for the 4th GB
    ; But we don't have a second L2 table allocated...
    ; Actually, we can reuse the same L2 table since we're using huge pages
    ; Wait, that won't work because L2[0] would map to 0x00000000, not 0xC0000000
    
    ; Better approach: Map the entire 4th GB (0xC0000000-0xFFFFFFFF) using L3[3]
    ; But we need another L2 table for that, or we can map it differently
    
    ; Actually, let's just map the MMIO region directly in L2
    ; But wait, L2[0x7F4] would map to 0xFE800000 if we're using the same L2 table
    ; The issue is that L3[0] only covers the first 1GB, so L2 entries above 511 aren't used
    
    ; Set up L3[3] to cover the 4th GB (0xC0000000-0xFFFFFFFF) and point it to a separate L2 table
    mov eax, page_table_l2_mmio
    or eax, 0b11           ; Present + Writable  
    mov edx, 0
    mov [page_table_l3 + 3 * 8], eax       ; L3[3] -> page_table_l2_mmio (covers 0xC0000000-0xFFFFFFFF)
    mov [page_table_l3 + 3 * 8 + 4], edx
    
    ; Now map the MMIO region in the MMIO L2 table (identity mapping)
    ; With L3[3] (covers 0xC0000000-0xFFFFFFFF), L2 entry N maps virtual address 0xC0000000 + (N * 2MB)
    ; For identity mapping of 0xFE800000: 
    ;   Virtual address = 0xFE800000
    ;   Physical address = 0xFE800000 (identity)
    ;   N = (0xFE800000 - 0xC0000000) / 0x200000 = 0x1F4
    ;   L2[0x1F4] should contain physical address 0xFE800000
    mov ecx, 0x1F4         ; Start at entry 0x1F4 (maps virtual 0xFE800000)
.map_mmio:
    ; Calculate physical address for identity mapping
    ; Virtual 0xFE800000 + (ecx - 0x1F4) * 2MB = Physical address
    mov eax, ecx
    sub eax, 0x1F4         ; Offset from start (0, 1, 2, or 3)
    mov ebx, 0x200000      ; 2MB per page
    mul ebx                ; eax = offset * 2MB
    add eax, 0xFE800000    ; Add base: 0xFE800000 + offset
    or eax, 0b10000011     ; Present + Writable + Huge Page
    mov edx, 0             ; Clear upper 32 bits
    mov [page_table_l2_mmio + ecx * 8], eax      ; Store low 32 bits in MMIO L2 table
    mov [page_table_l2_mmio + ecx * 8 + 4], edx  ; Store high 32 bits (0)
    
    inc ecx
    cmp ecx, 0x1F8         ; Map 4 entries (0x1F4-0x1F7, covers 0xFE800000-0xFF000000)
    jne .map_mmio

    ret

; Function: enable_paging
; Enables paging and switches to long mode
enable_paging:
    ; Load page table address
    mov eax, page_table_l4
    mov cr3, eax

    ; Enable PAE
    mov eax, cr4
    or eax, 1 << 5         ; Set PAE bit
    mov cr4, eax

    ; Enable long mode
    mov ecx, 0xC0000080    ; EFER MSR
    rdmsr
    or eax, 1 << 8         ; Set LM bit
    wrmsr

    ; Enable paging
    mov eax, cr0
    or eax, 1 << 31        ; Set PG bit
    mov cr0, eax

    ret

; Display error code on screen and halts
; Input: AL register contains error code character
error:
    ; Write "ERR: X" to video memory where X is the error code
    mov dword [0xb8000], 0x4f524f45   ; "ER"
    mov dword [0xb8004], 0x4f3a4f52   ; "R:"
    mov dword [0xb8008], 0x4f204f20   ; "  "
    mov byte  [0xb800a], al           ; Error code
    hlt

; BSS section - uninitialized data
section .bss
align 4096
page_table_l4:             
    resb 4096
page_table_l3:             
    resb 4096
page_table_l2:           ; L2 table for first 1GB
    resb 4096
page_table_l2_mmio:      ; L2 table for MMIO region (4th GB)
    resb 4096
stack_bottom:              ; Kernel stack (16KB)
    resb 4096 * 4
stack_top:

; Read-only data section
section .rodata
gdt64:                     ; Global Descriptor Table for 64-bit mode
    dq 0                   ; Null descriptor
.code_segment: equ $ - gdt64
    ; 64-bit code segment descriptor:
    ; - Execute/Read
    ; - 64-bit mode
    ; - Present
    ; - Ring 0 (kernel)
    dq (1 << 43) | (1 << 44) | (1 << 47) | (1 << 53) 
.pointer:                  ; GDT pointer structure
    dw $ - gdt64 - 1      ; Size
    dq gdt64              ; Address
