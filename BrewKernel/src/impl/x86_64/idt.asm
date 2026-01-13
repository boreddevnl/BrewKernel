;
; Brew Kernel
; Copyright (C) 2024-2026 boreddevnl
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

;
; Interrupt Descriptor Table (IDT)
; This file implements the x86_64 interrupt handling infrastructure
; It sets up the IDT and provides basic interrupt service routines (ISRs)
;

bits 64

; Constants for IDT structure
%define IDT_ENTRIES 256        ; Total number of possible interrupt vectors
%define IDT_ENTRY_SIZE 16      ; Size of each IDT entry in bytes

section .data
    ; IDT pointer structure (used by LIDT instruction)
    idt_ptr:
        dq IDT_ENTRIES * IDT_ENTRY_SIZE - 1  ; Size of IDT - 1
        dq idt                               ; Base address of IDT

section .bss
    ; Reserve space for the IDT
    idt resb IDT_ENTRIES * IDT_ENTRY_SIZE    ; Uninitialized IDT array

section .text
global load_idt

; Function: load_idt
; Loads the IDT pointer into the CPU's IDTR register
load_idt:
    lidt [idt_ptr]                 ; Load IDT pointer
    ret

; Function: set_idt_entry
; Sets up an IDT entry
; Input:
;   RAX = Address of ISR
;   RBX = Interrupt vector number
;   RCX = Segment selector (should be 0 for 64-bit)
set_idt_entry:
    mov rdx, rax                           ; Save ISR address
    
    shl rbx, 4                             ; Multiply vector by 16 (entry size)
    lea rsi, [idt + rbx]                   ; Calculate entry address
    
    ; Store low 16 bits of ISR address
    mov word [rsi], dx
    ; Store next 16 bits
    shr rdx, 16
    mov word [rsi + 6], dx
    ; Store high 32 bits
    shr rdx, 16
    mov dword [rsi + 8], edx
    
    ; Set segment selector (0 for 64-bit)
    mov word [rsi + 2], cx
    ; Set flags: P=1, DPL=0, Type=0xE (64-bit interrupt gate)
    mov byte [rsi + 5], 0x8E
    ret

section .text

; Interrupt Service Routines

; ISR 0: Division by Zero Exception Handler
isr_divide_by_zero:
    cli                                    ; Disable interrupts
    hlt                                    ; Halt the CPU
    iretq                                  ; Return from interrupt (64-bit)

; ISR 1: Debug Exception Handler
isr_debug:
    cli                                    ; Disable interrupts
    hlt                                    ; Halt the CPU
    iretq                                  ; Return from interrupt

; ISR 14: Page Fault Exception Handler
; In 64-bit mode, the error code is pushed automatically
; CR2 contains the faulting address
isr_page_fault:
    cli                                    ; Disable interrupts
    ; Get faulting address from CR2
    mov rax, cr2
    ; For now, just halt - in a real OS you'd handle this
    ; TODO: Print error message with fault address
    hlt                                    ; Halt the CPU
    iretq                                  ; Return from interrupt

; External C function for IRQ dispatching
extern irq_dispatcher
extern timer_tick

; Generic IRQ handler stub
; This is called for each IRQ and dispatches to the C handler
%macro IRQ_ISR 1
irq%1:
    push rax
    push rcx
    push rdx
    push rbx
    push rbp
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
    
    mov rdi, %1                            ; Pass IRQ number as first argument
    call irq_dispatcher                    ; Call C dispatcher
    
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rbp
    pop rbx
    pop rdx
    pop rcx
    pop rax
    iretq
%endmacro

; Create IRQ handlers for IRQ 0-15
IRQ_ISR 0
IRQ_ISR 1
IRQ_ISR 2
IRQ_ISR 3
IRQ_ISR 4
IRQ_ISR 5
IRQ_ISR 6
IRQ_ISR 7
IRQ_ISR 8
IRQ_ISR 9
IRQ_ISR 10
IRQ_ISR 11
IRQ_ISR 12
IRQ_ISR 13
IRQ_ISR 14
IRQ_ISR 15

; Function: init_idt
; Initializes the IDT with basic exception handlers and IRQ handlers
init_idt:
    push rbx
    push rcx
    
    ; Set up Division by Zero handler (Vector 0)
    mov rax, isr_divide_by_zero
    mov rbx, 0                             ; Vector number
    mov rcx, 0                             ; Segment selector
    call set_idt_entry

    ; Set up Debug Exception handler (Vector 1)
    mov rax, isr_debug
    mov rbx, 1                             ; Vector number
    mov rcx, 0                             ; Segment selector
    call set_idt_entry

    ; Set up Page Fault handler (Vector 14)
    mov rax, isr_page_fault
    mov rbx, 14                            ; Vector number
    mov rcx, 0                             ; Segment selector
    call set_idt_entry

    ; Set up IRQ handlers (Vectors 0x20-0x2F)
    mov rax, irq0
    mov rbx, 0x20                          ; IRQ 0 -> Vector 0x20
    mov rcx, 0
    call set_idt_entry
    
    mov rax, irq1
    mov rbx, 0x21                          ; IRQ 1 -> Vector 0x21
    mov rcx, 0
    call set_idt_entry
    
    mov rax, irq2
    mov rbx, 0x22                          ; IRQ 2 -> Vector 0x22
    mov rcx, 0
    call set_idt_entry
    
    mov rax, irq3
    mov rbx, 0x23                          ; IRQ 3 -> Vector 0x23
    mov rcx, 0
    call set_idt_entry
    
    mov rax, irq4
    mov rbx, 0x24                          ; IRQ 4 -> Vector 0x24
    mov rcx, 0
    call set_idt_entry
    
    mov rax, irq5
    mov rbx, 0x25                          ; IRQ 5 -> Vector 0x25
    mov rcx, 0
    call set_idt_entry
    
    mov rax, irq6
    mov rbx, 0x26                          ; IRQ 6 -> Vector 0x26
    mov rcx, 0
    call set_idt_entry
    
    mov rax, irq7
    mov rbx, 0x27                          ; IRQ 7 -> Vector 0x27
    mov rcx, 0
    call set_idt_entry
    
    mov rax, irq8
    mov rbx, 0x28                          ; IRQ 8 -> Vector 0x28
    mov rcx, 0
    call set_idt_entry
    
    mov rax, irq9
    mov rbx, 0x29                          ; IRQ 9 -> Vector 0x29
    mov rcx, 0
    call set_idt_entry
    
    mov rax, irq10
    mov rbx, 0x2A                          ; IRQ 10 -> Vector 0x2A
    mov rcx, 0
    call set_idt_entry
    
    mov rax, irq11
    mov rbx, 0x2B                          ; IRQ 11 -> Vector 0x2B
    mov rcx, 0
    call set_idt_entry
    
    mov rax, irq12
    mov rbx, 0x2C                          ; IRQ 12 -> Vector 0x2C
    mov rcx, 0
    call set_idt_entry
    
    mov rax, irq13
    mov rbx, 0x2D                          ; IRQ 13 -> Vector 0x2D
    mov rcx, 0
    call set_idt_entry
    
    mov rax, irq14
    mov rbx, 0x2E                          ; IRQ 14 -> Vector 0x2E
    mov rcx, 0
    call set_idt_entry
    
    mov rax, irq15
    mov rbx, 0x2F                          ; IRQ 15 -> Vector 0x2F
    mov rcx, 0
    call set_idt_entry

    call load_idt                          ; Load the IDT
    
    pop rcx
    pop rbx
    ret

section .text
global init_idt

; Note: init_idt is now called from C code (kernel_main)
; The old main function has been removed

section .data
idt_end: