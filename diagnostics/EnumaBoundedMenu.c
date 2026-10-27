// SPDX-License-Identifier: BSD-2-Clause-Patent
// Actual pinned UniXNU Ui/Menu on an opt-in, transient, bounded GOP window.
// ENUMA_FULL_MENU loads the real FV application while this window owns GOP.
// No production GOP admission, BDS launch, XNU, or physical input claim.
#include "UniXNUMenuInternal.h"
#include <Library/FrameBufferBltLib.h>
#include "InheritedVideo/MenuWindow.h"
#if ENUMA_FULL_MENU
#include "EnumaFullMenu.h"
#endif

STATIC ENUMA_MENU_WINDOW mWindow;
STATIC FRAME_BUFFER_CONFIGURE *mBlt;
STATIC UINT32 *mSurface;
STATIC EFI_GRAPHICS_OUTPUT_MODE_INFORMATION mInfo;
STATIC EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE mMode;
STATIC EFI_GRAPHICS_OUTPUT_PROTOCOL mGop;
#if !ENUMA_FULL_MENU
STATIC EFI_SYSTEM_TABLE mMenuSystemTable;
#endif

STATIC EFI_STATUS EFIAPI MenuQuery(EFI_GRAPHICS_OUTPUT_PROTOCOL *This,UINT32 Mode,UINTN *Size,EFI_GRAPHICS_OUTPUT_MODE_INFORMATION **Info) {
  if(This!=&mGop||!Size||!Info) {return EFI_INVALID_PARAMETER;}
  if(Mode) {return EFI_UNSUPPORTED;}
  *Info=AllocateCopyPool(sizeof(mInfo),&mInfo);
  if(!*Info) {return EFI_OUT_OF_RESOURCES;}
  *Size=sizeof(mInfo);return EFI_SUCCESS;
}
STATIC EFI_STATUS EFIAPI MenuBlt(EFI_GRAPHICS_OUTPUT_PROTOCOL *This,EFI_GRAPHICS_OUTPUT_BLT_PIXEL *Buffer,
  EFI_GRAPHICS_OUTPUT_BLT_OPERATION Operation,UINTN Sx,UINTN Sy,UINTN Dx,UINTN Dy,UINTN Width,UINTN Height,UINTN Delta) {
  EFI_STATUS Status;
  if(This!=&mGop||!mBlt||!mWindow.Ready||mWindow.Fault) {return EFI_NOT_READY;}
  Status=(EFI_STATUS)FrameBufferBlt(mBlt,Buffer,Operation,Sx,Sy,Dx,Dy,Width,Height,Delta);
  if(Status!=EFI_SUCCESS) {mWindow.Fault=1;return Status;}
  if(Operation==EfiBltVideoToBltBuffer) {return EFI_SUCCESS;}
  if(!EnumaMenuPresent(&mWindow,mSurface,(UINT32)Dx,(UINT32)Dy,(UINT32)Width,(UINT32)Height)) {return EFI_DEVICE_ERROR;}
  return EFI_SUCCESS;
}
STATIC EFI_STATUS EFIAPI MenuSet(EFI_GRAPHICS_OUTPUT_PROTOCOL *This,UINT32 Mode) {
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL Black={0};
  if(This!=&mGop||Mode) {return EFI_UNSUPPORTED;}
  return MenuBlt(This,&Black,EfiBltVideoFill,0,0,0,0,ENUMA_MENU_WIDTH,ENUMA_MENU_HEIGHT,0);
}

UINT32 EnumaBoundedMenu(const ENUMA_INHERITED_FRAME *Frame,
  ENUMA_PIXEL_READ Read,ENUMA_PIXEL_WRITE Write,ENUMA_PIXEL_BARRIER Barrier,
  ENUMA_PIXEL_REVALIDATE Validate,VOID *Context) {
  EFI_STATUS Status;EFI_HANDLE Handle=NULL;UINTN Size=0;UINT32 Result=1;
  UINT32 *Saved=NULL;EFI_SYSTEM_TABLE *Original=gST;
#if !ENUMA_FULL_MENU
  UINTN Index;UNIXNU_CONFIG Config={0};UNIXNU_UI Ui;UNIXNU_ENTRY Entries[2]={{0}};
#endif
  if(mSurface||mBlt||!Frame||mWindow.Ready||mWindow.Attempted) {return 1;}
#if ENUMA_FULL_MENU
  if(EFI_ERROR(FullMenuPrepare())) {goto Done;}
#endif
  Saved=AllocatePool(ENUMA_MENU_PIXELS*sizeof(UINT32));
  mSurface=AllocateZeroPool(ENUMA_MENU_PIXELS*sizeof(UINT32));
  if(!Saved||!mSurface) {goto Done;}
  ZeroMem(&mInfo,sizeof(mInfo));mInfo.HorizontalResolution=ENUMA_MENU_WIDTH;mInfo.VerticalResolution=ENUMA_MENU_HEIGHT;
  mInfo.PixelFormat=PixelBlueGreenRedReserved8BitPerColor;mInfo.PixelsPerScanLine=ENUMA_MENU_WIDTH;
  Status=(EFI_STATUS)FrameBufferBltConfigure(mSurface,&mInfo,NULL,&Size);
  if(Status!=EFI_BUFFER_TOO_SMALL||!Size) {goto Done;}
  mBlt=AllocatePool(Size);if(!mBlt) {goto Done;}
  if(FrameBufferBltConfigure(mSurface,&mInfo,mBlt,&Size)!=RETURN_SUCCESS) {goto Done;}
  mInfo.PixelFormat=PixelBltOnly;
  mMode.MaxMode=1;mMode.Mode=0;mMode.Info=&mInfo;mMode.SizeOfInfo=sizeof(mInfo);
  // Blt-only window: no raw physical address is exposed to consumers.
  mMode.FrameBufferBase=0;mMode.FrameBufferSize=0;
  mGop.QueryMode=MenuQuery;mGop.SetMode=MenuSet;mGop.Blt=MenuBlt;mGop.Mode=&mMode;
  if(!EnumaMenuSave(&mWindow,Frame,Saved,Read,Write,Barrier,Validate,Context)) {goto Done;}
  Status=gBS->InstallMultipleProtocolInterfaces(&Handle,&gEfiGraphicsOutputProtocolGuid,&mGop,NULL);
  if(Status!=EFI_SUCCESS) {goto Done;}
#if ENUMA_FULL_MENU
  if(EFI_ERROR(FullMenuRun(Handle))) {mWindow.Fault=1;}
#else
  CopyMem(&mMenuSystemTable,Original,sizeof(mMenuSystemTable));
  mMenuSystemTable.ConsoleOutHandle=Handle;mMenuSystemTable.ConOut=NULL;mMenuSystemTable.ConIn=NULL;
  gST=&mMenuSystemTable;
  Entries[0].Id="return";Entries[0].Title=L"Return to Android";Entries[0].Type=UniXNUEntryAction;
  Entries[1].Id="window";Entries[1].Title=L"GOP window test";Entries[1].Type=UniXNUEntryAction;
  Config.Title=L"UniXNU";Config.SourceLabel=L"enuma - bounded GOP experiment";
  Config.Entries=Entries;Config.EntryCount=2;Config.DefaultIndex=0;Config.Timeout=8;Config.FallbackTimeout=8;
  Config.Console=UniXNUConsoleGraphics;Config.Layout=UniXNULayoutAuto;Config.PortraitBelow=800;Config.Scale=2;
  UniXNUUiInit(&Config,&Ui);
  if(Ui.Mode==UniXNUUiGraphics&&Ui.Gop==&mGop) {
    Index=UniXNUMenuRun(&Ui,&Config,TRUE);
    if(Index!=0) {mWindow.Fault=1;}
  } else {mWindow.Fault=1;}
#endif
  gST=Original;
  Result=EnumaMenuRestore(&mWindow);
Done:
  gST=Original;
  // Revoke access even if protocol removal fails on a no-store setup path.
  mWindow.Ready=0;
#if ENUMA_FULL_MENU
  if(!FullMenuRelease()) {return Result==4?4:2;}
#endif
  if(Handle) {
    Status=gBS->UninstallMultipleProtocolInterfaces(Handle,&gEfiGraphicsOutputProtocolGuid,&mGop,NULL);
    if(Status!=EFI_SUCCESS) {return Result==4?4:2;} // Retain all referenced storage.
  }
  if(mBlt) {FreePool(mBlt);mBlt=NULL;}
  if(mSurface) {FreePool(mSurface);mSurface=NULL;}
  if(Saved) {FreePool(Saved);}
  return Result;
}
