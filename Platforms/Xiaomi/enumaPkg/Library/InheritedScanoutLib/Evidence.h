// SPDX-License-Identifier: BSD-2-Clause-Patent
#ifndef ENUMA_INHERITED_EVIDENCE_H
#define ENUMA_INHERITED_EVIDENCE_H
#include "Topology.h"
typedef struct { ENUMA_U32 Timing,CounterEnable,Active,Frame; } ENUMA_VIDEO_INTF;
typedef struct {
  ENUMA_INHERITED_SNAPSHOT Scanout;
  ENUMA_MIXER_SNAPSHOT Mixers;
  ENUMA_VIDEO_INTF Interfaces[2];
} ENUMA_VIDEO_SAMPLE;
typedef struct {
  ENUMA_VIDEO_SAMPLE Samples[3];
  ENUMA_INHERITED_LIMITS Limits;
  ENUMA_INHERITED_FRAME Frame;
} ENUMA_VIDEO_EVIDENCE;
// Exactly1 success; a hardware caller may use the already qualified Stall.
typedef int (*ENUMA_INHERITED_WAIT)(void *,ENUMA_U32);
// Up to969 read callbacks +two100ms waits, no pixels/physical pointers.
// Output unchanged on failure. Success qualifies stable register configuration
// and both counter windows, never CPU/SMMU identity or downstream panel routing.
ENUMA_U32 EnumaSampleVideo(ENUMA_INHERITED_READ,ENUMA_INHERITED_WAIT,void *,const ENUMA_INHERITED_LIMITS *,ENUMA_VIDEO_EVIDENCE *);
#endif
