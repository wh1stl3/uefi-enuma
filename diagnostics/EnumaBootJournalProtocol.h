// SPDX-License-Identifier: BSD-2-Clause-Patent
// Private DXE-only snapshot ABI. No writer, hardware, allocation or persistence.
#ifndef ENUMA_BOOT_JOURNAL_PROTOCOL_H
#define ENUMA_BOOT_JOURNAL_PROTOCOL_H
#include "EnumaBootJournal.h"
#define ENUMA_BOOT_JOURNAL_PROTOCOL_GUID {0xa84039f7,0xb861,0x5dbe,{0x9a,0x09,0xa4,0xf6,0xcc,0x74,0x6d,0x5b}}
typedef struct {
  UINT32 Version;
  UINT32 Bytes;
  BOOLEAN (EFIAPI *Snapshot)(ENUMA_BOOT_LOG *Output);
} ENUMA_BOOT_JOURNAL_PROTOCOL;
#endif
