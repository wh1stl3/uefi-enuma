// SPDX-License-Identifier: BSD-2-Clause-Patent
// No console contents access. Isolate only PSCI warm request from cold control.
#ifndef ENUMA_STAGE_CHECKPOINT_H
#define ENUMA_STAGE_CHECKPOINT_H
#include <Library/BaseLib.h>
#include <Library/ResetSystemLib.h>
#include <Library/TimerLib.h>
#if ENUMA_RAMOOPS_WARM_CONTROL != 1 || ENUMA_DIAG_STAGE != 10 || ENUMA_DIAG_DELAY_US != 2000000
#error Warm control requires explicit stage10/delay2 profile
#endif
STATIC VOID EnumaStageCheckpoint (UINTN Stage)
{
  if (Stage == 10) {
    MicroSecondDelay (ENUMA_DIAG_DELAY_US);
    ResetWarm ();
    CpuDeadLoop ();
  }
}
#endif
