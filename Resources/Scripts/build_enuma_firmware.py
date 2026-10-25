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
import re
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
    parser.add_argument('--gop-backend-selftest', action='store_true', help='offline link qualification: include non-autostart QEMU RAM-only GOP self-test; not a physical GOP candidate')
    parser.add_argument('--inherited-video-audit', action='store_true', help='read-only same-boot video audit instead of metadata SimpleFb; no GOP/pixels; requires exact bounded SimpleFb query profile')
    parser.add_argument('--inherited-video-entry-control', action='store_true', help='new isolation: same inherited-video INF/constructors, constant PASS body, no MMIO/map query')
    parser.add_argument('--inherited-video-terminal', action='store_true', help='both PASS/FAIL markers terminate at the same exact driver-return hook, delay2/7; no dispatcher fallback')
    parser.add_argument('--inherited-video-topology-query', type=int, choices=(0,1,2), default=0, help='first-status classification: 1 topology, 2 not-topology; 0 original full predicate; never GOP admission')
    parser.add_argument('--inherited-video-reason-target', choices=('off','counter-enable','stability','interface','refresh'),default='off',help='first-return reason predicate; never relaxes sampling')
    parser.add_argument('--inherited-video-reason-query',type=int,choices=(0,1,2),default=0,help='1 selected reason, 2 exact complement; 0 off')
    parser.add_argument('--native-route-blend',action='store_true',help='Explain captured blend rejection only; no extra reads')
    parser.add_argument('--native-route-layout',action='store_true',help='Classify first mixer layout rejection; no extra reads')
    parser.add_argument('--native-route-staging',action='store_true',help='Classify captured staging words only; no extra hardware read')
    parser.add_argument('--native-route-detail',action='store_true',help='Classify CTL/staging/mixer topology location without extra reads')
    parser.add_argument('--native-route-audit',action='store_true',help='Read-only native STATUS gated full video pipeline audit; five result categories')
    parser.add_argument('--native-mask-field',type=int,choices=range(8),default=0,help='0 off;1 active,2 timing,3 enable-stable,4 moving,5 exact-enable3,6 same-observation active/timing/enable/moving conjunction,7 all-zero timing with active/stable-enable/moving. Codes0..3 plus error use bounded2/7/12/17/22s cold terminal')
    parser.add_argument('--native-observe-control',action='store_true',help='same INF/constructors, query1 constant PASS/query2 constant FAIL, no map/MMIO for boot-clock calibration')
    parser.add_argument('--native-observe-query',type=int,choices=(0,1,2),default=0,help='independent native observer: 1 both links active/stable/moving, 2 exact complement; no GOP admission')
    parser.add_argument('--ram-payload', type=Path, help='build-only: exact FAT in device out/; enable bounded boot-v3 RAM BlockIO and batch report')
    parser.add_argument('--ramoops-canary-nonce', help='R-017 single console marker; exactly 32 lower hex digits; stage10/delay2/SimpleFb-omit profile only')
    parser.add_argument('--ramoops-map-control', action='store_true', help='isolation: same console map, original stage10/cold gate, no console contents accessed')
    parser.add_argument('--ramoops-read-header', action='store_true', help='three console header loads only; same map/stage10/cold profile')
    parser.add_argument('--ramoops-warm-control', action='store_true', help='same map/stage10/delay2, warm request, no console accesses')
    parser.add_argument('--stop-stage', type=int, choices=(0,3,4,5,6,7,8,9,10,11,12,13), default=0)
    parser.add_argument('--stop-delay', type=int, choices=(2,7), default=2)
    parser.add_argument('--dispatch-ordinal', type=int, default=1)
    parser.add_argument('--detail-point', type=int, choices=range(13,28), default=13)
    parser.add_argument('--dispatch-guid', help='optional complete file UUID filter for stage 13 detail gates')
    parser.add_argument('--dispatch-guid-only', action='store_true', help='select a complete file GUID without a guessed runtime ordinal; use ordinal 1 as placeholder')
    parser.add_argument('--dispatch-completion-fallback', action='store_true', help='GUID-only diagnostic: reset 7 s after initial CoreDispatcher returns, before missing-arch checks/BDS; cannot recover hangs')
    selection = parser.add_mutually_exclusive_group()
    selection.add_argument('--diagnostic-simplefb',action='store_true',help='metadata GOP provider; omit exact CPR/Display/UFS set; pixel experiment needs its separate explicit flag/review')
    selection.add_argument('--diagnostic-without-ufs',action='store_true',help='GOP isolation only; default firmware retains OEM UFSDxe')
    parser.add_argument('--diagnostic-gop-query', type=int, choices=(0,1,2), default=0, help='1: reset on read-only GOP audit pass; 2: reset on failure; 0: disabled')
    parser.add_argument('--diagnostic-gop-step', type=int, choices=range(24), default=0, help='0..12: existing read-only stages; 13: protected pixels; 14: DSI timing; 15: counter configuration; 16: bounded frame progress; 17: native active status; 18: two native frame windows; 19: staged source and selected DSI progress; 20: stable contained 1:1 output geometry; 21: conservative single-source no-dim blend; 22: frozen paired six-class timing; 23: isolated sole staging, native border allowed')
    parser.add_argument('--diagnostic-pixel-preview',action='store_true',help='BUILD ONLY R-005 proposal; step13/query1 only; hardware execution requires separate explicit pixel approval')
    parser.add_argument('--value-source', choices=('off','constant','register','raw'), default='off', help='separate calibrated transport; raw is the same target read as register without the priming access; never repurpose GOP steps')
    parser.add_argument('--value-code', type=int, choices=range(16), default=0)
    parser.add_argument('--value-index', type=int, choices=range(7), default=0)
    parser.add_argument('--value-shift', type=int, choices=range(0,29,4), default=0)
    args = parser.parse_args()
    ROOT = args.device_root.resolve()
    WORK = ROOT / 'out/firmware-build'
    if args.ramoops_canary_nonce:
        os.environ['ENUMA_RAMOOPS_CANARY'] = '1'
    else:
        os.environ.pop('ENUMA_RAMOOPS_CANARY', None)
    os.environ['ENUMA_RAMOOPS_WARM_CONTROL'] = '1' if args.ramoops_warm_control else '0'
    os.environ['ENUMA_RAMOOPS_READ_HEADER'] = '1' if args.ramoops_read_header else '0'
    if args.ramoops_canary_nonce is not None or args.ramoops_map_control or args.ramoops_read_header or args.ramoops_warm_control:
        if (sum([args.ramoops_canary_nonce is not None, args.ramoops_map_control, args.ramoops_read_header, args.ramoops_warm_control]) != 1 or
            (args.ramoops_canary_nonce is not None and (args.ramoops_map_control or re.fullmatch(r'[0-9a-f]{32}', args.ramoops_canary_nonce) is None)) or
            args.stop_stage != 10 or args.stop_delay != 2 or not args.diagnostic_simplefb or
            args.diagnostic_gop_query or args.diagnostic_gop_step or args.value_source != 'off' or
            args.inherited_video_audit or args.gop_backend_selftest or not args.ram_payload):
            parser.error('R-017 requires mutually exclusive nonce/map-only control, stage10/delay2, SimpleFb omission, full FAT, no other probe')
    if args.value_source != 'off':
        if not (args.diagnostic_gop_query == 1 and args.diagnostic_gop_step == 0 and args.stop_delay == 2):
            parser.error('value transport requires step0/query1/delay2; historical display-delay mode is excluded')
        if args.value_source in ('register','raw') and args.value_code:
            parser.error('register/raw transport does not accept a constant code')
        if args.value_source == 'constant' and (args.value_index or args.value_shift):
            parser.error('constant transport has no register index or shift')
    elif args.value_code or args.value_index or args.value_shift:
        parser.error('value parameters require an explicit value source')
    if args.diagnostic_gop_step and not args.diagnostic_gop_query:
        parser.error('GOP steps require --diagnostic-gop-query')
    if args.inherited_video_audit and (not args.diagnostic_simplefb or args.diagnostic_gop_step or not args.diagnostic_gop_query or args.value_source!='off' or args.gop_backend_selftest):
        parser.error('inherited video audit requires SimpleFb query step0, value off, no RAM self-test')
    if args.inherited_video_entry_control and not args.inherited_video_audit:
        parser.error('video entry control requires inherited video audit profile')
    if args.inherited_video_terminal and (not args.inherited_video_audit or args.diagnostic_gop_query!=1 or args.dispatch_completion_fallback):
        parser.error('video terminal requires video audit/query1 and no dispatcher completion fallback')
    if args.inherited_video_topology_query and (not args.inherited_video_terminal or args.inherited_video_entry_control):
        parser.error('topology status query requires symmetric video audit and excludes constant control')
    if args.native_route_blend and not args.native_route_layout:
        parser.error('blend classifier requires layout')
    if args.native_route_layout and (not args.native_route_detail or args.native_route_staging):
        parser.error('layout classifier requires detail and excludes staging')
    if args.native_route_staging and not args.native_route_detail:
        parser.error('staging classifier requires route detail')
    if args.native_route_detail and not args.native_route_audit:
        parser.error('route detail requires native route audit')
    if args.native_route_audit and (not args.inherited_video_terminal or args.native_observe_query or args.native_observe_control or args.native_mask_field or args.inherited_video_entry_control or args.inherited_video_topology_query or args.inherited_video_reason_target!='off'):
        parser.error('native route requires video terminal and excludes other selectors')
    if args.native_mask_field and (args.native_observe_query!=1 or args.native_observe_control):
        parser.error('native mask requires native query1 and excludes constant control')
    if args.native_observe_control and not args.native_observe_query:
        parser.error('native control requires native observation query')
    if args.native_observe_query and (not args.inherited_video_terminal or args.inherited_video_entry_control or args.inherited_video_topology_query or args.inherited_video_reason_target!='off'):
        parser.error('native observation requires symmetric video terminal and excludes other classifiers/control')
    os.environ['ENUMA_NATIVE_OBSERVE_QUERY']=str(args.native_observe_query)
    reason_target={'off':0,'counter-enable':1,'stability':2,'interface':3,'refresh':4}[args.inherited_video_reason_target]
    if bool(reason_target)!=bool(args.inherited_video_reason_query) or (reason_target and (not args.inherited_video_terminal or args.inherited_video_entry_control or args.inherited_video_topology_query)):
        parser.error('first-reason target/query requires symmetric video audit and excludes topology/control')
    if args.diagnostic_pixel_preview != (args.diagnostic_gop_step == 13) or (args.diagnostic_pixel_preview and args.diagnostic_gop_query != 1):
        parser.error('Pixel preview requires exactly step13/query1 and its explicit flag; hardware approval is separate')
    if args.diagnostic_gop_query and not (args.diagnostic_simplefb and args.stop_stage == 13 and args.stop_delay in (2, 20) and args.detail_point == 16 and args.dispatch_guid_only and (args.dispatch_completion_fallback or args.inherited_video_terminal) and args.dispatch_guid and args.dispatch_guid.lower() == 'dcfd1e6d-788d-4ffc-8e1b-ca2f75651a92'):
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
    # Binary write: Path.write_text() translates '\n' to '\r\n' on Windows, and
    # git then reads the object path as 'objects\r' and refuses every update-ref.
    (info/'alternates').write_bytes((objects.as_posix()+'\n').encode())
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
    if args.inherited_video_audit:
        os.environ['ENUMA_INHERITED_VIDEO_AUDIT']='1'
    else:
        os.environ.pop('ENUMA_INHERITED_VIDEO_AUDIT',None)
    if args.inherited_video_entry_control:
        os.environ['ENUMA_INHERITED_VIDEO_ENTRY_CONTROL']='1'
    else:
        os.environ.pop('ENUMA_INHERITED_VIDEO_ENTRY_CONTROL',None)
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
    board['firmware']['defines']['ENUMA_RAMOOPS_CANARY']='1' if args.ramoops_canary_nonce else '0'
    board['firmware']['defines']['ENUMA_RAMOOPS_MAP']='1' if (args.ramoops_canary_nonce or args.ramoops_map_control or args.ramoops_read_header or args.ramoops_warm_control) else '0'
    board['firmware']['defines']['ENUMA_RAMOOPS_WARM_CONTROL']='1' if args.ramoops_warm_control else '0'
    board['firmware']['defines']['ENUMA_RAMOOPS_READ_HEADER']='1' if args.ramoops_read_header else '0'
    for i in range(4):
        word = int(args.ramoops_canary_nonce[i*8:i*8+8], 16) if args.ramoops_canary_nonce else 0
        board['firmware']['defines']['ENUMA_RAMOOPS_NONCE_'+str(i)]=str(word)+'U'
    board['firmware']['defines']['ENUMA_GOP_SELFTEST']='1' if args.gop_backend_selftest else '0'
    board['firmware']['defines']['ENUMA_NATIVE_ROUTE_BLEND']='1' if args.native_route_blend else '0'
    board['firmware']['defines']['ENUMA_NATIVE_ROUTE_LAYOUT']='1' if args.native_route_layout else '0'
    board['firmware']['defines']['ENUMA_NATIVE_ROUTE_STAGING']='1' if args.native_route_staging else '0'
    board['firmware']['defines']['ENUMA_NATIVE_ROUTE_DETAIL']='1' if args.native_route_detail else '0'
    board['firmware']['defines']['ENUMA_NATIVE_ROUTE_AUDIT']='1' if args.native_route_audit else '0'
    board['firmware']['defines']['ENUMA_NATIVE_MASK_FIELD']=str(args.native_mask_field)
    board['firmware']['defines']['ENUMA_NATIVE_CONTROL']='1' if args.native_observe_control else '0'
    board['firmware']['defines']['ENUMA_NATIVE_OBSERVE_QUERY']=str(args.native_observe_query)
    board['firmware']['defines']['ENUMA_VIDEO_TERMINAL']='1' if args.inherited_video_terminal else '0'
    board['firmware']['defines']['ENUMA_VIDEO_TOPOLOGY_QUERY']=str(args.inherited_video_topology_query)
    board['firmware']['defines']['ENUMA_VIDEO_REASON_TARGET']=str(reason_target)
    board['firmware']['defines']['ENUMA_VIDEO_REASON_QUERY']=str(args.inherited_video_reason_query)
    board['firmware']['defines']['ENUMA_RAM_PAYLOAD'] = '1' if args.ram_payload else '0'
    if args.ram_payload:
        os.environ['ENUMA_RAM_PAYLOAD'] = str(args.ram_payload.resolve())
    else:
        os.environ.pop('ENUMA_RAM_PAYLOAD', None)
    board['firmware']['defines'].update(ENUMA_DIAG_STAGE=str(args.stop_stage),
        ENUMA_DIAG_DELAY_US=str(args.stop_delay*1000000 if args.stop_delay != 20 else 2000000),
        ENUMA_DIAG_DELAY_FROM_DISPLAY='1' if args.stop_delay == 20 else '0',
        ENUMA_DIAG_DISPLAY_VALUE_INDEX=str(args.diagnostic_gop_step & 7),
        ENUMA_DIAG_DISPATCH_ORDINAL=str(args.dispatch_ordinal),
        ENUMA_DIAG_DETAIL_POINT=str(args.detail_point),ENUMA_DIAG_DISABLE_UFS='1' if (args.diagnostic_without_ufs or args.diagnostic_simplefb) else '0',
        ENUMA_DIAG_SIMPLEFB='1' if args.diagnostic_simplefb else '0',
        ENUMA_DIAG_GOP_QUERY=str(args.diagnostic_gop_query),
        ENUMA_DIAG_GOP_STEP=str(args.diagnostic_gop_step),
        ENUMA_DIAG_PIXEL_PREVIEW='1' if args.diagnostic_pixel_preview else '0',
        ENUMA_VALUE_SOURCE=str({'off':0,'constant':1,'register':2,'raw':3}[args.value_source]),
        ENUMA_VALUE_CODE=str(args.value_code), ENUMA_VALUE_INDEX=str(args.value_index),
        ENUMA_VALUE_SHIFT=str(args.value_shift),
        ENUMA_DIAG_GUID_ONLY='1' if args.dispatch_guid_only else '0',
        ENUMA_DIAG_DISPATCH_FALLBACK='1' if args.dispatch_completion_fallback else '0', **guid_defines)
    ctx=SimpleNamespace(board=board,args=SimpleNamespace(skip_preflight=False),tree=tree,
                        logs=WORK/'logs',work=WORK/'context',jobs=args.jobs)
    fd=uefi/board['firmware']['fd'].format(target='DEBUG')
    report={'scope':'firmware-only invocation of the official backend; no device boot or enuma kernel',
        'complete':False,'fd':str(fd.relative_to(ROOT)),'uefi_source_commit':pin['commit'],
        'gop_backend_selftest_ram_only':args.gop_backend_selftest,
        'inherited_video_audit_read_only':args.inherited_video_audit,
        'inherited_video_entry_control':args.inherited_video_entry_control,
        'inherited_video_terminal':args.inherited_video_terminal,
        'native_route_blend':args.native_route_blend,'native_route_layout':args.native_route_layout,'native_route_staging':args.native_route_staging,'native_route_detail':args.native_route_detail,'native_route_audit':args.native_route_audit,'native_observe_query':args.native_observe_query,'native_observe_control':args.native_observe_control,'native_mask_field':args.native_mask_field,
        'inherited_video_topology_query':args.inherited_video_topology_query,
        'inherited_video_reason_target':reason_target,'inherited_video_reason_query':args.inherited_video_reason_query,
        'ramoops_canary_nonce':args.ramoops_canary_nonce,
        'ramoops_map_control':args.ramoops_map_control,'ramoops_read_header':args.ramoops_read_header,'ramoops_warm_control':args.ramoops_warm_control,
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
        'diagnostic_simplefb_metadata_only':args.diagnostic_simplefb and not args.diagnostic_pixel_preview and not args.inherited_video_audit,
        'diagnostic_pixel_preview':args.diagnostic_pixel_preview,
        'hardware_review_required':'R-017-WARM' if args.ramoops_warm_control else 'R-017-READ' if args.ramoops_read_header else 'R-017-MAP' if args.ramoops_map_control else ('R-017' if args.ramoops_canary_nonce else ('R-005' if args.diagnostic_pixel_preview else None)),
        'diagnostic_gop_query':args.diagnostic_gop_query,
        'diagnostic_gop_step':args.diagnostic_gop_step,
        'value_probe':{'source':args.value_source,'constant_code':args.value_code,
                       'register_index':args.value_index,'shift':args.value_shift,
                       'extra_base_seconds':0,'seconds_per_code':3},
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
            if args.inherited_video_audit:
                staged=uefi/'Silicon/Silicium/SiliciumPkg/Drivers/SimpleFbDxe'
                entry='EnumaNativeObserve.c' if args.native_observe_query else ('EnumaInheritedVideoEntryControl.c' if args.inherited_video_entry_control else 'EnumaInheritedVideoAudit.c')
                assert (staged/'SimpleFb.c').read_bytes()==(SOURCE/'diagnostics'/entry).read_bytes()
                for name in ('InheritedScanout.h','Decode.c','Capture.c','VideoCapture.c','Topology.h','Topology.c','Evidence.h','Evidence.c','VideoResult.h','NativeObserve.h','NativeObserve.c','NativeMaskResult.h','NativeRouteResult.h'):
                    assert (staged/'InheritedVideo'/name).read_bytes()==(SOURCE/'Platforms/Xiaomi/enumaPkg/Library/InheritedScanoutLib'/name).read_bytes()
                report['inherited_video_compiled_contract']={
                    'entry_source_sha256':hashlib.sha256((staged/'SimpleFb.c').read_bytes()).hexdigest(),
                    'all_staged_sources_equal':True,'maximum_read_callbacks':0 if (args.inherited_video_entry_control or args.native_observe_control) else (26 if args.native_observe_query else 330),'maximum_wait_calls':0 if (args.inherited_video_entry_control or args.native_observe_control) else 2,
                    'entry_constant_marker':(1 if args.native_observe_query==1 else 5) if args.native_observe_control else (1 if args.inherited_video_entry_control else None),
                    'pixel_access':False,'protocol_published':False}
            if args.value_source != 'off':
                command=(module/'GNUmakefile').read_text()
                for name in ('ENUMA_VALUE_SOURCE','ENUMA_VALUE_CODE','ENUMA_VALUE_INDEX','ENUMA_VALUE_SHIFT'):
                    expected=board['firmware']['defines'][name]
                    if re.findall(r'-D'+name+r'=([^\s]+)',command) != [expected]:
                        raise ValueError('value define absent from actual compiler flags: '+name)
                report['value_probe_compiled_contract']={
                    'compiler_makefile_sha256':hashlib.sha256(command.encode()).hexdigest(),
                    'source_sha256':hashlib.sha256((uefi/'Silicon/Silicium/SiliciumPkg/Drivers/SimpleFbDxe/SimpleFb.c').read_bytes()).hexdigest(),
                    'header_sha256':hashlib.sha256((uefi/'Silicon/Silicium/SiliciumPkg/Drivers/SimpleFbDxe/EnumaValueProbe.h').read_bytes()).hexdigest(),
                    'actual_defines_checked':True}
        report['checks']=[{'backend':'unixnu.firmware.build','exit_code':0,'log':str((ctx.logs/'firmware.log').relative_to(ROOT))}]
    except Exception as error:
        report['complete']=False;report['error']=str(error);raise
    finally:
        (ROOT/'reports/enuma-firmware-build.json').write_text(json.dumps(report,indent=2)+'\n')
    print('PASS: official firmware backend and patch reversion; FD '+report['fd_sha256'])


if __name__=='__main__':main()
