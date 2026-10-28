// SPDX-License-Identifier: BSD-2-Clause-Patent
// Exception-only terminal capture, existing 320x160 ROI and route policy.
#ifndef ENUMA_OEM_EXCEPTION_DISPLAY_H
#define ENUMA_OEM_EXCEPTION_DISPLAY_H
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/TimerLib.h>
#undef ENUMA_BOOT_LOG_REPLAY
#define ENUMA_BOOT_LOG_REPLAY 3
#undef ENUMA_BOOT_LOG_HOLD_US
#define ENUMA_BOOT_LOG_HOLD_US 8000000
#undef ENUMA_NATIVE_ROUTE_CPU_POLICY
#define ENUMA_NATIVE_ROUTE_CPU_POLICY 1
#define ENUMA_CAPTURE_PREPARED_ONLY 1
#include "OemInherited/Decode.c"
#include "OemInherited/VideoCapture.c"
#include "OemInherited/Topology.c"
#include "OemInherited/Evidence.c"
#include "OemInherited/CpuStage1.h"
#include "EnumaOemCapturePixel.h"
#include "EnumaOemExceptionView.h"
STATIC ENUMA_CAPTURE_PIXEL mExPixel;
STATIC ENUMA_VIDEO_EVIDENCE mExEvidence,mExVerify;
STATIC ENUMA_INHERITED_LIMITS mExLimits;
STATIC UINT32 mExReads,mExWaits,mExPixelReads,mExWrites,mExBarriers,mExHolds,mExValidations;
STATIC __attribute__((noinline)) BOOLEAN ExMapped(UINTN A,UINTN N,BOOLEAN Device){
  UINT64 El,Sctlr,Tcr,Mmfr,Par,Saved;UINTN Last;
  if(!A||!N||N>4096||A>MAX_UINTN-N){return FALSE;}
  __asm__ volatile("mrs %0, CurrentEL\nmrs %1, id_aa64mmfr0_el1":"=r"(El),"=r"(Mmfr));
  if(El!=4||(Mmfr&15)>5){return FALSE;}
  __asm__ volatile("mrs %0, sctlr_el1\nmrs %1, tcr_el1":"=r"(Sctlr),"=r"(Tcr));
  if(!(Sctlr&1)||(Tcr&((1ULL<<39)|(1ULL<<40)))){return FALSE;}
  if(Device){if(A<0xae00000||A+N>0xae85000){return FALSE;}}
  else if(!((A>=0x98900000&&A+N<=0x9c000000)||
            (A>=0x9f800000&&A+N<=0x9ff8c000)||
            (A>=0x9ff90000&&A+N<=0x9ffd0000))){return FALSE;}
  Last=(A+N-1)&~(UINTN)4095;
  __asm__ volatile("mrs %0, par_el1":"=r"(Saved));
  for(UINTN P=A&~(UINTN)4095;;P+=4096){
    __asm__ volatile("at s1e1r, %1\nisb\nmrs %0, par_el1":"=r"(Par):"r"(P):"memory");
    UINT32 Attr=(UINT32)(Par>>56);
    BOOLEAN Good=!(Par&1)&&(Par&0x000ffffffffff000ULL)==P&&
      (Device?(Attr==0||Attr==4||Attr==8||Attr==12):
       (EnumaCpuPolicyNibble(Attr>>4)>=2&&EnumaCpuPolicyNibble(Attr>>4)<=4&&
        EnumaCpuPolicyNibble(Attr&15)>=2&&EnumaCpuPolicyNibble(Attr&15)<=4));
    if(!Good){__asm__ volatile("msr par_el1, %0"::"r"(Saved):"memory");return FALSE;}
    if(P==Last){break;}
  }
  __asm__ volatile("msr par_el1, %0"::"r"(Saved):"memory");return TRUE;
}
STATIC __attribute__((noinline)) BOOLEAN ExReadMeta(VOID *C,UINTN A,VOID *D,UINTN N){
  (VOID)C;if(!D||!ExMapped(A,N,FALSE)){return FALSE;}
  for(UINTN I=0;I<N;I++){((UINT8 *)D)[I]=((CONST volatile UINT8 *)A)[I];}return TRUE;
}
STATIC int ExRead(VOID *C,ENUMA_U32 O,ENUMA_U32 *V){
  (VOID)C;if(!V||(O&3)||O>0x6c26c||mExReads>=660||!ExMapped(0xae00000U+O,4,TRUE)){return 0;}
  mExReads++;*V=*(volatile UINT32 *)(UINTN)(0xae00000U+O);return 1;
}
STATIC int ExWait(VOID *C,ENUMA_U32 Us){
  (VOID)C;if(Us!=100000||mExWaits>=4){return 0;}mExWaits++;MicroSecondDelay(Us);return 1;
}
STATIC int ExAddress(ENUMA_U64 A){
  CONST ENUMA_INHERITED_FRAME *F=&mExPixel.Frame;ENUMA_U64 O;
  if(!EnumaPixelFrameValid(F)||(A&3)||A<F->Base||A<0x9c000000ULL||A>0x9e3ffffcULL){return 0;}
  O=A-F->Base;return O/F->Stride>=96&&O/F->Stride<256&&O%F->Stride>=256&&O%F->Stride<1536;
}
STATIC int ExPixelRead(VOID *C,ENUMA_U64 A,ENUMA_U32 *V){
  (VOID)C;if(!V||!ExAddress(A)||mExPixelReads>=3*ENUMA_PIXEL_WORDS){return 0;}
  mExPixelReads++;*V=*(volatile UINT32 *)(UINTN)A;return 1;
}
STATIC int ExPixelWrite(VOID *C,ENUMA_U64 A,ENUMA_U32 V){
  (VOID)C;if(!ExAddress(A)||mExWrites>=2*ENUMA_PIXEL_WORDS){return 0;}
  mExWrites++;*(volatile UINT32 *)(UINTN)A=V;return 1;
}
STATIC int ExBarrier(VOID *C){(VOID)C;if(mExBarriers>=3){return 0;}mExBarriers++;__asm__ volatile("dsb sy":::"memory");return 1;}
STATIC int ExHold(VOID *C,ENUMA_U32 Us){(VOID)C;if(mExHolds||Us!=8000000){return 0;}mExHolds++;MicroSecondDelay(Us);return 1;}
STATIC int ExValidate(VOID *C,CONST ENUMA_INHERITED_FRAME *F){
  UINT32 Reason=0,Status;UINT64 El=0;(VOID)C;
  if(mExValidations||!EnumaPixelBytesEqual(F,&mExEvidence.Frame,sizeof(*F))){return 0;}mExValidations++;
  Status=EnumaSampleNativeAlphaViewDetailed(ExRead,ExWait,NULL,&mExLimits,&mExVerify,&Reason);
  return Status==EnumaInheritedOk&&Reason==EnumaVideoReasonComplete&&
    EnumaPixelEvidenceSame(&mExEvidence,&mExVerify)&&EnumaCpuStage1ArmDetailed(F,&El)==3&&El==4;
}
#include "EnumaOemExceptionPage.h"
STATIC __attribute__((noinline)) BOOLEAN ExPresent(CONST ENUMA_EXCEPTION_RECORD *R,UINT32 Page){
  UINT32 Reason=0,Status;UINT64 Affinity;
  __asm__ volatile("mrs %0, mpidr_el1\nmsr daifset, #15":"=r"(Affinity)::"memory");
  if(Affinity&0xff00ffffffULL||Page<1||Page>3){return FALSE;}
  ZeroMem(&mExPixel,sizeof(mExPixel));
  mExReads=mExWaits=mExPixelReads=mExWrites=mExBarriers=mExHolds=mExValidations=0;
  mExLimits=(ENUMA_INHERITED_LIMITS){0};mExLimits.Width=1600;mExLimits.Height=2560;
  mExLimits.SplashBase=0x9c000000;mExLimits.SplashBytes=0x2400000;mExLimits.Mode=ENUMA_MODE_VIDEO;
  Status=EnumaSampleNativeAlphaViewDetailed(ExRead,ExWait,NULL,&mExLimits,&mExEvidence,&Reason);
  if(Status!=EnumaInheritedOk||Reason!=EnumaVideoReasonComplete){return FALSE;}
  mExPixel.Frame=mExEvidence.Frame;mExPixel.Policy=EnumaCpuStage1ArmDetailed(&mExPixel.Frame,&mExPixel.El);
  mExPixel.Read=ExPixelRead;mExPixel.Write=ExPixelWrite;mExPixel.Validate=ExValidate;
  mExPixel.Barrier=ExBarrier;mExPixel.Hold=ExHold;mExPixel.PagePrepared=TRUE;
  ExPage(&mExPixel.Page,R,Page);return EnumaCapturePixelRun(&mExPixel)==5;
}
#endif
