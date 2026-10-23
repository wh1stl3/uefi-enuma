# Staged GOP diagnostics with Core-owned reset

Current transport: core-warning-v2. Replaces the provider-local reset used by
250849f (that candidate failed automatic return; cause remains unknown).

Default ENUMA_DIAG_GOP_QUERY=0 and ENUMA_DIAG_GOP_STEP=0. No new dependencies
are added to SimpleFbDxe.inf. The provider never calls TimerLib or ResetSystemLib.
The existing Core final StartImage hook, GUID filter, Timer/PSCI checkpoint and
normal-dispatch completion fallback perform all result selection and reset.

Steps are cumulative, and every allocated temporary buffer is freed before
returning. Step 0 runs after installation without any GOP audit service call;
1 enumerates the unique expected GOP handle and frees the list; 2 also calls
HandleProtocol and matches the instance; 3 also validates Mode/Info/span;
4 also calls QueryMode, compares defined fields and frees the returned info.
Steps 1/2 still cannot distinguish a service hang from its list cleanup.

The provider returns warning value 1 for stage pass, 5 for stage error. These
are private diagnostic markers, not glyph/stale-data diagnoses. Both are
non-errors: the fixed Mu UefiDriverEntryPoint preserves the status, CoreExit
copies it, and CoreStartImage unloads a driver only on EFI_ERROR. Actual driver
entry and Core return excerpts are host-tested against the pinned archive.
Installation errors do not become markers and follow the original cleanup.

Query selector 1/2 means match pass/error marker respectively at Core point13,
using detail16, exact SimpleFb GUID, stage13, delay2, GUID-only and completion
fallback. EFI_SUCCESS, other warnings, errors and other hook locations do not
match. Existing full GUID filtering and no-ordinal-guess rules remain intact.
An unmatched result may reach the seven-second normal-dispatch fallback; it is
not an asynchronous watchdog and cannot recover a hung operation.

No framebuffer dereference, Blt, SetMode, MMIO, new physical reservation or
verified flag. Metadata consistency does not establish physical scanout or
safe pixel ownership. All new stages remain hardware-unverified until separately
tested with current recovery guards. Old 250849f images must not be retried.
