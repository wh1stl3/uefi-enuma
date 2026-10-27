// SPDX-License-Identifier: BSD-2-Clause-Patent
// Runtime OEM experiment: stop before formatting or another pixel/MMIO access.
#ifndef ENUMA_UNHANDLED_EXCEPTION_CHECKPOINT_H
#define ENUMA_UNHANDLED_EXCEPTION_CHECKPOINT_H
#include <Library/ResetSystemLib.h>
#include <Library/BaseLib.h>
STATIC VOID EnumaUnhandledExceptionCheckpoint(VOID){
  ResetCold();for(;;){CpuPause();}
}
#endif
