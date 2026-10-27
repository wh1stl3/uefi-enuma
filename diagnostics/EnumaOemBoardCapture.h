// SPDX-License-Identifier: BSD-2-Clause-Patent
// Compile-only board adapter. No GOP/ConOut. Runtime ownership, exception and
// recovery qualification are still required before removing the stage10 gate.
#ifndef ENUMA_OEM_BOARD_CAPTURE_H
#define ENUMA_OEM_BOARD_CAPTURE_H
#include <Library/MemoryMapHelperLib.h>
#include <Library/ResetSystemLib.h>
#include <Library/BaseLib.h>
#include <Library/TimerLib.h>
#ifndef ENUMA_OEM_RUNTIME
#define ENUMA_OEM_RUNTIME 0
#endif
#undef ENUMA_BOOT_LOG_REPLAY
#undef ENUMA_BOOT_LOG_HOLD_US
#define ENUMA_BOOT_LOG_REPLAY 3
#define ENUMA_BOOT_LOG_HOLD_US 8000000
#undef ENUMA_NATIVE_ROUTE_CPU_POLICY
#define ENUMA_NATIVE_ROUTE_CPU_POLICY 1
#include "OemInherited/Decode.c"
#include "OemInherited/VideoCapture.c"
#include "OemInherited/Topology.c"
#include "OemInherited/Evidence.c"
#include "OemInherited/CpuStage1.h"
#include "EnumaOemCapturePixel.h"
STATIC ENUMA_CAPTURE_PIXEL mOemPixel;
STATIC ENUMA_VIDEO_EVIDENCE mOemEvidence,mOemVerify;
STATIC ENUMA_INHERITED_LIMITS mOemLimits;
STATIC UINT32 mOemReads,mOemWaits,mOemPixelReads,mOemWrites,mOemBarriers,mOemHolds,mOemValidations;
STATIC EFI_BOOT_SERVICES *mOemOriginalBoot;
STATIC int OemRead(VOID *C,ENUMA_U32 Offset,ENUMA_U32 *V){
  (VOID)C;if(!V||(Offset&3)||Offset>0x6c26c||mOemReads>=660){return 0;}
  mOemReads++;*V=*(volatile UINT32 *)(UINTN)(0xae00000U+Offset);return 1;
}
STATIC int OemWait(VOID *C,ENUMA_U32 Us){
  (VOID)C;if(Us!=100000||mOemWaits>=4){return 0;}mOemWaits++;
  if(ENUMA_OEM_RUNTIME){MicroSecondDelay(Us);return 1;}
  return mOemOriginalBoot->Stall(Us)==EFI_SUCCESS;
}
STATIC int OemAddress(ENUMA_U64 A){
  CONST ENUMA_INHERITED_FRAME *F=&mOemPixel.Frame;ENUMA_U64 O;
  if(!EnumaPixelFrameValid(F)||(A&3)||A<F->Base||A<0x9c000000ULL||A>0x9e3ffffcULL){return 0;}
  O=A-F->Base;return O/F->Stride>=96&&O/F->Stride<256&&O%F->Stride>=256&&O%F->Stride<1536;
}
STATIC int OemPixelRead(VOID *C,ENUMA_U64 A,ENUMA_U32 *V){
  (VOID)C;if(!V||!OemAddress(A)||mOemPixelReads>=3*ENUMA_PIXEL_WORDS){return 0;}
  mOemPixelReads++;*V=*(volatile UINT32 *)(UINTN)A;return 1;
}
STATIC int OemPixelWrite(VOID *C,ENUMA_U64 A,ENUMA_U32 V){
  (VOID)C;if(!OemAddress(A)||mOemWrites>=2*ENUMA_PIXEL_WORDS){return 0;}
  mOemWrites++;*(volatile UINT32 *)(UINTN)A=V;return 1;
}
STATIC int OemBarrier(VOID *C){
  (VOID)C;if(mOemBarriers>=3){return 0;}mOemBarriers++;__asm__ volatile("dsb sy" ::: "memory");return 1;
}
STATIC int OemHold(VOID *C,ENUMA_U32 Us){
  (VOID)C;if(mOemHolds||Us!=8000000){return 0;}mOemHolds++;
  if(ENUMA_OEM_RUNTIME){MicroSecondDelay(Us);return 1;}
  return mOemOriginalBoot->Stall(Us)==EFI_SUCCESS;
}
STATIC int OemValidate(VOID *C,CONST ENUMA_INHERITED_FRAME *F){
  ENUMA_U32 Reason=0,Status;ENUMA_U64 El=0;(VOID)C;
  if(mOemValidations||!EnumaPixelBytesEqual(F,&mOemEvidence.Frame,sizeof(*F))){return 0;}
  mOemValidations++;
  Status=EnumaSampleNativeAlphaViewDetailed(OemRead,OemWait,NULL,&mOemLimits,&mOemVerify,&Reason);
  return Status==EnumaInheritedOk&&Reason==EnumaVideoReasonComplete&&
    EnumaPixelEvidenceSame(&mOemEvidence,&mOemVerify)&&EnumaCpuStage1ArmDetailed(F,&El)==3&&El==4;
}
STATIC VOID OemCaptureTerminal(VOID *C){
  (VOID)C;ResetCold();for(;;){CpuPause();}
}
STATIC BOOLEAN OemBoardSinkInner(VOID *Opaque,CONST CHAR8 *Line,UINTN Size){
  ENUMA_PRECALL_CAPTURE *Capture=Opaque;EFI_MEMORY_REGION_DESCRIPTOR Mdp={0},Splash={0};
  STATIC CONST CHAR8 End[]="OEM capture end ";UINTN I;ENUMA_U32 Reason=0,Status;
  if(Size<sizeof(End)-1){return FALSE;}
  for(I=0;I<sizeof(End)-1;I++){if(Line[I]!=End[I]){return TRUE;}}
  // No I/O until the selected snapshot has reached its footer. The original
  // boot table is used directly, so recording cannot recurse through wrappers.
  if(mOemPixel.Attempted||!mOemOriginalBoot||!mOemOriginalBoot->Stall){return FALSE;}
  if(LocateMemoryRegionByName("Display MDP",&Mdp)!=EFI_SUCCESS||Mdp.Address!=0xae00000||Mdp.Length!=0x85000||
     Mdp.ArmAttributes!=NS_DEVICE||Mdp.ResourceAttribute!=UNCACHEABLE||Mdp.MemoryType!=MmIO||Mdp.HobOption!=AddDev||
     LocateMemoryRegionByName("Display Reserved",&Splash)!=EFI_SUCCESS||Splash.Address!=0x9c000000||Splash.Length!=0x2400000){return FALSE;}
  mOemLimits.Width=1600;mOemLimits.Height=2560;mOemLimits.SplashBase=Splash.Address;
  mOemLimits.SplashBytes=Splash.Length;mOemLimits.Mode=ENUMA_MODE_VIDEO;
  Status=EnumaSampleNativeAlphaViewDetailed(OemRead,OemWait,NULL,&mOemLimits,&mOemEvidence,&Reason);
  if(Status!=EnumaInheritedOk||Reason!=EnumaVideoReasonComplete){return FALSE;}
  mOemPixel.Frame=mOemEvidence.Frame;mOemPixel.Policy=EnumaCpuStage1ArmDetailed(&mOemPixel.Frame,&mOemPixel.El);
  mOemPixel.Capture=Capture;mOemPixel.Read=OemPixelRead;mOemPixel.Write=OemPixelWrite;
  mOemPixel.Validate=OemValidate;mOemPixel.Barrier=OemBarrier;mOemPixel.Hold=OemHold;
  mOemPixel.Stop=OemCaptureTerminal;
  return EnumaCapturePixelSink(&mOemPixel,Line,Size);
}
STATIC BOOLEAN OemBoardSink(VOID *Opaque,CONST CHAR8 *Line,UINTN Size){
  UINT64 Saved,Affinity;BOOLEAN Result;
  if(!ENUMA_OEM_RUNTIME){return OemBoardSinkInner(Opaque,Line,Size);}
  // The first wrapper is on the BSP. Mask local asynchronous CPU writers for
  // the entire bounded transaction; timer polling does not call Boot Services.
  // This does not claim ownership against external DMA or an active AP.
  __asm__ volatile("mrs %0, mpidr_el1" : "=r"(Affinity));
  if(Affinity&0xff00ffffffULL){return FALSE;}
  __asm__ volatile("mrs %0, daif\nmsr daifset, #15" : "=r"(Saved) :: "memory");
  Result=OemBoardSinkInner(Opaque,Line,Size);
  __asm__ volatile("msr daif, %0" :: "r"(Saved) : "memory");
  return Result;
}
#endif
