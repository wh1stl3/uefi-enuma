// SPDX-License-Identifier: BSD-2-Clause-Patent
// Explicit late ConOut consumer. Never ConnectController, SetMode, clear screen,
// raw framebuffer, DEBUG, or a fallback sink. Caller owns the quiescent snapshot.
#ifndef ENUMA_JOURNAL_CONSOLE_H
#define ENUMA_JOURNAL_CONSOLE_H
#include <Uefi.h>
#include <Protocol/SimpleTextOut.h>
#include "EnumaBootJournalProtocol.h"
#include "EnumaBootJournalView.h"

typedef struct {
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *Output;
  EFI_STATUS LastStatus;
} ENUMA_JOURNAL_CONSOLE;

STATIC inline EFI_STATUS EnumaConsoleWriteLine(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *Output, CONST CHAR16 *Text) {
  CHAR16 Buffer[130]; UINTN N=0;
  if (!Text) { return EFI_INVALID_PARAMETER; }
  if (!Output || !Output->OutputString) { return EFI_NOT_READY; }
  while (N<127 && Text[N]) { Buffer[N]=Text[N]; N++; }
  if (Text[N]) { return EFI_BAD_BUFFER_SIZE; }
  Buffer[N++]=L'\r'; Buffer[N++]=L'\n'; Buffer[N]=0;
  return Output->OutputString(Output,Buffer);
}

// A failed/warning output may already have emitted bytes. Latch it and never
// silently resume this session or replace its first failure with later success.
STATIC inline EFI_STATUS EnumaConsoleSessionWriteLine(ENUMA_JOURNAL_CONSOLE *Session, CONST CHAR16 *Text) {
  if (!Session) { return EFI_INVALID_PARAMETER; }
  if (Session->LastStatus!=EFI_SUCCESS) { return Session->LastStatus; }
  Session->LastStatus=EnumaConsoleWriteLine(Session->Output,Text);
  return Session->LastStatus;
}

STATIC inline BOOLEAN EnumaJournalConsoleSink(VOID *Context, CONST CHAR8 *Line, UINTN Bytes) {
  ENUMA_JOURNAL_CONSOLE *Sink=Context;
  CHAR16 Text[ENUMA_BOOT_VIEW_LINE_BYTES+1]; UINTN I,N=0;
  if (!Sink) { return FALSE; }
  Sink->LastStatus=EFI_INVALID_PARAMETER;
  if (!Line || !Bytes || Bytes>=ENUMA_BOOT_VIEW_LINE_BYTES) { return FALSE; }
  // Formatter emits exactly one printable ASCII line with a final LF.
  // Validate before making any OutputString call; CRLF is a single conversion.
  for (I=0; I<Bytes; I++) {
    if (I==Bytes-1 && Line[I]=='\n') { Text[N++]=L'\r'; Text[N++]=L'\n'; }
    else if ((UINT8)Line[I]>=0x20 && (UINT8)Line[I]<=0x7e) { Text[N++]=(UINT8)Line[I]; }
    else { return FALSE; }
  }
  if (Line[Bytes-1]!='\n') { return FALSE; }
  Text[N]=0;
  if (!Sink->Output || !Sink->Output->OutputString) { Sink->LastStatus=EFI_NOT_READY; return FALSE; }
  Sink->LastStatus=Sink->Output->OutputString(Sink->Output,Text);
  // Warnings such as unknown glyphs are not complete transcript acceptance.
  return Sink->LastStatus==EFI_SUCCESS;
}

// No lookup or writes until ABI/callback/console are checked. Workspace must be
// separate from Core storage. One caller-controlled line at a time thereafter.
STATIC inline EFI_STATUS EnumaJournalConsoleSnapshot(
  CONST ENUMA_BOOT_JOURNAL_PROTOCOL *Journal, EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *Output,
  ENUMA_BOOT_LOG *Snapshot) {
  if (!Journal || !Snapshot) { return EFI_INVALID_PARAMETER; }
  if (Journal->Version!=1 || Journal->Bytes!=sizeof(*Journal) || !Journal->Snapshot) { return EFI_INCOMPATIBLE_VERSION; }
  if (!Output || !Output->OutputString) { return EFI_NOT_READY; }
  if (!Journal->Snapshot(Snapshot)) { return EFI_DEVICE_ERROR; }
  return EnumaBootLogViewValid(Snapshot)?EFI_SUCCESS:EFI_COMPROMISED_DATA;
}

// One bounded replay: loss header plus at most128 records. Accepted counts only
// complete successful OutputString calls. On warning/error return immediately;
// caller must not automatically retry this snapshot or continue to the menu.
STATIC inline EFI_STATUS EnumaJournalConsoleDrain(CONST ENUMA_BOOT_LOG *Snapshot,
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *Output, UINT32 *Accepted) {
  ENUMA_JOURNAL_CONSOLE Sink={Output,EFI_SUCCESS}; UINT32 Cursor=0;
  if (!Accepted) { return EFI_INVALID_PARAMETER; }
  *Accepted=0;
  if (!EnumaBootLogViewValid(Snapshot)) { return EFI_COMPROMISED_DATA; }
  if (!Output || !Output->OutputString) { return EFI_NOT_READY; }
  while (Cursor<=Snapshot->Count) {
    if (!EnumaBootLogReplayAttributedNext(Snapshot,&Cursor,EnumaJournalConsoleSink,&Sink)) {
      *Accepted=Cursor;
      return Sink.LastStatus==EFI_SUCCESS?EFI_COMPROMISED_DATA:Sink.LastStatus;
    }
  }
  *Accepted=Cursor;return EFI_SUCCESS;
}
#endif
