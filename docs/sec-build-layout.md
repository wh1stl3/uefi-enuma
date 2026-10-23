# SEC source-epoch banner isolation

The 2026-10-23 step3 control timeout was manually recovered. Actual FD comparison with the previous successful control found identical DxeCore/OEM payloads, SimpleFb/Pcd RSDS-only differences, and SEC wall-clock literals/address references. No fault PC is known; this is not proof of a root cause.

A local Clang preprocessing test showed SOURCE_DATE_EPOCH=1792540800 does not replace __TIME__/__DATE__ on this toolchain. The staged sec-fixed-epoch-banner.patch replaces only the SEC banner arguments with 00:00:00 / Oct 21 2026, matching the build wrapper's already fixed source epoch. The banner explicitly labels the source epoch, not the actual compilation wall time. Original imported source and all memory configuration stay unchanged. Preflight asserts the volatile builtins are absent from Sec.c and reports its staged digest.

This controls one observed source of layout variation; it is not a hardware fix or a full reproducible-build guarantee (other modules/debug identities may still vary). New offline candidates use an installation-only step0 control, not the old failed step3 image. Repeat identical builds before any runtime claim. No pixel/MMIO write, memory extension, verification change or partition operation is introduced.

Evidence update (2026-10-24): repeated identical-input builds produced byte-identical SEC TE payloads; remaining executable differences were eight Pcd RSDS GUID bytes, so full FDs were not identical. New f15d520 step0 and step9/10 returned Android without buttons, as did later ab0cb79 read-only probes. This does not establish the old step3 root cause or repair. See companion reports/enuma-sec-epoch-rebuild.json and current STATUS for current source pin and runtime limits.
