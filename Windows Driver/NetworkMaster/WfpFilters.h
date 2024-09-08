#pragma once
#include "Driver.h"


#define GUID_FORMAT "%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX"
#define GUID_ARG(guid) (guid).Data1, (guid).Data2, (guid).Data3, (guid).Data4[0], (guid).Data4[1], (guid).Data4[2], (guid).Data4[3], (guid).Data4[4], (guid).Data4[5], (guid).Data4[6], (guid).Data4[7]

#define FILTER_PREFIX L"NetworkMaster Filter: "
#define MAX_NAME_LENGTH 256

extern GUID outboundSubLayerKey;
extern GUID inboundSubLayerKey;

// Maximum number of filters that can be stored
#define MAX_FILTERS 10

extern UINT16 filterCount;

// Array to store filter GUIDs
extern GUID filterGuids[MAX_FILTERS];

VOID RemoveAllFilters();
NTSTATUS RemoveFilter(GUID* filterGuid);

NTSTATUS FindFilterKeyByName(wchar_t* name, GUID* placeholderKey);

VOID ZeroGuid(GUID* guid);
BOOL IsGuidZero(const GUID* guid);

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