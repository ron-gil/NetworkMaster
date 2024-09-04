#include "WfpCallout.h"
#include "Driver.h"

GUID calloutKey;

NTSTATUS CreateCallout(
    void* deviceObject,             
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
    NTSTATUS status = GenerateGUID(&calloutKey);

    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "NetworkMaster: CreateCallout failed calling GenerateGUID with status %X\n", status));
        return status;
    }

    // Register the callout
    sCallout.calloutKey = calloutKey;
    //???? maybe check this options later
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