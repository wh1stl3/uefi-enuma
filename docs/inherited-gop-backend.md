# Inherited video GOP backend qualification

Companion decision D-195, 2026-10-25. This is an actual FrameBufferBltLib-backed
GOP implementation, not a physical display success claim. The board driver and
protocol publication remain gated on same-boot hardware evidence.

`InheritedGopLib` implements QueryMode with an independently allocated complete
mode-info copy, mode-number validation, SetMode black clear, all four Blt
operations, overflow/rectangle checks, and allocation cleanup. Creation has no
pixel access. Every operation revalidates ownership; writes happen at TPL_NOTIFY
only after an exact EFI_SUCCESS. No MMIO programming, CTL_START, cache operations
or timer refresh is imported. The board caller must establish CPU identity and
an uncached mapping; the existing write-through Display Reserved mapping does
not meet this policy. The library never installs a protocol. Publication and
DriverBinding/consumer lifetime must be owned by the eventual board driver.

`Topology.c` reads 144 LM words through bounded callbacks and resolves the two
local mixer origins from LM_OP_MODE bit31, never numeric LM order. It supports
one video CTL with both DSI interface bits, two full-height mixers and distinct
full-tile sources. Unsupported source-split/shared-pipe, partial update, multiple
CTL, border/virtual/unknown staging, dim and unreviewed processing are rejected.
This is a conservative supported topology, not a claim that enuma uses it.

`Evidence.c` gathers three same-boot scanout/LM/interface snapshots with two
100ms waits: at most969 reads, max offset0x73830, no physical pixel dereference.
Only the two CTL-selected mixers are read; unsupported CTL/mixer-count state
stops before any LM blend reads. The separate full144-read LM capture API is
for offline analysis; it is not called by the live sampler.
Each interface must be active, timing/counter enabled and increase1..64 in each
window without wrap; descriptors, staging and blend must remain stable. Transient
START/FLUSH are recorded but excluded from stability. Success retains ADDRESS
and TOPOLOGY requirements: PP/DSC/DSI downstream routing, SMMU/CPU relationship and
CPU memory attributes still need independent evidence. Shadow registers and
moving counters do not prove the panel is showing this allocation.

Register facts reference MiCode Xiaomi_Kernel_OpenSource
52a2f180865f8368828985d6384130acd093fa93 sde_hw_lm.c/sde_crtc.c and the already
pinned v1 CTL/interface sources. GPL implementation is not imported. The existing
cas algorithm provenance remains in InheritedScanoutLib/sources.json. Decode.c
now correctly shifts the staging extension into bit3 and includes OriginY in
coverage-boundary checks.

Host qualification links the actual pinned Mu FrameBufferBltLib, UEFI headers
and production backend, with only allocation/TPL/BaseLib primitives modeled.
Tests cover24 byte permutations, four operations, overlap, padding/guards,
allocation failures and stale-ownership refusal; topology includes reversed LM
identities and144 read failures; evidence injects failure at all969 positions.
All execute with ASan/UBSan; AArch64 objects are compiled separately.

`--gop-backend-selftest` includes a non-autostart UEFI application in the FD for
link qualification. Its GOP uses allocated RAM, installs no protocol and does
not touch display MMIO. The extracted application is intended for QEMU only;
the resulting FD is not a qualified enuma physical-display boot candidate.
Default builds omit this application. No board/bringup-plan memory field or
verified marker is changed. Raw field media stay local in companion out/.

The companion has executed the linked8d85d39 RAM-only application under QEMU:
`ENUMA_GOP_SELFTEST PASS RAM ONLY`. This is not physical GOP qualification.
The opt-in `--inherited-video-audit` replaces the metadata SimpleFb entry with
`diagnostics/EnumaInheritedVideoAudit.c`, preserving its existing INF/GUID and
Core-warning transport. It publishes no protocol and never calls the new GOP
backend. It checks the exact R-004 map record, samples only within that aperture,
uses the existing two100ms Stall calls and returns a predicate marker. A2s gate
versus7s dispatcher fallback is host timing evidence, never a unique log or a
claim that the panel displays those sources. A75s host timeout stops all further
boots and requires recovery. This new read-only candidate is independent of old
consumed pixel/value-transport experiments, which remain frozen.

S-008 sample audit898ae25 timed out and was consumed; Android/root/A-slot and
original partition/backup digests were independently rechecked after manual
recovery. No sampler-entry or register-specific conclusion follows. The NEW
`--inherited-video-entry-control` keeps this INF/GUID and constructors but uses
a constant PASS body with no map queries/MMIO/pixels/allocations/waits/protocols.
It isolates entry/return transport from the sample body, not a retry of S-008 or
the old frozen value plan. A separate actual-PE/full-input qualification is
required before any temporary boot. See companion D-197 operations/handoff.
