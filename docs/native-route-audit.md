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

S-029 returned the topology category with verified Android recovery. Optional
`--native-route-detail` records the first failing phase without extra reads or
continuation: code1 other/contract rejection,2 CTL selection,3 source staging,
4 mixer capture/origin/layout topology,5 stable candidate. Only statusTopology
can produce2/3/4. The default route category mapping normalizes new internal
phase reasons back to the original pipeline reason. Strict sampler reasons and
callback order remain unchanged. Full native sampling is compared against the
fafb8a3 fixed source at all330 read failure boundaries; detailed selector has
10080 status/reason/requirements cases. This does not loosen any topology gate.

S-030 localized the rejection to source staging. Optional
`--native-route-staging` (requires route detail) classifies only the24 words
already captured at that failure. No continuation or additional MMIO occurs.
Local copies ignore only known CTL border bit24. Code2 requires the entire
normalized route to contain exactly two distinct single-source mixers with
known encodings and no other fields, with at least one border bit present.
Code3 means known-source count/encoding rejection,4 other/virtual fields,
5 mixer cardinality or duplicate source,1 all other outcomes. Code5 is not
success in this mode. Raw snapshots and strict admission remain unchanged.
The classifier has64 border-mask and768 single-bit perturbation checks plus
negative shape/field/cardinality cases; the marker selector has12480 cases.
