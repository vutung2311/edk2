/** @file
UEFI application to force undervolt on Alienware 13 R3

This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiSmm.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/MpService.h>
#include <Protocol/S3SmmSaveState.h>

// SVID fixed VCCIN voltages
#define _DYNAMIC_SVID 0x0 // FIVR-controlled SVID

// Adaptive negative dynamic voltage offsets for all domains
#define _DYNAMIC_FVID 0x0			  // no change to default VID (Vcore)
#define _FVID_MINUS_10_MV 0xFEC00000  // -10 mV  (-0.010 V)
#define _FVID_MINUS_20_MV 0xFD800000  // -20 mV  (-0.020 V)
#define _FVID_MINUS_30_MV 0xFC200000  // -30 mV  (-0.030 V)
#define _FVID_MINUS_40_MV 0xFAE00000  // -40 mV  (-0.040 V)
#define _FVID_MINUS_50_MV 0xF9A00000  // -50 mV  (-0.050 V)
#define _FVID_MINUS_60_MV 0xF8600000  // -60 mV  (-0.060 V)
#define _FVID_MINUS_65_MV 0xF7A00000  // -65 mV  (-0.065 V)
#define _FVID_MINUS_70_MV 0xF7000000  // -70 mV  (-0.070 V)
#define _FVID_MINUS_75_MV 0xF6800000  // -75 mV  (-0.075 V)
#define _FVID_MINUS_80_MV 0xF5C00000  // -80 mV  (-0.080 V)
#define _FVID_MINUS_90_MV 0xF4800000  // -90 mV  (-0.090 V)
#define _FVID_MINUS_95_MV 0xF3E00000  // -95 mV  (-0.095 V)
#define _FVID_MINUS_100_MV 0xF3400000 // -100 mV (-0.100 V)
#define _FVID_MINUS_110_MV 0xF1E00000 // -110 mV (-0.110 V)
#define _FVID_MINUS_120_MV 0xF0A00000 // -120 mV (-0.120 V)
#define _FVID_MINUS_130_MV 0xEF600000 // -130 mV (-0.130 V)
#define _FVID_MINUS_140_MV 0xEE200000 // -140 mV (-0.140 V)
#define _FVID_MINUS_150_MV 0xECC00000 // -150 mV (-0.150 V)

// toolbox for MSR OC Mailbox (experimental)
#define OC_MB_COMMAND_EXEC 0x8000000000000000
#define OC_MB_GET_CPU_CAPS 0x0000000100000000
#define OC_MB_GET_TURBO_RATIOS 0x0000000200000000
#define OC_MB_GET_FVIDS_RATIOS 0x0000001000000000
#define OC_MB_SET_FVIDS_RATIOS 0x0000001100000000
#define OC_MB_GET_SVID_PARAMS 0x0000001200000000
#define OC_MB_SET_SVID_PARAMS 0x0000001300000000
#define OC_MB_GET_FIVR_PARAMS 0x0000001400000000
#define OC_MB_SET_FIVR_PARAMS 0x0000001500000000

#define OC_MB_DOMAIN_IACORE 0x0000000000000000 // IA Core domain
#define OC_MB_DOMAIN_GFX 0x0000010000000000	// GFX domain
#define OC_MB_DOMAIN_CLR 0x0000020000000000	// CLR (CBo/LLC/Ring) a.k.a. Cache/Uncore domain
#define OC_MB_DOMAIN_SA 0x0000030000000000	 // System Agent (SA) domain
#define OC_MB_DOMAIN_AIO 0x0000040000000000	// AIO domain

#define OC_MB_FIVR_FAULTS_OVRD_EN 0x1			   // bit 0
#define OC_MB_FIVR_EFF_MODE_OVRD_EN 0x2			   // bit 1
#define OC_MB_FIVR_DYN_SVID_CONTROL_DIS 0x80000000 // bit 31

#define OC_MB_SUCCESS 0x0
#define OC_MB_REBOOT_REQUIRED 0x7

// Model Specific Registers
#define MSR_IA32_BIOS_SIGN_ID 0x08B
#define MSR_PLATFORM_INFO 0x0CE
#define MSR_OC_MAILBOX 0x150
#define MSR_FLEX_RATIO 0x194
#define MSR_TURBO_RATIO_LIMIT 0x1AD
#define MSR_TURBO_RATIO_LIMIT1 0x1AE
#define MSR_TURBO_RATIO_LIMIT2 0x1AF
#define MSR_UNCORE_RATIO_LIMIT 0x620

// constants
#define AP_EXEC_TIMEOUT 1000000 // 1 second
#define CPUID_VERSION_INFO 0x1
#define CPUID_BRAND_STRING_BASE 0x80000002
#define CPUID_BRAND_STRING_LEN 0x30
#define MSR_FLEX_RATIO_OC_LOCK_BIT 0x100000				 // bit 20, set to lock MSR 0x194
#define MSR_TURBO_RATIO_SEMAPHORE_BIT 0x8000000000000000 // set to execute changes writen to MSR 0x1AD, 0x1AE, 0x1AF

// build options
#define BUILD_TARGET_CPUID_SIGN 0xFFFFFFFF		  // target CPUID, set 0xFFFFFFFF to bypass checking

// driver settings
const BOOLEAN CPU_SET_FIXED_VCCIN = FALSE;		  // set fixed VCCIN (reboot required)
const BOOLEAN CPU_SET_OC_LOCK = TRUE;			  // set Overlocking Lock at completion of programming
const BOOLEAN UNCORE_PERF_PLIMIT_OVRD_EN = FALSE; // disable Uncore P-states (i.e. force max frequency)
const BOOLEAN FIVR_FAULTS_OVRD_EN = FALSE;		  // disable FIVR Faults
const BOOLEAN FIVR_EFF_MODE_OVRD_EN = FALSE;	  // disable FIVR Efficiency Mode
const BOOLEAN FIVR_DYN_SVID_CONTROL_DIS = FALSE;  // disable FIVR SVID Control ("PowerCut"), forces CPU_SET_FIXED_VCCIN = TRUE

// Serial Voltage Identification (SVID) fixed voltages per package, adjust as needed
const UINT32 SVID_FIXED_VCCIN = _DYNAMIC_SVID;

// Domain 0 (IA Core) dynamic voltage offsets per package, adjust as needed
const UINT32 IACORE_ADAPTIVE_OFFSET = _FVID_MINUS_100_MV;

// Domain 1 (GFX) dynamic voltage offsets per package, adjust as needed
const UINT32 GFX_ADAPTIVE_OFFSET = _FVID_MINUS_50_MV;

// Domain 2 (CLR) dynamic voltage offsets per package, adjust as needed
const UINT32 CLR_ADAPTIVE_OFFSET = _FVID_MINUS_100_MV;

// Domain 3 (SA) dynamic voltage offsets per package, adjust as needed
const UINT32 SA_ADAPTIVE_OFFSET = _FVID_MINUS_50_MV;

// Domain 4 (AIO) dynamic voltage offsets per package, adjust as needed
const UINT32 AIO_ADAPTIVE_OFFSET = _FVID_MINUS_50_MV;

// objects
typedef struct _DOMAIN_OBJECT
{
	UINT32 OffsetVoltage; // Adaptive offset voltage
	UINTN MinRatio;		  // min allowable ratio
	UINTN MaxRatio;		  // max fused ratio
	UINTN FlexRatio;	  // max non-turbo ratio (MNTR) aka Flex ratio, High Frequency Mode (HFM) ratio
	UINTN RatioLimit;	 // max allowable ratio
} DOMAIN_OBJECT, *PDOMAIN_OBJECT;

typedef struct _PACKAGE_OBJECT
{
	DOMAIN_OBJECT IACore;	  // IA Core domain (0)
	DOMAIN_OBJECT GFX;		   // GFX domain (1)
	DOMAIN_OBJECT CLR;		   // CLR domain (2)
	DOMAIN_OBJECT SA;		   // SA domain (3)
	DOMAIN_OBJECT AIO;		   // AIO domain (4)
	UINT32 InputVoltage;	   // VCCIN
	UINT32 CPUID;			   // cpuid
	CHAR16 Specification[128]; // CPU brand name
	UINTN APICID;			   // APIC ID
	UINTN Cores;			   // core count
	UINTN Threads;			   // thread count
} PACKAGE_OBJECT, *PPACKAGE_OBJECT;

typedef struct _PLATFORM_OBJECT
{
	UINTN Packages;					// physical processor package count
	UINTN LogicalProcessors;		// total logical processors
	UINTN EnabledLogicalProcessors; // total enabled logical processors
} PLATFORM_OBJECT, *PPLATFORM_OBJECT;

typedef struct _SYSTEM_OBJECT
{
	PACKAGE_OBJECT Package;	// Package objects
	PPLATFORM_OBJECT Platform; // Platform object
	UINTN BootstrapProcessor;  // bootstrap processor (BSP) assignment at driver entry
	BOOLEAN RebootRequired;	// flag set when reboot required
} SYSTEM_OBJECT, *PSYSTEM_OBJECT;

// globals
EFI_MP_SERVICES_PROTOCOL *MpServicesProtocol = NULL; // MP Services Protocol handle
STATIC PSYSTEM_OBJECT System;						 // System object
UINT64 ResponseBuffer;								 // general buffer for MSR data reading
UINT64 ProgramBuffer;								 // general buffer for MSR data writing

// function prototypes
EFI_STATUS EFIAPI SaveToBootScript(VOID);
EFI_STATUS EFIAPI InitializeSystem(VOID);
EFI_STATUS EFIAPI GatherPlatformInfo(IN OUT PPLATFORM_OBJECT *PlatformObject);
EFI_STATUS EFIAPI EnumeratePackage(VOID);
BOOLEAN EFIAPI IsValidPackage(VOID);
VOID EFIAPI ProgramPackage(VOID);
VOID EFIAPI FinalizeProgramming(VOID);
VOID EFIAPI SetFIVRConfiguration(VOID);

VOID EFIAPI OC_MAILBOX_DISPATCH(VOID);
VOID EFIAPI OC_MAILBOX_REQUEST(VOID);
BOOLEAN EFIAPI OC_MAILBOX_CLEANUP(VOID);

// driver main
EFI_STATUS
EFIAPI
EfiDriverEntry(
	IN EFI_HANDLE ImageHandle,
	IN EFI_SYSTEM_TABLE *SystemTable)
{
	EFI_STATUS Status;
	gBS = SystemTable->BootServices;

	// initialize system data
	Status = InitializeSystem();
	if (EFI_ERROR(Status))
	{
		goto DriverError;
	}

	// initialize platform data
	Status = GatherPlatformInfo(&System->Platform);
	if (EFI_ERROR(Status))
	{
		goto DriverError;
	}

	// enumerate processors and gather processor data
	Status = EnumeratePackage();
	if (EFI_ERROR(Status))
	{
		goto DriverError;
	}

	ProgramPackage();

	// save configuration to boot script
	Status = SaveToBootScript();
	if (EFI_ERROR(Status))
	{
		goto DriverError;
	}

	// notify if system reboot required
	if (System->RebootRequired == TRUE)
	{
		SystemTable->RuntimeServices->ResetSystem(EfiResetWarm, EFI_SUCCESS, 0, NULL);
		CpuDeadLoop();
	}

	// always return success, no cleanup as everything is automatically destroyed
	return EFI_SUCCESS;

DriverError:
	return Status;
}

EFI_STATUS
EFIAPI
InitializeSystem(VOID)
{
	EFI_GUID EfiMpServiceProtocolGuid = EFI_MP_SERVICES_PROTOCOL_GUID;
	EFI_STATUS Status;

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
		return Status;
	}

	// get default BSP (bootstrap processor)
	Status = MpServicesProtocol->WhoAmI(
		MpServicesProtocol,
		&System->BootstrapProcessor);

	if (EFI_ERROR(Status))
	{
		return Status;
	}

	return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
SaveToBootScript(VOID)
{
	EFI_STATUS Status;
	EFI_S3_SAVE_STATE_PROTOCOL *S3SmmSaveStateProtocol;

	Status = gBS->LocateProtocol(
		&gEfiS3SmmSaveStateProtocolGuid,
		NULL,
		(VOID **)&S3SmmSaveStateProtocol);
	if (EFI_ERROR(Status) && Status != EFI_ALREADY_STARTED)
	{
		return Status;
	}

	// Register programing package from S3 resume
	return S3SmmSaveStateProtocol->Write(
		S3SmmSaveStateProtocol,
		EFI_BOOT_SCRIPT_DISPATCH_OPCODE,
		ProgramPackage);
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

// creates Packages object(s) and initializes all member variables
EFI_STATUS
EFIAPI
EnumeratePackage(VOID)
{
	// count number of Threads, detect HyperThreading, calculate Core count and APIC ID
	EFI_PROCESSOR_INFORMATION processor_info;
	CHAR8 ProcessorBrandStringBuffer[CPUID_BRAND_STRING_LEN + 1];
	UINT32 CpuIdString[4] = {0, 0, 0, 0};
	UINTN ThreadCounter = 0;
	UINTN HttEnabled = 0;
	UINTN k;

	for (UINTN ThreadIndex = 0; ThreadIndex < System->Platform->LogicalProcessors; ThreadIndex++)
	{
		MpServicesProtocol->GetProcessorInfo(
			MpServicesProtocol,
			ThreadIndex,
			&processor_info);

		ThreadCounter++;

		// detect if HyperThreading enabled
		if ((processor_info.Location.Thread == 1) && (HttEnabled != 1))
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

	// VCCIN
	System->Package.InputVoltage = SVID_FIXED_VCCIN;

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
			CopyMem(
				ProcessorBrandStringBuffer + k,
				&CpuIdString[j],
				4);

			k += 4;
		}
	}

	ProcessorBrandStringBuffer[CPUID_BRAND_STRING_LEN + 1] = '\0';

	// convert ASCII to Unicode
	AsciiStrToUnicodeStrS(
		ProcessorBrandStringBuffer,
		System->Package.Specification,
		CPUID_BRAND_STRING_LEN + 1);

	// IA Core offset voltage
	System->Package.IACore.OffsetVoltage = IACORE_ADAPTIVE_OFFSET;

	// IA Core min ratio, flex ratio
	ResponseBuffer = AsmReadMsr64(
		MSR_PLATFORM_INFO);

	System->Package.IACore.MinRatio = (ResponseBuffer >> 40) & 0xFF;
	System->Package.IACore.FlexRatio = (ResponseBuffer >> 8) & 0xFF;

	// IA Core max ratio
	ProgramBuffer = OC_MB_GET_CPU_CAPS | OC_MB_DOMAIN_IACORE | OC_MB_COMMAND_EXEC;

	OC_MAILBOX_DISPATCH();

	if (OC_MAILBOX_CLEANUP())
	{
		return EFI_ABORTED;
	}

	System->Package.IACore.MaxRatio = ResponseBuffer & 0xFF;

	// CLR offset voltage
	System->Package.CLR.OffsetVoltage = CLR_ADAPTIVE_OFFSET;

	// CLR min ratio
	ResponseBuffer = AsmReadMsr64(
		MSR_UNCORE_RATIO_LIMIT);

	System->Package.CLR.MinRatio = (ResponseBuffer >> 8) & 0xFF;

	// CLR max ratio
	ProgramBuffer = OC_MB_GET_CPU_CAPS | OC_MB_DOMAIN_CLR | OC_MB_COMMAND_EXEC;

	OC_MAILBOX_DISPATCH();

	if (OC_MAILBOX_CLEANUP())
	{
		return EFI_ABORTED;
	}

	System->Package.CLR.MaxRatio = ResponseBuffer & 0xFF;

	// GFX offset voltage
	System->Package.GFX.OffsetVoltage = GFX_ADAPTIVE_OFFSET;

	// SA offset voltage
	System->Package.SA.OffsetVoltage = SA_ADAPTIVE_OFFSET;

	// AIO offset voltage
	System->Package.AIO.OffsetVoltage = AIO_ADAPTIVE_OFFSET;

	return EFI_SUCCESS;
}

BOOLEAN
EFIAPI
IsValidPackage(VOID)
{
	// check OC Lock Bit not set
	ResponseBuffer = AsmReadMsr64(
		MSR_FLEX_RATIO);

	if ((ResponseBuffer & MSR_FLEX_RATIO_OC_LOCK_BIT) == MSR_FLEX_RATIO_OC_LOCK_BIT)
	{
		return FALSE;
	}

	// ensure package CPUID matches build target CPUID or override set
	if ((System->Package.CPUID != BUILD_TARGET_CPUID_SIGN) && (BUILD_TARGET_CPUID_SIGN != 0xFFFFFFFF))
	{
		return FALSE;
	}

	// verify no processor microcode revision patch loaded
	AsmWriteMsr64(
		MSR_IA32_BIOS_SIGN_ID,
		0);

	AsmCpuid(
		CPUID_VERSION_INFO,
		NULL,
		NULL,
		NULL,
		NULL);

	ResponseBuffer = AsmReadMsr64(
		MSR_IA32_BIOS_SIGN_ID);

	return TRUE;
}

// programs processor package; MUST be executed in context of package to be programmed
VOID
	EFIAPI
		ProgramPackage(VOID)
{
	// validate package can be programmed
	if (IsValidPackage())
	{
		// FVIR Configuration
		SetFIVRConfiguration();

		// OC Lock Bit
		if (CPU_SET_OC_LOCK == TRUE)
		{
			FinalizeProgramming();
		}
	}

	return;
}

VOID
	EFIAPI
		SetFIVRConfiguration(VOID)
{
	// IA Core Adaptive Mode offset voltage
	if (System->Package.IACore.OffsetVoltage != _DYNAMIC_FVID)
	{
		ProgramBuffer = OC_MB_SET_FVIDS_RATIOS | System->Package.IACore.OffsetVoltage | OC_MB_DOMAIN_IACORE | OC_MB_COMMAND_EXEC;

		OC_MAILBOX_DISPATCH();
	}

	// GFX Adaptive Mode offset voltage
	if (System->Package.GFX.OffsetVoltage != _DYNAMIC_FVID)
	{
		ProgramBuffer = OC_MB_SET_FVIDS_RATIOS | System->Package.GFX.OffsetVoltage | OC_MB_DOMAIN_GFX | OC_MB_COMMAND_EXEC;

		OC_MAILBOX_DISPATCH();
	}

	// CLR Adaptive Mode voltage offset
	if (System->Package.CLR.OffsetVoltage != _DYNAMIC_FVID)
	{
		ProgramBuffer = OC_MB_SET_FVIDS_RATIOS | System->Package.CLR.OffsetVoltage | OC_MB_DOMAIN_CLR | OC_MB_COMMAND_EXEC;

		OC_MAILBOX_DISPATCH();
	}

	// SA Adaptive Mode voltage offse
	if (System->Package.SA.OffsetVoltage != _DYNAMIC_FVID)
	{
		ProgramBuffer = OC_MB_SET_FVIDS_RATIOS | System->Package.SA.OffsetVoltage | OC_MB_DOMAIN_SA | OC_MB_COMMAND_EXEC;

		OC_MAILBOX_DISPATCH();
	}

	// AIO Adaptive Mode voltage offse
	if (System->Package.AIO.OffsetVoltage != _DYNAMIC_FVID)
	{
		ProgramBuffer = OC_MB_SET_FVIDS_RATIOS | System->Package.AIO.OffsetVoltage | OC_MB_DOMAIN_AIO | OC_MB_COMMAND_EXEC;

		OC_MAILBOX_DISPATCH();
	}

	// FIRV Faults
	if (FIVR_FAULTS_OVRD_EN == TRUE)
	{
		ProgramBuffer = OC_MB_FIVR_FAULTS_OVRD_EN | OC_MB_SET_FIVR_PARAMS | OC_MB_DOMAIN_IACORE | OC_MB_COMMAND_EXEC;

		OC_MAILBOX_DISPATCH();
	}

	// FIVR Efficiency Mode
	if (FIVR_EFF_MODE_OVRD_EN == TRUE)
	{
		ProgramBuffer = OC_MB_FIVR_EFF_MODE_OVRD_EN | OC_MB_SET_FIVR_PARAMS | OC_MB_DOMAIN_IACORE | OC_MB_COMMAND_EXEC;

		OC_MAILBOX_DISPATCH();
	}

	// Fixed VCCIN, Dynamic SVID Control (PowerCut)
	if ((CPU_SET_FIXED_VCCIN == TRUE) || (FIVR_DYN_SVID_CONTROL_DIS == TRUE))
	{
		ProgramBuffer = OC_MB_SET_SVID_PARAMS | System->Package.InputVoltage | OC_MB_DOMAIN_IACORE | OC_MB_COMMAND_EXEC;

		// fail safe in case of compile with no data set
		if (System->Package.InputVoltage == _DYNAMIC_SVID)
		{
			ProgramBuffer |= _DYNAMIC_SVID;
		}

		// Dynamic SVID Control (PowerCut)
		if (FIVR_DYN_SVID_CONTROL_DIS == TRUE)
		{
			ProgramBuffer |= OC_MB_FIVR_DYN_SVID_CONTROL_DIS;
		}

		OC_MAILBOX_DISPATCH();
	}

	OC_MAILBOX_CLEANUP();

	return;
}

// prevents any changes to write-once MSR values once set (reboot required to clear)
VOID
	EFIAPI
		FinalizeProgramming(VOID)
{
	ResponseBuffer = AsmReadMsr64(
		MSR_FLEX_RATIO);

	ProgramBuffer = ResponseBuffer | MSR_FLEX_RATIO_OC_LOCK_BIT;

	AsmWriteMsr64(
		MSR_FLEX_RATIO,
		ProgramBuffer);

	return;
}

VOID
	EFIAPI
		OC_MAILBOX_REQUEST(
			VOID)
{
	// read from mailbox
	ResponseBuffer = AsmReadMsr64(
		MSR_OC_MAILBOX);

	return;
}

VOID
	EFIAPI
		OC_MAILBOX_DISPATCH(
			VOID)
{
	// write to mailbox
	AsmWriteMsr64(
		MSR_OC_MAILBOX,
		ProgramBuffer);

	// retrieve immediate response from mailbox
	ResponseBuffer = AsmReadMsr64(
		MSR_OC_MAILBOX);

	return;
}

BOOLEAN
EFIAPI
OC_MAILBOX_CLEANUP(VOID)
{
	// special case where response indicates reboot is required for setting to take effect
	if (((ResponseBuffer >> 32) & 0xFF) == OC_MB_REBOOT_REQUIRED)
	{
		System->RebootRequired = TRUE;

		return FALSE;
	}

	// if non-zero there was an error
	if (((ResponseBuffer >> 32) & 0xFF) != OC_MB_SUCCESS)
	{
		return TRUE;
	}

	return FALSE;
}