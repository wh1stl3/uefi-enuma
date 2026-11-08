# uefi-enuma

小米平板 5 Pro 5G（enuma / SM8250-AC）的 Mu-Silicium UEFI 固件仓。

- 基线：UniXNU/uefi-sheng@66421cb7bb5694843aec8de1ca3d1de4c20cca38（2026-11-06 派生，已去除 sheng 历史）。

## 当前进度（2026-11-08）
- 阶段：官方 steps 第 5 步**已完成**（固件可构建、含设备仓 preflight）；第 7 步 bring-up 阶段 1–2 已实机到达 UniXNU 内置菜单。
- **实机里程碑（2026-11-08，用户当轮授权并录像）**：`fastboot boot` 走完 SEC→DXE→BDS→UniXNU 菜单（`Return to firmware / Restart / Power off / Tools`，音量键移动、电源键确认可用，Tools→Advanced Logger 二维码轮播可用）；日志与视频留本地 `.cache/media`（不入库）。最终镜像 `stage12-simplefb/boot.img` sha256 `ef782289…6dca`（本轮 exp 分支 `1070b371` 构建）。
- **日志通道（ramoops）**：`RamoopsLogLib` 把 DEBUG 输出写进 stock 内核的 ramoops console 区（`0xB0000000`/`0x400000`，DBGC 头）；SEC 把 PON 复位类型设为 warm + bootloader 原因，ASSERT 或看门狗复位都热复位回 fastboot 且 RAM 保留。读回：设备在 fastboot → `fastboot continue`（不复位）→ Android 起来后 root 读 `/sys/fs/pstore/console-ramoops-0`。硬复位会丢日志。
- **本轮 exp 分支并入（`exp/enuma-custom-display` → main）**：RamoopsLogLib、DXE Heap 2（`0x93200000`/`0x05700000`，DXE 堆 51→~138 MiB）、移除 FeatureEnablerDxe（QseeComDxe.c +131 断言）、改回 SimpleFbDxe（`USE_CUSTOM_DISPLAY_DRIVER=0`，厂商 DisplayDxe 入口挂住熄屏）、SEC 入口 x0 打印、ASSERT 重启前停留。
- **已完成（源码已构建；self-checked，未经 Verifier 独立复跑）**：
  - UniXNU 启动集成：DSC `[Components]` 与 FDF `[FV.FvMain]` 含 `UniXNU`、`UniXNUDxe`、`UniXNURamDiskDxe`、`DarwinBoot`、`AdvancedLogDxe`；`UniXNULib` 与 `PcdUniXNUDeviceModel="enuma"` 已接。
  - 固件日志导出器（Tools → Advanced Logger）：DSC 映射 `AdvancedLoggerAccessLib`/`QrEncoderLib`/`AssertLib`/`AdvancedLoggerHdwPortLib`（走 SerialPortLib）；DEBUG 下 `DebugLib=BaseDebugLibAdvancedLogger`；`AdvancedLoggerLib` 按阶段映射 SEC(PeilessArm)/DXE_CORE/DXE+UEFI/Runtime；日志缓冲动态分配，`PcdAdvancedLoggerBase` 保持 0。
  - 厂商 DXE 接入：APRIORI/DXE/RAW.inc 改为 enuma 实机 XBL 的扁平厂商模块路径（`EnumaInputs/*.inc`），移除 37 个 `USE_CUSTOM_DISPLAY_DRIVER=0` 屏蔽的面板 XML；复位槽位用通用 MdeModulePkg `ResetSystemRuntimeDxe`（与 pipa/DXE.inc 一致；厂商 `ResetRuntimeDxe` 此前只出现在 APRIORI、不在 FV，已移除）。更正（2026-11-08）：SdccDxe/FeatureEnablerDxe/PILProxyDxe/PILDxe/SPSSDxe/AdcDxe/UsbPwrCtrlDxe/QcomChargerDxeLA/ChargerExDxe/UsbfnDwc3Dxe/XhciPciEmulation/XhciDxe/UsbMsdDxe/UsbDeviceDxe/UsbConfigDxe/UsbInitDxe/ButtonsDxe/TsensDxe/LimitsDxe 这 19 个模块实为 enuma 自己 xbl_a 内的原件（xbl_a sha256 `7ac67cbf…565057`），此前只因 `extract_enuma_xbl.py` 的 SELECT 白名单遗漏而被误判为「enuma XBL 无」并删除；现已按原件导入恢复，厂商模块总数回到 52 个。
  - enuma.fdf 补 `[Rule.Common.DXE_RUNTIME_DRIVER.BINARY]`（EnvDxe/UFSDxe/ResetRuntimeDxe 是 DXE_RUNTIME_DRIVER）。
  - MemoryMapLib.c DDR 表 23 行（逐行引用 enuma XBL uefiplat.cfg `[MemoryMap]` 行号）；ConfigurationMapLib.c 全表 31 项现与 `[ConfigParameters]` 完全一致（`EnableUfsIOC` 取 cfg 的 0，`EnableLogFsSyncInRetail` 保持 1，不偏离 stock 附带写入）。
  - enuma.dsc SEC 补 `DebugLib=BaseDebugLibSerialPort`、栈基址 0x9FF90000、SMBIOS SKU=M2105K81C/Board=K81C；enuma.toml `[uefi_fd]` 0x9FC00000/0x300000（同 SoC pipa/alioth/nabu 一致，用户确认保持）。
- **构建**：`./unixnu build firmware ... --edk2 ~/enuma-fw/mu_basecore` 退出 0，产物 `SILICIUM_UEFI.fd` = 3145728 B = 0x300000（实现者 self-checked，未经 Verifier 独立复跑；无设备证据，不标 device-verified）。
- **仍待上机证据**：板级 DTB 内容未验证（候选 sha256 `42d36a00…f698`，ABL 已接受但内容未核）；Kernel 预留与 owned_memory_size 窗口（阶段 4）；`[payload]` 未构建、bring-up 阶段 3 起未测；厂商 `Binaries/enuma`（私有输入，经 `.cache/firmware/` 导入，不入库）。
- **已知问题（实机）**：基座 `AdvancedLogDxe.c` 退出键只认 CR/LF/ESC、不认电源键 `SCAN_SUSPEND`（AL 页面电源键退不出，基座只读）；自动复位看门狗来源未证实（QcomChargerDxe 打印 `Enabled WDOG`）。
- **已知启动链高风险（未改动共享 UniXNU，待 High 复核）**：
  - SEC 仅保留 FDT 指针 HOB，不负责 initrd 的映射/保留；`/chosen` 的 initrd 区间须由板级内存映射保留后才可安全发布 Block I/O。
  - 共享 `UniXNURamDiskDxe` 把 initrd 大小按 512 字节扇区取整，未占有补齐段、未检查溢出。
- **reviewer 审查（2026-11-08，Sol）**：INSUFFICIENT EVIDENCE，未放行修复前镜像。已修：(a) 复位模块未入 FV → 已改回通用 ResetSystemRuntimeDxe；(b) 无法证明「启动不写分区」→ 用户已修订为「写入分级」（允许厂商固件正常启动的附带写入，不偏离 stock）；(c) `EnableUfsIOC` 与 cfg 不一致 → 已改为 0。仍开放：板级 DTB 内容未验证（ABL 已接受 boot.img，但 DTB 内容未核）、Kernel 预留与 owned_memory_size 窗口待阶段 4。RAM Partition 协议生产者已实机确认（RamManagerDxe 加载并映射 RAM，日志可见）。**待复核**。
- 下一步：第 7 步 kernel/package/payload（bring-up 阶段 3）。

## 约定
规则见 `../../docs/governance-2026-11-06.md`（本机共享）。OEM 二进制与 vendor 输入放 `.cache/firmware/` 不入库；含 Apple 组件产物不上传（治理 §7）。
