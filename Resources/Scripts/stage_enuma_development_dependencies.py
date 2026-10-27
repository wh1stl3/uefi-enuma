"""Fetch exact Mu gitlink inputs into a new device out/ directory only."""
import argparse
import json
from pathlib import Path
import subprocess


def git(path,*args):
    return subprocess.check_output(['git','-C',str(path),*args],text=True).strip()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--device-root',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();root=args.device_root.resolve();out=args.output.resolve()
    out.relative_to(root/'out');out.mkdir(parents=True,exist_ok=False)
    core=root/'firmware/Mu_Basecore'
    pin=json.loads((root/'config/firmware-build.lock.json').read_text())['reference_gitlinks']['Mu_Basecore']
    if git(core,'rev-parse','HEAD')!=pin:raise ValueError('Mu base revision mismatch')
    inputs={
        'BaseTools/Source/C/BrotliCompress/brotli':'https://github.com/google/brotli.git',
        'MdePkg/Library/BaseFdtLib/libfdt':'https://github.com/devicetree-org/pylibfdt.git',
        'MdePkg/Library/MipiSysTLib/mipisyst':'https://github.com/MIPI-Alliance/public-mipi-sys-t.git',
        'SecurityPkg/DeviceSecurity/SpdmLib/libspdm':'https://github.com/DMTF/libspdm.git',
    }
    result={'mu_commit':pin,'complete':False,'dependencies':{}}
    try:
        for name,url in inputs.items():
            mode,kind,revision,_=git(core,'ls-tree','HEAD',name).split()
            if (mode,kind)!=('160000','commit'):raise ValueError('not a gitlink')
            target=out/Path(name).name
            subprocess.run(['git','init','-q',str(target)],check=True)
            subprocess.run(['git','-C',str(target),'fetch','--depth','1',url,revision],check=True,timeout=180)
            if git(target,'rev-parse','FETCH_HEAD')!=revision:raise ValueError('fetched revision mismatch')
            subprocess.run(['git','-C',str(target),'checkout','--detach','FETCH_HEAD'],check=True)
            if git(target,'status','--porcelain'):raise ValueError('dependency checkout dirty')
            result['dependencies'][name]={'commit':revision,'repository':url,'path':str(target)}
        result['complete']=True
    finally:
        (out/'receipt.json').write_text(json.dumps(result,indent=2)+'\n')


if __name__=='__main__':main()
