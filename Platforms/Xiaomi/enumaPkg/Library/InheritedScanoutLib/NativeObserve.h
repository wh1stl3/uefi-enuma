// SPDX-License-Identifier: BSD-2-Clause-Patent
// Independent diagnostic observation, never GOP admission or a frame descriptor.
#ifndef ENUMA_NATIVE_OBSERVE_H
#define ENUMA_NATIVE_OBSERVE_H
#include "Evidence.h"
typedef struct {
  ENUMA_VIDEO_INTF Interfaces[2];
  ENUMA_U32 FrameReadMask;
} ENUMA_NATIVE_SAMPLE;
typedef struct {
  ENUMA_U32 Revision;
  ENUMA_NATIVE_SAMPLE Samples[3];
  ENUMA_U32 ActiveThroughoutMask,TimingStableMask,EnableStableMask;
  ENUMA_U32 MovingMask,ExactEnable3Mask,ReadCount,WaitCount;
} ENUMA_NATIVE_OBSERVATION;
// Up to26 callback reads and two100ms waits. STATUS bit0 gates Frame reads,
// matching Xiaomi52a2f18 v1 get_status behavior. Timing/Enable are observations;
// neither Enable&3==3 nor nonzero Timing aborts collection. Full capture unchanged.
// Exactly1 callback success; failure leaves Out unchanged. Inactive Frame fields
// are placeholders identified by FrameReadMask, never observed hardware zeros.
ENUMA_U32 EnumaObserveNativeVideo(ENUMA_INHERITED_READ,ENUMA_INHERITED_WAIT,void *,ENUMA_NATIVE_OBSERVATION *);
#endif
