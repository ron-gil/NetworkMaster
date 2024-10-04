#include "UserModeCommunication.h"
#include "Driver.h"
#include "WfpCallout.h"
#include "WfpFilters.h"
#include "IoctlHandler.h"


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

    // Create the inbound subLayer
    status = CreateSubLayer(L"NetworkMaster Sublayer: Inbound", L"Sublayer for NetworkMaster inbound filters", &inboundSubLayerKey);
    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "NetworkMaster: CreateSubLayer failed with status %X\n", status));
        return status;
    }

    // Create the outbound subLayer
    status = CreateSubLayer(L"NetworkMaster Sublayer: Outbound", L"Sublayer for NetworkMaster outbound filters", &outboundSubLayerKey);
    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "NetworkMaster: CreateSubLayer failed with status %X\n", status));
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

    NTSTATUS status;
    WDFDEVICE hDevice;
    WDF_IO_QUEUE_CONFIG ioQueueConfig;
    WDF_OBJECT_ATTRIBUTES ioQueueAttributes;
    PDEVICE_OBJECT pDeviceObject;
    DECLARE_CONST_UNICODE_STRING(symbolicLinkName, DOS_DEVICE_NAME);

    // Debugging info: Start of EvtDeviceAdd
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetworkMaster: EvtDeviceAdd - Start\n"));

    // Create the device object
    status = WdfDeviceCreate(&DeviceInit, WDF_NO_OBJECT_ATTRIBUTES, &hDevice);
    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "NetworkMaster: WdfDeviceCreate failed with status %X\n", status));
        return status;
    }

    // Create the symbolic link
    status = WdfDeviceCreateSymbolicLink(hDevice, &symbolicLinkName);
    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "NetworkMaster: WdfDeviceCreateSymbolicLink failed with status %X\n", status));
        return status;
    }

    // Initialize the I/O queue configuration
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&ioQueueConfig, WdfIoQueueDispatchSequential);
    ioQueueConfig.EvtIoDeviceControl = NetworkMasterEvtIoDeviceControl; // Set the I/O control callback

    // Create the default I/O queue
    WDF_OBJECT_ATTRIBUTES_INIT(&ioQueueAttributes);
    status = WdfIoQueueCreate(hDevice, &ioQueueConfig, &ioQueueAttributes, WDF_NO_HANDLE);
    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "NetworkMaster: WdfIoQueueCreate failed with status %X\n", status));
        return status;
    }

    // Create a timer to clean up the user-mode communications resources when it is finished
    status = CreateTimer(hDevice);
    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Failed to create timer.\n"));
        return status;
    }

    isPacketLoggingEnabled = FALSE;

    // Obtain the underlying WDM device object
    pDeviceObject = WdfDeviceWdmGetDeviceObject(hDevice);

    GUID loggingPacketsCalloutKey = LOGGING_PACKETS_CALLOUT_GUID;

    // Create a callout
    status = CreateCallout(
        pDeviceObject,
        &loggingPacketsCalloutKey,
        &FWPM_LAYER_ALE_AUTH_RECV_ACCEPT_V4,
        LoggingPacketsClassifyFn,
        LoggingPacketsNotifyFn,
        NULL,
        L"NetworkMaster Callout: Logging Packets",
        L"The callout driver that writes the received packets to the shared memory space shared with user-mode"
    );
    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "NetworkMaster: CreateCallout failed with status %X\n", status));
        return status;
    }

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

        GUID loggingCalloutKey = LOGGING_PACKETS_CALLOUT_GUID;
        RemoveCallout(&loggingCalloutKey);

        FwpmSubLayerDeleteByKey(engineHandle, &inboundSubLayerKey);
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetworkMaster: Removed inbound subLayer\n"));

        FwpmSubLayerDeleteByKey(engineHandle, &inboundSubLayerKey);
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetworkMaster: Removed outbound subLayer\n"));

        FwpmEngineClose(engineHandle);
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetworkMaster: WfpEngineClosed\n"));
    }

    // Stopping the clean up timer if it is active
    if (timer != NULL && MmIsAddressValid(timer)) {
        WdfTimerStop(timer, FALSE);
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetworkMaster: Released Timer resources\n"));
    }

    // Cleaning up the resources related to user-mode communications if they haven't been released
    CleanupResources();
    isPacketLoggingEnabled = FALSE;
    
    // Debugging info: WfpCleanup finished
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetworkMaster: WfpCleanup - End\n"));
}

VOID
DriverUnload(
    _In_ PDRIVER_OBJECT DriverObject
)
{
    UNREFERENCED_PARAMETER(DriverObject);
    
    // Debugging info: DriverUnload called
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetworkMaster: DriverUnload - Start\n"));

    // Clean up WFP resources
    WfpCleanup();

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
    PVOID* outBuffer = NULL;
    size_t bufferSize = 0;
    size_t returnedInformationSize = 0;

    UNREFERENCED_PARAMETER(Queue);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetworkMaster: Received IOCTL Code: %X\n", IoControlCode));

    // Retrieve the output buffer
    status = WdfRequestRetrieveOutputBuffer(Request, sizeof(NTSTATUS), (PVOID*)&outBuffer, &bufferSize);
    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "NetworkMaster: Failed to retrieve outputBuffer\n"));
        WdfRequestComplete(Request, status);
        return status;
    }

    switch (IoControlCode) {
    case IOCTL_INIT_PACKET_LOGGING:
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetworkMaster: Handling IOCTL_INIT_PACKET_LOGGING\n"));
        // Init packet logging
        status = InitPacketLogging();
        if (NT_SUCCESS(status)) {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetworkMaster: Successfully initalized packet logging.\n"));
        }
        else {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "NetworkMaster: Failed to initialize packet logging. Status: %X\n", status));
            break;
        }

        // Ensure the output buffer length is sufficient for a PVOID (the size of a pointer)
        if (OutputBufferLength < sizeof(PVOID)) {
            status = STATUS_BUFFER_TOO_SMALL;
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "NetworkMaster: Output buffer too small\n"));
            break;
        }

        // Retrieve the output buffer
        status = WdfRequestRetrieveOutputBuffer(Request, sizeof(PVOID), (PVOID*)&outBuffer, &bufferSize);
        if (!NT_SUCCESS(status)) {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "NetworkMaster: Failed to retrieve output buffer\n"));
            break;
        }

        // Copy sharedMemoryUserVa to the output buffer
        *outBuffer = sharedMemoryUserVa;
        returnedInformationSize = sizeof(PVOID);
        break;
    //case IOCTL_ADD_FILTER:
    //    // Handle adding a filter
    //    // Make sure to return to the user the index of the added filter's guid, it will be: filterCount - 1
    //    // Parse the input buffer to get the filter parameters
    //    status = AddFilterFromBuffer(Request);
    //    break;

    //case IOCTL_REMOVE_FILTER:
    //    // Handle removing a filter
    //    // Parse the input buffer to get the filter GUID
    //    status = RemoveFilterFromBuffer(Request);
    //    break;

    case IOCTL_STOP_INBOUND:
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetworkMaster: Handling IOCTL_STOP_INBOUND\n"));
        // Handle stopping inbound traffic
        status = StopInboundTraffic();
        if (NT_SUCCESS(status)) {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetworkMaster: Successfully stopped inbound traffic.\n"));
        }
        else {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "NetworkMaster: Failed to stop inbound traffic. Status: %X\n", status));
        }
        break;

    //case IOCTL_STOP_OUTBOUND:
    //    // Handle stopping outbound traffic
    //    status = StopOutboundTraffic();
    //    break;

    case IOCTL_START_INBOUND:
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetworkMaster: Handling IOCTL_START_INBOUND\n"));
        // Handle starting inbound traffic
        status = StartInboundTraffic();
        if (NT_SUCCESS(status)) {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetworkMaster: Successfully started inbound traffic.\n"));
        }
        else {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "NetworkMaster: Failed to start inbound traffic. Status: 0x%X\n", status));
        }
        break;

    //case IOCTL_START_OUTBOUND:
    //    // Handle starting outbound traffic
    //    status = StartOutboundTraffic();
    //    break;

    //case IOCTL_MODIFY_PACKET:
    //    // Handle modifying a packet
    //    // Parse the input buffer to get the packet ID and structure
    //    status = ModifyPacketFromBuffer(Request);
    //    break;

    default:
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "NetworkMaster: Invalid IOCTL code received: %X\n", IoControlCode));
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    // Complete the request and return the status
    WdfRequestCompleteWithInformation(Request, status, returnedInformationSize);

    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetworkMaster: IOCTL request completed with status: %X\n", status));
   
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