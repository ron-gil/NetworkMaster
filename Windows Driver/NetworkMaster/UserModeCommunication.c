#include "UserModeCommunication.h"
#include "WfpFilters.h"

WDFTIMER timer = NULL;
WDFDEVICE device = NULL;

PMDL sharedMemoryMdl = NULL;
PVOID sharedMemoryUserVa = NULL;
HANDLE hEvent = NULL;

UINT16 loggingPacketsFilterIndex = 0;

BOOLEAN isPacketLoggingEnabled;
NTSTATUS CreateSharedMemoryAndEvent()
{
    NTSTATUS status;
    UNICODE_STRING eventName;
    OBJECT_ATTRIBUTES eventAttributes;
    PVOID sharedMemoryKernelVa = NULL;
    PHYSICAL_ADDRESS lowAddress;
    PHYSICAL_ADDRESS highAddress;
    PHYSICAL_ADDRESS skipBytes;
    SIZE_T totalBytes = SHARED_MEMORY_SIZE;

    // Initialize physical address ranges
    lowAddress.QuadPart = 0;
    highAddress.QuadPart = MAXULONG64;
    skipBytes.QuadPart = 0;

    // Allocate non-paged memory pages for shared memory
    sharedMemoryMdl = MmAllocatePagesForMdlEx(lowAddress, highAddress, skipBytes, totalBytes, MmNonCached, 0);
    if (!sharedMemoryMdl) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Failed to allocate pages for shared memory.\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Map the allocated pages into kernel space
    sharedMemoryKernelVa = MmGetSystemAddressForMdlSafe(sharedMemoryMdl, NormalPagePriority);
    if (!sharedMemoryKernelVa) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Failed to map shared memory into kernel space.\n"));
        MmFreePagesFromMdl(sharedMemoryMdl);
        IoFreeMdl(sharedMemoryMdl);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Shared Memory Kernel Address: %p\n", sharedMemoryKernelVa));

    // Map the shared memory pages into user-mode space
    sharedMemoryUserVa = MmMapLockedPagesSpecifyCache(sharedMemoryMdl, UserMode, MmCached, NULL, FALSE, NormalPagePriority);
    if (!sharedMemoryUserVa) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Failed to map shared memory into user space.\n"));
        MmFreePagesFromMdl(sharedMemoryMdl);
        IoFreeMdl(sharedMemoryMdl);
        return STATUS_UNSUCCESSFUL;
    }

    // Create an event for notifying user-mode when data is available
    RtlInitUnicodeString(&eventName, L"\\BaseNamedObjects\\"PACKET_CAPTURE_EVENT_NAME);
    InitializeObjectAttributes(&eventAttributes, &eventName, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);

    status = ZwCreateEvent(&hEvent, EVENT_ALL_ACCESS, &eventAttributes, SynchronizationEvent, FALSE);
    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Failed to create user-mode event.\n"));
        return status;
    }
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Shared Memory User Address: %p\n", sharedMemoryUserVa));

    return STATUS_SUCCESS;
}

VOID CleanupResources()
{
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetworkMaster: CleanupResources - Start\n"));
    NTSTATUS status;

    // Delete logging filter
    RemoveFilterByIndex(loggingPacketsFilterIndex);

    // Free any allocated memory and release MDL, if applicable
    if (sharedMemoryMdl != NULL)
    {
        // Unmap from user space, if mapped
        if (sharedMemoryUserVa != NULL) {
            MmUnmapLockedPages(sharedMemoryUserVa, sharedMemoryMdl);
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Unmapped locked pages from user space.\n"));
            sharedMemoryUserVa = NULL;
        }

        // Free the allocated pages
        MmFreePagesFromMdl(sharedMemoryMdl); // This frees the kernel space pages
        IoFreeMdl(sharedMemoryMdl); // This frees the MDL itself
        sharedMemoryMdl = NULL;
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Freed MDL and associated memory.\n"));
    }

    // Close the event handle
    if (hEvent != NULL)
    {
        status = ZwClose(hEvent);
        if (!NT_SUCCESS(status)) {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Failed to close event handle with status: %X\n", status));
        }
        else 
        {
            hEvent = NULL;
        }
    }

    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetworkMaster: CleanupResources - End\n"));
}

VOID ProcessCapturedPacket(
    _In_ const BYTE* packetData, // Pointer to raw packet data
    _In_ SIZE_T packetSize        // Size of the raw packet data
)
{
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Writing the packetData to the shared memory\n"));

    // Ensure the packet size does not exceed the shared memory size
    SIZE_T sizeToCopy = min(packetSize, SHARED_MEMORY_SIZE);

    // Get the kernel address for the shared memory
    PVOID sharedMemoryKernelVa = MmGetSystemAddressForMdlSafe(sharedMemoryMdl, NormalPagePriority);
    if (sharedMemoryKernelVa == NULL) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Error processing packet: Failed to get kernel address for shared memory.\n"));
        return;
    }

    // Copy the packet data to the shared memory
    RtlCopyMemory(sharedMemoryKernelVa, packetData, sizeToCopy);

    // Notify user-mode that new data is available
    NTSTATUS status = ZwSetEvent(hEvent, NULL);
    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Failed to set event with status: %X\n", status));
    }
}


VOID TimerCallback(
    _In_ WDFTIMER Timer
)
{
    UNREFERENCED_PARAMETER(Timer);

    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Timer has been triggered.\n"));

    // Call cleanup function
    CleanupResources();

    isPacketLoggingEnabled = FALSE;
}

NTSTATUS CreateTimer(WDFDEVICE hDevice)
{
    NTSTATUS status;
    WDF_TIMER_CONFIG timerConfig;
    WDF_OBJECT_ATTRIBUTES timerAttributes;

    // Initialize the timer configuration
    WDF_TIMER_CONFIG_INIT(&timerConfig, TimerCallback);
    timerConfig.Period = 0; // Timer should not be periodic initially
    timerConfig.AutomaticSerialization = TRUE;    

    // Initialize object attributes and set the parent object
    WDF_OBJECT_ATTRIBUTES_INIT(&timerAttributes);
    timerAttributes.ParentObject = hDevice;  // Set the parent to the device object
    timerAttributes.ExecutionLevel = WdfExecutionLevelPassive;

    // Create the timer with the parent object
    status = WdfTimerCreate(&timerConfig, &timerAttributes, &timer);
    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "CreateTimer failed when calling WdfTimerCreate with status %X\n", status));
        return status;
    }

    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetworkMaster: Successfully created timer\n"));

    return STATUS_SUCCESS;
}


/*
add ioctl to end packet logging which will be called by the cli upon exiting and will just call timer.stop
*/