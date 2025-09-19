#ifndef KEYBOARD_H
#define KEYBOARD_H

// Keyboard scan codes for modifier keys
#define SCAN_CODE_LEFT_SHIFT  0x2A
#define SCAN_CODE_RIGHT_SHIFT 0x36
#define SCAN_CODE_LEFT_SHIFT_RELEASE  0xAA
#define SCAN_CODE_RIGHT_SHIFT_RELEASE 0xB6

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

void brewing(int iterations);

#endif // KEYBOARD_H