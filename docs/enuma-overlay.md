# enuma 固件接入

底座为 UniXNU/uefi-sheng 329b473df1e53a3bfa64b2b39b79670c4c02279b 的 KonaPkg，Mu_Basecore cfb93724 与 UniXNU feeds 一致。设备增量在 Platforms/Xiaomi/enumaPkg；完整源码和输出只放忽略的 out/。

本机原厂 XBL 已只读采集并锁定 SHA256，从中提取45项内存/MMIO区域、31项数值配置、33个 AArch64 厂商驱动及原始 DEPEX/APRIORI、两份 K81 面板资源。MemoryMapLib、ConfigurationMapLib、PlatformConfig.inc 和 Resources/Configs/enuma.toml 从这些事实生成。原厂二进制与原始配置仅保存在 out/firmware-vendor，不提交或再分发。

GOP 使用本机 DisplayDxe、CPRDxe 和 K81 XML；1600×2560 同时得到当前 Android DRM 和原厂面板资源支持。framebuffer 的基址、stride、格式由运行时 GOP 提供，不用面板24bpp链路参数推断它们。DEBUG 早期 SerialPortLib 先使用 Null，避免基于未知 framebuffer 写屏。

补丁包括：Kona 保留 MDP SMMU 映射；SEC 跳过清空整个 Display Reserved（该区域包含 DFPS）；复用参考 GIC 未处理 IRQ 屏蔽补丁。完整编译在独立可写归档工作区执行，补丁根目录单独初始化，参考仓只读。

完整 FD 构建和最终 PE/DEPEX 字节审计结果见 reports/enuma-firmware-build.json、reports/enuma-fd-audit.json；候选镜像见 reports/enuma-boot-candidate.json。候选用于首阶段固件实验，未附带 XNU/iPadOS payload，也没有实机启动结论。

打包使用固定 AOSP 工具的 Android boot v3、raw BootShim+FD、空 ramdisk，保留原厂 vendor_boot。当前固定 UniXNU 通用 bootimg.py 的 gzip+追加DTB及旧头部校验不适用于此布局，因此使用 scripts/package_enuma_boot.py，并只复用其 compile_shim。

完整操作与必须实机确认的项目见 docs/operations/2026-10-22-native-platform-firmware.md。生成镜像和可恢复启动的授权已记录于 docs/NEED-REVIEW.md A-002；owned-memory仍为获批512 MiB，物理内存窗口需实际UEFI图确认。
