// SPDX-License-Identifier: BSD-2-Clause-Patent
// Video-only config capture after live interface eligibility. Up to53 reads:
// six ACTIVE words, one CTL_TOP, 24 route words, only two selected SSPP sets.
// No CTL transient, command refresh, PP or unused SSPP register accesses.
#include "Evidence.h"
ENUMA_U32 EnumaCaptureVideoConfigDetailed(ENUMA_INHERITED_READ Read,void *Ctx,ENUMA_U32 Revision,ENUMA_INHERITED_SNAPSHOT *Out,ENUMA_VIDEO_SCOPE *Scope,ENUMA_U32 *Reason) {
  static const ENUMA_U32 Pipes[8]={0x5000,0x7000,0x9000,0xb000,0x25000,0x27000,0x29000,0x2b000};
  static const ENUMA_U32 Fields[11]={0,8,12,16,20,36,48,52,56,0x170,0x70};
  ENUMA_INHERITED_SNAPSHOT S; ENUMA_U32 C=0,I,J,M,Count=0,Mask,PipeMask,Status,B;
  if(Reason) { *Reason=EnumaVideoReasonPipeline; }
  if(!Read||!Out||!Scope) { return EnumaInheritedArguments; }
  if((Revision>>16)!=0x6000) { return EnumaInheritedRevision; }
  for(I=0;I<sizeof(S);I++) { ((volatile unsigned char *)&S)[I]=0; } S.Revision=Revision;
#define TAKE(o,f) do { if(Read(Ctx,(o),&(f))!=1) { return EnumaInheritedBounds; } } while(0)
  if(Reason) { *Reason=EnumaVideoReasonNativeCtl; }
  for(I=0;I<6;I++) {
    TAKE(0x2000+I*0x200+0xf4,S.Ctls[I].IntfActive);
    if(S.Ctls[I].IntfActive) { if(S.Ctls[I].IntfActive!=6||++Count!=1) { return EnumaInheritedTopology; } C=I; }
  }
  if(Count!=1) { return EnumaInheritedTopology; }
  B=0x2000+C*0x200; TAKE(B+0x14,S.Ctls[C].Top);
  if(S.Ctls[C].Top&(1u<<17)) { return EnumaInheritedMode; }
  for(M=0;M<6;M++) {
    TAKE(B+(M==5?0x24:M*4),S.Ctls[C].Layers[M][0]); TAKE(B+0x40+M*4,S.Ctls[C].Layers[M][1]);
    TAKE(B+0x70+M*4,S.Ctls[C].Layers[M][2]); TAKE(B+0xa0+M*4,S.Ctls[C].Layers[M][3]);
  }
  if(Reason) { *Reason=EnumaVideoReasonNativeStaging; }
  Status=EnumaSelectVideoRoute(&S,&C,&Mask,&PipeMask); if(Status) { return Status; }
  if(Reason) { *Reason=EnumaVideoReasonNativePipe; }
  for(I=0;I<8;I++) {
    ENUMA_INHERITED_PIPE *P=&S.Pipes[I];
    ENUMA_U32 *V[11]={&P->Size,&P->Xy,&P->OutSize,&P->OutXy,&P->Address,&P->Stride,&P->Format,&P->Unpack,&P->OpMode,&P->Multirect,&P->Secure};
    if(!(PipeMask&(1u<<I))) { continue; }
    for(J=0;J<11;J++) { TAKE(Pipes[I]+Fields[J],*V[J]); }
  }
#undef TAKE
  for(I=0;I<sizeof(S);I++) { ((volatile unsigned char *)Out)[I]=((const unsigned char *)&S)[I]; }
  Scope->CtlActiveMask=63; Scope->CtlRouteMask=1u<<C; Scope->PipeMask=PipeMask;
  Scope->MixerMask=Mask; Scope->CommandRefreshMask=0; Scope->CtlTransientMask=0;
  return EnumaInheritedOk;
}
ENUMA_U32 EnumaCaptureVideoConfig(ENUMA_INHERITED_READ Read,void *Ctx,ENUMA_U32 Revision,ENUMA_INHERITED_SNAPSHOT *Out,ENUMA_VIDEO_SCOPE *Scope) {
  return EnumaCaptureVideoConfigDetailed(Read,Ctx,Revision,Out,Scope,0);
}
