// SPDX-License-Identifier: BSD-2-Clause-Patent
// Reuse the original ordinal counter, timer and PSCI reset gate.
#ifndef ENUMA_DISPATCH_DETAIL_CHECKPOINT_H
#define ENUMA_DISPATCH_DETAIL_CHECKPOINT_H
#include "EnumaDispatchCheckpoint.h"
#ifndef ENUMA_DIAG_DETAIL_POINT
#define ENUMA_DIAG_DETAIL_POINT 13
#endif
#ifndef ENUMA_DIAG_GUID_ENABLED
#define ENUMA_DIAG_GUID_ENABLED 0
#endif
#ifndef ENUMA_DIAG_GUID_LOW64
#define ENUMA_DIAG_GUID_LOW64 0
#endif
#ifndef ENUMA_DIAG_GUID_HIGH64
#define ENUMA_DIAG_GUID_HIGH64 0
#endif
#if ENUMA_DIAG_DETAIL_POINT < 13 || ENUMA_DIAG_DETAIL_POINT > 25
#error Unsupported enuma dispatch detail point
#endif
#if ENUMA_DIAG_GUID_ENABLED != 0 && ENUMA_DIAG_GUID_ENABLED != 1
#error Unsupported enuma GUID filter
#endif

STATIC VOID EnumaDispatchDetailCheckpoint (
  UINTN Point,
  CONST EFI_GUID *FileName,
  EFI_STATUS Status
  )
{
  UINTN SelectedPoint;
  (VOID) FileName;
  SelectedPoint = Point;
  // 13: any StartImage return; 14: any LoadImage return; 15: before StartImage.
  // 16/17: failed/successful StartImage; 18/19: failed/successful LoadImage.
  // 20: before the PE entry call; 21: direct PE return; 22: after Exit/LongJump.
  // 23 is a separate global unhandled-exception gate, never called here.
  // 24/25: vendor canary table prefix present/absent before the PE call.
  if (Point == 13 && ENUMA_DIAG_DETAIL_POINT == 16 && EFI_ERROR (Status)) {
    SelectedPoint = 16;
  }
  if (Point == 13 && ENUMA_DIAG_DETAIL_POINT == 17 && !EFI_ERROR (Status)) {
    SelectedPoint = 17;
  }
  if (Point == 14 && ENUMA_DIAG_DETAIL_POINT == 18 && EFI_ERROR (Status)) {
    SelectedPoint = 18;
  }
  if (Point == 14 && ENUMA_DIAG_DETAIL_POINT == 19 && !EFI_ERROR (Status)) {
    SelectedPoint = 19;
  }
  if (ENUMA_DIAG_STAGE != 13 || SelectedPoint != ENUMA_DIAG_DETAIL_POINT ||
      EnumaDispatchOrdinal != ENUMA_DIAG_DISPATCH_ORDINAL) {
    return;
  }
#if ENUMA_DIAG_GUID_ENABLED
  if (FileName == NULL ||
      ReadUnaligned64 ((CONST UINT64 *) FileName) != ENUMA_DIAG_GUID_LOW64 ||
      ReadUnaligned64 ((CONST UINT64 *) ((CONST UINT8 *) FileName + 8)) != ENUMA_DIAG_GUID_HIGH64) {
    return;
  }
#endif
  EnumaStageCheckpoint (13);
}
#endif
