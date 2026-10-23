// SPDX-License-Identifier: BSD-2-Clause-Patent
// Global pool backing-page search result. Caller already holds memory lock.
// Timer/PSCI only: no allocation, map walk, logging or resource mutation.
#ifndef ENUMA_POOL_SEARCH_CHECKPOINT_H
#define ENUMA_POOL_SEARCH_CHECKPOINT_H
#include "EnumaStageCheckpoint.h"
#ifndef ENUMA_DIAG_DETAIL_POINT
#define ENUMA_DIAG_DETAIL_POINT 13
#endif
STATIC VOID EnumaPoolSearchCheckpoint (UINT64 Start)
{
  if (ENUMA_DIAG_STAGE == 13 &&
      ((ENUMA_DIAG_DETAIL_POINT == 26 && Start == 0) ||
       (ENUMA_DIAG_DETAIL_POINT == 27 && Start != 0))) {
    EnumaStageCheckpoint (13);
  }
}
#endif
