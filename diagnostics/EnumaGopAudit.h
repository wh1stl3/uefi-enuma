// SPDX-License-Identifier: BSD-2-Clause-Patent
// Read protocol metadata only. Never reads or writes the physical framebuffer.
#ifndef ENUMA_GOP_AUDIT_H
#define ENUMA_GOP_AUDIT_H
#include <Uefi.h>
#include <Protocol/GraphicsOutput.h>

STATIC BOOLEAN
EnumaGopInfoEqual (
  CONST EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *A,
  CONST EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *B
  )
{
  return A->Version == B->Version &&
    A->HorizontalResolution == B->HorizontalResolution &&
    A->VerticalResolution == B->VerticalResolution &&
    A->PixelFormat == B->PixelFormat &&
    A->PixelsPerScanLine == B->PixelsPerScanLine &&
    A->PixelInformation.RedMask == B->PixelInformation.RedMask &&
    A->PixelInformation.GreenMask == B->PixelInformation.GreenMask &&
    A->PixelInformation.BlueMask == B->PixelInformation.BlueMask &&
    A->PixelInformation.ReservedMask == B->PixelInformation.ReservedMask;
}

STATIC EFI_STATUS
EnumaGopAudit (
  EFI_BOOT_SERVICES *Services,
  EFI_HANDLE ExpectedHandle,
  EFI_GRAPHICS_OUTPUT_PROTOCOL *ExpectedGop
  )
{
  EFI_HANDLE *Handles = NULL;
  EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop = NULL;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Query = NULL;
  EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *Mode;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Info;
  EFI_STATUS Status, FreeStatus;
  UINTN Count = 0, Size = 0;
  UINT64 Required;

  if (Services == NULL || ExpectedHandle == NULL || ExpectedGop == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  Status = Services->LocateHandleBuffer (ByProtocol,
    &gEfiGraphicsOutputProtocolGuid, NULL, &Count, &Handles);
  if (EFI_ERROR (Status)) {
    goto Done;
  }
  Status = EFI_COMPROMISED_DATA;
  if (Handles == NULL || Count != 1 || Handles[0] != ExpectedHandle) {
    goto Done;
  }
  Status = Services->HandleProtocol (Handles[0],
    &gEfiGraphicsOutputProtocolGuid, (VOID **)&Gop);
  if (EFI_ERROR (Status)) {
    goto Done;
  }
  Status = EFI_COMPROMISED_DATA;
  if (Gop != ExpectedGop || Gop->Mode == NULL || Gop->QueryMode == NULL) {
    goto Done;
  }
  Mode = Gop->Mode;
  Info = Mode->Info;
  if (Mode->MaxMode != 1 || Mode->Mode != 0 || Info == NULL ||
      Mode->SizeOfInfo != sizeof (*Info) || Info->Version != 0 ||
      Info->HorizontalResolution == 0 || Info->VerticalResolution == 0 ||
      Info->PixelsPerScanLine < Info->HorizontalResolution ||
      Info->PixelFormat != PixelBlueGreenRedReserved8BitPerColor ||
      Mode->FrameBufferBase == 0) {
    goto Done;
  }
  // Multiply in UINT64 with an explicit overflow guard, including 32-bit UINTN.
  Required = (UINT64)Info->PixelsPerScanLine * Info->VerticalResolution;
  if (Required > MAX_UINT64 / 4) {
    goto Done;
  }
  Required *= 4;
  if (Required > Mode->FrameBufferSize ||
      Mode->FrameBufferBase > MAX_UINT64 - Mode->FrameBufferSize) {
    goto Done;
  }
  Status = Gop->QueryMode (Gop, Mode->Mode, &Size, &Query);
  if (EFI_ERROR (Status)) {
    goto Done;
  }
  Status = EFI_COMPROMISED_DATA;
  if (Query != NULL && Size == sizeof (*Query) &&
      EnumaGopInfoEqual (Info, Query)) {
    Status = EFI_SUCCESS;
  }
Done:
  if (Query != NULL) {
    FreeStatus = Services->FreePool (Query);
    if (EFI_ERROR (FreeStatus)) { Status = FreeStatus; }
  }
  if (Handles != NULL) {
    FreeStatus = Services->FreePool (Handles);
    if (EFI_ERROR (FreeStatus)) { Status = FreeStatus; }
  }
  return Status;
}
#endif
