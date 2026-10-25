# Calibrated display-value transport

2026-10-24. New transport uses step0/query1 and separate --value-source,
--value-code, --value-index (0..6), --value-shift (0..28 by four).
Historical step1..7 are restored to GOP audit semantics. Historical delay20
is rejected: its display-delay defines were not passed to the DxeCore compiler
and its header contains a latent STAT typo. Historical timings are not register
values without matching compiled-code evidence.

constant source never invokes display priming, MMIO, framebuffer or cache
callbacks. Code zero adds no wait; code3 adds nine seconds. register source
performs the historical priming read plus one selected display-window read,
outputs one nibble using three seconds per code, and never writes pixels/MMIO.
The priming read is an experimental variable, not a proven safety mechanism.

Normal DxeCore marker return remains two seconds; completion fallback seven
seconds. No new memory mapping, reserved log, reset method or pixel write.
Before register execution, measure zero/code3/zero with matching-source
artifacts and reject baseline drift or ambiguous decoding. One trial per
invocation, exact image/FD/build/source binding, documents published before
each trial. A failed/ambiguous plan is frozen and never resumed implicitly.

Authoritative device status and recovery requirements live in the companion
UniXNU-enuma docs/STATUS.md and AGENTS.md. No hardware success is asserted by
this implementation or host tests.


## Structural observation after S-034

Opt-in --native-route-structure requires native-route-audit and excludes all
detail profiles. S-034 classified a selected blend operation mismatch; it did
not establish its numeric value or visual semantics. The new independent
sampler asks a narrower structural question. It skips blend/dim/processing
admission inside the pure geometry resolver, retains actual Mode bit31 for
left/right assignment, all source/stage/geometry/format/security/address checks,
and three raw snapshots with two100ms windows. Raw mixer words still participate
in stability. No data is normalized to pretend the blend check passed.
Success carries requirements22=BLEND|ADDRESS|TOPOLOGY, and only the new selector
can classify that as stable STRUCTURE. All old selectors reject requirements22.
No GOP, pixels, MMIO writes or new mappings. This is not a rendering candidate.
Xiaomi52a2f18 sde_crtc.c420..476 supports multiple blend operations; its
sde_hw_lm.c setup_out locates the right mixer using bit31 independently of
setup_blend_config/setup_color3. Unknown composition remains explicitly unproven.
