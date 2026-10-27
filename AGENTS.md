# uefi-enuma working rules

<!-- path-migration-policy-2026-10-26 -->
## 路径迁移与后续工作约定（2026-10-26）

本机型统一工作根为 `D:/Code/PortingiOS/enuma`；设备仓为 `D:/Code/PortingiOS/enuma/UniXNU-enuma`，迁入的独立固件仓为 `D:/Code/PortingiOS/enuma/uefi-enuma`。本文件所在仓库为 `D:/Code/PortingiOS/enuma/UniXNU-enuma/firmware`。

enuma设备仓实际使用的固件源码仍在 `D:/Code/PortingiOS/enuma/UniXNU-enuma/firmware` 子模块；它与迁入的独立 `uefi-enuma` 检出不得当成同一工作树混用。

所有后续工作、新文档、脚本调用示例及交接统一使用上述新路径；nuwa/enuma必须隔离各自工作目录，不跨机型混用。旧资料库现为 `D:/Code/PortingiOSLegacy`。历史证据中的旧路径只用于追溯，不能作为当前操作位置；用[统一迁移索引](../../../PATH-MIGRATION.md)定位。保留的2395份历史/绑定/生成记录并未被统一改写，旧验证不自动覆盖迁移后修改的文件。

This repository was explicitly authorized by the user on 2026-10-22. This repository owns Resources/Scripts/build_enuma_firmware.py. Companion repository D:/Code/PortingiOS/enuma/UniXNU-enuma owns the verified compatibility adapter, output directory, packaging/device gates and authoritative handoff documents. Follow its AGENTS.md for versioning and device operations. This repository and the companion are writable; nuwa and PortingiOS references remain read-only.

Do not commit OEM binaries, backups, identifiers or generated images. Keep outputs inside the companion out/ directory. Preserve fixed dependency revisions and imported patch provenance. New memory size/base changes or *_verified=true still require the companion review process. Do not infer hardware success from a build or an automatic reset.

Commits use [unixnu-enuma][firmware] with [Change], [Evidence], [Risk]. Do not rewrite pushed history or force push. No device action is part of a source-only migration.

After a manually recovered probe, follow the companion’s 2026-10-23 updated rule: automatically continue offline analysis, fixes, tests and new candidate builds. Do not treat successful manual recovery as a reason to end the work. Stop additional boots at a timeout, recover first, and qualify a newly isolated candidate before further device execution.

2026-10-24 takeover: use the calibrated value transport described in docs/value-transport.md. Constants must have no display reads; register index/shift are separate from historical GOP steps. Historical delay20 is frozen, not execution-qualified. Do not treat a build define as an active probe without checking the generated compiler command and packaged code. Pixel Sync and CPU Read precede Write, so timeout does not by itself locate a pixel-store failure.

2026-10-24 user rule: reconcile all applicable documentation and commit/push it before the next device trial or hard stop, including current STATUS/HANDOFF/decisions/review/human tasks and entry docs. Keep current facts separate from historical operation records. Any real timeout still requires same-turn stop/recovery records and refresh after recovery. Offline analysis/builds may continue while documentation is being reconciled.
