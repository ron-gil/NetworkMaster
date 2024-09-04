#include "Driver.h"
#include "WfpCallout.h"

#define GUID_FORMAT "%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX"
#define GUID_ARG(guid) (guid).Data1, (guid).Data2, (guid).Data3, (guid).Data4[0], (guid).Data4[1], (guid).Data4[2], (guid).Data4[3], (guid).Data4[4], (guid).Data4[5], (guid).Data4[6], (guid).Data4[7]

HANDLE engineHandle = NULL;

GUID subLayerGuids[MAX_SUB_LAYER_GUID];
ULONG subLayerCount = 0;

// Array to store the filter GUIDs
GUID filterGuids[MAX_FILTERS];
ULONG filterCount = 0;

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING RegistryPath
)
{
    NTSTATUS status = STATUS_SUCCESS;
    WDF_DRIVER_CONFIG config;

    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetworkMaster: DriverEntry - Start\n"));

    WDF_DRIVER_CONFIG_INIT(&config, NetworkMasterEvtDeviceAdd);

    status = WdfDriverCreate(DriverObject, RegistryPath, WDF_NO_OBJECT_ATTRIBUTES, &config, WDF_NO_HANDLE);
    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "NetworkMaster: WdfDriverCreate failed with status %X\n", status));
        return status;
    }

    DriverObject->DriverUnload = DriverUnload;

    // Initialize the WFP engine
    status = FwpmEngineOpen0(NULL, RPC_C_AUTHN_WINNT, NULL, NULL, &engineHandle);
    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "NetworkMaster: FwpmEngineOpen failed with status %X\n", status));
        return status;
    }

    // Create a subLayer
    status = CreateSubLayer(L"NetworkMaster Sublayer", L"Sublayer for NetworkMaster filters");
    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "NetworkMaster: CreateSubLayer failed with status %X\n", status));
        return status;
    }

    // Create a callout
    status = CreateCallout(DriverObject->DeviceObject, &FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4, NULL, NULL, NULL, L"NetworkMaster Callout", NULL);
    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "NetworkMaster: CreateCallout failed with status %X\n", status));
        return status;
    }

    // Create filter
    status = CreateFilter(
        &FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4,
        FWP_ACTION_BLOCK,
        &calloutKey,
        &subLayerGuids[0],
        0,
        NULL,
        L"NetworkMaster Filter test",
        NULL
    );
    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "NetworkMaster: CreateFilter failed with status %X\n", status));
        return status;
    }

    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetworkMaster: DriverEntry - End\n"));

    return status;
}

NTSTATUS
NetworkMasterEvtDeviceAdd(
    _In_    WDFDRIVER       Driver,
    _Inout_ PWDFDEVICE_INIT DeviceInit
)
{
    UNREFERENCED_PARAMETER(Driver);

    NTSTATUS status = STATUS_SUCCESS;
    WDFDEVICE hDevice;

    // Debugging info: Start of EvtDeviceAdd
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetworkMaster: EvtDeviceAdd - Start\n"));

    // Create the device object
    status = WdfDeviceCreate(&DeviceInit, WDF_NO_OBJECT_ATTRIBUTES, &hDevice);
    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "NetworkMaster: WdfDeviceCreate failed with status %X\n", status));
        return status;
    }

    // Initialize WFP engine for the device if required (in this case, it's already initialized in DriverEntry)

    // Debugging info: End of EvtDeviceAdd
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetworkMaster: EvtDeviceAdd - End\n"));

    return status;
}

VOID WfpCleanup()
{
    // Debugging info: WfpCleanup called
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetworkMaster: WfpCleanup - Start\n"));

    if (engineHandle) {
        RemoveAllFilters();

        FwpmCalloutDeleteByKey0(engineHandle, &calloutKey);
        FwpsCalloutUnregisterByKey0(&calloutKey);
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetworkMaster: Removed and unregistered the callout"));

        FwpmSubLayerDeleteByKey(engineHandle, &subLayerGuids[0]);
        subLayerCount--;
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetworkMaster: Removed subLayer\n"));

        FwpmEngineClose(engineHandle);
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetworkMaster: WfpEngineClosed\n"));
    }

    // Debugging info: WfpCleanup finished
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetworkMaster: WfpCleanup - End\n"));
}

VOID
DriverUnload(
    _In_ PDRIVER_OBJECT DriverObject
)
{
    // Debugging info: DriverUnload called
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetworkMaster: DriverUnload - Start\n"));

    // Clean up WFP resources
    WfpCleanup();

    UNREFERENCED_PARAMETER(DriverObject);

    // Debugging info: DriverUnload finished
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetworkMaster: DriverUnload - End\n"));
}

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

NTSTATUS GenerateGUID(GUID* myGuid) {
    UINT16 tryCount = 0;
    UINT16 const MAX_TRY_AMOUNT = 5;
    // Generate a GUID
    NTSTATUS status = ExUuidCreate(myGuid);
    while (status == STATUS_RETRY && tryCount < MAX_TRY_AMOUNT) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetworkMaster: UuidCreate retrying...\n"));
        status = ExUuidCreate(myGuid);
        tryCount++;
    }
    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "NetworkMaster: UuidCreate failed with status %X\n", status));
        return STATUS_UNSUCCESSFUL;
    }
    return STATUS_SUCCESS;
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

/*##################################
structure of NetworkMaster wfp driver

- provider of NetworkMaster
    - subLayer for outbound traffic
        - filter
        - filter
    - subLayer for inbound traffic
        - filter
        - filter

####################################*/