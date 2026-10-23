// SPDX-License-Identifier: BSD-2-Clause-Patent
// Included only by Dispatcher.c: Image.c calls this DxeCore-local bridge.
// CoreDispatcher already rejects recursive dispatch. Nested StartImage on a
// different handle must not impersonate the current scheduled driver.
#ifndef ENUMA_IMAGE_ENTRY_CHECKPOINT_H
#define ENUMA_IMAGE_ENTRY_CHECKPOINT_H
STATIC EFI_HANDLE EnumaStartingImage;
STATIC CONST EFI_GUID *EnumaStartingFile;

VOID EnumaImageEntryCheckpoint (UINTN Point, EFI_HANDLE Handle, EFI_STATUS Status)
{
  if (EnumaStartingImage == NULL || Handle != EnumaStartingImage ||
      EnumaStartingFile == NULL) {
    return;
  }
  EnumaDispatchDetailCheckpoint (Point, EnumaStartingFile, Status);
}

// Read only: match the first 64 GUID bits used by the locked OEM wrapper.
// Do not read or publish VendorTable, seed a canary, or call an RNG/SMC here.
VOID EnumaImageCanaryTableCheckpoint (EFI_HANDLE Handle, EFI_SYSTEM_TABLE *SystemTable)
{
#if ENUMA_DIAG_STAGE == 13 && (ENUMA_DIAG_DETAIL_POINT == 24 || ENUMA_DIAG_DETAIL_POINT == 25)
  UINTN Index;
  UINTN Point;
  if (EnumaStartingImage == NULL || Handle != EnumaStartingImage ||
      EnumaStartingFile == NULL || SystemTable == NULL ||
      SystemTable->NumberOfTableEntries > 4096 ||
      (SystemTable->NumberOfTableEntries != 0 && SystemTable->ConfigurationTable == NULL)) {
    return;
  }
  // Bounded diagnostic walk, not a claim that 4096 is a UEFI limit.
  Point = 25;
  for (Index = 0; Index < SystemTable->NumberOfTableEntries; ++Index) {
    if (ReadUnaligned64 ((CONST UINT64 *) &SystemTable->ConfigurationTable[Index].VendorGuid) ==
        0x40f7080ab898d8dcULL) {
      Point = 24;
      break;
    }
  }
  EnumaImageEntryCheckpoint (Point, Handle, 0);
#else
  (VOID) Handle;
  (VOID) SystemTable;
#endif
}
#endif
