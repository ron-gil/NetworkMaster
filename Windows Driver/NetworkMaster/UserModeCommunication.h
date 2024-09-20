#pragma once

#include <ntifs.h>
#include <wdf.h>

#define CLEANUP_TIMEOUT 10000 // Timeout in milliseconds (e.g., 10 seconds)
#define SHARED_MEMORY_SIZE 4096  // Size of the shared memory
#define SHARED_MEMORY_NAME L"NetworkMasterPacketDump"
#define PACKET_CAPTURE_EVENT_NAME L"PacketCaptureEvent"

extern BOOLEAN isPacketLoggingEnabled;

extern WDFTIMER timer;
extern WDFDEVICE device;

extern PVOID sharedMemory;
extern HANDLE sharedMemorySectionHandle;
extern HANDLE hEvent;

NTSTATUS CreateSharedMemoryAndEvent(VOID);

VOID ProcessCapturedPacket(_In_ const BYTE* packetData, _In_ SIZE_T packetSize);

VOID CleanupResources(VOID);

VOID TimerCallback(_In_ WDFTIMER Timer);

NTSTATUS CreateTimer(WDFDEVICE hDevice);