#include "Driver.h"

HANDLE engineHandle = NULL;   // Define the global variable

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING RegistryPath
)
{
    NTSTATUS status = STATUS_SUCCESS;
    WDF_DRIVER_CONFIG config;

    // Debugging info: Start of DriverEntry
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetworkMaster: DriverEntry - Start\n"));

    // Initialize the driver configuration object to register the EvtDeviceAdd callback
    WDF_DRIVER_CONFIG_INIT(&config, NetworkMasterEvtDeviceAdd);

    // Create the driver object
    status = WdfDriverCreate(DriverObject, RegistryPath, WDF_NO_OBJECT_ATTRIBUTES, &config, WDF_NO_HANDLE);
    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "NetworkMaster: WdfDriverCreate failed with status 0x%X\n", status));
        return status;
    }

    // Initialize the WFP engine here
    status = FwpmEngineOpen0(NULL, RPC_C_AUTHN_WINNT, NULL, NULL, &engineHandle);
    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "NetworkMaster: FwpmEngineOpen0 failed with status 0x%X\n", status));
        return status;
    }

    // Set the unload routine
    DriverObject->DriverUnload = DriverUnload;

    // Debugging info: End of DriverEntry
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
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "NetworkMaster: WdfDeviceCreate failed with status 0x%X\n", status));
        return status;
    }

    // Initialize WFP engine for the device if required (in this case, it's already initialized in DriverEntry)

    // Debugging info: End of EvtDeviceAdd
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetworkMaster: EvtDeviceAdd - End\n"));

    return status;
}

VOID
WfpCleanup()
{
    // Debugging info: WfpCleanup called
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetworkMaster: WfpCleanup - Start\n"));

    if (engineHandle) {
        FwpmEngineClose0(engineHandle);
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
