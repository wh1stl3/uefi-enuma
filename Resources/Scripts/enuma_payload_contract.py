"""Bind a staged RAM payload driver to exact local inputs; no device I/O."""
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import zlib


def generate(device, fat, target):
    fat = fat.resolve()
    if not fat.is_relative_to((device / 'out').resolve()):
        raise ValueError('FAT must be inside device out/')
    raw = fat.read_bytes()
    sha = lambda b: hashlib.sha256(b).hexdigest()
    expected = json.loads((device / 'reports/enuma-full-payload.json').read_text())
    if len(raw) != expected['payload_bytes'] or sha(raw) != expected['payload_sha256']:
        raise ValueError('FAT differs from the reviewed system-input report')
    sectors = struct.unpack_from('<H', raw, 19)[0] or struct.unpack_from('<I', raw, 32)[0]
    if not 1 << 20 <= len(raw) <= 128 << 20 or raw[510:512] != b'\x55\xaa' or struct.unpack_from('<H', raw, 11)[0] != 512 or sectors * 512 != len(raw):
        raise ValueError('expected bounded exact FAT superfloppy')
    vendor = (device / 'out/device-tests/20261022T201404/vendor_boot_a.bin').read_bytes()
    if sha(vendor) != '3da202d0c9a5b618b61cd1923e87810536c5f70c9da7875de80075290641daba':
        raise ValueError('vendor_boot snapshot changed')
    if vendor[:8] != b'VNDRBOOT' or struct.unpack_from('<I', vendor, 8)[0] != 3:
        raise ValueError('requires stock vendor boot v3')
    page = struct.unpack_from('<I', vendor, 12)[0]
    size = struct.unpack_from('<I', vendor, 24)[0]
    header = struct.unpack_from('<I', vendor, 2096)[0]
    if page != 4096 or header != 2112 or not 0 < size < 1 << 20:
        raise ValueError('unsupported vendor prefix geometry')
    start = (header + page - 1) // page * page
    prefix = vendor[start:start + size]
    if len(prefix) != size:
        raise ValueError('vendor prefix truncated')
    entries = []
    for path, key in [('Payloads/kernel.macho', 'kernel.macho'), ('Payloads/kernel.macho.adt', 'Payloads/kernel.macho.adt'),
                      ('Payloads/kernel.macho.trustcache', 'kernel.macho.trustcache'), ('unixnu.toml', 'unixnu.toml')]:
        blob = subprocess.check_output(['mtype', '-i', str(fat), '::/EFI/UniXNU/' + path])
        if len(blob) < 64 or sha(blob) != expected['fat_readback'][key]:
            raise ValueError('FAT file differs: ' + path)
        entries.append(dict(path=path, bytes=len(blob), sha256=sha(blob), head_crc32=zlib.crc32(blob[:64])))
    values = {'ENUMA_PREFIX_BYTES': str(size) + 'ULL', 'ENUMA_PREFIX_CRC': hex(zlib.crc32(prefix)) + 'U',
              'ENUMA_FAT_BYTES': str(len(raw)) + 'ULL', 'ENUMA_FAT_CRC': hex(zlib.crc32(raw)) + 'U',
              'ENUMA_FILE_SIZES': '{' + ','.join(str(e['bytes']) + 'ULL' for e in entries) + '}',
              'ENUMA_FILE_HEAD_CRCS': '{' + ','.join(hex(e['head_crc32']) + 'U' for e in entries) + '}'}
    target.write_text('// Generated local transport integrity contract; not authentication.\n' +
                      '\n'.join('#define ' + k + ' ' + v for k, v in values.items()) + '\n')
    report = dict(scope='offline exact-input contract; runtime prefix/layout/mapping not yet observed',
                  fat_sha256=sha(raw), fat_bytes=len(raw), prefix_bytes=size, prefix_sha256=sha(prefix),
                  prefix_crc32=zlib.crc32(prefix), fat_crc32=zlib.crc32(raw), files=entries,
                  header_sha256=sha(target.read_bytes()), integrity='CRC32 for transport errors only; host SHA256 binds source inputs, no authentication claim',
                  new_memory_mapping=False, hardware_tested=False)
    (device / 'reports/enuma-ram-payload-contract.json').write_text(json.dumps(report, indent=2) + '\n')
    return report
