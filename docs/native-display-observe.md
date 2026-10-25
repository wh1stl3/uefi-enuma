# Independent native video observation

`NativeObserve.c/.h` is an unbound callback-only diagnostic, not a replacement
for `EnumaSampleVideoDetailed` or GOP admission. It does not appear in any INF,
DSC, runtime entry or build selector. No hardware pointer, pixel access, CTL/DSI
write, protocol publication, allocation or reset is present.

The exact status-before-frame rule is based on the fixed MiCode52a2f18
`sde_hw_intf_v1_get_status`: STATUS bit0 gates FrameCount. Kernel source is a
behavior reference only; no GPL implementation was imported. Timing/Enable are
independent observations, so enable0/1/2 do not stop acquisition. This does not
change the full sampler's enable predicate or clear frame requirements.

It samples two interfaces three times, with two100ms waits, at most26 reads:
revision, each Timing/Enable/STATUS and active-only FrameCount, final revision.
Offsets are 0, 6b800/6c000 plus0/a8/26c/ac within the existing companion R-004
aperture. The caller remains responsible for exact mapping and target scope.
No CTL, SSPP, LM, PP or unused-bank reads are performed.

FrameReadMask distinguishes an unobserved inactive Frame field from a hardware
zero. ActiveThroughout, nonzero/stable Timing, stable Enable, two-window
nonwrapping Frame increments1..64, and exact-enable3 have separate masks.
Acquisition success includes negative masks; it never means visible display.
Failure leaves output untouched. The unchanged complete sampler keeps all
existing predicates and ADDRESS/TOPOLOGY requirements.

Companion `check_native_display_observe.py` runs48 modeled/boundary cases with
ASan/UBSan and compiles a freestanding AArch64 object with no undefined helpers.
Existing actual Mu Blt, topology,330 callback failure oracle and164 selector
cases also pass. No new FD, runtime binding, device action or visible-output
qualification is claimed. Before use, bind a distinct runtime profile and
audit actual PE/flags/map/full payload plus companion recovery/camera gates.
