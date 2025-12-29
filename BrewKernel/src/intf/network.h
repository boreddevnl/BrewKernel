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

#ifndef NETWORK_H
#define NETWORK_H

#include <stdint.h>
#include <stddef.h>

// Maximum Ethernet frame size (including header)
#define ETH_FRAME_MAX_SIZE 1518
#define ETH_HEADER_SIZE 14
#define ETH_DATA_MAX_SIZE (ETH_FRAME_MAX_SIZE - ETH_HEADER_SIZE)

// Ethernet ethertypes
#define ETH_ETHERTYPE_ARP  0x0806
#define ETH_ETHERTYPE_IPV4 0x0800

// Ethernet MAC address (6 bytes)
typedef struct {
    uint8_t bytes[6];
} mac_address_t;

// Ethernet frame header
typedef struct {
    uint8_t dest_mac[6];
    uint8_t src_mac[6];
    uint16_t ethertype;
} __attribute__((packed)) eth_header_t;

// IPv4 address (4 bytes)
typedef struct {
    uint8_t bytes[4];
} ipv4_address_t;

// ARP operation codes
#define ARP_OP_REQUEST 1
#define ARP_OP_REPLY   2

// ARP header
typedef struct {
    uint16_t hw_type;      // Hardware type (1 = Ethernet)
    uint16_t proto_type;   // Protocol type (0x0800 = IPv4)
    uint8_t  hw_len;       // Hardware address length (6 for MAC)
    uint8_t  proto_len;    // Protocol address length (4 for IPv4)
    uint16_t opcode;       // Operation (1 = request, 2 = reply)
    uint8_t  sender_mac[6];
    uint8_t  sender_ip[4];
    uint8_t  target_mac[6];
    uint8_t  target_ip[4];
} __attribute__((packed)) arp_header_t;

// IPv4 protocol numbers
#define IP_PROTO_ICMP 1
#define IP_PROTO_TCP  6
#define IP_PROTO_UDP  17

// IPv4 header
typedef struct {
    uint8_t  version_ihl;  // Version (4 bits) + IHL (4 bits)
    uint8_t  tos;          // Type of Service
    uint16_t total_length; // Total length (header + data)
    uint16_t id;           // Identification
    uint16_t flags_frag;   // Flags (3 bits) + Fragment offset (13 bits)
    uint8_t  ttl;          // Time to Live
    uint8_t  protocol;     // Protocol
    uint16_t checksum;     // Header checksum
    uint8_t  src_ip[4];    // Source IP address
    uint8_t  dest_ip[4];   // Destination IP address
} __attribute__((packed)) ipv4_header_t;

// UDP header
typedef struct {
    uint16_t src_port;     // Source port
    uint16_t dest_port;    // Destination port
    uint16_t length;       // UDP length (header + data)
    uint16_t checksum;     // UDP checksum
} __attribute__((packed)) udp_header_t;

// ARP cache entry
#define ARP_CACHE_SIZE 16
#define ARP_CACHE_TIMEOUT 300  // 5 minutes in seconds (simplified)

typedef struct {
    ipv4_address_t ip;
    mac_address_t mac;
    uint32_t timestamp;  // Simple timestamp (can be uptime-based)
    int valid;
} arp_cache_entry_t;

// Initialize network subsystem
int network_init(void);

// Get MAC address
int network_get_mac_address(mac_address_t* mac);

// Get/set IPv4 address
int network_get_ipv4_address(ipv4_address_t* ip);
int network_set_ipv4_address(const ipv4_address_t* ip);

// Send an Ethernet frame
int network_send_frame(const void* data, size_t length);

// Receive an Ethernet frame (non-blocking)
// Returns number of bytes received, 0 if no frame available
int network_receive_frame(void* buffer, size_t buffer_size);

// Process received Ethernet frames (call this periodically)
void network_process_frames(void);

// ARP functions
int arp_send_request(const ipv4_address_t* target_ip);
int arp_lookup(const ipv4_address_t* ip, mac_address_t* mac);
void arp_process_packet(const arp_header_t* arp, size_t length);

// DHCP (best-effort) - acquire an IP from router
int network_dhcp_acquire(void);

// IPv4 functions
int ipv4_send_packet(const ipv4_address_t* dest_ip, uint8_t protocol,
                     const void* data, size_t data_length);
void ipv4_process_packet(const ipv4_header_t* ip, size_t length);

// UDP functions
int udp_send_packet(const ipv4_address_t* dest_ip, uint16_t dest_port,
                    uint16_t src_port, const void* data, size_t data_length);
void udp_process_packet(const udp_header_t* udp, const ipv4_address_t* src_ip,
                        size_t length);

// UDP socket callback type
typedef void (*udp_callback_t)(const ipv4_address_t* src_ip, uint16_t src_port,
                                const void* data, size_t length);

// Register UDP callback for a port
int udp_register_callback(uint16_t port, udp_callback_t callback);

// Check if network is initialized
int network_is_initialized(void);

// Debug: Get packet statistics
int network_get_frames_received(void);
int network_get_udp_packets_received(void);
int network_get_udp_callbacks_called(void);
int network_get_e1000_receive_calls(void);
int network_get_e1000_receive_empty(void);
int network_get_process_calls(void);

#endif // NETWORK_H

