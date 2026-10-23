# R-004 read-only descriptor probe

Human approval on 2026-10-23 authorizes only a device mapping at 0x0ae00000 / 0x85000 and bounded read-only probes. The staged display-mdp-map.patch adds NS_DEVICE/UNCACHEABLE/MMIO; original imported table bytes and prior migration hashes remain preserved. No RAM/heap/owned-memory or verification flags change.

Step 3 is the same-source non-MMIO return control. Additive step 8 reads SRC0_ADDR for eight live-DT pipe offsets, then up to eight extra fields only for addresses in the live splash interval [0x9c000000,0x9e300000). Maximum 72 32-bit reads. No register writes, pixel access, QueryMode, allocation or new constructor/library dependency.

The return predicate requires exactly one candidate with a conservative 1600x2560, 6400-byte pitch, linear single-plane 4-byte layout, no crop/rotation/solid-fill/compression or nonzero additional planes, and span inside the smaller live splash range. A successful predicate is not a numeric snapshot, not proof a descriptor is active, and not permission to render. Color channel order remains unknown. Core warning transport and fallback remain synchronous and cannot recover a stalled MMIO access.

Register facts: fixed Qualcomm kernel/msm d066b4b959a95fd4686754ee82a7c6d21466b27c drivers/gpu/drm/msm/sde/sde_hw_sspp.c; source SHA b0ecee22c82a4d72977973ee5ea232b34effd0014ffffb6182ca0b9bf6c4d8ec. Register layout constants are hardware facts; implementation is independently written. Live enuma DT base/pipe offsets and original map audit are in companion reports/enuma-scanout-source-audit.json and enuma-display-mmio-coverage.json.

Additive step9 checks only SRC0_ADDR on the same eight pipe locations, returning on any value in the splash interval. It performs at most eight 32-bit reads and never reads geometry/format or pixels. This checks address presence only: a byte at the region end or multiple candidates may pass, intentionally; neither span safety nor uniqueness/activity is established. Step8 semantics are preserved. Predicate rejection with normal recovery does not itself hard-stop offline work or the next authorized read-only probe.
