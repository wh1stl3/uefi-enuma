// SPDX-License-Identifier: BSD-2-Clause-Patent
#ifndef ENUMA_INHERITED_EVIDENCE_H
#define ENUMA_INHERITED_EVIDENCE_H
#include "Topology.h"
typedef struct { ENUMA_U32 Timing,CounterEnable,Active,Frame; } ENUMA_VIDEO_INTF;
// Zero-filled unobserved fields in Scanout are placeholders, not hardware zeros.
typedef struct { ENUMA_U32 CtlActiveMask,CtlRouteMask,PipeMask,MixerMask,CommandRefreshMask,CtlTransientMask; } ENUMA_VIDEO_SCOPE;
typedef struct {
  ENUMA_INHERITED_SNAPSHOT Scanout;
  ENUMA_MIXER_SNAPSHOT Mixers;
  ENUMA_VIDEO_INTF Interfaces[2];
  ENUMA_VIDEO_SCOPE Scope;
} ENUMA_VIDEO_SAMPLE;
typedef struct {
  ENUMA_VIDEO_SAMPLE Samples[3];
  ENUMA_INHERITED_LIMITS Limits;
  ENUMA_INHERITED_FRAME Frame;
} ENUMA_VIDEO_EVIDENCE;
// Exactly1 success; a hardware caller may use the already qualified Stall.
typedef int (*ENUMA_INHERITED_WAIT)(void *,ENUMA_U32);
// First-return reason only: no extra read, no continuing after a rejection.
enum { EnumaVideoReasonUnknown,EnumaVideoReasonArguments,EnumaVideoReasonRead,
  EnumaVideoReasonRevision,EnumaVideoReasonInterfaceInitial,EnumaVideoReasonCounterInitial,
  EnumaVideoReasonCounterLater,EnumaVideoReasonWait,EnumaVideoReasonPipeline,
  EnumaVideoReasonStableConfig,EnumaVideoReasonStableInterface,EnumaVideoReasonStableFrame,
  EnumaVideoReasonComplete,EnumaVideoReasonInterfaceLater,
  EnumaVideoReasonNativeCtl,EnumaVideoReasonNativeStaging,EnumaVideoReasonNativePipe,
  EnumaVideoReasonNativeMixerCapture,EnumaVideoReasonNativeMixerLayout,EnumaVideoReasonNativeDecode,
  EnumaVideoReasonStagingOther,EnumaVideoReasonStagingBorderOnly,EnumaVideoReasonStagingSourceShape,
  EnumaVideoReasonStagingUnknownFields,EnumaVideoReasonStagingCardinality,
  EnumaVideoReasonLayoutOther,EnumaVideoReasonLayoutBlend,EnumaVideoReasonLayoutMode,EnumaVideoReasonLayoutGeometry,EnumaVideoReasonLayoutSides,
  EnumaVideoReasonBlendOther,EnumaVideoReasonBlendSelectedDim,EnumaVideoReasonBlendOtherDim,EnumaVideoReasonBlendOperation,EnumaVideoReasonBlendAlpha };
ENUMA_U32 EnumaSampleVideoDetailed(ENUMA_INHERITED_READ,ENUMA_INHERITED_WAIT,void *,const ENUMA_INHERITED_LIMITS *,ENUMA_VIDEO_EVIDENCE *,ENUMA_U32 *);
// Independent native STATUS policy: zero timing and non-3 enables allowed;
// their full words must remain stable. Does not change strict GOP admission.
ENUMA_U32 EnumaSampleNativeVideoDetailed(ENUMA_INHERITED_READ,ENUMA_INHERITED_WAIT,void *,const ENUMA_INHERITED_LIMITS *,ENUMA_VIDEO_EVIDENCE *,ENUMA_U32 *);
// Up to330 read callbacks +two100ms waits, no pixels/physical pointers.
// Output unchanged on failure. Success qualifies stable register configuration
// and both counter windows, never CPU/SMMU identity or downstream panel routing.
ENUMA_U32 EnumaSampleVideo(ENUMA_INHERITED_READ,ENUMA_INHERITED_WAIT,void *,const ENUMA_INHERITED_LIMITS *,ENUMA_VIDEO_EVIDENCE *);
ENUMA_U32 EnumaCaptureVideoConfig(ENUMA_INHERITED_READ,void *,ENUMA_U32,ENUMA_INHERITED_SNAPSHOT *,ENUMA_VIDEO_SCOPE *);
ENUMA_U32 EnumaCaptureVideoConfigDetailed(ENUMA_INHERITED_READ,void *,ENUMA_U32,ENUMA_INHERITED_SNAPSHOT *,ENUMA_VIDEO_SCOPE *,ENUMA_U32 *);

ENUMA_U32 EnumaCaptureNativeBorderConfig(ENUMA_INHERITED_READ,void *,ENUMA_U32,ENUMA_INHERITED_SNAPSHOT *,ENUMA_VIDEO_SCOPE *,ENUMA_U32 *);
ENUMA_U32 EnumaSampleNativeBorderVideoDetailed(ENUMA_INHERITED_READ,ENUMA_INHERITED_WAIT,void *,const ENUMA_INHERITED_LIMITS *,ENUMA_VIDEO_EVIDENCE *,ENUMA_U32 *);
// Structural observation retains BLEND, ADDRESS and TOPOLOGY requirements.
ENUMA_U32 EnumaSampleNativeStructureDetailed(ENUMA_INHERITED_READ,ENUMA_INHERITED_WAIT,void *,const ENUMA_INHERITED_LIMITS *,ENUMA_VIDEO_EVIDENCE *,ENUMA_U32 *);
#endif
