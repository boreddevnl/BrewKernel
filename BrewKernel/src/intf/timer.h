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

#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

// PIT (Programmable Interval Timer) ports
#define PIT_CHANNEL0    0x40
#define PIT_CHANNEL1    0x41
#define PIT_CHANNEL2    0x42
#define PIT_COMMAND     0x43

// PIT command byte
#define PIT_CHANNEL0_SELECT    0x00
#define PIT_ACCESS_LO_HI       0x30  // Access mode: low byte, then high byte
#define PIT_MODE_RATE_GEN      0x04  // Mode 2: Rate generator
#define PIT_MODE_SQUARE_WAVE   0x06  // Mode 3: Square wave generator

// PIT frequency (Hz)
#define PIT_FREQUENCY   1193182

// Default timer frequency (Hz) - 100Hz = 10ms per tick
#define TIMER_FREQUENCY 100

// Initialize the PIT timer
void timer_init(uint32_t frequency);

// Get current tick count
uint64_t timer_get_ticks(void);

// Sleep for specified milliseconds (approximate)
void timer_sleep_ms(uint32_t milliseconds);

// Timer interrupt handler (called from IRQ dispatcher)
void timer_handler(void);

#endif // TIMER_H

