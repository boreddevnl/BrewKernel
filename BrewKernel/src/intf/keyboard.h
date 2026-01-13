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
#ifndef KEYBOARD_H
#define KEYBOARD_H

// Keyboard scan codes for modifier keys
#define SCAN_CODE_LEFT_SHIFT  0x2A
#define SCAN_CODE_RIGHT_SHIFT 0x36
#define SCAN_CODE_LEFT_SHIFT_RELEASE  0xAA
#define SCAN_CODE_RIGHT_SHIFT_RELEASE 0xB6

// Special key scan codes
#define SCAN_CODE_UP_ARROW    0x48
#define SCAN_CODE_DOWN_ARROW  0x50

// Check if a key is available to read
// Returns 1 if a key is available, 0 otherwise
extern int check_keyboard(void);

// Read a scan code from the keyboard
// Returns the scan code of the pressed key
extern unsigned char read_scan_code(void);

// Convert a scan code to an ASCII character considering shift state
// Returns 0 if the scan code doesn't map to a printable character
extern char scan_code_to_ascii(unsigned char scan_code);

// Get the current state of the shift key
// Returns 1 if shift is pressed, 0 otherwise
extern int is_shift_pressed(void);

// Check if a scan code is for a special key (arrows, etc)
// Returns 1 if it's a special key, 0 otherwise
static inline int is_special_key(unsigned char scan_code) {
    return scan_code == SCAN_CODE_UP_ARROW || scan_code == SCAN_CODE_DOWN_ARROW;
}

void brewing(int iterations);

#endif // KEYBOARD_H