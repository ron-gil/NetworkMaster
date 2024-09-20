#pragma once
#include "Driver.h"

#define STOP_INBOUND_FILTER_NAME L"Block All Inbound Packets"
// {B872359B-C1B7-4AFA-B312-A6628C64D4B4}
DEFINE_GUID(STOP_INBOUND_FILTER_GUID,
    0xb872359b, 0xc1b7, 0x4afa, 0xb3, 0x12, 0xa6, 0x62, 0x8c, 0x64, 0xd4, 0xb4);

#define LOG_INBOUND_PACKETS_FILTER_NAME L"Log Inbound Packets"
// {5D8DF53A-D1F2-4446-9C8E-1AE524896C4E}
DEFINE_GUID(LOG_INBOUND_PACKETS_FILTER_GUID,
    0x5d8df53a, 0xd1f2, 0x4446, 0x9c, 0x8e, 0x1a, 0xe5, 0x24, 0x89, 0x6c, 0x4e);


#define GUID_FORMAT "%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX"
#define GUID_ARG(guid) (guid).Data1, (guid).Data2, (guid).Data3, (guid).Data4[0], (guid).Data4[1], (guid).Data4[2], (guid).Data4[3], (guid).Data4[4], (guid).Data4[5], (guid).Data4[6], (guid).Data4[7]

#define FILTER_PREFIX L"NetworkMaster Filter: "
#define MAX_NAME_LENGTH 256

extern GUID outboundSubLayerKey;
extern GUID inboundSubLayerKey;

// Maximum number of filters that can be stored
#define MAX_FILTERS 10

// Array to store filter GUIDs
extern GUID filterGuids[MAX_FILTERS];
extern UINT16 filterCount;

typedef struct _FILTER_INFO {
    // Add relevant fields for your filter information (e.g., layer, conditions, actions, etc.)
    GUID filterGuid;
    // Other fields...
} FILTER_INFO, * PFILTER_INFO;


VOID RemoveAllFilters();
NTSTATUS RemoveFilter(GUID* filterGuid);

NTSTATUS FindFilterKeyByName(wchar_t* name, GUID* placeholderKey);

VOID ZeroGuid(GUID* guid);
BOOL IsGuidZero(const GUID* guid);
NTSTATUS SaveFilterGuid(const GUID* guid);

NTSTATUS CreateFilter(
    const GUID* definedFilterKey,
    const GUID* layerKey,
    FWP_ACTION_TYPE actionType,
    const GUID* calloutKey,
    const GUID* subLayerKey,
    UINT32 numConditions,
    FWPM_FILTER_CONDITION0* conditions,
    wchar_t* name,
    wchar_t* description
);
NTSTATUS CreateSubLayer(wchar_t* name, wchar_t* description, GUID* placeHolderKey);