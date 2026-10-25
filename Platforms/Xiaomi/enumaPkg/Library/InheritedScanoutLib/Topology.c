// SPDX-License-Identifier: BSD-2-Clause-Patent
// Register facts: pinned Xiaomi SDE LM setup_out/SDM845 blend configuration.
// No kernel implementation imported. No MMIO, pixel access or protocol install.
#include "Topology.h"
static ENUMA_U32 Stage(const ENUMA_U32 *L,ENUMA_U32 P) {
  static const ENUMA_U32 Shift[6]={0,3,6,26,18,21};
  if(P<6) { return ((L[0]>>Shift[P])&7)|(((L[1]>>(P<4?P*2:16+(P-4)*2))&1)<<3); }
  return (L[2]>>((P-6)*4))&15;
}
static ENUMA_U32 CaptureMask(ENUMA_U32 Mask,ENUMA_INHERITED_READ Read,void *Ctx,ENUMA_MIXER_SNAPSHOT *Out) {
  ENUMA_MIXER_SNAPSHOT S; ENUMA_U32 M,J,B;
  if(!Read||!Out) { return EnumaInheritedArguments; }
  for(J=0;J<sizeof(S);J++) { ((volatile unsigned char *)&S)[J]=0; }
  for(M=0;M<6;M++) {
    if(!(Mask&(1u<<M))) { continue; }
    B=0x45000+M*0x1000;
#define TAKE(o,f) do { if(Read(Ctx,(o),&(f))!=1) { return EnumaInheritedBounds; } } while(0)
    TAKE(B,S.Mixers[M].Mode); TAKE(B+4,S.Mixers[M].Size);
    for(J=0;J<11;J++) { TAKE(B+0x20+J*0x18,S.Mixers[M].Blend[J]); TAKE(B+0x24+J*0x18,S.Mixers[M].Alpha[J]); }
#undef TAKE
  }
  for(J=0;J<sizeof(S);J++) { ((volatile unsigned char *)Out)[J]=((const unsigned char *)&S)[J]; }
  return EnumaInheritedOk;
}
ENUMA_U32 EnumaCaptureMixers(ENUMA_INHERITED_READ Read,void *Ctx,ENUMA_MIXER_SNAPSHOT *Out) {
  return CaptureMask(63,Read,Ctx,Out);
}
ENUMA_U32 EnumaCaptureRoutedMixers(const ENUMA_INHERITED_SNAPSHOT *S,ENUMA_INHERITED_READ Read,void *Ctx,ENUMA_MIXER_SNAPSHOT *Out) {
  ENUMA_U32 C,M,J,Count=0,Mixers=0,Mask=0;
  if(!S||!Read||!Out) { return EnumaInheritedArguments; }
  for(C=0;C<6;C++) {
    if(!S->Ctls[C].IntfActive) { continue; }
    if(++Count!=1||S->Ctls[C].IntfActive!=6||(S->Ctls[C].Top&(1u<<17))) { return EnumaInheritedTopology; }
    for(M=0;M<6;M++) {
      ENUMA_U32 Any=0;
      for(J=0;J<4;J++) { Any|=S->Ctls[C].Layers[M][J]; }
      if(Any) { Mask|=1u<<M; Mixers++; }
    }
  }
  // Never probe blend registers of idle mixers or an unsupported CTL topology.
  if(Count!=1||Mixers!=2) { return EnumaInheritedTopology; }
  return CaptureMask(Mask,Read,Ctx,Out);
}
ENUMA_U32 EnumaResolveMixerOrigins(const ENUMA_INHERITED_SNAPSHOT *S,const ENUMA_MIXER_SNAPSHOT *T,const ENUMA_INHERITED_LIMITS *Input,ENUMA_INHERITED_LIMITS *Out) {
  ENUMA_INHERITED_LIMITS L; ENUMA_U32 C,M,P,J,Mask=0,Count=0,Left=6,Right=6,Used=0,Owners[8]={0};
  if(!S||!T||!Input||!Out) { return EnumaInheritedArguments; }
  if((S->Revision>>16)!=0x6000||Input->Mode!=ENUMA_MODE_VIDEO||Input->Width==0||Input->Height==0) { return EnumaInheritedTopology; }
  L=*Input; L.OriginMask=0;
  for(P=0;P<8;P++) { L.OriginX[P]=0; L.OriginY[P]=0; }
  // Exactly one video CTL driving both DSI interfaces. Other topologies are
  // unsupported, not guessed. This is a precondition, not downstream proof.
  for(C=0;C<6;C++) {
    if(!S->Ctls[C].IntfActive) { continue; }
    if(S->Ctls[C].IntfActive!=6||(S->Ctls[C].Top&(1u<<17))||++Count!=1) { return EnumaInheritedTopology; }
    Used=C;
    for(M=0;M<6;M++) {
      const ENUMA_U32 *V=S->Ctls[C].Layers[M]; ENUMA_U32 Sources=0,Pipe=8,Encoded=0,Expected[4]={0};
      // Exactly one known non-virtual source, no border/unknown staging.
      for(P=0;P<8;P++) { ENUMA_U32 E=Stage(V,P); if(E) { ++Sources; Pipe=P; Encoded=E; } }
      if(!Sources) { for(J=0;J<4;J++) { if(V[J]) { return EnumaInheritedTopology; } } continue; }
      if(Sources!=1||Encoded>12) { return EnumaInheritedTopology; }
      if(Pipe<6) {
        static const ENUMA_U32 Shift[6]={0,3,6,26,18,21};
        Expected[0]=(Encoded&7)<<Shift[Pipe]; Expected[1]=(Encoded>>3)<<(Pipe<4?Pipe*2:16+(Pipe-4)*2);
      } else { Expected[2]=Encoded<<((Pipe-6)*4); }
      for(J=0;J<4;J++) { if(V[J]!=Expected[J]) { return EnumaInheritedTopology; } }
      if(Owners[Pipe]) { return EnumaInheritedTopology; } Owners[Pipe]=1; Mask|=1u<<Pipe;
      // No dim stage, exact constant FG255/BG0 for non-base stage.
      for(J=0;J<11;J++) { if(T->Mixers[M].Blend[J]&(1u<<16)) { return EnumaInheritedTopology; } }
      if(Encoded>1&&(T->Mixers[M].Blend[Encoded-2]!=0x100||T->Mixers[M].Alpha[Encoded-2]!=0x00ff0000)) { return EnumaInheritedTopology; }
      // Reject unreviewed LM processing bits. bit30 is not silently accepted.
      if(T->Mixers[M].Mode&~(1u<<31)) { return EnumaInheritedTopology; }
      if((T->Mixers[M].Size>>16)!=Input->Height||!(T->Mixers[M].Size&0xffff)) { return EnumaInheritedTopology; }
      if(T->Mixers[M].Mode&(1u<<31)) { if(Right!=6) { return EnumaInheritedTopology; } Right=M; L.OriginX[Pipe]=~0u; }
      else { if(Left!=6) { return EnumaInheritedTopology; } Left=M; }
      // Each source fills its local mixer; partial update/scaler paths rejected.
      if(S->Pipes[Pipe].OutXy||S->Pipes[Pipe].OutSize!=T->Mixers[M].Size) { return EnumaInheritedTopology; }
    }
  }
  (void)Used;
  if(Count!=1||Left==6||Right==6||
     (T->Mixers[Left].Size&0xffff)+(T->Mixers[Right].Size&0xffff)!=Input->Width) { return EnumaInheritedTopology; }
  for(P=0;P<8;P++) { if(L.OriginX[P]==~0u) { L.OriginX[P]=T->Mixers[Left].Size&0xffff; } }
  L.OriginMask=Mask;
  *Out=L;
  return EnumaInheritedOk;
}
