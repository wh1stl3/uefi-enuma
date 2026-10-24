/** @file
  Read-only enuma boot-v3 payload transport. BlockIO/device-path and FDT
  pattern adapted from reAAAq/UniXNU-umi 3eb72578597d2ca16471b172d45eddc096a34a04,
  overlay/Platforms/Xiaomi/umiPkg/Drivers/UmiRamPayloadDxe/UmiRamPayload.c.
  Copyright (c) 2026, UniXNU contributors.
  SPDX-License-Identifier: BSD-2-Clause-Patent
  No partition access, allocation of payload RAM, new map or persistent log.
**/
#include <Uefi.h>
#include <Guid/FileInfo.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/FdtLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryMapLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/BlockIo.h>
#include <Protocol/DevicePath.h>
#include <Protocol/SimpleFileSystem.h>
#include "TransportBounds.h"
#include "TransportReport.h"
#include "PayloadContract.h" // Generated only in ignored build staging.
#define BLOCK_SIZE 512U
STATIC UINT8 *mRamBase;
STATIC EFI_BLOCK_IO_MEDIA mMedia;
STATIC EFI_HANDLE mHandle;
STATIC EFI_GUID mReportGuid = ENUMA_TRANSPORT_REPORT_GUID;
STATIC ENUMA_TRANSPORT_REPORT mReport;
STATIC EFI_STATUS EFIAPI
RamReset (EFI_BLOCK_IO_PROTOCOL *This, BOOLEAN ExtendedVerification)
{
  (VOID)This;
  (VOID)ExtendedVerification;
  return EFI_SUCCESS;
}

STATIC EFI_STATUS EFIAPI
RamRead (EFI_BLOCK_IO_PROTOCOL *This, UINT32 MediaId, EFI_LBA Lba,
         UINTN BufferSize, VOID *Buffer)
{
  (VOID)This;
  if (MediaId != mMedia.MediaId) {
    return EFI_MEDIA_CHANGED;
  }
  if (BufferSize == 0) {
    return EFI_SUCCESS;
  }
  if (Buffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  if ((BufferSize % BLOCK_SIZE) != 0) {
    return EFI_BAD_BUFFER_SIZE;
  }
  if ((Lba > mMedia.LastBlock) ||
      ((BufferSize / BLOCK_SIZE) > (mMedia.LastBlock - Lba + 1))) {
    return EFI_INVALID_PARAMETER;
  }
  CopyMem (Buffer, mRamBase + (UINTN)Lba * BLOCK_SIZE, BufferSize);
  if (mReport.Reads < 4) {
    DEBUG ((DEBUG_WARN, "ENUMA payload read lba=%Lu bytes=%Lu\n", Lba, (UINT64)BufferSize));
  }
  mReport.Reads++;
  return EFI_SUCCESS;
}

STATIC EFI_STATUS EFIAPI
RamWrite (EFI_BLOCK_IO_PROTOCOL *This, UINT32 MediaId, EFI_LBA Lba,
          UINTN BufferSize, VOID *Buffer)
{
  (VOID)This; (VOID)MediaId; (VOID)Lba; (VOID)BufferSize; (VOID)Buffer;
  return EFI_WRITE_PROTECTED;
}

STATIC EFI_STATUS EFIAPI
RamFlush (EFI_BLOCK_IO_PROTOCOL *This)
{
  (VOID)This;
  return EFI_SUCCESS;
}

STATIC EFI_BLOCK_IO_PROTOCOL mBlockIo = {
  EFI_BLOCK_IO_PROTOCOL_REVISION, &mMedia, RamReset, RamRead, RamWrite, RamFlush
};

#pragma pack(1)
typedef struct {
  VENDOR_DEVICE_PATH Vendor;
  EFI_DEVICE_PATH_PROTOCOL End;
} RAM_PATH;
#pragma pack()

STATIC RAM_PATH mPath = {
  {{HARDWARE_DEVICE_PATH, HW_VENDOR_DP, {sizeof (VENDOR_DEVICE_PATH), 0}},
   {0x684577D8, 0x2902, 0x4B57, {0x9E, 0x05, 0x49, 0xF5, 0xB4, 0xBA, 0x4B, 0x57}}},
  {END_DEVICE_PATH_TYPE, END_ENTIRE_DEVICE_PATH_SUBTYPE, {4, 0}}
};

STATIC EFI_STATUS Record (UINTN Step, EFI_STATUS Status) {
  mReport.Status[Step] = Status;
  if (!EFI_ERROR (Status)) { mReport.Passed |= (1U << Step); }
  else { mReport.Passed &= ~(1U << Step); }
  DEBUG ((DEBUG_WARN, "ENUMA payload step=%u status=%r\n", (UINT32)Step, Status));
  return Status;
}

STATIC EFI_STATUS Stop (UINTN Step, EFI_STATUS Status) {
  Record (Step, Status);
  // Keep the image resident: its published report and callbacks reference it.
  // EFI_SUCCESS here is driver residency, never payload qualification.
  return EFI_SUCCESS;
}

STATIC EFI_STATUS CrcMatches (CONST VOID *Data, UINTN Bytes, UINT32 Expected) {
  UINT32 Crc;
  EFI_STATUS Status = gBS->CalculateCrc32 ((VOID *)Data, Bytes, &Crc);
  if (EFI_ERROR (Status)) { return Status; }
  // Transport corruption check only; not authentication or a trust policy.
  return Crc == Expected ? EFI_SUCCESS : EFI_CRC_ERROR;
}

STATIC EFI_STATUS ReadAddress (CONST VOID *Fdt, INT32 Node, CONST CHAR8 *Name, UINT64 *Address) {
  INT32 Length;
  CONST VOID *Data = FdtGetProp (Fdt, Node, Name, &Length);
  if (Data == NULL) { return EFI_NOT_FOUND; }
  if (Length == 4) { *Address = Fdt32ToCpu (ReadUnaligned32 (Data)); }
  else if (Length == 8) { *Address = Fdt64ToCpu (ReadUnaligned64 (Data)); }
  else { return EFI_COMPROMISED_DATA; }
  return EFI_SUCCESS;
}

STATIC EFI_STATUS EFIAPI CheckFiles (ENUMA_TRANSPORT_REPORT *Report) {
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *Fs;
  EFI_FILE_PROTOCOL *Root, *File;
  EFI_STATUS Status;
  UINTN Index, InfoSize, Bytes;
  union { EFI_FILE_INFO Align; UINT8 Bytes[1024]; } Info;
  UINT8 Head[64];
  STATIC CONST CHAR16 *Names[4] = {L"EFI\\UniXNU\\Payloads\\kernel.macho",
    L"EFI\\UniXNU\\Payloads\\kernel.macho.adt", L"EFI\\UniXNU\\Payloads\\kernel.macho.trustcache",
    L"EFI\\UniXNU\\unixnu.toml"};
  STATIC CONST UINT64 Sizes[4] = ENUMA_FILE_SIZES;
  STATIC CONST UINT32 Crcs[4] = ENUMA_FILE_HEAD_CRCS;
  if (Report != &mReport || (mReport.Passed & (1U << 6)) == 0) { return EFI_NOT_READY; }
  if ((mReport.Passed & (1U << 12)) != 0) { return EFI_SUCCESS; }
  // This can run again at BDS after FatDxe has been dispatched. No polling.
  Status = gBS->ConnectController (mHandle, NULL, NULL, TRUE);
  Record (7, Status);
  Status = gBS->HandleProtocol (mHandle, &gEfiSimpleFileSystemProtocolGuid, (VOID **)&Fs);
  if (EFI_ERROR (Record (8, Status))) { return Status; }
  Status = Fs->OpenVolume (Fs, &Root);
  if (EFI_ERROR (Status)) { return Record (9, Status); }
  for (Index = 0; Index < 4; Index++) {
    File = NULL;
    Status = Root->Open (Root, &File, (CHAR16 *)Names[Index], EFI_FILE_MODE_READ, 0);
    if (!EFI_ERROR (Status)) {
      InfoSize = sizeof (Info);
      Status = File->GetInfo (File, &gEfiFileInfoGuid, &InfoSize, &Info);
      if (!EFI_ERROR (Status) && (InfoSize < SIZE_OF_EFI_FILE_INFO ||
          Info.Align.FileSize != Sizes[Index] || (Info.Align.Attribute & EFI_FILE_DIRECTORY) != 0)) {
        Status = EFI_COMPROMISED_DATA;
      }
      if (!EFI_ERROR (Status)) {
        Bytes = sizeof (Head);
        Status = File->Read (File, &Bytes, Head);
        if (!EFI_ERROR (Status)) {
          Status = Bytes == sizeof (Head) ? CrcMatches (Head, Bytes, Crcs[Index]) : EFI_END_OF_FILE;
        }
      }
      File->Close (File);
    }
    Record (9 + Index, Status);
    if (EFI_ERROR (Status)) { break; }
  }
  Root->Close (Root);
  return Status;
}

EFI_STATUS EFIAPI EnumaRamPayloadEntry (EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
  EFI_HOB_GUID_TYPE *Hob;
  EFI_MEMORY_REGION_DESCRIPTOR *Map;
  EFI_STATUS Status;
  EFI_HANDLE ReportHandle = NULL;
  UINT8 Count, Index;
  UINT64 Base = 0, Window = 0, Dtb, Start = 0, End = 0;
  UINTN TreeSize, Step;
  UINT32 Sectors;
  INT32 Chosen;
  VOID *Fdt;
  (VOID)ImageHandle; (VOID)SystemTable;
  mReport.Version = 1;
  mReport.CheckFiles = CheckFiles;
  for (Step = 0; Step < ENUMA_TRANSPORT_STEPS; Step++) { mReport.Status[Step] = EFI_NOT_READY; }
  Status = gBS->InstallProtocolInterface (&ReportHandle, &mReportGuid, EFI_NATIVE_INTERFACE, &mReport);
  if (EFI_ERROR (Status)) { return Status; }
  // Reuse the existing named mapped/reserved region, never a copied umi address.
  GetMemoryMap (&Map, &Count);
  for (Index = 0; Index < Count; Index++) {
    if (AsciiStrCmp (Map[Index].Name, "Kernel") != 0) { continue; }
    if (Window != 0 || Map[Index].HobOption != AddMem || Map[Index].MemoryType != Reserv ||
        Map[Index].ArmAttributes != WRITE_BACK_XN ||
        !Contains (Map[Index].Address, Map[Index].Length, Map[Index].Address, 1)) {
      return Stop (0, EFI_UNSUPPORTED);
    }
    Base = Map[Index].Address; Window = Map[Index].Length;
  }
  if (Window == 0) { return Stop (0, EFI_NOT_FOUND); }
  Record (0, EFI_SUCCESS);
  Hob = GetFirstGuidHob (&gSiliciumEntryArgHobGuid);
  if (Hob == NULL || GET_GUID_HOB_DATA_SIZE (Hob) != sizeof (Dtb)) { return Stop (1, EFI_NOT_FOUND); }
  Dtb = ReadUnaligned64 (GET_GUID_HOB_DATA (Hob));
  mReport.Dtb = Dtb;
  if ((Dtb & 7) != 0 || !Contains (Base, Window, Dtb, 40)) { return Stop (1, EFI_UNSUPPORTED); }
  Fdt = (VOID *)(UINTN)Dtb;
  if (FdtCheckHeader (Fdt) != 0) { return Stop (1, EFI_COMPROMISED_DATA); }
  TreeSize = FdtTotalSize (Fdt);
  if (TreeSize < 40 || TreeSize > SIZE_2MB || !Contains (Base, Window, Dtb, TreeSize)) {
    return Stop (1, EFI_COMPROMISED_DATA);
  }
  Record (1, EFI_SUCCESS);
  Chosen = FdtPathOffset (Fdt, "/chosen");
  if (Chosen < 0) { return Stop (2, EFI_NOT_FOUND); }
  Status = ReadAddress (Fdt, Chosen, "linux,initrd-start", &Start);
  if (!EFI_ERROR (Status)) { Status = ReadAddress (Fdt, Chosen, "linux,initrd-end", &End); }
  mReport.InitrdStart = Start; mReport.InitrdEnd = End;
  if (EFI_ERROR (Status)) { return Stop (2, Status); }
  if (!TransportSpan (Base, Window, Dtb, TreeSize, Start, End, ENUMA_PREFIX_BYTES, ENUMA_FAT_BYTES)) {
    return Stop (2, EFI_COMPROMISED_DATA);
  }
  Record (2, EFI_SUCCESS);
  Status = CrcMatches ((VOID *)(UINTN)Start, ENUMA_PREFIX_BYTES, ENUMA_PREFIX_CRC);
  if (EFI_ERROR (Record (3, Status))) { return EFI_SUCCESS; }
  // The exact v3 prefix has now matched. Never round its length to a sector.
  mRamBase = (UINT8 *)(UINTN)(Start + ENUMA_PREFIX_BYTES);
  mReport.FatStart = Start + ENUMA_PREFIX_BYTES; mReport.FatBytes = ENUMA_FAT_BYTES;
  Sectors = ReadUnaligned16 ((UINT16 *)(mRamBase + 19));
  if (Sectors == 0) { Sectors = ReadUnaligned32 ((UINT32 *)(mRamBase + 32)); }
  if (mRamBase[510] != 0x55 || mRamBase[511] != 0xaa ||
      ReadUnaligned16 ((UINT16 *)(mRamBase + 11)) != BLOCK_SIZE ||
      (UINT64)Sectors * BLOCK_SIZE != ENUMA_FAT_BYTES) { return Stop (4, EFI_COMPROMISED_DATA); }
  Record (4, EFI_SUCCESS);
  Status = CrcMatches (mRamBase, ENUMA_FAT_BYTES, ENUMA_FAT_CRC);
  if (EFI_ERROR (Record (5, Status))) { return EFI_SUCCESS; }
  mMedia.MediaId = 1; mMedia.MediaPresent = TRUE; mMedia.ReadOnly = TRUE;
  mMedia.BlockSize = BLOCK_SIZE; mMedia.IoAlign = 1; mMedia.LastBlock = Sectors - 1;
  Status = gBS->InstallMultipleProtocolInterfaces (&mHandle,
    &gEfiDevicePathProtocolGuid, &mPath, &gEfiBlockIoProtocolGuid, &mBlockIo, NULL);
  if (EFI_ERROR (Record (6, Status))) { return EFI_SUCCESS; }
  // Defer ConnectController/SFS/files to the BDS application. DXE can dispatch
  // us before all architectural protocols and filesystem drivers are ready.
  return EFI_SUCCESS;
}

