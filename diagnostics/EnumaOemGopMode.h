// SPDX-License-Identifier: BSD-2-Clause-Patent
// Post-DXE only. Call the authenticated OEM GOP; never draw or touch MDP here.
#ifndef ENUMA_OEM_GOP_MODE_H
#define ENUMA_OEM_GOP_MODE_H
#include <Uefi.h>
#include <Protocol/GraphicsOutput.h>
#include <Library/BaseMemoryLib.h>

typedef struct {
  UINT32 Phase;
  EFI_STATUS LocateStatus,QueryStatus,SetStatus,CleanupStatus;
  UINT64 FrameBase,FrameBytes;
  UINT32 Width,Height,Stride,Format;
  BOOLEAN Attempted,Validated;
} ENUMA_GOP_MODE_RESULT;
STATIC ENUMA_GOP_MODE_RESULT mOemGopMode;
STATIC UINT64 mOemGopMemory[8192]; // bounded 64 KiB data buffer, not the stack
STATIC UINTN mOemGopMapBytes,mOemGopDescriptorBytes;

STATIC __attribute__((noinline)) BOOLEAN OemGopMap(EFI_BOOT_SERVICES *B){
  UINTN Key=0;UINT32 Version=0;EFI_STATUS S;
  mOemGopMapBytes=sizeof(mOemGopMemory);mOemGopDescriptorBytes=0;
  S=B->GetMemoryMap(&mOemGopMapBytes,(EFI_MEMORY_DESCRIPTOR *)mOemGopMemory,
                    &Key,&mOemGopDescriptorBytes,&Version);
  return S==EFI_SUCCESS&&Version==EFI_MEMORY_DESCRIPTOR_VERSION&&
    mOemGopDescriptorBytes>=sizeof(EFI_MEMORY_DESCRIPTOR)&&mOemGopDescriptorBytes<=128&&
    !(mOemGopDescriptorBytes&7)&&mOemGopMapBytes<=sizeof(mOemGopMemory)&&
    mOemGopMapBytes&&mOemGopMapBytes%mOemGopDescriptorBytes==0;
}
STATIC __attribute__((noinline)) BOOLEAN OemGopSpan(CONST VOID *P,UINTN N){
  UINTN A=(UINTN)P;
  if(!A||!N||A>MAX_UINTN-N||!mOemGopDescriptorBytes||
     mOemGopMapBytes>sizeof(mOemGopMemory)){return FALSE;}
  for(UINTN I=0;I<mOemGopMapBytes;I+=mOemGopDescriptorBytes){
    CONST EFI_MEMORY_DESCRIPTOR *D=(CONST VOID *)((CONST UINT8 *)mOemGopMemory+I);
    UINT64 Bytes;
    if(D->Type!=EfiBootServicesCode&&D->Type!=EfiBootServicesData&&
       D->Type!=EfiRuntimeServicesCode&&D->Type!=EfiRuntimeServicesData){continue;}
    if(D->NumberOfPages>MAX_UINT64/EFI_PAGE_SIZE){continue;}
    Bytes=D->NumberOfPages*EFI_PAGE_SIZE;
    if(D->PhysicalStart<=A&&A-D->PhysicalStart<=Bytes&&N<=Bytes-(A-D->PhysicalStart)){return TRUE;}
  }
  return FALSE;
}
STATIC __attribute__((noinline)) BOOLEAN OemGopCode(UINTN P,UINTN Base,UINTN Bytes){
  return Base&&Bytes&&Base<=MAX_UINTN-Bytes&&P>=Base&&P-Base<Bytes&&!(P&3);
}
STATIC __attribute__((noinline)) BOOLEAN OemGopOwner(EFI_GRAPHICS_OUTPUT_PROTOCOL *G,UINTN Base,UINTN Bytes){
  return OemGopSpan(G,sizeof(*G))&&
    OemGopCode((UINTN)G->QueryMode,Base,Bytes)&&
    OemGopCode((UINTN)G->SetMode,Base,Bytes)&&
    OemGopCode((UINTN)G->Blt,Base,Bytes);
}
STATIC __attribute__((noinline)) BOOLEAN OemGopInfo(CONST EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *I){
  if(I->Version||I->HorizontalResolution!=1600||I->VerticalResolution!=2560||
     I->PixelsPerScanLine<1600||I->PixelsPerScanLine>4096){return FALSE;}
  if(I->PixelFormat==PixelRedGreenBlueReserved8BitPerColor||
     I->PixelFormat==PixelBlueGreenRedReserved8BitPerColor){return TRUE;}
  return I->PixelFormat==PixelBitMask&&I->PixelInformation.GreenMask==0xff00&&
    ((I->PixelInformation.RedMask==0xff&&I->PixelInformation.BlueMask==0xff0000)||
     (I->PixelInformation.RedMask==0xff0000&&I->PixelInformation.BlueMask==0xff))&&
    (I->PixelInformation.ReservedMask==0||I->PixelInformation.ReservedMask==0xff000000);
}
STATIC __attribute__((noinline)) BOOLEAN OemGopFinal(EFI_GRAPHICS_OUTPUT_PROTOCOL *G){
  EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *M=G->Mode;
  UINT64 Need;
  if(!OemGopSpan(M,sizeof(*M))||M->Mode!=0||!M->MaxMode||M->MaxMode>8||
     M->SizeOfInfo<sizeof(*M->Info)||M->SizeOfInfo>4096||
     !OemGopSpan(M->Info,sizeof(*M->Info))||!OemGopInfo(M->Info)){return FALSE;}
  Need=(UINT64)M->Info->PixelsPerScanLine*2560*4;
  // Existing enuma reservation; this code neither changes it nor dereferences FB.
  if((M->FrameBufferBase&3)||M->FrameBufferBase<0x9c000000ULL||
     M->FrameBufferBase>=0x9e400000ULL||M->FrameBufferSize<Need||
     M->FrameBufferSize>0x9e400000ULL-M->FrameBufferBase){return FALSE;}
  mOemGopMode.FrameBase=M->FrameBufferBase;mOemGopMode.FrameBytes=M->FrameBufferSize;
  mOemGopMode.Width=M->Info->HorizontalResolution;mOemGopMode.Height=M->Info->VerticalResolution;
  mOemGopMode.Stride=M->Info->PixelsPerScanLine;mOemGopMode.Format=M->Info->PixelFormat;
  return TRUE;
}
STATIC __attribute__((noinline)) BOOLEAN OemGopAdvance(EFI_BOOT_SERVICES *B,UINTN Base,UINTN Bytes){
  STATIC CONST EFI_GUID Guid=EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
  EFI_HANDLE *Handles=NULL;UINTN Count=0,Matches=0,InfoBytes=0;
  EFI_GRAPHICS_OUTPUT_PROTOCOL *G=NULL,*Candidate=NULL;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Info=NULL;EFI_TPL Tpl;BOOLEAN Good=FALSE;
  if(mOemGopMode.Attempted){return FALSE;}mOemGopMode.Attempted=TRUE;
  mOemGopMode.Phase=1;
  if(!B||!B->GetMemoryMap||!B->LocateHandleBuffer||!B->HandleProtocol||!B->FreePool||
     !B->RaiseTPL||!B->RestoreTPL||!Base||!Bytes||Base>MAX_UINTN-Bytes){return FALSE;}
  Tpl=B->RaiseTPL(TPL_HIGH_LEVEL);B->RestoreTPL(Tpl);
  if(Tpl!=TPL_APPLICATION){return FALSE;}
  mOemGopMode.LocateStatus=B->LocateHandleBuffer(ByProtocol,(EFI_GUID *)&Guid,NULL,&Count,&Handles);
  if(mOemGopMode.LocateStatus!=EFI_SUCCESS){return FALSE;}
  if(!Handles){return FALSE;}
  if(!Count||Count>8||!OemGopMap(B)||!OemGopSpan(Handles,Count*sizeof(*Handles))){goto Done;}
  for(UINTN I=0;I<Count;I++){
    Candidate=NULL;
    if(B->HandleProtocol(Handles[I],(EFI_GUID *)&Guid,(VOID **)&Candidate)!=EFI_SUCCESS){continue;}
    if(OemGopMap(B)&&OemGopOwner(Candidate,Base,Bytes)){G=Candidate;Matches++;}
  }
  if(Matches!=1||!OemGopMap(B)||!OemGopOwner(G,Base,Bytes)||
     !OemGopSpan(G->Mode,sizeof(*G->Mode))||!G->Mode->MaxMode||G->Mode->MaxMode>8){goto Done;}
  mOemGopMode.Phase=2;
  mOemGopMode.QueryStatus=G->QueryMode(G,0,&InfoBytes,&Info);
  if(mOemGopMode.QueryStatus!=EFI_SUCCESS||!Info){goto Done;}
  // Only free a real successful native allocation after checking its RAM span.
  // An invalid returned pointer is left untouched until the terminal reset.
  if(!OemGopMap(B)||!OemGopSpan(Info,sizeof(*Info))){goto Done;}
  Good=InfoBytes>=sizeof(*Info)&&InfoBytes<=4096&&OemGopInfo(Info);
  mOemGopMode.CleanupStatus=B->FreePool(Info);Info=NULL;
  if(!Good||mOemGopMode.CleanupStatus!=EFI_SUCCESS){Good=FALSE;goto Done;}
  Good=FALSE;
  if(!OemGopMap(B)||!OemGopOwner(G,Base,Bytes)){goto Done;}
  mOemGopMode.Phase=3;
  mOemGopMode.SetStatus=G->SetMode(G,0); // exactly one real call, never fabricated
  if(mOemGopMode.SetStatus!=EFI_SUCCESS){goto Done;}
  mOemGopMode.Phase=4;
  if(OemGopMap(B)&&OemGopOwner(G,Base,Bytes)){Good=OemGopFinal(G);}
Done:
  if(OemGopMap(B)&&OemGopSpan(Handles,sizeof(*Handles))){
    mOemGopMode.CleanupStatus=B->FreePool(Handles);
    if(mOemGopMode.CleanupStatus!=EFI_SUCCESS){Good=FALSE;}
  }else{Good=FALSE;} // do not free an unvalidated pointer; bounded until reset
  mOemGopMode.Validated=Good;
  if(Good){mOemGopMode.Phase=5;}
  return Good;
}
#endif
