"""Firmware-only wrapper around the pinned official UniXNU [firmware] backend.

Sources remain clean; outputs and writable Mu patch staging belong to out/.
The top-level example guard is preserved; this does not build an enuma kernel.
"""
import argparse
import copy
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tarfile
import uuid
from types import SimpleNamespace
SOURCE = Path(__file__).resolve().parents[2]
ROOT = None
WORK = None


def diagnostic_guid_defines(value):
    if value is None:
        return {'ENUMA_DIAG_GUID_ENABLED':'0', 'ENUMA_DIAG_GUID_LOW64':'0', 'ENUMA_DIAG_GUID_HIGH64':'0'}
    data = uuid.UUID(value).bytes_le
    return {'ENUMA_DIAG_GUID_ENABLED':'1',
            'ENUMA_DIAG_GUID_LOW64':'0x'+data[:8][::-1].hex()+'ULL',
            'ENUMA_DIAG_GUID_HIGH64':'0x'+data[8:][::-1].hex()+'ULL'}


def source_helper():
    spec = importlib.util.spec_from_file_location('enuma_firmware_inputs', SOURCE / 'Resources/Scripts/enuma_preflight.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


# Keep the test API lazy: the CLI must check the source pin before executing
# any code from the firmware checkout.
def apply_patch_series(patches):
    return source_helper().apply_patch_series(patches)


def stage_archives(lock):
    WORK.mkdir(parents=True, exist_ok=True)
    for name, url in lock['archives'].items():
        if name == 'uefi-source.tar.gz': continue  # UEFI comes from the fixed source gitlink
        archive = WORK / name
        if not archive.exists():
            subprocess.run(['curl', '-fL', '--retry', '2', '--connect-timeout', '10', '--max-time', '90',
                            '-o', str(archive), url], check=True)
        if hashlib.sha256(archive.read_bytes()).hexdigest() != lock['archive_sha256'][name]:
            raise ValueError('source archive hash mismatch: ' + name)
        root = WORK / ('uefi' if name == 'uefi-source.tar.gz' else 'Mu_Basecore')
        marker = root / '.enuma-archive-staged'
        if marker.exists(): continue
        with tarfile.open(archive) as source:
            members = source.getmembers(); prefix = members[0].name.split('/')[0] + '/'
            for item in members:
                if not item.isfile(): continue
                relative = item.name.removeprefix(prefix)
                if name == 'uefi-source.tar.gz' and not relative.startswith(('Silicon/', 'BootShim/', 'Resources/MuPatches/')) and relative not in ('.gitmodules', 'LICENSE'):
                    continue
                target = (root / relative).resolve()
                if not target.is_relative_to(root.resolve()): raise ValueError('archive path escapes staging')
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(source.extractfile(item).read())
        marker.write_text(lock['archive_sha256'][name])


def main(device_root=None):
    global ROOT, WORK
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--device-root', type=Path, default=device_root, required=device_root is None, help='device contract repository; all outputs stay in its out/')
    parser.add_argument('--reference', type=Path, default=SOURCE)
    parser.add_argument('--unixnu', type=Path, required=True)
    parser.add_argument('--jobs', type=int, default=4)
    parser.add_argument('--stop-stage', type=int, choices=(0,3,4,5,6,7,8,9,10,11,12,13), default=0)
    parser.add_argument('--stop-delay', type=int, choices=(2,7), default=2)
    parser.add_argument('--dispatch-ordinal', type=int, default=1)
    parser.add_argument('--detail-point', type=int, choices=range(13,28), default=13)
    parser.add_argument('--dispatch-guid', help='optional complete file UUID filter for stage 13 detail gates')
    parser.add_argument('--dispatch-guid-only', action='store_true', help='select a complete file GUID without a guessed runtime ordinal; use ordinal 1 as placeholder')
    parser.add_argument('--dispatch-completion-fallback', action='store_true', help='GUID-only diagnostic: reset 7 s after initial CoreDispatcher returns, before missing-arch checks/BDS; cannot recover hangs')
    selection = parser.add_mutually_exclusive_group()
    selection.add_argument('--diagnostic-simplefb',action='store_true',help='metadata-only SimpleFb; omit exact CPR/Display/UFS set, no pixel drawing')
    selection.add_argument('--diagnostic-without-ufs',action='store_true',help='GOP isolation only; default firmware retains OEM UFSDxe')
    parser.add_argument('--diagnostic-gop-query', type=int, choices=(0,1,2), default=0, help='1: reset on read-only GOP audit pass; 2: reset on failure; 0: disabled')
    parser.add_argument('--diagnostic-gop-step', type=int, choices=range(13), default=0, help='0: control; 1: enumerate/free; 2: HandleProtocol; 3: full Mode/span; 4: QueryMode; 5: Mode pointer presence; 6: Mode header; 7: Info fields; 8: read-only MDP descriptor predicate; 9: splash source address presence; 10: candidate linear packing; 11: fixed candidate geometry/span; 12: decoded bounded descriptor')
    args = parser.parse_args()
    ROOT = args.device_root.resolve()
    WORK = ROOT / 'out/firmware-build'
    if args.diagnostic_gop_step and not args.diagnostic_gop_query:
        parser.error('GOP steps require --diagnostic-gop-query')
    if args.diagnostic_gop_query and not (args.diagnostic_simplefb and args.stop_stage == 13 and args.stop_delay == 2 and args.detail_point == 16 and args.dispatch_guid_only and args.dispatch_completion_fallback and args.dispatch_guid and args.dispatch_guid.lower() == 'dcfd1e6d-788d-4ffc-8e1b-ca2f75651a92'):
        parser.error('GOP query requires SimpleFb GUID-only stage13/detail16/delay2 with completion fallback')
    if not 1 <= args.dispatch_ordinal <= 128: parser.error('--dispatch-ordinal must be 1..128')
    if args.dispatch_guid_only and (args.stop_stage != 13 or args.dispatch_guid is None or args.dispatch_ordinal != 1 or args.detail_point in (23,26,27)):
        parser.error('GUID-only requires stage 13, complete GUID, driver detail and ordinal 1 placeholder')
    if args.dispatch_completion_fallback and not args.dispatch_guid_only:
        parser.error('dispatch completion fallback requires --dispatch-guid-only')
    if args.detail_point in (23,26,27) and (args.dispatch_guid is not None or args.dispatch_ordinal != 1):
        parser.error('global exception/pool point has no driver GUID/ordinal attribution; omit GUID and use ordinal 1')
    if args.stop_stage != 13 and (args.detail_point != 13 or args.dispatch_guid is not None):
        parser.error('detail point and GUID filters require --stop-stage 13')
    try:
        guid_defines = diagnostic_guid_defines(args.dispatch_guid)
    except ValueError:
        parser.error('--dispatch-guid must be a valid UUID')
    if sys.version_info < (3,11):
        import tomli
        sys.modules['tomllib'] = tomli
    import tomllib
    pin = json.loads((ROOT / 'config/uefi-source.lock.json').read_text())
    def git(path, *argv):
        return subprocess.check_output(['git','-c','safe.directory='+path.as_posix(),'-C',str(path),*argv],text=True).strip()
    if git(SOURCE, 'rev-parse', 'HEAD') != pin['commit']:
        raise ValueError('uefi-enuma revision differs from the device lock')
    if git(SOURCE, 'status', '--porcelain', '--ignore-submodules=all'):
        raise ValueError('uefi-enuma source must be clean')
    for name, wanted in pin['file_sha256'].items():
        if hashlib.sha256((SOURCE/name).read_bytes()).hexdigest() != wanted:
            raise ValueError('uefi-enuma source hash mismatch: '+name)
    spec = importlib.util.spec_from_file_location('enuma_device_upstream', ROOT/'scripts/upstream_source.py')
    upstream = importlib.util.module_from_spec(spec); spec.loader.exec_module(upstream)
    qualification = upstream.qualify(args.unixnu)
    lock = json.loads((ROOT/'config/firmware-build.lock.json').read_text())
    stage_archives(lock)
    uefi = WORK/'uefi'; core = WORK/'Mu_Basecore'
    # Export the actual fixed firmware checkout, never write patches into it.
    for relative in ('BootShim','Silicon','Resources/MuPatches'):
        shutil.copytree(SOURCE/relative, uefi/relative, dirs_exist_ok=True,
                        ignore=shutil.ignore_patterns('.git','Build','__pycache__'))
    # The official backend reads the packaging profile before its preflight.
    for relative in ('Resources/Configs/enuma.toml','Resources/Scripts/mkbootimg.py'):
        target=uefi/relative; target.parent.mkdir(parents=True,exist_ok=True)
        shutil.copyfile(SOURCE/relative,target)
    # A Git metadata view lets the official backend read the source timestamp;
    # no checkout or submodule operation modifies the published source.
    if not (uefi/'.git').exists(): subprocess.run(['git','init','-q',str(uefi)],check=True)
    objects = Path(git(SOURCE, 'rev-parse', '--absolute-git-dir')) / 'objects'
    info = uefi/'.git/objects/info'; info.mkdir(parents=True,exist_ok=True)
    (info/'alternates').write_text(objects.as_posix()+'\n')
    subprocess.run(['git','-C',str(uefi),'update-ref','refs/heads/enuma-source',pin['commit']],check=True)
    subprocess.run(['git','-C',str(uefi),'symbolic-ref','HEAD','refs/heads/enuma-source'],check=True)
    for relative in ('BaseTools/Source/C/BrotliCompress/brotli', 'MdeModulePkg/Library/BrotliCustomDecompressLib/brotli',
                     'MdePkg/Library/BaseFdtLib/libfdt', 'MdePkg/Library/MipiSysTLib/mipisyst',
                     'SecurityPkg/DeviceSecurity/SpdmLib/libspdm'):
        target=core/relative
        if not target.exists() or not any(target.iterdir()):
            shutil.copytree(args.reference/'Mu_Basecore'/relative,target,dirs_exist_ok=True,
                            ignore=shutil.ignore_patterns('.git','unit_test','os_stub','doc','samples'))
    tree=WORK/'tree'; (tree/'scripts').mkdir(parents=True,exist_ok=True); (tree/'drivers').mkdir(exist_ok=True)
    for origin, link in ((args.unixnu/'package',tree/'package'),
                         (args.unixnu/'drivers/DarwinBoot',tree/'drivers/DarwinBoot')):
        if not link.exists(): link.symlink_to(origin.resolve(),target_is_directory=True)
        elif link.resolve()!=origin.resolve(): raise ValueError('Unexpected official tree link')
    shutil.copyfile(SOURCE/'Resources/Scripts/enuma_firmware_preflight.py',tree/'scripts/enuma_firmware_preflight.py')
    for field, value in {'DEVICE':ROOT,'SOURCE':SOURCE,'UEFI':uefi,'EDK2':core,
                         'UNIXNU':args.unixnu,'REFERENCE':args.reference,'VENDOR':ROOT/'out/firmware-vendor'}.items():
        os.environ['ENUMA_'+field]=str(value.resolve())
    wrappers=core/'BaseTools/BinWrappers/PosixLike'
    for path in wrappers.iterdir():
        if path.is_file(): path.chmod(path.stat().st_mode | 0o111)
    os.environ.update(PATH=str(wrappers)+os.pathsep+os.environ['PATH'],
        PYTHONPATH=str(core/'BaseTools/Source/Python'), PYTHON_COMMAND=sys.executable,
        CLANG_BIN=str(Path(shutil.which('clang')).parent)+'/', SOURCE_DATE_EPOCH='1792540800',
        PYTHONDONTWRITEBYTECODE='1')
    sys.path.insert(0,str(args.unixnu/'scripts'))
    from unixnu import firmware
    board=tomllib.loads((ROOT/'board.toml').read_text())
    assert board['board']['example'] is True
    board=copy.deepcopy(board)
    board['firmware']['defines'].update(ENUMA_DIAG_STAGE=str(args.stop_stage),
        ENUMA_DIAG_DELAY_US=str(args.stop_delay*1000000),ENUMA_DIAG_DISPATCH_ORDINAL=str(args.dispatch_ordinal),
        ENUMA_DIAG_DETAIL_POINT=str(args.detail_point),ENUMA_DIAG_DISABLE_UFS='1' if (args.diagnostic_without_ufs or args.diagnostic_simplefb) else '0',
        ENUMA_DIAG_SIMPLEFB='1' if args.diagnostic_simplefb else '0',
        ENUMA_DIAG_GOP_QUERY=str(args.diagnostic_gop_query),
        ENUMA_DIAG_GOP_STEP=str(args.diagnostic_gop_step),
        ENUMA_DIAG_GUID_ONLY='1' if args.dispatch_guid_only else '0',
        ENUMA_DIAG_DISPATCH_FALLBACK='1' if args.dispatch_completion_fallback else '0', **guid_defines)
    ctx=SimpleNamespace(board=board,args=SimpleNamespace(skip_preflight=False),tree=tree,
                        logs=WORK/'logs',work=WORK/'context',jobs=args.jobs)
    fd=uefi/board['firmware']['fd'].format(target='DEBUG')
    report={'scope':'firmware-only invocation of the official backend; no device boot or enuma kernel',
        'complete':False,'fd':str(fd.relative_to(ROOT)),'uefi_source_commit':pin['commit'],
        'official_backend_sha256':hashlib.sha256((args.unixnu/'scripts/unixnu/firmware.py').read_bytes()).hexdigest(),
        'unixnu_qualification':qualification,'firmware_configuration':board['firmware'],
        'diagnostic_stop_stage':args.stop_stage,'diagnostic_delay_seconds':args.stop_delay,
        'diagnostic_dispatch_ordinal':args.dispatch_ordinal,'archive_sha256':lock['archive_sha256'],
        'diagnostic_detail_point':args.detail_point,
        'diagnostic_dispatch_guid':str(uuid.UUID(args.dispatch_guid)) if args.dispatch_guid else None,
        'diagnostic_dispatch_guid_only':args.dispatch_guid_only,
        'diagnostic_dispatch_completion_fallback':args.dispatch_completion_fallback,
        'diagnostic_dispatch_completion_delay_seconds':7 if args.dispatch_completion_fallback else None,
        'diagnostic_omitted_oem_modules':(['CPRDxe','DisplayDxe','UFSDxe'] if args.diagnostic_simplefb else ['UFSDxe'] if args.diagnostic_without_ufs else []),
        'diagnostic_simplefb_metadata_only':args.diagnostic_simplefb,
        'diagnostic_gop_query':args.diagnostic_gop_query,
        'diagnostic_gop_step':args.diagnostic_gop_step,
        'diagnostic_gop_reset_owner':'DxeCore' if args.diagnostic_gop_query else None,
        'diagnostic_gop_transport':'core-warning-v2' if args.diagnostic_gop_query else 'disabled',
        'reference_gitlinks':lock['reference_gitlinks'],'checks':[]}
    try:
        firmware.build(ctx,{},uefi,core)
        report['complete']=fd.is_file()
        report['fd_exists']=fd.is_file()
        if fd.is_file():report['fd_sha256']=hashlib.sha256(fd.read_bytes()).hexdigest()
        report['source_inputs_sha256']={
            'firmware/'+name:hashlib.sha256((SOURCE/name).read_bytes().replace(b'\r\n',b'\n')).hexdigest()
            for name in pin['file_sha256'] if Path(name).suffix in ('.c','.h','.inf','.inc','.dsc','.fdf','.patch','.toml','.py','.json')}
        report['probe_upstream_sha256']=json.loads((ROOT/'reports/enuma-firmware-preflight.json').read_text())['probe_upstream_sha256']
        baseline=json.loads((SOURCE/'Resources/Enuma/reset.json').read_text())['diagnostic_baseline_lf_sha256']
        report['core_patch_reversion_checked']=all(hashlib.sha256((core/n).read_bytes()).hexdigest()==sha
            for n,sha in baseline.items() if n.startswith(('MdeModulePkg/','UefiCpuPkg/')))
        if not report['core_patch_reversion_checked']:raise ValueError('Official backend left core patches applied')
        if args.diagnostic_gop_query:
            module=uefi/'Build/enumaPkg/DEBUG_CLANGPDB/AARCH64/SiliciumPkg/Drivers/SimpleFbDxe/SimpleFbDxe'
            autogen=(module/'DEBUG/AutoGen.c').read_text()
            libraries=(module/'OUTPUT/static_library_files.lst').read_text()
            relative='Silicon/Silicium/SiliciumPkg/Drivers/SimpleFbDxe/SimpleFbDxe.inf'
            assert (uefi/relative).read_bytes().replace(b'\r\n',b'\n')==(SOURCE/relative).read_bytes().replace(b'\r\n',b'\n')
            assert 'ArmPsciResetSystemLibConstructor' not in autogen
            assert not any(name in libraries.lower() for name in ('pscireset','resetsystemlib'))
            report['gop_provider_generated_contract']={
                'inf_matches_baseline':True,'reset_library_absent':True,'psci_constructor_absent':True,
                'autogen_sha256':hashlib.sha256(autogen.encode()).hexdigest(),
                'static_library_list_sha256':hashlib.sha256(libraries.encode()).hexdigest(),
                'core_checkpoint_sha256':hashlib.sha256((SOURCE/'diagnostics/EnumaStageCheckpoint.h').read_bytes()).hexdigest()}
        report['checks']=[{'backend':'unixnu.firmware.build','exit_code':0,'log':str((ctx.logs/'firmware.log').relative_to(ROOT))}]
    except Exception as error:
        report['complete']=False;report['error']=str(error);raise
    finally:
        (ROOT/'reports/enuma-firmware-build.json').write_text(json.dumps(report,indent=2)+'\n')
    print('PASS: official firmware backend and patch reversion; FD '+report['fd_sha256'])


if __name__=='__main__':main()
