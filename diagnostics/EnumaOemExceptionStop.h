// SPDX-License-Identifier: BSD-2-Clause-Patent
// Runtime OEM experiment: stop before formatting or another pixel/MMIO access.
#ifndef ENUMA_UNHANDLED_EXCEPTION_CHECKPOINT_H
#define ENUMA_UNHANDLED_EXCEPTION_CHECKPOINT_H
#include <Library/ResetSystemLib.h>
#include <Library/BaseLib.h>
#if ENUMA_OEM_EXIT_CLOCK
#include "EnumaOemExitClock.h"
#endif
STATIC VOID EnumaUnhandledExceptionCheckpoint(VOID){
#if ENUMA_OEM_EXIT_CLOCK
  EnumaOemClockExit(1);
#else
  ResetCold();for(;;){CpuPause();}
#endif
}
#endif
