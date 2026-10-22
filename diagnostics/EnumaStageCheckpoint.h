// SPDX-License-Identifier: BSD-2-Clause-Patent
// Diagnostic-only reset gates. Zero leaves the production path unchanged.
// Reuses the fixed Mu TimerLib and ArmPsciResetSystemLib, no MMIO marker.
#ifndef ENUMA_STAGE_CHECKPOINT_H
#define ENUMA_STAGE_CHECKPOINT_H
#include <Library/BaseLib.h>
#include <Library/ResetSystemLib.h>
#include <Library/TimerLib.h>
#ifndef ENUMA_DIAG_STAGE
#define ENUMA_DIAG_STAGE 0
#endif
#ifndef ENUMA_DIAG_DELAY_US
#define ENUMA_DIAG_DELAY_US 2000000
#endif
#if ENUMA_DIAG_DELAY_US != 2000000 && ENUMA_DIAG_DELAY_US != 7000000
#error Unsupported enuma diagnostic delay
#endif
#if ENUMA_DIAG_STAGE != 0 && (ENUMA_DIAG_STAGE < 3 || ENUMA_DIAG_STAGE > 13)
#error Unsupported enuma diagnostic stop stage
#endif
STATIC VOID EnumaStageCheckpoint (UINTN Stage)
{
  if (ENUMA_DIAG_STAGE != 0 && Stage == ENUMA_DIAG_STAGE) {
    MicroSecondDelay (ENUMA_DIAG_DELAY_US);
    ResetCold ();
    CpuDeadLoop (); // Reset returned: never accidentally continue the probe.
  }
}
#endif
