#include "UserModeCommunication.h"
#include "WfpFilters.h"

WDFTIMER timer;
WDFDEVICE device;

PVOID sharedMemory = NULL;
HANDLE sharedMemorySectionHandle = NULL;
HANDLE hEvent = NULL;

BOOLEAN isPacketLoggingEnabled;

NTSTATUS CreateSharedMemoryAndEvent()
{
    NTSTATUS status;
    UNICODE_STRING eventName;
    OBJECT_ATTRIBUTES objAttributes;

    // Create a section object (shared memory)
    LARGE_INTEGER sectionSize;
    sectionSize.QuadPart = SHARED_MEMORY_SIZE;

    status = ZwCreateSection(&sharedMemorySectionHandle, SECTION_ALL_ACCESS, NULL, &sectionSize, PAGE_READWRITE, SEC_COMMIT, NULL);
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
    RtlInitUnicodeString(&eventName, L"\\BaseNamedObjects\\PacketCaptureEvent");
    InitializeObjectAttributes(&objAttributes, &eventName, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);
    
    status = ZwCreateEvent(&hEvent, EVENT_ALL_ACCESS, &objAttributes, SynchronizationEvent, FALSE);
    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Failed to create user-mode event.\n"));
        return status;
    }

    return STATUS_SUCCESS;
}

VOID ProcessCapturedPacket(_In_ PACKET_INFO packet)
{
    // Write the packet data to the shared memory
    RtlCopyMemory(sharedMemory, &packet, sizeof(PACKET_INFO));

    // Notify user-mode that new data is available
    ZwSetEvent(hEvent, NULL);
}

VOID CleanupResources()
{
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

NTSTATUS CreateTimer()
{
    NTSTATUS status;
    WDF_TIMER_CONFIG timerConfig;

    WDF_TIMER_CONFIG_INIT(&timerConfig, TimerCallback);
    timerConfig.Period = 0; // Timer should not be periodic initially

    status = WdfTimerCreate(&timerConfig, WDF_NO_OBJECT_ATTRIBUTES, &timer);
    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Failed to create timer.\n"));
        return status;
    }
    
    return STATUS_SUCCESS;
}
