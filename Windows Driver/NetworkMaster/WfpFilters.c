#include "WfpFilters.h"


GUID subLayerGuids[MAX_SUB_LAYER_GUID];
ULONG subLayerCount = 0;

// Array to store the filter GUIDs
GUID filterGuids[MAX_FILTERS];
ULONG filterCount = 0;

VOID RemoveAllFilters() {
    ULONG tempFilterCount = filterCount;
    for (ULONG i = 0; i < tempFilterCount; ++i) {
        RemoveFilter(filterGuids[i]);
    }
    if (filterCount > 0) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Failed to remove all filters\n"));
    }
    else
    {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetworkMaster: Removed all filters\n"));
    }
}

VOID RemoveFilter(const GUID filterGuid) {
    NTSTATUS status;
    status = FwpmFilterDeleteByKey(engineHandle, &filterGuid);
    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Failed to delete filter with GUID "GUID_FORMAT" (status: %X)\n", GUID_ARG(filterGuid), status));
    }
    else {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Successfully deleted filter with GUID "GUID_FORMAT"\n", GUID_ARG(filterGuid)));
        filterCount--;
    }
}

NTSTATUS CreateFilter(
    const GUID* layerKey,
    FWP_ACTION_TYPE actionType,
    const GUID* actionCalloutKey,
    const GUID* subLayerKey,
    UINT32 numConditions,
    FWPM_FILTER_CONDITION0* conditions,
    wchar_t* name,
    wchar_t* description
) {
    // Add filter to block traffic on IP V4 for all applications.
    FWPM_FILTER0 fwpFilter;
    RtlZeroMemory(&fwpFilter, sizeof(FWPM_FILTER0));

    GUID filterKey;
    NTSTATUS status = GenerateGUID(&filterKey);

    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "NetworkMaster: CreateFilter failed calling GenerateGUID with status %X\n", status));
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
    fwpFilter.numFilterConditions = numConditions; // applies to all traffic
    fwpFilter.filterCondition = conditions;

    fwpFilter.displayData.name = name;
    fwpFilter.displayData.description = description;

    status = FwpmFilterAdd0(engineHandle, &fwpFilter, NULL, NULL);
    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "NetworkMaster: FwpmFilterAdd0 failed with status %X\n", status));
        return status;
    }

    if (filterCount >= MAX_FILTERS) {
        status = STATUS_UNSUCCESSFUL;
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "NetworkMaster: Adding filter failed - Max filters reached\n"));
        return status;
    }

    filterGuids[filterCount++] = filterKey;

    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetworkMaster: Added filter\n"));

    return STATUS_SUCCESS;
}

NTSTATUS CreateSubLayer(wchar_t* name, wchar_t* description) {
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

    subLayerGuids[subLayerCount++] = subLayerKey;

    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetworkMaster: Added subLayer\n"));

    return STATUS_SUCCESS;
}