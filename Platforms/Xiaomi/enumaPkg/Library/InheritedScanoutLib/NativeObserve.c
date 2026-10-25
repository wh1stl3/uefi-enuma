// SPDX-License-Identifier: BSD-2-Clause-Patent
#include "NativeObserve.h"
ENUMA_U32 EnumaObserveNativeVideo(ENUMA_INHERITED_READ Read,ENUMA_INHERITED_WAIT Wait,void *Ctx,ENUMA_NATIVE_OBSERVATION *Out) {
  ENUMA_NATIVE_OBSERVATION O;
  ENUMA_U32 N,I,After;
  if(!Read||!Wait||!Out) { return EnumaInheritedArguments; }
  for(I=0;I<sizeof(O);I++) { ((volatile unsigned char *)&O)[I]=0; }
#define TAKE(off,dst) do { O.ReadCount++; if(Read(Ctx,(off),&(dst))!=1) { return EnumaInheritedBounds; } } while(0)
  TAKE(0,O.Revision);
  if((O.Revision>>16)!=0x6000) { return EnumaInheritedRevision; }
  O.ActiveThroughoutMask=O.TimingStableMask=O.EnableStableMask=O.MovingMask=O.ExactEnable3Mask=3;
  for(N=0;N<3;N++) {
    if(N) {
      O.WaitCount++;
      if(Wait(Ctx,100000)!=1) { return EnumaInheritedRefreshMode; }
    }
    for(I=0;I<2;I++) {
      ENUMA_U32 Base=0x6b800+I*0x800,Bit=1u<<I;
      ENUMA_VIDEO_INTF *V=&O.Samples[N].Interfaces[I];
      TAKE(Base,V->Timing); TAKE(Base+0xa8,V->CounterEnable); TAKE(Base+0x26c,V->Active);
      if(V->Active&1) { TAKE(Base+0xac,V->Frame); O.Samples[N].FrameReadMask|=Bit; }
      else { O.ActiveThroughoutMask&=~Bit; O.MovingMask&=~Bit; }
      if(!V->Timing) { O.TimingStableMask&=~Bit; }
      if((V->CounterEnable&3)!=3) { O.ExactEnable3Mask&=~Bit; }
      if(N) {
        const ENUMA_VIDEO_INTF *P=&O.Samples[N-1].Interfaces[I];
        if(P->Timing!=V->Timing) { O.TimingStableMask&=~Bit; }
        if(P->CounterEnable!=V->CounterEnable) { O.EnableStableMask&=~Bit; }
        if(!(O.Samples[N-1].FrameReadMask&Bit)||!(O.Samples[N].FrameReadMask&Bit)||
           V->Frame<=P->Frame||V->Frame-P->Frame>64) { O.MovingMask&=~Bit; }
      }
    }
  }
  TAKE(0,After);
  if(After!=O.Revision) { return EnumaInheritedInconsistent; }
#undef TAKE
  for(I=0;I<sizeof(O);I++) { ((volatile unsigned char *)Out)[I]=((const unsigned char *)&O)[I]; }
  // Complete observation, including negative masks, is not a display PASS.
  return EnumaInheritedOk;
}
