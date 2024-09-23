#include "UserModeCommunication.h"
#include "WfpCallout.h"
#include "Driver.h"

NTSTATUS CreateCallout(
    void* deviceObject,             
    const GUID* definedCalloutKey, 
    const GUID* layerKey,
    FWPS_CALLOUT_CLASSIFY_FN classifyFn, 
    FWPS_CALLOUT_NOTIFY_FN notifyFn,     
    FWPS_CALLOUT_FLOW_DELETE_NOTIFY_FN flowDeleteNotifyFn,
    wchar_t* name,
    wchar_t* description
)
{
    FWPM_CALLOUT mCallout = { 0 };
    
    FWPS_CALLOUT3 sCallout = { 0 };

    NTSTATUS status = STATUS_SUCCESS;
    GUID calloutKey;
    if (!definedCalloutKey) {
        status = GenerateGUID(&calloutKey);
    }
    else {
        calloutKey = *definedCalloutKey;
    }

    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "NetworkMaster: CreateCallout failed calling GenerateGUID with status %X\n", status));
        return status;
    }

    // Register the callout
    sCallout.calloutKey = calloutKey;

    sCallout.classifyFn = classifyFn;
    sCallout.notifyFn = notifyFn;
    sCallout.flowDeleteFn = flowDeleteNotifyFn;

    status = FwpsCalloutRegister3(deviceObject, &sCallout, NULL);
    if (!NT_SUCCESS(status))
    {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "FwpsCalloutRegister3 failed with status %X\n", status));
        return status;
    }

    // Add the callout to the engine
    mCallout.calloutKey = calloutKey;
    mCallout.applicableLayer = *layerKey;

    mCallout.displayData.name = name;
    mCallout.displayData.description = description;

    status = FwpmCalloutAdd0(engineHandle, &mCallout, NULL, NULL);
    if (!NT_SUCCESS(status))
    {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "FwpmCalloutAdd0 failed with status %X\n", status));
        return status;
    }

    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetworkMaster: Added callout\n"));
    return status;
}

NTSTATUS RemoveCallout(const GUID* calloutKey) {
    NTSTATUS status;
    status = FwpmCalloutDeleteByKey0(engineHandle, calloutKey);
    if (!NT_SUCCESS(status))
    {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "FwpmCalloutDeleteByKey0 failed with status %X\n", status));
        return status;
    }
    status = FwpsCalloutUnregisterByKey0(calloutKey);
    if (!NT_SUCCESS(status))
    {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "FwpsCalloutUnregisterByKey0 failed with status %X\n", status));
        return status;
    }
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetworkMaster: Removed and unregistered the callout\n"));
    return status;
}

VOID NTAPI LoggingPacketsClassifyFn(
    const FWPS_INCOMING_VALUES* inFixedValues,
    const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
    VOID* layerData,
    const void* classifyContext,
    const FWPS_FILTER* filter,
    UINT64 flowContext,
    FWPS_CLASSIFY_OUT* classifyOut
) {
    UNREFERENCED_PARAMETER(classifyContext);
    UNREFERENCED_PARAMETER(flowContext);
    UNREFERENCED_PARAMETER(filter);
    UNREFERENCED_PARAMETER(inMetaValues);
    UNREFERENCED_PARAMETER(inFixedValues);

    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetworkMaster: Classify function called for a new packet\n"));

    // Allow the packet to pass
    classifyOut->actionType = FWP_ACTION_PERMIT;

    if (layerData != NULL) {
        // Cast layerData to NET_BUFFER_LIST
        const NET_BUFFER_LIST* netBufferList = (const NET_BUFFER_LIST*)layerData;

        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetworkMaster: Extracting packet data from NET_BUFFER_LIST\n"));

        // Calculate the size of the packet data
        SIZE_T packetSize = 0;
        const BYTE* packetData = GetPacketData(netBufferList, &packetSize);

        if (packetData != NULL) {
            // Process the captured packet by sending it to the user-mode application
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetworkMaster: Packet captured, size: %Iu bytes\n", packetSize));
            ProcessCapturedPacket(packetData, packetSize);

            // Free the allocated memory for packet data
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetworkMaster: Freeing memory allocated for packet data\n"));
            ExFreePoolWithTag((VOID*)packetData, 'pktd');
        }
        else {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "NetworkMaster: Failed to extract packet data\n"));
        }
    }
    else {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "NetworkMaster: No layer data available\n"));
    }
}

SIZE_T GetPacketSize(const NET_BUFFER_LIST* nbl) {
    SIZE_T packetSize = 0;

    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetworkMaster: Calculating packet size\n"));

    if (nbl != NULL) {
        const NET_BUFFER* nb = NET_BUFFER_LIST_FIRST_NB(nbl);
        while (nb != NULL) {
            ULONG dataLength = NET_BUFFER_DATA_LENGTH(nb);
            packetSize += dataLength;
            nb = NET_BUFFER_NEXT_NB(nb);
        }
    }

    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetworkMaster: Packet size calculated: %Iu bytes\n", packetSize));
    return packetSize;
}

const BYTE* GetPacketData(const NET_BUFFER_LIST* nbl, SIZE_T* packetSize) {
    if (nbl == NULL) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "NetworkMaster: NET_BUFFER_LIST is NULL\n"));
        if (packetSize != NULL) *packetSize = 0;
        return NULL;
    }

    // Calculate the total packet size
    *packetSize = GetPacketSize(nbl);
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetworkMaster: Packet size calculated: %zu\n", *packetSize));

    // Ensure packet size is valid before allocation
    if (*packetSize == 0 || *packetSize > SHARED_MEMORY_SIZE) { // Define MAX_PACKET_SIZE as appropriate
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "NetworkMaster: Invalid packet size: %zu\n", *packetSize));
        return NULL;
    }

    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetworkMaster: Allocating memory for packet data\n"));

    // Allocate memory for the packet data
    BYTE* packetData = ExAllocatePool2(NonPagedPool, *packetSize, 'pktd');
    if (packetData == NULL) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "NetworkMaster: Failed to allocate memory for packet data\n"));
        *packetSize = 0;
        return NULL;
    }

    // Copy the data from NET_BUFFER_LIST to packetData
    BYTE* dst = packetData;
    const NET_BUFFER* nb = NET_BUFFER_LIST_FIRST_NB(nbl);
    while (nb != NULL) {
        ULONG dataLength = NET_BUFFER_DATA_LENGTH(nb);
        ULONG mdlOffset = NET_BUFFER_DATA_OFFSET(nb);
        MDL* mdl = NET_BUFFER_FIRST_MDL(nb);

        BYTE* src = MmGetSystemAddressForMdlSafe(mdl, NormalPagePriority);
        if (src == NULL) {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "NetworkMaster: Failed to get system address for MDL\n"));
            ExFreePoolWithTag(packetData, 'pktd');
            return NULL;
        }

        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetworkMaster: Copying %u bytes from MDL to packet data buffer\n", dataLength));
        RtlCopyMemory(dst, src + mdlOffset, dataLength);
        dst += dataLength;

        nb = NET_BUFFER_NEXT_NB(nb);
    }

    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetworkMaster: Packet data successfully extracted\n"));
    return packetData;
}

NTSTATUS LoggingPacketsNotifyFn(
    FWPS_CALLOUT_NOTIFY_TYPE notifyType,
    const GUID* filterKey,
    const FWPS_FILTER3* filter
)
{
    UNREFERENCED_PARAMETER(filterKey);
    UNREFERENCED_PARAMETER(filter);

    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetworkMaster: Callout notify function triggered.\n"));
    if (notifyType == FWPS_CALLOUT_NOTIFY_ADD_FILTER) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Filter is being added.\n"));
    }
    else if (notifyType == FWPS_CALLOUT_NOTIFY_DELETE_FILTER) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "Filter is being deleted.\n"));
    }

    return STATUS_SUCCESS;
}
