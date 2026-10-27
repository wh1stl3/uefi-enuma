// SPDX-License-Identifier: BSD-2-Clause-Patent
// Private post-StartImage handoff. Not a production GOP or BDS protocol.
#ifndef ENUMA_DEFERRED_MENU_H
#define ENUMA_DEFERRED_MENU_H
#include "EnumaBootJournal.h"
#define ENUMA_DEFERRED_MENU_GUID {0xf8b4e674,0x74db,0x4ea7,{0x88,0x74,0x8b,0xa5,0xb4,0xd9,0x57,0x27}}
typedef struct ENUMA_DEFERRED_MENU_PROTOCOL ENUMA_DEFERRED_MENU_PROTOCOL;
struct ENUMA_DEFERRED_MENU_PROTOCOL {
  UINT32 Version;
  UINT32 Bytes;
  EFI_HANDLE Owner;
  UINT32 (EFIAPI *Run)(ENUMA_DEFERRED_MENU_PROTOCOL *This,CONST ENUMA_BOOT_LOG *Snapshot);
};
// Accept only the newest completed driver return, never a preceding driver's
// success or entry marker. Snapshot storage is owned by the caller.
STATIC inline BOOLEAN EnumaDeferredReturnValid(CONST ENUMA_BOOT_LOG *Log) {
  CONST ENUMA_BOOT_EVENT *E;UINTN I;
  if(!Log||!Log->Count||Log->Count>ENUMA_BOOT_LOG_CAPACITY||Log->Head>=ENUMA_BOOT_LOG_CAPACITY){return FALSE;}
  E=&Log->Events[(Log->Head+Log->Count-1)%ENUMA_BOOT_LOG_CAPACITY];
  if(E->Point!=13||E->Status!=EFI_SUCCESS||!E->HasGuid||E->Sequence==MAX_UINT64||E->Sequence+1!=Log->NextSequence){return FALSE;}
  for(I=0;I<8;I++){
    if(E->GuidLe[I]!=(UINT8)(0x4ffc788ddcfd1e6dULL>>(I*8))||
       E->GuidLe[I+8]!=(UINT8)(0x921a65752fca1b8eULL>>(I*8))){return FALSE;}
  }
  return TRUE;
}
#endif
