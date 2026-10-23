// SPDX-License-Identifier: BSD-2-Clause-Patent
// Decode a bounded descriptor hypothesis. No pixels, activity or IOVA proof.
#ifndef ENUMA_SCANOUT_DESCRIPTOR_H
#define ENUMA_SCANOUT_DESCRIPTOR_H
#include "EnumaScanoutAudit.h"
typedef struct {
  UINT64 Base, Start, End;
  UINT32 Width, Height, Pitch, X, Y;
  UINTN Source;
} ENUMA_SCANOUT_DESCRIPTOR;

// Failure leaves Out unchanged. The caller must check the returned status.
STATIC inline EFI_STATUS
EnumaScanoutDecodeDescriptor (
  UINT32 Base, UINT32 Size, UINT32 RawPitch, UINT32 XY,
  UINT32 Format, UINT32 Operation, ENUMA_SCANOUT_DESCRIPTOR *Out)
{
  UINT32 Width = Size & 0xffffU, Height = Size >> 16;
  UINT32 Pitch = RawPitch & 0xffffU, X = XY & 0xffffU, Y = XY >> 16;
  UINT64 Start, End;
  if (Out == NULL) { return EFI_INVALID_PARAMETER; }
  if (Base < 0x9c000000U || Base >= 0x9e300000U || (Base & 3U) != 0 ||
      Width == 0 || Height == 0 || Pitch == 0 || (Pitch & 3U) != 0 ||
      (UINT64)Pitch < ((UINT64)X + Width) * 4 ||
      ((Format >> 9) & 3) != 3 || (Format >> 30) != 0 ||
      ((Format >> 19) & 3) != 0 || (Format & 0x3f) != 0x3f ||
      ((Format >> 23) & 3) != 0 ||
      (Format & ((1U<<22)|(1U<<15)|(1U<<14)|(1U<<11))) != 0 || (Operation & 1) != 0) {
    return EFI_UNSUPPORTED;
  }
  // All factors above are 16-bit fields, so these UINT64 sums cannot overflow.
  Start = (UINT64)Base + (UINT64)Y * Pitch + (UINT64)X * 4;
  End = (UINT64)Base + ((UINT64)Y + Height - 1) * Pitch + ((UINT64)X + Width) * 4;
  if (Start < Base || End <= Start || End > 0x9e300000ULL) { return EFI_UNSUPPORTED; }
  Out->Base = Base; Out->Start = Start; Out->End = End;
  Out->Width = Width; Out->Height = Height; Out->Pitch = Pitch;
  Out->X = X; Out->Y = Y; Out->Source = 0;
  return EFI_SUCCESS;
}

// Step12: first individually bounded descriptor, at most 48 register reads.
// This does not certify other candidates, uniqueness, activity, physical/IOVA
// identity or channel order. No physical pixel address is dereferenced.
STATIC inline EFI_STATUS
EnumaScanoutFindBoundedDescriptor (ENUMA_SCANOUT_READ32 Read, ENUMA_SCANOUT_DESCRIPTOR *Out)
{
  CONST UINT32 Pipes[8] = {0x5000,0x7000,0x9000,0xb000,0x25000,0x27000,0x29000,0x2b000};
  UINTN I;
  if (Read == NULL || Out == NULL) { return EFI_INVALID_PARAMETER; }
  for (I = 0; I < 8; I++) {
    UINTN P = 0x0ae00000U + Pipes[I];
    UINT32 Base = Read(P + 0x14);
    if (Base >= 0x9c000000U && Base < 0x9e300000U) {
      UINT32 Size = Read(P), Pitch = Read(P + 0x24), XY = Read(P + 0x08);
      UINT32 Format = Read(P + 0x30), Operation = Read(P + 0x38);
      if (!EFI_ERROR (EnumaScanoutDecodeDescriptor (Base, Size, Pitch, XY, Format, Operation, Out))) {
        Out->Source = P;
        return EFI_SUCCESS;
      }
    }
  }
  return EFI_UNSUPPORTED;
}
#endif
