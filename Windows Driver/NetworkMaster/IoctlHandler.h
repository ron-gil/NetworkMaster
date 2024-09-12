#pragma once
#include <ntddk.h>


typedef struct _PACKET_MODIFICATION_INFO {
    // Add relevant fields for packet modification (e.g., original packet data, changes to apply, etc.)
    UINT32 packetId;
    // Other fields...
} PACKET_MODIFICATION_INFO, * PPACKET_MODIFICATION_INFO;

NTSTATUS InitPacketLogging();
//NTSTATUS AddFilterFromBuffer(_In_ WDFREQUEST Request);
//NTSTATUS RemoveFilterFromBuffer(_In_ WDFREQUEST Request);
NTSTATUS StopInboundTraffic();
//NTSTATUS StopOutboundTraffic();
NTSTATUS StartInboundTraffic();
//NTSTATUS StartOutboundTraffic();
//NTSTATUS ModifyPacketFromBuffer(_In_ WDFREQUEST Request);
