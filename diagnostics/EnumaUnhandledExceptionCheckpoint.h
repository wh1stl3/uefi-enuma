// SPDX-License-Identifier: BSD-2-Clause-Patent
// Global diagnostic at DumpCpuContext entry, before formatting or ConOut.
// No driver attribution, context allocation, MMIO or retained-RAM writes.
#ifndef ENUMA_UNHANDLED_EXCEPTION_CHECKPOINT_H
#define ENUMA_UNHANDLED_EXCEPTION_CHECKPOINT_H
#include "EnumaStageCheckpoint.h"
#ifndef ENUMA_DIAG_DETAIL_POINT
#define ENUMA_DIAG_DETAIL_POINT 13
#endif
STATIC VOID EnumaUnhandledExceptionCheckpoint (VOID)
{
  if (ENUMA_DIAG_STAGE == 13 && ENUMA_DIAG_DETAIL_POINT == 23) {
    EnumaStageCheckpoint (13);
  }
}
#endif
