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

;
; 64-bit Long Mode Entry Point
; This file contains the code that runs after successfully transitioning to 64-bit mode
; It sets up segment registers and calls the C kernel main function
;

global long_mode_start
extern kernel_main

section .text
bits 64

long_mode_start:
    ; Initialize all segment registers to 0
    mov ax, 0
    mov ss, ax      ; Stack Segment
    mov ds, ax      ; Data Segment
    mov es, ax      ; Extra Segment
    mov fs, ax      ; Extra Segment 2
    mov gs, ax      ; Extra Segment 3

    ; Multiboot info pointer is in EBX (32-bit register)
    ; Move it to RDI for the first argument of kernel_main (64-bit register)
    mov edi, ebx

    ; Call the C kernel main function
    call kernel_main
    hlt            ; Halt the CPU if kernel_main returns
