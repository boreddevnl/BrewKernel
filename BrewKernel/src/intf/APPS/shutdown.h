#ifndef SHUTDOWN_H
#define SHUTDOWN_H

#include "../print.h"
#include "../io.h"

// Forward declaration of brewing function from main.c
void brewing(int iterations);

void shutdown_command() {
    brew_str("\nInitiating system shutdown...\n");
    brewing(10000000);  // Small delay for message to be visible

    // Try ACPI shutdown first
    // Write "S5" to prepare for shutdown
    outw(0xB004, 0x2000);
    
    // Fallback method using keyboard controller
    // Send shutdown command to the keyboard controller
    outb(0x64, 0xFE);
    
    // If we get here, shutdown failed
    brew_str("WARNING: System shutdown failed.\n");
    brew_str("It is now safe to turn off your computer.\n");
}

#endif