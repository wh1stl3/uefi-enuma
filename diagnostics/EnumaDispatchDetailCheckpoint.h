// SPDX-License-Identifier: BSD-2-Clause-Patent
// Reuse the original ordinal counter, timer and PSCI reset gate.
#ifndef ENUMA_DISPATCH_DETAIL_CHECKPOINT_H
#define ENUMA_DISPATCH_DETAIL_CHECKPOINT_H
#include "EnumaDispatchCheckpoint.h"
#include "EnumaGopResult.h"
#ifndef ENUMA_DIAG_DETAIL_POINT
#define ENUMA_DIAG_DETAIL_POINT 13
#endif
#ifndef ENUMA_DIAG_GUID_ENABLED
#define ENUMA_DIAG_GUID_ENABLED 0
#endif
#ifndef ENUMA_DIAG_GUID_ONLY
#define ENUMA_DIAG_GUID_ONLY 0
#endif
#ifndef ENUMA_DIAG_GOP_QUERY
#define ENUMA_DIAG_GOP_QUERY 0
#endif
#ifndef ENUMA_NATIVE_ROUTE_AUDIT
#define ENUMA_NATIVE_ROUTE_AUDIT 0
#endif
#if ENUMA_NATIVE_ROUTE_AUDIT < 0 || ENUMA_NATIVE_ROUTE_AUDIT > 1
#error Invalid native route audit
#endif
#ifndef ENUMA_NATIVE_MASK_FIELD
#define ENUMA_NATIVE_MASK_FIELD 0
#endif
#ifndef ENUMA_VIDEO_TERMINAL
#define ENUMA_VIDEO_TERMINAL 0
#endif
#if ENUMA_DIAG_GUID_ONLY != 0 && ENUMA_DIAG_GUID_ONLY != 1
#error Unsupported enuma GUID-only selection
#endif
#if ENUMA_DIAG_GUID_ONLY && (!ENUMA_DIAG_GUID_ENABLED || ENUMA_DIAG_STAGE != 13 || ENUMA_DIAG_DETAIL_POINT == 23 || ENUMA_DIAG_DETAIL_POINT >= 26)
#error GUID-only requires a stage 13 driver detail and a complete GUID
#endif
#ifndef ENUMA_DIAG_GUID_LOW64
#define ENUMA_DIAG_GUID_LOW64 0
#endif
#ifndef ENUMA_DIAG_GUID_HIGH64
#define ENUMA_DIAG_GUID_HIGH64 0
#endif
#if ENUMA_DIAG_DETAIL_POINT < 13 || ENUMA_DIAG_DETAIL_POINT > 27
#error Unsupported enuma dispatch detail point
#endif
#if ENUMA_DIAG_GUID_ENABLED != 0 && ENUMA_DIAG_GUID_ENABLED != 1
#error Unsupported enuma GUID filter
#endif
#if ENUMA_NATIVE_ROUTE_AUDIT && (!ENUMA_VIDEO_TERMINAL || ENUMA_NATIVE_MASK_FIELD)
#error Native route requires terminal and excludes mask field
#endif
#if ENUMA_NATIVE_MASK_FIELD < 0 || ENUMA_NATIVE_MASK_FIELD > 7 || (ENUMA_NATIVE_MASK_FIELD && !ENUMA_VIDEO_TERMINAL)
#error Unsupported native mask terminal
#endif
#if ENUMA_VIDEO_TERMINAL != 0 && ENUMA_VIDEO_TERMINAL != 1
#error Unsupported video terminal selector
#endif
#if ENUMA_VIDEO_TERMINAL && (ENUMA_DIAG_STAGE != 13 || ENUMA_DIAG_DETAIL_POINT != 16 || ENUMA_DIAG_GOP_QUERY != 1 || !ENUMA_DIAG_GUID_ONLY || !ENUMA_DIAG_GUID_ENABLED || ENUMA_DIAG_DISPATCH_FALLBACK || ENUMA_DIAG_DELAY_US != 2000000)
#error Video terminal requires exact GUID-only return point, query1, delay2, no completion fallback
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
  // Only the final CoreStartImage return and an exact diagnostic marker count.
  // EFI_SUCCESS, unrelated warnings and install/load failures are not markers.
  if (ENUMA_DIAG_GOP_QUERY != 0) {
#if ENUMA_NATIVE_MASK_FIELD || ENUMA_NATIVE_ROUTE_AUDIT
    if (Point != 13 || ENUMA_DIAG_DETAIL_POINT != 16 || Status < 1 || Status > 5) {
      return;
    }
#elif ENUMA_VIDEO_TERMINAL
    if (Point != 13 || ENUMA_DIAG_DETAIL_POINT != 16 ||
        (Status != ENUMA_GOP_RESULT_PASS && Status != ENUMA_GOP_RESULT_FAIL)) {
      return;
    }
#else
    if (Point != 13 || ENUMA_DIAG_DETAIL_POINT != 16 ||
        Status != (ENUMA_DIAG_GOP_QUERY == 1 ?
          ENUMA_GOP_RESULT_PASS : ENUMA_GOP_RESULT_FAIL)) {
      return;
    }
#endif
    SelectedPoint = 16;
  }
  // 13: any StartImage return; 14: any LoadImage return; 15: before StartImage.
  // 16/17: failed/successful StartImage; 18/19: failed/successful LoadImage.
  // 20: before the PE entry call; 21: direct PE return; 22: after Exit/LongJump.
  // 23 is a separate global unhandled-exception gate, never called here.
  // 24/25: vendor canary table prefix present/absent before the PE call.
  // 26/27 are separate global pool-search failure/success gates.
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
      (!ENUMA_DIAG_GUID_ONLY && EnumaDispatchOrdinal != ENUMA_DIAG_DISPATCH_ORDINAL)) {
    return;
  }
#if ENUMA_DIAG_GUID_ENABLED
  if (FileName == NULL ||
      ReadUnaligned64 ((CONST UINT64 *) FileName) != ENUMA_DIAG_GUID_LOW64 ||
      ReadUnaligned64 ((CONST UINT64 *) ((CONST UINT8 *) FileName + 8)) != ENUMA_DIAG_GUID_HIGH64) {
    return;
  }
#endif
#if ENUMA_NATIVE_MASK_FIELD || ENUMA_NATIVE_ROUTE_AUDIT
  // Exact full GUID and five bounded warning codes only. No fallback or writes.
  MicroSecondDelay (2000000 + 5000000 * ((UINTN)Status - 1));
  ResetCold ();
  CpuDeadLoop ();
#elif ENUMA_VIDEO_TERMINAL
  // Both exact markers stop HERE, after full GUID filtering. A predicate
  // refusal must never fall through into later dispatcher drivers.
  if (Status == ENUMA_GOP_RESULT_FAIL) {
    MicroSecondDelay (7000000);
    ResetCold ();
    CpuDeadLoop ();
  }
#endif
  EnumaStageCheckpoint (13);
}
#endif
