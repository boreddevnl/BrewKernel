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

#ifndef IRQ_H
#define IRQ_H

// IRQ numbers (mapped to interrupt vectors 0x20-0x2F)
#define IRQ0_TIMER     0
#define IRQ1_KEYBOARD  1
#define IRQ2_CASCADE   2
#define IRQ3_COM2      3
#define IRQ4_COM1      4
#define IRQ5_LPT2      5
#define IRQ6_FLOPPY    6
#define IRQ7_LPT1      7
#define IRQ8_RTC       8
#define IRQ9_LEGACY    9
#define IRQ10_RESERVED 10
#define IRQ11_RESERVED 11
#define IRQ12_PS2      12
#define IRQ13_FPU      13
#define IRQ14_ATA      14
#define IRQ15_ATA      15

// IRQ handler function type
typedef void (*irq_handler_t)(void);

// Register an IRQ handler
void irq_register_handler(unsigned char irq, irq_handler_t handler);

// Unregister an IRQ handler
void irq_unregister_handler(unsigned char irq);

// Initialize IRQ handling
void irq_init(void);

#endif // IRQ_H

