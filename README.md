# uefi-enuma

Mu-Silicium firmware source for Xiaomi Pad 5 Pro 5G (enuma / SM8250). Companion device repository: [UniXNU-enuma](https://github.com/wh1stl3/UniXNU-enuma).

Based on UniXNU/uefi-sheng `329b473df1e53a3bfa64b2b39b79670c4c02279b`. Existing enuma platform files, six patches and diagnostic headers are imported from device commit `1ab7ee8`, with file hashes in `Resources/Enuma/migration.json`. The upstream root license is preserved; device additions retain `LICENSE.enuma` and their SPDX notices.

This repository owns `Platforms/Xiaomi/enumaPkg`, `Resources/Configs/enuma.toml`, `diagnostics/`, `patches/` and the enuma preflight. Mu submodules remain pinned. No enuma OEM binaries, raw XBL, XML/config originals, device backups or serial numbers are published. The foreign Device-Binaries/ACPI/OpenSSL gitlinks are removed from the enuma dependency graph.

Build through the companion device repository's `board.toml [firmware]` and `scripts/build_enuma_firmware.py`. It stages sources under the device's ignored `out/`, runs the official UniXNU firmware backend and invokes this repository's preflight. Mu core patches are applied and reverted by that backend. The preflight verifies local vendor hashes before staging private inputs.

The wrapper accepts `--unixnu`, `--reference` (a checkout containing the pinned Mu submodules; defaults to the firmware source) and diagnostic `--stop-stage`, `--stop-delay`, `--dispatch-ordinal`. Private vendor inputs must already be extracted into the device repository's `out/firmware-vendor`.

The companion also supports `--dispatch-guid-only --dispatch-guid <full UUID>` for stage 13 driver detail gates, with ordinal 1 as an unused placeholder. Optional `--dispatch-completion-fallback` resets seven seconds after the initial `CoreDispatcher` returns, before architectural-protocol checks and BDS. This is a synchronous normal-path fallback, not a watchdog: a hung driver or dispatcher prevents it from running. Neither a fallback reset nor static APRIORI position proves target execution. Defaults retain ordinal matching and disable the fallback.

The device remains `example=true`: the official top-level build guard still rejects an incomplete full-device build. The explicitly scoped wrapper builds firmware only; it does not prelink XNU, boot a device, change memory values or set verification flags. Android boot v3 packaging remains in the device repository; generic gzip/v0 packaging must not replace it.

Status, recovery rules and evidence: [device STATUS](https://github.com/wh1stl3/UniXNU-enuma/blob/main/docs/STATUS.md). Current physical evidence reaches the first scheduled queue entry before loading; GOP and dispatch completion are unproven. Imported `docs/enuma-overlay.md` is historical prototype documentation, not the current status.
