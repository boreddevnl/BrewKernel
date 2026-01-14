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

#include "print.h"
#include "network.h"
#include "pci.h"
#include "e1000.h"
#include "network_cli.h"

static int strcmp_kernel_cli(const char *s1, const char *s2) {
	while (*s1 && (*s1 == *s2)) {
		s1++;
		s2++;
	}
	return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

static int strncmp_kernel_cli(const char *s1, const char *s2, int n) {
	for (int i = 0; i < n; i++) {
		if (s1[i] != s2[i] || s1[i] == '\0' || s2[i] == '\0') {
			return (unsigned char)s1[i] - (unsigned char)s2[i];
		}
	}
	return 0;
}

static int brew_strlen_cli(const char* s) {
	int n = 0;
	while (s[n] != '\0') n++;
	return n;
}

// UDP test state and echo callback moved from main.c
static int udp_test_active = 0;
static int udp_received_flag = 0;
static ipv4_address_t udp_received_src_ip;
static uint16_t udp_received_src_port;
static uint16_t udp_received_length;
static uint16_t udp_message_length;  // Actual length of stored message
#define UDP_MESSAGE_BUFFER_SIZE 256
static char udp_received_message[UDP_MESSAGE_BUFFER_SIZE] = {0};  // Initialize to zeros

static void udp_echo_callback(const ipv4_address_t* src_ip, uint16_t src_port,
			      const mac_address_t* src_mac, const void* data, size_t length) {
	udp_received_flag = 1;
	udp_received_src_ip = *src_ip;
	udp_received_src_port = src_port;
	udp_received_length = (uint16_t)length;
	
	// Store message data (with size limit)
	size_t copy_len = length;
	if (copy_len > UDP_MESSAGE_BUFFER_SIZE) {
		copy_len = UDP_MESSAGE_BUFFER_SIZE;
	}
	udp_message_length = (uint16_t)copy_len;
	
	// Clear buffer first
	for (int i = 0; i < UDP_MESSAGE_BUFFER_SIZE; i++) {
		udp_received_message[i] = 0;
	}
	
	// Copy message data - copy raw bytes directly
	if (data && copy_len > 0) {
		const uint8_t* src = (const uint8_t*)data;
		for (size_t i = 0; i < copy_len; i++) {
			udp_received_message[i] = src[i];
		}
	}
	
	// Echo back the data as-is
	udp_send_packet_to_mac(src_ip, src_mac, src_port, 12345, data, length);
}

void net_check_udp_received(void) {
	if (udp_received_flag && udp_test_active) {
		udp_received_flag = 0;
		brew_str("\n[UDP] Received ");
		char num[16];
		int pos = 0;
		int n = (int)udp_received_length;
		if (n == 0) {
			num[pos++] = '0';
		} else {
			while (n > 0 && pos < 15) {
				num[pos++] = '0' + (n % 10);
				n /= 10;
			}
			for (int i = 0; i < pos / 2; i++) {
				char tmp = num[i];
				num[i] = num[pos - 1 - i];
				num[pos - 1 - i] = tmp;
			}
		}
		num[pos] = '\0';
		brew_str(num);
		brew_str(" bytes from ");
		for (int i = 0; i < 4; i++) {
			if (i > 0) brew_str(".");
			n = udp_received_src_ip.bytes[i];
			pos = 0;
			if (n >= 100) {
				num[pos++] = '0' + (n / 100);
				n %= 100;
			}
			if (n >= 10 || pos > 0) {
				num[pos++] = '0' + (n / 10);
				n %= 10;
			}
			num[pos++] = '0' + n;
			num[pos] = '\0';
			brew_str(num);
		}
		brew_str(":");
		n = udp_received_src_port;
		pos = 0;
		if (n == 0) {
			num[pos++] = '0';
		} else {
			while (n > 0 && pos < 15) {
				num[pos++] = '0' + (n % 10);
				n /= 10;
			}
			for (int i = 0; i < pos / 2; i++) {
				char tmp = num[i];
				num[i] = num[pos - 1 - i];
				num[pos - 1 - i] = tmp;
			}
		}
		num[pos] = '\0';
		brew_str(num);
		
		// Only print message if we have data
		if (udp_message_length > 0) {
			brew_str(" - Message: \"");
			
			// Print the received message
			for (uint16_t i = 0; i < udp_message_length; i++) {
				char ch = udp_received_message[i];
				// Print all bytes as-is, including non-printable
				if (ch == '\n') {
					brew_str("\\n");
				} else if (ch == '\r') {
					brew_str("\\r");
				} else if (ch >= 32 && ch < 127) {
					char buf[2] = {ch, '\0'};
					brew_str(buf);
				} else {
					brew_str("?");
				}
			}
			
			brew_str("\"");
		}
		
		brew_str("\n");
	}
}

static void handle_netinfo(void) {
	brew_str("\n");
	if (network_is_initialized()) {
		mac_address_t mac;
		ipv4_address_t ip;
		if (network_get_mac_address(&mac) == 0) {
			brew_str("Network: Initialized\n");
			brew_str("MAC Address: ");
			char hex[] = "0123456789ABCDEF";
			for (int i = 0; i < 6; i++) {
				if (i > 0) brew_str(":");
				char hex_digit[2] = {hex[(mac.bytes[i] >> 4) & 0xF], '\0'};
				brew_str(hex_digit);
				hex_digit[0] = hex[mac.bytes[i] & 0xF];
				brew_str(hex_digit);
			}
			brew_str("\n");
		} else {
			brew_str("Network: Initialized (MAC address unavailable)\n");
		}
		if (network_get_ipv4_address(&ip) == 0) {
			brew_str("IP Address: ");
			char num[4];
			for (int i = 0; i < 4; i++) {
				if (i > 0) brew_str(".");
				int n = ip.bytes[i];
				int pos = 0;
				if (n >= 100) {
					num[pos++] = '0' + (n / 100);
					n %= 100;
				}
				if (n >= 10 || pos > 0) {
					num[pos++] = '0' + (n / 10);
					n %= 10;
				}
				num[pos++] = '0' + n;
				num[pos] = '\0';
				brew_str(num);
			}
			brew_str("\n");
		}
		brew_str("Debug Stats:\n");
		brew_str("  Frames received: ");
		int frames = network_get_frames_received();
		char num[16];
		int pos = 0;
		if (frames == 0) {
			num[pos++] = '0';
		} else {
			int n = frames;
			while (n > 0 && pos < 15) {
				num[pos++] = '0' + (n % 10);
				n /= 10;
			}
			for (int i = 0; i < pos / 2; i++) {
				char tmp = num[i];
				num[i] = num[pos - 1 - i];
				num[pos - 1 - i] = tmp;
			}
		}
		num[pos] = '\0';
		brew_str(num);
		brew_str("\n  UDP packets received: ");
		int udp = network_get_udp_packets_received();
		pos = 0;
		if (udp == 0) {
			num[pos++] = '0';
		} else {
			int n = udp;
			while (n > 0 && pos < 15) {
				num[pos++] = '0' + (n % 10);
				n /= 10;
			}
			for (int i = 0; i < pos / 2; i++) {
				char tmp = num[i];
				num[i] = num[pos - 1 - i];
				num[pos - 1 - i] = tmp;
			}
		}
		num[pos] = '\0';
		brew_str(num);
		brew_str("\n  UDP callbacks called: ");
		int callbacks = network_get_udp_callbacks_called();
		pos = 0;
		if (callbacks == 0) {
			num[pos++] = '0';
		} else {
			int n = callbacks;
			while (n > 0 && pos < 15) {
				num[pos++] = '0' + (n % 10);
				n /= 10;
			}
			for (int i = 0; i < pos / 2; i++) {
				char tmp = num[i];
				num[i] = num[pos - 1 - i];
				num[pos - 1 - i] = tmp;
			}
		}
		num[pos] = '\0';
		brew_str(num);
		brew_str("\n  e1000 receive calls: ");
		int e1000_calls = network_get_e1000_receive_calls();
		pos = 0;
		if (e1000_calls == 0) {
			num[pos++] = '0';
		} else {
			int n = e1000_calls;
			while (n > 0 && pos < 15) {
				num[pos++] = '0' + (n % 10);
				n /= 10;
			}
			for (int i = 0; i < pos / 2; i++) {
				char tmp = num[i];
				num[i] = num[pos - 1 - i];
				num[pos - 1 - i] = tmp;
			}
		}
		num[pos] = '\0';
		brew_str(num);
		brew_str("\n  e1000 receive empty: ");
		int e1000_empty = network_get_e1000_receive_empty();
		pos = 0;
		if (e1000_empty == 0) {
			num[pos++] = '0';
		} else {
			int n = e1000_empty;
			while (n > 0 && pos < 15) {
				num[pos++] = '0' + (n % 10);
				n /= 10;
			}
			for (int i = 0; i < pos / 2; i++) {
				char tmp = num[i];
				num[i] = num[pos - 1 - i];
				num[pos - 1 - i] = tmp;
			}
		}
		num[pos] = '\0';
		brew_str(num);
		brew_str("\n  network_process_frames calls: ");
		int process_calls = network_get_process_calls();
		pos = 0;
		if (process_calls == 0) {
			num[pos++] = '0';
		} else {
			int n = process_calls;
			while (n > 0 && pos < 15) {
				num[pos++] = '0' + (n % 10);
				n /= 10;
			}
			for (int i = 0; i < pos / 2; i++) {
				char tmp = num[i];
				num[i] = num[pos - 1 - i];
				num[pos - 1 - i] = tmp;
			}
		}
		num[pos] = '\0';
		brew_str(num);
		brew_str("\n");
	} else {
		brew_str("Network: Not initialized\n");
		brew_str("Use NETINIT to initialize the network card\n");
	}
}

static void handle_netinit(void) {
	brew_str("\n");
	if (network_is_initialized()) {
		brew_str("Network already initialized\n");
		return;
	}
	brew_str("Initializing network...\n");

	pci_device_t device;
	if (pci_find_device(E1000_VENDOR_ID, E1000_DEVICE_ID_82540EM, &device)) {
		uint32_t bar0 = pci_read_config(device.bus, device.device, device.function, 0x10);
		brew_str("Found e1000 device\n");
		brew_str("BAR0: 0x");
		char hex[] = "0123456789ABCDEF";
		for (int h = 7; h >= 0; h--) {
			char hex_digit[2] = {hex[(bar0 >> (h * 4)) & 0xF], '\0'};
			brew_str(hex_digit);
		}
		brew_str("\n");
		if (bar0 & 1) {
			brew_str("Device is I/O mapped (not supported)\n");
		} else {
			uint32_t mmio_base = bar0 & ~0xF;
			brew_str("MMIO base: 0x");
			for (int h = 7; h >= 0; h--) {
				char hex_digit2[2] = {hex[(mmio_base >> (h * 4)) & 0xF], '\0'};
				brew_str(hex_digit2);
			}
			brew_str("\n");
			if (mmio_base < 0x40000000 || (mmio_base >= 0xFE800000 && mmio_base < 0xFF000000)) {
				brew_str("MMIO address is in mapped range\n");
			} else {
				brew_str("WARNING: MMIO address is NOT in mapped range!\n");
				brew_str("This will cause a page fault. Skipping initialization.\n");
				return;
			}
		}
	} else {
		brew_str("e1000 device not found\n");
		return;
	}
	if (network_init() == 0) {
		brew_str("Network initialized successfully\n");
		brew_str("Use IPSET to configure IP address (e.g., IPSET 10.0.2.15)\n");
	} else {
		brew_str("Network initialization failed\n");
	}
}

static void handle_udptest(int* return_to_prompt) {
	(void)return_to_prompt;
	brew_str("\n");
	if (!network_is_initialized()) {
		brew_str("Network not initialized. Use NETINIT first.\n");
		if (return_to_prompt) *return_to_prompt = 1;
		return;
	}
	
	// Print current network info
	ipv4_address_t ip;
	if (network_get_ipv4_address(&ip) == 0) {
		brew_str("Current IP: ");
		char num[4];
		for (int i = 0; i < 4; i++) {
			if (i > 0) brew_str(".");
			int n = ip.bytes[i];
			int pos = 0;
			if (n >= 100) {
				num[pos++] = '0' + (n / 100);
				n %= 100;
			}
			if (n >= 10 || pos > 0) {
				num[pos++] = '0' + (n / 10);
				n %= 10;
			}
			num[pos++] = '0' + n;
			num[pos] = '\0';
			brew_str(num);
		}
		brew_str("\n");
	} else {
		brew_str("ERROR: Could not get IP address\n");
		return;
	}
	
	if (!udp_test_active) {
		if (udp_register_callback(12345, udp_echo_callback) == 0) {
			brew_str("UDP echo server started on port 12345\n");
			brew_str("Listening for packets...\n");
			brew_str("Send UDP packets to this IP:port from another machine\n");
			brew_str("(Press Ctrl+C to stop, or use NETINFO to check stats)\n");
			udp_test_active = 1;
		} else {
			brew_str("Failed to register UDP callback\n");
		}
	} else {
		brew_str("UDP test already active on port 12345\n");
	}
}

static void handle_ipset(const char* command_buffer) {
	brew_str("\n");
	if (!network_is_initialized()) {
		brew_str("Network not initialized. Use NETINIT first.\n");
		return;
	}
	
	const char* args = &command_buffer[5];  // Skip "IPSET"
	while (*args == ' ') args++;
	
	if (*args == '\0') {
		brew_str("Usage: IPSET <ip address>\n");
		brew_str("Example: IPSET 10.0.2.15\n");
		return;
	}
	
	ipv4_address_t ip;
	int ip_bytes[4] = {0};
	int ip_idx = 0;
	int current = 0;
	const char* p = args;
	
	while (*p && ip_idx < 4) {
		if (*p >= '0' && *p <= '9') {
			current = current * 10 + (*p - '0');
		} else if (*p == '.' || *p == ' ' || *p == '\0') {
			if (ip_idx < 4) {
				ip_bytes[ip_idx++] = current;
			}
			current = 0;
			if (*p != '.') break;
		}
		p++;
	}
	
	if (ip_idx < 4 && current > 0) {
		ip_bytes[ip_idx++] = current;
	}
	
	if (ip_idx != 4) {
		brew_str("Invalid IP address format\n");
		return;
	}
	
	for (int i = 0; i < 4; i++) {
		ip.bytes[i] = (uint8_t)ip_bytes[i];
	}
	
	if (network_set_ipv4_address(&ip) == 0) {
		brew_str("IP address set to ");
		char num[4];
		for (int i = 0; i < 4; i++) {
			if (i > 0) brew_str(".");
			int n = ip.bytes[i];
			int pos = 0;
			if (n >= 100) {
				num[pos++] = '0' + (n / 100);
				n %= 100;
			}
			if (n >= 10 || pos > 0) {
				num[pos++] = '0' + (n / 10);
				n %= 10;
			}
			num[pos++] = '0' + n;
			num[pos] = '\0';
			brew_str(num);
		}
		brew_str("\n");
	} else {
		brew_str("Failed to set IP address\n");
	}
}

static void handle_udpsend(const char* command_buffer) {
	brew_str("\n");
	if (!network_is_initialized()) {
		brew_str("Network not initialized. Use NETINIT first.\n");
		return;
	}
	const char* args = &command_buffer[8];
	while (*args == ' ') args++;
	if (*args == '\0') {
		brew_str("Usage: UDPSEND <ip> <port> <message>\n");
		brew_str("Example: UDPSEND 10.0.2.2 12345 hello\n");
		return;
	}
	ipv4_address_t dest_ip;
	int ip_bytes[4] = {0};
	int ip_idx = 0;
	int current = 0;
	const char* p = args;
	while (*p && ip_idx < 4) {
		if (*p >= '0' && *p <= '9') {
			current = current * 10 + (*p - '0');
		} else if (*p == '.' || *p == ' ') {
			ip_bytes[ip_idx++] = current;
			current = 0;
			if (*p == ' ') break;
		}
		p++;
	}
	if (ip_idx < 4 && current > 0) {
		ip_bytes[ip_idx++] = current;
	}
	for (int i = 0; i < 4; i++) {
		dest_ip.bytes[i] = (uint8_t)ip_bytes[i];
	}
	while (*p == ' ') p++;
	int port = 0;
	while (*p >= '0' && *p <= '9') {
		port = port * 10 + (*p - '0');
		p++;
	}
	while (*p == ' ') p++;
	const char* message = p;
	size_t msg_len = 0;
	while (message[msg_len] != '\0' && msg_len < 200) msg_len++;
	if (msg_len > 0) {
		if (udp_send_packet(&dest_ip, (uint16_t)port, 54321, message, msg_len) == 0) {
			brew_str("UDP packet sent successfully\n");
		} else {
			brew_str("Failed to send UDP packet\n");
		}
	} else {
		brew_str("No message provided\n");
	}
}

int net_handle_command(const char* cmd_upper, const char* command_buffer, int* return_to_prompt) {
	if (strcmp_kernel_cli(cmd_upper, "NETINFO") == 0) {
		handle_netinfo();
		return 1;
	}
	if (strcmp_kernel_cli(cmd_upper, "NETINIT") == 0) {
		handle_netinit();
		return 1;
	}
	if (strcmp_kernel_cli(cmd_upper, "IPSET") == 0 ||
	    (brew_strlen_cli(cmd_upper) > 5 && strncmp_kernel_cli(cmd_upper, "IPSET ", 6) == 0)) {
		handle_ipset(command_buffer);
		return 1;
	}
	if (strcmp_kernel_cli(cmd_upper, "UDPTEST") == 0 ||
	    (brew_strlen_cli(cmd_upper) > 7 && strncmp_kernel_cli(cmd_upper, "UDPTEST ", 8) == 0)) {
		handle_udptest(return_to_prompt);
		return 1;
	}
	if (strcmp_kernel_cli(cmd_upper, "UDPSEND") == 0 ||
	    (brew_strlen_cli(cmd_upper) > 7 && strncmp_kernel_cli(cmd_upper, "UDPSEND ", 8) == 0)) {
		handle_udpsend(command_buffer);
		return 1;
	}
	return 0;
}


