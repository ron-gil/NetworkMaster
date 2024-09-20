#pragma once
#define NDIS61 1

#include <ntddk.h>
#include <fwpsk.h>

// {E4DD73E3-6EC4-43D8-A8DB-03B779896FE6}
DEFINE_GUID(LOGGING_PACKETS_CALLOUT_GUID,
    0xe4dd73e3, 0x6ec4, 0x43d8, 0xa8, 0xdb, 0x3, 0xb7, 0x79, 0x89, 0x6f, 0xe6);


NTSTATUS CreateCallout(
    void* deviceObject,
    const GUID* definedCalloutKey,
    const GUID* layerKey,
    FWPS_CALLOUT_CLASSIFY_FN classifyFn,
    FWPS_CALLOUT_NOTIFY_FN notifyFn,
    FWPS_CALLOUT_FLOW_DELETE_NOTIFY_FN flowDeleteNotifyFn,
    wchar_t* name,
    wchar_t* description
);

NTSTATUS RemoveCallout(const GUID* calloutKey);

VOID NTAPI
LoggingPacketsClassifyFn(
    const FWPS_INCOMING_VALUES* inFixedValues,
    const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
    VOID* layerData,
    const void* classifyContext,
    const FWPS_FILTER* filter,
    UINT64 flowContext,
    FWPS_CLASSIFY_OUT* classifyOut
);

SIZE_T GetPacketSize(const NET_BUFFER_LIST* nbl);

const BYTE* GetPacketData(const NET_BUFFER_LIST* nbl, SIZE_T* packetSize);


NTSTATUS LoggingPacketsNotifyFn(
    FWPS_CALLOUT_NOTIFY_TYPE notifyType,
    const GUID* filterKey,
    const FWPS_FILTER3* filter
);