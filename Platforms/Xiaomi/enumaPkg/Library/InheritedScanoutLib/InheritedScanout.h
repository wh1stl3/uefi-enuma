// SPDX-License-Identifier: BSD-2-Clause-Patent
// Portable saved-snapshot types. No UEFI, MMIO, physical pointers or panel constants.
#ifndef ENUMA_INHERITED_SCANOUT_H
#define ENUMA_INHERITED_SCANOUT_H
typedef unsigned int ENUMA_U32;
typedef unsigned long long ENUMA_U64;
typedef char EnumaU32Size[(sizeof(ENUMA_U32)==4)?1:-1];
typedef char EnumaU64Size[(sizeof(ENUMA_U64)==8)?1:-1];
#define ENUMA_PIPE_COUNT 8
#define ENUMA_CTL_COUNT 6
#define ENUMA_MIXER_COUNT 6
#define ENUMA_MODE_VIDEO 0
#define ENUMA_MODE_COMMAND 1
typedef struct { ENUMA_U32 Size,Xy,OutSize,OutXy,Address,Stride,Format,Unpack,OpMode,Multirect,Secure; } ENUMA_INHERITED_PIPE;
typedef struct { ENUMA_U32 Top,IntfActive,Start,Flush; ENUMA_U32 Layers[6][4]; } ENUMA_INHERITED_CTL;
typedef struct { ENUMA_U32 Revision; ENUMA_INHERITED_PIPE Pipes[8]; ENUMA_INHERITED_CTL Ctls[6]; ENUMA_U32 IntfAutoRefresh[2],IntfTearEnable[2],PpAutoRefresh[6]; } ENUMA_INHERITED_SNAPSHOT;
typedef struct {
  ENUMA_U64 SplashBase,SplashBytes;
  ENUMA_U32 Width,Height,Mode;
  // Explicitly established full-panel origin of each pipe's local output space.
  ENUMA_U32 OriginMask,OriginX[8],OriginY[8];
} ENUMA_INHERITED_LIMITS;
// Candidate layout only: external qualification requirements remain.
#define ENUMA_REQUIRE_ACTIVITY 1u
#define ENUMA_REQUIRE_BLEND 2u
#define ENUMA_REQUIRE_ADDRESS 4u
#define ENUMA_REQUIRE_REFRESH 8u
#define ENUMA_REQUIRE_TOPOLOGY 16u
#define ENUMA_REQUIRE_ALL 31u
typedef struct { ENUMA_U64 Base,Bytes; ENUMA_U32 Width,Height,Stride,RedMask,GreenMask,BlueMask,ReservedMask,RedShift,GreenShift,BlueShift,PipeMask,CtlMask,ManualCtlMask,IntfMask,Requirements; } ENUMA_INHERITED_FRAME;
enum { EnumaInheritedOk,EnumaInheritedRevision,EnumaInheritedRoute,EnumaInheritedVirtualRect,EnumaInheritedGeometry,EnumaInheritedFormat,EnumaInheritedSecure,EnumaInheritedBounds,EnumaInheritedInconsistent,EnumaInheritedCoverage,EnumaInheritedRefreshMode,EnumaInheritedArguments,EnumaInheritedMode,EnumaInheritedTopology };
// OutXy is local to a known origin. OriginMask must cover every selected pipe;
// origins come from separately established topology, never guessed LM order.
// Decode failure clears Result; capture failure leaves Out unchanged.
// Neither function dereferences pixel addresses.
ENUMA_U32 EnumaDecodeInherited(const ENUMA_INHERITED_SNAPSHOT *,const ENUMA_INHERITED_LIMITS *,ENUMA_INHERITED_FRAME *);
// Read callback receives a relative SDE register offset; caller owns aperture/mapping.
typedef int (*ENUMA_INHERITED_READ)(void *,ENUMA_U32,ENUMA_U32 *);
ENUMA_U32 EnumaCaptureInherited(ENUMA_INHERITED_READ,void *,ENUMA_INHERITED_SNAPSHOT *);
#endif
