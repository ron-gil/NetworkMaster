#pragma once
#include <ntddk.h>

#define OUTPUT_BUFFER_LENGTH 1024

NTSTATUS InitPacketLogging();
//NTSTATUS AddFilterFromBuffer(_In_ WDFREQUEST Request);
//NTSTATUS RemoveFilterFromBuffer(_In_ WDFREQUEST Request);
NTSTATUS StopInboundTraffic();
//NTSTATUS StopOutboundTraffic();
NTSTATUS StartInboundTraffic();
//NTSTATUS StartOutboundTraffic();
//NTSTATUS ModifyPacketFromBuffer(_In_ WDFREQUEST Request);
