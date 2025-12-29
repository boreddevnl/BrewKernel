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

#include "pci.h"
#include "io.h"
#include <stdint.h>

uint32_t pci_read_config(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    // Create configuration address
    uint32_t address = (uint32_t)((1 << 31) |           // Enable bit
                                  (bus << 16) |          // Bus number
                                  (device << 11) |       // Device number
                                  (function << 8) |      // Function number
                                  (offset & 0xFC));      // Register offset (aligned to 4 bytes)
    
    // Write address to configuration address port (32-bit write)
    outl(PCI_CONFIG_ADDRESS, address);
    
    // Read from configuration data port (32-bit read)
    return inl(PCI_CONFIG_DATA);
}

// Write to PCI configuration space
void pci_write_config(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value) {
    // Create configuration address
    uint32_t address = (uint32_t)((1 << 31) |           // Enable bit
                                  (bus << 16) |          // Bus number
                                  (device << 11) |       // Device number
                                  (function << 8) |      // Function number
                                  (offset & 0xFC));      // Register offset (aligned to 4 bytes)
    
    // Write address to configuration address port (32-bit write)
    outl(PCI_CONFIG_ADDRESS, address);
    
    // Write to configuration data port (32-bit write)
    outl(PCI_CONFIG_DATA, value);
}

// Check if a PCI device exists
int pci_device_exists(uint8_t bus, uint8_t device, uint8_t function) {
    uint16_t vendor_id = pci_get_vendor_id(bus, device, function);
    return vendor_id != 0xFFFF;  // 0xFFFF means no device
}

// Get vendor ID
uint16_t pci_get_vendor_id(uint8_t bus, uint8_t device, uint8_t function) {
    uint32_t config = pci_read_config(bus, device, function, 0x00);
    return (uint16_t)(config & 0xFFFF);
}

// Get device ID
uint16_t pci_get_device_id(uint8_t bus, uint8_t device, uint8_t function) {
    uint32_t config = pci_read_config(bus, device, function, 0x00);
    return (uint16_t)((config >> 16) & 0xFFFF);
}

// Get class code
uint8_t pci_get_class_code(uint8_t bus, uint8_t device, uint8_t function) {
    uint32_t config = pci_read_config(bus, device, function, 0x08);
    return (uint8_t)((config >> 24) & 0xFF);
}

// Get subclass
uint8_t pci_get_subclass(uint8_t bus, uint8_t device, uint8_t function) {
    uint32_t config = pci_read_config(bus, device, function, 0x08);
    return (uint8_t)((config >> 16) & 0xFF);
}

// Get programming interface
uint8_t pci_get_prog_if(uint8_t bus, uint8_t device, uint8_t function) {
    uint32_t config = pci_read_config(bus, device, function, 0x08);
    return (uint8_t)((config >> 8) & 0xFF);
}

// Enumerate all PCI devices
int pci_enumerate_devices(pci_device_t* devices, int max_devices) {
    int count = 0;
    uint8_t bus;
    uint8_t device;
    uint8_t function;
    uint8_t num_functions;
    uint8_t header_type;
    uint32_t config_val;
    
    // Scan all buses, devices, and functions
    for (bus = 0; bus < 256 && count < max_devices; bus++) {
        for (device = 0; device < 32 && count < max_devices; device++) {
            // Check function 0 first
            if (pci_device_exists(bus, device, 0)) {
                config_val = pci_read_config(bus, device, 0, 0x0C);
                header_type = (uint8_t)((config_val >> 16) & 0xFF);
                num_functions = (header_type & 0x80) ? 8 : 1;
                
                for (function = 0; function < num_functions && count < max_devices; function++) {
                    if (pci_device_exists(bus, device, function)) {
                        devices[count].bus = bus;
                        devices[count].device = device;
                        devices[count].function = function;
                        devices[count].vendor_id = pci_get_vendor_id(bus, device, function);
                        devices[count].device_id = pci_get_device_id(bus, device, function);
                        devices[count].class_code = pci_get_class_code(bus, device, function);
                        devices[count].subclass = pci_get_subclass(bus, device, function);
                        devices[count].prog_if = pci_get_prog_if(bus, device, function);
                        count++;
                    }
                }
            }
        }
    }
    
    return count;
}

// Find PCI device by vendor and device ID
int pci_find_device(uint16_t vendor_id, uint16_t device_id, pci_device_t* device) {
    pci_device_t devices[32];
    int count = pci_enumerate_devices(devices, 32);
    
    for (int i = 0; i < count; i++) {
        if (devices[i].vendor_id == vendor_id && devices[i].device_id == device_id) {
            *device = devices[i];
            return 1;
        }
    }
    
    return 0;
}

// Find PCI device by class code
int pci_find_device_by_class(uint8_t class_code, uint8_t subclass, pci_device_t* device) {
    pci_device_t devices[32];
    int count = pci_enumerate_devices(devices, 32);
    
    for (int i = 0; i < count; i++) {
        if (devices[i].class_code == class_code && devices[i].subclass == subclass) {
            *device = devices[i];
            return 1;
        }
    }
    
    return 0;
}

