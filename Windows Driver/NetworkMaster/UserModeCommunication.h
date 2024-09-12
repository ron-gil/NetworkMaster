#pragma once

#include <ntifs.h>
#include <wdf.h>

#define CLEANUP_TIMEOUT 10000 // Timeout in milliseconds (e.g., 10 seconds)
#define SHARED_MEMORY_SIZE 4096  // Size of the shared memory

typedef struct _PACKET_INFO
{
    UINT32 srcIP;         // Source IP address (IPv4)
    UINT32 destIP;        // Destination IP address (IPv4)
    UINT16 srcPort;       // Source port number
    UINT16 destPort;      // Destination port number
    UINT32 protocol;      // Protocol (TCP, UDP, ICMP, etc.)
    UINT32 packetLength;  // Total packet length
    BYTE packetData[256]; // Part of the packet data (first 256 bytes for example)
} PACKET_INFO, * PPACKET_INFO;

extern BOOLEAN isSharedMemoryCreated;

extern WDFTIMER timer;
extern WDFDEVICE device;

extern PVOID sharedMemory;
extern HANDLE sharedMemorySectionHandle;
extern HANDLE hEvent;

NTSTATUS CreateSharedMemoryAndEvent(VOID);

VOID ProcessCapturedPacket(_In_ PACKET_INFO packet);

VOID CleanupResources(VOID);

VOID TimerCallback(_In_ WDFTIMER Timer);

NTSTATUS CreateTimer(VOID);