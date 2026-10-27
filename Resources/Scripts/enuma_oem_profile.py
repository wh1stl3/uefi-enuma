"""Bind exact OEM variants in disposable development build staging only."""
import hashlib
import json
from pathlib import Path

PE = 'Binaries/enuma/DisplayDxe/DisplayDxe.efi'
ORIGINAL = '6c6351c3c770d4f698e5e22097021fe8d20d35a8d4181df05676a48c4347f425'
PATCHED = 'ed37a4e137cca2fcf4d5fa5718e18a319c2aa1b7a1cfab480693325bfbe2b5e3'
PROFILES = ('oem-control', 'oem-mmu-only')


def sha(data): return hashlib.sha256(data).hexdigest()


def read_tree(path):
    files = {}
    for p in sorted(path.rglob('*')):
        if p.is_symlink(): raise ValueError('vendor symlink refused')
        if p.is_file(): files[p.relative_to(path).as_posix()] = p.read_bytes()
    return files


def bind(profile, pair, snapshot, out):
    if profile not in PROFILES: raise ValueError('unknown OEM profile')
    pair, snapshot, out = map(lambda p: Path(p).resolve(), (pair, snapshot, out))
    snapshot.relative_to(out)
    if pair == out or out in pair.parents or pair in out.parents:
        raise ValueError('pair and build output overlap')
    lock_path = snapshot/'Resources/Enuma/vendor.json'
    historical = lock_path.read_bytes()
    lock = json.loads(historical)
    control = read_tree(pair/'oem-control/vendor')
    experiment = read_tree(pair/'oem-mmu-only/vendor')
    hashes = lambda f: {n: sha(b) for n, b in f.items()}
    if hashes(control) != lock['files'] or sha(control.get(PE, b'')) != ORIGINAL:
        raise ValueError('control differs from firmware vendor lock')
    expected = dict(control)
    if control[PE][0x1994:0x1998] != bytes.fromhex('e1031f32'):
        raise ValueError('unexpected patch instruction')
    expected[PE] = control[PE][:0x1994]+bytes.fromhex('01008052')+control[PE][0x1998:]
    if sha(expected[PE]) != PATCHED or hashes(experiment) != hashes(expected):
        raise ValueError('MMU variant differs outside approved instruction')
    files = control if profile == 'oem-control' else experiment
    vendor = out/'oem-vendor'
    vendor.mkdir(exist_ok=False)
    for n, b in files.items():
        p = vendor/n; p.parent.mkdir(parents=True, exist_ok=True)
        with p.open('xb') as stream: stream.write(b)
    if hashes(read_tree(vendor)) != hashes(files): raise ValueError('vendor copy readback failed')
    derivative = dict(lock, files=hashes(files), development_derivative=True, oem_profile=profile)
    with (lock_path.parent/'vendor.json.historical').open('xb') as stream: stream.write(historical)
    lock_path.write_text(json.dumps(derivative, indent=2)+'\n')
    return vendor, dict(profile=profile, expected_pe_sha256=sha(files[PE]),
                        vendor_files=hashes(files), original_vendor_lock_sha256=sha(historical),
                        derivative_vendor_lock_sha256=sha(lock_path.read_bytes()),
                        stop_before_dispatch=True, trace_attached=False, deployment_ready=False)


def defines(profile, values):
    if profile not in PROFILES: raise ValueError('unknown OEM profile')
    values.update(ENUMA_DIAG_STAGE='10', ENUMA_DIAG_SIMPLEFB='0', ENUMA_GOP_SELFTEST='0',
                  ENUMA_DIAG_DISABLE_UFS='1', ENUMA_DIRECT_MENU='0', ENUMA_BOOT_LOG_REPLAY='0',
                  ENUMA_BOUNDED_MENU='0', ENUMA_FULL_MENU='0', ENUMA_DEFERRED_MENU='0')
