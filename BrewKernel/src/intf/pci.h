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

#ifndef PCI_H
#define PCI_H

#include <stdint.h>

// PCI configuration space ports
#define PCI_CONFIG_ADDRESS  0xCF8
#define PCI_CONFIG_DATA     0xCFC

// PCI device structure
typedef struct {
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t bus;
    uint8_t device;
    uint8_t function;
    uint8_t class_code;
    uint8_t subclass;
    uint8_t prog_if;
} pci_device_t;

// PCI class codes
#define PCI_CLASS_NETWORK_CONTROLLER  0x02
#define PCI_CLASS_ETHERNET_CONTROLLER 0x00

// Function to read from PCI configuration space
uint32_t pci_read_config(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);

// Function to write to PCI configuration space
void pci_write_config(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value);

// Check if a PCI device exists
int pci_device_exists(uint8_t bus, uint8_t device, uint8_t function);

// Get vendor ID
uint16_t pci_get_vendor_id(uint8_t bus, uint8_t device, uint8_t function);

// Get device ID
uint16_t pci_get_device_id(uint8_t bus, uint8_t device, uint8_t function);

// Get class code
uint8_t pci_get_class_code(uint8_t bus, uint8_t device, uint8_t function);

// Get subclass
uint8_t pci_get_subclass(uint8_t bus, uint8_t device, uint8_t function);

// Get programming interface
uint8_t pci_get_prog_if(uint8_t bus, uint8_t device, uint8_t function);

// Enumerate all PCI devices
// Returns number of devices found
int pci_enumerate_devices(pci_device_t* devices, int max_devices);

// Find PCI device by vendor and device ID
int pci_find_device(uint16_t vendor_id, uint16_t device_id, pci_device_t* device);

// Find PCI device by class code
int pci_find_device_by_class(uint8_t class_code, uint8_t subclass, pci_device_t* device);

#endif // PCI_H

