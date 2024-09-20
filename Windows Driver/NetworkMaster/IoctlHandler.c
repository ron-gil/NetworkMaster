#include "UserModeCommunication.h"
#include "IoctlHandler.h"
#include "Driver.h"
#include "WfpFilters.h"
#include "WfpCallout.h"


NTSTATUS InitPacketLogging() {
    NTSTATUS status = STATUS_SUCCESS;
    // Create shared memory
    if (! isPacketLoggingEnabled) {
        status = CreateSharedMemoryAndEvent();
        if (!NT_SUCCESS(status)) {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "NetworkMaster: InitPacketLogging failed with status %X\n", status));
            return status;
        }

        GUID filterKey = LOG_INBOUND_PACKETS_FILTER_GUID;
        GUID layerKey = FWPM_LAYER_INBOUND_MAC_FRAME_ETHERNET;
        GUID calloutKey = LOGGING_PACKETS_CALLOUT_GUID;
        // Activate the packet logging using a filter
        status = CreateFilter(
            &filterKey,
            &layerKey,
            FWP_ACTION_CALLOUT_INSPECTION,
            &calloutKey,
            &inboundSubLayerKey,
            0,
            NULL,
            LOG_INBOUND_PACKETS_FILTER_NAME,
            L"A filter to log all packets that are inbound"
        );
        if (!NT_SUCCESS(status)) {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "NetworkMaster: InitPacketLogging failed to create a filter with status %X\n", status));
            return status;
        }

        isPacketLoggingEnabled = TRUE;
    }

    // Update the shared memory cleaning timer to expire after CLEANUP_TIMEOUT milliseconds
    BOOLEAN isTimerUpdated = WdfTimerStart(timer, WDF_REL_TIMEOUT_IN_MS(CLEANUP_TIMEOUT));
    if (isTimerUpdated) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetworkMaster: Clean up timer has been reset\n"));
    }
    else
    {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetworkMaster: Clean up timer has been started\n"));
    }

    return status;
}


//// Function to handle adding a filter from the input buffer
//NTSTATUS AddFilterFromBuffer(_In_ WDFREQUEST Request)
//{
//    PVOID inputBuffer;
//    size_t inputBufferSize;
//    NTSTATUS status;
//
//    // Get the input buffer from the request
//    status = WdfRequestRetrieveInputBuffer(Request, sizeof(FILTER_INFO), &inputBuffer, &inputBufferSize);
//    if (!NT_SUCCESS(status)) {
//        return status;
//    }
//
//    // Assuming FILTER_INFO structure defines the filter parameters
//    PFILTER_INFO filterInfo = (PFILTER_INFO)inputBuffer;
//
//    // Logic to add the filter using WFP API
//    status = AddFilter(filterInfo);
//
//    return status;
//}
//
//// Function to handle removing a filter from the input buffer
//NTSTATUS RemoveFilterFromBuffer(_In_ WDFREQUEST Request)
//{
//    PVOID inputBuffer;
//    size_t inputBufferSize;
//    NTSTATUS status;
//
//    // Get the input buffer from the request
//    status = WdfRequestRetrieveInputBuffer(Request, sizeof(GUID), &inputBuffer, &inputBufferSize);
//    if (!NT_SUCCESS(status)) {
//        return status;
//    }
//
//    // Assuming the input buffer contains the filter GUID to be removed
//    GUID* filterGuid = (GUID*)inputBuffer;
//
//    // Logic to remove the filter using WFP API
//    status = RemoveFilter(filterGuid);
//
//    return status;
//}
//
// Function to stop inbound traffic
NTSTATUS StopInboundTraffic()
{
    NTSTATUS status;

    GUID filterGuid = STOP_INBOUND_FILTER_GUID;

    // Define the layer for Ethernet MAC frame traffic (this includes ARP packets)
    GUID layerKey = FWPM_LAYER_INBOUND_MAC_FRAME_ETHERNET;

    // No callout is needed, so pass NULL
    GUID* calloutKey = NULL;

    // Action to block the traffic
    FWP_ACTION_TYPE actionType = FWP_ACTION_BLOCK;

    // Define a description for the filter
    wchar_t* description = L"Blocks all inbound packets at the MAC layer depth";

    // Create the filter using the CreateFilter function
    status = CreateFilter(
        &filterGuid,
        &layerKey,          // Ethernet frame layer (Layer 2)
        actionType,         // Block action
        calloutKey,         // No callout needed
        &inboundSubLayerKey,       // InboundSubLayer
        0,      // No conditions, block all packets
        NULL,
        STOP_INBOUND_FILTER_NAME,               // Filter name
        description         // Filter description
    );
    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "NetworkMaster: CreateFilter failed with status %X\n", status));
    }

    return status;
}

//
//// Function to stop outbound traffic
//NTSTATUS StopOutboundTraffic()
//{
//    // Logic to block outbound traffic
//    // This would involve setting a WFP filter that drops outgoing packets
//
//    NTSTATUS status = BlockOutboundPackets(); // Placeholder function
//    return status;
//}
//
// Function to start allowing inbound traffic
NTSTATUS StartInboundTraffic()
{
    GUID filterGuid = STOP_INBOUND_FILTER_GUID;
    NTSTATUS status = RemoveFilter(&filterGuid);

    return status;
}
//
//// Function to start allowing outbound traffic
//NTSTATUS StartOutboundTraffic()
//{
//    // Logic to re-enable outbound traffic
//    // This might involve removing the WFP filter that was blocking outbound packets
//
//    NTSTATUS status = AllowOutboundPackets(); // Placeholder function
//    return status;
//}
//
//// Function to modify a packet from the input buffer
//NTSTATUS ModifyPacketFromBuffer(_In_ WDFREQUEST Request)
//{
//    PVOID inputBuffer;
//    size_t inputBufferSize;
//    NTSTATUS status;
//
//    // Get the input buffer from the request
//    status = WdfRequestRetrieveInputBuffer(Request, sizeof(PACKET_MODIFICATION_INFO), &inputBuffer, &inputBufferSize);
//    if (!NT_SUCCESS(status)) {
//        return status;
//    }
//
//    // Assuming PACKET_MODIFICATION_INFO defines the packet to be modified and changes to be applied
//    PPACKET_MODIFICATION_INFO modificationInfo = (PPACKET_MODIFICATION_INFO)inputBuffer;
//
//    // Logic to modify the packet using WFP API
//    status = ModifyPacket(modificationInfo);
//
//    return status;
//}
