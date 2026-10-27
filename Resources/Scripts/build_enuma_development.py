"""Isolated complete development FD build. No deployment or release qualification.

This deliberately separate entry takes an exported, hash-bound working-tree
snapshot, not a clean-commit claim. Historical source locks/reports and staging
are inputs only. All patch application and metadata derivatives stay in a new
output directory. The normal published-source adapter is unchanged.
"""
import argparse
import hashlib
import importlib.util
import json
import os
import re
from pathlib import Path
import shutil
import subprocess
import sys
from types import SimpleNamespace


def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def save(p, value): p.write_text(json.dumps(value, indent=2)+'\n')
def git(p, *args):
    return subprocess.check_output(['git', '-c', 'safe.directory='+str(p), '-C', str(p), *args], text=True).strip()
def load(p, name):
    spec=importlib.util.spec_from_file_location(name,p)
    module=importlib.util.module_from_spec(spec); spec.loader.exec_module(module)
    return module


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--device-root',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--dependency-root',type=Path,required=True,
                        help='separate fixed Mu nested dependency checkouts under device out/')
    parser.add_argument('--profile',choices=['stage10','attributed-log','diagnostic-log','menu-window','full-menu','deferred-menu','oem-control','oem-mmu-only'],default='stage10')
    parser.add_argument('--oem-profile-pair',type=Path,help='explicit verified pair; OEM profiles remain stage10 compile-only')
    parser.add_argument('--direct-menu',action='store_true',help='select real UniXNU BDS application for offline stage10 link checking; early stop remains')
    parser.add_argument('--jobs',type=int,default=4)
    args=parser.parse_args()
    is_oem=args.profile in ('oem-control','oem-mmu-only')
    if is_oem != (args.oem_profile_pair is not None):
        parser.error('OEM profiles require a pair; other profiles forbid it')
    if args.direct_menu and args.profile!='stage10':
        parser.error('--direct-menu is separate from frozen log experiment profiles')
    root=args.device_root.resolve(); out=args.output.resolve()
    out.relative_to(root/'out'); out.mkdir(parents=True,exist_ok=False)
    source=root/'firmware'; unixnu=root/'out/sources/UniXNU'
    report={'scope':'complete development FD; never deployment qualification',
            'complete':False,'deployment_ready':False,'hardware_execution':False}
    try:
        print('Checking fixed dependencies and approved UniXNU patch without modifying sources',flush=True)
        # Fixed dependency checks are read-only. Never call qualify(), which
        # reverses/reapplies the upstream patch in its original working tree.
        upstream=json.loads((root/'config/sources.lock.json').read_text())['sources']['UniXNU']
        if git(unixnu,'rev-parse','HEAD')!=upstream['commit']: raise ValueError('UniXNU revision mismatch')
        for name,wanted in upstream['gitlinks'].items():
            p=unixnu/name
            if git(p,'rev-parse','HEAD')!=wanted or git(p,'status','--porcelain','--ignore-submodules=all'):
                raise ValueError('UniXNU module changed: '+name)
        for name,wanted in upstream['file_sha256'].items():
            if sha(unixnu/name)!=wanted: raise ValueError('UniXNU input changed: '+name)
        # Reconstruct only the approved patched files in disposable staging.
        expected=out/'upstream-check'; expected.mkdir()
        permitted=[]
        for patch in upstream.get('patches',[]):
            p=root/patch['path']
            if sha(p)!=patch['sha256']: raise ValueError('upstream patch changed')
            for line in p.read_text().splitlines():
                if line.startswith('+++ b/'):
                    name=line[6:]; permitted.append(name)
                    target=expected/name; target.parent.mkdir(parents=True,exist_ok=True)
                    target.write_bytes(subprocess.check_output(['git','-C',str(unixnu),'show','HEAD:'+name]))
        subprocess.run(['git','init','-q',str(expected)],check=True)
        for patch in upstream.get('patches',[]):
            subprocess.run(['git','apply',str(root/patch['path'])],cwd=expected,check=True)
        if sorted(git(unixnu,'diff','--name-only','HEAD').splitlines())!=sorted(set(permitted)):
            raise ValueError('unexpected UniXNU tracked changes')
        if any((expected/n).read_bytes()!=(unixnu/n).read_bytes() for n in permitted):
            raise ValueError('UniXNU patch effect mismatch')
        # No Git commit or source permission changes. Export tracked files plus
        # explicit nonignored new files; gitlinks are dependencies, not files.
        snapshot=out/'source'; snapshot.mkdir()
        print('Exporting current firmware source snapshot',flush=True)
        names=subprocess.check_output(['git','-C',str(source),'ls-files','--cached','--others','--exclude-standard','-z']).decode().split('\0')
        original={}
        for name in sorted(set(names)):
            p=source/name
            if not name or not p.is_file(): continue
            if p.is_symlink(): raise ValueError('unexpected source symlink: '+name)
            target=snapshot/name; target.parent.mkdir(parents=True,exist_ok=True)
            shutil.copyfile(p,target); original[name]=sha(p)
        base=git(source,'rev-parse','HEAD')
        # Requalification metadata is a visibly named derivative in out/.
        # Preserve historical records as originals, record every new binding.
        derivatives={}
        for filename in ('migration.json','extensions.json'):
            p=snapshot/'Resources/Enuma'/filename; before=p.read_bytes()
            (p.parent/(filename+'.historical')).write_bytes(before)
            data=json.loads(before)
            if filename=='migration.json':
                for item in data['files'].values(): item['sha256']=sha(snapshot/item['destination'])
            else:
                for name in list(data['files']): data['files'][name]=sha(snapshot/name)
                for name in original:
                    if name.startswith(('diagnostics/','Platforms/','Silicon/Silicium/SiliciumPkg/Library/BootManagerLib/')):
                        data['files'][name]=sha(snapshot/name)
            data['development_derivative']=True
            save(p,data); derivatives[filename]={'original_sha256':hashlib.sha256(before).hexdigest(),'derivative_sha256':sha(p)}
        vendor=root/'out/firmware-vendor'
        oem_binding=None
        if is_oem:
            pair=args.oem_profile_pair.resolve(); pair.relative_to(root/'out')
            oem_helper=load(snapshot/'Resources/Scripts/enuma_oem_profile.py','oem_profile')
            vendor,oem_binding=oem_helper.bind(args.profile,pair,snapshot,out)
            save(out/'oem-profile-binding.json',oem_binding)
            derivatives['vendor.json']={'original_sha256':oem_binding['original_vendor_lock_sha256'],
                                        'derivative_sha256':oem_binding['derivative_vendor_lock_sha256']}
        files={str(p.relative_to(snapshot)):sha(p) for p in snapshot.rglob('*') if p.is_file()}
        manifest={'scope':'uncommitted development snapshot, not base-commit binaries','base_commit':base,
                  'origin':str(source),'original_files':original,'snapshot_files':files,'metadata_derivatives':derivatives}
        save(out/'source-manifest.json',manifest)
        print('Source manifest bound; staging fresh Mu archive',flush=True)
        # Official firmware backend, fresh archive staging only. No old cache
        # copies, no restore_mu_baseline, no source-lock refresh or downloads.
        backend=load(snapshot/'Resources/Scripts/build_enuma_firmware.py','development_backend')
        backend.WORK=out/'work'
        lock=json.loads((root/'config/firmware-build.lock.json').read_text())
        backend.WORK.mkdir()
        for name,wanted in lock['archive_sha256'].items():
            p=root/'out/firmware-build'/name
            if sha(p)!=wanted: raise ValueError('archive hash mismatch: '+name)
            shutil.copyfile(p,backend.WORK/name)
        backend.stage_archives(lock)
        uefi=backend.WORK/'uefi'; core=backend.WORK/'Mu_Basecore'
        # Archive has no Git metadata. Stop discovery at this disposable tree;
        # a Windows parent worktree gitfile is not resolvable by WSL Git.
        subprocess.run(['git','init','-q',str(core)],check=True)
        for name in ('BootShim','Silicon','Resources/MuPatches'):
            shutil.copytree(snapshot/name,uefi/name,dirs_exist_ok=True)
        for name in ('Resources/Configs/enuma.toml','Resources/Scripts/mkbootimg.py'):
            p=uefi/name;p.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(snapshot/name,p)
        # Base Git metadata is solely the timestamp source required by the
        # official backend. The manifest, never HEAD, identifies actual inputs.
        subprocess.run(['git','init','-q',str(uefi)],check=True)
        objects=Path(git(source,'rev-parse','--absolute-git-dir'))/'objects'
        info=uefi/'.git/objects/info';info.mkdir(parents=True,exist_ok=True)
        (info/'alternates').write_bytes((str(objects)+'\n').encode())
        subprocess.run(['git','-C',str(uefi),'update-ref','refs/heads/timestamp-base',base],check=True)
        subprocess.run(['git','-C',str(uefi),'symbolic-ref','HEAD','refs/heads/timestamp-base'],check=True)
        dependency_root=args.dependency_root.resolve();dependency_root.relative_to(root/'out')
        dependency_bindings={}
        for name in ('BaseTools/Source/C/BrotliCompress/brotli','MdeModulePkg/Library/BrotliCustomDecompressLib/brotli',
                     'MdePkg/Library/BaseFdtLib/libfdt','MdePkg/Library/MipiSysTLib/mipisyst','SecurityPkg/DeviceSecurity/SpdmLib/libspdm'):
            revision=git(source/'Mu_Basecore','ls-tree','HEAD',name).split()[2]
            dependency=dependency_root/Path(name).name
            if git(dependency,'rev-parse','HEAD')!=revision or git(dependency,'status','--porcelain'):
                raise ValueError('nested Mu dependency changed: '+name)
            dependency_bindings[name]={'revision':revision,'path':str(dependency)}
            shutil.copytree(dependency,core/name,dirs_exist_ok=True,
                            ignore=shutil.ignore_patterns('.git','unit_test','os_stub','doc','samples'))
        save(out/'dependency-bindings.json',dependency_bindings)
        print('Fixed nested Mu dependencies staged',flush=True)
        tree=out/'tree';tree.mkdir()
        (tree/'package').symlink_to(unixnu/'package',target_is_directory=True)
        (tree/'drivers').mkdir();(tree/'drivers/DarwinBoot').symlink_to(unixnu/'drivers/DarwinBoot',target_is_directory=True)
        if sys.version_info<(3,11):
            import tomli
            sys.modules['tomllib']=tomli
        import tomllib
        board=tomllib.loads((root/'board.toml').read_text())
        if board['board']['example'] is not True: raise ValueError('example guard changed')
        # Build/link the new backend without a physical activation path. Stop
        # before BDS; this image is compile qualification, not a menu trial.
        defs=board['firmware']['defines']
        # The existing diagnostic options patch forwards each of these DSC
        # macros to C. Define every unused selector as zero, not an unresolved
        # $(NAME); the effective complete map is recorded in the build report.
        options=(snapshot/'patches/dispatch-detail-options.patch').read_text()
        for name in re.findall(r'\$\((ENUMA_[A-Z0-9_]+)\)',options): defs.setdefault(name,'0')
        defs.update(ENUMA_DIAG_STAGE='10',ENUMA_DIAG_SIMPLEFB='1',ENUMA_DIAG_DISABLE_UFS='1',
                    ENUMA_BOOT_JOURNAL='1',ENUMA_BOOT_LOG_REPLAY='0',ENUMA_BOOT_LOG_HOLD_US='2000000',
                    ENUMA_RAM_PAYLOAD='0',ENUMA_GOP_SELFTEST='1')
        defs['ENUMA_DIRECT_MENU']='1' if args.direct_menu else '0'
        if is_oem: oem_helper.defines(args.profile,defs)
        if args.profile in ('attributed-log','diagnostic-log','menu-window','full-menu','deferred-menu'):
            # Frozen recipe is read-only provenance, not authorization for this derivative.
            recipe=root/'out/inherited-gop/qualified-dxe-log-8s-8680ea7/firmware-build.json'
            if sha(recipe)!='1fae735c5bbf07e69a117f3296c07c4b74d5737a4d426611e7256cfe0fc1e644':
                raise ValueError('frozen recipe changed')
            defs.clear();defs.update(json.loads(recipe.read_text())['firmware_configuration']['defines'])
            defs['ENUMA_BOOT_LOG_REPLAY']='3' if args.profile=='diagnostic-log' else '2'
            defs['ENUMA_PROBE_JOURNAL_REPLAY']='1' if args.profile=='diagnostic-log' else '0'
            if args.profile in ('menu-window','full-menu','deferred-menu'):
                defs.update(ENUMA_BOUNDED_MENU='1',ENUMA_BOOT_LOG_REPLAY='0',ENUMA_NATIVE_ROUTE_CPU_TEXT='0')
                if args.profile in ('full-menu','deferred-menu'):defs['ENUMA_FULL_MENU']='1'
                if args.profile=='deferred-menu':defs['ENUMA_DEFERRED_MENU']='1'
        env={k:v for k,v in os.environ.items() if not k.startswith('ENUMA_')}
        os.environ.clear();os.environ.update(env)
        if args.profile in ('attributed-log','diagnostic-log','menu-window','full-menu','deferred-menu'):
            os.environ['ENUMA_INHERITED_VIDEO_AUDIT']='1'
            os.environ['ENUMA_RAM_PAYLOAD']=str(root/'out/full-boot/payload/payload.img')
            if args.profile in ('menu-window','full-menu','deferred-menu'):os.environ['ENUMA_BOUNDED_MENU']='1'
        preflight=load(snapshot/'Resources/Scripts/enuma_preflight.py','development_preflight')
        preflight.prepare(root,snapshot,uefi,core,unixnu,source,vendor,out/'preflight.json')
        print('Preflight complete; compiling BaseTools and full enuma FD',flush=True)
        wrappers=core/'BaseTools/BinWrappers/PosixLike'
        # WSL DrvFS already exposes new files as executable. Never chmod.
        if not os.access(wrappers/'build',os.X_OK): raise ValueError('build wrapper not executable; no permission changes permitted')
        os.environ.update(PATH=str(wrappers)+os.pathsep+env['PATH'],PYTHONPATH=str(core/'BaseTools/Source/Python'),
                          PYTHON_COMMAND=sys.executable,CLANG_BIN=str(Path(shutil.which('clang')).parent)+'/',
                          SOURCE_DATE_EPOCH='1792540800',PYTHONDONTWRITEBYTECODE='1')
        sys.path.insert(0,str(unixnu/'scripts'))
        from unixnu import firmware
        ctx=SimpleNamespace(board=board,args=SimpleNamespace(skip_preflight=True),tree=tree,
                            logs=out/'logs',work=out/'context',jobs=args.jobs)
        fd=firmware.build(ctx,{},uefi,core)
        if any(sha(snapshot/n)!=wanted for n,wanted in files.items()): raise ValueError('snapshot changed during build')
        if any(sha(source/n)!=wanted for n,wanted in original.items()): raise ValueError('source changed during build')
        baseline=json.loads((snapshot/'Resources/Enuma/reset.json').read_text())['diagnostic_baseline_lf_sha256']
        if any(sha(core/n)!=wanted for n,wanted in baseline.items() if n.startswith(('MdeModulePkg/','UefiCpuPkg/'))):
            raise ValueError('core patches not reverted')
        report.update(profile=args.profile,direct_menu_selected=args.direct_menu,complete=True,fd=str(fd.relative_to(root)),fd_sha256=sha(fd),
                      source_manifest_sha256=sha(out/'source-manifest.json'),base_commit=base,
                      firmware_configuration=board['firmware'],diagnostic_omitted_oem_modules=(['UFSDxe'] if is_oem else ['CPRDxe','DisplayDxe','UFSDxe']),
                      oem_profile_binding=oem_binding,
                      diagnostic_simplefb_metadata_only=args.profile=='stage10',core_patch_reversion_checked=True,
                      gop_backend_selftest_ram_only=args.profile=='stage10',boot_journal={'enabled':True,'global_debug':False},
                      note=('OEM stage10 compile-only; stops before dispatch; no trace or deployment qualification' if is_oem else
                            'stage10 stops before BDS; selftest not auto-launched' if args.profile=='stage10' else
                            'opt-in transient 800x600 GOP; full-menu runs FV UniXNU application with private readonly config; no BDS/XNU/input; needs separate qualification' if args.profile in ('menu-window','full-menu','deferred-menu') else
                            args.profile+' bounded replay; new offline candidate, no execution approval'))
    except Exception as error:
        report['error']=str(error);raise
    finally:
        save(out/'build-result.json',report)
    print('PASS: complete development FD; not deployment ready')


if __name__=='__main__': main()
