#pragma once
#include <ntddk.h>
#include <stdint.h>

#pragma pack(push, 1) // Ensure no padding is added by the compiler
typedef struct _ETHERNET_HEADER {
    uint8_t pre[7];             // Preamble indicates the receiver that frame is coming
    uint8_t sfd;                // Start of frame delimiter always set to 10101011
    uint8_t DestinationAddress[6]; // Destination mac address
    uint8_t SourceAddress[6];      // Source mac address
    uint16_t EtherType;         // Network layer protocol (e.g., IPv4, ARP)
} ETHERNET_HEADER;

typedef struct _ARP_HEADER {
    uint16_t htype;           // Hardware type (e.g., 1 for Ethernet)
    uint16_t ptype;           // Protocol type (e.g., 0x0800 for IPv4)
    uint8_t hlen;             // Hardware address length (varies, e.g., 6 for MAC)
    uint8_t plen;             // Protocol address length (varies, e.g., 4 for IPv4)
    uint16_t operation;       // ARP operation (1 = request, 2 = reply)
    uint8_t* senderMac;       // Pointer to sender's hardware (MAC) address
    uint8_t* senderIP;        // Pointer to sender's protocol (IPv4) address
    uint8_t* targetMac;       // Pointer to target's hardware (MAC) address
    uint8_t* targetIP;        // Pointer to target's protocol (IPv4) address
} ARP_HEADER;

/*
ARP_HEADER arpHeader;

// Example: Assuming you have parsed hlen and plen from the ARP packet
arpHeader.hlen = parsedHlen;  // Get hardware address length from the ARP header
arpHeader.plen = parsedPlen;  // Get protocol address length from the ARP header

// Allocate memory for sender and target addresses based on hlen and plen
arpHeader.senderMac = (uint8_t*)malloc(arpHeader.hlen);
arpHeader.senderIP = (uint8_t*)malloc(arpHeader.plen);
arpHeader.targetMac = (uint8_t*)malloc(arpHeader.hlen);
arpHeader.targetIP = (uint8_t*)malloc(arpHeader.plen);

// Copy the data from the packet into these fields (e.g., from a buffer)
memcpy(arpHeader.senderMac, packetBuffer + senderMacOffset, arpHeader.hlen);
memcpy(arpHeader.senderIP, packetBuffer + senderIPOffset, arpHeader.plen);
memcpy(arpHeader.targetMac, packetBuffer + targetMacOffset, arpHeader.hlen);
memcpy(arpHeader.targetIP, packetBuffer + targetIPOffset, arpHeader.plen);


free(arpHeader.senderMac);
free(arpHeader.senderIP);
free(arpHeader.targetMac);
free(arpHeader.targetIP);

*/


typedef struct _IPV4_HEADER {
    uint8_t  version : 4;       // Version (should be 4 for IPv4)
    uint8_t  ihl : 4;           // Internet Header Length (number of 32-bit words)
    uint8_t  dscp : 6;          // Differentiated Services Code Point
    uint8_t  ecn : 2;           // Explicit Congestion Notification
    uint16_t totalLength;     // Total length of the IP packet (header + data)
    uint16_t identification;  // Identification field (used for fragmentation)
    uint16_t flags : 3;         // Flags (e.g., Don't Fragment, More Fragments)
    uint16_t fragmentOffset : 13;// Fragment Offset
    uint8_t  ttl;             // Time to Live (TTL)
    uint8_t  protocol;        // Protocol (e.g., TCP = 6, UDP = 17)
    uint16_t headerChecksum;  // Header checksum (for error-checking)
    uint32_t srcAddr;         // Source IP address (32 bits)
    uint32_t destAddr;        // Destination IP address (32 bits)
    
    // Optional IPv4 options (pointer to options field, based on IHL)
    uint8_t* options;          // Variable length, interpreted based on IHL, options_size = ((ihl * 4) - 20) if ihl > 5
} IPV4_HEADER;

typedef struct _UDP_HEADER {
    uint16_t srcPort;       // Source port (16 bits)
    uint16_t destPort;      // Destination port (16 bits)
    uint16_t length;        // Length of the UDP header and payload (16 bits)
    uint16_t checksum;      // Checksum for error-checking (16 bits)
} UDP_HEADER;

typedef struct _TCP_HEADER {
    uint16_t srcPort;         // Source port (16 bits)
    uint16_t destPort;        // Destination port (16 bits)
    uint32_t seqNumber;       // Sequence number (32 bits)
    uint32_t ackNumber;       // Acknowledgment number (32 bits)
    uint8_t  dataOffset : 4;  // Data offset (4 bits)
    uint8_t  reserved : 4;    // Reserved (4 bits)
    uint8_t  flags;           // Flags (9 bits, lower part of the first byte and the second byte)
    uint16_t windowSize;      // Window size (16 bits)
    uint16_t checksum;        // Checksum (16 bits)
    uint16_t urgentPointer;   // Urgent pointer (16 bits)

    // Optional tcp options (pointer to options field, based on dataOffset)
    uint8_t* options;         // Variable length, interpreted based on dataOffset, options_size = ((dataOffset * 4) - 20) if dataOffset > 5
} TCP_HEADER;





#pragma pack(pop)
