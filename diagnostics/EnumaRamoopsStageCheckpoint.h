// SPDX-License-Identifier: BSD-2-Clause-Patent
// R-017 standalone variant, staged only into DxeMain. Baseline gates unchanged.
#ifndef ENUMA_STAGE_CHECKPOINT_H
#define ENUMA_STAGE_CHECKPOINT_H
#include <Library/BaseLib.h>
#include <Library/ResetSystemLib.h>
#include <Library/TimerLib.h>
#if ENUMA_RAMOOPS_CANARY != 1
#error R-017 stage variant requires explicit canary profile
#endif
#include "EnumaRamoopsCanary.h"
STATIC VOID EnumaStageCheckpoint (UINTN Stage)
{
  if (Stage == 10) {
    if (EnumaWriteRamoopsCanary () != 44) {
      ResetCold ();
      CpuDeadLoop ();
    }
    MicroSecondDelay (ENUMA_DIAG_DELAY_US);
    ResetWarm (); // PSCI feature-checked, unsupported platforms fall back cold.
    CpuDeadLoop (); // A reset returning must not dispatch drivers.
  }
}
#endif
