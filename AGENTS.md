# uefi-enuma working rules

This repository was explicitly authorized by the user on 2026-10-22. Companion repository D:/Code/UniXNU-enuma owns the build wrapper, output directory and authoritative five handoff documents. Follow its AGENTS.md for versioning and device operations. This repository and the companion are writable; nuwa and PortingiOS references remain read-only.

Do not commit OEM binaries, backups, identifiers or generated images. Keep outputs inside the companion out/ directory. Preserve fixed dependency revisions and imported patch provenance. New memory size/base changes or *_verified=true still require the companion review process. Do not infer hardware success from a build or an automatic reset.

Commits use [unixnu-enuma][firmware] with [Change], [Evidence], [Risk]. Do not rewrite pushed history or force push. No device action is part of a source-only migration.
