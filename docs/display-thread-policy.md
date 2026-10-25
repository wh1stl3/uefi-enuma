# enuma DisplayDxe thread policy

After full-input S-001 timed out and was manually recovered with original
partition digests intact, change only EnableDisplayThread from 1 to 0.
The tested same-SM8250 [umi configuration](https://github.com/reAAAq/uefi-umi/blob/48bc41218c480519bfd299c22205f44e418df9d0/Platforms/Xiaomi/umiPkg/Library/ConfigurationMapLib/ConfigurationMapLib.c)
also uses zero. No foreign addresses, memory fields, CPU count or other
thread/log/storage policy are imported.

Actual enuma DisplayDxe SHA256
6c6351c3c770d4f698e5e22097021fe8d20d35a8d4181df05676a48c4347f425
loads "EnableDisplayThread" through the real UEFI Config Lib GetCfgInfoVal
slot0x18 (helper RVA0x3874, indirect branch0x38bc). Its zero branch at0x1bfc
goes to0x1c60 and avoids the ThreadCreate/Resume path. The original PE remains
unchanged. The existing PlatformPei shim maps this slot to the real
LocateConfigurationEntry32/provider, not a fabricated success stub.

This only removes the display entry's request for asynchronous work. It does
not prove synchronous panel initialization, GOP mode, visible pixels, or the
cause of S-001. If runtime Kernel interface was absent, old code could already
clear the flag and return; in that case this policy may have no effect.
Other OEM entry work and later BDS/SetMode can still fail. Full FAT/13-stage
transport remains enabled for the next qualified candidate; no early reset
is introduced. Device execution remains governed by companion AGENTS.
