#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "SystemInformation.h"
#include "Overclock.h"

EFI_STATUS
EFIAPI
InitializeSystem(VOID)
{
    EFI_GUID EfiMpServiceProtocolGuid = EFI_MP_SERVICES_PROTOCOL_GUID;
    EFI_STATUS Status = EFI_DEVICE_ERROR;

    // create System object
    System = (PSYSTEM_OBJECT)AllocatePool(sizeof(SYSTEM_OBJECT));

    if (!System)
    {
        return EFI_OUT_OF_RESOURCES;
    }
    else
    {
        // zero it
        SetMem(System, sizeof(SYSTEM_OBJECT), 0);
    }

    // get handle to MP (Multiprocessor) Services Protocol
    Status = gBS->LocateProtocol(
        &EfiMpServiceProtocolGuid,
        NULL,
        (VOID **)&MpServicesProtocol);

    if (EFI_ERROR(Status))
    {
#ifdef UNDERVOLT_DXE
        Print(
            L"[FAILURE] Unable to locate MP Services Protocol (%r)\r\n\0",
            Status);
#endif
        return Status;
    }

    // get default BSP (bootstrap processor)
    Status = MpServicesProtocol->WhoAmI(
        MpServicesProtocol,
        &System->BootstrapProcessor);

    if (EFI_ERROR(Status))
    {
#ifdef UNDERVOLT_DXE
        Print(
            L"[FAILURE] Unable to get bootstrap processor (%r)\r\n\0",
            Status);
#endif
        return Status;
    }

    return EFI_SUCCESS;
}

// creates Platform object and intializes all member variables
EFI_STATUS
EFIAPI
GatherPlatformInfo(
    IN OUT PPLATFORM_OBJECT *PlatformObject)
{
    // allocate memory
    PPLATFORM_OBJECT Platform = (PPLATFORM_OBJECT)AllocatePool(sizeof(PLATFORM_OBJECT));

    if (!Platform)
    {
        return EFI_OUT_OF_RESOURCES;
    }
    else
    {
        // zero it
        SetMem(Platform, sizeof(PLATFORM_OBJECT), 0);
    }

    // get number of logical processors, enabled logical processors
    EFI_STATUS status = MpServicesProtocol->GetNumberOfProcessors(
        MpServicesProtocol,
        &Platform->LogicalProcessors,
        &Platform->EnabledLogicalProcessors);

    if (EFI_ERROR(status))
    {
        return status;
    }

    *PlatformObject = Platform;

    return EFI_SUCCESS;
}

// creates Package object(s) and initializes all member variables
EFI_STATUS
EFIAPI
EnumeratePackage(VOID)
{
    // count number of Threads, detect HyperThreading, calculate Core count and APIC ID
    EFI_PROCESSOR_INFORMATION ProcessorInfo;
    CHAR8 ProcessorBrandStringBuffer[CPUID_BRAND_STRING_LEN + 1];
    UINT32 CpuIdString[4] = {0, 0, 0, 0};
    UINT64 ProgramBuffer = 0;
    UINT64 ResponseBuffer = 0;
    UINTN ThreadCounter = 0;
    UINTN HttEnabled = 0;
    UINTN k;

#ifdef UNDERVOLT_DXE
    Print(
        L"Enumerating processors...\r\n\0");
#endif

    for (UINTN ThreadIndex = 0; ThreadIndex < System->Platform->LogicalProcessors; ThreadIndex++)
    {
        MpServicesProtocol->GetProcessorInfo(
            MpServicesProtocol,
            ThreadIndex,
            &ProcessorInfo);

        ThreadCounter++;

        // detect if HyperThreading enabled
        if ((ProcessorInfo.Location.Thread == 1) && (HttEnabled != 1))
        {
            HttEnabled = 1;
        }

        // last logical processor
        if (ThreadIndex == (System->Platform->LogicalProcessors - 1))
        {
            System->Package.Threads = ThreadCounter;
            System->Package.Cores = System->Package.Threads / (HttEnabled + 1);
            System->Package.APICID = (ThreadIndex - ThreadCounter) + 1;

            break;
        }
    }

    // CPUID
    AsmCpuid(
        CPUID_VERSION_INFO,
        &System->Package.CPUID,
        NULL,
        NULL,
        NULL);

    // Specification
    k = 0;
    for (UINTN i = 0; i < 3; i++)
    {
        AsmCpuid(
            (UINT32)(CPUID_BRAND_STRING_BASE + i),
            &CpuIdString[0],
            &CpuIdString[1],
            &CpuIdString[2],
            &CpuIdString[3]);

        for (UINTN j = 0; j < 4; j++)
        {
            CopyMem(ProcessorBrandStringBuffer + k, &CpuIdString[j], 4);
            k += 4;
        }
    }

    ProcessorBrandStringBuffer[CPUID_BRAND_STRING_LEN + 1] = '\0';

    // convert ASCII to Unicode
    AsciiStrToUnicodeStrS(
        ProcessorBrandStringBuffer,
        System->Package.Specification,
        CPUID_BRAND_STRING_LEN + 1);
#ifdef UNDERVOLT_DXE
    Print(
        L"CPU: %s (%dC/%dT)\r\n\0",
        System->Package.Specification,
        System->Package.Cores,
        System->Package.Threads);
#endif
    // IA Core min ratio, flex ratio
    ResponseBuffer = AsmReadMsr64(MSR_PLATFORM_INFO);

    System->Package.IACore.MinRatio = (ResponseBuffer >> 40) & 0xFF;
    System->Package.IACore.FlexRatio = (ResponseBuffer >> 8) & 0xFF;

    // IA Core max ratio
    ProgramBuffer = OC_MB_GET_CPU_CAPS | OC_MB_DOMAIN_IACORE | OC_MB_COMMAND_EXEC;
    ResponseBuffer = OC_MAILBOX_DISPATCH(ProgramBuffer);

    if (OC_MAILBOX_CLEANUP(ResponseBuffer))
    {
#ifdef UNDERVOLT_DXE
        Print(
            L"[FAILURE] Error getting maximum (1C) Core turbo ratio\r\n\0");
#endif
        return EFI_ABORTED;
    }

    System->Package.IACore.MaxRatio = ResponseBuffer & 0xFF;

    // CLR min ratio
    ResponseBuffer = AsmReadMsr64(MSR_UNCORE_RATIO_LIMIT);
    System->Package.CLR.MinRatio = (ResponseBuffer >> 8) & 0xFF;

    // CLR max ratio
    ProgramBuffer = OC_MB_GET_CPU_CAPS | OC_MB_DOMAIN_CLR | OC_MB_COMMAND_EXEC;
    ResponseBuffer = OC_MAILBOX_DISPATCH(ProgramBuffer);

    if (OC_MAILBOX_CLEANUP(ResponseBuffer))
    {
#ifdef UNDERVOLT_DXE
        Print(
            L"[FAILURE] Error getting maximum Uncore ratio\r\n\0");
#endif
        return EFI_ABORTED;
    }

    return EFI_SUCCESS;
}
