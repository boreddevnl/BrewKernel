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

#include "timer.h"
#include "pic.h"
#include "io.h"
#include "network.h"
#include <stdint.h>

// Global tick counter
static volatile uint64_t timer_ticks = 0;
static volatile uint32_t network_tick_counter = 0;

// Initialize the PIT timer
void timer_init(uint32_t frequency) {
    // Calculate divisor
    uint32_t divisor = PIT_FREQUENCY / frequency;
    
    // Send command byte: channel 0, access mode lo/hi, mode 3 (square wave)
    outb(PIT_COMMAND, PIT_CHANNEL0_SELECT | PIT_ACCESS_LO_HI | PIT_MODE_SQUARE_WAVE);
    
    // Send divisor (low byte, then high byte)
    outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFF));
    outb(PIT_CHANNEL0, (uint8_t)((divisor >> 8) & 0xFF));
}

// Timer interrupt handler (called from IRQ handler)
// This is called from the IRQ dispatcher
void timer_handler(void) {
    timer_ticks++;
    
    // Process network frames every 10 ticks (100ms at 100Hz)
    network_tick_counter++;
    if (network_tick_counter >= 10) {
        network_tick_counter = 0;
        if (network_is_initialized()) {
            network_process_frames();
        }
    }
}

// Get current tick count
uint64_t timer_get_ticks(void) {
    return timer_ticks;
}

// Sleep for specified milliseconds (approximate)
void timer_sleep_ms(uint32_t milliseconds) {
    uint64_t start_ticks = timer_get_ticks();
    uint64_t target_ticks = start_ticks + (milliseconds / 10);  // Assuming 100Hz = 10ms per tick
    
    while (timer_get_ticks() < target_ticks) {
        // Busy wait - in a real OS you'd yield to other tasks
        __asm__ __volatile__("pause");
    }
}

