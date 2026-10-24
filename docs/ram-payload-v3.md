# enuma boot-v3 read-only RAM payload

Build-only integration, disabled by default. `--ram-payload FILE` accepts a FAT
inside the companion repository's ignored out/ matching enuma-full-payload.json.
Preflight derives a local PayloadContract.h in staging. No OEM bytes or generated
header are committed. The host locks SHA256; runtime CRC32 detects transport
corruption and is explicitly not authentication or an Apple trust policy.

The BlockIO/device-path implementation and FDT handoff pattern reuse
reAAAq/UniXNU-umi 3eb72578597d2ca16471b172d45eddc096a34a04,
overlay/Platforms/Xiaomi/umiPkg/Drivers/UmiRamPayloadDxe/UmiRamPayload.c,
BSD-2-Clause-Patent (Copyright 2026 UniXNU contributors). enuma uses its own
existing named Kernel MemoryMapLib region; no umi physical window is imported.

Boot-v3 concatenation follows [AOSP's pinned header contract](https://android.googlesource.com/platform/system/tools/mkbootimg/+/808d42e197506319ee65eb078d3defd9602e2ac2/include/bootimg/bootimg.h).
The exact stock vendor prefix length/checksum is generated from the pinned local
vendor_boot backup. Runtime must match the exact combined size and prefix CRC
before deriving the FAT address. A sector-misaligned FAT base is permitted:
BlockIO uses byte CopyMem and IoAlign=1; lengths/LBAs remain sectors. No scanning,
rounding, fallback prefix or guessed physical base is allowed. Unmatched layout
is an explicit report failure, not a reason to try a second offset.

The entry x0 HOB must point to an aligned bounded FDT inside the existing reserved
mapped Kernel region. Its full size and initrd start/end must remain in that
region, be disjoint, have exact expected length, and pass prefix/BPB/full-FAT
CRC before BlockIO publication. Invalid input is never dereferenced outside this
range. This does not establish that ABL will supply such input on real enuma.

One same-boot report records 13 results: map, FDT, initrd, prefix, BPB, full-FAT
CRC, BlockIO, ConnectController, SFS, kernel, ADT, trust, config. Report fields are
boot-service lifetime only; no cross-reset retention claim. Entry failures after
report publication keep the DXE image resident so pointers cannot dangle.
EFI_SUCCESS for image residency does not mean payload success.

ConnectController and filesystem checks run from the existing BDS EnumaProbe
application, after DXE dispatch. Four read-only opens check exact size and the
first 64 bytes' CRC; the preceding full-FAT CRC covers file data as a whole.
Kernel data is never allocated as a giant DXE buffer. SFS/file failure halts the
application before UniXNURun. ConnectController may return an error for an
already bound device; SFS and file results determine usability independently.
With payload mode enabled the old 60-second probe pause is omitted. Every stage
remains separately reported; pending entries are EFI_NOT_READY. Print/DiagDraw
reuse existing output and do not establish visible output on enuma.

Source-only integration adds no new memory map, partition write, reset/pixel
probe or verified flag. Default firmware builds omit the new driver. Device
execution still requires companion AGENTS logging/recovery and document gates.
