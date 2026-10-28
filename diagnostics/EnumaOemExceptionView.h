// SPDX-License-Identifier: BSD-2-Clause-Patent
// Bounded numeric exception snapshot and UEFI debug-table attribution. No I/O.
#ifndef ENUMA_OEM_EXCEPTION_VIEW_H
#define ENUMA_OEM_EXCEPTION_VIEW_H
#include <Uefi.h>
#include <Protocol/DebugSupport.h>
#include <Guid/DebugImageInfoTable.h>
#include <Protocol/DevicePath.h>
#ifndef ENUMA_OEM_CONTEXT
#define ENUMA_OEM_CONTEXT 0
#endif
typedef BOOLEAN (*ENUMA_EX_READ)(VOID *,UINTN,VOID *,UINTN);
typedef struct {
  UINT64 Type,Pc,Esr,Far,Lr,Spsr,Session,Base,Bytes,Rva;
  UINT32 Owner; // 0 unavailable, 1 unique, 2 no match, 3 ambiguous, 4 busy, 5 invalid, 6 limit
  BOOLEAN GuidValid;
  EFI_GUID Guid;
#if ENUMA_OEM_CONTEXT
  UINT64 X0,X8,X9,X10,X20,X21,El,Ttbr0,Ttbr1,Tcr,Sctlr,ParR,ParW,ProbeValid;
#endif
} ENUMA_EXCEPTION_RECORD;
STATIC BOOLEAN ExEqual(CONST VOID *A,CONST VOID *B,UINTN N){
  CONST UINT8 *X=A,*Y=B;for(UINTN I=0;I<N;I++){if(X[I]!=Y[I]){return FALSE;}}return TRUE;
}
STATIC __attribute__((noinline)) BOOLEAN ExSnapshot(ENUMA_EXCEPTION_RECORD *R,UINTN Type,
    CONST EFI_SYSTEM_CONTEXT_AARCH64 *C,UINT64 Session){
  if(!R||!C||!Session||Type>3){return FALSE;}
  *R=(ENUMA_EXCEPTION_RECORD){0};R->Type=Type;R->Pc=C->ELR;R->Esr=C->ESR;
  R->Far=C->FAR;R->Lr=C->LR;R->Spsr=C->SPSR;R->Session=Session;return TRUE;
}
STATIC VOID ExForgetOwner(ENUMA_EXCEPTION_RECORD *R,UINT32 State){
  R->Owner=State;R->Base=R->Bytes=R->Rva=0;R->GuidValid=FALSE;R->Guid=(EFI_GUID){0};
}
STATIC __attribute__((noinline)) VOID ExFindOwner(ENUMA_EXCEPTION_RECORD *R,
    UINTN System,ENUMA_EX_READ Read,VOID *Ctx){
  STATIC CONST EFI_GUID Debug=EFI_DEBUG_IMAGE_INFO_TABLE_GUID;
  EFI_SYSTEM_TABLE St;EFI_CONFIGURATION_TABLE Ct;EFI_DEBUG_IMAGE_INFO_TABLE_HEADER H,After;
  EFI_DEBUG_IMAGE_INFO Entry;EFI_DEBUG_IMAGE_INFO_NORMAL Normal;
  EFI_LOADED_IMAGE_PROTOCOL L,Chosen={0};UINTN Header=0,Found=0;
  if(!R||!Read){return;}ExForgetOwner(R,5);
  if(!Read(Ctx,System,&St,sizeof(St))||St.Hdr.Signature!=EFI_SYSTEM_TABLE_SIGNATURE||
     St.Hdr.HeaderSize<sizeof(St)||St.Hdr.HeaderSize>4096||St.NumberOfTableEntries>64){return;}
  for(UINTN I=0;I<St.NumberOfTableEntries;I++){
    UINTN A=(UINTN)St.ConfigurationTable;
    if(A>MAX_UINTN-I*sizeof(Ct)||!Read(Ctx,A+I*sizeof(Ct),&Ct,sizeof(Ct))){return;}
    if(ExEqual(&Ct.VendorGuid,&Debug,sizeof(Debug))){if(Header){return;}Header=(UINTN)Ct.VendorTable;}
  }
  if(!Header||!Read(Ctx,Header,&H,sizeof(H))){return;}
  if(H.UpdateStatus&EFI_DEBUG_IMAGE_INFO_UPDATE_IN_PROGRESS){ExForgetOwner(R,4);return;}
  if(H.TableSize>128){ExForgetOwner(R,6);return;}
  if(H.TableSize&&!H.EfiDebugImageInfoTable){return;}
  for(UINTN I=0;I<H.TableSize;I++){
    UINTN A=(UINTN)H.EfiDebugImageInfoTable;
    if(A>MAX_UINTN-I*sizeof(Entry)||!Read(Ctx,A+I*sizeof(Entry),&Entry,sizeof(Entry))){return;}
    if(!Entry.NormalImage){continue;}
    if(!Read(Ctx,(UINTN)Entry.NormalImage,&Normal,sizeof(Normal))||
       Normal.ImageInfoType!=EFI_DEBUG_IMAGE_INFO_TYPE_NORMAL||
       !Read(Ctx,(UINTN)Normal.LoadedImageProtocolInstance,&L,sizeof(L))){return;}
    if(!L.ImageBase||!L.ImageSize||L.ImageSize>0x4000000ULL||
       (UINTN)L.ImageBase>MAX_UINTN-L.ImageSize){return;}
    if(R->Pc>=(UINTN)L.ImageBase&&R->Pc-(UINTN)L.ImageBase<L.ImageSize){Chosen=L;Found++;}
  }
  if(!Read(Ctx,Header,&After,sizeof(After))||!ExEqual(&H,&After,sizeof(H))){return;}
  if(Found!=1){ExForgetOwner(R,Found?3:2);return;}
  R->Owner=1;R->Base=(UINTN)Chosen.ImageBase;R->Bytes=Chosen.ImageSize;R->Rva=R->Pc-R->Base;
  // Fixed maximum 8 nodes / 256 bytes. No PE/PDB parsing or arbitrary strings.
  UINTN A=(UINTN)Chosen.FilePath,Used=0;EFI_DEVICE_PATH_PROTOCOL D;EFI_GUID Selected={0};BOOLEAN Has=FALSE;
  for(UINTN I=0;I<8;I++){
    if(!A||!Read(Ctx,A,&D,sizeof(D))){return;}
    UINTN N=D.Length[0]+((UINTN)D.Length[1]<<8);
    if(N<4||N>256-Used||A>MAX_UINTN-N){return;}
    if(D.Type==MEDIA_DEVICE_PATH&&D.SubType==MEDIA_PIWG_FW_FILE_DP){
      EFI_GUID G;
      if(N!=20||!Read(Ctx,A+4,&G,sizeof(G))||Has){return;}
      Selected=G;Has=TRUE;
    }
    if(D.Type==END_DEVICE_PATH_TYPE){
      if(D.SubType==END_ENTIRE_DEVICE_PATH_SUBTYPE&&N==4&&Has){R->Guid=Selected;R->GuidValid=TRUE;}return;
    }
    Used+=N;A+=N;
  }
  R->GuidValid=FALSE;
}
#endif
