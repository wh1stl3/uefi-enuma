/** @file
  Single-core ECC-free Linux persistent console buffer writer.

  Adapted from UniXNU-umi 3eb72578597d2ca16471b172d45eddc096a34a04,
  overlay/Platforms/Xiaomi/umiPkg/Library/RamoopsSerialPortLib/RamoopsSerialPortLib.c.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  No physical address, mapping, reset, initialization hook or SerialPort binding.
  Caller supplies aligned reserved RAM, mapped normal uncached, with interrupts
  excluded and no concurrent writer. MemoryFence does not flush writeback RAM.
  A reset during payload or metadata publication may leave a partial message.
**/
#ifndef ENUMA_RAMOOPS_BUFFER_H
#define ENUMA_RAMOOPS_BUFFER_H

#include <Base.h>
#include <Library/BaseLib.h>

#define ENUMA_RAMOOPS_SIGNATURE  0x43474244U

typedef struct {
  UINT32 Signature;
  UINT32 Start;
  UINT32 Size;
} ENUMA_RAMOOPS_HEADER;

STATIC_ASSERT (sizeof (ENUMA_RAMOOPS_HEADER) == 12, "Linux DBGC header size");
STATIC_ASSERT (OFFSET_OF (ENUMA_RAMOOPS_HEADER, Start) == 4, "Linux start offset");
STATIC_ASSERT (OFFSET_OF (ENUMA_RAMOOPS_HEADER, Size) == 8, "Linux size offset");

// RegionSize includes the 12-byte header. ECC and nonzero signature XOR are
// unsupported. Source must not overlap Region. Zero/invalid arguments do not
// touch Region. Valid old data is preserved; invalid metadata starts a new ring.
STATIC
UINTN
EnumaRamoopsAppend (
  IN volatile VOID *Region,
  IN UINTN RegionSize,
  IN CONST UINT8 *Buffer,
  IN UINTN NumberOfBytes
  )
{
  volatile ENUMA_RAMOOPS_HEADER *Header;
  volatile UINT8 *Data;
  UINT32 Capacity;
  UINT32 Start;
  UINT32 Size;
  UINTN Count;
  UINTN Index;

  if ((Region == NULL) || (Buffer == NULL) || (NumberOfBytes == 0) ||
      (((UINTN)Region & 3U) != 0) ||
      (RegionSize <= sizeof (ENUMA_RAMOOPS_HEADER)) ||
      (RegionSize - sizeof (ENUMA_RAMOOPS_HEADER) > 0x7FFFFFFFU)) {
    return 0;
  }

  Header = (volatile ENUMA_RAMOOPS_HEADER *)Region;
  Capacity = (UINT32)(RegionSize - sizeof (*Header));
  Start = Header->Start;
  Size = Header->Size;
  if ((Header->Signature != ENUMA_RAMOOPS_SIGNATURE) ||
      (Size > Capacity) || (Start > Size)) {
    Header->Signature = 0;
    MemoryFence ();
    Header->Start = 0;
    Header->Size = 0;
    MemoryFence ();
    Header->Signature = ENUMA_RAMOOPS_SIGNATURE;
    MemoryFence ();
    Start = 0;
    Size = 0;
  }

  if (Start == Capacity) {
    Start = 0;
  }
  Data = (volatile UINT8 *)Region + sizeof (*Header);
  Count = NumberOfBytes;
  if (Count > Capacity) {
    Buffer += Count - Capacity;
    Count = Capacity;
  }
  for (Index = 0; Index < Count; Index++) {
    Data[Start++] = Buffer[Index];
    if (Start == Capacity) {
      Start = 0;
    }
  }
  Size = (Count > Capacity - Size) ? Capacity : Size + (UINT32)Count;
  MemoryFence ();
  Header->Size = Size;
  MemoryFence ();
  Header->Start = Start;
  MemoryFence ();
  return NumberOfBytes;
}

#endif
