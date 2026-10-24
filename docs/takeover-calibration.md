# 接手后的校准与显示访问隔离

2026-10-24。用户要求按首轮建议及硬停要求推进，全部文档落地。

## 已完成与主判断

独立固件仓fast-forward到b63c9bd基线，递归子模块脏状态未动。恢复脚本只读核验身份/启动/root/槽位/解锁、设备及备份摘要；保存独立证据并更新current.local.json，失败试验原记录不改。

主判断（高置信）：旧取数证据和诊断编排不够可靠，是先要修复的工程问题。低置信硬件嫌疑按CPU映射/属性/cache、显示域状态、构建/代码布局与时序保留；没有依据宣布unclocked access或写指令已经定案。可推翻判据：有效编译宏/确切代码与0/3/0对照闭合可重建数值通道；同步异常ESR/FAR可把映射与外部访问故障区分；电源/时钟状态和读边界需自己的实测。

额外确认：dispatch-detail-options.patch未把ENUMA_DIAG_DELAY_FROM_DISPLAY和ENUMA_DIAG_DISPLAY_VALUE_INDEX传给C编译器；header默认display=0，未启用分支还有STAT typo。旧delay20时序不能作为寄存器读数。GOP_QUERY的detail16是CoreStartImage后精确marker门，不是PE入口前；旧上下文因果叙述失效。旧记录保留并以新决策supersede，不篡改当时观测。

像素函数先Sync，再Read，再二次Snapshot，再Write；需分别定位。SSPP_SRC0_ADDR是显示设备地址，范围过滤不证明CPU物理地址。GCD条目本身不等同页表。DisplayDxe依赖不满足也不能推出必须重写整个显示栈。

## 新通道与校准

--value-source constant/register/off，--value-code 0..15，--value-index 0..6，--value-shift 0..28(4步长)。固定step0/query1/detail16/delay2/GUID-only及completion fallback；旧delay20拒绝，旧step1..7恢复GOP元数据分步语义。

常量0/3不调用任何显示读或cache callback，3码=额外9秒；寄存器模式仅既有R-004映射内一次预读和一次目标读。码间距3秒，避免原一秒粒度与主机抖动重叠；无需更改reset、内存、映射或日志区。

0/3/0每笔均匹配相同源码/编译契约，以fastboot_return_seconds优先、否则adb_return_seconds取时；三笔须同一metric。两笔零码漂移<=0.75秒，已知码残差<=0.75秒后才能解码；未知码也要求<=0.75秒且0..15，模糊不舍入硬解。单笔固定时序并不能证明marker命中，已知码对照是必要证据。每个寄存器片段须记录index/shift，不能把一片nibble称完整地址。

run_value_probes默认离线检查不可变raw/FD/image/构建/审计/源码与C编译宏绑定，不重新打包；--execute最多一笔。issued先落盘，失败/中断/模糊冻结计划，原样失败镜像不重试。每笔后更新文档与结果、提交推送再执行下一笔。无完整资格即不启动；不通过降低验收条件推进。

## 后续里程碑

先校准常量；之后最小显示读，失败则停止追加并恢复，再离线隔离。校准不通过可转更宽可观察通道提案，但R-008仍pending，不启用保留区或warm reset。缓存/CPU读隔离和新像素实验另锁具体构建/摘要，单次像素授权不继承已consumed记录。

FAT publisher/完整ADT/donor预链接可独立离线准备；实际XNU入口、SM8250 UFS、APFS/信任/Data和用户态逐层验收。复用上游通用实现，仅适配板级差异。

## 公开来源（实施锁版本）

- UniXNU固定5fdbd5d，远端新HEAD bd127c仅差异待审：https://github.com/UniXNU/UniXNU
- LineageOS SM8250内核6b34d247：https://github.com/LineageOS/android_kernel_xiaomi_sm8250/tree/6b34d24765327d746074c8e53b5ad0d24a51acc7
- SSPP布局：https://github.com/LineageOS/android_kernel_xiaomi_sm8250/blob/6b34d24765327d746074c8e53b5ad0d24a51acc7/techpack/display/msm/sde/sde_hw_sspp.c
- DMA/IOMMU：https://github.com/LineageOS/android_kernel_xiaomi_sm8250/blob/6b34d24765327d746074c8e53b5ad0d24a51acc7/drivers/gpu/drm/msm/msm_gem.c
- 社区elish/enuma设备树0d41d4d2，仍有elish专用配置，不能当本机完整事实：https://github.com/xiaomi-enuma-dev/android_device_xiaomi_elish_enuma/tree/0d41d4d29ae7329a454c6bb4e4b8c66331177249
- MiCode原有enuma DTS/kernel锁43b11af4/52a2f18维持。专有firmware/XML按本机来源锁；Android ELF HAL不可直接用作Darwin驱动。仅公开实现缺失接口再逆向PE/ELF，不补空协议冒充实现。
