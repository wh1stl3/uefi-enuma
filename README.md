# uefi-enuma

小米平板 5 Pro 5G（enuma / SM8250-AC）的 Mu-Silicium UEFI 固件仓。

- 基线：UniXNU/uefi-sheng@66421cb7bb5694843aec8de1ca3d1de4c20cca38（2026-11-06 派生，已去除 sheng 历史）。

## 当前进度（2026-11-07）
- 阶段：官方 steps 第 5 步进行中，`enumaPkg` 是 pipa 派生骨架，尚无可构建固件。
- **已完成（仅源码接线，未编译/未上机）**：
  - UniXNU 启动集成：DSC `[Components]` 与 FDF `[FV.FvMain]` 含 `UniXNU`、`UniXNUDxe`、`UniXNURamDiskDxe`、`DarwinBoot`、`AdvancedLogDxe`；`UniXNULib` 与 `PcdUniXNUDeviceModel="enuma"` 已接。
  - 固件日志导出器（Tools → Advanced Logger）：DSC 映射 `AdvancedLoggerAccessLib`/`QrEncoderLib`/`AssertLib`/`AdvancedLoggerHdwPortLib`（走 SerialPortLib）；DEBUG 下 `DebugLib=BaseDebugLibAdvancedLogger`；`AdvancedLoggerLib` 按阶段映射 SEC(PeilessArm)/DXE_CORE/DXE+UEFI/Runtime。日志缓冲由 DXE Core 动态分配，`PcdAdvancedLoggerBase` 保持 0（不虚构固定地址）。
  - `RamManagerDxe` 补 `MemoryRange = NULL` 初始化（GetEfiMemoryMap 失败时 cleanup 前未赋值）。
- 原厂 DXE、固件 DTB、ACPI、FD/stack/DDR 保留区和 SKU 仍待 enuma 证据；`TODO REQUIRED` 由设备仓 preflight 拦截。
- **构建阻塞**：`Mu_Basecore`（MdePkg/MdeModulePkg 等）子模块未初始化、openssl clone 未完成（secureboot=0 不依赖 openssl）；`Binaries/enuma` 厂商二进制不入库。
- **已知启动链高风险（未改动共享 UniXNU，待 High 复核）**：
  - SEC 仅保留 FDT 指针 HOB，不负责 initrd 的映射/保留；`/chosen` 的 initrd 区间须由板级内存映射保留后才可安全发布 Block I/O。
  - 共享 `UniXNURamDiskDxe` 把 initrd 大小按 512 字节扇区取整，未占有补齐段、未检查溢出。
- 下一步：从 enuma 原厂输入提取并记录来源与 SHA-256；初始化 `Mu_Basecore` 后做 DEBUG 固件编译冒烟。

## 约定
规则见 `../../docs/governance-2026-11-06.md`（本机共享）。OEM 二进制与 vendor 输入放 `.cache/firmware/` 不入库；含 Apple 组件产物不上传（治理 §7）。
