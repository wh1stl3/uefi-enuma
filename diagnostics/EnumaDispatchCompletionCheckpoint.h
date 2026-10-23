// SPDX-License-Identifier: BSD-2-Clause-Patent
// Normal-dispatch completion fallback, NOT an asynchronous watchdog.
#ifndef ENUMA_DISPATCH_COMPLETION_CHECKPOINT_H
#define ENUMA_DISPATCH_COMPLETION_CHECKPOINT_H
#include "EnumaStageCheckpoint.h"
#ifndef ENUMA_DIAG_DISPATCH_FALLBACK
#define ENUMA_DIAG_DISPATCH_FALLBACK 0
#endif
#ifndef ENUMA_DIAG_GUID_ONLY
#define ENUMA_DIAG_GUID_ONLY 0
#endif
#ifndef ENUMA_DIAG_GUID_ENABLED
#define ENUMA_DIAG_GUID_ENABLED 0
#endif
#ifndef ENUMA_DIAG_DETAIL_POINT
#define ENUMA_DIAG_DETAIL_POINT 13
#endif
#if ENUMA_DIAG_DISPATCH_FALLBACK != 0 && ENUMA_DIAG_DISPATCH_FALLBACK != 1
#error Unsupported enuma dispatch fallback
#endif
#if ENUMA_DIAG_DISPATCH_FALLBACK && (ENUMA_DIAG_STAGE != 13 || !ENUMA_DIAG_GUID_ONLY || !ENUMA_DIAG_GUID_ENABLED || ENUMA_DIAG_DETAIL_POINT == 23 || ENUMA_DIAG_DETAIL_POINT >= 26)
#error Dispatch fallback requires a GUID-only stage 13 driver detail
#endif
STATIC VOID EnumaDispatchCompletionCheckpoint (VOID)
{
#if ENUMA_DIAG_DISPATCH_FALLBACK
  // If a target is absent/skipped or LoadImage fails, do not enter BDS.
  // Cannot recover a hung driver, exception loop or dispatcher deadlock.
  // Fixed 7 s: timing alone cannot distinguish this from a 7 s target gate.
  MicroSecondDelay (7000000);
  ResetCold ();
  CpuDeadLoop ();
#endif
}
#endif
