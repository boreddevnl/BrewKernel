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

#ifndef E1000_H
#define E1000_H

#include <stdint.h>
#include <stddef.h>
#include "pci.h"
#include "network.h"

// Intel 82540EM device IDs
#define E1000_VENDOR_ID 0x8086
#define E1000_DEVICE_ID_82540EM 0x100E

// E1000 register offsets
#define E1000_REG_CTRL     0x0000  // Device Control
#define E1000_REG_STATUS   0x0008  // Device Status
#define E1000_REG_EECD     0x0010  // EEPROM/Flash Control/Data
#define E1000_REG_EERD     0x0014  // EEPROM Read
#define E1000_REG_CTRL_EXT 0x0018  // Extended Device Control
#define E1000_REG_ICR      0x00C0  // Interrupt Cause Read
#define E1000_REG_ICS      0x00C8  // Interrupt Cause Set
#define E1000_REG_IMS      0x00D0  // Interrupt Mask Set/Read
#define E1000_REG_IMC      0x00D8  // Interrupt Mask Clear
#define E1000_REG_RCTL     0x0100  // Receive Control
#define E1000_REG_TCTL     0x0400  // Transmit Control
#define E1000_REG_TIPG     0x0410  // Transmit Inter Packet Gap
#define E1000_REG_RDBAL    0x2800  // Receive Descriptor Base Address Low
#define E1000_REG_RDBAH    0x2804  // Receive Descriptor Base Address High
#define E1000_REG_RDLEN    0x2808  // Receive Descriptor Length
#define E1000_REG_RDH      0x2810  // Receive Descriptor Head
#define E1000_REG_RDT      0x2818  // Receive Descriptor Tail
#define E1000_REG_TDBAL    0x3800  // Transmit Descriptor Base Address Low
#define E1000_REG_TDBAH    0x3804  // Transmit Descriptor Base Address High
#define E1000_REG_TDLEN    0x3808  // Transmit Descriptor Length
#define E1000_REG_TDH      0x3810  // Transmit Descriptor Head
#define E1000_REG_TDT      0x3818  // Transmit Descriptor Tail
#define E1000_REG_RAL      0x5400  // Receive Address Low (MAC address)
#define E1000_REG_RAH      0x5404  // Receive Address High

// Control register bits
#define E1000_CTRL_RST     (1 << 26)  // Device Reset
#define E1000_CTRL_SLU     (1 << 6)   // Set Link Up
#define E1000_CTRL_ASDE    (1 << 5)   // Auto-Speed Detection Enable
#define E1000_CTRL_FRCSPD  (1 << 11)  // Force Speed
#define E1000_CTRL_FRCDPX  (1 << 12)  // Force Duplex

// Receive Control register bits
#define E1000_RCTL_EN      (1 << 1)   // Receiver Enable
#define E1000_RCTL_SBP     (1 << 2)   // Store Bad Packets
#define E1000_RCTL_UPE     (1 << 3)   // Unicast Promiscuous Enable
#define E1000_RCTL_MPE     (1 << 4)   // Multicast Promiscuous Enable
#define E1000_RCTL_LPE     (1 << 5)   // Long Packet Enable
#define E1000_RCTL_LBM_NONE (0 << 6)  // No Loopback
#define E1000_RCTL_RDMTS_HALF (0 << 8) // Minimum Threshold Select
#define E1000_RCTL_MO_36   (0 << 12)  // Multicast Offset
#define E1000_RCTL_BAM     (1 << 15)  // Broadcast Accept Mode
#define E1000_RCTL_BSIZE_2048 (1 << 16) // Buffer Size 2048
#define E1000_RCTL_SECRC   (1 << 26)  // Strip Ethernet CRC

// Transmit Control register bits
#define E1000_TCTL_EN      (1 << 1)   // Transmit Enable
#define E1000_TCTL_PSP     (1 << 3)   // Pad Short Packets
#define E1000_TCTL_CT      (0xF << 4) // Collision Threshold
#define E1000_TCTL_COLD    (0x3F << 12) // Collision Distance
#define E1000_TCTL_SWXOFF  (1 << 22)  // Software XOFF Transmission

// Interrupt bits
#define E1000_ICR_TXDW     (1 << 0)   // Transmit Descriptor Written Back
#define E1000_ICR_TXQE     (1 << 1)   // Transmit Queue Empty
#define E1000_ICR_LSC      (1 << 2)   // Link Status Change
#define E1000_ICR_RXSEQ    (1 << 3)   // Receive Sequence Error
#define E1000_ICR_RXDMT0   (1 << 4)   // Receive Descriptor Minimum Threshold
#define E1000_ICR_RXO      (1 << 6)   // Receiver Overrun
#define E1000_ICR_RXT0     (1 << 7)   // Receiver Timer Interrupt
#define E1000_ICR_MDAC     (1 << 9)   // MDIO Access Complete
#define E1000_ICR_RXCFG    (1 << 10)  // Receive /C/ Ordered Sets
#define E1000_ICR_GPI      (1 << 18)  // General Purpose Interrupts
#define E1000_ICR_TXD_LOW  (1 << 15)  // Transmit Descriptor Low Threshold

// Descriptor flags
#define E1000_TXD_CMD_EOP  (1 << 0)   // End of Packet
#define E1000_TXD_CMD_IFCS (1 << 1)   // Insert FCS
#define E1000_TXD_CMD_RS   (1 << 3)   // Report Status
#define E1000_TXD_STAT_DD  (1 << 0)   // Descriptor Done

#define E1000_RXD_STAT_DD  (1 << 0)   // Descriptor Done
#define E1000_RXD_STAT_EOP (1 << 1)   // End of Packet

// Descriptor ring sizes
#define E1000_TX_RING_SIZE 32
#define E1000_RX_RING_SIZE 32

// Transmit descriptor
typedef struct {
    uint64_t buffer_addr;
    uint16_t length;
    uint8_t cso;      // Checksum Offset
    uint8_t cmd;      // Command
    uint8_t status;
    uint8_t css;      // Checksum Start
    uint16_t special;
} __attribute__((packed)) e1000_tx_desc_t;

// Receive descriptor
typedef struct {
    uint64_t buffer_addr;
    uint16_t length;
    uint16_t checksum;
    uint8_t status;
    uint8_t errors;
    uint16_t special;
} __attribute__((packed)) e1000_rx_desc_t;

// E1000 device structure
typedef struct {
    uint32_t io_base;        // I/O base address (if I/O mapped)
    volatile uint32_t* mmio_base;  // Memory-mapped I/O base
    pci_device_t pci_dev;
    int initialized;
    mac_address_t mac_address;
    
    // Transmit descriptors
    e1000_tx_desc_t* tx_descriptors;
    void* tx_buffers[E1000_TX_RING_SIZE];
    uint16_t tx_head;
    uint16_t tx_tail;
    
    // Receive descriptors
    e1000_rx_desc_t* rx_descriptors;
    void* rx_buffers[E1000_RX_RING_SIZE];
    uint16_t rx_head;
    uint16_t rx_tail;
} e1000_device_t;

// Initialize e1000 device
int e1000_init(pci_device_t* pci_dev);

// Read from e1000 register
static inline uint32_t e1000_read_reg(volatile uint32_t* mmio_base, uint16_t offset) {
    return mmio_base[offset / 4];
}

// Write to e1000 register
static inline void e1000_write_reg(volatile uint32_t* mmio_base, uint16_t offset, uint32_t value) {
    mmio_base[offset / 4] = value;
}

// Get e1000 device instance
e1000_device_t* e1000_get_device(void);

// Send a packet
int e1000_send_packet(const void* data, size_t length);

// Receive a packet
int e1000_receive_packet(void* buffer, size_t buffer_size);

#endif // E1000_H

