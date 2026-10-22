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


## Direct PE entry bridge (2026-10-23)

Points 20/21/22 extend the existing stage13 detail selection. Dispatcher sets the current scheduled image handle and original file GUID immediately around CoreStartImage and clears them after return. Image.c calls a DxeCore-local bridge before the actual Image->EntryPoint call (20), after a direct C return but before CoreExit (21), and after Exit/LongJump but before post-image TPL restoration/cleanup (22).

The bridge delegates to the same Dispatcher ordinal/GUID filter; it does not create a per-translation-unit counter. Different handles from nested StartImage are ignored. CoreDispatcher already refuses reentrant dispatch. Null/cleared context does not match. No timer, reset, allocation, MMIO or persistent-memory implementation is replaced.

A match at 20 supports readiness for the PE call only. A PE may run constructors inside its own entry stub. Point21 is absent if the image calls Exit instead of returning directly, while point22 may still run. None proves successful device initialization, GOP, or an exact fault cause. Use same-source complete GUID and 2/7/2 timing controls; retain all observations. Default detail13 and stage0/12 behavior remains.


## Global unhandled exception point 23

Point23 is independent of driver dispatch. It runs at the beginning of the fixed AArch64 DumpCpuContext path, before formatting or console operations, using the same real TimerLib and PSCI ResetCold. AArch64 library dependencies are declared explicitly. It is disabled unless stage13/detail23 are selected; dispatch/image gates never synthesize point23.

This is global to the default unhandled-exception dump, not tied to a driver handle, GUID or ordinal. The device builder must reject a driver GUID or ordinal other than the placeholder1 in this mode. Registered exception handlers, secure-world failures, a stopped CPU before this hook, or failure of the timer/reset itself can bypass observation; negative evidence does not exclude all exceptions. Preserve 2/7 timing controls and separate this source/layout from the earlier PE-specific samples.
