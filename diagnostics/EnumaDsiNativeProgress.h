// SPDX-License-Identifier: BSD-2-Clause-Patent
// Follow the pinned Xiaomi v1 status read path; no register writes.
#ifndef ENUMA_DSI_NATIVE_PROGRESS_H
#define ENUMA_DSI_NATIVE_PROGRESS_H
#include "EnumaDsiCounters.h"
STATIC inline EFI_STATUS
EnumaDsiNativeActive (ENUMA_SCANOUT_READ32 Read)
{
  UINT32 Version, A, B;
  if (Read == NULL) { return EFI_INVALID_PARAMETER; }
  Version = Read (0x0ae00000U);
  if ((Version >> 28) != 6) { return EFI_UNSUPPORTED; }
  A = Read (0x0ae6ba6cU) & 1U;
  B = Read (0x0ae6c26cU) & 1U;
  return (A | B) != 0 ? EFI_SUCCESS : EFI_NOT_READY;
}
// Step18: <=26 reads, <=two existing Stall(100000) calls. At least one
// originally active interface must advance without wrap in BOTH windows.
STATIC inline EFI_STATUS
EnumaDsiNativeProgress (ENUMA_SCANOUT_READ32 Read, ENUMA_DSI_WAIT Wait)
{
  CONST UINTN Bases[2] = {0x0ae6b800U, 0x0ae6c000U};
  ENUMA_DSI_COUNTER_STATE Before[2];
  UINT32 Version, Mask = 0, Moving = 3;
  UINTN I, Window;
  EFI_STATUS Status;
  if (Read == NULL || Wait == NULL) { return EFI_INVALID_PARAMETER; }
  Version = Read (0x0ae00000U);
  if ((Version >> 28) != 6) { return EFI_UNSUPPORTED; }
  for (I = 0; I < 2; I++) {
    UINT32 Active = Read (Bases[I] + 0x26c) & 1U;
    Before[I].Timing = Read (Bases[I]);
    Before[I].Enable = Read (Bases[I] + 0xa8);
    if (Active != 0) { Mask |= 1U << I; }
  }
  if (Mask == 0) { return EFI_NOT_READY; }
  for (I = 0; I < 2; I++) {
    if ((Mask & (1U << I)) != 0) { Before[I].Frame = Read (Bases[I] + 0xac); }
  }
  Moving = Mask;
  for (Window = 0; Window < 2; Window++) {
    Status = Wait (ENUMA_DSI_SAMPLE_US);
    if (Status != EFI_SUCCESS) { return EFI_ERROR (Status) ? Status : EFI_NOT_READY; }
    for (I = 0; I < 2; I++) {
      if ((Mask & (1U << I)) != 0) {
        UINT32 Frame = Read (Bases[I] + 0xac);
        UINT32 Active = Read (Bases[I] + 0x26c) & 1U;
        UINT32 Timing = Read (Bases[I]);
        UINT32 Enable = Read (Bases[I] + 0xa8);
        if (Active == 0 || Timing != Before[I].Timing || Enable != Before[I].Enable ||
            Frame < Before[I].Frame || Frame - Before[I].Frame > ENUMA_DSI_MAX_FRAME_DELTA) {
          return EFI_NOT_READY;
        }
        if (Frame == Before[I].Frame) { Moving &= ~(1U << I); }
        Before[I].Frame = Frame;
      }
    }
  }
  if (Read (0x0ae00000U) != Version) { return EFI_NOT_READY; }
  return Moving != 0 ? EFI_SUCCESS : EFI_NOT_READY;
}
#endif
