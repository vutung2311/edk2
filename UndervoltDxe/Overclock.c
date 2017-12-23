#include <Library/BaseLib.h>

#include "Overclock.h"

VOID
	EFIAPI
		SetFIVRConfiguration(VOID)
{
	UINT64 ProgramBuffer = 0;
	UINT64 ResponseBuffer = 0;

	// IA Core Adaptive Mode offset voltage
	ProgramBuffer = OC_MB_SET_FVIDS_RATIOS | IACORE_ADAPTIVE_OFFSET | OC_MB_DOMAIN_IACORE | OC_MB_COMMAND_EXEC;
	ResponseBuffer = OC_MAILBOX_DISPATCH(ProgramBuffer);

	// GFX Adaptive Mode offset voltage
	ProgramBuffer = OC_MB_SET_FVIDS_RATIOS | GFX_ADAPTIVE_OFFSET | OC_MB_DOMAIN_GFX | OC_MB_COMMAND_EXEC;
	ResponseBuffer = OC_MAILBOX_DISPATCH(ProgramBuffer);

	// CLR Adaptive Mode voltage offset
	ProgramBuffer = OC_MB_SET_FVIDS_RATIOS | CLR_ADAPTIVE_OFFSET | OC_MB_DOMAIN_CLR | OC_MB_COMMAND_EXEC;
	ResponseBuffer = OC_MAILBOX_DISPATCH(ProgramBuffer);

	// SA Adaptive Mode voltage offse
	ProgramBuffer = OC_MB_SET_FVIDS_RATIOS | SA_ADAPTIVE_OFFSET | OC_MB_DOMAIN_SA | OC_MB_COMMAND_EXEC;
	ResponseBuffer = OC_MAILBOX_DISPATCH(ProgramBuffer);

	// AIO Adaptive Mode voltage offse
	ProgramBuffer = OC_MB_SET_FVIDS_RATIOS | AIO_ADAPTIVE_OFFSET | OC_MB_DOMAIN_AIO | OC_MB_COMMAND_EXEC;
	ResponseBuffer = OC_MAILBOX_DISPATCH(ProgramBuffer);

	// FIRV Faults
	if (FIVR_FAULTS_OVRD_EN == TRUE)
	{
		ProgramBuffer = OC_MB_FIVR_FAULTS_OVRD_EN | OC_MB_SET_FIVR_PARAMS | OC_MB_DOMAIN_IACORE | OC_MB_COMMAND_EXEC;
		ResponseBuffer = OC_MAILBOX_DISPATCH(ProgramBuffer);
	}

	// FIVR Efficiency Mode
	if (FIVR_EFF_MODE_OVRD_EN == TRUE)
	{
		ProgramBuffer = OC_MB_FIVR_EFF_MODE_OVRD_EN | OC_MB_SET_FIVR_PARAMS | OC_MB_DOMAIN_IACORE | OC_MB_COMMAND_EXEC;
		ResponseBuffer = OC_MAILBOX_DISPATCH(ProgramBuffer);
	}

	// Fixed VCCIN, Dynamic SVID Control (PowerCut)
	if ((CPU_SET_FIXED_VCCIN == TRUE) || (FIVR_DYN_SVID_CONTROL_DIS == TRUE))
	{
		ProgramBuffer = OC_MB_SET_SVID_PARAMS | SVID_FIXED_VCCIN | OC_MB_DOMAIN_IACORE | OC_MB_COMMAND_EXEC;

		// fail safe in case of compile with no data set
		if (SVID_FIXED_VCCIN == _DYNAMIC_SVID)
		{
			ProgramBuffer |= _DYNAMIC_SVID;
		}

		// Dynamic SVID Control (PowerCut)
		if (FIVR_DYN_SVID_CONTROL_DIS == TRUE)
		{
			ProgramBuffer |= OC_MB_FIVR_DYN_SVID_CONTROL_DIS;
		}
		ResponseBuffer = OC_MAILBOX_DISPATCH(ProgramBuffer);
	}

	OC_MAILBOX_CLEANUP(ResponseBuffer);

	return;
}

// prevents any changes to write-once MSR values once set (reboot required to clear)
VOID
	EFIAPI
		FinalizeProgramming(VOID)
{
	UINT64 ResponseBuffer = AsmReadMsr64(MSR_FLEX_RATIO);

	AsmWriteMsr64(MSR_FLEX_RATIO, ResponseBuffer | MSR_FLEX_RATIO_OC_LOCK_BIT);

	return;
}

UINT64
EFIAPI
OC_MAILBOX_REQUEST(
	VOID)
{
	// read from mailbox
	return AsmReadMsr64(MSR_OC_MAILBOX);
}

UINT64
EFIAPI
OC_MAILBOX_DISPATCH(
	UINT64 ProgramBuffer)
{
	// write to mailbox
	AsmWriteMsr64(MSR_OC_MAILBOX, ProgramBuffer);

	// retrieve immediate response from mailbox
	return AsmReadMsr64(MSR_OC_MAILBOX);
}

BOOLEAN
EFIAPI
OC_MAILBOX_CLEANUP(UINT64 ResponseBuffer)
{
	// special case where response indicates reboot is required for setting to take effect
	if (((ResponseBuffer >> 32) & 0xFF) == OC_MB_REBOOT_REQUIRED)
	{
		RebootRequired = TRUE;

		return FALSE;
	}

	// if non-zero there was an error
	if (((ResponseBuffer >> 32) & 0xFF) != OC_MB_SUCCESS)
	{
		return TRUE;
	}

	return FALSE;
}

BOOLEAN
EFIAPI
IsValidPackage(VOID)
{
	// check OC Lock Bit not set
	UINT64 ResponseBuffer = AsmReadMsr64(MSR_FLEX_RATIO);

	if ((ResponseBuffer & MSR_FLEX_RATIO_OC_LOCK_BIT) == MSR_FLEX_RATIO_OC_LOCK_BIT)
	{
		return FALSE;
	}

	return TRUE;
}

// programs processor package; MUST be executed in context of package to be programmed
VOID
	EFIAPI
		ProgramPackage(VOID)
{
	// FVIR Configuration
	SetFIVRConfiguration();

	// OC Lock Bit
	if (CPU_SET_OC_LOCK == TRUE)
	{
		FinalizeProgramming();
	}

	return;
}