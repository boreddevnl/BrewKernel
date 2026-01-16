# Brew Kernel V3.3

## Brewkernel is now officially 2 years old!
I've been working on this project since The 14th of January 2024 thinking it would probably a small project, touch it once and never again..
Well that for sure hasn't happened. I've grown this out to an actually daily usable kernel (if you lived in the 80's) haha. 
Enjoy the brew kernel, and i for sure hope to keep my interest on this massive project! 

<img src="asciiart.png" width="200" /> </br>
Brew Kernel is a simple x86_64 hobbyist kernel. 
It features a custom bootloader, VGA text mode output with customizable colors, a basic IDT and a basic ramdisk-like filesystem.

<img src="img/brewkernel.png" width="500" alt="QEMU Screenshot" /><br />
<sub><i>Note: This screenshot may be outdated.</i></sub>
## Features
- Basic Networking 
- Basic ramdisk filesystem
- 64-bit long mode support
- Multiboot2 compliant
- Custom VGA text mode driver with 16-color palette
- IDT
- Ability to run on actual x86_64 hardware
- CLI

## Prerequisites

To build the kernel, you'll need Docker installed on your system. The build environment is containerized to ensure consistency across different systems.

## Building
### Don't want to compile it yourself? grab the [latest patch or full release](https://github.com/BoredDevNL/BrewKernel/releases)
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
