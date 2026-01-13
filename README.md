# Brew Kernel V3.0

<img src="asciiart.png" width="200" /> </br>
Brew Kernel is a simple x86_64 hobbyist kernel. 
It features a custom bootloader, VGA text mode output with customizable colors, a basic IDT and a basic ramdisk-like filesystem.

<img src="img/brewkernel.png" width="500" alt="QEMU Screenshot" /><br />
<sub><i>Note: This screenshot may be outdated.</i></sub>
## Features
- Basic Networking 
- Basic ramdisk-like filesystem
- 64-bit long mode support
- Multiboot2 compliant
- Custom VGA text mode driver with 16-color palette support
- Basic Interrupt Descriptor Table (IDT) implementation
- ASCII character display demo
- Integer output support with signed and unsigned number handling
- Ability to run on actual x86_64 hardware
- Interactive Command Line Interface (CLI) with the following commands:
  - HELP - Display list of available commands
  - DATE - Display current date and time with timezone support
  - MATH - Basic arithmetic calculator
  - ABOUT - Display system information
  - MAN - Show detailed user manual
  - CLEAR - Clear the screen
  - LICENSE - Display GNU GPLv3 license
  - UPTIME - Show system uptime
  - EXIT - Return to regular typing mode
  - TXTEDIT - text editor, files saved to RAM
  - BEEP - beeps using speaker output.

## Prerequisites

To build the kernel, you'll need Docker installed on your system. The build environment is containerized to ensure consistency across different systems.

## Building
### Don't want to compile it yourself? grab the [latest "stable" version](https://github.com/BoredDevNL/BrewKernel/tree/main/BrewKernel/dist/x86_64)
1. First, build the Docker container for the build environment:

```sh
cd BrewKernel
docker build buildenv -t brewkernel    
```

2. Run the build script:

```sh
docker run --rm -it -v "$(pwd)":/root/env -w /root/env --platform linux/amd64 brewkernel make build-x86_64    
```

The build process will create:
- A kernel binary at `dist/x86_64/kernel.bin`
- A bootable ISO image at `dist/x86_64/kernel.iso`

## Running

You can run the kernel using QEMU:

```sh
qemu-system-x86_64 -cdrom dist/x86_64/kernel.iso
```

Or run Brew on actual hardware. </br>

*note: This is not recommended, i am not liable if this software breaks your system, this is **completely at YOUR OWN RISK.** This software comes with **ZERO** warranty.*

- Step 1:
    Install BalenaEtcher on your device </br>
    *From own testing this should work, if you would like to use DD, then do so. At again, your own risk.*

- Step 2:
    Open BalenaEtcher and choose the kernel.iso, an external USB drive and flash it, this should take a second, if not less.
- Step 3:
    Find a compatible x86_64 system.
    ```
    Tested Hardware:
    HP EliteDesk 705 G4 DM 65W SBKPF 
     CPU:   AMD Ryzen 5 PRO 2400G (8) @ 3.600GHz
     GPU:   AMD ATI Radeon Vega Series (VGA ONLY.)
    ```
- Step 4:
    Enable legacy boot in your BIOS
- Step 5:
    Insert the flashed USB Drive and boot using the legacy boot option **NOT UEFI**. </br>
- Step 6:
    Enjoy the Brew Kernel!


## Project Structure

- `src/impl/kernel/` - Main kernel implementation
- `src/impl/x86_64/` - Architecture-specific code
- `src/intf/` - Header files and interfaces
- `targets/` - Target-specific files (linker scripts, GRUB config)
- `buildenv/` - Docker build environment
- `dist/` - Build output directory

## Technical Details

### Memory Map

- Kernel is loaded at 1MB (0x100000)
- Stack is 16KB
- Page tables are set up for identity mapping

### VGA Text Mode

- Resolution: 80x25 characters
- 16-color palette support with RGB customization
- Memory mapped at 0xB8000
- Hardware cursor with:
  - Configurable cursor shape (start and end lines)
  - Enable/disable functionality
  - Position tracking and automatic updating
- Automatic screen scrolling when bottom is reached
- Support for special characters (newline, backspace)
- Text wrapping at screen boundaries

### Text Input & Display

- 256-character input buffer for commands
- Backspace handling with proper cursor movement
- Support for case-insensitive commands
- Automatic line wrapping for long text
- Color attributes for foreground and background
- Scroll support for long documents (license, manual)

### Integer Output Support

- Signed integer printing with negative number handling
- Unsigned integer printing for positive numbers only
- Base-10 (decimal) output format
- Automatic buffer management for any integer size
- Numbers automatically wrapped at screen boundaries
- Zero-padding support for formatted output
- Proper handling of integer overflow

### Interrupts

Basic IDT setup with handlers for:
- Division by Zero (Vector 0)
- Debug Exception (Vector 1)
- Page Fault (Vector 14)

### Keyboard Input

- PS/2 keyboard driver with full scan code support
- Shift key state tracking for uppercase and special characters
- Backspace handling with proper cursor movement
- ASCII character mapping with support for printable characters
- Key repeat rate control for smooth typing experience
- Hardware cursor support with enable/disable functionality
- Up/Down arrows for command history

### Real-Time Clock & Timezone Support

- RTC driver for reading current date and time
- BCD to binary conversion for accurate timekeeping
- Comprehensive timezone database covering major regions:
  - North America (UTC-10 to UTC-5)
  - South America (UTC-5 to UTC-3)
  - Europe (UTC+1 to UTC+3)
  - Asia (UTC+5:30 to UTC+9)
  - Oceania (UTC+10 to UTC+12)
  - Africa (UTC+0 to UTC+3)
- Interactive timezone selection by continent and region
- Support for half-hour timezone offsets (e.g., UTC+5:30)
- System uptime tracking

### Built-in Applications

- Calculator (MATH command)
  - Basic arithmetic operations (+, -, *, /)
  - Integer math with proper handling of negative numbers
  - Interactive user input with validation

- System Information (ABOUT command)
  - Displays kernel version and build date
  - Shows architecture details (x86_64)
  - Reports compiler type and version
  - Includes ASCII art logo with color support

- Manual Viewer (MAN command)
  - Comprehensive documentation with scrolling support
  - Navigation using arrow keys
  - Organized sections for features and commands

- License Viewer (LICENSE command)
  - Full GNU GPLv3 license text
  - Paginated display with scrolling
  - Easy navigation with arrow keys

- Date/Time Tool (DATE command)
  - Current date and time display
  - Interactive timezone selection
  - Support for all major global regions

- System Monitor (UPTIME command)
  - Tracks system runtime since boot
  - Displays hours, minutes, and seconds
  - Handles day wraparound

### any of the above could be outdated, refer to the MANUAL (MAN command in CLI) or the [online user manual](https://brew.boreddev.nl/manual.html) for updated information.



###
###

<h2 align="left">Help me brew some coffee! ☕️</h2>

###

<p align="left">
  If you enjoy this project, and like what i'm doing here, consider buying me a coffee!
  <br><br>
  <a href="https://buymeacoffee.com/boreddevnl" target="_blank">
    <img src="https://cdn.buymeacoffee.com/buttons/v2/default-yellow.png" alt="Buy Me A Coffee" height="50" style="border-radius: 8px;" />
  </a>
</p>

###


## License

Copyright (C) 2024-2026 boreddevnl

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

NOTICE
------

This product includes software developed by Chris ("boreddevnl") as part of the BrewKernel project.

Copyright (C) 2024–2026 Chris / boreddevnl (previously boreddevhq)

All source files in this repository contain copyright and license
headers that must be preserved in redistributions and derivative works.

If you distribute or modify this project (in whole or in part),
you MUST:

  - Retain all copyright and license headers at the top of each file.
  - Include this NOTICE file along with any redistributions or
    derivative works.
  - Provide clear attribution to the original author in documentation
    or credits where appropriate.

The above attribution requirements are informational and intended to
ensure proper credit is given. They do not alter or supersede the
terms of the GNU General Public License (GPL), which governs this work.
