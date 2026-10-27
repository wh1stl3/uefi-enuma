// SPDX-License-Identifier: BSD-2-Clause-Patent
// An owned read-only memory volume, not a partition or a filesystem driver.
// Its only file is the reviewed menu configuration. No fallback payload files.
#ifndef ENUMA_MENU_CONFIG_FS_H
#define ENUMA_MENU_CONFIG_FS_H
#include <Protocol/SimpleFileSystem.h>
#include <Guid/FileInfo.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>

#if ENUMA_DEFERRED_MENU
#define ENUMA_MENU_ENTRY_TITLE "Driver returned - Return to firmware"
#else
#define ENUMA_MENU_ENTRY_TITLE "Return to firmware"
#endif
STATIC CONST CHAR8 mMenuConfig[]=
  "version = 1\n[boot]\ndefault = \"return\"\ntimeout = 8\nfallback_timeout = 8\nconnect_all = false\n"
  "[ui]\ntitle = \"UniXNU\"\nconsole = \"graphics\"\nlayout = \"landscape\"\nscale = 1\nlogo = true\n"
  "[[entry]]\nid = \"return\"\ntitle = \"" ENUMA_MENU_ENTRY_TITLE "\"\ntype = \"action\"\naction = \"exit\"\n";
typedef struct {
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL Protocol;
  UINTN OpenCount,ReadBytes;
} ENUMA_MENU_CONFIG_FS;
typedef struct {
  EFI_FILE_PROTOCOL Protocol;
  ENUMA_MENU_CONFIG_FS *Fs;
  UINT64 Position;
  BOOLEAN Root;
} ENUMA_MENU_FILE;
STATIC EFI_STATUS MenuFileNew(ENUMA_MENU_CONFIG_FS *,BOOLEAN,EFI_FILE_PROTOCOL **);
STATIC EFI_STATUS EFIAPI MenuFileOpen(EFI_FILE_PROTOCOL *This,EFI_FILE_PROTOCOL **New,
  CHAR16 *Name,UINT64 Mode,UINT64 Attributes) {
  ENUMA_MENU_FILE *File=(ENUMA_MENU_FILE *)This;(VOID)Attributes;
  if(!New||!Name){return EFI_INVALID_PARAMETER;}*New=NULL;
  if(Mode!=EFI_FILE_MODE_READ){return EFI_WRITE_PROTECTED;}
  if(!File->Root){return EFI_NOT_FOUND;}
  if(StrCmp(Name,L"\\EFI\\UniXNU\\unixnu.toml")!=0){return EFI_NOT_FOUND;}
  return MenuFileNew(File->Fs,FALSE,New);
}
STATIC EFI_STATUS EFIAPI MenuFileClose(EFI_FILE_PROTOCOL *This) {
  ENUMA_MENU_FILE *File=(ENUMA_MENU_FILE *)This;
  File->Fs->OpenCount--;FreePool(File);return EFI_SUCCESS;
}
STATIC EFI_STATUS EFIAPI MenuFileDelete(EFI_FILE_PROTOCOL *This) {
  MenuFileClose(This);return EFI_WARN_DELETE_FAILURE;
}
STATIC EFI_STATUS EFIAPI MenuFileRead(EFI_FILE_PROTOCOL *This,UINTN *Size,VOID *Buffer) {
  ENUMA_MENU_FILE *File=(ENUMA_MENU_FILE *)This;UINTN Count;
  if(!Size||(*Size&&!Buffer)){return EFI_INVALID_PARAMETER;}
  if(File->Root){*Size=0;return EFI_SUCCESS;}
  Count=MIN(*Size,sizeof(mMenuConfig)-1-(UINTN)File->Position);
  if(Count){CopyMem(Buffer,mMenuConfig+(UINTN)File->Position,Count);}
  *Size=Count;File->Position+=Count;File->Fs->ReadBytes+=Count;return EFI_SUCCESS;
}
STATIC EFI_STATUS EFIAPI MenuFileWrite(EFI_FILE_PROTOCOL *This,UINTN *Size,VOID *Buffer) {
  (VOID)This;(VOID)Size;(VOID)Buffer;return EFI_WRITE_PROTECTED;
}
STATIC EFI_STATUS EFIAPI MenuFilePosition(EFI_FILE_PROTOCOL *This,UINT64 *Position) {
  if(!Position){return EFI_INVALID_PARAMETER;}*Position=((ENUMA_MENU_FILE *)This)->Position;return EFI_SUCCESS;
}
STATIC EFI_STATUS EFIAPI MenuFileSeek(EFI_FILE_PROTOCOL *This,UINT64 Position) {
  ENUMA_MENU_FILE *File=(ENUMA_MENU_FILE *)This;
  if(File->Root&&Position){return EFI_UNSUPPORTED;}
  if(Position==MAX_UINT64){Position=sizeof(mMenuConfig)-1;}
  if(Position>sizeof(mMenuConfig)-1){return EFI_INVALID_PARAMETER;}
  File->Position=Position;return EFI_SUCCESS;
}
STATIC EFI_STATUS EFIAPI MenuFileInfo(EFI_FILE_PROTOCOL *This,EFI_GUID *Type,UINTN *Size,VOID *Buffer) {
  EFI_GUID FileInfo=EFI_FILE_INFO_ID;EFI_FILE_INFO *Info;ENUMA_MENU_FILE *File=(ENUMA_MENU_FILE *)This;
  UINTN Needed=SIZE_OF_EFI_FILE_INFO+sizeof(L"unixnu.toml");
  if(!Type||!Size){return EFI_INVALID_PARAMETER;}
  if(!CompareGuid(Type,&FileInfo)){return EFI_UNSUPPORTED;}
  if(*Size<Needed){*Size=Needed;return EFI_BUFFER_TOO_SMALL;}
  if(!Buffer){return EFI_INVALID_PARAMETER;}ZeroMem(Buffer,Needed);Info=Buffer;
  Info->Size=Needed;Info->FileSize=File->Root?0:sizeof(mMenuConfig)-1;Info->PhysicalSize=Info->FileSize;
  Info->Attribute=EFI_FILE_READ_ONLY|(File->Root?EFI_FILE_DIRECTORY:0);
  CopyMem(Info->FileName,L"unixnu.toml",sizeof(L"unixnu.toml"));*Size=Needed;return EFI_SUCCESS;
}
STATIC EFI_STATUS EFIAPI MenuFileSetInfo(EFI_FILE_PROTOCOL *This,EFI_GUID *Type,UINTN Size,VOID *Buffer) {
  (VOID)This;(VOID)Type;(VOID)Size;(VOID)Buffer;return EFI_WRITE_PROTECTED;
}
STATIC EFI_STATUS EFIAPI MenuFileFlush(EFI_FILE_PROTOCOL *This) {(VOID)This;return EFI_SUCCESS;}
STATIC EFI_STATUS MenuFileNew(ENUMA_MENU_CONFIG_FS *Fs,BOOLEAN Root,EFI_FILE_PROTOCOL **Out) {
  ENUMA_MENU_FILE *File;
  if(!Fs||!Out){return EFI_INVALID_PARAMETER;}*Out=NULL;
  if(Fs->OpenCount>=8){return EFI_OUT_OF_RESOURCES;}
  File=AllocateZeroPool(sizeof(*File));if(!File){return EFI_OUT_OF_RESOURCES;}
  File->Fs=Fs;File->Root=Root;File->Protocol.Revision=EFI_FILE_PROTOCOL_REVISION;
  File->Protocol.Open=MenuFileOpen;File->Protocol.Close=MenuFileClose;File->Protocol.Delete=MenuFileDelete;
  File->Protocol.Read=MenuFileRead;File->Protocol.Write=MenuFileWrite;File->Protocol.GetPosition=MenuFilePosition;
  File->Protocol.SetPosition=MenuFileSeek;File->Protocol.GetInfo=MenuFileInfo;File->Protocol.SetInfo=MenuFileSetInfo;
  File->Protocol.Flush=MenuFileFlush;Fs->OpenCount++;*Out=&File->Protocol;return EFI_SUCCESS;
}
STATIC EFI_STATUS EFIAPI MenuConfigOpenVolume(EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *This,EFI_FILE_PROTOCOL **Root) {
  return MenuFileNew((ENUMA_MENU_CONFIG_FS *)This,TRUE,Root);
}
STATIC VOID MenuConfigInit(ENUMA_MENU_CONFIG_FS *Fs) {
  ZeroMem(Fs,sizeof(*Fs));Fs->Protocol.Revision=EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_REVISION;
  Fs->Protocol.OpenVolume=MenuConfigOpenVolume;
}
#endif
