# uefi-enuma

小米平板 5 Pro 5G（enuma / SM8250-AC）的 Mu-Silicium UEFI 固件仓。

- 基线：UniXNU/uefi-sheng@66421cb7bb5694843aec8de1ca3d1de4c20cca38（2026-11-06 派生，已去除 sheng 历史）。

## 当前进度（2026-11-06）
- 阶段：**基座落位完成，未开始板级适配**（官方 steps 第 5 步（UEFI 集成）待启动）。
- 下一步：官方 steps 第 5 步——建立 enumaPkg 板级差异（SM8250 显示/时钟链路有旧轮经验可参考，按官方阶段重新验收）。

## 约定
规则见 `../docs/governance-2026-11-06.md`（本机共享）。OEM 二进制与 vendor 输入放 `.cache/firmware/` 不入库；含 Apple 组件产物不上传（治理 §7）。
