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
