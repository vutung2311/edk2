/** @file
UEFI application to force overclock feature enable on Alienware 13 R3

Copyright (c) 2017, Tung Vu. All rights reserved.
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiApplicationEntryPoint.h>


/**
  as the real entry point for the application.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.  
  @param[in] SystemTable    A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
	EFI_STATUS Status = EFI_SUCCESS;
	UINTN Offset = (UINTN)0x66F;
	UINT8 Value = (UINT8)0x1;
	EFI_GUID SetupVariableGuid = { 0xEC87D643, 0xEBA4, 0x4BB5, {0xA1, 0xE5, 0x3F, 0x3E, 0x36, 0xB2, 0x0D, 0xA9}}; // AMI Setup
	UINT32 SetupVariableAttributes;
	UINT8* SetupVariableBuffer;
	UINTN SetupVariableSize = 0;

	// Reading Setup variable
	Status = gRT->GetVariable(L"Setup", &SetupVariableGuid, NULL, &SetupVariableSize, NULL);
	if (Status == EFI_BUFFER_TOO_SMALL) {
		Status = gBS->AllocatePool(EfiBootServicesData, SetupVariableSize, &SetupVariableBuffer);
		if (EFI_ERROR(Status)) {
			Print(L"gBS->AllocatePool call returned %r.\n", Status);
			return Status;
		}

		Status = gRT->GetVariable(L"Setup", &SetupVariableGuid, &SetupVariableAttributes, &SetupVariableSize, SetupVariableBuffer);
		if (EFI_ERROR(Status)) {
			Print(L"Second gRT->GetVariable call returned %r.\n", Status);
			return Status;
		}
	} else {
		Print(L"First gRT->GetVariable call returned %r.\n", Status);
		return Status;
	}
	Print(L"NVRAM variable \"Setup\" read, data size 0x%X, attributes 0x%X.\n", SetupVariableSize, SetupVariableAttributes);
  
	// Change current value at Offset to Value
	if (Offset >= SetupVariableSize) {
		Print(L"Offset value 0x%X is greater than variable size 0x%X, aborting.\n", Offset, SetupVariableSize);
		return EFI_BUFFER_TOO_SMALL;
	}
	Print(L"Current Setup[0x%X] is 0x%X\n", Offset ,SetupVariableBuffer[Offset]);
	SetupVariableBuffer[Offset] = Value;

	// Writing the modified variable
	Status = gRT->SetVariable(L"Setup", &SetupVariableGuid, SetupVariableAttributes, SetupVariableSize, SetupVariableBuffer);
	if (EFI_ERROR(Status)) {
		Print(L"gRT->SetVariable call returned %r.\n", Status);
		return Status;
	}
	Print(L"NVRAM variable \"Setup\" written.\n");

	// Reading the variable again
	Status = gRT->GetVariable(L"Setup", &SetupVariableGuid, &SetupVariableAttributes, &SetupVariableSize, SetupVariableBuffer);
	if (EFI_ERROR(Status)) {
		Print(L"Third gRT->GetVariable call returned %r.\n", Status);
		return Status;
	}
	Print(L"Updated Setup[0x%X] is 0x%X\n", Offset, SetupVariableBuffer[Offset]);

	return EFI_SUCCESS;
}