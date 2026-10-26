# Bounded DXE status journal (offline implementation)

`--boot-journal` defines `ENUMA_BOOT_JOURNAL=1`; default is 0. The staging
preflight copies EnumaBootJournal.h beside the existing dispatcher hook. The
hook records BEFORE all marker/GUID/terminal filtering, preserving those gates.
It captures actual checkpoint point, ordinal, full EFI_STATUS and full file GUID
bytes: 14 LoadImage return; 15 before StartImage; 20 before PE entry;
21 direct PE return; 22 after Exit/LongJump; 13 CoreStartImage return.
Missing return events are not synthesized. Nested unrelated image calls remain
excluded by the existing image bridge; this is not an exhaustive image tracer.

There is no physical address, timer, allocation, file write, reset, framebuffer
access, protocol publication, cache or page-table change in the journal.
Storage is volatile DXE Core BSS (retain real stores under LTO), zero initialized each boot. Volatile does not add synchronization or persistence. Latest 128 records are kept;
overwrite loss is counted (saturating UINT64). Sequence exhaustion rejects append.
No cross-reset persistence or physical RAM region is implied.

Single writer only: dispatcher/image bridge execution with no concurrent AP or
exception producer. EnumaDxeBootLogSnapshot copies into distinct caller-owned
storage with producer quiescent. The plain --boot-journal profile has a Core-local function only. The separate
--boot-log-replay profile publishes the private v1 read-only Snapshot protocol
once at the first dispatcher detail checkpoint (one attempt, no retry). Its EFIAPI
callback copies the journal while the producer is quiescent in the caller image.
The protocol and code live in DXE Core until ExitBootServices. No append service
is exposed; protocol install failure causes the consumer to refuse before pixels.
ReplayNext consumes an immutable snapshot, emits the loss header first and at most
one event per call. Caller can cancel between calls. A rejecting sink leaves the
cursor unchanged. All numbers are fixed-width hex; g= contains in-memory LE GUID
bytes, or '-' when absent. It has no timestamp and does not measure execution time.
The formatter accepts typed records only; it does not parse arbitrary log strings.

The separate --boot-log-replay sink selects the newest real point13 return and
renders three cached rows: C0d O<8hex ordinal>[+ if overwritten], S<high32 status>,
 <low32 status>. No synthetic entry-success substitutes a returned status.
It uses origin256x96,3x5 hex glyphs at4x scale,2s hold and full save/revalidation/
verification/restoration/readback: <=73728 loads,49152 stores,3DSB; two Native6
samplers <=660 MDP reads/four100ms. Only EL1 and full-range NormalWT are accepted.
This is cached status replay and does not capture global DEBUG,
SEC, exceptions or BDS/ConOut status. SerialPortLib stays Null. Existing Print and
DiagDraw remain coupled and unexecuted by the early terminal. Requirements22,
alpha, SMMU and all verified fields are unchanged.

Companion tests: `out/firmware-tools/bin/python -B scripts/test_boot_journal.py`.
Host ASan/UBSan covers 10000 events, wrap/order/loss, GUID/status fidelity,
snapshot isolation, rejected sink retry, invalid arguments and saturation;
production hook is tested on/off, and three objects use pinned EDK AArch64 headers.
Those tests do not establish FD execution or screen visibility. Before hardware:
qualify the exact FD/PE/macros and payload, design/approve the precise replay pixel
scope and gates, publish current documents, then fresh identity/backup/recovery and
camera checks. No consumed S-001..047 image or R-018/R-019 permission is reused.
