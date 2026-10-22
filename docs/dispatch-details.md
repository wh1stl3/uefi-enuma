# Dispatch detail gates

The original 25 migration inputs and six patches remain byte-identical. New
files and the preparation helper are pinned separately in
`Resources/Enuma/extensions.json`. The DSC extension is applied only to the
device repository's ignored build staging, after copying the original DSC.

Use stage 13 and `ENUMA_DIAG_DETAIL_POINT` to select a finer boundary of the
actual scheduled ordinal. Default 13 preserves the original return gate:

| Point | Boundary |
| --- | --- |
| 13 | Any CoreStartImage return |
| 14 | Any CoreLoadImage return |
| 15 | Immediately before CoreStartImage |
| 16 | CoreStartImage returned an EFI error |
| 17 | CoreStartImage returned without an EFI error |
| 18 | CoreLoadImage returned an EFI error |
| 19 | CoreLoadImage returned without an EFI error |

The load gates run only when CoreLoadImage is actually called. Already-loaded
images and firmware-volume items can bypass them. A success return does not
prove all expected protocols were installed. A missing reset can also mean an
unreached ordinal or GUID mismatch, so timing controls and adjacent boundaries
remain necessary.

Optional `ENUMA_DIAG_GUID_ENABLED=1` additionally requires both 64-bit halves
of the file GUID to match `ENUMA_DIAG_GUID_LOW64`/`HIGH64`. They encode the
little-endian EFI_GUID memory bytes, not UUID text order. The device wrapper
accepts a UUID string and performs this conversion. A NULL pointer does not
match an enabled filter.

These gates reuse the original running counter, Mu TimerLib and PSCI reset.
Stage 0 remains inactive; stage 12 retains its original load-before gate. No
address marker, partition/NVRAM logging, protocol stub or memory-map change is
introduced. Firmware success must still be established on hardware.
