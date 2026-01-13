/*
 * Brew Kernel
 * Copyright (C) 2024-2026 boreddevnl
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

/* Specify the entry point symbol that the bootloader will jump to */
ENTRY(start)

SECTIONS
{
    /* Start the kernel at 1MB physical address */
    . = 1M;

    /* 
     * Boot section containing multiboot header
     */
    .boot :
    {
        KEEP(*(.multiboot_header))
    }

    /* 
     * Text section containing executable code
     */
    .text :
    {
        *(.text)
    }
}
