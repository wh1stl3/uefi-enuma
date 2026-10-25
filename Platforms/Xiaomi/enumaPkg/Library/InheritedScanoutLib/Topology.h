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
ENUMA_U32 EnumaCaptureRoutedMixers(const ENUMA_INHERITED_SNAPSHOT *,ENUMA_INHERITED_READ,void *,ENUMA_MIXER_SNAPSHOT *);
// Pure strict route selector; no hardware reads. Outputs unchanged on failure.
ENUMA_U32 EnumaSelectVideoRoute(const ENUMA_INHERITED_SNAPSHOT *,ENUMA_U32 *,ENUMA_U32 *,ENUMA_U32 *);
ENUMA_U32 EnumaResolveMixerOrigins(const ENUMA_INHERITED_SNAPSHOT *,const ENUMA_MIXER_SNAPSHOT *,const ENUMA_INHERITED_LIMITS *,ENUMA_INHERITED_LIMITS *);
// Pure classification of already captured staging; never admission.
enum { EnumaStagingOther,EnumaStagingBorderOnly,EnumaStagingSourceShape,EnumaStagingUnknownFields,EnumaStagingCardinality };
ENUMA_U32 EnumaClassifyVideoStaging(const ENUMA_INHERITED_SNAPSHOT *);
ENUMA_U32 EnumaNormalizeVideoBorder(const ENUMA_INHERITED_SNAPSHOT *,ENUMA_INHERITED_SNAPSHOT *);
// First failed pure layout predicate; no extra reads or admission changes.
enum { EnumaLayoutOther,EnumaLayoutBlend,EnumaLayoutMode,EnumaLayoutGeometry,EnumaLayoutSides };
ENUMA_U32 EnumaResolveMixerOriginsDetailed(const ENUMA_INHERITED_SNAPSHOT *,const ENUMA_MIXER_SNAPSHOT *,const ENUMA_INHERITED_LIMITS *,ENUMA_INHERITED_LIMITS *,ENUMA_U32 *);
enum { EnumaBlendOther,EnumaBlendSelectedDim,EnumaBlendOtherDim,EnumaBlendOperation,EnumaBlendAlpha };
ENUMA_U32 EnumaClassifyVideoBlend(const ENUMA_INHERITED_SNAPSHOT *,const ENUMA_MIXER_SNAPSHOT *);
ENUMA_U32 EnumaResolveMixerGeometry(const ENUMA_INHERITED_SNAPSHOT *,const ENUMA_MIXER_SNAPSHOT *,const ENUMA_INHERITED_LIMITS *,ENUMA_INHERITED_LIMITS *,ENUMA_U32 *);
enum { EnumaGeometryOther,EnumaGeometryMixerSize,EnumaGeometrySourceLocal,EnumaGeometrySides,EnumaGeometryCombinedWidth };
ENUMA_U32 EnumaClassifyMixerGeometry(const ENUMA_INHERITED_SNAPSHOT *,const ENUMA_MIXER_SNAPSHOT *,const ENUMA_INHERITED_LIMITS *);
ENUMA_U32 EnumaNormalizeGlobalMixerOutput(const ENUMA_INHERITED_SNAPSHOT *,const ENUMA_MIXER_SNAPSHOT *,const ENUMA_INHERITED_LIMITS *,ENUMA_INHERITED_SNAPSHOT *);
#endif
