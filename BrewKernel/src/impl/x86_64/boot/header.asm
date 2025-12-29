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
; Multiboot2 Header
; This file defines the multiboot2 header structure required by GRUB2 bootloader
;

section .multiboot_header
header_start:
    ; Magic number (required by multiboot2 specification)
    ; Value 0xe85250d6 identifies this as a multiboot2 header
    dd 0xe85250d6

    ; Architecture field
    ; Value 0 indicates protected mode i386
    dd 0

    ; Header length
    ; Calculated as the difference between header_end and header_start
    dd header_end - header_start

    ; Checksum field
    ; Formula: -(magic + architecture + header_length)
    dd 0x100000000 - (0xe85250d6 + 0 + (header_end - header_start))

    ; End tag
    dw 0    ; type
    dw 0    ; flags
    dd 8    ; size
header_end:
