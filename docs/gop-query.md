# Read-only SimpleFb GOP audit

Default disabled (`ENUMA_DIAG_GOP_QUERY=0`). This extends the existing
metadata-only SimpleFb diagnostic, not the display driver or memory map.
The companion build wrapper permits selector 1 (audit pass) or 2 (audit
failure) only with stage13, delay2, detail16, the full SimpleFb file GUID,
GUID-only selection and the existing normal-dispatch completion fallback.

After successful protocol installation the provider enumerates GOP handles,
requires exactly its installed handle and protocol instance, validates current
single-mode BGRX metadata/span arithmetic, calls QueryMode and compares every
defined information field. Temporary handle/query buffers are freed before the
gate. No framebuffer dereference, Blt, SetMode or new address is used.

The selected predicate uses the existing TimerLib/PSCI two-second reset gate.
Core detail gates are suppressed for both selectors: otherwise an installation
failure could impersonate query success through the existing status gate.
An unmatched predicate continues with the original successful entry return and
the seven-second normal-dispatch fallback. The driver does not return a new
error after publishing protocols, because image unload could leave dangling
protocol functions. Hangs are not covered by this synchronous fallback.

Timing can support the predicate, not numeric metadata capture, physical scanout,
pixel ownership, menu visibility or XNU entry. Companion reports own host and
device evidence. Original SimpleFb provider and its existing contract patch are
reused; the new helper is diagnostic instrumentation, not borrowed panel data.
