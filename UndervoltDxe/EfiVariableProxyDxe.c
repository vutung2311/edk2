#include <PiDxe.h>

#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiDriverEntryPoint.h>

#include "EfiVariable.h"

EFI_GUID EfiVariableProxyGuid = {0x740ed803, 0xb3f9, 0x4fb9, {0x9c, 0x07, 0x5f, 0x5a, 0xdf, 0xb4, 0x19, 0x0c}};

EFI_GET_VARIABLE OriginalEfiGetVariable;

EFI_STATUS
EFIAPI EfiGetVariableProxy(
    IN CHAR16 *VariableName,
    IN EFI_GUID *VendorGuid,
    OUT UINT32 *Attributes, OPTIONAL IN OUT UINTN *DataSize,
    OUT VOID *Data OPTIONAL)
{
    EFI_STATUS Status;

    Status = OriginalEfiGetVariable(VariableName, VendorGuid, Attributes, DataSize, Data);
    if (EFI_ERROR(Status))
    {
        return Status;
    }

    if (CompareGuid(VendorGuid, &AMISetupVariableGuid) && StrCmp(VariableName, AmiSetupVariableName) == 0)
    {
        ((UINT8 *)Data)[0x66F] = 1;   // Enable Overclock
        ((UINT8 *)Data)[0x675] = 150; // Core Voltage Offset
        ((UINT8 *)Data)[0x677] = 1;   // Core Voltage Offset Prefix
        ((UINT8 *)Data)[0x877] = 150; // Uncore Voltage Offset
        ((UINT8 *)Data)[0x879] = 1;   // Uncore Voltage Offset Prefix
        ((UINT8 *)Data)[0x87F] = 50;  // GT Voltage Offset
        ((UINT8 *)Data)[0x881] = 1;   // GT Voltage Offset Prefix
    }

    return EFI_SUCCESS;
}

// driver main
EFI_STATUS
EFIAPI
EfiVariableProxyDxeEntryPoint(
    IN EFI_HANDLE ImageHandle,
    IN EFI_SYSTEM_TABLE *SystemTable)
{
    EFI_HANDLE Handle = NULL;

    OriginalEfiGetVariable = SystemTable->RuntimeServices->GetVariable;
    SystemTable->RuntimeServices->GetVariable = EfiGetVariableProxy;

    return SystemTable->BootServices->InstallProtocolInterface(
        &Handle,
        &EfiVariableProxyGuid,
        EFI_NATIVE_INTERFACE,
        NULL);
}