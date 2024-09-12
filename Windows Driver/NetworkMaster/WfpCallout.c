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
    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "NetworkMaster: Removed and unregistered the callout"));
    return status;
}

VOID NTAPI
LoggingPacketsClassifyFn(
    const FWPS_INCOMING_VALUES* inFixedValues,
    const FWPS_INCOMING_METADATA_VALUES* inMetaValues,
    VOID* layerData,
    const void* classifyContext,
    const FWPS_FILTER* filter,
    UINT64 flowContext,
    FWPS_CLASSIFY_OUT* classifyOut
)
{
    UNREFERENCED_PARAMETER(classifyContext);
    UNREFERENCED_PARAMETER(flowContext);

    if (layerData != NULL) {
        // Extract packet info (for TCP/IP packets)
        // Assume we're capturing at the FWPM_LAYER_INBOUND_TRANSPORT_V4 layer

        const FWPS_INCOMING_VALUES* incomingValues = inFixedValues;
        FWPS_PACKET_INJECTION_STATE packetState;

        // Example code to extract packet details
        if (incomingValues->layerId == FWPS_LAYER_INBOUND_TRANSPORT_V4)
        {
            const NET_BUFFER_LIST* netBufferList = (const NET_BUFFER_LIST*)layerData;
            // Parse the packet data to get the source IP, destination IP, etc.
            PACKET_INFO packetInfo = { 0 };

            // Fill packetInfo with actual data from netBufferList
            // This is where you would parse the headers and extract useful information
            packetInfo.srcIP = ...;    // Extracted source IP
            packetInfo.destIP = ...;   // Extracted destination IP
            packetInfo.srcPort = ...;  // Extracted source port
            packetInfo.destPort = ...; // Extracted destination port
            packetInfo.protocol = ...; // Protocol (TCP, UDP, etc.)
            packetInfo.packetLength = ...; // Packet length
            RtlCopyMemory(packetInfo.packetData, ..., 256); // Copy part of the packet data

            // Process the captured packet
            ProcessCapturedPacket(packetInfo);
        }
    }

    // Allow the packet to pass (since we're only capturing and not blocking here)
    classifyOut->actionType = FWP_ACTION_PERMIT;
}


/*

*/