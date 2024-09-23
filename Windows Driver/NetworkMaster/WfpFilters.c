#include "WfpFilters.h"


GUID outboundSubLayerKey;
GUID inboundSubLayerKey;

// Array to store the filter GUIDs
GUID filterGuids[MAX_FILTERS];
UINT16 filterCount = 0;

UINT16 stopInboundFilterIndex;

BOOL IsGuidZero(const GUID* guid) {
    if (guid->Data1 == 0 &&
        guid->Data2 == 0 &&
        guid->Data3 == 0 &&
        memcmp(guid->Data4, "\0\0\0\0\0\0\0\0", sizeof(guid->Data4)) == 0) {
        return TRUE;
    }
    return FALSE;
}

VOID ZeroGuid(GUID* guid) {
    guid->Data1 = 0;
    guid->Data2 = 0;
    guid->Data3 = 0;
    memset(guid->Data4, 0, sizeof(guid->Data4));
}

VOID RemoveAllFilters() {
    for (ULONG i = 0; i < MAX_FILTERS; ++i) {
        if (! IsGuidZero(&filterGuids[i])) {
            RemoveFilter(&filterGuids[i]);
        }
    }

    filterCount = 0;

    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetworkMaster: Removed all filters\n"));
}

NTSTATUS RemoveFilter(GUID* filterGuid) {
    NTSTATUS status;
    status = FwpmFilterDeleteByKey0(engineHandle, filterGuid);
    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Failed to delete filter with GUID "GUID_FORMAT" (status: %X)\n", GUID_ARG(*filterGuid), status));
    }
    else {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Successfully deleted filter with GUID "GUID_FORMAT"\n", GUID_ARG(*filterGuid)));
        // Zero out the guid to signal no longer viable for use
        ZeroGuid(filterGuid);
        
        // Decrease the current filters amount
        filterCount--;
    }
    return status;
}

NTSTATUS RemoveFilterByIndex(UINT16 index) {
    NTSTATUS status;
    if (index < 0 || index >= MAX_FILTERS) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Failed to delete filter: Invalid index given\n"));
    }
    if (IsGuidZero(&filterGuids[index])) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Filter already deleted.\n"));
        return STATUS_SUCCESS;
    }
    status = RemoveFilter(&filterGuids[index]);

    return status;
}

NTSTATUS CreateFilter(
    const GUID* definedFilterKey,
    const GUID* layerKey,
    FWP_ACTION_TYPE actionType,
    const GUID* actionCalloutKey,
    const GUID* subLayerKey,
    UINT32 numConditions,
    FWPM_FILTER_CONDITION0* conditions,
    wchar_t* name,
    wchar_t* description,
    UINT16* filterIndex
) {
    wchar_t fullName[MAX_NAME_LENGTH];

    FWPM_FILTER0 fwpFilter;
    RtlZeroMemory(&fwpFilter, sizeof(FWPM_FILTER0));

    NTSTATUS status = STATUS_SUCCESS;

    GUID filterKey;
    if (!definedFilterKey) {
        status = GenerateGUID(&filterKey);
    }
    else {
        filterKey = *definedFilterKey;
    }
    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "NetworkMaster: CreateFilter failed calling GenerateGUID with status %X\n", status));
        return status;
    }

    status = SaveFilterGuid(&filterKey, filterIndex);
    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "NetworkMaster: CreateFilter failed saving filter guid with status %X\n", status));
        return status;
    }

    fwpFilter.filterKey = filterKey;
    //fwpFilter.layerKey = FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4;
    fwpFilter.layerKey = *layerKey;
    //fwpFilter.action.type = FWP_ACTION_PERMIT;
    fwpFilter.action.type = actionType;
    if (actionCalloutKey) {
        fwpFilter.action.calloutKey = *actionCalloutKey;
    }
    //fwpFilter.subLayerKey = subLayer.subLayerKey;
    fwpFilter.subLayerKey = *subLayerKey;
    fwpFilter.weight.type = FWP_EMPTY; // auto-weight
    //fwpFilter.numFilterConditions = 0; // applies to all traffic
    fwpFilter.numFilterConditions = numConditions;
    fwpFilter.filterCondition = conditions;

    // Ensure the combined length does not exceed the buffer size
    if ((wcslen(FILTER_PREFIX) + wcslen(name)) < MAX_NAME_LENGTH) {
        // Copy the prefix and append the original name
        wcscpy(fullName, FILTER_PREFIX);
        wcscat(fullName, name);
    }
    else {
        status = STATUS_UNSUCCESSFUL;
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "NetworkMaster:  Adding filter failed - Filter name exceeds max length\n"));
        return status;
    }
    fwpFilter.displayData.name = fullName;
    fwpFilter.displayData.description = description;

    status = FwpmFilterAdd0(engineHandle, &fwpFilter, NULL, NULL);
    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "NetworkMaster: FwpmFilterAdd0 failed with status %X\n", status));
        return status;
    }

    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetworkMaster: Added filter\n"));

    return STATUS_SUCCESS;
}

NTSTATUS CreateSubLayer(wchar_t* name, wchar_t* description, GUID* placeHolderKey) {
    // Define and add the sublayer
    FWPM_SUBLAYER0 subLayer;
    RtlZeroMemory(&subLayer, sizeof(FWPM_SUBLAYER0));

    GUID subLayerKey;
    NTSTATUS status = GenerateGUID(&subLayerKey);

    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "NetworkMaster: CreateSubLayer failed calling GenerateGUID with status %X\n", status));
        return status;
    }

    subLayer.subLayerKey = subLayerKey;
    //subLayer.displayData.name = L"NetworkMaster Sublayer";
    subLayer.displayData.name = name;
    //subLayer.displayData.description = L"Sublayer for NetworkMaster filters";
    subLayer.displayData.description = description;
    subLayer.flags = 0;
    subLayer.weight = 0x100;

    status = FwpmSubLayerAdd0(engineHandle, &subLayer, NULL);
    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "NetworkMaster: FwpmSubLayerAdd failed with status %X\n", status));
        return status;
    }

    *placeHolderKey = subLayerKey;

    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetworkMaster: Added subLayer\n"));

    return STATUS_SUCCESS;
}

NTSTATUS FindFilterKeyByName(wchar_t* name, GUID* placeholderKey) {
    FWPM_FILTER_ENUM_TEMPLATE0 enumTemplate;
    FWPM_FILTER0** filters = NULL;
    UINT32 numFilters = 0;
    NTSTATUS status;
    UINT32 i = 0;

    // Initialize the enumeration template (here we're leaving it open to all layers)
    RtlZeroMemory(&enumTemplate, sizeof(enumTemplate));

    // Enumerate all filters
    status = FwpmFilterEnum0(engineHandle, &enumTemplate, 0xFFFFFFFF, &filters, &numFilters);
    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Error enumerating filters: %X\n", status));
        return status;
    }

    // Iterate through the list of filters and compare their names
    for (i = 0; i < numFilters; i++) {
        if (filters[i]->displayData.name && wcscmp(filters[i]->displayData.name, name) == 0) {
            // Found a match, copy the GUID to the placeholder
            *placeholderKey = filters[i]->filterKey;
            break;
        }
    }

    if (i >= numFilters) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Filter of name %ls doesn't exist\n", name));
        return STATUS_UNSUCCESSFUL;
    }

    // Free memory
    FwpmFreeMemory0(filters);
    return STATUS_SUCCESS;
}

NTSTATUS SaveFilterGuid(const GUID* guid, UINT16* filterIndex) {
    // Check if there's room for another filter
    if (filterCount >= MAX_FILTERS) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "NetworkMaster: Adding filter failed - Max filters reached\n"));
        return STATUS_UNSUCCESSFUL;
    }
    // Save the filter by going over the array and looking for an empty space
    for (UINT16 i = 0; i < MAX_FILTERS; i++)
    {
        if (IsGuidZero(&filterGuids[i])) {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetworkMaster: Saving filter guid in index: %d, current amount of filters: %d\n", i, filterCount));
            filterGuids[i] = *guid;
            *filterIndex = i;
            filterCount++;
            return STATUS_SUCCESS;
        }
    }
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "NetworkMaster: Adding filter failed - Failed to find empty space in guid array\n"));
    return STATUS_UNSUCCESSFUL;
}
