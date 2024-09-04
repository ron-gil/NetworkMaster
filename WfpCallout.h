#pragma once
#define NDIS61 1

#include <ntddk.h>

#include <fwpsk.h>

extern GUID calloutKey;

NTSTATUS CreateCallout(
    void* deviceObject,
    const GUID* layerKey,
    FWPS_CALLOUT_CLASSIFY_FN classifyFn,
    FWPS_CALLOUT_NOTIFY_FN notifyFn,
    FWPS_CALLOUT_FLOW_DELETE_NOTIFY_FN flowDeleteNotifyFn,
    wchar_t* name,
    wchar_t* description
);