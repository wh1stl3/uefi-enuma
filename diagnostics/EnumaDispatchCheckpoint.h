// SPDX-License-Identifier: BSD-2-Clause-Patent
// Count scheduled entries, not a guessed vendor/APRIORI execution order.
#ifndef ENUMA_DISPATCH_CHECKPOINT_H
#define ENUMA_DISPATCH_CHECKPOINT_H
#include "EnumaStageCheckpoint.h"
#ifndef ENUMA_DIAG_DISPATCH_ORDINAL
#define ENUMA_DIAG_DISPATCH_ORDINAL 1
#endif
#if ENUMA_DIAG_DISPATCH_ORDINAL < 1 || ENUMA_DIAG_DISPATCH_ORDINAL > 128
#error Unsupported enuma dispatch ordinal
#endif
STATIC UINTN EnumaDispatchOrdinal;
STATIC VOID EnumaDispatchCheckpoint (UINTN Point)
{
  if (ENUMA_DIAG_STAGE == 12 || ENUMA_DIAG_STAGE == 13) {
    if (Point == 12) { EnumaDispatchOrdinal++; }
    if (EnumaDispatchOrdinal == ENUMA_DIAG_DISPATCH_ORDINAL) {
      EnumaStageCheckpoint (Point);
    }
  }
}
#endif
