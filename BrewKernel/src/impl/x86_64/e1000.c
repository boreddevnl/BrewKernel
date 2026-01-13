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

#include "e1000.h"
#include "pci.h"
#include "irq.h"
#include "pic.h"
#include "io.h"
#include "network.h"
#include <stdint.h>
#include <stddef.h>

// Simple memcpy implementation for freestanding environment
static void* memcpy(void* dest, const void* src, size_t n) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dest;
}

// Static device instance
static e1000_device_t e1000_dev;
static int e1000_initialized = 0;

// Static buffers for descriptors and packet data (physically contiguous)
static e1000_tx_desc_t tx_descriptors[E1000_TX_RING_SIZE] __attribute__((aligned(16)));
static e1000_rx_desc_t rx_descriptors[E1000_RX_RING_SIZE] __attribute__((aligned(16)));
static uint8_t tx_buffers[E1000_TX_RING_SIZE][2048] __attribute__((aligned(16)));
static uint8_t rx_buffers[E1000_RX_RING_SIZE][2048] __attribute__((aligned(16)));

// Read MAC address from EEPROM
static int e1000_read_eeprom(volatile uint32_t* mmio_base, uint16_t offset, uint16_t* data) {
    // Check if mmio_base is valid
    if (!mmio_base) {
        return -1;
    }
    
    uint32_t eerd = 0;
    eerd |= (offset << 8);
    eerd |= 1;  // Start read
    
    e1000_write_reg(mmio_base, E1000_REG_EERD, eerd);
    
    // Wait for read to complete (timeout after 1000 attempts)
    for (int i = 0; i < 1000; i++) {
        eerd = e1000_read_reg(mmio_base, E1000_REG_EERD);
        if (eerd & (1 << 4)) {  // Done bit
            *data = (uint16_t)(eerd >> 16);
            return 0;
        }
    }
    
    return -1;  // Timeout
}

// Get MAC address from EEPROM or registers
static int e1000_get_mac_address(volatile uint32_t* mmio_base, mac_address_t* mac) {
    // Check if mmio_base is valid
    if (!mmio_base || !mac) {
        return -1;
    }
    
    // Try reading from EEPROM first (but don't fail if it doesn't work)
    uint16_t data;
    int eeprom_ok = 0;
    if (e1000_read_eeprom(mmio_base, 0, &data) == 0) {
        mac->bytes[0] = (uint8_t)(data & 0xFF);
        mac->bytes[1] = (uint8_t)(data >> 8);
        if (e1000_read_eeprom(mmio_base, 1, &data) == 0) {
            mac->bytes[2] = (uint8_t)(data & 0xFF);
            mac->bytes[3] = (uint8_t)(data >> 8);
            if (e1000_read_eeprom(mmio_base, 2, &data) == 0) {
                mac->bytes[4] = (uint8_t)(data & 0xFF);
                mac->bytes[5] = (uint8_t)(data >> 8);
                eeprom_ok = 1;
            }
        }
    }
    
    // If EEPROM read failed, use RAL/RAH registers
    if (!eeprom_ok) {
        uint32_t ral = e1000_read_reg(mmio_base, E1000_REG_RAL);
        uint32_t rah = e1000_read_reg(mmio_base, E1000_REG_RAH);
        
        mac->bytes[0] = (uint8_t)(ral & 0xFF);
        mac->bytes[1] = (uint8_t)((ral >> 8) & 0xFF);
        mac->bytes[2] = (uint8_t)((ral >> 16) & 0xFF);
        mac->bytes[3] = (uint8_t)((ral >> 24) & 0xFF);
        mac->bytes[4] = (uint8_t)(rah & 0xFF);
        mac->bytes[5] = (uint8_t)((rah >> 8) & 0xFF);
    }
    
    return 0;
}

// Network card interrupt handler
static void e1000_irq_handler(void) {
    // Safety check - don't access device if not initialized
    if (!e1000_initialized || !e1000_dev.initialized || !e1000_dev.mmio_base) {
        return;
    }
    
    volatile uint32_t* mmio = e1000_dev.mmio_base;
    uint32_t icr = e1000_read_reg(mmio, E1000_REG_ICR);
    
    // Handle transmit interrupt
    if (icr & E1000_ICR_TXDW) {
        // Transmit descriptor written back - can free buffers
        // For now, we'll handle this in the transmit function
    }
    
    // Handle receive interrupt
    if (icr & E1000_ICR_RXT0) {
        // Receive timer interrupt - check for received packets
        // This will be handled in the receive function
    }
}

// Initialize e1000 device
int e1000_init(pci_device_t* pci_dev) {
    if (e1000_initialized) {
        return 0;  // Already initialized
    }
    
    // Get base address from PCI BAR0
    uint32_t bar0 = pci_read_config(pci_dev->bus, pci_dev->device, pci_dev->function, 0x10);
    
    // Check if BAR0 is valid (not all zeros or all ones)
    if (bar0 == 0 || bar0 == 0xFFFFFFFF) {
        return -1;  // Invalid BAR
    }
    
    // Check if it's memory-mapped (bit 0 = 0) or I/O mapped (bit 0 = 1)
    if (bar0 & 1) {
        // I/O mapped - not supported for now
        return -1;
    }
    
    // Get memory-mapped base address (clear lower bits)
    uint32_t mmio_base_phys = bar0 & ~0xF;
    
    // Validate the address is reasonable (should be in low 4GB for now)
    if (mmio_base_phys == 0 || mmio_base_phys >= 0x100000000ULL) {
        return -1;  // Invalid address
    }
    
    // Check if MMIO address is in mapped memory range
    // Page tables map:
    // - First 1GB: 0x00000000 - 0x40000000
    // - MMIO region: 0xFE800000 - 0xFF000000 (8MB)
    // If MMIO is outside these ranges, it won't be accessible
    if (mmio_base_phys >= 0x40000000 && (mmio_base_phys < 0xFE800000 || mmio_base_phys >= 0xFF000000)) {
        // MMIO region is outside mapped memory - would cause page fault
        return -1;  // MMIO not accessible - not mapped in page tables
    }
    
    // Enable bus mastering and memory space access BEFORE mapping
    uint32_t command = pci_read_config(pci_dev->bus, pci_dev->device, pci_dev->function, 0x04);
    command |= (1 << 2);  // Bus master enable
    command |= (1 << 1);  // Memory space enable
    pci_write_config(pci_dev->bus, pci_dev->device, pci_dev->function, 0x04, command);
    
    // Map physical address to virtual (for now, assume identity mapping)
    // Store the address but don't access it yet
    e1000_dev.mmio_base = (volatile uint32_t*)(uintptr_t)mmio_base_phys;
    e1000_dev.pci_dev = *pci_dev;
    e1000_dev.io_base = 0;
    e1000_dev.initialized = 0;  // Mark as not initialized yet
    
    // CRITICAL: The MMIO region might not be accessible if page tables don't cover it
    // For now, we'll skip initialization if the address is in a problematic range
    // QEMU typically maps e1000 at 0xFEB00000 or similar, which should be accessible
    // But if it crashes here, the page tables might not include this region
    
    // At this point, we've verified the MMIO address is in a mapped range
    // But we still need to be careful - the actual device might not be ready
    // The issue is that even if the address is in range, the page table entry
    // might not be correctly set up, or the device might not be accessible yet
    
    // Before accessing MMIO, let's double-check the address is reasonable
    // QEMU typically maps e1000 at 0xFEB00000, which should be in our mapped range
    // But if it's at a different address, we need to map it
    
    // Try to read STATUS register first (read-only, safer than CTRL)
    // This is a test to see if MMIO is accessible
    // If this causes a page fault, the system will crash (page fault handler just halts)
    // So we need to be absolutely sure the address is mapped
    
    // Use a memory barrier to ensure the write to mmio_base is visible
    __asm__ __volatile__("" ::: "memory");
    
    // CRITICAL: This read will cause a page fault if the MMIO isn't mapped
    // There's no safe way to test this without potentially crashing
    // If we get here and it crashes, the page table entry for this address is wrong
    volatile uint32_t status;
    status = e1000_read_reg(e1000_dev.mmio_base, E1000_REG_STATUS);
    (void)status;  // Suppress unused warning - we're just testing access
    
    // If we get here, the MMIO is accessible
    
    // Now try reset - but be more careful
    uint32_t ctrl = e1000_read_reg(e1000_dev.mmio_base, E1000_REG_CTRL);
    
    // Only reset if device is not already reset
    if (!(ctrl & E1000_CTRL_RST)) {
        e1000_write_reg(e1000_dev.mmio_base, E1000_REG_CTRL, ctrl | E1000_CTRL_RST);
        
        // Wait for reset to complete (with timeout)
        int reset_timeout = 0;
        for (int i = 0; i < 100000; i++) {
            ctrl = e1000_read_reg(e1000_dev.mmio_base, E1000_REG_CTRL);
            if (!(ctrl & E1000_CTRL_RST)) {
                reset_timeout = 1;
                break;
            }
        }
        
        if (!reset_timeout) {
            // Reset didn't complete in time
            return -1;
        }
    }
    
    // Get MAC address - try RAL/RAH first (more reliable than EEPROM)
    uint32_t ral = e1000_read_reg(e1000_dev.mmio_base, E1000_REG_RAL);
    uint32_t rah = e1000_read_reg(e1000_dev.mmio_base, E1000_REG_RAH);
    
    // Check if MAC address is valid (not all zeros or all ones)
    if ((ral == 0 && rah == 0) || (ral == 0xFFFFFFFF && (rah & 0xFFFF) == 0xFFFF)) {
        // Try EEPROM method as fallback
        if (e1000_get_mac_address(e1000_dev.mmio_base, &e1000_dev.mac_address) != 0) {
            return -1;
        }
    } else {
        // Use RAL/RAH values
        e1000_dev.mac_address.bytes[0] = (uint8_t)(ral & 0xFF);
        e1000_dev.mac_address.bytes[1] = (uint8_t)((ral >> 8) & 0xFF);
        e1000_dev.mac_address.bytes[2] = (uint8_t)((ral >> 16) & 0xFF);
        e1000_dev.mac_address.bytes[3] = (uint8_t)((ral >> 24) & 0xFF);
        e1000_dev.mac_address.bytes[4] = (uint8_t)(rah & 0xFF);
        e1000_dev.mac_address.bytes[5] = (uint8_t)((rah >> 8) & 0xFF);
    }
    
    // Initialize transmit descriptors
    e1000_dev.tx_descriptors = tx_descriptors;
    e1000_dev.tx_head = 0;
    e1000_dev.tx_tail = 0;
    
    for (int i = 0; i < E1000_TX_RING_SIZE; i++) {
        e1000_dev.tx_buffers[i] = tx_buffers[i];
        e1000_dev.tx_descriptors[i].buffer_addr = (uint64_t)(uintptr_t)tx_buffers[i];
        e1000_dev.tx_descriptors[i].length = 0;
        e1000_dev.tx_descriptors[i].cso = 0;
        e1000_dev.tx_descriptors[i].cmd = 0;
        e1000_dev.tx_descriptors[i].status = 0;
        e1000_dev.tx_descriptors[i].css = 0;
        e1000_dev.tx_descriptors[i].special = 0;
    }
    
    // Set up transmit descriptor ring
    uint64_t tx_desc_phys = (uint64_t)(uintptr_t)tx_descriptors;
    e1000_write_reg(e1000_dev.mmio_base, E1000_REG_TDBAL, (uint32_t)(tx_desc_phys & 0xFFFFFFFF));
    e1000_write_reg(e1000_dev.mmio_base, E1000_REG_TDBAH, (uint32_t)(tx_desc_phys >> 32));
    e1000_write_reg(e1000_dev.mmio_base, E1000_REG_TDLEN, E1000_TX_RING_SIZE * sizeof(e1000_tx_desc_t));
    e1000_write_reg(e1000_dev.mmio_base, E1000_REG_TDH, 0);
    e1000_write_reg(e1000_dev.mmio_base, E1000_REG_TDT, 0);
    
    // Configure transmit control
    uint32_t tctl = E1000_TCTL_EN | E1000_TCTL_PSP | (E1000_TCTL_CT & (0x10 << 4)) | (E1000_TCTL_COLD & (0x40 << 12));
    e1000_write_reg(e1000_dev.mmio_base, E1000_REG_TCTL, tctl);
    
    // Set inter-packet gap
    e1000_write_reg(e1000_dev.mmio_base, E1000_REG_TIPG, 0x0060200A);
    
    // Initialize receive descriptors
    e1000_dev.rx_descriptors = rx_descriptors;
    e1000_dev.rx_head = 0;
    e1000_dev.rx_tail = E1000_RX_RING_SIZE - 1;  // Start with tail at last position
    
    for (int i = 0; i < E1000_RX_RING_SIZE; i++) {
        e1000_dev.rx_buffers[i] = rx_buffers[i];
        e1000_dev.rx_descriptors[i].buffer_addr = (uint64_t)(uintptr_t)rx_buffers[i];
        e1000_dev.rx_descriptors[i].length = 0;
        e1000_dev.rx_descriptors[i].checksum = 0;
        e1000_dev.rx_descriptors[i].status = 0;
        e1000_dev.rx_descriptors[i].errors = 0;
        e1000_dev.rx_descriptors[i].special = 0;
    }
    
    // Set up receive descriptor ring
    uint64_t rx_desc_phys = (uint64_t)(uintptr_t)rx_descriptors;
    e1000_write_reg(e1000_dev.mmio_base, E1000_REG_RDBAL, (uint32_t)(rx_desc_phys & 0xFFFFFFFF));
    e1000_write_reg(e1000_dev.mmio_base, E1000_REG_RDBAH, (uint32_t)(rx_desc_phys >> 32));
    e1000_write_reg(e1000_dev.mmio_base, E1000_REG_RDLEN, E1000_RX_RING_SIZE * sizeof(e1000_rx_desc_t));
    e1000_write_reg(e1000_dev.mmio_base, E1000_REG_RDH, 0);
    e1000_write_reg(e1000_dev.mmio_base, E1000_REG_RDT, E1000_RX_RING_SIZE - 1);
    
    // Configure receive control
    uint32_t rctl = E1000_RCTL_EN | E1000_RCTL_SBP | E1000_RCTL_UPE | E1000_RCTL_MPE |
                    E1000_RCTL_LPE | E1000_RCTL_LBM_NONE | E1000_RCTL_RDMTS_HALF |
                    E1000_RCTL_MO_36 | E1000_RCTL_BAM | E1000_RCTL_BSIZE_2048 | E1000_RCTL_SECRC;
    e1000_write_reg(e1000_dev.mmio_base, E1000_REG_RCTL, rctl);
    
    // Enable link
    ctrl = e1000_read_reg(e1000_dev.mmio_base, E1000_REG_CTRL);
    e1000_write_reg(e1000_dev.mmio_base, E1000_REG_CTRL, ctrl | E1000_CTRL_SLU);
    
    // Enable interrupts (IRQ line from PCI config)
    // Note: We'll set up interrupts but don't enable them yet to avoid crashes
    // Interrupts can be enabled later once everything is stable
    uint8_t irq_line = (uint8_t)(pci_read_config(pci_dev->bus, pci_dev->device, pci_dev->function, 0x3C) & 0xFF);
    if (irq_line < 16 && irq_line > 0) {
        irq_register_handler(irq_line, e1000_irq_handler);
        // Don't enable PIC interrupt yet - enable it after testing
        // pic_irq_enable(irq_line);
        
        // Don't enable interrupt mask yet
        // e1000_write_reg(e1000_dev.mmio_base, E1000_REG_IMS, E1000_ICR_TXDW | E1000_ICR_RXT0);
    }
    
    e1000_dev.initialized = 1;
    e1000_initialized = 1;
    
    return 0;
}

// Get e1000 device instance
e1000_device_t* e1000_get_device(void) {
    if (!e1000_initialized) {
        return NULL;
    }
    return &e1000_dev;
}

// Send a packet
int e1000_send_packet(const void* data, size_t length) {
    if (!e1000_initialized || !e1000_dev.initialized) {
        return -1;
    }
    
    if (length > 2048) {
        return -1;  // Packet too large
    }
    
    volatile uint32_t* mmio = e1000_dev.mmio_base;
    
    // Check if there's space in the transmit ring
    uint16_t next_tail = (e1000_dev.tx_tail + 1) % E1000_TX_RING_SIZE;
    if (next_tail == e1000_dev.tx_head) {
        // Ring is full - wait for a descriptor to be freed
        // For now, just return error
        return -1;
    }
    
    // Copy packet data to buffer
    memcpy(e1000_dev.tx_buffers[e1000_dev.tx_tail], data, length);
    
    // Set up descriptor
    e1000_dev.tx_descriptors[e1000_dev.tx_tail].length = (uint16_t)length;
    e1000_dev.tx_descriptors[e1000_dev.tx_tail].cmd = E1000_TXD_CMD_EOP | E1000_TXD_CMD_IFCS | E1000_TXD_CMD_RS;
    e1000_dev.tx_descriptors[e1000_dev.tx_tail].status = 0;
    
    // Update tail pointer (this tells the hardware to start transmitting)
    e1000_dev.tx_tail = next_tail;
    e1000_write_reg(mmio, E1000_REG_TDT, e1000_dev.tx_tail);
    
    return 0;
}

// Receive a packet
int e1000_receive_packet(void* buffer, size_t buffer_size) {
    if (!e1000_initialized || !e1000_dev.initialized) {
        return 0;
    }
    
    volatile uint32_t* mmio = e1000_dev.mmio_base;
    
    // Read current hardware head position
    uint16_t hw_head = e1000_read_reg(mmio, E1000_REG_RDH);
    uint16_t tail = e1000_read_reg(mmio, E1000_REG_RDT);
    
    // Check if ring is empty (head == tail means no packets)
    // Note: In e1000, when head == tail, the ring is empty
    // But we need to check if head has wrapped around
    uint16_t next_idx = (tail + 1) % E1000_RX_RING_SIZE;
    
    // If head equals next_idx, ring is empty
    if (hw_head == next_idx) {
        return 0;  // No packets available
    }
    
    // Check if this descriptor has a packet (status bit DD set)
    if (!(e1000_dev.rx_descriptors[next_idx].status & E1000_RXD_STAT_DD)) {
        return 0;  // Packet not ready yet
    }
    
    // Get packet length (subtract CRC)
    uint16_t length = e1000_dev.rx_descriptors[next_idx].length - 4;
    if (length > buffer_size) {
        length = (uint16_t)buffer_size;
    }
    
    // Copy packet data
    memcpy(buffer, e1000_dev.rx_buffers[next_idx], length);
    
    // Clear descriptor status for next use
    e1000_dev.rx_descriptors[next_idx].status = 0;
    e1000_dev.rx_descriptors[next_idx].length = 0;
    
    // Update tail to give descriptor back to hardware
    tail = next_idx;
    e1000_write_reg(mmio, E1000_REG_RDT, tail);
    e1000_dev.rx_tail = tail;
    
    return (int)length;
}

