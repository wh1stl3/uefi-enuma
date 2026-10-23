# uefi-enuma working rules

This repository was explicitly authorized by the user on 2026-10-22. This repository owns Resources/Scripts/build_enuma_firmware.py. Companion repository D:/Code/UniXNU-enuma owns the verified compatibility adapter, output directory, packaging/device gates and authoritative handoff documents. Follow its AGENTS.md for versioning and device operations. This repository and the companion are writable; nuwa and PortingiOS references remain read-only.

Do not commit OEM binaries, backups, identifiers or generated images. Keep outputs inside the companion out/ directory. Preserve fixed dependency revisions and imported patch provenance. New memory size/base changes or *_verified=true still require the companion review process. Do not infer hardware success from a build or an automatic reset.

Commits use [unixnu-enuma][firmware] with [Change], [Evidence], [Risk]. Do not rewrite pushed history or force push. No device action is part of a source-only migration.

After a manually recovered probe, follow the companion’s 2026-10-23 updated rule: automatically continue offline analysis, fixes, tests and new candidate builds. Do not treat successful manual recovery as a reason to end the work. Stop additional boots at a timeout, recover first, and qualify a newly isolated candidate before further device execution.

2026-10-24 user rule: reconcile all applicable documentation and commit/push it before the next device trial or hard stop, including current STATUS/HANDOFF/decisions/review/human tasks and entry docs. Keep current facts separate from historical operation records. Any real timeout still requires same-turn stop/recovery records and refresh after recovery. Offline analysis/builds may continue while documentation is being reconciled.
