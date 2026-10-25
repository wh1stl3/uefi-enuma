// SPDX-License-Identifier: BSD-2-Clause-Patent
// Diagnostic marker selection only. No admission, MMIO, waiting or GOP install.
#ifndef ENUMA_VIDEO_RESULT_H
#define ENUMA_VIDEO_RESULT_H
#include "Evidence.h"
// Query0 is the original complete predicate. Queries1/2 are complementary
// questions about the FIRST returned status, not evidence of GOP admission.
// 1/5 reuse the exact existing warning transport; 0 is deliberately unrecognized.
static inline ENUMA_U32 EnumaVideoResult(ENUMA_U32 Status,ENUMA_U32 Requirements,ENUMA_U32 Query) {
  if(Query>2||Status>EnumaInheritedTopology) { return 0; }
  if(Status==EnumaInheritedOk&&Requirements!=(ENUMA_REQUIRE_ADDRESS|ENUMA_REQUIRE_TOPOLOGY)) { return 0; }
  if(Query) { return ((Status==EnumaInheritedTopology)==(Query==1))?1:5; }
  return Status==EnumaInheritedOk?1:5;
}
// Targets: 1 first-sample enable gate; 2 later eligibility/stability;
// 3 first-sample timing/active; 4 any RefreshMode status. No admission changes.
static inline ENUMA_U32 EnumaVideoReasonResult(ENUMA_U32 Status,ENUMA_U32 Requirements,ENUMA_U32 Reason,ENUMA_U32 Target,ENUMA_U32 Query) {
  ENUMA_U32 Match=0,Refresh;
  if(!Target||Target>4||!Query||Query>2||!Reason||Reason>EnumaVideoReasonInterfaceLater||Status>EnumaInheritedTopology) { return 0; }
  Refresh=Reason==EnumaVideoReasonInterfaceInitial||Reason==EnumaVideoReasonCounterInitial||Reason==EnumaVideoReasonCounterLater||Reason==EnumaVideoReasonWait||Reason==EnumaVideoReasonStableConfig||Reason==EnumaVideoReasonStableInterface||Reason==EnumaVideoReasonStableFrame||Reason==EnumaVideoReasonInterfaceLater;
  if((Status==EnumaInheritedOk)!=(Reason==EnumaVideoReasonComplete)||
     (Status==EnumaInheritedRefreshMode)!=Refresh||
     (Status==EnumaInheritedOk&&Requirements!=(ENUMA_REQUIRE_ADDRESS|ENUMA_REQUIRE_TOPOLOGY))) { return 0; }
  if(Target==1) { Match=Reason==EnumaVideoReasonCounterInitial; }
  if(Target==2) { Match=Reason==EnumaVideoReasonCounterLater||Reason==EnumaVideoReasonStableConfig||Reason==EnumaVideoReasonStableInterface||Reason==EnumaVideoReasonStableFrame||Reason==EnumaVideoReasonInterfaceLater; }
  if(Target==3) { Match=Reason==EnumaVideoReasonInterfaceInitial; }
  if(Target==4) { Match=Status==EnumaInheritedRefreshMode; }
  return (Match==(Query==1))?1:5;
}
#endif
