#pragma once

#include <ntifs.h>
#include <wdf.h>

#define CLEANUP_TIMEOUT 60 * 1000 // Timeout in milliseconds
#define SHARED_MEMORY_SIZE 4096  // Size of the shared memory
#define SHARED_MEMORY_NAME L"NetworkMasterPacketDump"
#define PACKET_CAPTURE_EVENT_NAME L"PacketCaptureEvent"

extern BOOLEAN isPacketLoggingEnabled;

extern WDFTIMER timer;
extern WDFDEVICE device;

extern PMDL sharedMemoryMdl;
extern PVOID sharedMemoryUserVa;
extern HANDLE hEvent;

extern UINT16 loggingPacketsFilterIndex;

NTSTATUS CreateSharedMemoryAndEvent(VOID);

VOID ProcessCapturedPacket(_In_ const BYTE* packetData, _In_ SIZE_T packetSize);

VOID CleanupResources(VOID);

VOID TimerCallback(_In_ WDFTIMER Timer);

NTSTATUS CreateTimer(WDFDEVICE hDevice);