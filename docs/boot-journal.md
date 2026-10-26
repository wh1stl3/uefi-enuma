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
Storage is DXE Core BSS, zero initialized each boot. Latest 128 records are kept;
overwrite loss is counted (saturating UINT64). Sequence exhaustion rejects append.
No cross-reset persistence or physical RAM region is implied.

Single writer only: dispatcher/image bridge execution with no concurrent AP or
exception producer. EnumaDxeBootLogSnapshot copies into distinct caller-owned
storage with producer quiescent. It is a Core-local C function, NOT a public UEFI
protocol, so an application cannot yet retrieve this journal.
ReplayNext consumes an immutable snapshot, emits the loss header first and at most
one event per call. Caller can cancel between calls. A rejecting sink leaves the
cursor unchanged. All numbers are fixed-width hex; g= contains in-memory LE GUID
bytes, or '-' when absent. It has no timestamp and does not measure execution time.
The formatter accepts typed records only; it does not parse arbitrary log strings.

The screen sink is deliberately not connected. This does not capture global DEBUG,
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
