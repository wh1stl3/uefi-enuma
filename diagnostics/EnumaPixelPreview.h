// SPDX-License-Identifier: BSD-2-Clause-Patent
// R-005 proposal: callers must obtain pixel-write approval before execution.
#ifndef ENUMA_PIXEL_PREVIEW_H
#define ENUMA_PIXEL_PREVIEW_H
#include "EnumaScanoutDescriptor.h"
#define ENUMA_PIXEL_WIDTH 128U
#define ENUMA_PIXEL_HEIGHT 8U
#define ENUMA_PIXEL_COUNT (ENUMA_PIXEL_WIDTH * ENUMA_PIXEL_HEIGHT)
#define ENUMA_PIXEL_DELAY_US 500000U
typedef EFI_STATUS (*ENUMA_PIXEL_SNAPSHOT)(ENUMA_SCANOUT_DESCRIPTOR *Descriptor);
typedef UINT32 (*ENUMA_PIXEL_READ)(UINTN Address);
typedef VOID (*ENUMA_PIXEL_WRITE)(UINTN Address, UINT32 Value);
typedef VOID (*ENUMA_PIXEL_SYNC)(UINTN Address, UINTN Bytes);
typedef EFI_STATUS (*ENUMA_PIXEL_WAIT)(UINTN Microseconds);

STATIC inline BOOLEAN EnumaPixelDescriptorBounded (CONST ENUMA_SCANOUT_DESCRIPTOR *D)
{
  if (D->Base < 0x9c000000ULL || D->Base >= 0x9e300000ULL || (D->Base & 3) != 0 ||
      D->Width < ENUMA_PIXEL_WIDTH || D->Width > 0xffffU ||
      D->Height < ENUMA_PIXEL_HEIGHT || D->Height > 0xffffU ||
      D->Pitch == 0 || D->Pitch > 0xffffU || (D->Pitch & 3) != 0 ||
      D->X > 0xffffU || D->Y > 0xffffU ||
      (UINT64)D->Pitch < ((UINT64)D->X + D->Width) * 4) { return FALSE; }
  return D->Start == D->Base + (UINT64)D->Y * D->Pitch + (UINT64)D->X * 4 &&
    D->End == D->Base + ((UINT64)D->Y + D->Height - 1) * D->Pitch + ((UINT64)D->X + D->Width) * 4 &&
    D->Start >= D->Base && D->End > D->Start && D->End <= 0x9e300000ULL;
}
STATIC inline BOOLEAN EnumaPixelDescriptorEqual (CONST ENUMA_SCANOUT_DESCRIPTOR *A, CONST ENUMA_SCANOUT_DESCRIPTOR *B)
{
  return A->Base == B->Base && A->Start == B->Start && A->End == B->End &&
    A->Width == B->Width && A->Height == B->Height && A->Pitch == B->Pitch &&
    A->X == B->X && A->Y == B->Y && A->Source == B->Source;
}
// Scratch is caller-owned, disjoint from the entire reserved display interval.
// Normal/recoverable paths restore and verify every saved word. A CPU exception
// or hung callback can prevent restoration; this is not a watchdog/exception guard.
STATIC inline EFI_STATUS EnumaPixelPreview (
  ENUMA_PIXEL_SNAPSHOT Snapshot, ENUMA_PIXEL_READ Read, ENUMA_PIXEL_WRITE Write,
  ENUMA_PIXEL_SYNC Sync, ENUMA_PIXEL_WAIT Wait, UINT32 *Scratch, UINTN ScratchPixels)
{
  ENUMA_SCANOUT_DESCRIPTOR Before, After;
  EFI_STATUS Status;
  UINTN Row, Column, N, ScratchBase = (UINTN)Scratch;
  if (Snapshot == NULL || Read == NULL || Write == NULL || Sync == NULL || Wait == NULL ||
      Scratch == NULL || ScratchPixels < ENUMA_PIXEL_COUNT ||
      ScratchBase > MAX_UINTN - ENUMA_PIXEL_COUNT * sizeof (UINT32) ||
      (ScratchBase < 0x9e300000U && ScratchBase + ENUMA_PIXEL_COUNT * sizeof (UINT32) > 0x9c000000U)) {
    return EFI_INVALID_PARAMETER;
  }
  Status = Snapshot (&Before);
  if (EFI_ERROR (Status)) { return Status; }
  if (!EnumaPixelDescriptorBounded (&Before)) { return EFI_UNSUPPORTED; }
  N = 0;
  for (Row = 0; Row < ENUMA_PIXEL_HEIGHT; Row++) {
    for (Column = 0; Column < ENUMA_PIXEL_WIDTH; Column++) {
      Scratch[N++] = Read ((UINTN)(Before.Start + Row * (UINT64)Before.Pitch + Column * 4));
    }
  }
  Status = Snapshot (&After);
  if (EFI_ERROR (Status)) { return Status; }
  if (!EnumaPixelDescriptorBounded (&After) || !EnumaPixelDescriptorEqual (&Before, &After)) {
    return EFI_MEDIA_CHANGED;
  }
  for (Row = 0; Row < ENUMA_PIXEL_HEIGHT; Row++) {
    UINTN Address = (UINTN)(Before.Start + Row * (UINT64)Before.Pitch);
    for (Column = 0; Column < ENUMA_PIXEL_WIDTH; Column++) { Write (Address + Column * 4, 0xffffffffU); }
    Sync (Address, ENUMA_PIXEL_WIDTH * sizeof (UINT32));
  }
  Status = Wait (ENUMA_PIXEL_DELAY_US);
  N = 0;
  for (Row = 0; Row < ENUMA_PIXEL_HEIGHT; Row++) {
    UINTN Address = (UINTN)(Before.Start + Row * (UINT64)Before.Pitch);
    for (Column = 0; Column < ENUMA_PIXEL_WIDTH; Column++) { Write (Address + Column * 4, Scratch[N++]); }
    Sync (Address, ENUMA_PIXEL_WIDTH * sizeof (UINT32));
  }
  N = 0;
  for (Row = 0; Row < ENUMA_PIXEL_HEIGHT; Row++) {
    for (Column = 0; Column < ENUMA_PIXEL_WIDTH; Column++) {
      if (Read ((UINTN)(Before.Start + Row * (UINT64)Before.Pitch + Column * 4)) != Scratch[N]) {
        Status = EFI_DEVICE_ERROR;
      }
      N++;
    }
  }
  return Status;
}
#endif
