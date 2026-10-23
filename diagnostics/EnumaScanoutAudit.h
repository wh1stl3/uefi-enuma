// SPDX-License-Identifier: BSD-2-Clause-Patent
// R-004: read-only source-descriptor predicate, not proof a pipe is active.
#ifndef ENUMA_SCANOUT_AUDIT_H
#define ENUMA_SCANOUT_AUDIT_H
#include <Uefi.h>
typedef UINT32 (*ENUMA_SCANOUT_READ32)(UINTN Address);
STATIC EFI_STATUS
EnumaScanoutCheck (ENUMA_SCANOUT_READ32 Read)
{
  CONST UINT32 Pipes[8] = {0x5000,0x7000,0x9000,0xb000,0x25000,0x27000,0x29000,0x2b000};
  UINTN I, Matches = 0, Valid = 0;
  if (Read == NULL) { return EFI_INVALID_PARAMETER; }
  for (I = 0; I < 8; I++) {
    UINTN P = 0x0ae00000U + Pipes[I];
    UINT32 Base = Read(P + 0x14);
    if (Base >= 0x9c000000U && Base < 0x9e300000U) {
      UINT32 Size = Read(P), Pitch = Read(P + 0x24);
      UINT32 Format = Read(P + 0x30), Operation = Read(P + 0x38);
      UINT32 XY = Read(P + 0x08), Extra1 = Read(P + 0x18);
      UINT32 Extra2 = Read(P + 0x1c), Extra3 = Read(P + 0x20);
      UINT64 Span = (UINT64)6400 * 2560;
      Matches++;
      // Exact linear, single-plane 4-byte descriptor hypothesis. Refuse
      // crop, rotation, solid fill, compression and secondary plane addresses.
      // Even matching stale descriptors do not establish live scanout ownership.
      if (Size == ((2560U << 16) | 1600U) && Pitch == 6400 &&
          ((Format >> 9) & 3) == 3 && (Format >> 30) == 0 &&
          ((Format >> 19) & 3) == 0 && (Format & ((1U<<11)|(1U<<22))) == 0 &&
          (Operation & 1) == 0 && XY == 0 && Extra1 == 0 && Extra2 == 0 && Extra3 == 0 &&
          (UINT64)Base + Span <= 0x9e300000ULL) { Valid++; }
    }
  }
  return Matches == 1 && Valid == 1 ? EFI_SUCCESS : EFI_UNSUPPORTED;
}
#endif
