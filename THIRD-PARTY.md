# 固件来源与许可

底座固定为 UniXNU/uefi-sheng `329b473df1e53a3bfa64b2b39b79670c4c02279b`，源于 Mu-Silicium。上游根 `LICENSE` 为 BSD-2-Clause；本目录所衍生 DSC/FDF/PlatformSec 文件明确标注 BSD-2-Clause-Patent，保留各文件原有版权与 SPDX 标头，许可证全文在本目录 LICENSE。APRIORI/DXE 的包引用不包含所引用模块的源码或二进制，其许可仍由各组件决定。

来源文件和摘要固定于 `config/porting-sources.lock.json`；只下载选定文件到忽略目录 out/references。引用 UniXNU/UniXNUPkg、DarwinBootPkg 的实现，不将其源码复制进 Git。EnumaProbe 构建时在 out/ 中原样复用 DarwinBoot 的 GetMap、RunEnd、DiagDraw、BootVideo.h 和 DiagFont.h；保留头部许可，DiagFont.h 的 Android 原始版权/许可保持。未分发厂商 Device-Binaries，不能由开源包的许可推断厂商二进制的再分发权限。

本轮新增从用户当前enuma的XBL本地提取厂商驱动及K81资源。原始cfg、PE/DEPEX和面板XML只放out/，不随Git提交；来源摘要及模块清单保留在reports/。未独立验证厂商签名或与官方ROM匹配，不用开源底座许可证覆盖这些厂商二进制。内存与数值配置表由设备事实重新生成。提取工具固定uefi_firmware 1.16；Android打包工具固定AOSP 028bdc06，Apache-2.0原许可保留在引用树中。
