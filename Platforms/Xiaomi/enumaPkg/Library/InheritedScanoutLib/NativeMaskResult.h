// SPDX-License-Identifier: BSD-2-Clause-Patent
#ifndef ENUMA_NATIVE_MASK_RESULT_H
#define ENUMA_NATIVE_MASK_RESULT_H
#include "NativeObserve.h"
// Warning codes1..4 represent mask0..3;5 is distinct collection/argument failure.
static ENUMA_U32 EnumaNativeMaskResult(ENUMA_U32 Status,const ENUMA_NATIVE_OBSERVATION *O,ENUMA_U32 Field) {
  ENUMA_U32 Mask;
  if(Status!=EnumaInheritedOk||!O) { return 5; }
  switch(Field) {
    case 1: Mask=O->ActiveThroughoutMask; break;
    case 2: Mask=O->TimingStableMask; break;
    case 3: Mask=O->EnableStableMask; break;
    case 4: Mask=O->MovingMask; break;
    case 5: Mask=O->ExactEnable3Mask; break;
    default:return 5;
  }
  return Mask<=3?Mask+1:5;
}
#endif
