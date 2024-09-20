#include "UserModeCommunication.h"
#include "WfpFilters.h"

WDFTIMER timer = NULL;
WDFDEVICE device = NULL;

PVOID sharedMemory = NULL;
HANDLE sharedMemorySectionHandle = NULL;
HANDLE hEvent = NULL;

BOOLEAN isPacketLoggingEnabled;

NTSTATUS CreateSharedMemoryAndEvent()
{
    NTSTATUS status;
    UNICODE_STRING eventName; 
    UNICODE_STRING sectionName;
    OBJECT_ATTRIBUTES objAttributes;
    RtlInitUnicodeString(&sectionName, L"\\BaseNamedObjects\\"SHARED_MEMORY_NAME);

    // Create a section object (shared memory)
    LARGE_INTEGER sectionSize;
    sectionSize.QuadPart = SHARED_MEMORY_SIZE;

    InitializeObjectAttributes(&objAttributes, &sectionName, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);

    status = ZwCreateSection(&sharedMemorySectionHandle, SECTION_ALL_ACCESS, &objAttributes, &sectionSize, PAGE_READWRITE, SEC_COMMIT, NULL);
    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Failed to create shared memory section.\n"));
        return status;
    }

    // Map a view of the section into the kernel address space
    SIZE_T viewSize = 0;
    status = ZwMapViewOfSection(sharedMemorySectionHandle, NtCurrentProcess(), &sharedMemory, 0, SHARED_MEMORY_SIZE, NULL, &viewSize, ViewUnmap, 0, PAGE_READWRITE);
    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Failed to map shared memory into kernel space.\n"));
        return status;
    }

    // Create an event for notifying user-mode when data is available
    RtlInitUnicodeString(&eventName, L"\\BaseNamedObjects\\"PACKET_CAPTURE_EVENT_NAME);
    InitializeObjectAttributes(&objAttributes, &eventName, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);
    
    status = ZwCreateEvent(&hEvent, EVENT_ALL_ACCESS, &objAttributes, SynchronizationEvent, FALSE);
    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Failed to create user-mode event.\n"));
        return status;
    }

    return STATUS_SUCCESS;
}

VOID ProcessCapturedPacket(
    _In_ const BYTE* packetData, // Pointer to raw packet data
    _In_ SIZE_T packetSize        // Size of the raw packet data
)
{
    // Ensure the packet size does not exceed the shared memory size
    SIZE_T sizeToCopy = min(packetSize, SHARED_MEMORY_SIZE);

    // Write the packet data to the shared memory
    RtlCopyMemory(sharedMemory, packetData, sizeToCopy);

    // Notify user-mode that new data is available
    ZwSetEvent(hEvent, NULL);
}

VOID CleanupResources()
{
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetworkMaster: CleanupResources - Start\n"));
    NTSTATUS status;

    // Unmap the shared memory view
    if (sharedMemory != NULL)
    {
        status = ZwUnmapViewOfSection(NtCurrentProcess(), sharedMemory);
        if (!NT_SUCCESS(status)) {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Failed to unmap shared memory view.\n"));
        }
        sharedMemory = NULL;
    }

    // Close the shared memory section handle
    if (sharedMemorySectionHandle != NULL)
    {
        status = ZwClose(sharedMemorySectionHandle);
        if (!NT_SUCCESS(status)) {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Failed to close shared memory section handle.\n"));
        }
        sharedMemorySectionHandle = NULL;
    }

    // Close the event handle
    if (hEvent != NULL)
    {
        status = ZwClose(hEvent);
        if (!NT_SUCCESS(status)) {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Failed to close event handle.\n"));
        }
        hEvent = NULL;
    }

    // Delete logging filter
    GUID filterKey = LOG_INBOUND_PACKETS_FILTER_GUID;
    RemoveFilter(&filterKey);

    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetworkMaster: CleanupResources - End\n"));
}

VOID TimerCallback(
    _In_ WDFTIMER Timer
)
{
    UNREFERENCED_PARAMETER(Timer);

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