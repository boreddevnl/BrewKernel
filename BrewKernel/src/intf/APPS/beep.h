#ifndef BEEP_H
#define BEEP_H

#include "../print.h"
#include "../io.h"

// Forward declaration of brewing function from main.c
void brewing(int iterations);

void beep_command() {
    brew_str("\n");
    brew_str("BEEP!");
    // Add a 500ms delay before the beep
    brewing(5000000);

    // Set the PIT to the desired frequency (1000 Hz for high pitch)
    outb(0x43, 0xB6);  // Command byte: channel 2, mode 3, binary
    int frequency = 1000;
    int divisor = 1193180 / frequency;
    outb(0x42, divisor & 0xFF);
    outb(0x42, (divisor >> 8) & 0xFF);

    // Turn speaker on
    outb(0x61, inb(0x61) | 0x03);

    // Keep the sound for 1 second
    brewing(10000000);

    // Turn speaker off
    outb(0x61, inb(0x61) & 0xFC);

    brewing(1000000);

    outb(0x61, inb(0x61) | 0x03);

    // Keep the sound for 1 second
    brewing(50000000);

    // Turn speaker off
    outb(0x61, inb(0x61) & 0xFC);    

    // Turn speaker on
    outb(0x61, inb(0x61) | 0x03);

    // Keep the sound for 1 second
    brewing(10000000);

    // Turn speaker off
    outb(0x61, inb(0x61) & 0xFC);

    brewing(1000000);

    outb(0x61, inb(0x61) | 0x03);

    // Keep the sound for 1 second
    brewing(50000000);

    // Turn speaker off
    outb(0x61, inb(0x61) & 0xFC);    


}

#endif