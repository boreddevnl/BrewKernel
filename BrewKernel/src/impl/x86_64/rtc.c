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
#include "rtc.h"

// CMOS I/O ports
#define CMOS_ADDRESS 0x70
#define CMOS_DATA    0x71

// Function to get a specific register from the CMOS
static unsigned char get_rtc_register(int reg) {
    unsigned char result;
    __asm__ __volatile__ ("outb %b0, %1" : : "a"(reg), "N"(CMOS_ADDRESS));
    __asm__ __volatile__ ("inb %1, %0" : "=a" (result) : "N"(CMOS_DATA));
    return result;
}

// Function to convert BCD to binary
static int bcd_to_bin(unsigned char bcd) {
    return ((bcd / 16) * 10) + (bcd & 0x0F);
}

void get_datetime(int* year, int* month, int* day, int* hour, int* minute, int* second) {
    // Wait until RTC is not updating
    while (get_rtc_register(0x0A) & 0x80);

    *second = bcd_to_bin(get_rtc_register(0x00));
    *minute = bcd_to_bin(get_rtc_register(0x02));
    *hour = bcd_to_bin(get_rtc_register(0x04));
    *day = bcd_to_bin(get_rtc_register(0x07));
    *month = bcd_to_bin(get_rtc_register(0x08));
    *year = bcd_to_bin(get_rtc_register(0x09));

    // Add century prefix (assuming 21st century for now)
    *year += 2000;
}
