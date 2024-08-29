#include <ntddk.h>
#include <wdf.h>
#include <fwpmk.h>

// Global variables
extern HANDLE engineHandle;

// Function prototypes
DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD NetworkMasterEvtDeviceAdd;
VOID DriverUnload(_In_ PDRIVER_OBJECT DriverObject);
VOID WfpCleanup();
