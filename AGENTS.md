# Agent instructions — uefi-enuma

## 治理规则（必读）
1. 长期规则：`../../docs/governance-2026-11-06.md`（12 条，v2 对齐官方 format 2：官方优先/三仓+组件仓/结构/流程/上游同步/脱敏/Apple 合规/验证精简/Linux 优先/磁盘治理/交接/违例判定）。
2. AI 分工：`../../docs/ai-orchestration-2026-11-06.md`。
3. enuma 目标与路线：`../UniXNU-enuma/AGENTS.md`、`../UniXNU-enuma/README.md`（iPadOS 17.7.11 / 21H461 / iPad7,1，SM8250-AC）。

## 职责
维护 enuma（SM8250-AC，骁龙 870）的 Mu-Silicium UEFI 集成：内存保留、平台初始化、固件打包、硬件交接。
本仓派生自 UniXNU/uefi-sheng@66421cb（2026-11-06）；sheng 板级值为跨 SoC 参考，仅可借鉴实现结构，禁止照抄（MMIO/IRQ/面板/电源须 enuma 证据）。

## enumaPkg 现状（2026-11-06）
`Platforms/Xiaomi/enumaPkg/` 由同 SoC 的 `pipaPkg`（Xiaomi Pad 6，SM8250-AC）派生：KonaPkg 基座、`SOC_TYPE = 2`、SMBIOS/面板已换成 Pad 5 Pro 5G 的公开规格。
**仍是骨架**，以下 `TODO REQUIRED` 必须用 enuma 证据替换后才能上机：
- `Resources/Configs/enuma.toml` 的 `[uefi_fd] base/size`（当前沿用 pipa 值，仅占位）
- `MemoryMapLib.c` 的 DDR/保留区/DXE heap/Display reserved/FD/stack 段
- `enuma.dsc` 的 `PcdCPUCoresStackBase`、`PcdSmbiosSystemRetailSku`、`PcdSmbiosSystemBoardModel`
- `enuma.fdf` 的板级 DTB（`FdtBlob/sm8250-xiaomi-enuma.dtb`）与 ACPI 表
- 厂商 `Binaries/enuma/`（原厂 DXE UFS/Display/PMIC/GPI…）：私有输入，只经 `.cache/firmware/` 导入，不入库

## 必读（官方文档）
1. README.md、Building.md、`Platforms/Xiaomi/shengPkg/PORTING.md`（作为 enumaPkg 结构参考）。
2. [firmware integration](https://unixnu.com/zh/porting/uefi-firmware)、[SoC support](https://unixnu.com/zh/porting/soc-support)、[porting](https://unixnu.com/zh/porting/)、[repository specification](https://unixnu.com/zh/porting/device-repository)、[component catalog](https://unixnu.com/zh/reference/components)、[device-tree specification](https://unixnu.com/zh/porting/device-tree)、[bring-up](https://unixnu.com/zh/porting/bring-up)。
3. 所选组件的 README、构建接口与实现；[payload](https://unixnu.com/zh/concepts/payload)、[building](https://unixnu.com/zh/building)。

## 硬边界
- 禁止提交 OEM 二进制、设备标识符、生成镜像；vendor 输入放 `.cache/firmware/` 不入库（治理 §6/§7）。
- 内存大小/基址修改与 `*_verified=true` 须用户当轮确认。
- DSC/FDF 必须含 `UniXNUPkg/Application/UniXNU`、`UniXNUPkg/Driver/UniXNUDxe`、`DarwinBootPkg/Driver/DarwinBoot`；用 FDT initrd 负载时再加 `UniXNURamDiskDxe` + `DiskIoDxe` + FAT。
- 提交格式 `[uefi-enuma][<area>]` + Change/Evidence/Risk；不 force push（main 已设保护）。
