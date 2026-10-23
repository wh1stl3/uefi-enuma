"""Prepare hash-locked enuma inputs for the official UniXNU firmware backend.

Only the device repository's ignored out/ is writable. This module never
downloads proprietary inputs, changes partitions, or changes memory values.
"""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def apply_patch_series(patches):
    for target in dict.fromkeys(target for target, _ in patches):
        if not (target / '.git').exists():
            subprocess.run(['git', 'init', '-q', str(target)], check=True)
    reverse_patches(patches)
    for target, patch in patches:
        subprocess.run(['git', 'apply', '--check', str(patch)], cwd=target, check=True)
        subprocess.run(['git', 'apply', str(patch)], cwd=target, check=True)


def reverse_patches(patches):
    for target, patch in reversed(patches):
        result = subprocess.run(['git', 'apply', '--reverse', '--check', str(patch)],
                                cwd=target, capture_output=True)
        if result.returncode == 0:
            subprocess.run(['git', 'apply', '--reverse', str(patch)], cwd=target, check=True)


def prepare(device, source, uefi, core, unixnu, reference, vendor):
    device, source, uefi, core, unixnu, reference, vendor = [Path(p).resolve() for p in
        (device, source, uefi, core, unixnu, reference, vendor)]
    for writable in (uefi, core):
        if not writable.is_relative_to(device / 'out'):
            raise ValueError('Firmware and Mu staging must be inside device out/')
    meta = source / 'Resources/Enuma'
    migration = json.loads((meta / 'migration.json').read_text())
    for item in migration['files'].values():
        if digest(source / item['destination']) != item['sha256']:
            raise ValueError('Migrated firmware input changed: ' + item['destination'])
    extensions = json.loads((meta / 'extensions.json').read_text())['files']
    for relative, wanted in extensions.items():
        if digest(source / relative) != wanted:
            raise ValueError('Firmware extension changed: ' + relative)
    vendor_lock = json.loads((meta / 'vendor.json').read_text())['files']
    for relative, wanted in vendor_lock.items():
        if digest(vendor / relative) != wanted:
            raise ValueError('Vendor input missing/changed: ' + relative)
    dependencies = json.loads((meta / 'dependencies.json').read_text())['reference_gitlinks']
    for relative, wanted in dependencies.items():
        path = reference / relative
        actual = subprocess.check_output(['git', '-c', 'safe.directory=' + path.as_posix(),
                                          '-C', str(path), 'rev-parse', 'HEAD'], text=True).strip()
        if actual != wanted:
            raise ValueError('Mu dependency revision mismatch: ' + relative)
    for relative in ('Common/Mu', 'Common/Mu_OEM_Sample'):
        link = uefi / relative
        link.parent.mkdir(parents=True, exist_ok=True)
        if not link.exists():
            link.symlink_to(reference / relative, target_is_directory=True)
        elif link.resolve() != (reference / relative).resolve():
            raise ValueError('Unexpected library materialization: ' + relative)
    # Copy only the published enuma overlay. Base Mu sources are staged by the
    # device wrapper from its separately pinned source archive.
    for item in migration['files'].values():
        relative = item['destination']
        if relative.startswith(('Platforms/', 'Resources/Configs/', 'patches/', 'diagnostics/')):
            target = uefi / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(source / relative, target)
    for relative in extensions:
        if relative.startswith(('patches/', 'diagnostics/')):
            target = uefi / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(source / relative, target)
    simplefb = 'Silicon/Silicium/SiliciumPkg/Drivers/SimpleFbDxe/SimpleFb.c'
    (uefi/simplefb).write_bytes((source/simplefb).read_bytes().replace(b'\r\n', b'\n'))
    target = uefi / 'Resources/Scripts/mkbootimg.py'
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(source / 'Resources/Scripts/mkbootimg.py', target)
    upstream = unixnu / 'drivers/DarwinBoot/DarwinBootPkg/Driver'
    interfaces = json.loads((meta / 'interfaces.json').read_text())
    probe = uefi / 'Platforms/Xiaomi/enumaPkg/Application/EnumaProbe'
    reused = {}
    for name in ('DarwinBoot.c', 'BootVideo.h', 'DiagFont.h'):
        relative = 'drivers/DarwinBoot/DarwinBootPkg/Driver/' + name
        if digest(upstream / name) != interfaces[relative]:
            raise ValueError('DarwinBoot interface changed: ' + name)
        reused[relative] = interfaces[relative]
        if name != 'DarwinBoot.c':
            shutil.copyfile(upstream / name, probe / name)
    original = (upstream / 'DarwinBoot.c').read_text()
    begin = original.index('STATIC\nEFI_STATUS\nGetMap (')
    end = original.index('STATIC\nEFI_STATUS\nAllocateWindow (', begin)
    (probe / 'DarwinProbe.inc').write_text('// SPDX-License-Identifier: BSD-2-Clause-Patent\n' + original[begin:end])
    for relative in ('Binaries', 'raw'):
        shutil.copytree(vendor / relative, uefi / relative, dirs_exist_ok=True)
    includes = uefi / 'EnumaInputs'; includes.mkdir(exist_ok=True)
    for name in ('Vendor.dsc.inc', 'Vendor.fdf.inc', 'Vendor.apriori.inc', 'Vendor.raw.fdf.inc'):
        shutil.copyfile(vendor / name, includes / name)
    sec = uefi / 'Silicon/Silicium/SiliciumPkg/Sec'
    dxe = core / 'MdeModulePkg/Core/Dxe'
    exception = core/'UefiCpuPkg/Library/CpuExceptionHandlerLib'
    for path in (sec/'Sec.c', sec/'Sec.inf', dxe/'DxeMain/DxeMain.c',
                 dxe/'DxeMain.inf', dxe/'Dispatcher/Dispatcher.c', dxe/'Image/Image.c', dxe/'Mem/Page.c',
                 exception/'AArch64/CpuExceptionHandlerLib.c', exception/'DxeCpuExceptionHandlerLib.inf'):
        path.write_bytes(path.read_bytes().replace(b'\r\n', b'\n'))
    # Remove previous diagnostic patches left by the legacy builder. The official
    # backend applies this series and reverts it in finally on every build.
    core_patches = [(core, uefi / name) for name in (
        'patches/dxe-stage-checkpoints.patch', 'patches/dxe-dispatch-return.patch',
        'patches/dxe-driver-checkpoints.patch', 'patches/dxe-dispatch-details.patch',
        'Resources/MuPatches/Gic-Spurious.patch')]
    reverse_patches(core_patches)
    reset = json.loads((meta / 'reset.json').read_text())
    for relative, wanted in reset['mu_reset_sha256'].items():
        if digest(core / relative) != wanted:
            raise ValueError('Reset implementation changed: ' + relative)
    for relative, wanted in reset['diagnostic_baseline_lf_sha256'].items():
        if relative.startswith(('MdeModulePkg/', 'UefiCpuPkg/')):
            if digest(core / relative) != wanted:
                raise ValueError('Mu diagnostic baseline changed: ' + relative)
    apply_patch_series([(uefi, source/'patches'/name) for name in (
        'kona-preserve-display.patch', 'sec-preserve-display.patch', 'sec-stage-checkpoints.patch',
        'dispatch-detail-options.patch', 'apriori-metronome.patch', 'diagnostic-ufs-selection.patch',
        'simplefb-contract.patch', 'simplefb-diagnostic-selection.patch')])
    for target in (sec, dxe/'DxeMain', dxe/'Dispatcher', dxe/'Mem'):
        shutil.copyfile(source/'diagnostics/EnumaStageCheckpoint.h', target/'EnumaStageCheckpoint.h')
    shutil.copyfile(source/'diagnostics/EnumaDispatchCheckpoint.h', dxe/'Dispatcher/EnumaDispatchCheckpoint.h')
    shutil.copyfile(source/'diagnostics/EnumaDispatchDetailCheckpoint.h', dxe/'Dispatcher/EnumaDispatchDetailCheckpoint.h')
    shutil.copyfile(source/'diagnostics/EnumaImageEntryCheckpoint.h', dxe/'Dispatcher/EnumaImageEntryCheckpoint.h')
    for name in ('EnumaStageCheckpoint.h', 'EnumaUnhandledExceptionCheckpoint.h'):
        shutil.copyfile(source/'diagnostics'/name, exception/'AArch64'/name)
    shutil.copyfile(source/'diagnostics/EnumaPoolSearchCheckpoint.h', dxe/'Mem/EnumaPoolSearchCheckpoint.h')
    report = {'scope': 'fixed inputs and writable staging, no firmware execution',
              'vendor_files_checked': len(vendor_lock), 'reference_gitlinks': dependencies,
              'probe_upstream_sha256': reused, 'core_ready_for_official_patch_series': True}
    (device/'reports/enuma-firmware-preflight.json').write_text(json.dumps(report, indent=2)+'\n')
    return report


def main():
    fields = ('DEVICE', 'SOURCE', 'UEFI', 'EDK2', 'UNIXNU', 'REFERENCE', 'VENDOR')
    values = [os.environ['ENUMA_' + field] for field in fields]
    prepare(*values)


if __name__ == '__main__': main()
