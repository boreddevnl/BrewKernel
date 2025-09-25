#ifndef REBOOT_H
#define REBOOT_H

#include "../print.h"
#include "../io.h"

// Forward declaration of brewing function from main.c
void brewing(int iterations);

void reboot_command() {
    brew_str("\nInitiating system reboot...\n");
    brewing(10000000);  // Small delay for message to be visible

    // Method 1: Use keyboard controller to trigger a reset
    // Wait for the keyboard controller input buffer to be empty
    while ((inb(0x64) & 2) != 0) {
        brewing(1000);
    }
    
    // Send the reset command to the keyboard controller
    outb(0x64, 0xFE);
    
    // If keyboard controller method fails, try CPU reset
    brewing(50000000);  // Wait a bit to see if first method worked
    
    // Method 2: Triple fault (force CPU reset)
    asm volatile ("lidt %0" : : "m"(*(char*)0));  // Load an invalid IDT
    asm volatile ("int $0x3");  // Trigger an interrupt
    
    // If we get here, both methods failed
    brew_str("WARNING: System reboot failed.\n");
    brew_str("Please reset your computer manually.\n");
}

#endif