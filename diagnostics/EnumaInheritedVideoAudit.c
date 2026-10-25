// SPDX-License-Identifier: BSD-2-Clause-Patent
// Opt-in READ ONLY audit entry, materialized instead of metadata SimpleFb.c.
// Keeps the existing audited entry/Core-warning transport; publishes NO GOP.
#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryMapHelperLib.h>
#include <Library/PcdLib.h>
#include "EnumaGopResult.h"
#include "InheritedVideo/Decode.c"
#include "InheritedVideo/Capture.c"
#include "InheritedVideo/Topology.c"
#include "InheritedVideo/Evidence.c"
STATIC UINTN mReads,mWaits;
STATIC ENUMA_VIDEO_EVIDENCE mEvidence;
STATIC int Read(void *Context,ENUMA_U32 Offset,ENUMA_U32 *Value) {
  (VOID)Context;
  if(!Value||(Offset&3)||Offset>0x73830||mReads>=969) { return 0; }
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
  EFI_MEMORY_REGION_DESCRIPTOR Mdp={0},Splash={0}; ENUMA_INHERITED_LIMITS L={0}; ENUMA_U32 Status;
  (VOID)Image; (VOID)ST;
  if(LocateMemoryRegionByName("Display MDP",&Mdp)!=EFI_SUCCESS||Mdp.Address!=0xae00000||Mdp.Length!=0x85000||
     Mdp.ArmAttributes!=NS_DEVICE||Mdp.ResourceAttribute!=UNCACHEABLE||Mdp.MemoryType!=MmIO||Mdp.HobOption!=AddDev||
     LocateMemoryRegionByName("Display Reserved",&Splash)!=EFI_SUCCESS||Splash.Address!=0x9c000000||Splash.Length!=0x2400000) { return ENUMA_GOP_RESULT_FAIL; }
  L.Width=FixedPcdGet32(PcdFrameBufferWidth); L.Height=FixedPcdGet32(PcdFrameBufferHeight);
  if(L.Width!=1600||L.Height!=2560) { return ENUMA_GOP_RESULT_FAIL; }
  L.SplashBase=Splash.Address; L.SplashBytes=Splash.Length; L.Mode=ENUMA_MODE_VIDEO;
  Status=EnumaSampleVideo(Read,Wait,NULL,&L,&mEvidence);
  // This is a configuration/counter predicate only, never GOP/address success.
  return Status==EnumaInheritedOk&&mEvidence.Frame.Requirements==(ENUMA_REQUIRE_ADDRESS|ENUMA_REQUIRE_TOPOLOGY)?ENUMA_GOP_RESULT_PASS:ENUMA_GOP_RESULT_FAIL;
}
