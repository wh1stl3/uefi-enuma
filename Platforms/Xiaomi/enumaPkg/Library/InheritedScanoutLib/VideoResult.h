// SPDX-License-Identifier: BSD-2-Clause-Patent
// Diagnostic marker selection only. No admission, MMIO, waiting or GOP install.
#ifndef ENUMA_VIDEO_RESULT_H
#define ENUMA_VIDEO_RESULT_H
#include "InheritedScanout.h"
// Query0 is the original complete predicate. Queries1/2 are complementary
// questions about the FIRST returned status, not evidence of GOP admission.
// 1/5 reuse the exact existing warning transport; 0 is deliberately unrecognized.
static ENUMA_U32 EnumaVideoResult(ENUMA_U32 Status,ENUMA_U32 Requirements,ENUMA_U32 Query) {
  if(Query>2||Status>EnumaInheritedTopology) { return 0; }
  if(Status==EnumaInheritedOk&&Requirements!=(ENUMA_REQUIRE_ADDRESS|ENUMA_REQUIRE_TOPOLOGY)) { return 0; }
  if(Query) { return ((Status==EnumaInheritedTopology)==(Query==1))?1:5; }
  return Status==EnumaInheritedOk?1:5;
}
#endif
