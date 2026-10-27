// SPDX-License-Identifier: BSD-2-Clause-Patent
// Validate the text callbacks and geometry consumed by the pinned UniXNU UI.
// Does not connect a console, select a mode, clear pixels, or qualify visibility.
#ifndef ENUMA_MENU_CONSOLE_H
#define ENUMA_MENU_CONSOLE_H
#include <Uefi.h>
#include <Protocol/SimpleTextOut.h>
#include <Protocol/GraphicsOutput.h>
// Fixed Mu GraphicsConsoleInitializeModeData requires 80x25 cells of 8x19.
// This is a connection preflight, NOT GOP or physical scanout admission.
STATIC inline EFI_STATUS EnumaMenuGraphicsConsoleCheck(CONST EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop) {
  if (!Gop || !Gop->Mode || !Gop->Mode->Info || !Gop->QueryMode || !Gop->SetMode || !Gop->Blt ||
      Gop->Mode->SizeOfInfo<sizeof(*Gop->Mode->Info) || !Gop->Mode->MaxMode || Gop->Mode->Mode>=Gop->Mode->MaxMode) {
    return EFI_NOT_READY;
  }
  return Gop->Mode->Info->HorizontalResolution>=640 && Gop->Mode->Info->VerticalResolution>=475?EFI_SUCCESS:EFI_UNSUPPORTED;
}
STATIC inline EFI_STATUS EnumaMenuTextConsoleCheck(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *Output) {
  UINTN Columns=0,Rows=0; EFI_STATUS Status;
  if (!Output) { return EFI_NOT_FOUND; } // GOP/headless remains an independent path.
  if (!Output->Mode || !Output->QueryMode || !Output->OutputString ||
      !Output->SetAttribute || !Output->SetCursorPosition || !Output->ClearScreen || !Output->EnableCursor) {
    return EFI_NOT_READY;
  }
  if (Output->Mode->Mode<0 || Output->Mode->Mode>=Output->Mode->MaxMode) { return EFI_NOT_READY; }
  Status=Output->QueryMode(Output,(UINTN)Output->Mode->Mode,&Columns,&Rows);
  if (Status!=EFI_SUCCESS) { return Status; }
  // TextPrintAt subtracts one column; menu footer subtracts five rows. Keep
  // space for title, provenance, entries and the three status lines as well.
  return Columns>=2 && Rows>=8?EFI_SUCCESS:EFI_UNSUPPORTED;
}
#endif
