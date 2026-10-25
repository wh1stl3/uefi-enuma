// SPDX-License-Identifier: BSD-2-Clause-Patent
#ifndef ENUMA_INHERITED_TOPOLOGY_H
#define ENUMA_INHERITED_TOPOLOGY_H
#include "InheritedScanout.h"
// LM bit31 is the hardware right-mixer flag, not the LM number.
// This resolves local mixer coordinates only. PP/DSC/DSI routing and CPU
// translation still require independent qualification before GOP publication.
typedef struct { ENUMA_U32 Mode,Size,Blend[11],Alpha[11]; } ENUMA_MIXER_STATE;
typedef struct { ENUMA_MIXER_STATE Mixers[6]; } ENUMA_MIXER_SNAPSHOT;
ENUMA_U32 EnumaCaptureMixers(ENUMA_INHERITED_READ,void *,ENUMA_MIXER_SNAPSHOT *);
ENUMA_U32 EnumaResolveMixerOrigins(const ENUMA_INHERITED_SNAPSHOT *,const ENUMA_MIXER_SNAPSHOT *,const ENUMA_INHERITED_LIMITS *,ENUMA_INHERITED_LIMITS *);
#endif
