// SPDX-License-Identifier: BSD-2-Clause-Patent
// Read-only counter predicates; no panel, route or physical/IOVA proof.
#ifndef ENUMA_DSI_COUNTERS_H
#define ENUMA_DSI_COUNTERS_H
#include "EnumaScanoutAudit.h"
#define ENUMA_DSI_SAMPLE_US 100000U
#define ENUMA_DSI_MAX_FRAME_DELTA 64U
typedef EFI_STATUS (*ENUMA_DSI_WAIT)(UINTN Microseconds);
typedef struct { UINT32 Timing, Enable, Frame; } ENUMA_DSI_COUNTER_STATE;
STATIC inline UINTN
EnumaDsiCounterEligibility (ENUMA_SCANOUT_READ32 Read, ENUMA_DSI_COUNTER_STATE State[2])
{
  CONST UINTN Bases[2] = {0x0ae6b800U, 0x0ae6c000U};
  UINTN I, Mask = 0;
  for (I = 0; I < 2; I++) {
    State[I].Timing = Read (Bases[I]);
    State[I].Enable = Read (Bases[I] + 0xa8);
    if (State[I].Timing != 0 && (State[I].Enable & 3U) == 3U) { Mask |= (UINTN)1 << I; }
  }
  return Mask;
}
// Step15: four reads, no frame-counter access or wait.
STATIC inline EFI_STATUS
EnumaDsiCountersConfigured (ENUMA_SCANOUT_READ32 Read)
{
  ENUMA_DSI_COUNTER_STATE State[2];
  if (Read == NULL) { return EFI_INVALID_PARAMETER; }
  return EnumaDsiCounterEligibility (Read, State) != 0 ? EFI_SUCCESS : EFI_NOT_READY;
}
// Step16: at most twelve reads and one wait. Modulo-UINT32 delta 1..64
// is compatible with progress, but cannot exclude a reset near wrap.
STATIC inline EFI_STATUS
EnumaDsiCounterProgress (ENUMA_SCANOUT_READ32 Read, ENUMA_DSI_WAIT Wait)
{
  CONST UINTN Bases[2] = {0x0ae6b800U, 0x0ae6c000U};
  ENUMA_DSI_COUNTER_STATE Before[2];
  UINT32 After[2] = {0, 0};
  UINTN I, Mask;
  EFI_STATUS Status;
  BOOLEAN Advanced = FALSE;
  if (Read == NULL || Wait == NULL) { return EFI_INVALID_PARAMETER; }
  Mask = EnumaDsiCounterEligibility (Read, Before);
  if (Mask == 0) { return EFI_NOT_READY; }
  for (I = 0; I < 2; I++) {
    if ((Mask & ((UINTN)1 << I)) != 0) { Before[I].Frame = Read (Bases[I] + 0xac); }
  }
  Status = Wait (ENUMA_DSI_SAMPLE_US);
  if (Status != EFI_SUCCESS) { return EFI_ERROR (Status) ? Status : EFI_NOT_READY; }
  for (I = 0; I < 2; I++) {
    if ((Mask & ((UINTN)1 << I)) != 0) { After[I] = Read (Bases[I] + 0xac); }
  }
  for (I = 0; I < 2; I++) {
    if ((Mask & ((UINTN)1 << I)) != 0) {
      UINT32 Timing = Read (Bases[I]);
      UINT32 Enable = Read (Bases[I] + 0xa8);
      if (Timing != Before[I].Timing || Enable != Before[I].Enable) { return EFI_NOT_READY; }
    }
  }
  for (I = 0; I < 2; I++) {
    if ((Mask & ((UINTN)1 << I)) != 0) {
      UINT32 Delta = (UINT32)(After[I] - Before[I].Frame);
      if (Delta > ENUMA_DSI_MAX_FRAME_DELTA) { return EFI_NOT_READY; }
      if (Delta != 0) { Advanced = TRUE; }
    }
  }
  return Advanced ? EFI_SUCCESS : EFI_NOT_READY;
}
#endif
