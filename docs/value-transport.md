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


## S-036 structural geometry classification after raw stability

S-035 returned route-or-geometry rejection before completing all samples.
--native-route-structure-detail requires the independent structure profile.
Its Native4 sampler retains the same qualified CTL/source selection before
reading selected mixers, maximum330 reads/two100ms waits. It captures three
raw snapshots and validates both stable configuration and1..64 nonwrapping
frame deltas before running pure geometry/source decoding. It can continue
safe selected-register capture despite a pure geometry predicate that would
have rejected earlier in Native3; it does not bypass capture preconditions.
Code2..5 distinguishes mixer size, source local output, left/right assignment
and combined width rejection, respectively, only after raw stability passed.
Code1 includes unclassified collection/runtime/decode/success; code5 is a
rejection. It provides no actual register value or visual composition proof.
All previous APIs retain their ordering/results; any complete structure still
requires BLEND|ADDRESS|TOPOLOGY22. No MMIO writes/new mapping/GOP/pixels.


## S-037 exact global-output coordinate hypothesis

S-036 code3 supports raw stable source-local-output rejection after both frame
windows; it does not distinguish OutXy from OutSize. The independent Native5
profile --native-route-global-output requires structure and excludes geometry
detail. It captures/checks raw stability first, then requires exact two-source
route, unique left/right bit31, full-height/nonzero mixer widths, matching
source output sizes, combined panel width, left OutXy0 and right OutXy equal
to the left width (Y0). Only then may a RAM copy express right XY as local0
with the existing explicit logical origin. Raw snapshots remain unchanged.
Strict geometry and complete source format/stride/address/coverage decode
follow on that copy. This is a counterfactual logical descriptor contract,
not a proof of physical route or permission to change hardware coordinates.
Success still requires BLEND|ADDRESS|TOPOLOGY22 and publishes no GOP. Same
maximum330 reads/two100ms, no pixels/MMIO writes/new mapping. Old samplers
retain their previous behavior, including rejecting this global fixture.


## S-038 source decode status classification

S-037 code4 is broad source capture/decode rejection, not a direct numeric
status. --native-route-global-detail requires the existing global-output
profile. It changes only the result selector, not Native5 sampling or any
capture/geometry/format/address predicate. Only zero output requirements and
ReasonNativeDecode classify: code2 Geometry/Coverage,3 Format,4 Bounds/
Inconsistent,5 Route/VirtualRect/Secure/RefreshMode/Mode/Topology. Code1 is
unclassified, including collection, geometry, runtime and complete structure.
All19680 selector combinations are tested. Same exact GUID/Core1..5 timing,
330 reads/two100ms, full FAT, no fallback/writes/new mapping/GOP/pixels.
