// SPDX-License-Identifier: BSD-2-Clause-Patent
// Independent diagnostic only. No GOP, pixel access, or register writes.
#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryMapHelperLib.h>
#include <Library/PcdLib.h>
#include "EnumaGopResult.h"
#include "InheritedVideo/NativeObserve.c"
#if (ENUMA_NATIVE_OBSERVE_QUERY != 1 && ENUMA_NATIVE_OBSERVE_QUERY != 2) || !ENUMA_VIDEO_TERMINAL || ENUMA_VIDEO_TOPOLOGY_QUERY || ENUMA_VIDEO_REASON_TARGET
#error Native observer requires its own exact symmetric terminal profile
#endif
#if ENUMA_NATIVE_CONTROL != 0 && ENUMA_NATIVE_CONTROL != 1
#error Unsupported native constant control
#endif
#if ENUMA_NATIVE_MASK_FIELD < 0 || ENUMA_NATIVE_MASK_FIELD > 5 || (ENUMA_NATIVE_MASK_FIELD && (ENUMA_NATIVE_CONTROL || ENUMA_NATIVE_OBSERVE_QUERY != 1))
#error Unsupported native mask profile
#endif
#if ENUMA_NATIVE_MASK_FIELD
#include "InheritedVideo/NativeMaskResult.h"
#endif
#if !ENUMA_NATIVE_CONTROL
STATIC UINTN mReads,mWaits;
STATIC ENUMA_NATIVE_OBSERVATION mObservation;
STATIC int Read(void *Context,ENUMA_U32 Offset,ENUMA_U32 *Value) {
  ENUMA_U32 Local;
  (VOID)Context;
  if(!Value||mReads>=26) { return 0; }
  if(Offset!=0) {
    if(Offset>=0x6b800&&Offset<=0x6ba6c) { Local=Offset-0x6b800; }
    else if(Offset>=0x6c000&&Offset<=0x6c26c) { Local=Offset-0x6c000; }
    else { return 0; }
    if(Local!=0&&Local!=0xa8&&Local!=0xac&&Local!=0x26c) { return 0; }
  }
  mReads++; *Value=*(volatile UINT32 *)(UINTN)(0x0ae00000U+Offset); return 1;
}
STATIC int Wait(void *Context,ENUMA_U32 Us) {
  (VOID)Context;
  if(Us!=100000||mWaits>=2||!gBS||!gBS->Stall) { return 0; }
  mWaits++; return gBS->Stall(Us)==EFI_SUCCESS?1:0;
}
#endif
EFI_STATUS EFIAPI RegisterFrameBuffer(EFI_HANDLE Image,EFI_SYSTEM_TABLE *ST) {
#if ENUMA_NATIVE_CONTROL
  (VOID)Image; (VOID)ST;
  return ENUMA_NATIVE_OBSERVE_QUERY==1?ENUMA_GOP_RESULT_PASS:ENUMA_GOP_RESULT_FAIL;
#else
  EFI_MEMORY_REGION_DESCRIPTOR Mdp={0},Splash={0}; ENUMA_U32 Status;
#if ENUMA_NATIVE_MASK_FIELD
  ENUMA_U32 ResultCode=5;
#else
  BOOLEAN Match=FALSE;
#endif
  (VOID)Image; (VOID)ST;
  if(LocateMemoryRegionByName("Display MDP",&Mdp)==EFI_SUCCESS&&Mdp.Address==0xae00000&&Mdp.Length==0x85000&&
     Mdp.ArmAttributes==NS_DEVICE&&Mdp.ResourceAttribute==UNCACHEABLE&&Mdp.MemoryType==MmIO&&Mdp.HobOption==AddDev&&
     LocateMemoryRegionByName("Display Reserved",&Splash)==EFI_SUCCESS&&Splash.Address==0x9c000000&&Splash.Length==0x2400000&&
     FixedPcdGet32(PcdFrameBufferWidth)==1600&&FixedPcdGet32(PcdFrameBufferHeight)==2560) {
    Status=EnumaObserveNativeVideo(Read,Wait,NULL,&mObservation);
 #if ENUMA_NATIVE_MASK_FIELD
    ResultCode=EnumaNativeMaskResult(Status,&mObservation,ENUMA_NATIVE_MASK_FIELD);
 #else
    Match=(Status==EnumaInheritedOk&&mObservation.ActiveThroughoutMask==3&&mObservation.TimingStableMask==3&&
           mObservation.EnableStableMask==3&&mObservation.MovingMask==3);
 #endif
  }
  // ExactEnable3 is recorded separately, never a precondition here.
 #if ENUMA_NATIVE_MASK_FIELD
  return (EFI_STATUS)ResultCode;
 #else
  return (Match==(ENUMA_NATIVE_OBSERVE_QUERY==1))?ENUMA_GOP_RESULT_PASS:ENUMA_GOP_RESULT_FAIL;
 #endif
#endif
}
