// SPDX-License-Identifier: BSD-2-Clause-Patent
// Read-only platform observations before the unchanged UniXNU boot manager.
// No file/variable writes, MMIO probing, allocation of the owned window or EBS.
#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UniXNULib.h>
#include <Protocol/GraphicsOutput.h>
// Generated in isolated staging from hash-locked DarwinBoot; never edited here.
#include "BootVideo.h"
#include "DarwinProbe.inc"

#define SYSREG(Name, Value) __asm__ volatile ("mrs %0, " #Name : "=r" (Value))

STATIC VOID Line (UINTN Row, CONST CHAR16 *Text)
{
  Print (L"%s\n", Text);
  DiagDraw (Text, Row);
}

EFI_STATUS EFIAPI EnumaProbeEntry (EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
  EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop;
  EFI_MEMORY_DESCRIPTOR *Map, *Descriptor;
  EFI_STATUS Status, VideoStatus;
  UINTN Capacity, Size, Key, Stride, Offset;
  UINT64 El, Freq, Midr, Mpidr, Mmfr, Video[6], Best, Start, End, Aligned, Room;
  CHAR16 Text[128];
  (VOID)SystemTable;
  SYSREG (CurrentEL, El);
  SYSREG (CNTFRQ_EL0, Freq);
  SYSREG (MIDR_EL1, Midr);
  SYSREG (MPIDR_EL1, Mpidr);
  SYSREG (ID_AA64MMFR0_EL1, Mmfr);
  Line (0, L"ENUMA PROBE 01 - UEFI, NOT XNU");
  UnicodeSPrint (Text, sizeof (Text), L"EL %lu CNTFRQ %lu 4K FIELD %lx", El >> 2, Freq, (Mmfr >> 28) & 15);
  Line (1, Text);
  UnicodeSPrint (Text, sizeof (Text), L"MIDR %lx MPIDR %lx", Midr, Mpidr);
  Line (2, Text);
  Gop = NULL;
  Status = gBS->HandleProtocol (gST->ConsoleOutHandle, &gEfiGraphicsOutputProtocolGuid, (VOID **)&Gop);
  if (EFI_ERROR (Status)) {
    Status = gBS->LocateProtocol (&gEfiGraphicsOutputProtocolGuid, NULL, (VOID **)&Gop);
  }
  VideoStatus = EFI_NOT_FOUND;
  ZeroMem (Video, sizeof (Video));
  if (!EFI_ERROR (Status)) { VideoStatus = DarwinBootVideoFromGop (Gop, Video); }
  UnicodeSPrint (Text, sizeof (Text), L"GOP %r DARWIN VIDEO %r", Status, VideoStatus);
  Line (3, Text);
  if (!EFI_ERROR (Status) && Gop->Mode != NULL && Gop->Mode->Info != NULL &&
      Gop->Mode->SizeOfInfo >= sizeof (*Gop->Mode->Info)) {
    UnicodeSPrint (Text, sizeof (Text), L"%ux%u STRIDE %u FORMAT %u",
      Gop->Mode->Info->HorizontalResolution, Gop->Mode->Info->VerticalResolution,
      Gop->Mode->Info->PixelsPerScanLine, Gop->Mode->Info->PixelFormat);
    Line (4, Text);
    UnicodeSPrint (Text, sizeof (Text), L"FB %lx BYTES %lx", Gop->Mode->FrameBufferBase, (UINT64)Gop->Mode->FrameBufferSize);
    Line (5, Text);
  }
  Map = NULL; Capacity = 0; Best = 0; Start = 0; Room = 0;
  Status = GetMap (&Map, &Capacity, &Size, &Key, &Stride);
  if (!EFI_ERROR (Status) && (Stride < sizeof (*Map) || Size % Stride != 0)) { Status = EFI_COMPROMISED_DATA; }
  if (!EFI_ERROR (Status)) {
    for (Offset = 0; Offset + Stride <= Size; Offset += Stride) {
      Descriptor = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)Map + Offset);
      if (Descriptor->Type != EfiConventionalMemory || Descriptor->NumberOfPages == 0 ||
          Descriptor->NumberOfPages > (MAX_UINT64 - Descriptor->PhysicalStart) / EFI_PAGE_SIZE) { continue; }
      End = RunEnd (Map, Size, Stride, Descriptor->PhysicalStart,
        Descriptor->PhysicalStart + EFI_PAGES_TO_SIZE (Descriptor->NumberOfPages));
      if (End - Descriptor->PhysicalStart > Best) { Best = End - Descriptor->PhysicalStart; Start = Descriptor->PhysicalStart; }
      if (Descriptor->PhysicalStart <= MAX_UINT64 - 0x1fffff) {
        Aligned = ALIGN_VALUE (Descriptor->PhysicalStart, 0x200000ULL);
        if (Aligned <= End && End - Aligned > Room) { Room = End - Aligned; }
      }
    }
  }
  UnicodeSPrint (Text, sizeof (Text), L"GETMAP %r BIG FREE %lx @ %lx", Status, Best, Start);
  Line (6, Text);
  UnicodeSPrint (Text, sizeof (Text), L"2M ALIGNED ROOM %lx REQUEST 20000000", Room);
  Line (7, Text);
  Line (8, L"MAP ONLY - NO 512M ALLOCATION TEST");
  Line (9, L"NO EXITBOOTSERVICES TEST / NO DISK WRITES");
  Line (10, L"PHOTO NOW; UNIXNU MENU IN 60 SECONDS");
  if (Map != NULL) { FreePool (Map); }
  if (!EFI_ERROR (VideoStatus)) { WriteBackDataCacheRange ((VOID *)(UINTN)Video[0], (UINTN)(Video[2] * Video[4])); }
  gBS->Stall (60 * 1000 * 1000);
  Status = UniXNURun (ImageHandle, UniXNUHostFirmware);
  return EFI_ERROR (Status) ? Status : EFI_ABORTED;
}
