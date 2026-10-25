// SPDX-License-Identifier: BSD-2-Clause-Patent
#include "Evidence.h"
#ifndef ENUMA_NATIVE_ROUTE_BLEND
#define ENUMA_NATIVE_ROUTE_BLEND 0
#endif
#ifndef ENUMA_NATIVE_ROUTE_LAYOUT
#define ENUMA_NATIVE_ROUTE_LAYOUT 0
#endif
static int Equal(const void *A,const void *B,ENUMA_U32 N) {
  const unsigned char *X=A,*Y=B; ENUMA_U32 I;
  for(I=0;I<N;I++) { if(X[I]!=Y[I]) { return 0; } } return 1;
}
static ENUMA_U32 StableReason(const ENUMA_VIDEO_SAMPLE *A,const ENUMA_VIDEO_SAMPLE *B,int Native) {
  ENUMA_U32 C,I;
  if(!Equal(&A->Scope,&B->Scope,sizeof(A->Scope))) { return EnumaVideoReasonStableConfig; }
  if(A->Scanout.Revision!=B->Scanout.Revision||!Equal(A->Scanout.Pipes,B->Scanout.Pipes,sizeof(A->Scanout.Pipes))||
     !Equal(&A->Mixers,&B->Mixers,sizeof(A->Mixers))||
     !Equal(A->Scanout.IntfAutoRefresh,B->Scanout.IntfAutoRefresh,sizeof(A->Scanout.IntfAutoRefresh))||
     !Equal(A->Scanout.IntfTearEnable,B->Scanout.IntfTearEnable,sizeof(A->Scanout.IntfTearEnable))||
     !Equal(A->Scanout.PpAutoRefresh,B->Scanout.PpAutoRefresh,sizeof(A->Scanout.PpAutoRefresh))) { return EnumaVideoReasonStableConfig; }
  for(C=0;C<6;C++) {
    // START/FLUSH are transient; retained in evidence but not stable predicates.
    if(A->Scanout.Ctls[C].Top!=B->Scanout.Ctls[C].Top||A->Scanout.Ctls[C].IntfActive!=B->Scanout.Ctls[C].IntfActive||
       !Equal(A->Scanout.Ctls[C].Layers,B->Scanout.Ctls[C].Layers,sizeof(A->Scanout.Ctls[C].Layers))) { return EnumaVideoReasonStableConfig; }
  }
  for(I=0;I<2;I++) {
    const ENUMA_VIDEO_INTF *X=&A->Interfaces[I],*Y=&B->Interfaces[I];
    if(!(X->Active&1)||!(Y->Active&1)||(!Native&&(!X->Timing||(X->CounterEnable&3)!=3))||X->Timing!=Y->Timing||
       X->CounterEnable!=Y->CounterEnable) { return EnumaVideoReasonStableInterface; }
    if(Y->Frame<=X->Frame||Y->Frame-X->Frame>64) { return EnumaVideoReasonStableFrame; }
  }
  return EnumaVideoReasonUnknown;
}
static ENUMA_U32 Finish(ENUMA_U32 Status,ENUMA_U32 Why,ENUMA_U32 *Reason) {
  if(Reason) { *Reason=Why; } return Status;
}
static ENUMA_U32 SampleDetailed(ENUMA_INHERITED_READ Read,ENUMA_INHERITED_WAIT Wait,void *Ctx,const ENUMA_INHERITED_LIMITS *L,ENUMA_VIDEO_EVIDENCE *Out,ENUMA_U32 *Reason,int Native) {
  ENUMA_INHERITED_SNAPSHOT View,Global; ENUMA_VIDEO_EVIDENCE E; ENUMA_U32 N,I,Status; ENUMA_INHERITED_LIMITS Limits; ENUMA_INHERITED_FRAME Frame;
  if(Reason) { *Reason=EnumaVideoReasonUnknown; }
  if(!Read||!Wait||!L||!Out||L->Mode!=ENUMA_MODE_VIDEO) { return Finish(EnumaInheritedArguments,EnumaVideoReasonArguments,Reason); }
  // No allocation or implicit runtime memory helpers in the freestanding path.
  for(I=0;I<sizeof(E);I++) { ((volatile unsigned char *)&E)[I]=0; }
  for(N=0;N<3;N++) {
    ENUMA_VIDEO_SAMPLE *S=&E.Samples[N];
    if(N&&Wait(Ctx,100000)!=1) { return Finish(EnumaInheritedRefreshMode,EnumaVideoReasonWait,Reason); }
    ENUMA_U32 Revision;
    if(Read(Ctx,0,&Revision)!=1) { return Finish(EnumaInheritedBounds,EnumaVideoReasonRead,Reason); }
    if((Revision>>16)!=0x6000) { return Finish(EnumaInheritedRevision,EnumaVideoReasonRevision,Reason); }
    for(I=0;I<2;I++) {
      ENUMA_U32 Base=0x6b800+I*0x800; ENUMA_VIDEO_INTF *V=&S->Interfaces[I];
      if(Read(Ctx,Base,&V->Timing)!=1||Read(Ctx,Base+0xa8,&V->CounterEnable)!=1||
         Read(Ctx,Base+0x26c,&V->Active)!=1) { return Finish(EnumaInheritedBounds,EnumaVideoReasonRead,Reason); }
      // Native v1 policy gates FRAME reads on STATUS bit0. The strict wrapper
      // retains its original callback order and timing/enable/active priority.
      if(Native&&!(V->Active&1)) { return Finish(EnumaInheritedRefreshMode,N?EnumaVideoReasonInterfaceLater:EnumaVideoReasonInterfaceInitial,Reason); }
      if(Read(Ctx,Base+0xac,&V->Frame)!=1) { return Finish(EnumaInheritedBounds,EnumaVideoReasonRead,Reason); }
      // Preserve the original short-circuit priority: timing, enable, active.
      if(!Native&&!V->Timing) { return Finish(EnumaInheritedRefreshMode,N?EnumaVideoReasonInterfaceLater:EnumaVideoReasonInterfaceInitial,Reason); }
      if(!Native&&(V->CounterEnable&3)!=3) { return Finish(EnumaInheritedRefreshMode,N?EnumaVideoReasonCounterLater:EnumaVideoReasonCounterInitial,Reason); }
      if(!(V->Active&1)) { return Finish(EnumaInheritedRefreshMode,N?EnumaVideoReasonInterfaceLater:EnumaVideoReasonInterfaceInitial,Reason); }
    }
    ENUMA_U32 CaptureReason=EnumaVideoReasonPipeline;
    Status=Native>=2?EnumaCaptureNativeBorderConfig(Read,Ctx,Revision,&S->Scanout,&S->Scope,&CaptureReason):Native?EnumaCaptureVideoConfigDetailed(Read,Ctx,Revision,&S->Scanout,&S->Scope,&CaptureReason):EnumaCaptureVideoConfig(Read,Ctx,Revision,&S->Scanout,&S->Scope);
    if(Status) { return Finish(Status,CaptureReason,Reason); }
    const ENUMA_INHERITED_SNAPSHOT *Scanout=&S->Scanout;
    if(Native>=2) {
      Status=EnumaNormalizeVideoBorder(&S->Scanout,&View);
      if(Status) { return Finish(Status,EnumaVideoReasonNativeStaging,Reason); }
      Scanout=&View;
    }
    Status=EnumaCaptureRoutedMixers(Scanout,Read,Ctx,&S->Mixers); if(Status) { return Finish(Status,Native?EnumaVideoReasonNativeMixerCapture:EnumaVideoReasonPipeline,Reason); }
    if(Native>=4) {
      if(N) { ENUMA_U32 Why=StableReason(&E.Samples[N-1],S,Native); if(Why) { return Finish(EnumaInheritedRefreshMode,Why,Reason); } }
      continue;
    }
    ENUMA_U32 LayoutReason=EnumaLayoutOther;
    Status=Native==3?EnumaResolveMixerGeometry(Scanout,&S->Mixers,L,&Limits,&LayoutReason):EnumaResolveMixerOriginsDetailed(Scanout,&S->Mixers,L,&Limits,&LayoutReason);
    if(Status) {
#if ENUMA_NATIVE_ROUTE_BLEND
      if(Native==2&&Status==EnumaInheritedTopology&&LayoutReason==EnumaLayoutBlend) {
        return Finish(Status,EnumaVideoReasonBlendOther+EnumaClassifyVideoBlend(Scanout,&S->Mixers),Reason);
      }
#endif
#if ENUMA_NATIVE_ROUTE_LAYOUT
      if(Native==2) { return Finish(Status,EnumaVideoReasonLayoutOther+LayoutReason,Reason); }
#endif
      return Finish(Status,Native?EnumaVideoReasonNativeMixerLayout:EnumaVideoReasonPipeline,Reason);
    }
    Status=EnumaDecodeInherited(Scanout,&Limits,&Frame); if(Status) { return Finish(Status,Native?EnumaVideoReasonNativeDecode:EnumaVideoReasonPipeline,Reason); }
    if(N) { ENUMA_U32 Why=StableReason(&E.Samples[N-1],S,Native); if(Why) { return Finish(EnumaInheritedRefreshMode,Why,Reason); } }
    if(!N) { E.Limits=Limits; E.Frame=Frame; }
  }
  if(Native>=4) {
    Status=EnumaNormalizeVideoBorder(&E.Samples[0].Scanout,&View);
    if(Status) { return Finish(Status,EnumaVideoReasonNativeStaging,Reason); }
    if(Native>=5) {
      Status=EnumaNormalizeGlobalMixerOutput(&View,&E.Samples[0].Mixers,L,&Global);
      if(Status) { return Finish(Status,EnumaVideoReasonNativeMixerLayout,Reason); }
      View=Global;
    }
    if(Native==6) {
      Status=EnumaNormalize8888AlphaView(&View,&Global);
      if(Status) { return Finish(Status,EnumaVideoReasonNativeMixerLayout,Reason); }
      View=Global;
    }
    Status=EnumaResolveMixerGeometry(&View,&E.Samples[0].Mixers,L,&Limits,0);
    if(Status) {
      return Finish(Status,Native==4&&Status==EnumaInheritedTopology?EnumaVideoReasonGeometryOther+EnumaClassifyMixerGeometry(&View,&E.Samples[0].Mixers,L):EnumaVideoReasonNativeMixerLayout,Reason);
    }
    Status=EnumaDecodeInherited(&View,&Limits,&Frame);
    if(Status) { return Finish(Status,EnumaVideoReasonNativeDecode,Reason); }
    E.Limits=Limits;E.Frame=Frame;
  }
  // Stable shadow configuration is not a downstream/physical address proof.
  // Clear only the demonstrated activity/blend/video counter requirements.
  E.Frame.Requirements=ENUMA_REQUIRE_ADDRESS|ENUMA_REQUIRE_TOPOLOGY|(Native>=3?ENUMA_REQUIRE_BLEND:0);
  for(I=0;I<sizeof(E);I++) { ((volatile unsigned char *)Out)[I]=((const unsigned char *)&E)[I]; }
  return Finish(EnumaInheritedOk,EnumaVideoReasonComplete,Reason);
}
ENUMA_U32 EnumaSampleVideoDetailed(ENUMA_INHERITED_READ Read,ENUMA_INHERITED_WAIT Wait,void *Ctx,const ENUMA_INHERITED_LIMITS *L,ENUMA_VIDEO_EVIDENCE *Out,ENUMA_U32 *Reason) {
  return SampleDetailed(Read,Wait,Ctx,L,Out,Reason,0);
}
ENUMA_U32 EnumaSampleNativeVideoDetailed(ENUMA_INHERITED_READ Read,ENUMA_INHERITED_WAIT Wait,void *Ctx,const ENUMA_INHERITED_LIMITS *L,ENUMA_VIDEO_EVIDENCE *Out,ENUMA_U32 *Reason) {
  return SampleDetailed(Read,Wait,Ctx,L,Out,Reason,1);
}
ENUMA_U32 EnumaSampleVideo(ENUMA_INHERITED_READ Read,ENUMA_INHERITED_WAIT Wait,void *Ctx,const ENUMA_INHERITED_LIMITS *L,ENUMA_VIDEO_EVIDENCE *Out) {
  return EnumaSampleVideoDetailed(Read,Wait,Ctx,L,Out,0);
}

ENUMA_U32 EnumaSampleNativeBorderVideoDetailed(ENUMA_INHERITED_READ Read,ENUMA_INHERITED_WAIT Wait,void *Ctx,const ENUMA_INHERITED_LIMITS *L,ENUMA_VIDEO_EVIDENCE *Out,ENUMA_U32 *Reason) {
  return SampleDetailed(Read,Wait,Ctx,L,Out,Reason,2);
}

ENUMA_U32 EnumaSampleNativeStructureDetailed(ENUMA_INHERITED_READ Read,ENUMA_INHERITED_WAIT Wait,void *Ctx,const ENUMA_INHERITED_LIMITS *L,ENUMA_VIDEO_EVIDENCE *Out,ENUMA_U32 *Reason) { return SampleDetailed(Read,Wait,Ctx,L,Out,Reason,3); }

ENUMA_U32 EnumaSampleNativeGeometryDetailed(ENUMA_INHERITED_READ Read,ENUMA_INHERITED_WAIT Wait,void *Ctx,const ENUMA_INHERITED_LIMITS *L,ENUMA_VIDEO_EVIDENCE *Out,ENUMA_U32 *Reason) { return SampleDetailed(Read,Wait,Ctx,L,Out,Reason,4); }

ENUMA_U32 EnumaSampleNativeGlobalOutputDetailed(ENUMA_INHERITED_READ Read,ENUMA_INHERITED_WAIT Wait,void *Ctx,const ENUMA_INHERITED_LIMITS *L,ENUMA_VIDEO_EVIDENCE *Out,ENUMA_U32 *Reason) { return SampleDetailed(Read,Wait,Ctx,L,Out,Reason,5); }

ENUMA_U32 EnumaSampleNativeAlphaViewDetailed(ENUMA_INHERITED_READ Read,ENUMA_INHERITED_WAIT Wait,void *Ctx,const ENUMA_INHERITED_LIMITS *L,ENUMA_VIDEO_EVIDENCE *Out,ENUMA_U32 *Reason) { return SampleDetailed(Read,Wait,Ctx,L,Out,Reason,6); }
