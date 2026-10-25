// SPDX-License-Identifier: BSD-2-Clause-Patent
#ifndef ENUMA_NATIVE_ROUTE_RESULT_H
#define ENUMA_NATIVE_ROUTE_RESULT_H
#include "Evidence.h"
// Five terminal categories, not the native two-bit mask encoding.
static inline ENUMA_U32 EnumaNativeRouteResult(ENUMA_U32 S,ENUMA_U32 R,ENUMA_U32 W) {
  if(S==EnumaInheritedOk) {
    return W==EnumaVideoReasonComplete&&R==(ENUMA_REQUIRE_ADDRESS|ENUMA_REQUIRE_TOPOLOGY)?5:1;
  }
  if(S>EnumaInheritedTopology||R||W==EnumaVideoReasonArguments||W==EnumaVideoReasonRead||
     W==EnumaVideoReasonRevision||W==EnumaVideoReasonWait||W==EnumaVideoReasonUnknown||W>EnumaVideoReasonInterfaceLater) { return 1; }
  if(S==EnumaInheritedRefreshMode) { return 2; }
  if(W!=EnumaVideoReasonPipeline) { return 1; }
  return S==EnumaInheritedTopology?3:4;
}
// Detailed topology location; no extra read and no continuing after failure.
static inline ENUMA_U32 EnumaNativeRouteDetailResult(ENUMA_U32 S,ENUMA_U32 R,ENUMA_U32 W) {
  if(S==EnumaInheritedOk) { return W==EnumaVideoReasonComplete&&R==(ENUMA_REQUIRE_ADDRESS|ENUMA_REQUIRE_TOPOLOGY)?5:1; }
  if(S!=EnumaInheritedTopology||R) { return 1; }
  if(W==EnumaVideoReasonNativeCtl) { return 2; }
  if(W==EnumaVideoReasonNativeStaging) { return 3; }
  if(W==EnumaVideoReasonNativeMixerCapture||W==EnumaVideoReasonNativeMixerLayout||W==EnumaVideoReasonNativeDecode) { return 4; }
  return 1;
}
#endif
