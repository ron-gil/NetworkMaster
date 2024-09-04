#pragma once
#include <ntddk.h>
#include <initguid.h>
#include <fwpmk.h>
#include <wdf.h>

// Global variables
extern HANDLE engineHandle;

#define MAX_SUB_LAYER_GUID 4

extern GUID subLayerGuids[MAX_SUB_LAYER_GUID];
extern ULONG subLayerCount;

// Maximum number of filters that can be stored
#define MAX_FILTERS 10

// Array to store filter GUIDs
extern GUID filterGuids[MAX_FILTERS];
extern ULONG filterCount;

// Driver-related function declarations
DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD NetworkMasterEvtDeviceAdd;
VOID DriverUnload(_In_ PDRIVER_OBJECT DriverObject);

VOID WfpCleanup();
VOID RemoveAllFilters();
VOID RemoveFilter(const GUID filterGuid);
NTSTATUS GenerateGUID(GUID* myGuid);

NTSTATUS CreateFilter(
    const GUID* layerKey,
    FWP_ACTION_TYPE actionType,
    const GUID* calloutKey,
    const GUID* subLayerKey,
    UINT32 numConditions,
    FWPM_FILTER_CONDITION0* conditions,
    wchar_t* name,
    wchar_t* description
);
NTSTATUS CreateSubLayer(wchar_t* name, wchar_t* description);