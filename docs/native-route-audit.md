# Native video route audit

Opt-in `--native-route-audit` uses the S-028 evidence: an interface can have
three zero Timing words while STATUS remains active, Enable remains stable and
FRAME advances. It invokes `EnumaSampleNativeVideoDetailed`, leaving the strict
production sampler and GOP admission unchanged. STATUS bit0 gates every FRAME
read; full Timing/Enable words must remain equal across three samples. Both
frame increments must be nonwrapping 1..64. Existing CTL/SSPP/LM capture,
topology and layout decoding remain unchanged.

The existing R-004 mapping bounds all reads: at most330 reads, two100ms waits,
maximum offset0x6c26c. No pixel or MMIO writes, no new mapping or protocol.
Successful evidence still requires ADDRESS and TOPOLOGY proof.

The exact SimpleFb GUID driver-return terminal accepts warning1..5 and cold
resets after2/7/12/17/22 seconds. Categories are collection/contract rejection,
runtime/stability rejection, topology rejection, other pipeline capture/layout
rejection, and stable candidate respectively. This is not the native mask
encoding. A stable candidate is not GOP or visible display success. No dispatcher
fallback. Fixed host boot-origin windows and independent recovery remain required.

Host ASan/UBSan covers330 native read failures, two wait failures, inactive
STATUS before FRAME, configuration/interface/frame stability, strict fixed-source
baseline equivalence, 7200 result mappings and504 terminal/GUID/status cases.
Hardware qualification belongs to the companion repository; this source commit
alone is not an on-device result.
