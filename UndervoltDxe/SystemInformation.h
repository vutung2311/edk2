
#ifndef __SYSTEM_INFORMATION_H__
#define __SYSTEM_INFORMATION_H__

#include <Uefi/UefiBaseType.h>
#include <Protocol/MpService.h>

// objects
typedef struct _DOMAIN_OBJECT
{
    UINTN MinRatio;   // min allowable ratio
    UINTN MaxRatio;   // max fused ratio
    UINTN FlexRatio;  // max non-turbo ratio (MNTR) aka Flex ratio, High Frequency Mode (HFM) ratio
    UINTN RatioLimit; // max allowable ratio
} DOMAIN_OBJECT, *PDOMAIN_OBJECT;

typedef struct _PACKAGE_OBJECT
{
    DOMAIN_OBJECT IACore;      // IA Core domain (0)
    DOMAIN_OBJECT GFX;         // GFX domain (1)
    DOMAIN_OBJECT CLR;         // CLR domain (2)
    DOMAIN_OBJECT SA;          // SA domain (3)
    DOMAIN_OBJECT AIO;         // AIO domain (4)
    UINT32 CPUID;              // cpuid
    CHAR16 Specification[128]; // CPU brand name
    UINTN APICID;              // APIC ID
    UINTN Cores;               // core count
    UINTN Threads;             // thread count
} PACKAGE_OBJECT;

typedef struct _PLATFORM_OBJECT
{
    UINTN LogicalProcessors;        // total logical processors
    UINTN EnabledLogicalProcessors; // total enabled logical processors
} PLATFORM_OBJECT, *PPLATFORM_OBJECT;

typedef struct _SYSTEM_OBJECT
{
    PACKAGE_OBJECT Package;    // Package objects
    PPLATFORM_OBJECT Platform; // Platform object
    UINTN BootstrapProcessor;  // bootstrap processor (BSP) assignment at driver entry
} SYSTEM_OBJECT, *PSYSTEM_OBJECT;

// globals
EFI_MP_SERVICES_PROTOCOL *MpServicesProtocol; // MP Services Protocol handle
PSYSTEM_OBJECT System;                        // System object

EFI_STATUS EFIAPI InitializeSystem(VOID);
EFI_STATUS EFIAPI GatherPlatformInfo(IN OUT PPLATFORM_OBJECT *PlatformObject);
EFI_STATUS EFIAPI EnumeratePackage(VOID);

#endif