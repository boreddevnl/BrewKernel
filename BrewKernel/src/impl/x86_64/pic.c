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

#include "pic.h"

// Initialize and remap the PIC
void pic_init(void) {
    // Save masks
    unsigned char a1 = inb(PIC1_DATA);
    unsigned char a2 = inb(PIC2_DATA);
    
    // Start initialization sequence (cascade mode)
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    
    // Set interrupt vector offsets
    outb(PIC1_DATA, PIC1_OFFSET);  // Master PIC: interrupts 0x20-0x27
    outb(PIC2_DATA, PIC2_OFFSET);  // Slave PIC: interrupts 0x28-0x2F
    
    // Tell master PIC where slave is connected (IRQ2)
    outb(PIC1_DATA, 4);  // Slave at IRQ2
    outb(PIC2_DATA, 2);  // Cascade identity
    
    // Set 8086 mode
    outb(PIC1_DATA, ICW4_8086);
    outb(PIC2_DATA, ICW4_8086);
    
    // Restore masks (disable all interrupts initially)
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}

// Send End of Interrupt to PIC
void pic_send_eoi(unsigned char irq) {
    if (irq >= 8) {
        outb(PIC2_COMMAND, PIC_EOI);  // Send EOI to slave
    }
    outb(PIC1_COMMAND, PIC_EOI);      // Send EOI to master
}

// Enable specific IRQ line
void pic_irq_enable(unsigned char irq) {
    unsigned char port;
    unsigned char value;
    
    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }
    
    value = inb(port) & ~(1 << irq);  // Clear the bit to enable
    outb(port, value);
}

// Disable specific IRQ line
void pic_irq_disable(unsigned char irq) {
    unsigned char port;
    unsigned char value;
    
    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }
    
    value = inb(port) | (1 << irq);  // Set the bit to disable
    outb(port, value);
}

