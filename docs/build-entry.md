# Firmware build ownership

`Resources/Scripts/build_enuma_firmware.py` owns firmware build orchestration and calls the pinned official UniXNU backend. Run in Linux/WSL with Python 3.11+ and the existing toolchain:

```sh
python Resources/Scripts/build_enuma_firmware.py --device-root /path/to/UniXNU-enuma --unixnu /path/to/pinned/UniXNU --reference /path/to/fixed/reference [diagnostic options]
```

The explicit device repository supplies board.toml, dependency/source locks, proprietary-input references and upstream qualification policy. This entry checks this firmware checkout against the device source lock before staging it. All staging, logs and outputs remain in the device repository's ignored out/; reports remain in its reports/. No output is written into firmware sources or reference repositories.

The device scripts/build_enuma_firmware.py is a compatibility adapter that verifies the pinned checkout before importing this implementation. Android boot packaging, device probes, backups/recovery and handoff documents remain device-owned. This is not a standalone build without the device contract and required local inputs. No boot/flash command is executed by this entry.
