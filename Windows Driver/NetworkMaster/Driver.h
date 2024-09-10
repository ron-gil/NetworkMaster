#pragma once
#include <ntddk.h>
#include <wdf.h>
#include <initguid.h>
#include <fwpmk.h>
//#include <winioctl.h>


#define DOS_DEVICE_NAME     L"\\DosDevices\\NetworkMaster"

#define IOCTL_ADD_FILTER           CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_REMOVE_FILTER        CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_STOP_INBOUND         CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_STOP_OUTBOUND        CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_START_INBOUND        CTL_CODE(FILE_DEVICE_UNKNOWN, 0x804, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_START_OUTBOUND       CTL_CODE(FILE_DEVICE_UNKNOWN, 0x805, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_MODIFY_PACKET        CTL_CODE(FILE_DEVICE_UNKNOWN, 0x806, METHOD_BUFFERED, FILE_ANY_ACCESS)


// Global variables
extern HANDLE engineHandle;

// Driver-related function declarations
DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD NetworkMasterEvtDeviceAdd;
NTSTATUS NetworkMasterEvtIoDeviceControl(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
);

VOID DriverUnload(_In_ PDRIVER_OBJECT DriverObject);

VOID WfpCleanup();

NTSTATUS GenerateGUID(GUID* myGuid);
