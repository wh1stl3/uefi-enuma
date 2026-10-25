// SPDX-License-Identifier: BSD-2-Clause-Patent
// Snapshot adapter: callback only, no direct MMIO or pixel access. At most267 reads.
#include "InheritedScanout.h"
ENUMA_U32 EnumaCaptureInherited(ENUMA_INHERITED_READ Read,void *Context,ENUMA_INHERITED_SNAPSHOT *Out) {
  static const ENUMA_U32 Pipes[8]={0x5000,0x7000,0x9000,0xb000,0x25000,0x27000,0x29000,0x2b000};
  static const ENUMA_U32 Fields[11]={0,8,12,16,20,36,48,52,56,0x170,0x70};
  static const ENUMA_U32 Intfs[2]={0x6b800,0x6c000};
  ENUMA_INHERITED_SNAPSHOT S; ENUMA_U32 I,J,M,B;
  if (!Read || !Out) { return EnumaInheritedArguments; }
  for(I=0;I<sizeof(S);I++) { ((volatile unsigned char *)&S)[I]=0; }
#define TAKE(offset,field) do { if (Read(Context,(offset),&(field)) != 1) { return EnumaInheritedBounds; } } while(0)
  TAKE(0,S.Revision);
  if ((S.Revision >>16)!=0x6000) { return EnumaInheritedRevision; }
  for(I=0;I<8;I++) {
    ENUMA_INHERITED_PIPE *P=&S.Pipes[I];
    ENUMA_U32 *Values[11]={&P->Size,&P->Xy,&P->OutSize,&P->OutXy,&P->Address,&P->Stride,
      &P->Format,&P->Unpack,&P->OpMode,&P->Multirect,&P->Secure};
    for(J=0;J<11;J++) { TAKE(Pipes[I]+Fields[J],*Values[J]); }
  }
  for(I=0;I<6;I++) {
    B=0x2000+I*0x200; TAKE(B+0x14,S.Ctls[I].Top); TAKE(B+0xf4,S.Ctls[I].IntfActive);
    TAKE(B+0x1c,S.Ctls[I].Start); TAKE(B+0x18,S.Ctls[I].Flush);
    for(M=0;M<6;M++) { TAKE(B+(M==5?0x24:M*4),S.Ctls[I].Layers[M][0]);
      TAKE(B+0x40+M*4,S.Ctls[I].Layers[M][1]); TAKE(B+0x70+M*4,S.Ctls[I].Layers[M][2]); TAKE(B+0xa0+M*4,S.Ctls[I].Layers[M][3]); }
  }
  for(I=0;I<2;I++) { TAKE(Intfs[I]+0x2b4,S.IntfAutoRefresh[I]); TAKE(Intfs[I]+0x284,S.IntfTearEnable[I]); }
  for(I=0;I<6;I++) { TAKE(0x71000+I*0x800+0x30,S.PpAutoRefresh[I]); }
#undef TAKE
  for(I=0;I<sizeof(S);I++) { ((volatile unsigned char *)Out)[I]=((const unsigned char *)&S)[I]; }
  return EnumaInheritedOk;
}
