#include <PiDxe.h>

#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>

#include "SystemInformation.h"
#include "Overclock.h"

// driver main
EFI_STATUS
EFIAPI
UndervoltDxeEntryPoint(
	IN EFI_HANDLE ImageHandle,
	IN EFI_SYSTEM_TABLE *SystemTable)
{
	EFI_STATUS Status;

	// driver init
	Status = gST->ConOut->OutputString(
		gST->ConOut,
		L"Intel(R) Core(TM) i7-7700HQ UEFI Undervolt DXE driver\r\n\0");
	if (EFI_ERROR(Status))
	{
		return Status;
	}

	Print(
		L"Build %s %s\r\n\0",
		BUILD_RELEASE_VER,
		BUILD_TARGET_CPU_DESC);

	// initialize system data
	Status = InitializeSystem();
	if (EFI_ERROR(Status))
	{
		gST->ConOut->OutputString(
			gST->ConOut,
			L"!!! Failed to initialize system !!!\r\n\0");
		return Status;
	}

	// initialize platform data
	Status = GatherPlatformInfo(&System->Platform);
	if (EFI_ERROR(Status))
	{
		gST->ConOut->OutputString(
			gST->ConOut,
			L"!!! Failed to get platform data !!!\r\n\0");
		return Status;
	}

	// enumerate processors and gather processor data
	Status = EnumeratePackage();
	if (EFI_ERROR(Status))
	{
		gST->ConOut->OutputString(
			gST->ConOut,
			L"!!! Failed to gather processor data !!!\r\n\0");
		return Status;
	}

	// validate package can be programmed
	if (!IsValidPackage())
	{
		gST->ConOut->OutputString(
			gST->ConOut,
			L"!!! CPU is not valid. Aborting !!!\r\n\0");
		return EFI_NOT_READY;
	}

	ProgramPackage();

	// notify if system reboot required
	if (RebootRequired == TRUE)
	{
		gST->ConOut->OutputString(
			gST->ConOut,
			L"!!! REBOOT REQUIRED FOR SOME SETTINGS TO TAKE EFFECT !!!\r\n\0");

		gRT->ResetSystem(EfiResetWarm, EFI_SUCCESS, 0, NULL);
		CpuDeadLoop();
	}

	gST->ConOut->OutputString(
		gST->ConOut,
		L"!!! Undervolt applied  !!!\r\n\0");

	// always return success, no cleanup as everything is automatically destroyed
	return EFI_SUCCESS;
}