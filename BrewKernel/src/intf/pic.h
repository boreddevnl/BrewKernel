/*
 * Brew Kernel
 * Copyright (C) 2024-2025 boreddevnl
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

#ifndef PIC_H
#define PIC_H

#include "io.h"

// PIC ports
#define PIC1_COMMAND    0x20
#define PIC1_DATA       0x21
#define PIC2_COMMAND    0xA0
#define PIC2_DATA       0xA1

// PIC initialization command words
#define ICW1_ICW4       0x01    // ICW4 needed
#define ICW1_SINGLE     0x02    // Single cascade mode
#define ICW1_INTERVAL4  0x04    // Call address interval 4
#define ICW1_LEVEL      0x08    // Level triggered mode
#define ICW1_INIT       0x10    // Initialization

#define ICW4_8086       0x01    // 8086/88 mode
#define ICW4_AUTO       0x02    // Auto EOI
#define ICW4_BUF_SLAVE  0x08    // Buffered mode/slave
#define ICW4_BUF_MASTER 0x0C    // Buffered mode/master
#define ICW4_SFNM       0x10    // Special fully nested mode

// End of interrupt command
#define PIC_EOI         0x20

// Remap PIC interrupts to avoid conflicts with CPU exceptions (0x00-0x1F)
// We'll map PIC1 to 0x20-0x27 and PIC2 to 0x28-0x2F
#define PIC1_OFFSET     0x20
#define PIC2_OFFSET     0x28

// Initialize and remap the PIC
void pic_init(void);

// Send End of Interrupt to PIC
void pic_send_eoi(unsigned char irq);

// Enable/disable specific IRQ lines
void pic_irq_enable(unsigned char irq);
void pic_irq_disable(unsigned char irq);

#endif // PIC_H

