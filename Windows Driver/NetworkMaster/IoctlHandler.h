#pragma once
#include <ntddk.h>

#define STOP_INBOUND_FILTER_NAME L"Block All Inbound Packets"
// {B872359B-C1B7-4AFA-B312-A6628C64D4B4}
DEFINE_GUID(STOP_INBOUND_FILTER_GUID ,
    0xb872359b, 0xc1b7, 0x4afa, 0xb3, 0x12, 0xa6, 0x62, 0x8c, 0x64, 0xd4, 0xb4);


typedef struct _FILTER_INFO {
    // Add relevant fields for your filter information (e.g., layer, conditions, actions, etc.)
    GUID filterGuid;
    // Other fields...
} FILTER_INFO, * PFILTER_INFO;

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
