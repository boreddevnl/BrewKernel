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

#include "irq.h"
#include "pic.h"
#include "timer.h"
#include <stddef.h>

// Array of IRQ handlers (16 IRQs)
static irq_handler_t irq_handlers[16] = {NULL};

// Initialize IRQ handling
void irq_init(void) {
    // Clear all handlers
    for (int i = 0; i < 16; i++) {
        irq_handlers[i] = NULL;
    }
    
    // Register default timer handler
    irq_register_handler(IRQ0_TIMER, timer_handler);
}

// Register an IRQ handler
void irq_register_handler(unsigned char irq, irq_handler_t handler) {
    if (irq < 16) {
        irq_handlers[irq] = handler;
    }
}

// Unregister an IRQ handler
void irq_unregister_handler(unsigned char irq) {
    if (irq < 16) {
        irq_handlers[irq] = NULL;
    }
}

// IRQ dispatcher (called from assembly ISRs)
// This must be visible to assembly code
void irq_dispatcher(unsigned char irq) {
    // Call the registered handler if it exists
    if (irq < 16 && irq_handlers[irq] != NULL) {
        irq_handlers[irq]();
    }
    
    // Send EOI to PIC
    pic_send_eoi(irq);
}

