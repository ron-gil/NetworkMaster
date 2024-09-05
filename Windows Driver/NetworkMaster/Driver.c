#include "Driver.h"
#include "WfpCallout.h"
#include "WfpFilters.h"


HANDLE engineHandle = NULL;

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

NTSTATUS
NetworkMasterEvtIoDeviceControl(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
)
{
    NTSTATUS status = STATUS_SUCCESS;
    PVOID buffer = NULL;

    UNREFERENCED_PARAMETER(Queue);
    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);

    switch (IoControlCode) {
    case IOCTL_ADD_FILTER:
        // Handle adding a filter
        // Parse the input buffer to get the filter parameters
        status = AddFilterFromBuffer(Request);
        break;

    case IOCTL_REMOVE_FILTER:
        // Handle removing a filter
        // Parse the input buffer to get the filter GUID
        status = RemoveFilterFromBuffer(Request);
        break;

    case IOCTL_STOP_INBOUND:
        // Handle stopping inbound traffic
        status = StopInboundTraffic();
        break;

    case IOCTL_STOP_OUTBOUND:
        // Handle stopping outbound traffic
        status = StopOutboundTraffic();
        break;

    case IOCTL_START_INBOUND:
        // Handle starting inbound traffic
        status = StartInboundTraffic();
        break;

    case IOCTL_START_OUTBOUND:
        // Handle starting outbound traffic
        status = StartOutboundTraffic();
        break;

    case IOCTL_MODIFY_PACKET:
        // Handle modifying a packet
        // Parse the input buffer to get the packet ID and structure
        status = ModifyPacketFromBuffer(Request);
        break;

    default:
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    WdfRequestComplete(Request, status);
    return status;
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