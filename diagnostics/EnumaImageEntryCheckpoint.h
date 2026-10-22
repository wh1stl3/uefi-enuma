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
#endif
