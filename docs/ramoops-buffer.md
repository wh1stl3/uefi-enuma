# Unbound native console writer

`diagnostics/EnumaRamoopsBuffer.h` adapts the BSD-2-Clause-Patent writer from
[UniXNU-umi 3eb7257](https://github.com/reAAAq/UniXNU-umi/blob/3eb72578597d2ca16471b172d45eddc096a34a04/overlay/Platforms/Xiaomi/umiPkg/Library/RamoopsSerialPortLib/RamoopsSerialPortLib.c).
The upstream root license is retained in `diagnostics/LICENSE.ramoops-upstream`.

The caller provides an aligned buffer and size. This file has no physical base,
SerialPort binding, constructor, boot hook, reset, allocation, or DEBUG calls.
No existing production source includes it. The caller must exclude interrupts
and concurrent writers, supply reserved normal uncached memory, and keep source
data separate from the ring. Fences do not flush writeback memory. Invalid
metadata starts a new ring; valid old data is preserved. Partial writes during
reset remain possible; this is not a crash-atomic protocol.

The format is ECC-free, signature XOR zero, a 12-byte DBGC/start/size header.
The companion's MiCode fixed-source audit is reference-only; no GPL kernel code
is imported. Host ASan/UBSan tests compare chronological bytes against a linear
queue over 51,210 append/boundary cases, and an AArch64 object is compiled using
the existing fixed build's EDK headers. Neither proves physical retention.

The companion R-017 A scope covers **only** an additional console mapping
at B0000000/2MiB and a single 44-byte nonce marker before a bounded reset.
Pmsg is not mapped or accessed. After the exact proposal the user explicitly
instructed direct execution with camera evidence. The new build-only
`--ramoops-canary-nonce` profile enables the conditional map and stages a separate
stage10 header into DxeMain only. Baseline migrated headers remain unchanged.
The standalone header writes once, waits two seconds and requests ResetWarm;
if ResetWarm returns, CpuDeadLoop prevents dispatcher fallthrough.
The Null SerialPort backend stays in use. Other profiles omit the new map.
The companion owns pstore collection, target/recovery gates and trial packaging.
See its `docs/operations/2026-10-25-ramoops-compatibility-audit.md`.

S-015 timed out and was manually recovered with independent verification. It is
consumed. The separate `--ramoops-map-control` profile enables only the same
console mapping (`ENUMA_RAMOOPS_MAP=1`, writer flag0), keeps the byte-identical
baseline stage10/2s/ResetCold gate, and never reads/writes console contents.
It is mutually exclusive with a nonce. This isolates the new map/early path
from console writes and warm reset; a return does not prove the map was used.
