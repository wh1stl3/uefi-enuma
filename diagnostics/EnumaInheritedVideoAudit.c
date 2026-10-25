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
#ifndef ENUMA_NATIVE_ROUTE_CPU_POLICY
#define ENUMA_NATIVE_ROUTE_CPU_POLICY 0
#endif
#if ENUMA_NATIVE_ROUTE_CPU_POLICY < 0 || ENUMA_NATIVE_ROUTE_CPU_POLICY > 1 || (ENUMA_NATIVE_ROUTE_CPU_POLICY && (!ENUMA_NATIVE_ROUTE_CPU_STAGE1 || ENUMA_NATIVE_ROUTE_CPU_ATTRIBUTE))
#error Whole-range CPU policy requires stage1 and excludes first-attribute selector
#endif
#ifndef ENUMA_NATIVE_ROUTE_CPU_ATTRIBUTE
#define ENUMA_NATIVE_ROUTE_CPU_ATTRIBUTE 0
#endif
#if ENUMA_NATIVE_ROUTE_CPU_ATTRIBUTE < 0 || ENUMA_NATIVE_ROUTE_CPU_ATTRIBUTE > 1 || (ENUMA_NATIVE_ROUTE_CPU_ATTRIBUTE && !ENUMA_NATIVE_ROUTE_CPU_STAGE1)
#error CPU attribute classification requires stage1 query
#endif
#ifndef ENUMA_NATIVE_ROUTE_CPU_STAGE1
#define ENUMA_NATIVE_ROUTE_CPU_STAGE1 0
#endif
#if ENUMA_NATIVE_ROUTE_CPU_STAGE1 < 0 || ENUMA_NATIVE_ROUTE_CPU_STAGE1 > 1 || (ENUMA_NATIVE_ROUTE_CPU_STAGE1 && !ENUMA_NATIVE_ROUTE_ALPHA_VIEW)
#error CPU stage1 requires exact alpha view
#endif
#if ENUMA_NATIVE_ROUTE_CPU_STAGE1
#include "InheritedVideo/CpuStage1.h"
#endif
#ifndef ENUMA_NATIVE_ROUTE_ALPHA_VIEW
#define ENUMA_NATIVE_ROUTE_ALPHA_VIEW 0
#endif
#if ENUMA_NATIVE_ROUTE_ALPHA_VIEW < 0 || ENUMA_NATIVE_ROUTE_ALPHA_VIEW > 1 || (ENUMA_NATIVE_ROUTE_ALPHA_VIEW && (!ENUMA_NATIVE_ROUTE_GLOBAL_OUTPUT || ENUMA_NATIVE_ROUTE_GLOBAL_DETAIL))
#error Alpha view requires global output and excludes global detail
#endif
#ifndef ENUMA_NATIVE_ROUTE_GLOBAL_DETAIL
#define ENUMA_NATIVE_ROUTE_GLOBAL_DETAIL 0
#endif
#if ENUMA_NATIVE_ROUTE_GLOBAL_DETAIL < 0 || ENUMA_NATIVE_ROUTE_GLOBAL_DETAIL > 1 || (ENUMA_NATIVE_ROUTE_GLOBAL_DETAIL && !ENUMA_NATIVE_ROUTE_GLOBAL_OUTPUT)
#error Global source detail requires global output profile
#endif
#ifndef ENUMA_NATIVE_ROUTE_GLOBAL_OUTPUT
#define ENUMA_NATIVE_ROUTE_GLOBAL_OUTPUT 0
#endif
#if ENUMA_NATIVE_ROUTE_GLOBAL_OUTPUT < 0 || ENUMA_NATIVE_ROUTE_GLOBAL_OUTPUT > 1 || (ENUMA_NATIVE_ROUTE_GLOBAL_OUTPUT && (!ENUMA_NATIVE_ROUTE_STRUCTURE || ENUMA_NATIVE_ROUTE_STRUCTURE_DETAIL))
#error Global output view requires structure and excludes structure detail
#endif
#ifndef ENUMA_NATIVE_ROUTE_STRUCTURE_DETAIL
#define ENUMA_NATIVE_ROUTE_STRUCTURE_DETAIL 0
#endif
#if ENUMA_NATIVE_ROUTE_STRUCTURE_DETAIL < 0 || ENUMA_NATIVE_ROUTE_STRUCTURE_DETAIL > 1 || (ENUMA_NATIVE_ROUTE_STRUCTURE_DETAIL && !ENUMA_NATIVE_ROUTE_STRUCTURE)
#error Structural geometry detail requires structure profile
#endif
#ifndef ENUMA_NATIVE_ROUTE_STRUCTURE
#define ENUMA_NATIVE_ROUTE_STRUCTURE 0
#endif
#if ENUMA_NATIVE_ROUTE_STRUCTURE < 0 || ENUMA_NATIVE_ROUTE_STRUCTURE > 1 || (ENUMA_NATIVE_ROUTE_STRUCTURE && (!ENUMA_NATIVE_ROUTE_AUDIT || ENUMA_NATIVE_ROUTE_DETAIL || ENUMA_NATIVE_ROUTE_STAGING || ENUMA_NATIVE_ROUTE_LAYOUT || ENUMA_NATIVE_ROUTE_BLEND))
#error Structural audit requires native route and excludes detail profiles
#endif
#ifndef ENUMA_NATIVE_ROUTE_BLEND
#define ENUMA_NATIVE_ROUTE_BLEND 0
#endif
#if ENUMA_NATIVE_ROUTE_BLEND < 0 || ENUMA_NATIVE_ROUTE_BLEND > 1 || (ENUMA_NATIVE_ROUTE_BLEND && !ENUMA_NATIVE_ROUTE_LAYOUT)
#error Blend classification requires layout classification
#endif
#ifndef ENUMA_NATIVE_ROUTE_LAYOUT
#define ENUMA_NATIVE_ROUTE_LAYOUT 0
#endif
#if ENUMA_NATIVE_ROUTE_LAYOUT < 0 || ENUMA_NATIVE_ROUTE_LAYOUT > 1 || (ENUMA_NATIVE_ROUTE_LAYOUT && (!ENUMA_NATIVE_ROUTE_DETAIL || ENUMA_NATIVE_ROUTE_STAGING))
#error Layout classification requires detail and excludes staging
#endif
#ifndef ENUMA_NATIVE_ROUTE_STAGING
#define ENUMA_NATIVE_ROUTE_STAGING 0
#endif
#if ENUMA_NATIVE_ROUTE_STAGING < 0 || ENUMA_NATIVE_ROUTE_STAGING > 1 || (ENUMA_NATIVE_ROUTE_STAGING && !ENUMA_NATIVE_ROUTE_DETAIL)
#error Staging classification requires route detail
#endif
#ifndef ENUMA_NATIVE_ROUTE_DETAIL
#define ENUMA_NATIVE_ROUTE_DETAIL 0
#endif
#if ENUMA_NATIVE_ROUTE_DETAIL < 0 || ENUMA_NATIVE_ROUTE_DETAIL > 1 || (ENUMA_NATIVE_ROUTE_DETAIL && !ENUMA_NATIVE_ROUTE_AUDIT)
#error Route detail requires native route audit
#endif
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
#if ENUMA_NATIVE_ROUTE_GLOBAL_DETAIL
  return (EFI_STATUS)EnumaNativeGlobalDetailResult(Status,Requirements,Reason);
#elif ENUMA_NATIVE_ROUTE_STRUCTURE_DETAIL
  return (EFI_STATUS)EnumaNativeGeometryResult(Status,Requirements,Reason);
#elif ENUMA_NATIVE_ROUTE_STRUCTURE
  return (EFI_STATUS)EnumaNativeStructureResult(Status,Requirements,Reason);
#elif ENUMA_NATIVE_ROUTE_BLEND
  return (EFI_STATUS)EnumaNativeBlendResult(Status,Requirements,Reason);
#elif ENUMA_NATIVE_ROUTE_LAYOUT
  return (EFI_STATUS)EnumaNativeLayoutResult(Status,Requirements,Reason);
#elif ENUMA_NATIVE_ROUTE_STAGING
  return (EFI_STATUS)EnumaNativeStagingResult(Status,Requirements,Reason);
#elif ENUMA_NATIVE_ROUTE_DETAIL
  return (EFI_STATUS)EnumaNativeRouteDetailResult(Status,Requirements,Reason);
#elif ENUMA_NATIVE_ROUTE_AUDIT
  if(Reason>=EnumaVideoReasonNativeCtl&&Reason<=EnumaVideoReasonNativeDecode) { Reason=EnumaVideoReasonPipeline; }
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
#if ENUMA_NATIVE_ROUTE_ALPHA_VIEW
  Status=EnumaSampleNativeAlphaViewDetailed(Read,Wait,NULL,&L,&mEvidence,&Reason);
#elif ENUMA_NATIVE_ROUTE_GLOBAL_OUTPUT
  Status=EnumaSampleNativeGlobalOutputDetailed(Read,Wait,NULL,&L,&mEvidence,&Reason);
#elif ENUMA_NATIVE_ROUTE_STRUCTURE_DETAIL
  Status=EnumaSampleNativeGeometryDetailed(Read,Wait,NULL,&L,&mEvidence,&Reason);
#elif ENUMA_NATIVE_ROUTE_STRUCTURE
  Status=EnumaSampleNativeStructureDetailed(Read,Wait,NULL,&L,&mEvidence,&Reason);
#elif ENUMA_NATIVE_ROUTE_STAGING
  Status=EnumaSampleNativeVideoDetailed(Read,Wait,NULL,&L,&mEvidence,&Reason);
#else
  Status=EnumaSampleNativeBorderVideoDetailed(Read,Wait,NULL,&L,&mEvidence,&Reason);
#endif
#else
  Status=EnumaSampleVideoDetailed(Read,Wait,NULL,&L,&mEvidence,&Reason);
#endif
#if ENUMA_NATIVE_ROUTE_CPU_STAGE1
  if(Status==EnumaInheritedOk&&Reason==EnumaVideoReasonComplete&&mEvidence.Frame.Requirements==22) {
    return (EFI_STATUS)EnumaCpuStage1Arm(&mEvidence.Frame);
  }
  // Collapse all earlier failures; never classify them as translation results.
  (VOID)Result(Status,mEvidence.Frame.Requirements,Reason);
  return 1;
#endif
  // This is a configuration/counter predicate only, never GOP/address success.
  return Result(Status,mEvidence.Frame.Requirements,Reason);
}
