// SPDX-License-Identifier: BSD-2-Clause-Patent
// Opt-in READ ONLY audit entry, materialized instead of metadata SimpleFb.c.
// Keeps the existing audited entry/Core-warning transport; publishes NO GOP.
#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryMapHelperLib.h>
#include <Library/PcdLib.h>
#include "EnumaGopResult.h"
#include "InheritedVideo/Decode.c"
#include "InheritedVideo/VideoCapture.c"
#include "InheritedVideo/Topology.c"
#include "InheritedVideo/Evidence.c"
#include "InheritedVideo/VideoResult.h"
#include "InheritedVideo/NativeRouteResult.h"
#ifndef ENUMA_NATIVE_ROUTE_AUDIT
#define ENUMA_NATIVE_ROUTE_AUDIT 0
#endif
#ifndef ENUMA_VIDEO_TOPOLOGY_QUERY
#define ENUMA_VIDEO_TOPOLOGY_QUERY 0
#endif
#ifndef ENUMA_VIDEO_REASON_TARGET
#define ENUMA_VIDEO_REASON_TARGET 0
#endif
#ifndef ENUMA_VIDEO_REASON_QUERY
#define ENUMA_VIDEO_REASON_QUERY 0
#endif
#if ENUMA_VIDEO_REASON_TARGET < 0 || ENUMA_VIDEO_REASON_TARGET > 4 || ENUMA_VIDEO_REASON_QUERY < 0 || ENUMA_VIDEO_REASON_QUERY > 2 || (!!ENUMA_VIDEO_REASON_TARGET != !!ENUMA_VIDEO_REASON_QUERY)
#error Unsupported first-reason selector
#endif
#if ENUMA_VIDEO_REASON_TARGET && (!ENUMA_VIDEO_TERMINAL || ENUMA_VIDEO_TOPOLOGY_QUERY)
#error First-reason selector requires symmetric transport and excludes topology query
#endif
#if ENUMA_VIDEO_TOPOLOGY_QUERY < 0 || ENUMA_VIDEO_TOPOLOGY_QUERY > 2
#error Unsupported topology status query
#endif
#if ENUMA_VIDEO_TOPOLOGY_QUERY && !ENUMA_VIDEO_TERMINAL
#error Topology status query requires symmetric terminal transport
#endif
STATIC EFI_STATUS Result(ENUMA_U32 Status,ENUMA_U32 Requirements,ENUMA_U32 Reason) {
#if ENUMA_NATIVE_ROUTE_AUDIT
  return (EFI_STATUS)EnumaNativeRouteResult(Status,Requirements,Reason);
#elif ENUMA_VIDEO_REASON_TARGET
  return (EFI_STATUS)EnumaVideoReasonResult(Status,Requirements,Reason,ENUMA_VIDEO_REASON_TARGET,ENUMA_VIDEO_REASON_QUERY);
#else
  (VOID)Reason;
  return (EFI_STATUS)EnumaVideoResult(Status,Requirements,ENUMA_VIDEO_TOPOLOGY_QUERY);
#endif
}
STATIC UINTN mReads,mWaits;
STATIC ENUMA_VIDEO_EVIDENCE mEvidence;
STATIC int Read(void *Context,ENUMA_U32 Offset,ENUMA_U32 *Value) {
  (VOID)Context;
  if(!Value||(Offset&3)||Offset>0x6c26c||mReads>=330) { return 0; }
  mReads++;
  *Value=*(volatile UINT32 *)(UINTN)(0x0ae00000U+Offset);
  return 1;
}
STATIC int Wait(void *Context,ENUMA_U32 Us) {
  (VOID)Context;
  if(Us!=100000||mWaits>=2||!gBS||!gBS->Stall) { return 0; }
  mWaits++; return gBS->Stall(Us)==EFI_SUCCESS?1:0;
}
EFI_STATUS EFIAPI RegisterFrameBuffer(EFI_HANDLE Image,EFI_SYSTEM_TABLE *ST) {
  EFI_MEMORY_REGION_DESCRIPTOR Mdp={0},Splash={0}; ENUMA_INHERITED_LIMITS L={0}; ENUMA_U32 Status,Reason=0;
  (VOID)Image; (VOID)ST;
  if(LocateMemoryRegionByName("Display MDP",&Mdp)!=EFI_SUCCESS||Mdp.Address!=0xae00000||Mdp.Length!=0x85000||
     Mdp.ArmAttributes!=NS_DEVICE||Mdp.ResourceAttribute!=UNCACHEABLE||Mdp.MemoryType!=MmIO||Mdp.HobOption!=AddDev||
     LocateMemoryRegionByName("Display Reserved",&Splash)!=EFI_SUCCESS||Splash.Address!=0x9c000000||Splash.Length!=0x2400000) { return Result(EnumaInheritedArguments,0,EnumaVideoReasonArguments); }
  L.Width=FixedPcdGet32(PcdFrameBufferWidth); L.Height=FixedPcdGet32(PcdFrameBufferHeight);
  if(L.Width!=1600||L.Height!=2560) { return Result(EnumaInheritedArguments,0,EnumaVideoReasonArguments); }
  L.SplashBase=Splash.Address; L.SplashBytes=Splash.Length; L.Mode=ENUMA_MODE_VIDEO;
#if ENUMA_NATIVE_ROUTE_AUDIT
  Status=EnumaSampleNativeVideoDetailed(Read,Wait,NULL,&L,&mEvidence,&Reason);
#else
  Status=EnumaSampleVideoDetailed(Read,Wait,NULL,&L,&mEvidence,&Reason);
#endif
  // This is a configuration/counter predicate only, never GOP/address success.
  return Result(Status,mEvidence.Frame.Requirements,Reason);
}
