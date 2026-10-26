// SPDX-License-Identifier: BSD-2-Clause-Patent
// DXE-local, single-writer RAM journal. No timer, allocation, MMIO or DEBUG.
// Caller owns storage and serializes access; not an exception/AP logger.
#ifndef ENUMA_BOOT_JOURNAL_H
#define ENUMA_BOOT_JOURNAL_H
#include <Uefi.h>

#define ENUMA_BOOT_LOG_CAPACITY 128U
#define ENUMA_BOOT_LOG_LINE_BYTES 128U
typedef struct {
  UINT64 Sequence;
  UINT64 Status;
  UINT32 Point;
  UINT32 Ordinal;
  UINT8 GuidLe[16];
  BOOLEAN HasGuid;
} ENUMA_BOOT_EVENT;

typedef struct {
  UINT64 NextSequence;
  UINT64 Dropped;
  UINT32 Head;
  UINT32 Count;
  ENUMA_BOOT_EVENT Events[ENUMA_BOOT_LOG_CAPACITY];
} ENUMA_BOOT_LOG;

// Zero-initialize once per boot. Latest 128 events retained; loss is explicit.
STATIC inline BOOLEAN EnumaBootLogAppend (
  ENUMA_BOOT_LOG *Log, UINT32 Point, UINT32 Ordinal,
  CONST EFI_GUID *Guid, UINT64 Status
  )
{
  ENUMA_BOOT_EVENT *Event;
  UINT32 Index;
  if (Log == NULL || Log->Head >= ENUMA_BOOT_LOG_CAPACITY ||
      Log->Count > ENUMA_BOOT_LOG_CAPACITY || Log->NextSequence == MAX_UINT64) {
    return FALSE;
  }
  Index = (Log->Head + Log->Count) % ENUMA_BOOT_LOG_CAPACITY;
  if (Log->Count == ENUMA_BOOT_LOG_CAPACITY) {
    Log->Head = (Log->Head + 1) % ENUMA_BOOT_LOG_CAPACITY;
    if (Log->Dropped != MAX_UINT64) { Log->Dropped++; }
  } else { Log->Count++; }
  Event = &Log->Events[Index];
  Event->Sequence = Log->NextSequence++;
  Event->Status = Status;
  Event->Point = Point;
  Event->Ordinal = Ordinal;
  Event->HasGuid = Guid != NULL;
  for (Index = 0; Index < 16; Index++) {
    Event->GuidLe[Index] = Guid == NULL ? 0 : ((CONST UINT8 *)Guid)[Index];
  }
  return TRUE;
}

STATIC inline VOID EnumaBootLogHex (CHAR8 *Text, UINT64 Value, UINT32 Digits)
{
  STATIC CONST CHAR8 Hex[] = "0123456789abcdef";
  UINT32 Index;
  for (Index = 0; Index < Digits; Index++) {
    Text[Digits - Index - 1] = Hex[Value & 15];
    Value >>= 4;
  }
}

// Snapshot must be a separate, immutable caller-owned copy, made while producer
// is quiescent. One call emits at most one line; caller controls cancellation.
// Cursor advances only on sink acceptance; retry cannot silently skip a line.
// Cursor 0 emits dropped count first, then retained events in sequence order.
typedef BOOLEAN (*ENUMA_BOOT_LOG_SINK)(VOID *Context, CONST CHAR8 *Line, UINTN Bytes);
STATIC inline BOOLEAN EnumaBootLogReplayNext (
  CONST ENUMA_BOOT_LOG *Snapshot, UINT32 *Cursor,
  ENUMA_BOOT_LOG_SINK Sink, VOID *Context
  )
{
  CHAR8 Line[ENUMA_BOOT_LOG_LINE_BYTES];
  CONST ENUMA_BOOT_EVENT *Event;
  UINT32 Index, Length;
  STATIC CONST CHAR8 LossPrefix[] = "DXE replay dropped=";
  STATIC CONST CHAR8 Prefix[] = "DXE seq=";
  if (Snapshot == NULL || Cursor == NULL || Sink == NULL ||
      Snapshot->Count > ENUMA_BOOT_LOG_CAPACITY ||
      Snapshot->Head >= ENUMA_BOOT_LOG_CAPACITY || *Cursor > Snapshot->Count) {
    return FALSE;
  }
  Length = 0;
  if (*Cursor == 0) {
    for (Index = 0; Index < sizeof (LossPrefix) - 1; Index++) { Line[Length++] = LossPrefix[Index]; }
    EnumaBootLogHex (&Line[Length], Snapshot->Dropped, 16); Length += 16;
  } else {
    Event = &Snapshot->Events[(Snapshot->Head + *Cursor - 1) % ENUMA_BOOT_LOG_CAPACITY];
    for (Index = 0; Index < sizeof (Prefix) - 1; Index++) { Line[Length++] = Prefix[Index]; }
    EnumaBootLogHex (&Line[Length], Event->Sequence, 16); Length += 16;
    Line[Length++] = ' '; Line[Length++] = 'p'; Line[Length++] = '=';
    EnumaBootLogHex (&Line[Length], Event->Point, 8); Length += 8;
    Line[Length++] = ' '; Line[Length++] = 'o'; Line[Length++] = '=';
    EnumaBootLogHex (&Line[Length], Event->Ordinal, 8); Length += 8;
    Line[Length++] = ' '; Line[Length++] = 's'; Line[Length++] = '=';
    EnumaBootLogHex (&Line[Length], Event->Status, 16); Length += 16;
    Line[Length++] = ' '; Line[Length++] = 'g'; Line[Length++] = '=';
    if (Event->HasGuid) {
      // g= is the 16 in-memory EFI_GUID bytes (LE), not canonical UUID text.
      for (Index = 0; Index < 16; Index++) {
        EnumaBootLogHex (&Line[Length], Event->GuidLe[Index], 2); Length += 2;
      }
    } else { Line[Length++] = '-'; }
  }
  Line[Length++] = '\n'; Line[Length] = 0;
  if (!Sink (Context, Line, Length)) { return FALSE; }
  (*Cursor)++;
  return TRUE;
}
#endif
