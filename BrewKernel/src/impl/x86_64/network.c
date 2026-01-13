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

#include "network.h"
#include "e1000.h"
#include "pci.h"
#include <stdint.h>
#include <stddef.h>

static void* memcpy(void* dest, const void* src, size_t n) {
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dest;
}

static void* memset(void* dest, int value, size_t n) {
    uint8_t* d = (uint8_t*)dest;
    for (size_t i = 0; i < n; i++) {
        d[i] = (uint8_t)value;
    }
    return dest;
}

static int memcmp(const void* s1, const void* s2, size_t n) {
    const uint8_t* p1 = (const uint8_t*)s1;
    const uint8_t* p2 = (const uint8_t*)s2;
    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return (int)p1[i] - (int)p2[i];
        }
    }
    return 0;
}

static int network_initialized = 0;
static mac_address_t our_mac;
static ipv4_address_t our_ip = {{0, 0, 0, 0}};
static uint16_t ipv4_id_counter = 0;

static arp_cache_entry_t arp_cache[ARP_CACHE_SIZE];
static int arp_cache_initialized = 0;

#define UDP_MAX_CALLBACKS 8
typedef struct {
    uint16_t port;
    udp_callback_t callback;
    int valid;
} udp_callback_entry_t;

static udp_callback_entry_t udp_callbacks[UDP_MAX_CALLBACKS];

static int frames_received_count = 0;
static int udp_packets_received_count = 0;
static int udp_callbacks_called_count = 0;
static int e1000_receive_calls = 0;
static int e1000_receive_empty = 0;
static int e1000_receive_no_dd = 0;
static int network_process_calls = 0;

static uint16_t ipv4_checksum(const ipv4_header_t* header) {
    uint32_t sum = 0;
    const uint16_t* words = (const uint16_t*)header;
    
    for (int i = 0; i < 10; i++) {  
        sum += words[i];
    }
    
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    return ~((uint16_t)sum);
}

static uint16_t htons(uint16_t hostshort) {
    return ((hostshort & 0xFF) << 8) | ((hostshort >> 8) & 0xFF);
}

static uint16_t ntohs(uint16_t netshort) {
    return htons(netshort);
}

static uint32_t htonl(uint32_t hostlong) {
    return ((hostlong & 0xFF) << 24) |
           ((hostlong & 0xFF00) << 8) |
           ((hostlong >> 8) & 0xFF00) |
           ((hostlong >> 24) & 0xFF);
}

static void arp_cache_init(void) {
    if (arp_cache_initialized) {
        return;
    }
    memset(arp_cache, 0, sizeof(arp_cache));
    arp_cache_initialized = 1;
}

static arp_cache_entry_t* arp_cache_find(const ipv4_address_t* ip) {
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (arp_cache[i].valid && memcmp(&arp_cache[i].ip, ip, sizeof(ipv4_address_t)) == 0) {
            return &arp_cache[i];
        }
    }
    return NULL;
}

static void arp_cache_add(const ipv4_address_t* ip, const mac_address_t* mac) {
    arp_cache_entry_t* entry = arp_cache_find(ip);
    if (entry) {
        memcpy(&entry->mac, mac, sizeof(mac_address_t));
        entry->timestamp = 0; 
        return;
    }
    
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (!arp_cache[i].valid) {
            memcpy(&arp_cache[i].ip, ip, sizeof(ipv4_address_t));
            memcpy(&arp_cache[i].mac, mac, sizeof(mac_address_t));
            arp_cache[i].timestamp = 0;
            arp_cache[i].valid = 1;
            return;
        }
    }
    
    memcpy(&arp_cache[0].ip, ip, sizeof(ipv4_address_t));
    memcpy(&arp_cache[0].mac, mac, sizeof(mac_address_t));
    arp_cache[0].timestamp = 0;
    arp_cache[0].valid = 1;
}

int network_init(void) {
    if (network_initialized) {
        return 0;
    }
    
    pci_device_t device;
    if (!pci_find_device(E1000_VENDOR_ID, E1000_DEVICE_ID_82540EM, &device)) {
        return -1;  // Network card not found
    }
    
    // Initialize the e1000 device
    if (e1000_init(&device) != 0) {
        return -1;
    }
    
    // Get MAC address
    if (network_get_mac_address(&our_mac) != 0) {
        return -1;
    }
    
    // Initialize ARP cache
    arp_cache_init();
    
    // Initialize UDP callbacks
    memset(udp_callbacks, 0, sizeof(udp_callbacks));
    
    network_initialized = 1;
    
    // Try to acquire IP via DHCP (best-effort)
    (void)network_dhcp_acquire();
    return 0;
}

// Get MAC address
int network_get_mac_address(mac_address_t* mac) {
    e1000_device_t* dev = e1000_get_device();
    if (!dev) {
        return -1;
    }
    
    *mac = dev->mac_address;
    return 0;
}

// Get IPv4 address
int network_get_ipv4_address(ipv4_address_t* ip) {
    if (!network_initialized) {
        return -1;
    }
    *ip = our_ip;
    return 0;
}

// Set IPv4 address
int network_set_ipv4_address(const ipv4_address_t* ip) {
    if (!network_initialized) {
        return -1;
    }
    our_ip = *ip;
    return 0;
}

// Send an Ethernet frame
int network_send_frame(const void* data, size_t length) {
    if (!network_initialized) {
        return -1;
    }
    
    if (length > ETH_FRAME_MAX_SIZE) {
        return -1;
    }
    
    return e1000_send_packet(data, length);
}

// Receive an Ethernet frame (non-blocking)
int network_receive_frame(void* buffer, size_t buffer_size) {
    if (!network_initialized) {
        return 0;
    }
    
    e1000_receive_calls++;
    int result = e1000_receive_packet(buffer, buffer_size);
    if (result == 0) {
        e1000_receive_empty++;
    }
    return result;
}

// Process received Ethernet frames
void network_process_frames(void) {
    network_process_calls++;  // Debug counter
    
    if (!network_initialized) {
        return;
    }
    
    uint8_t frame_buffer[ETH_FRAME_MAX_SIZE];
    int frame_length;
    
    // Process all available frames
    while ((frame_length = network_receive_frame(frame_buffer, sizeof(frame_buffer))) > 0) {
        frames_received_count++;  // Debug counter
        
        if (frame_length < (int)sizeof(eth_header_t)) {
            continue;  // Frame too small
        }
        
        eth_header_t* eth = (eth_header_t*)frame_buffer;
        uint16_t ethertype = ntohs(eth->ethertype);
        
        // Check if frame is for us (or broadcast)
        int is_broadcast = 1;
        int is_for_us = 1;
        for (int i = 0; i < 6; i++) {
            if (eth->dest_mac[i] != 0xFF) {
                is_broadcast = 0;
            }
            if (eth->dest_mac[i] != our_mac.bytes[i]) {
                is_for_us = 0;
            }
        }
        
        if (!is_broadcast && !is_for_us) {
            continue;  // Not for us
        }
        
        // Route based on ethertype
        void* payload = frame_buffer + sizeof(eth_header_t);
        size_t payload_length = frame_length - sizeof(eth_header_t);
        
        if (ethertype == ETH_ETHERTYPE_ARP) {
            if (payload_length >= sizeof(arp_header_t)) {
                arp_process_packet((arp_header_t*)payload, payload_length);
            }
        } else if (ethertype == ETH_ETHERTYPE_IPV4) {
            if (payload_length >= sizeof(ipv4_header_t)) {
                ipv4_header_t* ip = (ipv4_header_t*)payload;
                // Verify checksum
                uint16_t checksum = ip->checksum;
                ip->checksum = 0;
                uint16_t calculated = ipv4_checksum(ip);
                ip->checksum = checksum;
                
                if (checksum == calculated) {
                    int is_for_our_ip = 1;
                    for (int i = 0; i < 4; i++) {
                        if (ip->dest_ip[i] != our_ip.bytes[i]) {
                            is_for_our_ip = 0;
                            break;
                        }
                    }
                    
                    if (is_for_our_ip || ip->dest_ip[0] == 255) {  // Broadcast
                        ipv4_process_packet(ip, payload_length);
                    }
                }
            }
        }
    }
}

// ARP: Send request
int arp_send_request(const ipv4_address_t* target_ip) {
    if (!network_initialized) {
        return -1;
    }
    
    uint8_t frame[ETH_FRAME_MAX_SIZE];
    eth_header_t* eth = (eth_header_t*)frame;
    arp_header_t* arp = (arp_header_t*)(frame + sizeof(eth_header_t));
    
    // Ethernet header
    memset(eth->dest_mac, 0xFF, 6);  // Broadcast
    memcpy(eth->src_mac, our_mac.bytes, 6);
    eth->ethertype = htons(ETH_ETHERTYPE_ARP);
    
    // ARP header
    arp->hw_type = htons(1);  // Ethernet
    arp->proto_type = htons(ETH_ETHERTYPE_IPV4);
    arp->hw_len = 6;
    arp->proto_len = 4;
    arp->opcode = htons(ARP_OP_REQUEST);
    memcpy(arp->sender_mac, our_mac.bytes, 6);
    memcpy(arp->sender_ip, our_ip.bytes, 4);
    memset(arp->target_mac, 0, 6);  // Unknown
    memcpy(arp->target_ip, target_ip->bytes, 4);
    
    size_t frame_length = sizeof(eth_header_t) + sizeof(arp_header_t);
    return network_send_frame(frame, frame_length);
}

// ARP: Lookup MAC address
int arp_lookup(const ipv4_address_t* ip, mac_address_t* mac) {
    if (!network_initialized) {
        return -1;
    }
    
    arp_cache_entry_t* entry = arp_cache_find(ip);
    if (entry && entry->valid) {
        *mac = entry->mac;
        return 0;
    }
    
    // Not in cache - send ARP request
    arp_send_request(ip);
    return -1;  // Not found (would need to wait for reply in real implementation)
}

// ARP: Process received packet
void arp_process_packet(const arp_header_t* arp, size_t length) {
    if (length < sizeof(arp_header_t)) {
        return;
    }
    
    // Only handle Ethernet/IPv4 ARP
    if (ntohs(arp->hw_type) != 1 || ntohs(arp->proto_type) != ETH_ETHERTYPE_IPV4) {
        return;
    }
    
    uint16_t opcode = ntohs(arp->opcode);
    ipv4_address_t sender_ip;
    mac_address_t sender_mac;
    memcpy(sender_ip.bytes, arp->sender_ip, 4);
    memcpy(sender_mac.bytes, arp->sender_mac, 6);
    
    // Add to ARP cache
    arp_cache_add(&sender_ip, &sender_mac);
    
    if (opcode == ARP_OP_REQUEST) {
        // Check if request is for us
        int is_for_us = 1;
        for (int i = 0; i < 4; i++) {
            if (arp->target_ip[i] != our_ip.bytes[i]) {
                is_for_us = 0;
                break;
            }
        }
        
        if (is_for_us) {
            // Send ARP reply
            uint8_t frame[ETH_FRAME_MAX_SIZE];
            eth_header_t* eth = (eth_header_t*)frame;
            arp_header_t* arp_reply = (arp_header_t*)(frame + sizeof(eth_header_t));
            
            // Ethernet header
            memcpy(eth->dest_mac, arp->sender_mac, 6);
            memcpy(eth->src_mac, our_mac.bytes, 6);
            eth->ethertype = htons(ETH_ETHERTYPE_ARP);
            
            // ARP reply
            arp_reply->hw_type = htons(1);
            arp_reply->proto_type = htons(ETH_ETHERTYPE_IPV4);
            arp_reply->hw_len = 6;
            arp_reply->proto_len = 4;
            arp_reply->opcode = htons(ARP_OP_REPLY);
            memcpy(arp_reply->sender_mac, our_mac.bytes, 6);
            memcpy(arp_reply->sender_ip, our_ip.bytes, 4);
            memcpy(arp_reply->target_mac, arp->sender_mac, 6);
            memcpy(arp_reply->target_ip, arp->sender_ip, 4);
            
            size_t frame_length = sizeof(eth_header_t) + sizeof(arp_header_t);
            network_send_frame(frame, frame_length);
        }
    }
}

// IPv4: Send packet
int ipv4_send_packet(const ipv4_address_t* dest_ip, uint8_t protocol,
                     const void* data, size_t data_length) {
    if (!network_initialized) {
        return -1;
    }
    
    // Look up MAC address (or use broadcast for local network)
    mac_address_t dest_mac;
    int arp_ok = arp_lookup(dest_ip, &dest_mac);
    
    // For now, if ARP lookup fails, try to send anyway (might be broadcast)
    // In a real implementation, we'd wait for ARP reply
    
    uint8_t frame[ETH_FRAME_MAX_SIZE];
    eth_header_t* eth = (eth_header_t*)frame;
    ipv4_header_t* ip = (ipv4_header_t*)(frame + sizeof(eth_header_t));
    void* ip_payload = frame + sizeof(eth_header_t) + sizeof(ipv4_header_t);
    
    // Check if we have MAC (or use broadcast)
    if (arp_ok != 0) {
        // Use broadcast MAC for local network (simplified)
        memset(dest_mac.bytes, 0xFF, 6);
    }
    
    // Ethernet header
    memcpy(eth->dest_mac, dest_mac.bytes, 6);
    memcpy(eth->src_mac, our_mac.bytes, 6);
    eth->ethertype = htons(ETH_ETHERTYPE_IPV4);
    
    // IPv4 header
    ip->version_ihl = (4 << 4) | 5;  // Version 4, IHL 5 (20 bytes)
    ip->tos = 0;
    ip->total_length = htons(sizeof(ipv4_header_t) + data_length);
    ip->id = htons(ipv4_id_counter++);
    ip->flags_frag = 0;  // No fragmentation
    ip->ttl = 64;
    ip->protocol = protocol;
    ip->checksum = 0;
    memcpy(ip->src_ip, our_ip.bytes, 4);
    memcpy(ip->dest_ip, dest_ip->bytes, 4);
    
    // Calculate checksum
    ip->checksum = ipv4_checksum(ip);
    
    // Copy payload
    memcpy(ip_payload, data, data_length);
    
    size_t frame_length = sizeof(eth_header_t) + sizeof(ipv4_header_t) + data_length;
    return network_send_frame(frame, frame_length);
}

// IPv4: Process received packet
void ipv4_process_packet(const ipv4_header_t* ip, size_t length) {
    if (length < sizeof(ipv4_header_t)) {
        return;
    }
    
    uint8_t ihl = (ip->version_ihl & 0x0F) * 4;  // Header length in bytes
    if (ihl < 20 || length < ihl) {
        return;  // Invalid header length
    }
    
    uint16_t total_length = ntohs(ip->total_length);
    if (total_length > length) {
        return;  // Invalid length
    }
    
    void* payload = (void*)ip + ihl;
    size_t payload_length = total_length - ihl;
    
    // Route based on protocol
    if (ip->protocol == IP_PROTO_UDP) {
        if (payload_length >= sizeof(udp_header_t)) {
            udp_packets_received_count++;  // Debug counter
            ipv4_address_t src_ip;
            memcpy(src_ip.bytes, ip->src_ip, 4);
            udp_process_packet((udp_header_t*)payload, &src_ip, payload_length);
        }
    }
    // ICMP and TCP can be added later
}

// UDP: Send packet
int udp_send_packet(const ipv4_address_t* dest_ip, uint16_t dest_port,
                    uint16_t src_port, const void* data, size_t data_length) {
    if (!network_initialized) {
        return -1;
    }
    
    uint8_t udp_packet[ETH_FRAME_MAX_SIZE];
    udp_header_t* udp = (udp_header_t*)udp_packet;
    void* udp_payload = udp_packet + sizeof(udp_header_t);
    
    // UDP header
    udp->src_port = htons(src_port);
    udp->dest_port = htons(dest_port);
    udp->length = htons(sizeof(udp_header_t) + data_length);
    udp->checksum = 0;  // Optional for IPv4, set to 0 for now
    
    // Copy payload
    memcpy(udp_payload, data, data_length);
    
    size_t udp_packet_length = sizeof(udp_header_t) + data_length;
    return ipv4_send_packet(dest_ip, IP_PROTO_UDP, udp_packet, udp_packet_length);
}

// UDP: Process received packet
void udp_process_packet(const udp_header_t* udp, const ipv4_address_t* src_ip,
                        size_t length) {
    if (length < sizeof(udp_header_t)) {
        return;
    }
    
    uint16_t dest_port = ntohs(udp->dest_port);
    uint16_t src_port = ntohs(udp->src_port);
    uint16_t udp_length = ntohs(udp->length);
    
    if (udp_length > length) {
        return;  // Invalid length
    }
    
    void* payload = (void*)udp + sizeof(udp_header_t);
    size_t payload_length = udp_length - sizeof(udp_header_t);
    
    // Find callback for this port
    for (int i = 0; i < UDP_MAX_CALLBACKS; i++) {
        if (udp_callbacks[i].valid && udp_callbacks[i].port == dest_port) {
            udp_callbacks_called_count++;  // Debug counter
            udp_callbacks[i].callback(src_ip, src_port, payload, payload_length);
            return;
        }
    }
}

// UDP: Register callback
int udp_register_callback(uint16_t port, udp_callback_t callback) {
    if (!network_initialized) {
        return -1;
    }
    
    // Find empty slot or existing entry for this port
    for (int i = 0; i < UDP_MAX_CALLBACKS; i++) {
        if (!udp_callbacks[i].valid || udp_callbacks[i].port == port) {
            udp_callbacks[i].port = port;
            udp_callbacks[i].callback = callback;
            udp_callbacks[i].valid = 1;
            return 0;
        }
    }
    
    return -1;  // No free slots
}

// Check if network is initialized
int network_is_initialized(void) {
    return network_initialized;
}

// Debug: Get packet statistics
int network_get_frames_received(void) {
    return frames_received_count;
}

int network_get_udp_packets_received(void) {
    return udp_packets_received_count;
}

int network_get_udp_callbacks_called(void) {
    return udp_callbacks_called_count;
}

int network_get_e1000_receive_calls(void) {
    return e1000_receive_calls;
}

int network_get_e1000_receive_empty(void) {
    return e1000_receive_empty;
}

int network_get_process_calls(void) {
    return network_process_calls;
}

// ----------------------------
// Minimal DHCP client (IPv4)
// ----------------------------

#define DHCP_CLIENT_PORT 68
#define DHCP_SERVER_PORT 67

#define DHCP_MAGIC_COOKIE 0x63825363U

#define DHCP_OP_BOOTREQUEST 1
#define DHCP_OP_BOOTREPLY   2

#define DHCP_HTYPE_ETHERNET 1
#define DHCP_HLEN_ETHERNET  6

#define DHCP_MSG_DISCOVER 1
#define DHCP_MSG_OFFER    2
#define DHCP_MSG_REQUEST  3
#define DHCP_MSG_DECLINE  4
#define DHCP_MSG_ACK      5
#define DHCP_MSG_NAK      6

#define DHCP_OPT_MSG_TYPE         53
#define DHCP_OPT_SERVER_ID        54
#define DHCP_OPT_REQ_IP           50
#define DHCP_OPT_PARAM_REQ_LIST   55
#define DHCP_OPT_END              255

typedef struct {
    uint8_t  op;
    uint8_t  htype;
    uint8_t  hlen;
    uint8_t  hops;
    uint32_t xid;
    uint16_t secs;
    uint16_t flags;
    uint32_t ciaddr;
    uint32_t yiaddr;
    uint32_t siaddr;
    uint32_t giaddr;
    uint8_t  chaddr[16];
    uint8_t  sname[64];
    uint8_t  file[128];
    uint32_t magic_cookie;
    uint8_t  options[312]; // enough for minimal usage
} __attribute__((packed)) dhcp_packet_t;

static volatile int dhcp_state = 0; // 0=start, 1=offered, 2=acked, -1=failed
static uint32_t dhcp_xid = 0;
static ipv4_address_t dhcp_offered_ip;
static uint32_t dhcp_server_id = 0;

static uint16_t htons16(uint16_t v) { return ((v & 0xFF) << 8) | ((v >> 8) & 0xFF); }
static uint32_t htonl32(uint32_t v) {
    return ((v & 0xFF) << 24) | ((v & 0xFF00) << 8) | ((v >> 8) & 0xFF00) | ((v >> 24) & 0xFF);
}
static uint32_t ntohl32(uint32_t v) { return htonl32(v); }

static void dhcp_build_discover(dhcp_packet_t* pkt) {
    // Zero packet then fill
    memset(pkt, 0, sizeof(dhcp_packet_t));
    pkt->op = DHCP_OP_BOOTREQUEST;
    pkt->htype = DHCP_HTYPE_ETHERNET;
    pkt->hlen = DHCP_HLEN_ETHERNET;
    pkt->hops = 0;
    pkt->xid = htonl32(dhcp_xid);
    pkt->secs = 0;
    pkt->flags = htons16(0x8000); // broadcast
    pkt->ciaddr = 0;
    pkt->yiaddr = 0;
    pkt->siaddr = 0;
    pkt->giaddr = 0;
    // chaddr: client hardware address
    memcpy(pkt->chaddr, our_mac.bytes, 6);
    pkt->magic_cookie = htonl32(DHCP_MAGIC_COOKIE);
    // options
    uint8_t* opt = pkt->options;
    // DHCP Message Type: DISCOVER
    *opt++ = DHCP_OPT_MSG_TYPE; *opt++ = 1; *opt++ = DHCP_MSG_DISCOVER;
    // Parameter request list
    *opt++ = DHCP_OPT_PARAM_REQ_LIST; *opt++ = 3; // length
    *opt++ = 1;  // Subnet Mask
    *opt++ = 3;  // Router
    *opt++ = 6;  // DNS
    // End
    *opt++ = DHCP_OPT_END;
}

static void dhcp_build_request(dhcp_packet_t* pkt) {
    memset(pkt, 0, sizeof(dhcp_packet_t));
    pkt->op = DHCP_OP_BOOTREQUEST;
    pkt->htype = DHCP_HTYPE_ETHERNET;
    pkt->hlen = DHCP_HLEN_ETHERNET;
    pkt->hops = 0;
    pkt->xid = htonl32(dhcp_xid);
    pkt->secs = 0;
    pkt->flags = htons16(0x8000); // broadcast
    memcpy(pkt->chaddr, our_mac.bytes, 6);
    pkt->magic_cookie = htonl32(DHCP_MAGIC_COOKIE);
    uint8_t* opt = pkt->options;
    // DHCP Message Type: REQUEST
    *opt++ = DHCP_OPT_MSG_TYPE; *opt++ = 1; *opt++ = DHCP_MSG_REQUEST;
    // Requested IP address
    *opt++ = DHCP_OPT_REQ_IP; *opt++ = 4;
    *opt++ = dhcp_offered_ip.bytes[0];
    *opt++ = dhcp_offered_ip.bytes[1];
    *opt++ = dhcp_offered_ip.bytes[2];
    *opt++ = dhcp_offered_ip.bytes[3];
    // Server Identifier
    *opt++ = DHCP_OPT_SERVER_ID; *opt++ = 4;
    *opt++ = (uint8_t)((dhcp_server_id >> 24) & 0xFF);
    *opt++ = (uint8_t)((dhcp_server_id >> 16) & 0xFF);
    *opt++ = (uint8_t)((dhcp_server_id >> 8) & 0xFF);
    *opt++ = (uint8_t)(dhcp_server_id & 0xFF);
    // End
    *opt++ = DHCP_OPT_END;
}

static uint8_t dhcp_get_option(const uint8_t* opts, uint8_t code) {
    // simple walker
    const uint8_t* p = opts;
    while (*p != DHCP_OPT_END) {
        uint8_t opt_code = *p++;
        uint8_t opt_len = *p++;
        if (opt_code == code) {
            return p[0]; // caller knows expected 1-byte option (message type)
        }
        p += opt_len;
    }
    return 0;
}

static void dhcp_udp_callback(const ipv4_address_t* src_ip, uint16_t src_port, void* payload, size_t payload_length) {
    (void)src_ip;
    if (src_port != DHCP_SERVER_PORT || payload_length < sizeof(dhcp_packet_t) - 312) {
        return;
    }
    dhcp_packet_t* pkt = (dhcp_packet_t*)payload;
    if (pkt->op != DHCP_OP_BOOTREPLY) {
        return;
    }
    if (ntohl32(pkt->xid) != dhcp_xid) {
        return;
    }
    if (ntohl32(pkt->magic_cookie) != DHCP_MAGIC_COOKIE) {
        return;
    }
    // Parse options
    uint8_t mtype = dhcp_get_option(pkt->options, DHCP_OPT_MSG_TYPE);
    if (mtype == DHCP_MSG_OFFER) {
        // Record offered IP and server id
        uint32_t yi = pkt->yiaddr;
        uint32_t yi_host = ntohl32(yi);
        dhcp_offered_ip.bytes[0] = (uint8_t)((yi_host >> 24) & 0xFF);
        dhcp_offered_ip.bytes[1] = (uint8_t)((yi_host >> 16) & 0xFF);
        dhcp_offered_ip.bytes[2] = (uint8_t)((yi_host >> 8) & 0xFF);
        dhcp_offered_ip.bytes[3] = (uint8_t)(yi_host & 0xFF);
        // Server identifier
        // Walk options to extract server id (length 4)
        const uint8_t* p = pkt->options;
        dhcp_server_id = 0;
        while (*p != DHCP_OPT_END) {
            uint8_t c = *p++;
            uint8_t l = *p++;
            if (c == DHCP_OPT_SERVER_ID && l == 4) {
                dhcp_server_id = ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
                break;
            }
            p += l;
        }
        if (dhcp_server_id != 0) {
            dhcp_state = 1; // offered
        }
    } else if (mtype == DHCP_MSG_ACK) {
        // Use yiaddr from ACK
        uint32_t yi = pkt->yiaddr;
        uint32_t yi_host = ntohl32(yi);
        our_ip.bytes[0] = (uint8_t)((yi_host >> 24) & 0xFF);
        our_ip.bytes[1] = (uint8_t)((yi_host >> 16) & 0xFF);
        our_ip.bytes[2] = (uint8_t)((yi_host >> 8) & 0xFF);
        our_ip.bytes[3] = (uint8_t)(yi_host & 0xFF);
        dhcp_state = 2; // acked
    } else if (mtype == DHCP_MSG_NAK) {
        dhcp_state = -1;
    }
}

int network_dhcp_acquire(void) {
    if (!network_initialized) {
        return -1;
    }
    // Register UDP callback for client port
    if (udp_register_callback(DHCP_CLIENT_PORT, dhcp_udp_callback) != 0) {
        return -1;
    }
    // Create transaction id (simple counter)
    dhcp_xid += 0x12345u + (uint32_t)ipv4_id_counter;
    dhcp_state = 0;
    dhcp_server_id = 0;
    
    // Send DISCOVER
    dhcp_packet_t pkt;
    dhcp_build_discover(&pkt);
    ipv4_address_t bcast = {{255, 255, 255, 255}};
    (void)udp_send_packet(&bcast, DHCP_SERVER_PORT, DHCP_CLIENT_PORT, &pkt, sizeof(dhcp_packet_t));
    
    // Poll for OFFER
    for (int i = 0; i < 50000 && dhcp_state == 0; i++) {
        network_process_frames();
    }
    if (dhcp_state != 1) {
        return -1;
    }
    
    // Send REQUEST
    dhcp_build_request(&pkt);
    (void)udp_send_packet(&bcast, DHCP_SERVER_PORT, DHCP_CLIENT_PORT, &pkt, sizeof(dhcp_packet_t));
    
    // Poll for ACK
    for (int i = 0; i < 50000 && dhcp_state == 1; i++) {
        network_process_frames();
    }
    return (dhcp_state == 2) ? 0 : -1;
}
