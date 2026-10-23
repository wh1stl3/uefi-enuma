// SPDX-License-Identifier: BSD-2-Clause-Patent
// R-004: read-only source-descriptor predicate, not proof a pipe is active.
#ifndef ENUMA_SCANOUT_AUDIT_H
#define ENUMA_SCANOUT_AUDIT_H
#include <Uefi.h>
typedef UINT32 (*ENUMA_SCANOUT_READ32)(UINTN Address);
// Step9: inspect source-address registers only; no geometry/format reads.
STATIC inline EFI_STATUS
EnumaScanoutAddressPresent (ENUMA_SCANOUT_READ32 Read)
{
  CONST UINT32 Pipes[8] = {0x5000,0x7000,0x9000,0xb000,0x25000,0x27000,0x29000,0x2b000};
  UINTN I;
  if (Read == NULL) { return EFI_INVALID_PARAMETER; }
  for (I = 0; I < 8; I++) {
    UINT32 Base = Read(0x0ae00000U + Pipes[I] + 0x14);
    if (Base >= 0x9c000000U && Base < 0x9e300000U) { return EFI_SUCCESS; }
  }
  return EFI_UNSUPPORTED;
}
// Step10: classify candidate packing only; not active-pipe/geometry proof.
STATIC inline EFI_STATUS
EnumaScanoutLinearCandidates (ENUMA_SCANOUT_READ32 Read)
{
  CONST UINT32 Pipes[8] = {0x5000,0x7000,0x9000,0xb000,0x25000,0x27000,0x29000,0x2b000};
  UINTN I, Matches = 0;
  if (Read == NULL) { return EFI_INVALID_PARAMETER; }
  for (I = 0; I < 8; I++) {
    UINTN P = 0x0ae00000U + Pipes[I];
    UINT32 Base = Read(P + 0x14);
    if (Base >= 0x9c000000U && Base < 0x9e300000U) {
      UINT32 Format = Read(P + 0x30), Operation = Read(P + 0x38);
      Matches++;
      if (((Format >> 9) & 3) != 3 || (Format >> 30) != 0 ||
          ((Format >> 19) & 3) != 0 || (Format & 0x3f) != 0x3f ||
          ((Format >> 23) & 3) != 0 ||
          (Format & ((1U<<22)|(1U<<15)|(1U<<14)|(1U<<11))) != 0 || (Operation & 1) != 0) {
        return EFI_UNSUPPORTED;
      }
    }
  }
  return Matches != 0 ? EFI_SUCCESS : EFI_UNSUPPORTED;
}
STATIC inline EFI_STATUS
EnumaScanoutGeometryCandidates (ENUMA_SCANOUT_READ32 Read)
{
  CONST UINT32 Pipes[8] = {0x5000,0x7000,0x9000,0xb000,0x25000,0x27000,0x29000,0x2b000};
  UINTN I, Matches = 0;
  if (Read == NULL) { return EFI_INVALID_PARAMETER; }
  for (I = 0; I < 8; I++) {
    UINTN P = 0x0ae00000U + Pipes[I];
    UINT32 Base = Read(P + 0x14);
    if (Base >= 0x9c000000U && Base < 0x9e300000U) {
      UINT32 Size = Read(P), Pitch = Read(P + 0x24);
      Matches++;
      // Step11: geometry/span hypothesis only, at most 24 reads. Format,
      // crop, secondary planes, uniqueness and activity remain unproven.
      if (Size != ((2560U << 16) | 1600U) || Pitch != 6400 ||
          (Base & 3U) != 0 || (UINT64)Base + 6400ULL * 2560 > 0x9e300000ULL) {
        return EFI_UNSUPPORTED;
      }
    }
  }
  return Matches != 0 ? EFI_SUCCESS : EFI_UNSUPPORTED;
}
STATIC inline EFI_STATUS
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
